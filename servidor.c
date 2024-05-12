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
#include <signal.h>
#include "rpc_service.h"
#include "rpc_service_clnt.c"
#include "rpc_service_xdr.c"


#include "send-recv.h"
#include "send-recv.c"

// Esto es una redefinición de la función printf para que siempre muestre el formato de servidor
#define printf(...) {printf(__VA_ARGS__); printf("\ns> "); fflush(stdout);}

// mutex y variables condicionales para proteger la copia del mensaje
pthread_mutex_t mutex_mensaje;
pthread_mutex_t mutex_funciones;
int busy = true;
pthread_cond_t cond_mensaje;
int iniciado;

void crear_directorio_para_usuario(const char *username) {
    // Crear una cadena para contener la ruta del directorio del usuario
    char path[256];
    snprintf(path, sizeof(path), "usuarios/%s", username); 

    // Crear el directorio
    pthread_mutex_lock(&mutex_funciones);
    if (mkdir(path, 0700) == 0) {
        printf("Directorio creado para el usuario %s", username);
    } else {
        perror("Error al crear el directorio");
    }
    pthread_mutex_unlock(&mutex_funciones);


    // Crear el archivo lista_archivos.txt dentro del directorio del usuario
    char archivo_path[256];
    strcpy(archivo_path, path);
    strcat(archivo_path, "/lista_archivos.txt");

    pthread_mutex_lock(&mutex_funciones);
    FILE *archivo = fopen(archivo_path, "w");
    if (archivo == NULL) {
        perror("Error al crear el archivo lista_archivos.txt");
        return;
    }
    fclose(archivo);
    pthread_mutex_unlock(&mutex_funciones);
}

int eliminar_directorio(const char *path) {
    // Elimina un directorio de usuario
    DIR *dir;
    struct dirent *entry;
    char full_path[256];

    // Abrir el directorio
    dir = opendir(path);
    if (dir == NULL) {
        perror("Error al abrir el directorio");
        return -1;
    }

    // Iterar sobre cada elemento del directorio
    while ((entry = readdir(dir)) != NULL) {
        // Ignorar los directorios . y ..
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            // Construir la ruta completa del elemento
            strcpy(full_path, path);
            strcat(full_path, "/");
            strcat(full_path, entry->d_name);

            // Bloquear el mutex antes de eliminar
            pthread_mutex_lock(&mutex_funciones);
            // Eliminar el elemento
            if (remove(full_path) != 0) {
                perror("Error al eliminar el archivo");
                closedir(dir);
                pthread_mutex_unlock(&mutex_funciones);
                return -1;
            }

            // Desbloquear el mutex después de eliminar
            pthread_mutex_unlock(&mutex_funciones);
        }
    }
    closedir(dir);
    return 0;
}

int borrar_linea(char *path, const char *usuario) {

    pthread_mutex_lock(&mutex_funciones);
    // Abre el archivo usuarios.txt en modo de lectura y escritura
    FILE *archivo = fopen(path, "r+");
    if (archivo == NULL) {
        perror("Error al abrir el archivo usuarios.txt");
        pthread_mutex_unlock(&mutex_funciones);
        return -1;
    }

    // Abre un archivo temporal para escribir los usuarios que no se van a eliminar
    FILE *temporal = fopen("temporal.txt", "w");
    if (temporal == NULL) {
        perror("Error al abrir el archivo temporal.txt");
        fclose(archivo);
        pthread_mutex_unlock(&mutex_funciones);
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
        pthread_mutex_unlock(&mutex_funciones);
        return -1;
    }
    pthread_mutex_unlock(&mutex_funciones);

    if (encontrado) {
        return 0;
    } else {
        printf("Usuario \"%s\" no encontrado en %s", usuario, path);
        return -1;
    }
}

