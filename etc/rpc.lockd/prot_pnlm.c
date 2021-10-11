#ifndef lint
static char sccsid[] = "@(#)prot_pnlm.c 1.1 92/07/30 SMI";
#endif

	/*
	 * Copyright (c) 1988 by Sun Microsystems, Inc.
	 */

	/*
	 * prot_pnlm.c
	 * consists of all procedures called bu nlm_prog
	 */

#include <stdio.h>
#include <sys/fcntlcom.h>
#include <sys/socket.h>
#include <netdb.h>
#include "prot_lock.h"

/* blocking lock list contains all lock requests that r blocked */
static struct reclock		*blocking_req = NULL;

extern int 		debug;
extern SVCXPRT 		*nlm_transp;

extern remote_result	res_working;

extern msg_entry 	*search_msg();
extern remote_result 	*local_lock();
extern remote_result 	*local_unlock();
extern remote_result 	*local_test();
extern remote_result    *local_grant();
extern remote_result 	*cont_test();
extern remote_result 	*cont_lock();
extern remote_result 	*cont_unlock();
extern remote_result 	*cont_cancel();
extern remote_result 	*cont_reclaim();
extern remote_result    *cont_grant();

extern nlm_testargs 	*get_nlm_testargs();
extern nlm_lockargs 	*get_nlm_lockargs();
extern nlm_unlockargs 	*get_nlm_unlockargs();
extern nlm_cancargs 	*get_nlm_cancargs();
extern nlm_testres	*get_nlm_testres();

extern char *copy_str();
extern struct reclock *find_block_req();
extern void dequeue_block_req();
extern int queue_block_req();

proc_nlm_test(a)
	struct reclock *a;
{
	remote_result *result;

	if (debug)
		printf("proc_nlm_test(%x) \n", a);
	result = local_test(a);
	nlm_reply(NLM_TEST, result, a);
	a->rel= 1;
}

proc_nlm_lock(a)
	struct reclock *a;
{
	remote_result *result;
	int grant_lock_flag = FALSE;

	if (debug)
		printf("enter proc_nlm_lock(%x) \n", a);
	result = local_lock(a, &grant_lock_flag);
	nlm_reply(NLM_LOCK, result, a);

	/* if the lock is blocked, add it into the blocking lock list */
	/* and DO NOT free the reclock 'cause its being chained	      */
	if (result->lstat == blocking) {
		if (!queue_block_req(a))
			a->rel = 1;
	} else
		a->rel= 1;

	if (grant_lock_flag) {
		/*
		 * call local_lock() again to see if we can grant any
		 * locks thats freed from the result of this 
		 * downgrding.
		 * NOTE that we reply to the lock request first before
		 * handling these granted locks
		 */
		result = local_lock(a, &grant_lock_flag);
	}
}

proc_nlm_cancel(a)
	struct reclock *a;
{
	remote_result *result;
	struct reclock *req;
	int grant_lock_flag = FALSE;

	if (debug)
		printf("enter proc_nlm_cancel(%x) \n", a);
	result = local_unlock(a, &grant_lock_flag);
	nlm_reply(NLM_CANCEL, result, a);

	if (grant_lock_flag) {
		/* call local_unlock() again to see if we can grant any locks */
		/* thats freed from the result of this unlock 		      */
		/* NOTE that we reply to the unlock request first before      */
		/* handling these granted locks				      */
		result = local_unlock(a, &grant_lock_flag);
	}

	/* rm the prev blocking request if its queued */
	if ((req = find_block_req(a)) != NULL)
		dequeue_block_req(req);

	a->rel= 1;
}

proc_nlm_unlock(a)
	struct reclock *a;
{
	remote_result *result;
	int grant_lock_flag = FALSE;

	if (debug)
		printf("enter proc_nlm_unlock(%x) \n", a);
	result = local_unlock(a, &grant_lock_flag);
	nlm_reply(NLM_UNLOCK, result, a);

	if (grant_lock_flag) {
		/* call local_unlock() again to see if we can grant any locks */
		/* thats freed from the result of this unlock 		      */
		/* NOTE that we reply to the unlock request first before      */
		/* handling these granted locks				      */
		result = local_unlock(a, &grant_lock_flag);
	}
	a->rel= 1;
}

proc_nlm_granted(a)
        struct reclock *a;
{
        remote_result *result;
 

        if (debug)
                printf("enter proc_nlm_granted(%x) \n", a);
        result = local_grant(a);
        nlm_reply(NLM_GRANTED, result, a);
	a->rel= 1;
}

