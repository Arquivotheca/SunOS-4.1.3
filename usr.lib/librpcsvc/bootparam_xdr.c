#ifndef lint
static	char sccsid[] = "@(#)bootparam_xdr.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifdef KERNEL
#include <rpc/rpc.h>
#include <rpcsvc/bootparam.h>
#else
#include <rpc/rpc.h>
#include <rpcsvc/bootparam.h>
#endif


bool_t
xdr_bp_machine_name_t(xdrs,objp)
	XDR *xdrs;
	bp_machine_name_t *objp;
{
	if (! xdr_string(xdrs, objp, MAX_MACHINE_NAME)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_bp_path_t(xdrs,objp)
	XDR *xdrs;
	bp_path_t *objp;
{
	if (! xdr_string(xdrs, objp, MAX_PATH_LEN)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_bp_fileid_t(xdrs,objp)
	XDR *xdrs;
	bp_fileid_t *objp;
{
	if (! xdr_string(xdrs, objp, MAX_FILEID)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_ip_addr_t(xdrs,objp)
	XDR *xdrs;
	ip_addr_t *objp;
{
	if (! xdr_char(xdrs, &objp->net)) {
		return(FALSE);
	}
	if (! xdr_char(xdrs, &objp->host)) {
		return(FALSE);
	}
	if (! xdr_char(xdrs, &objp->lh)) {
		return(FALSE);
	}
	if (! xdr_char(xdrs, &objp->impno)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_bp_address(xdrs,objp)
	XDR *xdrs;
	bp_address *objp;
{
	static struct xdr_discrim choices[] = {
		{ (int) IP_ADDR_TYPE, xdr_ip_addr_t },
		{ __dontcare__, NULL }
	};

	if (! xdr_union(xdrs, (enum_t *) &objp->address_type, (char *) &objp->bp_address, choices, (xdrproc_t) NULL)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_bp_whoami_arg(xdrs,objp)
	XDR *xdrs;
	bp_whoami_arg *objp;
{
	if (! xdr_bp_address(xdrs, &objp->client_address)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_bp_whoami_res(xdrs,objp)
	XDR *xdrs;
	bp_whoami_res *objp;
{
	if (! xdr_bp_machine_name_t(xdrs, &objp->client_name)) {
		return(FALSE);
	}
	if (! xdr_bp_machine_name_t(xdrs, &objp->domain_name)) {
		return(FALSE);
	}
	if (! xdr_bp_address(xdrs, &objp->router_address)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_bp_getfile_arg(xdrs,objp)
	XDR *xdrs;
	bp_getfile_arg *objp;
{
	if (! xdr_bp_machine_name_t(xdrs, &objp->client_name)) {
		return(FALSE);
	}
	if (! xdr_bp_fileid_t(xdrs, &objp->file_id)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_bp_getfile_res(xdrs,objp)
	XDR *xdrs;
	bp_getfile_res *objp;
{
	if (! xdr_bp_machine_name_t(xdrs, &objp->server_name)) {
		return(FALSE);
	}
	if (! xdr_bp_address(xdrs, &objp->server_address)) {
		return(FALSE);
	}
	if (! xdr_bp_path_t(xdrs, &objp->server_path)) {
		return(FALSE);
	}
	return(TRUE);
}


