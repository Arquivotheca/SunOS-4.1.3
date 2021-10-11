#ifndef lint
static char sccsid[] = "@(#)prot_priv.c 1.1 92/07/30 SMI";
#endif

	/*
	 * Copyright (c) 1988 by Sun Microsystems, Inc.
	 */

	/*
	 * consists of all private protocols for comm with
	 * status monitor to handle crash and recovery
	 */

#include <stdio.h>
#include <netdb.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include "prot_lock.h"
#include "priv_prot.h"
#include <rpcsvc/sm_inter.h>

extern int debug;
extern int pid;
extern char hostname[MAXHOSTNAMELEN];
extern int local_state;
extern struct msg_entry *retransmitted();
void proc_priv_crash(), proc_priv_recovery();
void reclaim_locks();
extern struct lm_vnode *find_me();
extern msg_entry *msg_q;	/* head of msg queue */
void reclaim_pending();

int cookie;

void
priv_prog(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	char *(*Local)();
	struct status stat;
	extern bool_t xdr_status();

	if (debug)
		printf("Enter PRIV_PROG ...............\n");

	switch (rqstp->rq_proc) {
	case PRIV_CRASH:
		Local = (char *(*)()) proc_priv_crash;
		break;
	case PRIV_RECOVERY:
		Local = (char *(*)()) proc_priv_recovery;
		break;
	default:
		svcerr_noproc(transp);
		return;
	}

	bzero(&stat, sizeof (struct status));
	if (!svc_getargs(transp, xdr_status, &stat)) {
		svcerr_decode(transp);
		return;
	}
	(*Local)(&stat);
	if (!svc_sendreply(transp, xdr_void, NULL)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_status, &stat)) {
		fprintf(stderr, "unable to free arguments\n");
		exit(1);
	}
}

void
proc_priv_crash(statp)
	struct status *statp;
{
	struct hostent *hp;
	char buf[128];
	struct eflock ld;
	int cmd, fd, err;

	if (debug)
		printf("enter proc_priv_CRASH....\n");

	if ((hp = gethostbyname(statp->mon_name)) == NULL) {
                if (debug)
                        printf( "RPC_UNKNOWNHOST\n");
        }
	sprintf(buf, "%02x%02x%02x%02x",
		(u_char) hp->h_addr[0], (u_char) hp->h_addr[1],
                (u_char) hp->h_addr[2], (u_char) hp->h_addr[3]);
	sscanf(buf,"%x", &ld.l_rsys);

	fd = open("/tmp/foo", O_CREAT|O_RDWR);

	cmd = F_RSETLK;
	ld.l_type = F_UNLKSYS;
	ld.l_whence = 0;
       	ld.l_start = 0;
       	ld.l_len = 0;
       	ld.l_pid = getpid();
       	ld.l_rpid = getpid();
       	if (debug) {
               	printf("ld.l_start=%d ld.l_len=%d ld.l_rpid=%d ld.l_rsys=%x\n",
                       	ld.l_start, ld.l_len, ld.l_rpid, ld.l_rsys);
       	}
       	if ((err = fcntl(fd, cmd, &ld)) == -1) {
               	perror("fcntl");
               	printf("rpc.lockd: unable to clear a lock. \n");
       	}
	close(fd);
	delete_hash(statp->mon_name);
	/*
	 * In case /tmp/foo never get removed.
	 */
	unlink("/tmp/foo");
	return;
}

void
proc_priv_recovery(statp)
	struct status *statp;
{
	struct lm_vnode *mp;
	struct priv_struct *privp;
	char *xmalloc();

	if (debug)
		printf("enter proc_priv_RECOVERY.....\n");
	privp = (struct priv_struct *) statp->priv;
	if (privp->pid != pid) {
		if (debug)
			printf("this is not for me(%d): %d\n", privp->pid, pid);
		return;
	}

	if (debug)
		printf("enter proc_lm_recovery due to %s state(%d)\n",
			statp->mon_name, statp->state);

	destroy_client_shares(statp->mon_name);

	delete_hash(statp->mon_name);
	if (!up(statp->state)) {
		if (debug)
			printf("%s is not up.\n", statp->mon_name);
		return;
	}
	if (strcmp(statp->mon_name, hostname) == 0) {
		if (debug)
			printf("I have been declared as failed!!!\n");
		/*
		 * update local status monitor number
		 */
		local_state = statp->state;
	}

	mp = find_me(statp->mon_name);
	reclaim_locks(mp->exclusive);
	reclaim_locks(mp->shared);
	reclaim_pending(mp->pending);
}

/*
 * reclaim_locks() -- will send out reclaim lock requests to the server.
 *		      listp is the list of established/granted lock requests.
 */
void
reclaim_locks(listp)
	struct reclock *listp;
{
	struct reclock *ff;

	for (ff = listp; ff; ff = ff->next) {
		/* set reclaim flag & send out the request */
		ff->reclaim = 1;
		if (nlm_call(NLM_LOCK_RECLAIM, ff, 0) == -1) {
			if (queue(ff, NLM_LOCK_RECLAIM) == NULL)
				fprintf(stderr,
"reclaim request (%x) cannot be sent and cannot be queued for resend later!\n", ff);
		}
		if (ff->next == listp)	return;
	}
}

/*
 * reclaim_pending() -- will setup the existing queued msgs of the pending
 *		      	lock requests to allow retransmission.
 *			note that reclaim requests for these pending locks
 *			r not sent out.
 */
void
reclaim_pending(listp)
	struct reclock *listp;
{
	msg_entry *msgp;
	struct reclock *ff;

	/* for each pending lock request for the recovered server */
	for (ff = listp; ff; ff = ff->next) {

		/* find the msg in the queue that holds this lock request */
		for (msgp = msg_q; msgp != NULL; msgp = msgp->nxt) {
			
			/* if msg is found, free & nullified the exisiting */
			/* response of this lock request.  this will allow */
			/* retransmission of the requests.		   */
			if (msgp->req == ff) {
				if (msgp->reply != NULL) {
					release_res(msgp->reply);
					msgp->reply = NULL;
				}
				break;
			}
		}
		if (ff->next == listp)	return;
	}
	return;
}

