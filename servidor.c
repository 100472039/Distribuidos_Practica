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
    // Crear el archivo lista_archivos.txt dentro del directorio del usuario
    char archivo_path[256];
    // snprintf(archivo_path, sizeof(512), "%s/lista_archivos.txt", path);
    strcpy(archivo_path, path);
    strcat(archivo_path, "/lista_archivos.txt");
    FILE *archivo = fopen(archivo_path, "w");
    if (archivo == NULL) {
        perror("Error al crear el archivo lista_archivos.txt");
        return;
    }
    fclose(archivo);
}

int eliminar_usuario(char *path, const char *usuario) {
    // Abre el archivo usuarios.txt en modo de lectura y escritura
    FILE *archivo = fopen(path, "r+");
    if (archivo == NULL) {
        perror("Error al abrir el archivo usuarios.txt");
        return -1;
    }

    // Abre un archivo temporal para escribir los usuarios que no se van a eliminar
    FILE *temporal = fopen("temporal.txt", "w");
    if (temporal == NULL) {
        perror("Error al abrir el archivo temporal.txt");
        fclose(archivo);
        return -1;
    }

    bool encontrado = false;
    char linea[256];
    char linea_aux[256];
    char *token;

    // Lee el archivo usuarios.txt línea por línea
    while (fgets(linea, sizeof(linea), archivo)) {
        // Elimina el carácter de nueva línea del nombre de usuario leído
        linea[strcspn(linea, "\n")] = '\0';
        strcpy(linea_aux, linea);
        token = strtok(linea_aux, " ");

        // Comprueba si el nombre de usuario coincide
        if (strcmp(token, usuario) == 0) {
            encontrado = true;
        } else {
            // Escribe el nombre de usuario en el archivo temporal si no se va a eliminar
            fprintf(temporal, "%s\n", linea);
        }
    }

    // Cierra ambos archivos
    fclose(archivo);
    fclose(temporal);

    // Elimina el archivo original
    if (remove(path) != 0) {
        perror("Error al eliminar el archivo usuarios.txt");
        return -1;
    }

    // Renombra el archivo temporal.txt como el original
    if (rename("temporal.txt", path) != 0) {
        perror("Error al renombrar el archivo temporal.txt");
        return -1;
    }

    if (encontrado) {
        printf("Usuario \"%s\" eliminado de %s", usuario, path);
        return 0;
    } else {
        printf("Usuario \"%s\" no encontrado en %s", usuario, path);
        return -1;
    }
}

int comprobar_usuario(char *path, char *usuario) {
    // 0: no encontrado, 1: encontrado
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
        linea[strcspn(linea, "\n")] = '\0';
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

int recibir_mensaje(int s_local, char *mensaje_recibido){
    int recv_status;
    int valor_ascii = 1;
    char valor_convertido;
    // char *mensaje_recibido = (char *)malloc(256);
    int devolucion;
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
        mensaje_recibido[i] = valor_convertido;
        // printf("Valor i: %d", valor_ascii);
        // printf("Valor convertido: %c", valor_convertido);
        // printf("Mensaje recibido: %c", mensaje_recibido[i]);
        
    }
    printf("mensaje recibido: %s", mensaje_recibido);
    return 0;
}

