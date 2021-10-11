#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)ypmaint_xdr.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * This contains xdr routines used by the NIS interface
 * for systems and maintenance programs only.  This is a separate module
 * because most NIS clients should not need to link to it.
 */

#define NULL 0
#include <rpc/rpc.h>
#include "yp_prot.h"
#include "ypclnt.h"

bool xdr_ypmaplist();
bool xdr_ypmaplist_wrap_string();

/*
 * Serializes/deserializes a ypresp_master structure.
 */
bool
xdr_ypresp_master(xdrs, ps)
	XDR * xdrs;
	struct ypresp_master *ps;
{
	return (xdr_u_long(xdrs, &ps->status) &&
	     xdr_ypowner_wrap_string(xdrs, &ps->master) );
}

/*
 * Serializes/deserializes a ypresp_order structure.
 */
bool
xdr_ypresp_order(xdrs, ps)
	XDR * xdrs;
	struct ypresp_order *ps;
{
	return (xdr_u_long(xdrs, &ps->status) &&
	     xdr_u_long(xdrs, &ps->ordernum) );
}

/*
 * This is like xdr_ypmap_wrap_string except that it serializes/deserializes
 * an array, instead of a pointer, so xdr_reference can work on the structure
 * containing the char array itself.
 */
bool
xdr_ypmaplist_wrap_string(xdrs, pstring)
	XDR * xdrs;
	char *pstring;
{
	char *s;

	s = pstring;
	return (xdr_string(xdrs, &s, YPMAXMAP) );
}

/*
 * Serializes/deserializes a ypmaplist.
 */
bool
xdr_ypmaplist(xdrs, lst)
	XDR *xdrs;
	struct ypmaplist **lst;
{
	bool more_elements;
	int freeing = (xdrs->x_op == XDR_FREE);
	struct ypmaplist **next;

	while (TRUE) {
		more_elements = (*lst != (struct ypmaplist *) NULL);
		
		if (! xdr_bool(xdrs, &more_elements))
			return (FALSE);
			
		if (! more_elements)
			return (TRUE);  /* All done */
			
		if (freeing)
			next = &((*lst)->ypml_next);

		if (! xdr_reference(xdrs, lst, (u_int) sizeof(struct ypmaplist),
		    xdr_ypmaplist_wrap_string))
			return (FALSE);
			
		lst = (freeing) ? next : &((*lst)->ypml_next);
	}
}


/*
 * Serializes/deserializes a ypresp_maplist.
 */
bool
xdr_ypresp_maplist(xdrs, ps)
	XDR * xdrs;
	struct ypresp_maplist *ps;

{
	return (xdr_u_long(xdrs, &ps->status) &&
	   xdr_ypmaplist(xdrs, &ps->list) );
}

/*
 * Serializes/deserializes a yppushresp_xfr structure.
 */
bool
xdr_yppushresp_xfr(xdrs, ps)
	XDR *xdrs;
	struct yppushresp_xfr *ps;
{
	return (xdr_u_long(xdrs, &ps->transid) &&
	    xdr_u_long(xdrs, &ps->status));
}


/*
 * Serializes/deserializes a ypreq_xfr structure.
 */
bool
xdr_ypreq_xfr(xdrs, ps)
	XDR * xdrs;
	struct ypreq_xfr *ps;
{
	return (xdr_ypmap_parms(xdrs, &ps->map_parms) &&
	    xdr_u_long(xdrs, &ps->transid) &&
	    xdr_u_long(xdrs, &ps->proto) &&
	    xdr_u_short(xdrs, &ps->port) );
}


/*
 * Serializes/deserializes a stream of struct ypresp_key_val's.  This is used
 * only by the client side of the batch enumerate operation.
 */
bool
xdr_ypall(xdrs, callback)
	XDR * xdrs;
	struct ypall_callback *callback;
{
	bool more;
	struct ypresp_key_val kv;
	bool s;
	char keybuf[YPMAXRECORD];
	char valbuf[YPMAXRECORD];

	if (xdrs->x_op == XDR_ENCODE)
		return(FALSE);

	if (xdrs->x_op == XDR_FREE)
		return(TRUE);

	kv.keydat.dptr = keybuf;
	kv.valdat.dptr = valbuf;
	kv.keydat.dsize = YPMAXRECORD;
	kv.valdat.dsize = YPMAXRECORD;
	
	for (;;) {
		if (! xdr_bool(xdrs, &more) )
			return (FALSE);
			
		if (! more)
			return (TRUE);

		s = xdr_ypresp_key_val(xdrs, &kv);
		
		if (s) {
			s = (*callback->foreach)(kv.status, kv.keydat.dptr,
			    kv.keydat.dsize, kv.valdat.dptr, kv.valdat.dsize,
			    callback->data);
			
			if (s)
				return (TRUE);
		} else {
			return (FALSE);
		}
	}
}


/*
 * Serializes/deserializes a ypbind_setdom structure.
 */
bool
xdr_ypbind_setdom(xdrs, ps)
	XDR *xdrs;
	struct ypbind_setdom *ps;
{
	char *domain = ps->ypsetdom_domain;
	
	return (xdr_ypdomain_wrap_string(xdrs, &domain) &&
	    xdr_yp_binding(xdrs, &ps->ypsetdom_binding) &&
	    xdr_u_short(xdrs, &ps->ypsetdom_vers));
}
