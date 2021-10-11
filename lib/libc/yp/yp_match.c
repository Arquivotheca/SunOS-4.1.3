#if	!defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)yp_match.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

#define	NULL 0
#include <sys/time.h>
#include <rpc/rpc.h>
#include "yp_prot.h"
#include "ypv1_prot.h"
#include "ypclnt.h"

extern struct timeval _ypserv_timeout;
extern int _yp_dobind();
extern unsigned int _ypsleeptime;
extern malloc_t malloc();
static int v2domatch(), v1domatch();

struct	cache {
	struct cache *next;
	unsigned int birth;
	char *domain;
	char *map;
	char *key;
	int  keylen;
	char *val;
	int  vallen;
};

static struct cache *head;
#define	CACHESZ 16
#define	CACHETO 60*5	/*5 minutes*/

static void
detachnode(prev, n)
	register struct cache *prev, *n;
{

	if (prev == 0) {
		/* assertion: n is head */
		head = n->next;
	} else {
		prev->next = n->next;
	}
	n->next = 0;
}

static void
freenode(n)
	register struct cache *n;
{

	if (n->val != 0)
	    free(n->val);
	if (n->key != 0)
	    free(n->key);
	if (n->map != 0)
	    free(n->map);
	if (n->domain != 0)
	    free(n->domain);
	bzero((char *) n, sizeof(*n));
	free((char *) n);
}

static struct cache *
makenode(domain, map, keylen, vallen)
	char *domain, *map;
	int keylen, vallen;
{
	register struct cache *n =
	    (struct cache *) malloc(sizeof(struct cache));

	if (n == 0)
	    return (0);
	bzero((char *) n, sizeof(*n));
	for (;;) {
		if ((n->domain = malloc(1 + strlen(domain))) == 0)
		    break;
		if ((n->map = malloc(1 + strlen(map))) == 0)
		    break;
		if ((n->key = malloc(keylen)) == 0)
		    break;
		if ((n->val = malloc(vallen)) == 0)
		    break;
		return (n);
	}
	freenode(n);
	return (0);
}

/*
 * Requests the NIS server associated with a given domain to attempt to match
 * the passed key datum in the named map, and to return the associated value
 * datum. This part does parameter checking, and implements the "infinite"
 * (until success) sleep loop.
 */
int
yp_match (domain, map, key, keylen, val, vallen)
	char *domain;
	char *map;
	char *key;
	register int  keylen;
	char **val;		/* returns value array */
	int  *vallen;		/* returns bytes in val */
{
	int domlen;
	int maplen;
	int reason;
	struct dom_binding *pdomb;
	register struct cache *c, *prev;
	int cnt, savesize;
	struct timeval now;
	struct timezone tz;
	int (*dofun)();

	if ( (map == NULL) || (domain == NULL) ) {
		return (YPERR_BADARGS);
	}

	domlen = strlen(domain);
	maplen = strlen(map);

	if ( (domlen == 0) || (domlen > YPMAXDOMAIN) ||
	    (maplen == 0) || (maplen > YPMAXMAP) ||
	    (key == NULL) || (keylen == 0) ) {
		return (YPERR_BADARGS);
	}
	/* is it in our cache ? */
	prev = 0;
	for (prev=0, cnt=0, c=head; c != 0; prev=c, c=c->next, cnt++) {
		if ((c->keylen == keylen) &&
		    (bcmp(key, c->key, keylen) == 0) &&
		    (strcmp(map, c->map) == 0) &&
		    (strcmp(domain, c->domain) == 0)) {
			/* cache hit */
			(void) gettimeofday(&now, &tz);
			if ((now.tv_sec - c->birth) > CACHETO) {
				/* rats.  it it too old to use */
				detachnode(prev, c);
				freenode(c);
				break;
			} else {
				/* NB: Copy two extra bytes; see below */
				savesize = c->vallen + 2;
				*val = malloc(savesize);
				if (*val == 0) {
					return (YPERR_RESRC);
				}
				bcopy(c->val, *val, savesize);
				*vallen = c->vallen;

/*reorganize the list to make this element the head if
 it is already not the head.  This check fixes bug 1023528*/

				if (prev != 0){
					prev->next = c->next;
					c->next = head;
					head = c;
					}
				return (0);
			}
		}
		if (cnt >= CACHESZ) {
			detachnode(prev, c);
			freenode(c);
			break;
		}
	}

	for (;;) {

		if (reason = _yp_dobind(domain, &pdomb) ) {
			return (reason);
		}

		dofun = (pdomb->dom_vers == YPVERS) ? v2domatch : v1domatch;

		reason = (*dofun)(domain, map, key, keylen, pdomb,
		    _ypserv_timeout, val, vallen);

		if (reason == YPERR_RPC) {
			yp_unbind(domain);
			(void) sleep(_ypsleeptime);
		} else {
			break;
		}
	}

	/* add to our cache */
	if (reason == 0) {
		/*
		 * NB: allocate and copy extract two bytes of the value;
		 * these two bytes are mandatory CR and NULL bytes.
		 */
		savesize = *vallen + 2;
		c = makenode(domain, map, keylen, savesize);
		if (c != 0) {
			(void) gettimeofday(&now, &tz);
			c->next = head;
			head = c;
			c->birth = now.tv_sec;
			(void) strcpy(c->domain, domain);
			(void) strcpy(c->map, map);
			bcopy(key, c->key, c->keylen = keylen);
			bcopy(*val, c->val, savesize);
			c->vallen = *vallen;
		}
	}
	return (reason);
}

