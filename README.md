Para la ejecución del proyecto se deberán seguir los siguientes pasos:

1- abrir una terminal para el servicio web con el siguiente comando: python3 ./servicio_web.py
2- abrir una segunda terminal para el servidor RPC haciendo un make: make clean -f Makefile.rpc_service
3- ejecutar el servidor RPC: ./rpc_service_server
4- ejecutar el servidor principal haciendo primero un make y ejecutando lo siguiente: ./servidor -p 8080
5- ejecutar el cliente de la siguiente manera: python3 ./client.py -s localhost -p 8080
6- llevar a cabo las funcionalidades deseadas en el cliente