proc_nlm_test_msg(a)
	struct reclock *a;
{
	remote_result *result;

	if (debug)
		printf("enter proc_nlm_test_msg(%x)\n", a);
	result = local_test(a);
	nlm_reply(NLM_TEST_MSG, result, a);
	a->rel= 1;
}

proc_nlm_lock_msg(a)
	struct reclock *a;
{
	remote_result *result;
	int grant_lock_flag = FALSE;

	if (debug)
		printf("enter proc_nlm_lock_msg(%x)\n", a);
	result = local_lock(a, &grant_lock_flag);
	nlm_reply(NLM_LOCK_MSG, result, a);

	/* if the lock is blocked, add it into the blocking lock list */
	/* and DO NOT free the reclock 'cause its being chained	      */
	/* NOTE that we reply to the unlock request first before      */
	/* handling these granted locks				      */
	if (result->lstat == blocking) {
		if (!queue_block_req(a))
			a->rel = 1;
	} else
		a->rel= 1;

	if (grant_lock_flag) {
		/*
		 * call local_lock() again to see if we can grant any
		 * locks thats freed from the result of this 
		 * downgrding.
		 * NOTE that we reply to the lock request first before
		 * handling these granted locks
		 */
		result = local_lock(a, &grant_lock_flag);
	}
}

proc_nlm_cancel_msg(a)
	struct reclock *a;
{
	remote_result *result;
	struct reclock *req;
	int grant_lock_flag = FALSE;

	if (debug)
		printf("enter proc_nlm_cancel_msg(%x)\n", a);
	result = local_unlock(a, &grant_lock_flag);
	nlm_reply(NLM_CANCEL_MSG, result, a);

	if (grant_lock_flag) {
		/* call local_unlock() again to see if we can grant any locks */
		/* thats freed from the result of this unlock 		      */
		/* NOTE that we reply to the unlock request first before      */
		/* handling these granted locks				      */
		result = local_unlock(a, &grant_lock_flag);
	}

	/* rm the prev blocking request if its queued */
	if ((req = find_block_req(a)) != NULL)
		dequeue_block_req(req);

	a->rel= 1;
}

proc_nlm_unlock_msg(a)
	struct reclock *a;
{
	remote_result *result;
	int grant_lock_flag = FALSE;

	if (debug)
		printf("enter proc_nlm_unlock_msg(%x)\n", a);
	result = local_unlock(a, &grant_lock_flag);
	nlm_reply(NLM_UNLOCK_MSG, result, a);

	if (grant_lock_flag) {
		/* call local_unlock() again to see if we can grant any locks */
		/* thats freed from the result of this unlock 		      */
		/* NOTE that we reply to the unlock request first before      */
		/* handling these granted locks				      */
		result = local_unlock(a, &grant_lock_flag);
	}
	a->rel= 1;
}

proc_nlm_granted_msg(a)
	struct reclock *a;
{
	remote_result *result;

	if (debug)
		printf("enter proc_nlm_granted_msg(%x)\n", a);
	result = local_grant(a);
	nlm_reply(NLM_GRANTED_MSG, result, a);
	a->rel= 1;
}


/*
 * return rpc calls;
 * if rpc calls, directly reply to the request;
 * if msg passing calls, initiates one way rpc call to reply!
 */
nlm_reply(proc, reply, a)
	int proc;
	remote_result *reply;
	struct reclock *a;
{
	bool_t (*xdr_reply)();
	int act;
	int nlmreply = 1;
	int newcall = 2;
	int rpc_err;
	char *name;
	int valid;