int verificar_archivo_existente(const char *username, char *nombre_archivo) {
    // Crear una cadena para contener la ruta del directorio del usuario
    char directorio_usuario[256];
    snprintf(directorio_usuario, sizeof(directorio_usuario), "usuarios/%s/", username);

    // Abrir el directorio del usuario
    DIR *dir = opendir(directorio_usuario);
    if (dir == NULL) {
        perror("Error al abrir el directorio del usuario");
        return -1; // Error al abrir el directorio
    }

    struct dirent *entry;

    // Recorrer los archivos dentro del directorio
    while ((entry = readdir(dir)) != NULL) {
        // Ignorar los directorios "." y ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        printf("Archivo: %s\n", entry->d_name);
        fflush(stdout);

        printf("Archivo 2: %s\n", nombre_archivo);
        fflush(stdout);

        // Quitar la extensión ".txt" del nombre del archivo
        char nombre_sin_extension[256];
        strncpy(nombre_sin_extension, entry->d_name, sizeof(nombre_sin_extension));
        char *punto = strstr(nombre_sin_extension, ".txt");
        if (punto != NULL) {
            *punto = '\0'; // Coloca un carácter nulo para truncar la cadena en el punto
        }

        printf("nombre sin extension: %s\n", nombre_sin_extension);
        fflush(stdout);
        // Verificar si el archivo tiene el mismo nombre que el buscado
        if (strcmp(nombre_sin_extension, nombre_archivo) == 0) {
            closedir(dir);
            return 1; // El archivo existe
        }
    }

    // Cerrar el directorio
    closedir(dir);

    // El archivo no se encontró
    return 0;
}

int borrar_archivo(const char *username, const char *nombre_archivo) {
    // Crear una cadena para contener la ruta del archivo a borrar
    char ruta_archivo[256];
    snprintf(ruta_archivo, sizeof(ruta_archivo), "usuarios/%s/%s.txt", username, nombre_archivo);

    // Intentar borrar el archivo
    if (remove(ruta_archivo) == 0) {
        printf("El archivo \"%s\" ha sido borrado para el usuario \"%s\".\n", nombre_archivo, username);
        return 0; // Borrado exitoso
    } else {
        perror("Error al borrar el archivo");
        return -1; // Error al borrar el archivo
    }
}

int contar_numero_archivos(char *path){
    DIR *dir;
    struct dirent *ent;
    int file_count = 0; // Contador de archivos

    // Abrir el directorio
    if ((dir = opendir(path)) != NULL) {
        // Contar el número de archivos en el directorio
        while ((ent = readdir(dir)) != NULL) {
            // Ignorar las entradas especiales "." y ".."
            if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                file_count++;
            }
        }

        // Reiniciar el puntero del directorio
        rewinddir(dir);
    }
    return file_count;
}

