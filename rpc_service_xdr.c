/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "rpc_service.h"

bool_t
xdr_operation_1_argument (XDR *xdrs, operation_1_argument *objp)
{
	 if (!xdr_string (xdrs, &objp->arg1, ~0))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->arg2, ~0))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->arg3, ~0))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->arg4, ~0))
		 return FALSE;
	return TRUE;
}