int comprobar_usuario(char *path, char *usuario) {
    // Comproba que el usuario existe en el archivo especificado
    // 0: no encontrado, 1: encontrado
    pthread_mutex_lock(&mutex_funciones);
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        perror("Error al abrir el archivo\n");
        pthread_mutex_unlock(&mutex_funciones);
        return -1;
    }

    char linea[256];
    char *token;

    // Leer el archivo línea por línea
    while (fgets(linea, sizeof(linea), fp) != NULL) {
        // Sustituir el \n al final de la linea por un valor nulo
        linea[strcspn(linea, "\n")] = '\0';
        // Dividir la línea en tokens usando el espacio como delimitador
        token = strtok(linea, " ");
        
        // Comprobar si el primer token (nombre de usuario) coincide con el usuario buscado
        if (strcmp(token, usuario) == 0) {
            // Si coincide, cerrar el archivo y devolver verdadero
            fclose(fp);
            pthread_mutex_unlock(&mutex_funciones);
            return 1;
        }
    }

    // Cerrar el archivo
    fclose(fp);
    // Si no se encuentra el nombre de usuario, devolver falso
    pthread_mutex_unlock(&mutex_funciones);
    return 0;
}

int crear_archivo_descripcion(const char *username, const char *nombre_archivo, const char *descripcion) {
    // Crear una cadena para contener la ruta del archivo del usuario
    char path[256];
    snprintf(path, sizeof(path), "usuarios/%s/%s.txt", username, nombre_archivo); 

    pthread_mutex_lock(&mutex_funciones);
    // Abrir el archivo para escritura
    FILE *archivo = fopen(path, "w");
    if (archivo == NULL) {
        perror("Error al crear el archivo");
        pthread_mutex_unlock(&mutex_funciones);
        return -1;
    }

    // Escribir la descripción en el archivo
    if (fprintf(archivo, "%s", descripcion) < 0) {
        perror("Error al escribir en el archivo");
        fclose(archivo);
        pthread_mutex_unlock(&mutex_funciones);
        return -1;
    }

    // Cerrar el archivo
    fclose(archivo);

    pthread_mutex_unlock(&mutex_funciones);
    printf("Archivo \"%s\" creado para el usuario \"%s\" con la descripción \"%s\"", nombre_archivo, username, descripcion);
    
    return 0;
}                                        

int escribir_archivo(char *path, char *contenido){
    // Añade una línea al final del archivo especificado
    pthread_mutex_lock(&mutex_funciones);

    // Abrir el archivo en modo append
    FILE *fp = fopen(path, "a");
    if (fp == NULL) {
        perror("Error al abrir el archivo\n");
        return -1;
    }
    fprintf(fp, "%s\n", contenido);
    fclose(fp);
    pthread_mutex_unlock(&mutex_funciones);
    return 0;
}

int escribir_usuario_ip_port(char *path, char *usuario, char *ip, char *puerto) {
    // Escribe el usuario, la ip, y el puerto en el archivo especificado
    pthread_mutex_lock(&mutex_funciones);
    // Abre el archivo en modo append
    FILE *fp = fopen(path, "a");
    if (fp == NULL) {
        perror("Error al abrir el archivo\n");
        pthread_mutex_unlock(&mutex_funciones);
        return -1;
    }
    fprintf(fp, "%s %s %s\n", usuario, ip, puerto);
    fclose(fp);

    pthread_mutex_unlock(&mutex_funciones);
    return 0;
}

int recibir_mensaje(int s_local, char *mensaje_recibido){
    // Recibe un mensaje del cliente mediante el socket
    int recv_status;
    int valor_ascii = 1;
    char valor_convertido;
    int devolucion;
    
    pthread_mutex_lock(&mutex_funciones);
    for (int i = 0; valor_ascii != 0; i++){
        // Recibe el mensaje
        recv_status = recvMessage(s_local, (char *)&valor_ascii, sizeof(char));
        if (recv_status == -1) {
            perror("Error en recepcion\n");
            devolucion = 50;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            close(s_local);

            pthread_mutex_unlock(&mutex_funciones);
            return -1;
        }
        // Lo transforma de ascii a caracter y lo guarda en mensaje_recibido
        valor_convertido = valor_ascii;
        mensaje_recibido[i] = valor_convertido;
        
    }

    pthread_mutex_unlock(&mutex_funciones);
    return 0;
}

