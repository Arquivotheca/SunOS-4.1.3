#ifndef lint
static char sccsid[] = "@(#)sm_monitor.c 1.1 92/07/30 SMI";
#endif

	/*
	 * Copyright (c) 1988 by Sun Microsystems, Inc.
	 */

	/*
	 * sm_monitor.c:
	 * simple interface to status monitor
	 */

#include "prot_lock.h"
#include "priv_prot.h"
#include <stdio.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <rpcsvc/sm_inter.h>
#include <sys/param.h>
#include "sm_res.h"
#define LM_UDP_TIMEOUT 15
#define RETRY_NOTIFY	10

extern char *xmalloc();
extern int debug;
extern int local_state;
extern char hostname[MAXHOSTNAMELEN];

struct stat_res *
stat_mon(sitename, svrname, my_prog, my_vers, my_proc, func, len, priv)
	char *sitename;
	char *svrname;
	int my_prog, my_vers, my_proc;
	int func;
	int len;
	char *priv;
{
	static struct stat_res Resp;
	static sm_stat_res resp;
	mon mond, *monp;
	mon_id *mon_idp;
	my_id *my_idp;
	char *svr;
 	int 	mon_retries;	/* Times we have tried to contact statd */
	int rpc_err;
	bool_t (*xdr_argument)(), (*xdr_result)();
	char *ip;
	int i;
	int valid;

	if (debug)
		printf("enter stat_mon .sitename=%s svrname=%s func=%d\n",
			sitename, svrname, func);
	monp = &mond;
	mon_idp = &mond.mon_id;
	my_idp = &mon_idp->my_id;

	bzero(monp, sizeof (mon));
	if (svrname == NULL)
		svrname = hostname;
	svr = xmalloc(strlen(svrname)+1);
	(void) strcpy(svr, svrname);
	if (sitename != NULL) {
		mon_idp->mon_name = xmalloc(strlen(sitename)+1);
		(void) strcpy(mon_idp->mon_name, sitename);
	}
	my_idp->my_name= xmalloc(strlen(hostname)+1);
	(void) strcpy(my_idp->my_name, hostname);

	my_idp->my_prog = my_prog;
	my_idp->my_vers = my_vers;
	my_idp->my_proc = my_proc;
	if (len > 16) {
		fprintf(stderr, "stat_mon: len(=%d) is greater than 16!\n", len);
		exit(1);
	}
	if (len != NULL) {
		for (i = 0; i< len; i++) {
			monp->priv[i] = priv[i];
		}
	}

	switch (func) {
	case SM_STAT:
		xdr_argument = xdr_sm_name;
		xdr_result = xdr_sm_stat_res;
		ip =  (char *) &mon_idp->mon_name;
		break;

	case SM_MON:
		xdr_argument = xdr_mon;
		xdr_result = xdr_sm_stat_res;
		ip = (char *)  monp;
		break;

	case SM_UNMON:
		xdr_argument = xdr_mon_id;
		xdr_result = xdr_sm_stat;
		ip =  (char *) mon_idp;
		break;

	case SM_UNMON_ALL:
		xdr_argument = xdr_my_id;
		xdr_result = xdr_sm_stat;
		ip = (char *) my_idp;
		break;

	case SM_SIMU_CRASH:
		xdr_argument = xdr_void;
		xdr_result = xdr_void;
		ip = NULL;
		break;

	default:
		fprintf(stderr, "stat_mon proc(%d) not supported\n", func);
		Resp.res_stat = stat_fail;
		return (&Resp);
	}

	if (debug)
		printf(" request monitor:(svr=%s) mon_name=%s, my_name=%s, func =%d\n",
			svr, sitename, my_idp->my_name, func);
	valid = 1;
	mon_retries = 0;
again:
	if ((rpc_err = call_udp(svr, SM_PROG, SM_VERS, func, xdr_argument, ip,
		xdr_result, &resp, valid, LM_UDP_TIMEOUT)) != (int) RPC_SUCCESS) {
		if (rpc_err == (int) RPC_TIMEDOUT) {
			if (debug) printf("timeout, retry contacting status monitor\n");
			mon_retries++;
			if (mon_retries > RETRY_NOTIFY) {
			    fprintf(stderr, "rpc.lockd: Cannot contact status monitor!\n");
				mon_retries = 0;
			}
			valid = 0;
			goto again;
		}
		else {
			if (debug) {
				clnt_perrno(rpc_err);
				fprintf(stderr, "\n");
			}
			if (mon_idp->mon_name)
				xfree(&mon_idp->mon_name);
			if (my_idp->my_name)
				xfree(&my_idp->my_name);
			if (svr)
				xfree(&svr);
			Resp.res_stat = stat_fail;
			Resp.u.rpc_err = rpc_err;
			return (&Resp);
		}
	} else {
		if (mon_idp->mon_name)
			xfree(&mon_idp->mon_name);
		if (my_idp->my_name)
			xfree(&my_idp->my_name);
		if (svr)
			xfree(&svr);
		Resp.res_stat = stat_succ;
		Resp.u.stat = resp;
		return (&Resp);
	}
}

cancel_mon()
{
	struct stat_res *resp;

	resp = stat_mon(NULL, hostname, PRIV_PROG, PRIV_VERS, PRIV_CRASH, SM_UNMON_ALL, NULL, NULL);
	if (resp->res_stat == stat_fail)
		return;
	resp = stat_mon(NULL, hostname, PRIV_PROG, PRIV_VERS, PRIV_RECOVERY, SM_UNMON_ALL, NULL, NULL);
	resp = stat_mon(NULL, NULL, 0, 0, 0, SM_SIMU_CRASH, NULL, NULL);
	if (resp->res_stat == stat_fail)
		return;
	if (resp->sm_stat == stat_succ)
		local_state = resp->sm_state;
	return;
}