/*
 * This talks v2 protocol to ypserv
 */
static int
v2domatch (domain, map, key, keylen, pdomb, timeout, val, vallen)
	char *domain;
	char *map;
	char *key;
	int  keylen;
	struct dom_binding *pdomb;
	struct timeval timeout;
	char **val;		/* return: value array */
	int  *vallen;		/* return: bytes in val */
{
	struct ypreq_key req;
	struct ypresp_val resp;
	unsigned int retval = 0;

	req.domain = domain;
	req.map = map;
	req.keydat.dptr = key;
	req.keydat.dsize = keylen;

	resp.valdat.dptr = NULL;
	resp.valdat.dsize = 0;

	/*
	 * Do the match request.  If the rpc call failed, return with status
	 * from this point.
	 */

	if(clnt_call(pdomb->dom_client,
	    YPPROC_MATCH, xdr_ypreq_key, &req, xdr_ypresp_val, &resp,
	    timeout) != RPC_SUCCESS) {
		return (YPERR_RPC);
	}

	/* See if the request succeeded */

	if (resp.status != YP_TRUE) {
		retval = ypprot_err((unsigned) resp.status);
	}

	/* Get some memory which the user can get rid of as he likes */

	if (!retval && ((*val = malloc((unsigned)
	    resp.valdat.dsize + 2)) == NULL)) {
		retval = YPERR_RESRC;
	}

	/* Copy the returned value byte string into the new memory */

	if (!retval) {
		*vallen = resp.valdat.dsize;
		bcopy(resp.valdat.dptr, *val, resp.valdat.dsize);
		(*val)[resp.valdat.dsize] = '\n';
		(*val)[resp.valdat.dsize + 1] = '\0';
	}

	CLNT_FREERES(pdomb->dom_client, xdr_ypresp_val, &resp);
	return (retval);

}

/*
 * This talks v1 protocol to ypserv
 */
static int
v1domatch (domain, map, key, keylen, pdomb, timeout, val, vallen)
	char *domain;
	char *map;
	char *key;
	int  keylen;
	struct dom_binding *pdomb;
	struct timeval timeout;
	char **val;		/* return: value array */
	int  *vallen;		/* return: bytes in val */
{
	struct yprequest req;
	struct ypresponse resp;
	unsigned int retval = 0;

	req.yp_reqtype = YPMATCH_REQTYPE;
	req.ypmatch_req_domain = domain;
	req.ypmatch_req_map = map;
	req.ypmatch_req_keyptr = key;
	req.ypmatch_req_keysize = keylen;

	resp.ypmatch_resp_valptr = NULL;
	resp.ypmatch_resp_valsize = 0;

	/*
	 * Do the match request.  If the rpc call failed, return with status
	 * from this point.
	 */

	if(clnt_call(pdomb->dom_client,
	    YPOLDPROC_MATCH, _xdr_yprequest, &req, _xdr_ypresponse, &resp,
	    timeout) != RPC_SUCCESS) {
		return (YPERR_RPC);
	}

	/* See if the request succeeded */

	if (resp.ypmatch_resp_status != YP_TRUE) {
		retval = ypprot_err((unsigned) resp.ypmatch_resp_status);
	}

	/* Get some memory which the user can get rid of as he likes */

	if (!retval && ((*val = malloc((unsigned)
	    resp.ypmatch_resp_valsize + 2)) == NULL)) {
		retval = YPERR_RESRC;
	}

	/* Copy the returned value byte string into the new memory */

	if (!retval) {
		*vallen = resp.ypmatch_resp_valsize;
		bcopy(resp.ypmatch_resp_valptr, *val,
		    resp.ypmatch_resp_valsize);
		(*val)[resp.ypmatch_resp_valsize] = '\n';
		(*val)[resp.ypmatch_resp_valsize + 1] = '\0';
	}

	CLNT_FREERES(pdomb->dom_client, _xdr_ypresponse, &resp);
	return (retval);

}
