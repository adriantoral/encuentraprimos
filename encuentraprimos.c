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
	// El numero tiene que ser mayor que 1 (float remainder by 0 exception)
	if (numero < 2) return 0;
	if ((numero % 2) == 0 && (numero % (numero / 2)) == 0) return 0;
	return 1;
}

void informar(char *texto, int verboso)
{
	if (verboso)
	{
		// Mostrar en pantalla el resultado
		printf("%s\n", texto);
	}

	// Escribir el resultado en el fichero
	FILE *primos = fopen(NOMBRE_FICH, "a");
	fputs(texto, primos);
	fclose(primos);
}

void imprimirJerarquiaProc(int pidraiz, int pidservidor, int *pidhijos, int numhijos)
{
	printf("\nRAIZ     SERV     CALC\n%5d    %5d    %5d\n", pidraiz, pidservidor, pidhijos[0]);
	for (int i=1; i<numhijos; i++) printf("-----    -----    %5d\n", pidhijos[i]);
	printf("\n");
}

int contarLineas()
{
	return 0;
}

int cuentasegundos = 0;

void alarmHandler(int signo)
{
	cuentasegundos += 5;
	alarm(INTERVALO_TIMER);
}

int main(int argc, char **argv)
{
	key_t key;

	T_MSG_BUFFER message;

	int pid, msgid, mypid, parentpid, pidservidor, *pidhijos, verboso = 1, numeroprimos = 0;

	FILE *cuentaprimos = fopen(NOMBRE_FICH_CUENTA, "a");

	int numhijos = 2; // Cambiar lectura por consola
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
				if ((pid = fork()) == 0)
				{
					parentpid = getppid();
					mypid = getpid();
				}
			}
		}

		if (mypid != pidservidor) // Si es calculador
		{
			// Cambia el tipo de codigo para el mensaje
			message.msg_type = COD_ESTOY_AQUI;

			// Establece el mensaje a enviar
			sprintf(message.msg_text, "%d", mypid);

			// Envia el mensaje a la cola
			msgsnd(msgid, &message, sizeof(message), IPC_NOWAIT);

			// Espera 10 segundos antes de recibir los limites
			sleep(10);

			// Lee el primer mensaje con el codigo de limites
			msgrcv(msgid, &message, sizeof(message), COD_LIMITES, 0);

			// Extrae los limites del mensaje leido
			int rangoInicial, rangoFinal;
			sscanf(message.msg_text, "%d %d", &rangoInicial, &rangoFinal);
			printf("[%d] Recivido el rango: %d-%d\n", mypid, rangoInicial, rangoFinal);

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

			// Termina el hijo
			exit(0);
		}

		else // Si es el servidor
		{
			for (int j=0; j<numhijos; j++)
			{
				// Crea la memoria dinamica para los pid
				pidhijos = realloc(pidhijos, sizeof(int) * (j + 1));

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
				sprintf(message.msg_text, "%d %d", 0, 10);

				// Muestra en pantalla el los limites enviados
				printf("[%d] Enviado el rango: %s\n", mypid, message.msg_text);

				// Envia los limites en el mensaje
				msgsnd(msgid, &message, sizeof(message), IPC_NOWAIT);
			}

			// Mostrar en pantalla la jerarquia de procesos
			imprimirJerarquiaProc(getppid(), pidservidor, pidhijos, numhijos);

			// Espera 60 segundos antes de terminar
			sleep(20);

			struct msqid_ds quedanMensajes;
			do // Leer los mensajes mientras quede alguno en la cola
			{
				int numeroPrimo = 0, pidPrimo = 0;

				// Lee los mensajes de la cola
				msgrcv(msgid, &message, sizeof(message), 0, 0);
				if (message.msg_type == COD_RESULTADOS)
				{
					char resultado[100], resultado2[100];

					int numeroPrimo = 0, pidPrimo = 0;

					// Leer los datos si el codigo es el de resultados
					sscanf(message.msg_text, "%d %d", &pidPrimo, &numeroPrimo);

					// Crear la cadena de resultado
					sprintf(resultado2, "[%d] Encontrado el numero primo: %d\n", pidPrimo, numeroPrimo);

					informar(resultado2, verboso);

					if (!(numeroprimos % 5))
					{
						sprintf(resultado, "%d\n", numeroPrimo);

						fputs(resultado, cuentaprimos);
					}
				}

				else if (message.msg_type == COD_FIN)
				{
					// Leer los datos si el codigo es el de fin
					sscanf(message.msg_text, "%d", &pidPrimo);
					printf("[%d] Proceso terminado\n", pidPrimo);
				}
			}
			while(!msgctl(msgid, IPC_STAT, &quedanMensajes));

			// Mostrar en pantalla los segundos pasados
			printf("Tiempo transcurrido: %d\n", cuentasegundos);

			// Elimina la cola
			msgctl(msgid, IPC_RMID, NULL);

			// Cierra los ficheros
			fclose(cuentaprimos);

			// Libera la memoria dinamica
			free(pidhijos);
		}
	}

	else
	{
		alarm(INTERVALO_TIMER);
		signal(SIGALRM, alarmHandler);
		for (;;) sleep(1);
	}

	return 0;
}