int verificar_archivo_existente(const char *username, char *nombre_archivo) {
    // Verifica que el archivo existe en el usuario especificado
    pthread_mutex_lock(&mutex_funciones);
    // Crear una cadena para contener la ruta del directorio del usuario
    char directorio_usuario[256];
    snprintf(directorio_usuario, sizeof(directorio_usuario), "usuarios/%s/", username);

    // Abrir el directorio del usuario
    DIR *dir = opendir(directorio_usuario);
    if (dir == NULL) {
        perror("Error al abrir el directorio del usuario");
        pthread_mutex_unlock(&mutex_funciones);
        return -1; // Error al abrir el directorio
    }

    struct dirent *entry;

    // Recorrer los archivos dentro del directorio
    while ((entry = readdir(dir)) != NULL) {
        // Ignorar los directorios "." y ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Quitar la extensión ".txt" del nombre del archivo
        char nombre_sin_extension[256];
        strncpy(nombre_sin_extension, entry->d_name, sizeof(nombre_sin_extension));
        char *punto = strstr(nombre_sin_extension, ".txt");
        if (punto != NULL) {
            *punto = '\0'; // Coloca un carácter nulo para truncar la cadena en el punto
        }

        // Verificar si el archivo tiene el mismo nombre que el buscado
        if (strcmp(nombre_sin_extension, nombre_archivo) == 0) {
            closedir(dir);
            pthread_mutex_unlock(&mutex_funciones);
            return 1; // El archivo existe
        }
    }

    // Cerrar el directorio
    closedir(dir);
    
    pthread_mutex_unlock(&mutex_funciones);
    
    // El archivo no se encontró
    return 0;
}

int borrar_archivo(const char *username, const char *nombre_archivo) {
    // Crear una cadena para contener la ruta del archivo a borrar
    char ruta_archivo[256];
    snprintf(ruta_archivo, sizeof(ruta_archivo), "usuarios/%s/%s.txt", username, nombre_archivo);

    pthread_mutex_lock(&mutex_funciones);
    // Intentar borrar el archivo
    if (remove(ruta_archivo) == 0) {
        printf("El archivo \"%s\" ha sido borrado para el usuario \"%s\"", nombre_archivo, username);
        pthread_mutex_unlock(&mutex_funciones);
        return 0; // Borrado exitoso
    } else {
        perror("Error al borrar el archivo");
        pthread_mutex_unlock(&mutex_funciones);
        return -1; // Error al borrar el archivo
    }
}

int contar_numero_archivos(char *path){
    DIR *dir;
    struct dirent *ent;
    int file_count = 0; // Contador de archivos


    pthread_mutex_lock(&mutex_funciones);
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

    pthread_mutex_unlock(&mutex_funciones);
    return file_count;
}

