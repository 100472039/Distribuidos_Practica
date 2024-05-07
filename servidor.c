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
#include <dirent.h>

#include "send-recv.h"
#include "send-recv.c"

#define printf(...) {printf(__VA_ARGS__); printf("\ns> "); fflush(stdout);}

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
        printf("Directorio creado para el usuario %s", username);
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
        printf("Usuario \"%s\" eliminado de usuarios.txt", usuario);
    } else {
        printf("Usuario \"%s\" no encontrado en usuarios.txt", usuario);
    }
}

int comprobar_usuario(char *path, char *usuario){
    // Abrir el archivo de texto para lectura
        FILE *fp = fopen(path, "r");
        if (fp == NULL) {
            perror("Error al abrir el archivo\n");
            return -1;
        }

        char nombre_usuario[256];

        // Leer el archivo línea por línea y buscar el nombre de usuario
        while (fgets(nombre_usuario, sizeof(nombre_usuario), fp) != NULL) {
            // Eliminar el carácter de nueva línea del nombre de usuario leído
            nombre_usuario[strcspn(nombre_usuario, "\n")] = '\0';

			
            // Comprobar si el nombre de usuario coincide
            if (strcmp(nombre_usuario, usuario) == 0) {
                return 1;
            }
        }
    
        // Cerrar el archivo
        fclose(fp);
        return 0;
}

int comprobar_usuario_conectado(char *path, char *usuario) {
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        perror("Error al abrir el archivo\n");
        return -1;
    }

    char linea[256];
    char *token;

    // Leer el archivo línea por línea
    while (fgets(linea, sizeof(linea), fp) != NULL) {
        // Dividir la línea en tokens usando el espacio como delimitador
        token = strtok(linea, " ");
        
        // Comprobar si el primer token (nombre de usuario) coincide
        if (strcmp(token, usuario) == 0) {
            // Si coincide, cerrar el archivo y devolver verdadero
            fclose(fp);
            return 1;
        }
    }

    // Cerrar el archivo
    fclose(fp);
    // Si no se encuentra el nombre de usuario, devolver falso
    return 0;
}

int crear_archivo_descripcion(const char *username, const char *nombre_archivo, const char *descripcion) {
    // Crear una cadena para contener la ruta del archivo del usuario
    char path[256];
    snprintf(path, sizeof(path), "usuarios/%s/%s.txt", username, nombre_archivo); 

    // Abrir el archivo para escritura
    FILE *archivo = fopen(path, "w");
    if (archivo == NULL) {
        perror("Error al crear el archivo");
        return -1;
    }

    // Escribir la descripción en el archivo
    if (fprintf(archivo, "%s", descripcion) < 0) {
        perror("Error al escribir en el archivo");
        fclose(archivo);
        return -1;
    }

    // Cerrar el archivo
    fclose(archivo);

    printf("Archivo \"%s\" creado para el usuario \"%s\" con la descripción \"%s\"\n", nombre_archivo, username, descripcion);
    
    return 0;
}



int escribir_usuario(char *path, char *usuario){
    FILE *fp = fopen(path, "a");
    if (fp == NULL) {
        perror("Error al abrir el archivo\n");
        return -1;
    }
    fprintf(fp, "%s\n", usuario);
    fclose(fp);
    return 0;
}

int escribir_usuario_ip_port(char *path, char *usuario, char *ip, char *puerto) {
    FILE *fp = fopen(path, "a");
    if (fp == NULL) {
        perror("Error al abrir el archivo\n");
        return -1;
    }
    fprintf(fp, "%s %s %s\n", usuario, ip, puerto);
    fclose(fp);
    return 0;
}

