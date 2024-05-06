from enum import Enum
import argparse
import socket
import threading
import sys 

class client :

    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** ATTRIBUTES ******************
    _server = None
    _port = -1

    # ******************** METHODS *******************


    @staticmethod
    def register(user):
        # Realizar registro
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            
        arguments = len(sys.argv)
        if arguments < 3:
            print('Uso: client_calc  <host> <port>')
            exit()

        server_address = (sys.argv[2], int(sys.argv[4]))
        print('connecting to {} port {}'.format(*server_address))
        sock.connect(server_address)
        register_op = "0"
        try:
            sock.sendall(register_op.encode())
            for character in user:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            resultado = sock.recv(1024)
            resultado = resultado.decode()

            if resultado == "0":
                print("REGISTER OK")
            elif resultado == "1":
                print("USERNAME IN USE")
            elif resultado == "2":
                print("REGISTER FAIL")
        finally:
            print('closing socket')
            sock.close()
        return client.RC.ERROR

   
    @staticmethod
    def  unregister(user) :
        # Registrar usuario
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            
        arguments = len(sys.argv)
        if arguments < 3:
            print('Uso: client_calc  <host> <port>')
            exit()

        server_address = (sys.argv[2], int(sys.argv[4]))
        print('connecting to {} port {}'.format(*server_address))
        sock.connect(server_address)
        register_op = "1"
        try:
            sock.sendall(register_op.encode())
            for character in user:
                sock.sendall(character.encode())
            sock.sendall(b'\0')
            
            resultado = sock.recv(1024)
            resultado = resultado.decode()

            if resultado == "0":
                print("UNREGISTER OK")
            elif resultado == "1":
                print("USER DOES NOT EXIST")
            elif resultado == "2":
                print("UNREGISTER FAIL")
        finally:
            print('closing socket')
            sock.close()
        return client.RC.ERROR

    
    @staticmethod
    def connect(user):
        # Crear un socket para recibir peticiones del cliente
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.bind(('localhost', 0))  # Enlazar a cualquier puerto disponible en localhost
        server_socket.listen(1)  # Escuchar una conexión entrante

        # Obtener el puerto asignado
        port = server_socket.getsockname()[1]
        print(f"Escuchando en el puerto {port}")

        # Crear un hilo para manejar las peticiones entrantes
        import threading

        def handle_client_requests(client_socket):

            pass  
        
        # Establecer conexión con el servidor
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        arguments = len(sys.argv)

        if arguments < 3:
            print('Uso: client_calc  <host> <port>')
            exit()

        server_address = (sys.argv[2], int(sys.argv[4]))
        print('Conectando a {} en el puerto {}'.format(*server_address))
        sock.connect(server_address)

        # Enviar la solicitud de conexión al servidor
        connect_op = "2"
        try:
            sock.sendall(connect_op.encode())
            
            for character in user:
                sock.sendall(character.encode())
            sock.sendall(b'\0')
            sock.sendall(str(port).encode())


            # Recibir el resultado de la operación desde el servidor
            resultado = sock.recv(1024)
            resultado = resultado.decode()

            if resultado == "0":
                # threading.Thread(target=handle_client_requests, args=(server_socket.accept()[0],)).start()
                print("CONNECT OK")
            elif resultado == "1":
                print("CONNECT FAIL, USER DOES NOT EXIST")
            elif resultado == "2":
                print("USER ALREADY CONNECTED")
            elif resultado == "3":
                print("CONNECT FAIL")

        finally:
            print('Cerrando el socket')
            sock.close()

        # Cerrar el socket del servidor
        server_socket.close()
        return client.RC.ERROR



    
    @staticmethod
    def  disconnect(user) :
        #  Write your code here
        return client.RC.ERROR

    @staticmethod
    def  publish(fileName,  description) :
        #  Write your code here
        return client.RC.ERROR

    @staticmethod
    def  delete(fileName) :
        #  Write your code here
        return client.RC.ERROR

    @staticmethod
    def  listusers() :
        #  Write your code here
        return client.RC.ERROR

    @staticmethod
    def  listcontent(user) :
        #  Write your code here
        return client.RC.ERROR

    @staticmethod
    def  getfile(user,  remote_FileName,  local_FileName) :
        #  Write your code here
        return client.RC.ERROR

    # *
    # **
    # * @brief Command interpreter for the client. It calls the protocol functions.
    @staticmethod
    def shell():

        while (True) :
            try :
                command = input("c> ")
                line = command.split(" ")
                if (len(line) > 0):

                    line[0] = line[0].upper()

                    if (line[0]=="REGISTER") :
                        if (len(line) == 2) :
                            client.register(line[1])
                        else :
                            print("Syntax error. Usage: REGISTER <userName>")

                    elif(line[0]=="UNREGISTER") :
                        if (len(line) == 2) :
                            client.unregister(line[1])
                        else :
                            print("Syntax error. Usage: UNREGISTER <userName>")

                    elif(line[0]=="CONNECT") :
                        if (len(line) == 2) :
                            client.connect(line[1])
                        else :
                            print("Syntax error. Usage: CONNECT <userName>")
                    
                    elif(line[0]=="PUBLISH") :
                        if (len(line) >= 3) :
                            #  Remove first two words
                            description = ' '.join(line[2:])
                            client.publish(line[1], description)
                        else :
                            print("Syntax error. Usage: PUBLISH <fileName> <description>")

                    elif(line[0]=="DELETE") :
                        if (len(line) == 2) :
                            client.delete(line[1])
                        else :
                            print("Syntax error. Usage: DELETE <fileName>")

                    elif(line[0]=="LIST_USERS") :
                        if (len(line) == 1) :
                            client.listusers()
                        else :
                            print("Syntax error. Use: LIST_USERS")

                    elif(line[0]=="LIST_CONTENT") :
                        if (len(line) == 2) :
                            client.listcontent(line[1])
                        else :
                            print("Syntax error. Usage: LIST_CONTENT <userName>")

                    elif(line[0]=="DISCONNECT") :
                        if (len(line) == 2) :
                            client.disconnect(line[1])
                        else :
                            print("Syntax error. Usage: DISCONNECT <userName>")

                    elif(line[0]=="GET_FILE") :
                        if (len(line) == 4) :
                            client.getfile(line[1], line[2], line[3])
                        else :
                            print("Syntax error. Usage: GET_FILE <userName> <remote_fileName> <local_fileName>")

                    elif(line[0]=="QUIT") :
                        if (len(line) == 1) :
                            break
                        else :
                            print("Syntax error. Use: QUIT")
                    else :
                        print("Error: command " + line[0] + " not valid.")
            except Exception as e:
                print("Exception: " + str(e))

    # *
    # * @brief Prints program usage
    @staticmethod
    def usage() :
        print("Usage: python3 client.py -s <server> -p <port>")


    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def  parseArguments(argv) :
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 client.py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535")
            return False
        
        _server = args.s
        _port = args.p

        return True


    # ******************** MAIN *********************
    @staticmethod
    def main(argv) :
        if (not client.parseArguments(argv)) :
            client.usage()
            return
        
        client.shell()
        print("+++ FINISHED +++")
    

if __name__=="__main__":
    client.main([])