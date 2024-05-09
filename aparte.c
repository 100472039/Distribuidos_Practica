@staticmethod
    def  listusers() :
        #  Listar usuarios
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            
        arguments = len(sys.argv)
        if arguments < 3:
            print('Uso: client_calc  <host> <port>')
            exit()

        server_address = (sys.argv[2], int(sys.argv[4]))
        print('connecting to {} port {}'.format(*server_address))
        sock.connect(server_address)
        register_op = "LIST_USERS"
        try:
            for character in register_op:
                sock.sendall(character.encode())
            sock.sendall(b'\0')
            
            print("Usted es el usuario "+str(client._user))
            for character in client._user:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            resultado = sock.recv(1024)
            resultado = resultado.decode()

            if resultado == "0":
                print("LIST_USERS OK")
                num_lines = int(sock.recv(1024).decode())
                while num_lines > 0:
                    # Recibir línea completa
                    user_info = sock.recv(1024).decode()
                    if not user_info:  # Si se recibe una cadena vacía, termina la transmisión
                        break
                    print(user_info)  # Imprimir la información del usuario
                    num_lines -= 1  # Decrementar el número de líneas restantes
            elif resultado == "1":
                print("LIST_USERS FAIL, USER DOES NOT EXIST")
            elif resultado == "2":
                print("LIST_USERS FAIL, USER NOT CONNECTED")
            else:
                print("LIST_USERS FAIL")
        finally:
            print('closing socket')
            sock.close()
        return client.RC.ERROR



if(strcmp("LIST_USERS", op_recibido) == 0) {
        
        FILE *fp = fopen("conectados.txt", "r");
        if (fp == NULL) {
            perror("Error al abrir el archivo\n");
            devolucion = 50;
            sendMessage(s_local, (char *)&devolucion, sizeof(char));
            return -1;
        }

        // Envía éxito al cliente antes de comenzar a enviar las líneas
        devolucion = 48;
        sendMessage(s_local, (char *)&devolucion, sizeof(char));

        char buffer[256];

        int num_lines = 0; // Contador para el número de líneas
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            
            num_lines++;
        }

        rewind(fp);
        sendMessage(s_local, (char *)&num_lines, sizeof(int));
        
        // Enviar cada línea al cliente
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            // Envía la línea completa al cliente
            sendMessage(s_local, buffer, strlen(buffer) + 1);
        }
        fclose(fp);
        // Envía un carácter nulo para indicar el final de la transmisión
        devolucion = '\0';
        sendMessage(s_local, (char *)&devolucion, sizeof(char));
    }