/* Author : Adrian Toral */
/* Author : Miguel Gracia */
/* Codigo : Cola de mensajes entre procesos */
/* Fecha  : 15-12-2022 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LONGITUD_MSG 100           // Payload del mensaje
#define LONGITUD_MSG_ERR 200       // Mensajes de error por pantalla

// Codigos de exit por error
#define ERR_ENTRADA_ERRONEA 2
#define ERR_SEND 3
#define ERR_RECV 4
#define ERR_FSAL 5

#define NOMBRE_FICH "primos.txt"
#define NOMBRE_FICH_CUENTA "cuentaprimos.txt"
#define CADA_CUANTOS_ESCRIBO 5

// rango de busqueda, desde BASE a BASE+RANGO
#define BASE 800000000
#define RANGO 2000

// Intervalo del temporizador para RAIZ
#define INTERVALO_TIMER 5

// Codigos de mensaje para el campo mesg_type del tipo T_MESG_BUFFER
#define COD_ESTOY_AQUI 5           // Un calculador indica al SERVER que esta preparado
#define COD_LIMITES 4              // Mensaje del SERVER al calculador indicando los la­mites de operacion
#define COD_RESULTADOS 6           // Localizado un primo
#define COD_FIN 7                  // Final del procesamiento de un calculador

typedef struct
{
	long msg_type;
	char msg_text[LONGITUD_MSG];
}
T_MSG_BUFFER;

int comprobarSiEsPrimo(long int numero)
{
	for (int i=2; i<=(numero/2); i++)
		if (!(numero % i)) return 0;

	return 1;
}

void informar(char *texto, int verboso)
{
	// Mostrar en pantalla el resultado
	if (verboso) printf("%s", texto);
}

void imprimirJerarquiaProc(int pidraiz, int pidservidor, int *pidhijos, int numhijos)
{
	printf("\nRAIZ     SERV     CALC\n%5d    %5d    %5d\n", pidraiz, pidservidor, pidhijos[0]);
	for (int i=1; i<numhijos; i++) printf("-----    -----    %5d\n", pidhijos[i]);
	printf("\n");
}

int contarLineas()
{
	// Abre el fichero
	FILE *cuentaprimos = fopen(NOMBRE_FICH, "r");

	// Salir si no existe el fichero
	if (cuentaprimos == NULL) exit(ERR_FSAL);

	// Variable de control
	int numeroLineas = 0;

	// Incrementar en 1 la variable si es una nueva linea
	while(!feof(cuentaprimos))
		if(fgetc(cuentaprimos) == '\n')
			numeroLineas++;

	// Cerrar el fichero
	fclose(cuentaprimos);

	// Devolver el numero de lineas
	return numeroLineas;
}

int cuentasegundos = 0;

void alarmHandler(int signo)
{
	// Incrementar el contador
	cuentasegundos += INTERVALO_TIMER;

	// Recursividad de la funcion
	alarm(INTERVALO_TIMER);
}

int main(int argc, char *argv[])
{
	key_t key;

	T_MSG_BUFFER message;

	int numhijos = 2, verboso = 0;

	if (argc >= 3)
	{
		// Definir las variables por parametros
		numhijos = strtol(argv[1], NULL, 10);
		verboso = strtol(argv[2], NULL, 10);
	}

	int pid, msgid, mypid, parentpid, pidservidor, hijosterminados = 0, *pidhijos = NULL, numeroprimos = 0, rango = BASE, limite = (int)(RANGO / numhijos);

	FILE *cuentaprimos = fopen(NOMBRE_FICH_CUENTA, "w"), *primos = fopen(NOMBRE_FICH, "w");

	if (cuentaprimos == NULL || primos == NULL)
	{
		// Mostrar mensaje de error
		perror("Fallo al crear los ficheros de salida");

		// Salir del programa
		exit(ERR_FSAL);
	}

	if ((pid = fork()) == 0) // Creacion del servidor (SERVER)
	{
		// Variables de control de procesos
		pid = getpid();
		pidservidor = pid;
		mypid = pid;

		// Creacion de la cola
		if ((key = ftok( "/tmp", 'C')) == -1)
		{
			perror("Fallo al pedir ftok");
			exit(1);
		}
		printf("Server: System V IPC key = %u\n", key);

		if ((msgid = msgget( key, IPC_CREAT | 0666)) == -1)
		{
			perror("Fallo al crear la cola de mensajes");
			exit(2);
		}
		printf("Server: Message queue id = %u\n", msgid);

		// Creacion de los hijos
		for (int i=0; i<numhijos; i++)
		{
			if (pid > 0)
			{
				if (!(pid = fork()))
				{
					parentpid = getppid();
					mypid = getpid();
				}
			}
		}

		if (mypid != pidservidor) // Si es calculador
		{
			// Enviar el mensaje de que esta activo
			message.msg_type = COD_ESTOY_AQUI;
			sprintf(message.msg_text, "%d", mypid);
			msgsnd(msgid, &message, sizeof(message), IPC_NOWAIT);

			// Espera 10 segundos antes de recibir los limites
			//sleep(INTERVALO_TIMER);

			// Lee el primer mensaje con el codigo de limites
			msgrcv(msgid, &message, sizeof(message), COD_LIMITES, 0);

			// Extrae los limites del mensaje leido
			int rangoInicial, rangoFinal;
			sscanf(message.msg_text, "%d %d", &rangoInicial, &rangoFinal);
			printf("[%d] Recibido el rango: %d-%d\n", mypid, rangoInicial, rangoFinal);

			// Calcula si es primo
			for (int i=rangoInicial; i<=rangoFinal; i++)
				if (comprobarSiEsPrimo(i))
				{
					// Si encuentra el primo enviarlo a la cola
					message.msg_type = COD_RESULTADOS;
					sprintf(message.msg_text, "%d %d", mypid, i);
					msgsnd(msgid, &message, sizeof(message), IPC_NOWAIT);
				}

			// Mandar mensaje de fin
			message.msg_type = COD_FIN;
			sprintf(message.msg_text, "%d", mypid);
			msgsnd(msgid, &message, sizeof(message), IPC_NOWAIT);

			// Termina el calculador
			exit(0);
		}

		else // Si es el servidor
		{
			for (int j=0; j<numhijos; j++)
			{
				// Crea la memoria dinamica para los pid
				pidhijos = (int *)realloc(pidhijos, sizeof(int) * (j + 1));

				// Lee los mensajes de la cola con el codigo estoy aqui
				msgrcv(msgid, &message, sizeof(message), COD_ESTOY_AQUI, 0);
				sscanf(message.msg_text, "%d", &pid);

				// Guarda en memoria dinamica los pid
				pidhijos[j] = pid;

				// Muestra en pantalla el mensaje recibido
				printf("[%d] Me ha enviado un mensaje el hijo %d\n", mypid, pid);
			}

			for (int j=0; j<numhijos; j++)
			{
				// Cambia el tipo de codigo para el mensaje
				message.msg_type = COD_LIMITES;

				// Establece los limites para los calculadores
				sprintf(message.msg_text, "%d %d", rango, rango + limite);
				rango += limite;

				// Muestra en pantalla el los limites enviados
				printf("[%d] Enviado el rango: %s\n", mypid, message.msg_text);

				// Envia los limites en el mensaje
				msgsnd(msgid, &message, sizeof(message), IPC_NOWAIT);
			}

			// Mostrar en pantalla la jerarquia de procesos
			imprimirJerarquiaProc(getppid(), pidservidor, pidhijos, numhijos);

			// Espera 60 segundos antes de terminar
			//sleep(60);

			// Estructura para comprobar si quedan mensajes
			do // Se ejecuta mientras haya un calculador activo
			{
				// Variables de control
				int numeroPrimo = 0, pidPrimo = 0;

				// Lee los mensajes de la cola
				msgrcv(msgid, &message, sizeof(message), 0, 0);
				if (message.msg_type == COD_RESULTADOS)
				{
					// Variables de control
					char resultado[LONGITUD_MSG], resultado2[LONGITUD_MSG];

					// Leer los datos si el codigo es el de resultados
					sscanf(message.msg_text, "%d %d", &pidPrimo, &numeroPrimo);

					// Crear la cadena de resultado
					sprintf(resultado2, "[%d] Encontrado el numero primo: %d\n", pidPrimo, numeroPrimo);

					// Mostrar en pantalla los mensajes y guardarlos en el fichero
					informar(resultado2, verboso);

					// Guardar el contenido en el fichero
					fputs(resultado2, primos);

					// Si coincide que el numero de primos sea divisible por 5
					if (!(++numeroprimos % CADA_CUANTOS_ESCRIBO))
					{
						// Crear la cadena de resultado
						sprintf(resultado, "%3d %d\n",numeroprimos, numeroPrimo);

						// Guardar la cadena en el fichero
						fputs(resultado, cuentaprimos);
					}
				}

				else if (message.msg_type == COD_FIN)
				{
					// Termino un calculador
					hijosterminados++;

					// Leer los datos si el codigo es el de fin
					sscanf(message.msg_text, "%d", &pidPrimo);

					// Mostrar en pantalla la salida del proceso
					printf("[%d] Proceso terminado\n", pidPrimo);
				}

				else
				{
					// Meter el mensaje de nuevo a la cola
					msgsnd(msgid, &message, sizeof(message), IPC_NOWAIT);
				}
			}
			while (hijosterminados < numhijos); // Leer los mensajes mientras quede alguno en la cola

			// Elimina la cola
			msgctl(msgid, IPC_RMID, NULL);

			// Cierra los ficheros
			fclose(cuentaprimos);
			fclose(primos);

			// Libera la memoria dinamica
			free(pidhijos);

			// Finalizar el SERVER
			exit(0);
		}
	}

	else
	{
		// Manda una senal de inicio
		alarm(INTERVALO_TIMER);
		signal(SIGALRM, alarmHandler);

		// Espera al hijo (SERVER)
		wait(NULL);

		// Mostrar en pantalla los segundos pasados
		printf("Tiempo transcurrido: %d\n", cuentasegundos);
		printf("RESULTADO: %d primos detectados\n", contarLineas());
	}

	return 0;
}

