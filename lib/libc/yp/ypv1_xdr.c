#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)ypv1_xdr.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * This contains the xdr functions needed by ypserv and the NIS
 * administrative tools to support the previous version of the NIS protocol.
 * Note that many "old" xdr functions are called, with the assumption that
 * they have not changed between the v1 protocol (which this module exists
 * to support) and the current v2 protocol.  
 */

#define NULL 0
#include <rpc/rpc.h>
#include "yp_prot.h"
#include "ypv1_prot.h"
#include "ypclnt.h"
typedef struct xdr_discrim XDR_DISCRIM;

extern bool xdr_ypreq_key();
extern bool xdr_ypreq_nokey();
extern bool xdr_ypresp_val();
extern bool xdr_ypresp_key_val();
extern bool xdr_ypmap_parms();


/*
 * Serializes/deserializes a yprequest structure.
 */
bool
_xdr_yprequest (xdrs, ps)
	XDR * xdrs;
	struct yprequest *ps;
{
	XDR_DISCRIM yprequest_arms[4];

	yprequest_arms[0].value = (int)YPREQ_KEY;
	yprequest_arms[1].value = (int)YPREQ_NOKEY;
	yprequest_arms[2].value = (int)YPREQ_KEY;
	yprequest_arms[3].value = __dontcare__;
	yprequest_arms[0].proc = (xdrproc_t)xdr_ypreq_key;
	yprequest_arms[1].proc = (xdrproc_t)xdr_ypreq_nokey;
	yprequest_arms[2].proc = (xdrproc_t)xdr_ypmap_parms;
	yprequest_arms[3].proc = (xdrproc_t)NULL;

	return(xdr_union(xdrs, &ps->yp_reqtype, &ps->yp_reqbody,
	    yprequest_arms, NULL) );
}

/*
 * Serializes/deserializes a ypresponse structure.
 */
bool
_xdr_ypresponse (xdrs, ps)
	XDR * xdrs;
	struct ypresponse *ps;

{
	XDR_DISCRIM ypresponse_arms[4];

	ypresponse_arms[0].value = (int)YPRESP_VAL;
	ypresponse_arms[1].value = (int)YPRESP_KEY_VAL;
	ypresponse_arms[2].value = (int)YPRESP_MAP_PARMS;
	ypresponse_arms[3].value = __dontcare__;
	ypresponse_arms[0].proc = (xdrproc_t)xdr_ypresp_val;
	ypresponse_arms[1].proc = (xdrproc_t)xdr_ypresp_key_val;
	ypresponse_arms[2].proc = (xdrproc_t)xdr_ypmap_parms;
	ypresponse_arms[3].proc = (xdrproc_t)NULL;

	return(xdr_union(xdrs, &ps->yp_resptype, &ps->yp_respbody,
			 ypresponse_arms, NULL) );
}

/*
 * Serializes/deserializes a ypbind_oldsetdom structure.
 */
bool
_xdr_ypbind_oldsetdom(xdrs, ps)
	XDR *xdrs;
	struct ypbind_setdom *ps;
{
	char *domain = ps->ypsetdom_domain;
	
	return(xdr_ypdomain_wrap_string(xdrs, &domain) &&
	    xdr_yp_binding(xdrs, &ps->ypsetdom_binding) );
}
