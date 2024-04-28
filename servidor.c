#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "send-recv.h"

// mutex y variables condicionales para proteger la copia del mensaje
pthread_mutex_t mutex_mensaje;
pthread_mutex_t mutex_lista1, mutex_lista2;
int busy = true;
pthread_cond_t cond_mensaje;
int iniciado;

int tratar_peticion(int * s){
	// Declaración de las variables que se van a utilizar
	int recv_status;
	int32_t resultado;	
    int s_local;
	char op_recibido;
	int key_recibido;
	char value1_recibido[256];
	int N_value2_recibido = 0;

	// Copia la dirección del cliente a local
    pthread_mutex_lock(&mutex_mensaje);
	s_local = (* (int *)s);
	busy = false;
	pthread_cond_signal(&cond_mensaje);
	pthread_mutex_unlock(&mutex_mensaje);

	// Recibe el operador del cliente
	recv_status = recvMessage(s_local, (char *)&op_recibido, sizeof(char));
	if (recv_status == -1) {
			perror("Error en recepcion\n");
			close(s_local);
			exit(-1);
	}
	printf("op_recibido: %d\n", op_recibido);
	fflush(stdout);
}

int main(int argc, char *argv[]){  
	// Declarar las variables para el socket y los hilos
	int sd_server, sd_client ;
	struct sockaddr_in server_addr,  client_addr;
	int opt = 1;
	socklen_t size;
	size = sizeof(client_addr);
    pthread_attr_t t_attr;  // Atributos de los hilos
    pthread_t thid;         // ID del hilo


    // abrir socket del server
    if ((sd_server = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        printf("SERVER: Error en el socket");
        return -1;
    }

	// socket options...
	if (setsockopt(sd_server, SOL_SOCKET, SO_REUSEADDR | SO_REUSEADDR, &opt, sizeof(opt)))
	{
		printf("SERVER: Error en las opciones del socket");
        return -1;
	}
	
	int puerto = atoi(argv[1]);
	int32_t netPuerto = (int32_t)puerto;
	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family      = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port        = htons(netPuerto);

	// bind + listen
	int err = bind(sd_server, (const struct sockaddr *)&server_addr, sizeof(server_addr));
	if (err == -1) {
		printf("Error en bind\n");
		return -1;
	}

    err = listen(sd_server, SOMAXCONN);
	if (err == -1) {
		printf("Error en listen\n");
		return -1;
	}

	// Inicializar mutex y variables condicionales
    pthread_mutex_init(&mutex_mensaje, NULL);
	pthread_mutex_init(&mutex_lista1, NULL);
    pthread_mutex_init(&mutex_lista2, NULL);
	pthread_cond_init(&cond_mensaje, NULL);
	pthread_attr_init(&t_attr);
    pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);

    // recibir del cliente
    while(1) {
		// Esperar connect del cliente
		sd_client = accept(sd_server, (struct sockaddr *) &client_addr, (socklen_t *)&size);
		if (sd_client == -1) {
			printf("Error en accept\n");
			return -1;
		}

		if (pthread_create(&thid, &t_attr, (void *)tratar_peticion, (void *)&sd_client)== 0) {
			// esperar a que el hijo copie el descriptor 
			pthread_mutex_lock(&mutex_mensaje);
			while (busy == true)
				pthread_cond_wait(&cond_mensaje, &mutex_mensaje);
			busy = true;
			pthread_mutex_unlock(&mutex_mensaje);
	 		}   
        }
	
	close(sd_server);
	
	// Destruir los mutex y las variables condicionales
	pthread_mutex_destroy(&mutex_mensaje);
	pthread_mutex_destroy(&mutex_lista1);
	pthread_mutex_destroy(&mutex_lista2);
	pthread_cond_destroy(&cond_mensaje);

	return 0;
}