	switch (proc) {
	case NLM_TEST:
		xdr_reply = xdr_nlm_testres;
		act = nlmreply;
		break;
	case NLM_GRANTED:
	case NLM_LOCK:
	case NLM_CANCEL:
	case NLM_UNLOCK:
		xdr_reply = xdr_nlm_res;
		act = nlmreply;
		break;
	case NLM_TEST_MSG:
		xdr_reply = xdr_nlm_testres;
		act = newcall;
		proc = NLM_TEST_RES;
		name = a->lck.clnt;
		if (a->lck.lox.type == F_UNLCK) {
			reply->lstat = nlm_granted;
		} else {
			reply->lstat = nlm_denied;
			reply->stat.nlm_testrply_u.holder.svid = a->lck.lox.pid;
			reply->stat.nlm_testrply_u.holder.l_offset =
				a->lck.lox.base;
			reply->stat.nlm_testrply_u.holder.l_len =
				a->lck.lox.length;
			if (a->lck.lox.type == F_WRLCK)
				reply->stat.nlm_testrply_u.holder.exclusive = TRUE;
			else
				reply->stat.nlm_testrply_u.holder.exclusive = FALSE;
		}
		if (debug) {
			printf("NLM_REPLY : stat=%d svid=%d l_offset=%d l_len=%d\n",
				reply->lstat,
				reply->stat.nlm_testrply_u.holder.svid,
				reply->stat.nlm_testrply_u.holder.l_offset,
				reply->stat.nlm_testrply_u.holder.l_len);
		}
		break;
	case NLM_LOCK_MSG:
		xdr_reply = xdr_nlm_res;
		act = newcall;
		proc = NLM_LOCK_RES;
		name = a->lck.clnt;
		break;
	case NLM_CANCEL_MSG:
		xdr_reply = xdr_nlm_res;
		act = newcall;
		proc = NLM_CANCEL_RES;
		name = a->lck.clnt;
		break;
	case NLM_UNLOCK_MSG:
		xdr_reply = xdr_nlm_res;
		act = newcall;
		proc = NLM_UNLOCK_RES;
		name = a->lck.clnt;
		break;
	case NLM_GRANTED_MSG:
		xdr_reply = xdr_nlm_res;
		act = newcall;
		proc = NLM_GRANTED_RES;
		name = a->lck.clnt;
		break;
	default:
		printf("unknown nlm_reply proc value: %d\n", proc);
		return;
	}
	if (act == nlmreply) { /* reply to nlm_transp */
		if (debug)
			printf("rpc nlm_reply %d: %d\n", proc, reply->lstat);
		if (!svc_sendreply(nlm_transp, xdr_reply, reply))
			svcerr_systemerr(nlm_transp);
		return;
	}
	else { /* issue a one way rpc call to reply */
		if (debug)
			printf("nlm_reply: (%s, %d), result = %d\n",
			name, proc, reply->lstat);

		/* malloc a copy of cookie for global variable reply */
		if (a->cookie.n_len) {
			if (obj_copy(&reply->cookie, &a->cookie) == -1) {
				reply->cookie.n_len= 0;
				reply->cookie.n_bytes= NULL;
			}
		} else {
			reply->cookie.n_len= 0;
			reply->cookie.n_bytes= NULL;
		}

		valid = 1;
		if ((rpc_err = call_udp(name, NLM_PROG, NLM_VERS, proc,
			xdr_reply, reply, xdr_void, NULL, valid, 0))
			!= (int) RPC_TIMEDOUT && rpc_err != RPC_CANTSEND) {
			/* in case of error, print out error msg */
			clnt_perrno(rpc_err);
			fprintf(stderr, "\n");
		} else if (rpc_err == RPC_CANTSEND) {
			if ((rpc_err = call_udp(name, NLM_PROG, NLM_VERS, proc,
				xdr_reply, reply, xdr_void, NULL, 0, 0))
				!= (int) RPC_TIMEDOUT &&
				rpc_err != (int) RPC_CANTSEND) {
				/* in case of error, print out error msg */
				clnt_perrno(rpc_err);
				fprintf(stderr, "\n");
			}
		}

		/* free up the cookie that was malloc'ed earlier */
		if (reply->cookie.n_len) {
			xfree(&reply->cookie.n_bytes);
			reply->cookie.n_len = 0;
			reply->cookie.n_bytes = NULL;
		}
	}
}

proc_nlm_test_res(reply)
	remote_result *reply;
{
	nlm_res_routine(reply, cont_test);
}

proc_nlm_lock_res(reply)
	remote_result *reply;
{
	nlm_res_routine(reply, cont_lock);
}

proc_nlm_cancel_res(reply)
	remote_result *reply;
{
	nlm_res_routine(reply, cont_cancel);
}

proc_nlm_unlock_res(reply)
	remote_result *reply;
{
	nlm_res_routine(reply, cont_unlock);
}

proc_nlm_granted_res(reply)
        remote_result *reply;
{
	nlm_res_routine(reply, cont_grant);
}

/*
 * common routine shared by all nlm routines that expects replies from svr nlm:
 * nlm_lock_res, nlm_test_res, nlm_unlock_res, nlm_cancel_res
 * private routine "cont" is called to continue local operation;
 * reply is match with msg in msg_queue according to cookie
 * and then attached to msg_queue;
 */
