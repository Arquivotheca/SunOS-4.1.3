#if	!defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)yp_order.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

#define	NULL 0
#include <sys/time.h>
#include <rpc/rpc.h>
#include "yp_prot.h"
#include "ypv1_prot.h"
#include "ypclnt.h"

static int v1doorder();
static int v2doorder();

extern struct timeval _ypserv_timeout;
extern int _yp_dobind();
extern unsigned int _ypsleeptime;
extern malloc_t malloc();

/*
 * This checks parameters, and implements the outer "until binding success"
 * loop.
 */
int
yp_order (domain, map, order)
	char *domain;
	char *map;
	unsigned long *order;
{
	int domlen;
	int maplen;
	int reason;
	struct dom_binding *pdomb;
	int (*dofun)();

	if ( (map == NULL) || (domain == NULL) ) {
		return (YPERR_BADARGS);
	}

	domlen = strlen(domain);
	maplen = strlen(map);

	if ( (domlen == 0) || (domlen > YPMAXDOMAIN) ||
	    (maplen == 0) || (maplen > YPMAXMAP) ||
	    (order == NULL) ) {
		return (YPERR_BADARGS);
	}

	for (;;) {

		if (reason = _yp_dobind(domain, &pdomb) ) {
			return (reason);
		}

		dofun = (pdomb->dom_vers == YPVERS) ? v2doorder : v1doorder;

		reason = (*dofun)(domain, map, pdomb, _ypserv_timeout,
		    order);

		if (reason == YPERR_RPC) {
			yp_unbind(domain);
			(void) sleep(_ypsleeptime);
		} else {
			break;
		}
	}

	return (reason);

}

/*
 * This talks v2 to ypserv
 */
static int
v2doorder (domain, map, pdomb, timeout, order)
	char *domain;
	char *map;
	struct dom_binding *pdomb;
	struct timeval timeout;
	unsigned long *order;
{
	struct ypreq_nokey req;
	struct ypresp_order resp;
	unsigned int retval = 0;

	req.domain = domain;
	req.map = map;
	resp.ordernum = 0;

	/*
	 * Do the get_order request.  If the rpc call failed, return with
	 * status from this point.
	 */

	if(clnt_call(pdomb->dom_client, YPPROC_ORDER, xdr_ypreq_nokey, &req,
	    xdr_ypresp_order, &resp, timeout) != RPC_SUCCESS) {
		return (YPERR_RPC);
	}

	/* See if the request succeeded */

	if (resp.status != YP_TRUE) {
		retval = ypprot_err((unsigned) resp.status);
	}

	*order = resp.ordernum;
	CLNT_FREERES(pdomb->dom_client, xdr_ypresp_order, &resp);
	return (retval);

}

/*
 * This talks v1 to ypserv
 */
static int
v1doorder(domain, map, pdomb, timeout, order)
	char *domain;
	char *map;
	struct dom_binding *pdomb;
	struct timeval timeout;
	unsigned long *order;
{
	struct yprequest req;
	struct ypresponse resp;
	enum clnt_stat clnt_stat;
	unsigned int retval = 0;

	req.yp_reqtype = YPPOLL_REQTYPE;
	req.yppoll_req_domain = domain;
	req.yppoll_req_map = map;
	resp.yppoll_resp_domain = resp.yppoll_resp_map =
	    resp.yppoll_resp_owner = (char *) NULL;

	/*
	 * Simulate a v2 "get order" request by doing a v1 "poll map"
	 * request, interpreting the response into current ypclnt.h
	 * return values, or returning the map order number.
	 */
	if( (clnt_stat = (enum clnt_stat) clnt_call(pdomb->dom_client,
	    YPOLDPROC_POLL, _xdr_yprequest, &req, _xdr_ypresponse,
	    &resp, timeout) ) != RPC_SUCCESS) {
		return (YPERR_RPC);
	}

	if (resp.yp_resptype != YPPOLL_RESPTYPE) {
		return (YPERR_YPERR);
	}

	if (!strcmp(resp.yppoll_resp_domain, domain) ) {

		if (!strcmp(resp.yppoll_resp_map, map) ) {

			if (resp.yppoll_resp_ordernum == 0) {
				retval = YPERR_BADDB;
			}

		} else {
			retval = YPERR_MAP;
		}

	} else {
		retval = YPERR_DOMAIN;
	}

	if (! retval) {
		*order = resp.yppoll_resp_ordernum;
	}

	CLNT_FREERES(pdomb->dom_client, _xdr_ypresponse, &resp);
	return (retval);
}
