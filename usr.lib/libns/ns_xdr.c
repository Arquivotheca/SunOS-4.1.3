/*	@(#)ns_xdr.c 1.1 92/07/30 SMI 	*/

/*
 *	XDR routines for RFS data structures for utilites and name server.
 */

#include <sys/types.h>
#include <sys/mount.h>
#include <rfs/message.h>
#include <rpc/rpc.h>
#include <rfs/nserve.h>
#include <rfs/ns_xdr.h>
#include <rfs/cirmgr.h>
#include <rfs/pn.h>
#include "nsports.h"

bool_t
xdr_char_array(xdrp, objp, len)
	XDR *xdrp;
	char *objp;
	u_int len;
{
	return( xdr_bytes(xdrp, &objp, &len, len));
}

bool_t
xdr_rfs_string(xdrp, objp)
	XDR *xdrp;
	char *objp;
{
	u_int len, maxlen;

	if (xdrp->x_op == XDR_ENCODE) {
		len = strlen(objp) + 1;
		maxlen = len;
	} else {
		maxlen = len = XR_STRING;
	}
	return( xdr_bytes(xdrp, (char **) &objp, &len, maxlen));
}


bool_t
xdr_rfs_token(xdrs,objp)
	XDR *xdrs;
	struct token *objp;
{
	if (! xdr_int(xdrs, &objp->t_id)) {
		return(FALSE);
	}
	if (! xdr_char_array(xdrs, objp->t_uname, sizeof(objp->t_uname))) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_rfs_ndata(xdrs,objp)
	XDR *xdrs;
	ndata_t *objp;
{
	if (! xdr_long(xdrs, &objp->n_hetero)) {
		return(FALSE);
	}
	if (! xdr_char_array(xdrs, objp->n_passwd, sizeof(objp->n_passwd))) {
		return(FALSE);
	}
	if (! xdr_rfs_token(xdrs, &objp->n_token)) {
		return(FALSE);
	}
	if (! xdr_char_array(xdrs, objp->n_netname, sizeof(objp->n_netname))) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_rfs_n_data(xdrs,objp)
	XDR *xdrs;
	struct n_data *objp;
{
	if (! xdr_rfs_ndata(xdrs, &objp->nd_ndata)) {
		return(FALSE);
	}
	if (! xdr_int(xdrs, &objp->n_vhigh)) {
		return(FALSE);
	}
	if (! xdr_int(xdrs, &objp->n_vlow)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_rfs_pkt_hd(xdrs,objp)
	XDR *xdrs;
	struct pkt_hd *objp;
{
	if (! xdr_long(xdrs, &objp->h_fill)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->h_id)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->h_index)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->h_total)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->h_size)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_rfs_first_msg(xdrs,objp)
	XDR *xdrs;
	struct first_msg *objp;
{
	if (! xdr_long(xdrs, &objp->version)) {
		return(FALSE);
	}
	if (! xdr_char_array(xdrs, objp->mode, sizeof(objp->mode))) {
		return(FALSE);
	}
	if (! xdr_char_array(xdrs, objp->addr, sizeof(objp->addr))) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_rfs_pnhdr(xdrs,objp)
	XDR *xdrs;
	pnhdr_t *objp;
{
	if (! xdr_char_array(xdrs, objp->pn_op, sizeof(objp->pn_op))) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->pn_lo)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->pn_hi)) {
		return(FALSE);
	}
	return(TRUE);
}