nlm_res_routine(reply, cont)
	remote_result *reply;
	remote_result *(*cont)();
{
	msg_entry *msgp;
	remote_result *resp;

	if (debug) {
		printf("enter nlm_res_routine...\n");
		pr_all();
		(void) fflush(stdout);
	}
	if ((msgp = search_msg(reply)) != NULL) {	/* found */
		if (msgp->reply != NULL) { /* reply already exists */
			if (msgp->reply->lstat != reply->lstat) {
				if (debug)
					printf("inconsistent reply (%d, %d) exists for lock(%x)\n", msgp->reply->lstat, reply->lstat, msgp->req);
			}
			release_res(reply);
			return;
		}
		/* continue process req according to remote reply */
		if (msgp->proc == NLM_LOCK_RECLAIM)
			/* reclaim response */
			resp = cont_reclaim(msgp->req, reply);
		else
			/* normal response */
			resp = cont(msgp->req, reply);

		if (resp == NULL)	return;

		/* if we got the result for a locking request, we need to */
		/* add it to our status monitor list			  */	
		if ((msgp->proc == NLM_LOCK) || (msgp->proc == NLM_LOCK_MSG))
			(void) add_req_to_me(msgp->req, resp->lstat);

		add_reply(msgp, resp);

		/* if the result is for CANCEL msgs, remove the queued 	*/
		/* request, we do not need to reply to the kernel 	*/
		/* 'cause we have replied to it when requested for the 	*/
		/* 1st time that its granted, the msg was queued to 	*/
		/* ensure that the msg sent to the client is ack'ed	*/
		if ((msgp->proc == NLM_CANCEL) || 
		    (msgp->proc == NLM_CANCEL_MSG) ||
		    (msgp->proc == NLM_LOCK_RECLAIM))	
			dequeue(msgp);
	}
	else
		release_res(reply);	/* discard this resply */
}

/*
 * rpc msg passing calls to nlm msg procedure;
 * used by local_lock, local_test and local_unloc;
 * proc specifis the name of nlm procedures;
 * retransmit indicate whether this is retransmission;
 * rpc_call return -1 if rpc call is not successful, clnt_perrno is printed out;
 * rpc_call return 0 otherwise
 */
nlm_call(proc, a, retransmit)
	int proc;
	struct reclock *a;
	int retransmit;
{
	int 		rpc_err;
	bool_t 		(*xdr_arg)();
	char 		*name, *args;
	int 		func;
	int 		valid;
	nlm_testargs 	*nlm_testargs_a;
	nlm_lockargs 	*nlm_lockargs_a;
	nlm_cancargs 	*nlm_cancargs_a;
	nlm_unlockargs 	*nlm_unlockargs_a;

	func = proc;		/* this is necc for NLM_LOCK_RECLAIM */
	if (retransmit == 0)
		valid = 1;	/* use cache value for first time calls */
	else
		valid = 0;	/* invalidate cache */
	switch (proc) {
	case NLM_TEST_MSG:
		xdr_arg = xdr_nlm_testargs;
		name = a->lck.svr;
		if ((nlm_testargs_a = get_nlm_testargs()) == NULL) {
			printf("rpc.lockd: unable to allocate nlm_testargs.\n");
			return (-1);
		}
		if (reclocktonlm_testargs(a, nlm_testargs_a) == -1) {
			release_nlm_testargs(nlm_testargs_a);
			return(-1);
		}
		args = (char *)nlm_testargs_a;
		break;

	case NLM_LOCK_RECLAIM:
		func = NLM_LOCK_MSG;
		valid = 0;	/* turn off udp cache */

	case NLM_LOCK_MSG:
		xdr_arg = xdr_nlm_lockargs;
		name = a->lck.svr;
		if ((nlm_lockargs_a = get_nlm_lockargs()) == NULL) {
			printf("rpc.lockd: unable to allocate nlm_lockargs.\n");
			return (-1);
		}
		if (reclocktonlm_lockargs(a, nlm_lockargs_a) == -1) {
			release_nlm_lockargs(nlm_lockargs_a);
			return(-1);
		}
		if (proc == NLM_LOCK_RECLAIM)
			nlm_lockargs_a->reclaim = TRUE;
		args = (char *)nlm_lockargs_a;
		break;

	case NLM_CANCEL_MSG:
		xdr_arg = xdr_nlm_cancargs;
		name = a->lck.svr;
		if ((nlm_cancargs_a = get_nlm_cancargs()) == NULL) {
			printf("rpc.lockd: unable to allocate nlm_cancargs.\n");
			return (-1);
		}
		if (reclocktonlm_cancargs(a, nlm_cancargs_a) == -1) {
			release_nlm_cancargs(nlm_cancargs_a);
			return(-1);
		}
		args = (char *)nlm_cancargs_a;
		break;

	case NLM_UNLOCK_MSG:
		xdr_arg = xdr_nlm_unlockargs;
		name = a->lck.svr;
		if ((nlm_unlockargs_a = get_nlm_unlockargs()) == NULL) {
			printf("rpc.lockd: unable to allocate nlm_unlockargs.\n");
			return (-1);
		}
		if (reclocktonlm_unlockargs(a, nlm_unlockargs_a) == -1) {
			release_nlm_unlockargs(nlm_unlockargs_a);
			return(-1);
		}
		args = (char *)nlm_unlockargs_a;
		break;
	
	case NLM_GRANTED_MSG:
                xdr_arg = xdr_nlm_testargs;
                name = a->lck.svr;

                if ((nlm_testargs_a = get_nlm_testargs()) == NULL) {
                        printf("rpc.lockd: unable to allocate nlm_testargs.\n");                        return (-1);
                }
                if (reclocktonlm_testargs(a, nlm_testargs_a) == -1) {
			release_nlm_testargs(nlm_testargs_a);
			return(-1);
		}
                args = (char *)nlm_testargs_a;
                break;

	default:
		printf("%d not supported in nlm_call\n", proc);
		return (-1);
	}

	if (debug)
		printf("nlm_call to (%s, %d) op=%d, (%d, %d); retran = %d, valid = %d\n",
			name, proc, a->lck.op, a->lck.lox.base,
			a->lck.lox.length, retransmit, valid);

	/*
	 * call is a one way rpc call to simulate msg passing
	 * no timeout nor reply is specified;
	 */
	if ((rpc_err = call_udp(name, NLM_PROG, NLM_VERS, func, xdr_arg,
		args, xdr_void, NULL, valid, 0)) == (int)RPC_TIMEDOUT ) {
		/*
		 * if rpc call is successful, add msg to msg_queue
		 */
		if (retransmit == 0)	/* first time calls */
			if (queue(a, proc) == NULL) {
				release_nlm_req(proc, args);
				return (-1);
			}
		release_nlm_req(proc, args);
		return (0);
	} else {
		if (debug) {
			clnt_perrno(rpc_err);
			fprintf(stderr, "\n");
		}	
		release_nlm_req(proc, args);
		return (-1);
	}
}

