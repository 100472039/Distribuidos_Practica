/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include "rpc_service.h"

bool_t
operation_1_svc(char *arg1, char *arg2, char *arg3, char *arg4, int *result,  struct svc_req *rqstp)
{
	bool_t retval;

    // Imprime los valores recibidos
    printf(" %s %s %s %s\n", arg1, arg2, arg3, arg4);

    // Realiza cualquier otra operación necesaria aquí

    // Establece el resultado
    retval = TRUE; 

    // Devuelve el valor de retorno
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