int tratar_peticion(int *s) {
    // Declaración de las variables que se van a utilizar
    int s_local;
    char *op_recibido = (char *)malloc(256);
    char *valor_total = (char *)malloc(256);
    int devolucion;
    
    pthread_mutex_lock(&mutex_mensaje);
    s_local = (* (int *)s);
    busy = false;
    pthread_cond_signal(&cond_mensaje);
    pthread_mutex_unlock(&mutex_mensaje);

    // Recibir operación
    recibir_mensaje(s_local, op_recibido);

    if (strcmp("REGISTER", op_recibido) == 0){
        int usuario_existente;
        printf("Realizar registro");
        
        // Recibir usuario
        recibir_mensaje(s_local, valor_total);

        usuario_existente = comprobar_usuario("usuarios.txt", valor_total);

        if (usuario_existente == 1) {
            // Enviar mensaje al cliente de que el nombre de usuario ya está en uso
            printf("El usuario coincide");
            
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
        printf("Realizar baja de registro");
        
        recibir_mensaje(s_local, valor_total);
        

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

            int eliminar = eliminar_usuario("usuarios.txt", valor_total);
            if (eliminar == -1){
                devolucion = 51;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                close(s_local); 
                return -1;
            }
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
        char *port = (char *)malloc(256);
        char *ip = (char *)malloc(256);
        printf("Realizar conexión");
        
        recibir_mensaje(s_local, valor_total);

        recibir_mensaje(s_local, port);

        recibir_mensaje(s_local, ip);
        

        int usuario_existente;
        usuario_existente = comprobar_usuario("usuarios.txt", valor_total);

        // Si el usuario no existe
        if (usuario_existente == 0){
            devolucion = 49;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;
        }            

        int connected = comprobar_usuario("conectados.txt", valor_total);
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

    if(strcmp("DISCONNECT", op_recibido) == 0){
        printf("Realizar desconexión");
        
        recibir_mensaje(s_local, valor_total);

        int connected = comprobar_usuario("conectados.txt", valor_total);
        if (connected == 1) {
            int eliminar = eliminar_usuario("conectados.txt", valor_total);
            if (eliminar == -1){
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
        char *fileName = (char *)malloc(256);
        char *fileContent = (char *)malloc(256);
        printf("Realizar publicación");
        
        recibir_mensaje(s_local, valor_total);
        
        recibir_mensaje(s_local, fileName);
        
        recibir_mensaje(s_local, fileContent);
        
        int usuario_existente = comprobar_usuario("usuarios.txt", valor_total);
        if (usuario_existente == 0) {
                devolucion = 49;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                return -1;
        }
        
        int connected = comprobar_usuario("conectados.txt", valor_total);
        if (connected == 0) {
            devolucion = 50;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;
        }

        
        int archivo_existente = verificar_archivo_existente(valor_total, fileName);
        if (archivo_existente == 1) {
            devolucion = 51;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;

        }else if(archivo_existente == 0){
            int crear = crear_archivo_descripcion(valor_total, fileName, fileContent);
            if (crear == -1){
                devolucion = 52;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                close(s_local); 
                return -1;
            }
            
            devolucion = 48;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
        }
    }

    if (strcmp("DELETE", op_recibido) == 0){
        char *fileName = (char *)malloc(256);
        printf("Realizar publicación");
        
        recibir_mensaje(s_local, valor_total);
        
        recibir_mensaje(s_local, fileName);

        int usuario_existente = comprobar_usuario("usuarios.txt", valor_total);
        if (usuario_existente == 0) {
                devolucion = 49;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                return -1;
        }
        
        int connected = comprobar_usuario("conectados.txt", valor_total);
        if (connected == 0) {
            devolucion = 50;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;
        }

        int archivo_existente = verificar_archivo_existente(valor_total, fileName);
        if (archivo_existente == 0) {
            devolucion = 51;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;

        }else if(archivo_existente == 1){
            int borrar = borrar_archivo(valor_total, fileName);
            if (borrar == -1){
                devolucion = 52;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                close(s_local); 
                return -1;
            }
            devolucion = 48;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
        }
    }

    if(strcmp("LIST_USERS", op_recibido) == 0){

        recibir_mensaje(s_local, valor_total);
        printf("Listar usuarios para %s", valor_total);

        int usuario_existente = comprobar_usuario("usuarios.txt", valor_total);
        if (usuario_existente == 0) {
                devolucion = 49;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                return -1;
        }

        int connected = comprobar_usuario("conectados.txt", valor_total);
        if (connected == 0) {
            devolucion = 50;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;
        }

        FILE *fp1 = fopen("conectados.txt", "r");
        if (fp1 == NULL) {
            perror("Error al abrir el archivo\n");
            return -1;
        }

        // Si todo ha ido bien, se enviará el mensaje de OK
        devolucion = 48;
        sendMessage(s_local, (char *)&devolucion, sizeof(char));
        sleep(0.1);

        char linea1[256];
        char linea[256];
        int n_lineas = 0;
        char *token;

        // Contar número de líneas
        while (fgets(linea1, sizeof(linea1), fp1) != NULL) {
            n_lineas++;
        }
        devolucion = n_lineas;
        printf("Número de líneas: %d", devolucion);
        sendMessage(s_local, (char *)&devolucion, sizeof(char));
        sleep(0.1);

        FILE *fp = fopen("conectados.txt", "r");
        if (fp == NULL) {
            perror("Error al abrir el archivo\n");
            return -1;
        }

        // Leer el archivo línea por línea
        while (fgets(linea, sizeof(linea), fp) != NULL) {
            // Dividir la línea en tokens usando el espacio como delimitador
            linea[strcspn(linea, "\n")] = '\0';
            token = strtok(linea, " ");
            // Imprimir cada token
            while (token != NULL) {
                printf("Token: %s", token);
                for (int i = 0; token[i] != '\0'; i++) {
                    // printf("Caracter %d: %c", i+1, token[i]);
                    sendMessage(s_local, (char *)&token[i], sizeof(char));
                    sleep(0.1);
                }
                devolucion = 0;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                sleep(0.1);
                token = strtok(NULL, " "); // Obtener el siguiente token
            }
            // for (int i = 0; linea[i] != '\0'; i++){
            //     printf("El caracter '%c' tiene el valor ASCII %d", linea[i], (int)linea[i]);
            //     devolucion = (int)linea[i];
            //     sendMessage(s_local, (char *)&devolucion, sizeof(char));
                // printf("%d", linea[i])
            // }
        }

        // Cerrar el archivo
        fclose(fp);
    }

    if(strcmp("LIST_CONTENT", op_recibido) == 0){
        char *usuario_deseado = (char *)malloc(256);

        recibir_mensaje(s_local, valor_total);

        // Comprobar que el usuario actual existe
        int usuario_existente = comprobar_usuario("usuarios.txt", valor_total);
        if (usuario_existente == 0) {
                devolucion = 49;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                return -1;
        }

        // Comprobar que el usuario actual está conectado
        int connected = comprobar_usuario("conectados.txt", valor_total);
        if (connected == 0) {
            devolucion = 50;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1; 
        }

        recibir_mensaje(s_local, usuario_deseado);
        printf("Listar contenido de %s para %s", usuario_deseado, valor_total);

        // Comprobar que el usuario al que se quiere acceder existe
        usuario_existente = comprobar_usuario("usuarios.txt", usuario_deseado);
        if (usuario_existente == 0) {
                devolucion = 49;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                return -1;
        }

        devolucion = 48;
        sendMessage(s_local, (char *)&devolucion, sizeof(char));
        sleep(0.1);

        char path[128];
        strcpy(path, "usuarios/");
        strcat(path, usuario_deseado);
        printf("Path: %s", path);
        int file_count = contar_numero_archivos(path);

        // Imprimir el número de archivos
        printf("Número de archivos en el directorio: %d", file_count);
        devolucion = file_count;
        sendMessage(s_local, (char *)&devolucion, sizeof(char));
        sleep(0.1);
    
        DIR *dir;
        struct dirent *ent;
        char filename[256];

        // Abrir el directorio
        if ((dir = opendir(path)) != NULL) {
            // Leer cada entrada en el directorio
            while ((ent = readdir(dir)) != NULL) {
                // Ignorar las entradas especiales "." y ".."
                if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                    // Imprimir el nombre del archivo
                    strcpy(filename, ent->d_name);
                    printf("Nombre del archivo: %s\n", filename);

                    // Abrir el archivo
                    FILE *archivo;
                    char filename_path[256]; // Tamaño suficiente para la ruta completa
                    // snprintf(filename, sizeof(filename), "%s/%s", path, ent->d_name);
                    strcpy(filename_path, path);
                    strcat(filename_path, "/");
                    strcat(filename_path, ent->d_name);
                    printf("Filename: %s", filename_path);
                    archivo = fopen(filename_path, "r");
                    
                    // Verificar si se pudo abrir el archivo
                    if (archivo == NULL) {
                        perror("Error al abrir el archivo");
                        return EXIT_FAILURE;
                    }

                    // Leer y mostrar el contenido del archivo
                    char description[256];
                    if (fgets(description, sizeof(description), archivo) == NULL) {
                        perror("No se pudo leer la descripción");
                        fclose(archivo);
                        continue; // Saltar a la siguiente iteración
                    }
                    printf("Descripción: %s\n", description);
                    // Cerrar el archivo
                    fclose(archivo);
                    char message[516];
                    sprintf(message, "\t%s \"%s\"", filename, description);
                    sendMessage(s_local, message, strlen(message));
                }
            }
            closedir(dir);
            
        } else {
            // Si no se pudo abrir el directorio
            perror("Error al abrir el directorio");
            return EXIT_FAILURE;
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