int
reclocktonlm_lockargs(from, to)
	struct reclock *from;
	nlm_lockargs   *to;
{
	if (from->cookie.n_len) {
		if (obj_copy(&to->cookie, &from->cookie) == -1)
			return(-1);
	} else
		to->cookie.n_bytes= 0;
	if (from->block)
		to->block = TRUE;
	else
		to->block = FALSE;
	if (from->lck.lox.type == F_WRLCK)
		to->exclusive = TRUE;
	else
		to->exclusive = FALSE;
	if ((to->alock.caller_name = copy_str(from->alock.caller_name)) == NULL)
		return(-1);
	if (from->alock.fh.n_len) {
		if (obj_copy(&to->alock.fh, &from->alock.fh) == -1)
			return(-1);
	} else
		to->alock.fh.n_len = 0;
	if (from->alock.oh.n_len) {
		if (obj_copy(&to->alock.oh, &from->alock.oh) == -1)
			return(-1);
	} else
		to->alock.oh.n_len = 0;
	to->alock.svid = from->alock.lox.pid;
	to->alock.l_offset = (((int)from->alock.lox.base) < 0) ? 0x7fffffff : from->alock.lox.base;
	to->alock.l_len = from->alock.lox.length;
	to->reclaim = from->reclaim;
	to->state = from->state;

	return(0);
}

int
reclocktonlm_cancargs(from, to)
	struct reclock *from;
	nlm_cancargs   *to;
{
	if (from->cookie.n_len) {
		if (obj_copy(&to->cookie, &from->cookie) == -1)
			return(-1);
	} else
		to->cookie.n_bytes= 0;
	if (from->block)
		to->block = TRUE;
	else
		to->block = FALSE;
	if (from->lck.lox.type == F_WRLCK)
		to->exclusive = TRUE;
	else
		to->exclusive = FALSE;
	if ((to->alock.caller_name = copy_str(from->alock.caller_name)) == NULL)		return(-1);
	if (from->alock.fh.n_len) {
		if (obj_copy(&to->alock.fh, &from->alock.fh) == -1)
			return(-1);
	} else
		to->alock.fh.n_len = 0;
	if (from->alock.oh.n_len) {
		if (obj_copy(&to->alock.oh, &from->alock.oh) == -1)
			return(-1);
	} else
		to->alock.oh.n_len = 0;
	to->alock.svid = from->alock.lox.pid;
	to->alock.l_offset = (((int)from->alock.lox.base) < 0) ? 0x7fffffff : from->alock.lox.base;
	to->alock.l_len = from->alock.lox.length;

	return(0);
}