int tratar_peticion(int *s) {
    // Declaración de las variables que se van a utilizar
    int recv_status;
    int s_local;
    int valor_ascii = 1;
    char valor_convertido;
    char *op_recibido = (char *)malloc(256);
    char *valor_total = (char *)malloc(256);
    int devolucion;
    
    pthread_mutex_lock(&mutex_mensaje);
    s_local = (* (int *)s);
    busy = false;
    pthread_cond_signal(&cond_mensaje);
    pthread_mutex_unlock(&mutex_mensaje);

    // Recibe el operador del cliente
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
        op_recibido[i] = valor_convertido;
        // printf("Valor i: %d", valor_ascii);
        // printf("Valor convertido: %c", valor_convertido);
        // printf("Valor total: %c", valor_total[i]);
        
    }
    printf("op_recibido: %s", op_recibido);

    // recv_status = recvMessage(s_local, (char *)&op_recibido, sizeof(char));
    // if (recv_status == -1) {
    //     perror("Error en recepcion\n");
    //     close(s_local);
    //     exit(-1);
    // }
    // op_recibido = op_recibido - 48;
    // printf("op_recibido: %d", op_recibido);
    // 

    if (strcmp("REGISTER", op_recibido) == 0){
        valor_ascii = 1;
        int usuario_existente;
        printf("Realizar registro");
        
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

        
        printf("Valor total: %s", valor_total);
        

        usuario_existente = comprobar_usuario("usuarios.txt", valor_total);

        if (usuario_existente == 1) {
            // Enviar mensaje al cliente de que el nombre de usuario ya está en uso
            printf("coincide");
            
            devolucion = 49;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
			return -1;
        } else if (usuario_existente == 0){
            // Abrir el archivo de texto para escritura (agregar al final)
            int escribir = escribir_usuario("usuarios.txt", valor_total);
            if (escribir == -1) {
                devolucion = 50;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                close(s_local);
                return -1;
            }
            
            // Crear directorio para el usuario
            crear_directorio_para_usuario(valor_total);

            // Enviar mensaje al cliente de que el registro fue exitoso
            devolucion = 48;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
        }
    }

    if (strcmp("UNREGISTER", op_recibido) == 0){
        valor_ascii = 1;
        int valor_ascii = 1;
        char valor_convertido;
        printf("Realizar baja de registro");
        
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
        printf("Valor total: %s", valor_total);
        

        int usuario_existente = comprobar_usuario("usuarios.txt", valor_total);

        if (usuario_existente == 1) {
            // Eliminar directorio del usuario
            char path[256];
            snprintf(path, sizeof(path), "usuarios/%s", valor_total);
            if (remove(path) == 0) {
                printf("Directorio eliminado para el usuario %s", valor_total);
            } else {
                perror("Error al eliminar el directorio");
            }

            eliminar_usuario(valor_total);
            // Enviar mensaje al cliente de que el registro fue exitoso
            devolucion = 48;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
        } else if(usuario_existente == 0){
            // Enviar mensaje al cliente de que el nombre de usuario ya está en uso
            devolucion = 49;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;
        }
    }

        if(strcmp("CONNECT", op_recibido) == 0){
            valor_ascii = 1;
            char valor_convertido;
            char *port = (char *)malloc(256);
            char *ip = (char *)malloc(256);
            printf("Realizar conexión");
            
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
            printf("Valor total: %s", valor_total);

            valor_ascii = 1;
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
                port[i] = valor_convertido;
            }
            printf("puerto: %s", port);

            valor_ascii = 1;
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
                ip[i] = valor_convertido;
            }
            printf("ip: %s", ip);
            

            int usuario_existente;
            usuario_existente = comprobar_usuario("usuarios.txt", valor_total);

            // Si el usuario no existe
            if (usuario_existente == 0){
                devolucion = 49;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                return -1;
            }
            

            int connected = comprobar_usuario_conectado("conectados.txt", valor_total);
            if (connected == 0) {
                int escribir = escribir_usuario_ip_port("conectados.txt", valor_total, ip, port);
                if (escribir == -1){
                    devolucion = 51;
                    sendMessage(s_local, (char *)&devolucion, sizeof(char));
                    close(s_local); 
                    return -1;
                }
                devolucion = 48;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
            }else{
                devolucion = 50;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                return -1;           
            }
        }

    if(strcmp("PUBLISH", op_recibido) == 0){
        valor_ascii = 1;
        char valor_convertido;
        char *fileName = (char *)malloc(256);
        char *fileContent = (char *)malloc(256);
        printf("Realizar conexión");
        
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
        printf("Valor total: %s", valor_total);
        
        valor_ascii = 1;
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
            fileName[i] = valor_convertido;
        }
        printf("fileName: %s", fileName);
        
        valor_ascii = 1;
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
            fileContent[i] = valor_convertido;
        }
        printf("fileContent: %s", fileContent);
        
        int usuario_existente = comprobar_usuario("usuarios.txt", valor_total);
        if (usuario_existente == 0) {
                devolucion = 49;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                return -1;
        }
        
        int connected = comprobar_usuario_conectado("conectados.txt", valor_total);
        if (connected == 0) {
            devolucion = 50;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;
        }

        crear_archivo_descripcion(valor_total, fileName, fileContent);
        devolucion = 48;
        sendMessage(s_local, (char *)&devolucion, sizeof(char));

        
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
		printf("Error en bind");
		return -1;
	}

    err = listen(sd_server, SOMAXCONN);
	if (err == -1) {
		printf("Error en listen");
		return -1;
	}
    printf("s> init server 127.0.0.1: %d", puerto);

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
			printf("Error en accept");
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