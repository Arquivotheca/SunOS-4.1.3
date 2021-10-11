#if	!defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)ypxdr.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * This contains xdr routines used by the NIS rpc interface.
 */

#define	NULL 0
#include <rpc/rpc.h>
#include "yp_prot.h"
#include "ypclnt.h"

typedef	struct xdr_discrim XDR_DISCRIM;
bool xdr_datum();
bool xdr_ypdomain_wrap_string();
bool xdr_ypmap_wrap_string();
bool xdr_ypreq_key();
bool xdr_ypreq_nokey();
bool xdr_ypresp_val();
bool xdr_ypresp_key_val();
bool xdr_ypbind_resp ();
bool xdr_yp_inaddr();
bool xdr_yp_binding();
bool xdr_ypmap_parms();
bool xdr_ypowner_wrap_string();
bool xdr_ypref();

extern malloc_t malloc();

/*
 * Serializes/deserializes a dbm datum data structure.
 */
bool
xdr_datum(xdrs, pdatum)
	XDR * xdrs;
	datum * pdatum;

{
	return (xdr_bytes(xdrs, &(pdatum->dptr), &(pdatum->dsize),
	    YPMAXRECORD));
}


/*
 * Serializes/deserializes a domain name string.  This is a "wrapper" for
 * xdr_string which knows about the maximum domain name size.
 */
bool
xdr_ypdomain_wrap_string(xdrs, ppstring)
	XDR * xdrs;
	char **ppstring;
{
	return (xdr_string(xdrs, ppstring, YPMAXDOMAIN) );
}

/*
 * Serializes/deserializes a map name string.  This is a "wrapper" for
 * xdr_string which knows about the maximum map name size.
 */
bool
xdr_ypmap_wrap_string(xdrs, ppstring)
	XDR * xdrs;
	char **ppstring;
{
	return (xdr_string(xdrs, ppstring, YPMAXMAP) );
}

/*
 * Serializes/deserializes a ypreq_key structure.
 */
bool
xdr_ypreq_key(xdrs, ps)
	XDR *xdrs;
	struct ypreq_key *ps;

{
	return (xdr_ypdomain_wrap_string(xdrs, &ps->domain) &&
	    xdr_ypmap_wrap_string(xdrs, &ps->map) &&
	    xdr_datum(xdrs, &ps->keydat) );
}

/*
 * Serializes/deserializes a ypreq_nokey structure.
 */
bool
xdr_ypreq_nokey(xdrs, ps)
	XDR * xdrs;
	struct ypreq_nokey *ps;
{
	return (xdr_ypdomain_wrap_string(xdrs, &ps->domain) &&
	    xdr_ypmap_wrap_string(xdrs, &ps->map) );
}

/*
 * Serializes/deserializes a ypresp_val structure.
 */

bool
xdr_ypresp_val(xdrs, ps)
	XDR * xdrs;
	struct ypresp_val *ps;
{
	return (xdr_u_long(xdrs, &ps->status) &&
	    xdr_datum(xdrs, &ps->valdat) );
}

/*
 * Serializes/deserializes a ypresp_key_val structure.
 */
bool
xdr_ypresp_key_val(xdrs, ps)
	XDR * xdrs;
	struct ypresp_key_val *ps;
{
	return (xdr_u_long(xdrs, &ps->status) &&
	    xdr_datum(xdrs, &ps->valdat) &&
	    xdr_datum(xdrs, &ps->keydat) );
}


/*
 * Serializes/deserializes an in_addr struct.
 *
 * Note:  There is a data coupling between the "definition" of a struct
 * in_addr implicit in this xdr routine, and the true data definition in
 * <netinet/in.h>.
 */
bool
xdr_yp_inaddr(xdrs, ps)
	XDR * xdrs;
	struct in_addr *ps;

{
	return (xdr_opaque(xdrs, &ps->s_addr, 4));
}

/*
 * Serializes/deserializes a ypbind_binding struct.
 */
bool
xdr_yp_binding(xdrs, ps)
	XDR * xdrs;
	struct ypbind_binding *ps;

{
	return (xdr_yp_inaddr(xdrs, &ps->ypbind_binding_addr) &&
	    xdr_opaque(xdrs, &ps->ypbind_binding_port, 2));
}

/*
 * Serializes/deserializes a ypbind_resp structure.
 */
bool
xdr_ypbind_resp(xdrs, ps)
	XDR * xdrs;
	struct ypbind_resp *ps;

{
	if (!xdr_enum(xdrs, &ps->ypbind_status)) {
		return (FALSE);
	}
	switch (ps->ypbind_status) {
	case YPBIND_SUCC_VAL:
		return (xdr_yp_binding(xdrs,
				       &ps->ypbind_respbody.ypbind_bindinfo));
	case YPBIND_FAIL_VAL:
		return (xdr_u_long(xdrs,
				   &ps->ypbind_respbody.ypbind_error));
	}
	return (FALSE);
}

/*
 * Serializes/deserializes a peer server's node name
 */
bool
xdr_ypowner_wrap_string(xdrs, ppstring)
	XDR * xdrs;
	char **ppstring;

{
	return (xdr_string(xdrs, ppstring, YPMAXPEER) );
}

/*
 * Serializes/deserializes a ypmap_parms structure.
 */
bool
xdr_ypmap_parms(xdrs, ps)
	XDR *xdrs;
	struct ypmap_parms *ps;

{
	return (xdr_ypdomain_wrap_string(xdrs, &ps->domain) &&
	    xdr_ypmap_wrap_string(xdrs, &ps->map) &&
	    xdr_u_long(xdrs, &ps->ordernum) &&
	    xdr_ypowner_wrap_string(xdrs, &ps->owner) );
}