int
reclocktonlm_unlockargs(from, to)
	struct reclock *from;
	nlm_unlockargs *to;
{
	if (from->cookie.n_len) {
		if (obj_copy(&to->cookie, &from->cookie) == -1)
			return(-1);
	} else
		to->cookie.n_bytes= 0;

	if ((to->alock.caller_name = copy_str(from->alock.caller_name)) == NULL)
		return(-1);
	if (from->alock.fh.n_len) {
		if (obj_copy(&to->alock.fh, &from->alock.fh) == -1)
			return(-1);
	} else
		to->alock.fh.n_len = 0;
	if (from->alock.oh.n_len) {
		if (obj_copy(&to->alock.oh, &from->alock.oh) == -1)
			return(-1);
	} else
		to->alock.oh.n_len = 0;
	to->alock.svid = from->alock.lox.pid;
	to->alock.l_offset = (((int)from->alock.lox.base) < 0) ? 0x7fffffff : from->alock.lox.base;
	to->alock.l_len = from->alock.lox.length;

	return(0);
}

int
reclocktonlm_testargs(from, to)
	struct reclock *from;
	nlm_testargs   *to;
{
	if (from->cookie.n_len) {
		if (obj_copy(&to->cookie, &from->cookie) == -1)
			return(-1);
	} else
		to->cookie.n_bytes= 0;
	if (from->lck.lox.type == F_WRLCK)
		to->exclusive = TRUE;
	else
		to->exclusive = FALSE;
	if ((to->alock.caller_name = copy_str(from->alock.caller_name)) == NULL)
		return(-1);
	if (from->alock.fh.n_len) {
		if (obj_copy(&to->alock.fh, &from->alock.fh) == -1)
			return(-1);
	} else
		to->alock.fh.n_len = 0;
	if (from->alock.oh.n_len) {
		if (obj_copy(&to->alock.oh, &from->alock.oh) == -1)
			return(-1);
	} else
		to->alock.oh.n_len = 0;
	to->alock.svid = from->alock.lox.pid;
	to->alock.l_offset = (((int)from->alock.lox.base) < 0) ? 0x7fffffff : from->alock.lox.base;
	to->alock.l_len = from->alock.lox.length;

	return(0);
}

int
nlm_lockargstoreclock(from, to)
	nlm_lockargs   *from;
	struct reclock *to;
{
	if (from->cookie.n_len) {
		if (obj_copy(&to->cookie, &from->cookie) == -1)
			return(-1);
	} else
		to->cookie.n_bytes= 0;
	if (from->block)
		to->block = TRUE;
	else
		to->block = FALSE;
	if (from->exclusive) {
		to->exclusive = TRUE;
		to->alock.lox.type = F_WRLCK;
	} else {
		to->exclusive = FALSE;
		to->alock.lox.type = F_RDLCK;
	}
	if ((to->alock.caller_name = copy_str(from->alock.caller_name)) == NULL)
		return(-1);
	if (from->alock.fh.n_len) {
		if (obj_copy(&to->alock.fh, &from->alock.fh) == -1)
			return(-1);
	} else
		to->alock.fh.n_len = 0;
	if (from->alock.oh.n_len) {
		if (obj_copy(&to->alock.oh, &from->alock.oh) == -1)
			return(-1);
	} else
		to->alock.oh.n_len = 0;
	to->alock.lox.pid = from->alock.svid;
	to->alock.lox.base = (((int)from->alock.l_offset) < 0) ? 0x7fffffff : from->alock.l_offset;
	to->alock.lox.length = from->alock.l_len;
	to->alock.lox.rpid = from->alock.svid;
	to->alock.lox.rsys = nlm_transp->xp_raddr.sin_addr.s_addr;
	to->alock.lox.class = LOCKMGR;
	to->reclaim = from->reclaim;

	return(0);
}

