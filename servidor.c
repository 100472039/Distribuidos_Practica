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
#include <sys/stat.h>

#include "send-recv.h"
#include "send-recv.c"

// mutex y variables condicionales para proteger la copia del mensaje
pthread_mutex_t mutex_mensaje;
pthread_mutex_t mutex_lista1, mutex_lista2;
int busy = true;
pthread_cond_t cond_mensaje;
int iniciado;

void crear_directorio_para_usuario(const char *username) {
    // Crear una cadena para contener la ruta del directorio del usuario
    char path[256];
    snprintf(path, sizeof(path), "usuarios/%s", username); // Puedes modificar esto según la estructura de tu directorio

    // Crear el directorio
    if (mkdir(path, 0700) == 0) {
        printf("Directorio creado para el usuario %s\n", username);
    } else {
        perror("Error al crear el directorio");
    }
}

void eliminar_usuario(const char *usuario) {
    // Abre el archivo usuarios.txt en modo de lectura y escritura
    FILE *archivo = fopen("usuarios.txt", "r+");
    if (archivo == NULL) {
        perror("Error al abrir el archivo usuarios.txt");
        return;
    }

    // Abre un archivo temporal para escribir los usuarios que no se van a eliminar
    FILE *temporal = fopen("temporal.txt", "w");
    if (temporal == NULL) {
        perror("Error al abrir el archivo temporal.txt");
        fclose(archivo);
        return;
    }

    char nombre_usuario[256];
    bool encontrado = false;

    // Lee el archivo usuarios.txt línea por línea
    while (fgets(nombre_usuario, sizeof(nombre_usuario), archivo)) {
        // Elimina el carácter de nueva línea del nombre de usuario leído
        nombre_usuario[strcspn(nombre_usuario, "\n")] = '\0';

        // Comprueba si el nombre de usuario coincide
        if (strcmp(nombre_usuario, usuario) == 0) {
            encontrado = true;
        } else {
            // Escribe el nombre de usuario en el archivo temporal si no se va a eliminar
            fprintf(temporal, "%s\n", nombre_usuario);
        }
    }

    // Cierra ambos archivos
    fclose(archivo);
    fclose(temporal);

    // Elimina el archivo usuarios.txt original
    if (remove("usuarios.txt") != 0) {
        perror("Error al eliminar el archivo usuarios.txt");
        return;
    }

    // Renombra el archivo temporal.txt como usuarios.txt
    if (rename("temporal.txt", "usuarios.txt") != 0) {
        perror("Error al renombrar el archivo temporal.txt");
        return;
    }

    if (encontrado) {
        printf("Usuario \"%s\" eliminado de usuarios.txt\n", usuario);
    } else {
        printf("Usuario \"%s\" no encontrado en usuarios.txt\n", usuario);
    }
}

