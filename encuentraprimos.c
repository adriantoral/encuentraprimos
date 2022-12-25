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
	return 0;
}

void informar(char *texto, int verboso)
{

}

void imprimirJerarquiaProc(int pidraiz, int pidservidor, int *pidhijos, int numhijos)
{

}

int contarLineas()
{
	return 0;
}

void alarmHandler(int signo)
{

}

int main(int argc, char **argv)
{
	key_t key;

	T_MSG_BUFFER message;

	int pid, msgid, mypid, parentpid, pidservidor;

	int numhijos = 2; // Cambiar lectura por consola
	if ((pid = fork()) == 0) // Creacion del servidor (SERVER)
	{
		pid = getpid();
		pidservidor = pid;

		if (( key = ftok( "/tmp", 'C' )) == -1 )
		{
			perror("Fallo al pedir ftok");
			exit( 1 );
		}
		printf("Server: System V IPC key = %u\n", key);

		if (( msgid = msgget( key, IPC_CREAT | 0666 )) == -1 )
		{
			perror( "Fallo al crear la cola de mensajes" );
			exit( 2 );
		}
		printf("Server: Message queue id = %u\n", msgid );

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

		if (mypid != pidservidor)
		{
			message.msg_type = COD_ESTOY_AQUI;
			sprintf(message.msg_text, "%d", mypid);
			msgsnd(msgid, &message, sizeof(message), IPC_NOWAIT);

			sleep(60);
			exit(0);
		}

		else
		{
			for (int j=0; j<numhijos; j++)
			{
				msgrcv(msgid, &message, sizeof(message), 0, 0);
				sscanf(message.msg_text, "%d", &pid);
				printf("\nMe ha enviado un mensaje el hijo %d\n", pid);
			}

			sleep(60);
			msgctl(msgid, IPC_RMID, NULL);
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