int
nlm_unlockargstoreclock(from, to)
	nlm_unlockargs *from;
	struct reclock *to;
{
	if (from->cookie.n_len) {
		if (obj_copy(&to->cookie, &from->cookie) == -1)
			return(-1);
	} else
		to->cookie.n_bytes= 0;
	if ((to->alock.caller_name = copy_str(from->alock.caller_name)) == NULL)
		return(-1);
	if (from->alock.fh.n_len ) {
		if (obj_copy(&to->alock.fh, &from->alock.fh) == -1)
			return(-1);
	} else
		to->alock.fh.n_len = 0;
	if (from->alock.oh.n_len) {
		if (obj_copy(&to->alock.oh, &from->alock.oh) == -1)
			return(-1);
	} else
		to->alock.oh.n_len = 0;
	to->alock.lox.pid = from->alock.svid;
	to->alock.lox.base = (((int)from->alock.l_offset) < 0) ? 0x7fffffff : from->alock.l_offset;
	to->alock.lox.length = from->alock.l_len;
	to->alock.lox.rpid = from->alock.svid;
	to->alock.lox.rsys = nlm_transp->xp_raddr.sin_addr.s_addr;
	to->alock.lox.class = LOCKMGR;

	return(0);
}

int
nlm_cancargstoreclock(from, to)
	nlm_cancargs   *from;
	struct reclock *to;
{
	if (from->cookie.n_len) {
		if (obj_copy(&to->cookie, &from->cookie) == -1)
			return(-1);
	} else
		to->cookie.n_bytes= 0;
	to->block = from->block;
	if (from->exclusive) {
                to->exclusive = TRUE;
                to->alock.lox.type = F_WRLCK;
        } else {
                to->exclusive = FALSE;
                to->alock.lox.type = F_RDLCK;
        }
	if ((to->alock.caller_name = copy_str(from->alock.caller_name)) == NULL)
		return(-1);
	if (from->alock.fh.n_len) {
		if (obj_copy(&to->alock.fh, &from->alock.fh) == -1)
			return(-1);
	} else
		to->alock.fh.n_len = 0;
	if (from->alock.oh.n_len) {
		if (obj_copy(&to->alock.oh, &from->alock.oh) == -1)
			return(-1);
	} else
		to->alock.oh.n_len = 0;
	to->alock.lox.pid = from->alock.svid;
	to->alock.lox.base = (((int)from->alock.l_offset) < 0) ? 0x7fffffff : from->alock.l_offset;
	to->alock.lox.length = from->alock.l_len;
	to->alock.lox.rpid = from->alock.svid;
	to->alock.lox.rsys = nlm_transp->xp_raddr.sin_addr.s_addr;
	to->alock.lox.class = LOCKMGR;

	return(0);
}

int
nlm_testargstoreclock(from, to)
	nlm_testargs   *from;
	struct reclock *to;
{
	if (from->cookie.n_len) {
		if (obj_copy(&to->cookie, &from->cookie) == -1)
			return(-1);
	} else
		to->cookie.n_bytes= 0;
	if (from->exclusive) {
                to->exclusive = TRUE;
                to->alock.lox.type = F_WRLCK;
        } else {
                to->exclusive = FALSE;
                to->alock.lox.type = F_RDLCK;
        }
	if ((to->alock.caller_name = copy_str(from->alock.caller_name)) == NULL)
		return(-1);
	if (from->alock.fh.n_len) {
		if (obj_copy(&to->alock.fh, &from->alock.fh) == -1)
			return(-1);
	} else
		to->alock.fh.n_len = 0;
	if (from->alock.oh.n_len) {
		if (obj_copy(&to->alock.oh, &from->alock.oh) == -1)
			return(-1);
	} else
		to->alock.oh.n_len = 0;
	to->alock.lox.pid = from->alock.svid;
	to->alock.lox.base = (((int)from->alock.l_offset) < 0) ? 0x7fffffff : from->alock.l_offset;
	to->alock.lox.length = from->alock.l_len;
	to->alock.lox.rpid = from->alock.svid;
	to->alock.lox.rsys = nlm_transp->xp_raddr.sin_addr.s_addr;
	to->alock.lox.class = LOCKMGR;

	return(0);
}

int
nlm_testrestoremote_result(from, to)
	nlm_testres   *from;
	remote_result *to;
{
	if (debug)
		printf("enter nlm_testrestoremote_result..\n");

	if (from->cookie.n_len) {
		if (obj_copy(&to->cookie, &from->cookie) == -1)
			return(-1);
	} else
		to->cookie.n_bytes= 0;
	to->stat.stat = from->stat.stat;
	if (from->stat.nlm_testrply_u.holder.exclusive)
		to->stat.nlm_testrply_u.holder.exclusive = TRUE;
	else
		to->stat.nlm_testrply_u.holder.exclusive = FALSE;
	to->stat.nlm_testrply_u.holder.svid =
		from->stat.nlm_testrply_u.holder.svid;
	if (obj_copy(&to->stat.nlm_testrply_u.holder.oh,
			&from->stat.nlm_testrply_u.holder.oh) == -1)
		return(-1);
	to->stat.nlm_testrply_u.holder.l_offset =
		from->stat.nlm_testrply_u.holder.l_offset;
	to->stat.nlm_testrply_u.holder.l_len =
		from->stat.nlm_testrply_u.holder.l_len;

	return(0);
}

