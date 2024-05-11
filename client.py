from enum import Enum
import argparse
import socket
import threading
import sys 
import time
import threading
import os
import builtins
import zeep

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
    thread_running = False
    _user = None
    thread = None
    conectados = []
    # ******************** METHODS *******************

    @staticmethod
    def get_datetime_from_service():
        # URL del servicio web SOAP
        wsdl = "http://localhost:8000/?wsdl"

        # Crear un cliente Zeep
        soap = zeep.Client(wsdl=wsdl) 

        try:
            # Llamar al método remoto para obtener la fecha y hora actual
            result = soap.service.get_datetime()
            print("c> Fecha y hora del servicio web:", result)
            return result
        except Exception as e:
            print("Error al llamar al servicio web:", e)

        return client.RC.ERROR

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
        register_op = "REGISTER"
        fecha = client.get_datetime_from_service()
        
        try:
            for character in register_op:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            for character in fecha:
                sock.sendall(character.encode())
            sock.sendall(b'\0')
            
            for character in user:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            resultado = sock.recv(1024)
            resultado = resultado.decode()

            if resultado == "0":
                print("c> REGISTER OK")
            elif resultado == "1":
                print("c> USERNAME IN USE")
            elif resultado == "2":
                print("c> REGISTER FAIL")
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
        register_op = "UNREGISTER"
        fecha = client.get_datetime_from_service()
        
        try:
            for character in register_op:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            for character in fecha:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            for character in user:
                sock.sendall(character.encode())
            sock.sendall(b'\0')
            
            resultado = sock.recv(1024)
            resultado = resultado.decode()

            if resultado == "0":
                print("c> UNREGISTER OK")
            elif resultado == "1":
                print("c> USER DOES NOT EXIST")
            elif resultado == "2":
                print("c> UNREGISTER FAIL")
        finally:
            print('closing socket')
            sock.close()
        return client.RC.ERROR
    
    @staticmethod
    def connect(user):
        # Paso 1: Obtener un puerto libre en el cliente
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.bind(('localhost', 0))
        ip, port = server_socket.getsockname()
        
        # Paso 2: Crear un hilo para manejar las solicitudes de descarga
            
        
        # Paso 3: Conectar al servidor principal
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_address = ('localhost', 8080)  # Cambia esto por la IP y puerto del servidor principal
        sock.connect(server_address)

        # Paso 4: Enviar la solicitud de conexión con la información necesaria
        register_op = "CONNECT"
        fecha = client.get_datetime_from_service()
        
        try:
            for character in register_op:
                sock.sendall(character.encode())
            sock.sendall(b'\0')
            for character in user:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            for character in fecha:
                sock.sendall(character.encode())
            sock.sendall(b'\0')
            
            port = str(port)
            for character in port:
                sock.sendall(character.encode())
            sock.sendall(b'\0')
            for character in ip:
                sock.sendall(character.encode())
            sock.sendall(b'\0')
            print("c> Puerto de escucha: ", port)
            print("c> IP: ", ip)
            # Paso 5: Recibir la respuesta del servidor
            resultado = sock.recv(1024)
            resultado = resultado.decode()
            # Paso 6: Procesar la respuesta del servidor
            if resultado == "0":
                client.thread_running = True
                server_socket.listen(5)
                client.thread = threading.Thread(target = client.handle_requests, args=(server_socket,), daemon=True)
                client.thread.start()
                client._user = user
                print("c> CONNECT OK")
            elif resultado == "1":
                print("c> CONNECT FAIL, USER DOES NOT EXIST")
            elif resultado == "2":
                print("c> USER ALREADY CONNECTED")
            else:
                print("c> CONNECT FAIL")

        finally:
            # Paso 7: Cerrar la conexión
            print('Closing socket')
            sock.close()

        return client.RC.ERROR

    
    @staticmethod
    def  disconnect(user) :
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            
        arguments = len(sys.argv)
        if arguments < 3:
            print('Uso: client_calc  <host> <port>')
            exit()

        server_address = (sys.argv[2], int(sys.argv[4]))
        print('connecting to {} port {}'.format(*server_address))
        sock.connect(server_address)
        register_op = "DISCONNECT"
        fecha = client.get_datetime_from_service()
        
        try:
            for character in register_op:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            for character in fecha:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            for character in user:
                sock.sendall(character.encode())
            sock.sendall(b'\0')
            
            resultado = sock.recv(1024)
            resultado = resultado.decode()

            if resultado == "0":
                client.thread_running = False
                client._user = None
                print("c> DISCONNECT OK")
            elif resultado == "1":
                print("c> USER DOES NOT EXIST")
            elif resultado == "2":
                print("c> DISCONNECT FAIL")
        finally:
            print('closing socket')
            sock.close()

        return client.RC.ERROR

    @staticmethod
    def  publish(fileName,  description) :
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            
        arguments = len(sys.argv)
        if arguments < 3:
            print('Uso: client_calc  <host> <port>')
            exit()

        server_address = (sys.argv[2], int(sys.argv[4]))
        print('connecting to {} port {}'.format(*server_address))
        sock.connect(server_address)
        register_op = "PUBLISH"
        fecha = client.get_datetime_from_service()
        
        try:
            
            for character in register_op:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            for character in fecha:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            for character in client._user:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            print("c> filename:",str(fileName))
            for character in fileName:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            print("c> description:", str(description))
            for character in description:
                sock.sendall(character.encode())
            sock.sendall(b'\0')
            
            resultado = sock.recv(1024)
            resultado = resultado.decode()

            if resultado == "0":
                print("c> PUBLISH OK")
            elif resultado == "1":
                print("c> PUBLISH FAIL, USER DOES NOT EXIST")
            elif resultado == "2":
                print("c> PUBLISH FAIL, USER NOT CONNECTED")
            elif resultado == "3":
                print("c> PUBLISH FAIL, CONTENT ALREADY PUBISHED")
            elif resultado == "4":
                print("c> PUBLISH FAIL")
        finally:
            print('closing socket')
            sock.close()
        return client.RC.ERROR

    @staticmethod
    def  delete(fileName) :
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            
        arguments = len(sys.argv)
        if arguments < 3:
            print('Uso: client_calc  <host> <port>')
            exit()

        server_address = (sys.argv[2], int(sys.argv[4]))
        print('connecting to {} port {}'.format(*server_address))
        sock.connect(server_address)
        register_op = "DELETE"
        fecha = client.get_datetime_from_service()
        
        try:
            
            for character in register_op:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            for character in fecha:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            for character in client._user:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            print("c> filename:",str(fileName))
            for character in fileName:
                sock.sendall(character.encode())
            sock.sendall(b'\0')
            
            resultado = sock.recv(1024)
            resultado = resultado.decode()

            if resultado == "0":
                print("c> DELETE OK")
            elif resultado == "1":
                print("c> DELETE FAIL, USER DOES NOT EXIST")
            elif resultado == "2":
                print("c> DELETE FAIL, USER NOT CONNECTED")
            elif resultado == "3":
                print("c> DELETE FAIL, CONTENT NOT PUBISHED")
            elif resultado == "4":
                print("c> DELETE FAIL")
        finally:
            print('closing socket')
            sock.close()
        return client.RC.ERROR

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
        fecha = client.get_datetime_from_service()
        
        try:
            for character in register_op:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            for character in fecha:
                sock.sendall(character.encode())
            sock.sendall(b'\0')
            
            print("c> Usted es el usuario "+str(client._user))
            for character in client._user:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            resultado = sock.recv(1024)
            resultado = resultado.decode()
            print("c> Resultado: "+resultado)

            if resultado == "0":
                print("c> LIST_USERS OK")
                # Recibe número de filas a imprimir
                resultado = ""
                palabra = ""
                n_lineas = sock.recv(1024)
                n_lineas = n_lineas.decode()
                n_lineas = ord(n_lineas)
                n_lineas = int(n_lineas)*3
                conectados = []
                while n_lineas > 0:
                    caracter = sock.recv(1024)
                    if caracter == b'\x00':
                        resultado += palabra+" "
                        n_lineas -= 1
                        if (n_lineas+1) % 3 == 0:
                            client.conectados.append({})
                            client.conectados[-1]["nombre"] = palabra
                        if (n_lineas+2) % 3 == 0:
                            # client.conectados[-1].append(palabra)
                            client.conectados[-1]["ip"] = palabra
                        if n_lineas % 3 == 0:
                            # client.conectados[-1].append(palabra)
                            client.conectados[-1]["puerto"] = palabra
                            resultado += "\n"
                        palabra = ""
                    else:
                        palabra += caracter.decode()
                print(resultado)
            elif resultado == "1":
                print("c> LIST_USERS FAIL, USER DOES NOT EXIST")
            elif resultado == "2":
                print("c> LIST_USERS FAIL, USER NOT CONNECTED")
            else:
                print("c> LIST_USERS FAIL")
        finally:
            print('closing socket')
            sock.close()
        return client.RC.ERROR

    @staticmethod
    def  listcontent(user) :
        #  Listar contenido
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            
        arguments = len(sys.argv)
        if arguments < 3:
            print('Uso: client_calc  <host> <port>')
            exit()

        server_address = (sys.argv[2], int(sys.argv[4]))
        print('connecting to {} port {}'.format(*server_address))
        sock.connect(server_address)
        register_op = "LIST_CONTENT"
        fecha = client.get_datetime_from_service()
        
        try:
            for character in register_op:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            for character in fecha:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            for character in client._user:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            for character in user:
                sock.sendall(character.encode())
            sock.sendall(b'\0')

            resultado = sock.recv(1024)
            resultado = resultado.decode()

            if resultado == "0":
                print("c> LIST_CONTENT OK")
                # Recibir número de archivos
                n_lineas = sock.recv(1024)
                n_lineas = n_lineas.decode()
                n_lineas = ord(n_lineas)
                while n_lineas > 0:
                    resultado = sock.recv(1024)
                    message = resultado.decode()
                    n_lineas -= 1
                    print(message)
            elif resultado == "1":
                print("c> LIST_CONTENT FAIL, USER DOES NOT EXIST")
            elif resultado == "2":
                print("c> LIST_CONTENT FAIL, USER NOT CONNECTED")
            elif resultado == "3":
                print("c> LIST_CONTENT FAIL, REMOTE USER DOES NOT EXIST")
            else:
                print("c> LIST_CONTENT FAIL")
        finally:
            print('closing socket')
            sock.close()
        return client.RC.ERROR

    @staticmethod
    def handle_requests(server_socket):

        print("c> Funcion finalizada")
        while client.thread_running:
            client_socket, _ = server_socket.accept()
            request = client_socket.recv(1024).decode()
            if request == "GET FILE":
                filename = client_socket.recv(1024).decode()
                user_directory = os.path.join("usuarios", client._user)
                remote_file_path = os.path.abspath(os.path.join(user_directory, filename))

                # Verificar si el archivo existe localmente
                if os.path.exists(remote_file_path):

                    # Enviar código 0 para indicar que el archivo se puede transferir
                    client_socket.sendall(b'0')

                    # Abrir el archivo y enviar su contenido al cliente
                    with open(remote_file_path, 'rb') as file:
                        data = True
                        while data:
                            data = file.read(1024)
                            client_socket.sendall(data)
                else:

                    # Enviar código 1 para indicar que el archivo no existe
                    client_socket.sendall(b'1')

            # Cerrar la conexión con el cliente remoto
            client_socket.close()
        server_socket.close()
        
    @staticmethod
    def  getfile(user,  remote_FileName,  local_FileName) :
        # Listar contenido

        # Conectar al cliente remoto
        remote_client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # Tomar ip y puerto
        usuario_conectado = False

        if len(client.conectados) == 0:
            print("c> GET_FILE FAIL, LIST_USERS NOT DONE")
            return client.RC.ERROR
        for diccionario in client.conectados:
            if diccionario["nombre"] == user:
                ip = diccionario["ip"]
                puerto = diccionario["puerto"]
                usuario_conectado = True
                break
        
        if not usuario_conectado:
            print("c> GET_FILE FAIL")
            return client.RC.ERROR
                   
        tupla = (ip, int(puerto))
        remote_client_socket.connect(tupla)  
        
        # Enviar la cadena "GET FILE"
        remote_client_socket.sendall("GET FILE".encode())
        time.sleep(0.1)

        # Enviar el nombre del archivo remoto
        remote_client_socket.sendall(remote_FileName.encode())
        
        # Recibir el resultado de la operación
        resultado = remote_client_socket.recv(1024)
        resultado = int.from_bytes(resultado, byteorder='big')


        user_directory = os.path.join("usuarios", client._user)
        local_file_path = os.path.abspath(os.path.join(user_directory, local_FileName))

        # Procesar el resultado
        if resultado == 48:
            with open(local_file_path, 'wb') as local_file:
                while True:
                    data = remote_client_socket.recv(1024)
                    if not data:
                        break
                    print(data)
                    local_file.write(data)
            print("c> GET_FILE OK")
        elif resultado == 49:
            print("c> GET_FILE FAIL / FILE NOT EXIST")
        else:
            print("c> GET_FILE FAIL")

        # Cerrar la conexión
        remote_client_socket.close()

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
                            if client._user == None:
                                client.connect(line[1])
                            else:
                                print("CONNECT FAIL, YOU ARE ALREADY CONNECTED AS", client._user)
                            if client.thread_running:
                                print("El hilo está en ejecución")
                            else:
                                print("El hilo no se ha iniciado o ya ha terminado")
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
                            if client.thread_running:
                                print("El hilo está en ejecución")
                            else:
                                print("El hilo no se ha iniciado o ya ha terminado")
                        else :
                            print("Syntax error. Usage: DISCONNECT <userName>")

                    elif(line[0]=="GET_FILE") :
                        if (len(line) == 4) :
                            client.getfile(line[1], line[2], line[3])
                        else :
                            print("Syntax error. Usage: GET_FILE <userName> <remote_fileName> <local_fileName>")

                    elif(line[0]=="QUIT") :
                        if (len(line) == 1) :
                            if client._user != None:
                                client.disconnect(client._user) 
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
    