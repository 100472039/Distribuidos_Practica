/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include "rpc_service.h"

bool_t
register_1_svc(char *arg1, char *arg2, char *arg3, int *result,  struct svc_req *rqstp)
{
	bool_t retval;

	printf("Nombre usuario: %s, Operación: %s, Fecha: %s\n", arg1, arg2, arg3);

    // Establece el resultado
    *result = 0; // O el valor que sea apropiado en tu caso

    // Devuelve el valor de retorno
    return retval;
}

bool_t
unregister_1_svc(char *arg1, char *arg2, char *arg3, int *result,  struct svc_req *rqstp)
{
	bool_t retval;

	/*
	 * insert server code here
	 */

	return retval;
}

bool_t
connect_1_svc(char *arg1, char *arg2, char *arg3, int *result,  struct svc_req *rqstp)
{
	bool_t retval;

	/*
	 * insert server code here
	 */

	return retval;
}

bool_t
disconnect_1_svc(char *arg1, char *arg2, char *arg3, int *result,  struct svc_req *rqstp)
{
	bool_t retval;

	/*
	 * insert server code here
	 */

	return retval;
}

bool_t
publish_1_svc(char *arg1, char *arg2, char *arg3, char *arg4, int *result,  struct svc_req *rqstp)
{
	bool_t retval;

	/*
	 * insert server code here
	 */

	return retval;
}

bool_t
delete_1_svc(char *arg1, char *arg2, char *arg3, char *arg4, int *result,  struct svc_req *rqstp)
{
	bool_t retval;

	/*
	 * insert server code here
	 */

	return retval;
}

bool_t
list_users_1_svc(char *arg1, char *arg2, char *arg3, int *result,  struct svc_req *rqstp)
{
	bool_t retval;

	/*
	 * insert server code here
	 */

	return retval;
}

bool_t
list_content_1_svc(char *arg1, char *arg2, char *arg3, int *result,  struct svc_req *rqstp)
{
	bool_t retval;

	/*
	 * insert server code here
	 */

	return retval;
}

int
server_1_freeresult (SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
	xdr_free (xdr_result, result);

	/*
	 * Insert additional freeing code here, if needed
	 */

	return 1;
}