int
nlm_restoremote_result(from, to)
	nlm_res *from;
	remote_result *to;
{
	if (from->cookie.n_len) {
		if (obj_copy(&to->cookie, &from->cookie) == -1)
			return(-1);
	} else
		to->cookie.n_bytes= 0;
	to->stat.stat = from->stat.stat;

	return(0);
}

/*
 * This procedure is ostensibly for mapping from VERS to VERSX.
 * In practice it _probably_ only applies to PC-NFS clients. However
 * the protocol specification specifically allows the length of
 * a lock to be an unsigned 32-bit integer, so it is possible to
 * imagine other cases (especially non-Unix).
 *
 * Even this mapping is inadequate. A better version would simply
 * test the sign bit, and treat ALL "negative" values as 0. We
 * may still get there in this bugfix.
 *
 * Geoff
 */
void
nlm_versx_conversion(procedure, from)
	int	procedure;
	char   *from;
{
	switch (procedure) {
	case NLM_LOCK:
	case NLM_NM_LOCK:
		if(((struct nlm_lockargs *)from)->alock.l_len == 0xffffffff) 
			((struct nlm_lockargs *)from)->alock.l_len= 0;
		return;
	case NLM_UNLOCK:
		if(((struct nlm_unlockargs *)from)->alock.l_len == 0xffffffff) 
			((struct nlm_unlockargs *)from)->alock.l_len= 0;
		return;
	case NLM_CANCEL:
		if(((struct nlm_cancargs *)from)->alock.l_len == 0xffffffff) 
			((struct nlm_cancargs *)from)->alock.l_len= 0;
		return;
	case NLM_TEST:
		if(((struct nlm_testargs *)from)->alock.l_len == 0xffffffff) 
			((struct nlm_testargs *)from)->alock.l_len= 0;
		return;
	default:
		return;
	}

}

/*
 * find_block_req() - given a reclock, chk if this lock request exists in
 *		      the blocking lock lists, if it exists, return the ptr
 *		      of the found reclock in the list
 */
struct reclock *
find_block_req(a)
	struct reclock	*a;
{
	struct reclock *cur = blocking_req;

	while (cur != NULL) {
		/*
		 * When local_unlock is called, the fcntl returns the
		 * lock region for the current requestor if lock is granted.
		 * Check if the lock request is WITHIN the region will ensure
		 * all the blocking lock for the requestor is granted.
		 */
		if (same_type(&(cur->lck.lox), &(a->lck.lox)) && 
		    WITHIN(&(cur->lck.lox), &(a->lck.lox)) &&
		    SAMEOWNER(&(cur->lck.lox), &(a->lck.lox)) &&
		    (cur->lck.lox.rsys == a->lck.lox.rsys))
			return (cur);

		cur = cur->next;
	}
	return (NULL);
}

/*
 * dequeue_block_req() -- remove the reclock lock request from the blocking
 *			  lock list
 */
void
dequeue_block_req(a)
	struct reclock	*a;
{
	struct reclock *cur = blocking_req;
	struct reclock *prev = NULL;

	while (cur != NULL) {
		if (a == cur) {
			if (prev == NULL)
				blocking_req = cur->next;
			else
				prev->next = cur->next;
			cur->rel = 1;
			release_reclock(cur);
			return;
		}
		prev = cur;
		cur = cur->next;
	}
}

/*
 * queue_block_req() -- queue the lock request in the blocking lock list
 */
int
queue_block_req(a)
	struct reclock	*a;
{
	char 	*tmp_ptr;

	/* if its a retransmitted blocking request, do not queue it */
	if (find_block_req(a) != NULL)
		return (FALSE);

	/* since we r re-using the reclock, we need to swap the */
	/* clnt & svr name & caller_name				*/
	tmp_ptr = a->lck.svr;
	a->lck.svr = a->lck.clnt;
	a->lck.clnt = tmp_ptr;
	a->lck.caller_name = tmp_ptr;

	/* add to front of blocking_req list */
	a->next = blocking_req; 
	blocking_req = a;
	return (TRUE);
}