int tratar_peticion(int *s) {
    // Declaración de las variables que se van a utilizar
    int s_local;
    char *op_recibido = (char *)malloc(256);
    char *fecha_recibida = (char *)malloc(256);
    char *usuario = (char *)malloc(256);
    char *filename = (char *)malloc(256);
    int devolucion;
    
    pthread_mutex_lock(&mutex_mensaje);
    s_local = (* (int *)s);
    busy = false;
    pthread_cond_signal(&cond_mensaje);
    pthread_mutex_unlock(&mutex_mensaje);

    // Recibir operación
    recibir_mensaje(s_local, op_recibido);

    // Recibir fecha
    recibir_mensaje(s_local, fecha_recibida);

    // Recibir usuario
    recibir_mensaje(s_local, usuario);
    printf("%s FROM %s",op_recibido, usuario);

    // Recibir filename
    if (strcmp(op_recibido, "PUBLISH")== 0 || strcmp(op_recibido, "DELETE") == 0){
        recibir_mensaje(s_local, filename);
    }
    else{
        filename = "\b";
    }

    //Enviar argumentos al servidor RPC
    CLIENT *clnt;
    enum clnt_stat retval_1;
    int result_1;
    char *host;
    host = "localhost";

    clnt = clnt_create (host, SERVER, SERVER_VERS, "tcp");
    if (clnt == NULL) {
        clnt_pcreateerror (host);
        exit (1);
    }
    
    retval_1 = operation_1(usuario, op_recibido, filename, fecha_recibida, &result_1, clnt);
    if (retval_1 != RPC_SUCCESS) {
        clnt_perror (clnt, "call failed");
    }
    clnt_destroy (clnt);

    // Seleccionar operación a realizar
    if (strcmp("REGISTER", op_recibido) == 0){
        // Realizar registro
        int usuario_existente;
        
        // Comprobar que el usuario se encuentra en "usuarios.txt"
        usuario_existente = comprobar_usuario("usuarios.txt", usuario);

        if (usuario_existente == 1) {
            // Enviar mensaje al cliente de que el nombre de usuario ya está en uso
            printf("El usuario coincide");
            
            devolucion = 49;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
			return -1;

        } else if (usuario_existente == 0){
            // Abrir el archivo de texto para escritura (agregar al final)
            int escribir = escribir_archivo("usuarios.txt", usuario);
            if (escribir == -1) {
                devolucion = 50;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                close(s_local);
                return -1;
            }
            
            // Crear directorio para el usuario
            crear_directorio_para_usuario(usuario);

            // Enviar mensaje al cliente de que el registro fue exitoso
            devolucion = 48;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
        }
    }

    if (strcmp("UNREGISTER", op_recibido) == 0){
        // Realizar baja de usuario

        // Comprobar que el usuario existe en "usuarios.txt"
        int usuario_existente = comprobar_usuario("usuarios.txt", usuario);

        if (usuario_existente == 1) {
            // Eliminar directorio del usuario
            char path[256];
            snprintf(path, sizeof(path), "usuarios/%s", usuario);
            if (eliminar_directorio(path) == 0) {
            } else {
                perror("Error al eliminar el directorio");
            }
            if (remove(path) != 0) {
                perror("Error al eliminar el directorio");
                return -1;
            }

            int eliminar = borrar_linea("usuarios.txt", usuario);
            if (eliminar == -1){
                devolucion = 50;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                close(s_local);
                return -1;
            }

            // Realizar disconnect del usuario
            if (comprobar_usuario("conectados.txt", usuario) == 1){
                int eliminar = borrar_linea("conectados.txt", usuario);
                if (eliminar == -1){
                    devolucion = 50;
                    sendMessage(s_local, (char *)&devolucion, sizeof(char));
                    close(s_local);
                    return -1;
                }
            devolucion = 48;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            }
        } else if(usuario_existente == 0){
            // Enviar mensaje al cliente de que el nombre de usuario ya está en uso
            devolucion = 49;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;
        }
    }

    if(strcmp("CONNECT", op_recibido) == 0){
        // Realizar connect
        char *port = (char *)malloc(256);
        char *ip = (char *)malloc(256);

        recibir_mensaje(s_local, port);

        recibir_mensaje(s_local, ip);

        // Comprobar que el usuario existe en "usuarios.txt"
        int usuario_existente;
        usuario_existente = comprobar_usuario("usuarios.txt", usuario);
        if (usuario_existente == 0){
            devolucion = 49;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;
        }

        // Comprobar que el usuario existe en "conectados.txt"
        int connected = comprobar_usuario("conectados.txt", usuario);
        if (connected == 0) {
            // Si no existe, lo escribimos
            int escribir = escribir_usuario_ip_port("conectados.txt", usuario, ip, port);
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
        // Realizar disconnect

        // Comprobar que el usuario existe en "usuarios.txt"
        int usuario_existente;
        usuario_existente = comprobar_usuario("usuarios.txt", usuario);
        if (usuario_existente == 0){
            devolucion = 49;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;
        }
        // Comprobar que el usuario existe en "conectados.txt"
        int connected = comprobar_usuario("conectados.txt", usuario);
        if (connected == 1) {
            // Si existe, lo eliminamos
            int eliminar = borrar_linea("conectados.txt", usuario);
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
        // Realizar publish
        char *fileContent = (char *)malloc(256);
        
        recibir_mensaje(s_local, fileContent);

        // Comprobar que el usuario existe en "usuarios.txt"
        int usuario_existente = comprobar_usuario("usuarios.txt", usuario);
        if (usuario_existente == 0) {
                devolucion = 49;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                return -1;
        }
        
        // Comprobar que el usuario existe en "conectados.txt"
        int connected = comprobar_usuario("conectados.txt", usuario);
        if (connected == 0) {
            devolucion = 50;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;
        }
            
        // Comprobar si el archivo existía anteriormente
        int archivo_existente = verificar_archivo_existente(usuario, filename);
        if (archivo_existente == 1) {
            devolucion = 51;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;
        }else if(archivo_existente == 0){
            // Si el archivo no existía, lo creamos
            int crear = crear_archivo_descripcion(usuario, filename, fileContent);
            if (crear == -1){
                devolucion = 52;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                close(s_local); 
                return -1;
            }
            char path[256];
            char content[256];
            // Añade el nombre y la descripción a "lista_archivos.txt"
            strcpy(path, "usuarios/");
            strcat(path, usuario);
            strcat(path, "/lista_archivos.txt");
            strcpy(content, filename);
            strcat(content, " \"");
            strcat(content, fileContent);
            strcat(content, "\"");
            escribir_archivo(path, content);
            devolucion = 48;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
        }
    }

    if (strcmp("DELETE", op_recibido) == 0){
        // Realizar delete

        // Comprobar que el usuario existe en "usuarios.txt"
        int usuario_existente = comprobar_usuario("usuarios.txt", usuario);
        if (usuario_existente == 0) {
                devolucion = 49;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                return -1;
        }
        
        // Comprobar que el usuario existe en "conectados.txt"
        int connected = comprobar_usuario("conectados.txt", usuario);
        if (connected == 0) {
            devolucion = 50;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;
        }

        // Comprobar si el archivo existía anteriormente
        int archivo_existente = verificar_archivo_existente(usuario, filename);
        if (archivo_existente == 0) {
            devolucion = 51;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;

        }else if(archivo_existente == 1){
            // Si el archivo existe, lo eliminamos
            int borrar = borrar_archivo(usuario, filename);
            char path[256];
            // Añade el nombre y la descripción a "lista_archivos.txt"
            strcpy(path, "usuarios/");
            strcat(path, usuario);
            strcat(path, "/lista_archivos.txt");
            borrar_linea(path, filename);
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
        // Listar usuarios

        // Comprobar que el usuario existe en "usuarios.txt"
        int usuario_existente = comprobar_usuario("usuarios.txt", usuario);
        if (usuario_existente == 0) {
                devolucion = 49;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                return -1;
        }

        // Comprobar que el usuario existe en "conectados.txt"
        int connected = comprobar_usuario("conectados.txt", usuario);
        if (connected == 0) {
            devolucion = 50;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;
        }
        
        pthread_mutex_lock(&mutex_funciones );
        // Lee el archivo que contiene los usuarios conectados
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
        sendMessage(s_local, (char *)&devolucion, sizeof(char));
        sleep(0.1);

        pthread_mutex_unlock(&mutex_funciones);

        pthread_mutex_lock(&mutex_funciones);
        FILE *fp = fopen("conectados.txt", "r");
        if (fp == NULL) {
            perror("Error al abrir el archivo\n");
            pthread_mutex_unlock(&mutex_funciones);
            return -1;
        }

        // Leer el archivo línea por línea
        while (fgets(linea, sizeof(linea), fp) != NULL) {
            // Sustituir el \n al final de la linea por un valor nulo
            linea[strcspn(linea, "\n")] = '\0';
            // Dividir la línea en tokens usando el espacio como delimitador
            token = strtok(linea, " ");
            // Imprimir cada token
            while (token != NULL) {
                for (int i = 0; token[i] != '\0'; i++) {
                    sendMessage(s_local, (char *)&token[i], sizeof(char));
                    sleep(0.1);
                }
                devolucion = 0;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                sleep(0.1);
                token = strtok(NULL, " "); // Obtener el siguiente token
            }
        }
        // Cerrar el archivo
        fclose(fp);
        pthread_mutex_unlock(&mutex_funciones);
    }

    if(strcmp("LIST_CONTENT", op_recibido) == 0){
        // Listar contenido

        char *usuario_deseado = (char *)malloc(256);

        // Comprobar que el usuario existe en "usuarios.txt"
        int usuario_existente = comprobar_usuario("usuarios.txt", usuario);
        if (usuario_existente == 0) {
                devolucion = 49;
                sendMessage(s_local, (char *)&devolucion, sizeof(char));
                return -1;
        }

        // Comprobar que el usuario existe en "conectados.txt"
        int connected = comprobar_usuario("conectados.txt", usuario);
        if (connected == 0) {
            devolucion = 50;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1; 
        }

        // Recibir el usuario del que se quiere conocer su contenido
        recibir_mensaje(s_local, usuario_deseado);
        printf("Listar contenido de %s para %s", usuario_deseado, usuario);

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
        // Contar el número de archivos que tiene el usuario deseado
        strcpy(path, "usuarios/");
        strcat(path, usuario_deseado);
        int file_count = contar_numero_archivos(path);

        // Imprimir el número de archivos
        devolucion = file_count - 1;
        sendMessage(s_local, (char *)&devolucion, sizeof(char));
        sleep(0.1);
    
        DIR *dir;
        struct dirent *ent;
        char filename[256];

        // Abrir el directorio
        if ((dir = opendir(path)) != NULL) {
            // Leer cada entrada en el directorio
            while ((ent = readdir(dir)) != NULL) {
                pthread_mutex_lock(&mutex_funciones);
                // Ignorar las entradas especiales "." y ".."
                if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                    strcpy(filename, ent->d_name);

                    // Abrir el archivo
                    FILE *archivo;
                    char filename_path[256]; // Tamaño suficiente para la ruta completa
                    strcpy(filename_path, path);
                    strcat(filename_path, "/");
                    strcat(filename_path, ent->d_name);
                    archivo = fopen(filename_path, "r");
                    
                    // Verificar si se pudo abrir el archivo
                    if (archivo == NULL) {
                        perror("Error al abrir el archivo");
                        pthread_mutex_unlock(&mutex_funciones);
                        return EXIT_FAILURE;
                    }

                    // Leer y mostrar el contenido del archivo
                    if (strcmp(filename,"lista_archivos.txt") != 0){
                        char description[256];
                        if (fgets(description, sizeof(description), archivo) == NULL) {
                            perror("No se pudo leer la descripción");
                            fclose(archivo);
                            continue; // Saltar a la siguiente iteración
                        }
                        // Cerrar el archivo
                        fclose(archivo);
                        char message[516];
                        sprintf(message, "\t%s \"%s\"", filename, description);
                        sendMessage(s_local, message, strlen(message));
                    }
                }
                pthread_mutex_unlock(&mutex_funciones);
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


	if (argc < 3){
        perror("SERVER FORMAT ERROR, ./servidor -p <port>");
    }
	int puerto = atoi(argv[2]);
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
    printf("s> init server 127.0.0.1 : %d", puerto);

	// Inicializar mutex y variables condicionales
    pthread_mutex_init(&mutex_mensaje, NULL);
    pthread_mutex_init(&mutex_funciones, NULL);
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
    pthread_mutex_destroy(&mutex_funciones);
	pthread_cond_destroy(&cond_mensaje);

	return 0;
}