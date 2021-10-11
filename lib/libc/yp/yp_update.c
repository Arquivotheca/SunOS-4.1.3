#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)yp_update.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif
/*
 * Copyright (C) 1990, Sun Microsystems, Inc.
 */

/*
 * Network Information Services updater interface
 */
#include <stdio.h>
#include <rpc/rpc.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/ypupdate_prot.h>
#include <des_crypt.h>

#define WINDOW (60*60)
#define TOTAL_TIMEOUT	300	

/*
 * Turn off debugging 
 */
#define debugging 0
/*#define debug(msg) fprintf(stderr,"%s",msg);
*/
#define debug(msg)


yp_update(domain, map, op, key, keylen, data, datalen)
	char *domain;
	char *map;
	unsigned op;
	char *key;
	int keylen;
	char *data;
	int datalen;
{
	struct ypupdate_args args;
	u_int rslt;	
	struct timeval total;
	CLIENT *client;
	struct sockaddr_in server_addr;
	char *ypmaster;
	char ypmastername[MAXNETNAMELEN+1];
	enum clnt_stat stat;
	u_int proc;

	switch (op) {
	case YPOP_DELETE:
		proc = YPU_DELETE;
		break;	
	case YPOP_INSERT:
		proc = YPU_INSERT;
		break;	
	case YPOP_CHANGE:
		proc = YPU_CHANGE;
		break;	
	case YPOP_STORE:
		proc = YPU_STORE;
		break;	
	default:
		return(YPERR_BADARGS);
	}
	if (yp_master(domain, map, &ypmaster) != 0) {
		debug("no master found");
		return (YPERR_BADDB);	
	}

	client = clnt_create(ypmaster, YPU_PROG, YPU_VERS, "tcp");
	if (client == NULL) {
		if (debugging) {
			clnt_pcreateerror("client create failed");
		}
		free(ypmaster);
		return (YPERR_YPSERV); /*really yp_update*/
	}

	if (! host2netname(ypmastername, ypmaster, domain)) {
		free(ypmaster);
		return (YPERR_BADARGS);
	}
	free(ypmaster);
	clnt_control(client, CLGET_SERVER_ADDR, &server_addr);
	client->cl_auth = authdes_create(ypmastername, WINDOW, 
					 &server_addr, NULL);
	if (client->cl_auth == NULL) {
		debug("auth create failed");
		clnt_destroy(client);
		return (YPERR_RESRC);	/* local auth failure */
	}

	args.mapname = map;	
	args.key.yp_buf_len = keylen;
	args.key.yp_buf_val = key;
	args.datum.yp_buf_len = datalen;
	args.datum.yp_buf_val = data;

	total.tv_sec = TOTAL_TIMEOUT; total.tv_usec = 0;
	clnt_control(client, CLSET_TIMEOUT, &total);
	stat = clnt_call(client, proc,
		xdr_ypupdate_args, &args,
		xdr_u_int, &rslt, total);

	if (stat != RPC_SUCCESS) {
		debug("ypu call failed");
		if (debugging) clnt_perror(client, "ypu call failed");
		if (stat == RPC_AUTHERROR ){
			rslt = YPERR_ACCESS;	/*remote auth failure*/
			} else {
			rslt = YPERR_RPC;
			}
	}
	auth_destroy(client->cl_auth);
	clnt_destroy(client);
	return (rslt);
}