int tratar_peticion(int *s) {
    // Declaración de las variables que se van a utilizar
    int recv_status;
    int s_local;
    char op_recibido;
    char num_valores;
    char *valor_total = (char *)malloc(256);
    int devolucion;
    
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
    op_recibido = op_recibido - 48;
    printf("op_recibido: %d\n", op_recibido);
    fflush(stdout);

    if (op_recibido == 0){
        int valor_ascii = 1;
        char valor_convertido;
        printf("Realizar registro\n");
        fflush(stdout);
        for (int i = 0; valor_ascii != 0; i++){
            recv_status = recvMessage(s_local, (char *)&valor_ascii, sizeof(char));
            if (recv_status == -1) {
                perror("Error en recepcion\n");
                devolucion = 50;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                close(s_local);
                return -1;
            }
            valor_convertido = valor_ascii;
            valor_total[i] = valor_convertido;
			// printf("Valor i: %d\n", valor_ascii);
			// printf("Valor convertido: %c\n", valor_convertido);
			// printf("Valor total: %c\n", valor_total[i]);
			fflush(stdout);
        }
        printf("Valor total: %s\n", valor_total);
        fflush(stdout);

        // Abrir el archivo de texto para lectura
        FILE *fp = fopen("usuarios.txt", "r");
        if (fp == NULL) {
            perror("Error al abrir el archivo\n");
            devolucion = 50;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            close(s_local);
            return -1;
        }

        char nombre_usuario[256];
        bool usuario_existente = false;

        // Leer el archivo línea por línea y buscar el nombre de usuario
        while (fgets(nombre_usuario, sizeof(nombre_usuario), fp) != NULL) {
            // Eliminar el carácter de nueva línea del nombre de usuario leído
            nombre_usuario[strcspn(nombre_usuario, "\n")] = '\0';

			
            // Comprobar si el nombre de usuario coincide
            if (strcmp(nombre_usuario, valor_total) == 0) {
                usuario_existente = true;
                break;
            }
        }

        // Cerrar el archivo
        fclose(fp);

        if (usuario_existente) {
            // Enviar mensaje al cliente de que el nombre de usuario ya está en uso
            printf("coincide\n");
            fflush(stdout);
            devolucion = 49;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
			return -1;
        } else {
            // Abrir el archivo de texto para escritura (agregar al final)
            fp = fopen("usuarios.txt", "a");
            if (fp == NULL) {
                perror("Error al abrir el archivo\n");
                devolucion = 50;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                close(s_local);
                return -1;
            }

            // Escribir el nombre de usuario en el archivo
            fprintf(fp, "%s\n", valor_total);

            // Crear directorio para el usuario
            crear_directorio_para_usuario(valor_total);
            // Cerrar el archivo
            fclose(fp);

            // Enviar mensaje al cliente de que el registro fue exitoso
            devolucion = 48;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
        }
    }

    if (op_recibido == 1){
        int valor_ascii;
        char valor_convertido;
        printf("Realizar registro\n");
        fflush(stdout);
        recv_status = recvMessage(s_local, (char *)&num_valores, sizeof(char));
        if (recv_status == -1) {
            perror("Error en recepcion\n");
            devolucion = 50;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            close(s_local);
            return -1;
        }
        num_valores = num_valores - 48;
        for (int i = 0; valor_ascii != 0; i++){
            recv_status = recvMessage(s_local, (char *)&valor_ascii, sizeof(char));
            if (recv_status == -1) {
                perror("Error en recepcion\n");
                devolucion = 50;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                close(s_local);
                return -1;
            }
            valor_convertido = valor_ascii;
            valor_total[i] = valor_convertido;
        }
        printf("Valor total: %s\n", valor_total);
        fflush(stdout);

        FILE *fp = fopen("usuarios.txt", "r+");
        if (fp == NULL) {
            perror("Error al abrir el archivo\n");
            devolucion = 50;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            close(s_local);
            return -1;
        }

        char nombre_usuario[256];
        bool usuario_existente = false;


        // Leer el archivo línea por línea y buscar el nombre de usuario
        while (fgets(nombre_usuario, sizeof(nombre_usuario), fp) != NULL) {
            // Eliminar el carácter de nueva línea del nombre de usuario leído
            nombre_usuario[strcspn(nombre_usuario, "\n")] = '\0';

			
            // Comprobar si el nombre de usuario coincide
            if (strcmp(nombre_usuario, valor_total) == 0) {
                usuario_existente = true;
                break;
            }
        }

        // Cerrar el archivo
        fclose(fp);

        if (usuario_existente) {
            // Eliminar directorio del usuario
            char path[256];
            snprintf(path, sizeof(path), "usuarios/%s", valor_total);
            if (remove(path) == 0) {
                printf("Directorio eliminado para el usuario %s\n", valor_total);
            } else {
                perror("Error al eliminar el directorio");
            }

            eliminar_usuario(valor_total);
            // Enviar mensaje al cliente de que el registro fue exitoso
            devolucion = 48;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
        } else {
            // Enviar mensaje al cliente de que el nombre de usuario ya está en uso
            devolucion = 49;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;
        }
    }

    return 0;
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
		printf("Ha llegado\n");
		fflush(stdout);

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