#ifndef lint
static char sccsid[] = "@(#)prot_proc.c 1.1 92/07/30 SMI";
#endif

	/*
	 * Copyright (c) 1988 by Sun Microsystems, Inc.
	 */

	/*
	 * prot_proc.c
	 * consists all local, remote, and continuation routines:
	 * local_xxx, remote_xxx, and cont_xxx.
	 */

#include <stdio.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include "prot_lock.h"
#include "lockf.h"

remote_result nlm_result;		  /* local nlm result */
remote_result *nlm_resp = &nlm_result;	  /* ptr to klm result */
remote_result *get_res();

remote_result *remote_cancel();
remote_result *remote_lock();
remote_result *remote_test();
remote_result *remote_unlock();
remote_result *local_test();
remote_result *local_lock();
remote_result *local_unlock();
remote_result *cont_test();
remote_result *cont_lock();
remote_result *cont_unlock();
remote_result *cont_cancel();
remote_result *cont_reclaim();

extern msg_entry *retransmitted();

extern int debug, errno;
extern msg_entry *msg_q;
extern bool_t obj_cmp();
extern int obj_copy();
extern struct reclock *find_block_req();
extern void dequeue_block_req();
extern char	*xmalloc();

extern struct fd_table	*fd_table;

static int used_fd = 0;
static int lastfd = 0;

print_fdtable()
{
	struct fd_table	*t;
	int i, j;

	printf("In print_fdtable()....used_fd=%d, lastfd=%d\n",
		used_fd, lastfd);

	for (t = fd_table, j = 0; j <= lastfd; t++, j++) {
		if (t->fd) {
			printf("ID=%d\n", t->fd);
			for (i = 0; i < t->fh.n_len; i++) {
				printf("%02x", (t->fh.n_bytes[i] & 0xff));
			}
			printf("\n");
		}
	}
}

void
remove_fd(fd)
	int	fd;
{
	struct fd_table	*t;
	int i;

	if (debug)
		printf("In remove_fd() ...\n");

	t = &fd_table[fd];
	if (t->fh.n_len) {
		if (debug) {
			for (i = 0; i < t->fh.n_len; i++) {
				printf("%02x",
					(t->fh.n_bytes[i] & 0xff));
			}
			printf("\n");
		}
		bzero(t->fh.n_bytes, sizeof (t->fh.n_bytes));
		xfree(&(t->fh.n_bytes));
		t->fh.n_len = 0;
		t->fd = 0;
		used_fd--;
		if (fd == lastfd)
			lastfd--;
	}
	if (debug)
		print_fdtable();
}

int
get_fd(a)
	struct reclock *a;
{
	struct fd_table	*t;
	int fd, cmd, i;
	int	j;
	struct {
		char    *fh;
		int	filemode;
	} fa;

	if (debug) {
		printf("enter get_fd ....\n");
	}

	for (t = fd_table, j = 0; j <= lastfd; t++, j++) {
		if (t->fd && obj_cmp(&(t->fh), &(a->lck.fh))) {
			if (debug) {
				printf("Found fd entry : a = ");
				for (i = 0; i < a->lck.fh.n_len; i++) {
					printf("%02x",
						(a->lck.fh.n_bytes[i] & 0xff));
				}
				printf("\nfd_table->fh = ");
				for (i = 0; i < 32; i++) {
					printf("%02x",
						(t->fh.n_bytes[i] & 0xff));
				}
				printf("\n");
			}
			return (t->fd);
		}
	}
	/*
	 * convert fh to fd
	 */
	cmd = F_CNVT;
	fa.fh = a->lck.fh.n_bytes;
	if (debug) {
		printf("Convert fd entry : ");
		for (i = 0; i < a->lck.fh.n_len; i++) {
			printf("%02x", (a->lck.fh.n_bytes[i] & 0xff));
		}
		printf("\n");
	}
	fa.filemode = O_RDWR;
	if ((fd = fcntl(0, cmd, &fa)) == -1) {
		perror("fcntl");
		printf("rpc.lockd: unable to do cnvt.\n");
		if ((errno == ENOLCK)||(errno == ESTALE))
			return (-1);
		else
			return (-2);
	}

	if (lastfd < fd)
		lastfd = fd;

	t = &fd_table[fd];
	obj_copy(&t->fh, &a->lck.fh);
	t->fd = fd;
	used_fd++;

	if (debug)
		print_fdtable();
	return (fd);
}

remote_result *
local_lock(a, grant_lock_flag)
	struct reclock *a;
	int *grant_lock_flag;
{
	int err, cmd;
	remote_result *remote_grant();
	struct reclock grant_lock;
	static struct eflock ld;
	static int fd;

	/* if there r locks to be granted, keep calling fcntl() to get 	*/
	/* info on these locks & send each of these granted locks to   	*/
	/* the respective lock mgr, NOTICE that only locks that r held 	*/
	/* by lockmgr is being replied here, granting of local blocking	*/
	/* locks is handled by the kernel.			      	*/
	if (*grant_lock_flag) {
		if (debug)
			printf("granting locks...\n");
		ld.l_type = F_RDLCK;
		ld.l_xxx = GRANT_LOCK_FLAG;
		while (fcntl(fd, F_RSETLKW, &ld) != -1) {
			grant_lock.block = TRUE;
			grant_lock.exclusive = FALSE;
			grant_lock.lck.lox.base = ld.l_start;
			grant_lock.lck.lox.length = ld.l_len;
			grant_lock.lck.lox.pid = ld.l_pid;
			grant_lock.lck.lox.rpid = ld.l_rpid;
			grant_lock.lck.lox.rsys = ld.l_rsys;
			grant_lock.lck.lox.type = ld.l_type;
			grant_lock.rel = 0; 

			/* send this granted lock to the respective remote */
			/* lock mgr					   */
			remote_grant(&grant_lock, MSG);

			/* no more granted locks, quit */
			if (ld.l_xxx < 0)	break;

			ld.l_type = F_RDLCK;
			ld.l_xxx = GRANT_LOCK_FLAG;
		}
		*grant_lock_flag = FALSE;
		return(NULL);
	}

	/*
	 * convert fh to fd
	 */
	if ((fd = get_fd(a)) < 0) {
		if (fd == -1)
			nlm_resp->lstat = nlm_denied_nolocks;
		else
			nlm_resp->lstat = denied;
		return (nlm_resp);
	}

	/*
	 * set the lock
	 */
	if (debug) {
		printf("enter local_lock...FD=%d\n", fd);
		pr_lock(a);
		(void) fflush(stdout);
	}
	if (a->block)
		cmd = F_RSETLKW;
	else
		cmd = F_RSETLK;
	if (a->exclusive)
		ld.l_type = F_WRLCK;
	else
		ld.l_type = F_RDLCK;
	ld.l_whence = 0;
	ld.l_start = a->lck.lox.base;
	ld.l_len = a->lck.lox.length;
	ld.l_pid = a->lck.lox.pid;
	ld.l_rpid = a->lck.lox.rpid;
	ld.l_rsys = a->lck.lox.rsys;
	ld.l_xxx = 0;
	if (debug) {
		printf("ld.l_start=%d ld.l_len=%d ld.l_rpid=%d ld.l_rsys=%x\n",
			ld.l_start, ld.l_len, ld.l_rpid, ld.l_rsys);
	}
	if ((err = fcntl(fd, cmd, &ld)) == -1) {
		if (errno == EINTR) {
			nlm_resp->lstat = blocking;
			a->w_flag = 1;
		} else if (errno == EDEADLK) {
			perror("fcntl");
			nlm_resp->lstat = deadlck;
			a->w_flag = 0;
		} else if (errno == ENOLCK) {
			perror("fcntl");
			printf("rpc.lockd: out of lock.\n");
			nlm_resp->lstat = nlm_denied_nolocks;
		} else if (((cmd == F_SETLK) || (cmd == F_RSETLK)) && (errno == EACCES)) {
			nlm_resp->lstat = denied;
		} else {
			perror("fcntl");
			printf("rpc.lockd: unable to set a lock. \n");
			nlm_resp->lstat = denied;
		}
	} else
		nlm_resp->lstat = nlm_granted;

	if (ld.l_xxx == GRANT_LOCK_FLAG)
		*grant_lock_flag = TRUE;

	return (nlm_resp);
}

/*
 * choice == RPC; rpc calls to remote;
 * choice == MSG; msg passing calls to remote;
 */
remote_result *
remote_lock(a, choice)
	struct reclock *a;
	int choice;
{
	if (debug) {
		printf("enter remote_lock\n");
		pr_lock(a);
		(void) fflush(stdout);
	}

	if (choice == MSG) {	/* msg passing */
		if (nlm_call(NLM_LOCK_MSG, a, 0) == -1)
			a->rel = 1;
	}
	return(NULL);
}

remote_result *
local_unlock(a, grant_lock_flag)
	struct reclock *a;
	int *grant_lock_flag;
{
	int cmd;
	static struct eflock ld;
	remote_result *remote_grant();
	struct reclock grant_lock;
	static int fd;

	if (debug)
		printf("enter local_unlock...................\n");

	/* if there r locks to be granted, keep calling fcntl() to get 	*/
	/* info on these locks & send each of these granted locks to   	*/
	/* the respective lock mgr, NOTICE that only locks that r held 	*/
	/* by lockmgr is being replied here, granting of local blocking	*/
	/* locks is handled by the kernel.			      	*/
	if (*grant_lock_flag) {
		if (debug)
			printf("granting locks...\n");
		ld.l_type = F_UNLCK;
		ld.l_xxx = GRANT_LOCK_FLAG;
		while (fcntl(fd, F_RSETLKW, &ld) != -1) {
			grant_lock.block = TRUE;
			grant_lock.exclusive = TRUE;
			grant_lock.lck.lox.base = ld.l_start;
			grant_lock.lck.lox.length = ld.l_len;
			grant_lock.lck.lox.pid = ld.l_pid;
			grant_lock.lck.lox.rpid = ld.l_rpid;
			grant_lock.lck.lox.rsys = ld.l_rsys;
			grant_lock.lck.lox.type = ld.l_type;
			grant_lock.rel = 0; 

			/* send this granted lock to the respective remote */
			/* lock mgr					   */
			remote_grant(&grant_lock, MSG);

			/* no more granted locks, quit */
			if (ld.l_xxx < 0)	break;

			ld.l_type = F_UNLCK;
			ld.l_xxx = GRANT_LOCK_FLAG;
		}
		*grant_lock_flag = FALSE;
		remove_fd(fd);
		close(fd);
		return(NULL);
	}

	*grant_lock_flag = FALSE;
	/*
	 * convert fh to fd
	 */
	if ((fd = get_fd(a)) < 0) {
		if (fd == -1)
			nlm_resp->lstat = nlm_denied_nolocks;
		else
			nlm_resp->lstat = nlm_denied;
		return (nlm_resp);
	}

	/*
	 * set the lock
	 */
	if (a->block)
		cmd = F_RSETLKW;
	else
		cmd = F_RSETLK;
	ld.l_type = F_UNLCK;
	ld.l_whence = 0;
	ld.l_start = a->lck.lox.base;
	ld.l_len = a->lck.lox.length;
	ld.l_pid = a->lck.lox.pid;
	ld.l_rpid = a->lck.lox.rpid;
	ld.l_rsys = a->lck.lox.rsys;
	ld.l_xxx = 0;
	if (debug) {
		printf("ld.l_start=%d ld.l_len=%d ld.l_pid=%d ld.l_rpid=%d ld.l_rsys=%x\n",
			ld.l_start, ld.l_len, ld.l_pid, ld.l_rpid, ld.l_rsys);
	}

	if (fcntl(fd, cmd, &ld) == -1) {
		if (errno == EINTR) {
			nlm_resp->lstat = blocking;
			a->w_flag = 1;
		} else if (errno == ENOLCK) {
			perror("fcntl");
			printf("rpc.lockd: out of lock.\n");
			nlm_resp->lstat = nlm_denied_nolocks;
		} else {
			perror("fcntl");
			printf("rpc.lockd: unable to unlock a lock. \n");
			nlm_resp->lstat = nlm_denied;
		}
		if (ld.l_xxx == GRANT_LOCK_FLAG)
			*grant_lock_flag = TRUE;
	} else {
		/* if fcntl() returned w/ l_xxx >0, means kernel tells us */
		/* that this unlock request results in some locks to be   */
		/* granted, & we need to call fcntl() again to get these  */
		/* granted locks.  we call fcntl() again in the beginning */
		/* of this routine by chking grant_lock_flag.		  */
		/* DO NOT close the fd & reset the ld 'cause it will be   */
		/* re-used						  */
		if (ld.l_xxx == GRANT_LOCK_FLAG)
			*grant_lock_flag = TRUE;
		else {
			/*
	 	 	 * Update fd table
	 	 	*/
			remove_fd(fd);
			close(fd);
		}
		nlm_resp->lstat = nlm_granted;
	}
	return (nlm_resp);
}

remote_result *
remote_unlock(a, choice)
	struct reclock *a;
	int choice;
{
	if (debug)
		printf("enter remote_unlock\n");

	if (choice == MSG) {
		if (nlm_call(NLM_UNLOCK_MSG, a, 0) == -1)
			a->rel = 1;	/* rpc error, discard */
	} else {
		printf("rpc not supported\n");
		a->rel = 1;		/* rpc error, discard */
	}
	(void) remove_req_in_me(a);

	return (NULL);			/* no reply available */
}


remote_result *
local_test(a)
	struct reclock *a;
{
	int fd, cmd;
	struct eflock ld;
	int	nofd = used_fd;	/* Save the previous no of used */
				/* fd, used to check if this */
				/* file is opened or not */

	/*
	 * convert fh to fd
	 */
	if ((fd = get_fd(a)) < 0) {
		if (fd == -1)
			nlm_resp->lstat = nlm_denied_nolocks;
		else
			nlm_resp->lstat = nlm_denied;
		return (nlm_resp);
	}

	/*
	 * test the lock
	 */
	cmd = F_RGETLK;
	if (a->exclusive)
		ld.l_type = F_WRLCK;
	else
		ld.l_type = F_RDLCK;
	ld.l_whence = 0;
	ld.l_start = (a->lck.lox.base >= 0) ? a->lck.lox.base : 0;
	ld.l_len = a->lck.lox.length;
	ld.l_pid = a->lck.lox.pid;
	ld.l_rpid = a->lck.lox.rpid;
	ld.l_rsys = a->lck.lox.rsys;
	if (fcntl(fd, cmd, &ld) == -1) {
		if (errno == EINTR) {
			nlm_resp->lstat = blocking;
			a->w_flag = 1;
		} else if (errno == ENOLCK) {
			perror("fcntl");
			printf("rpc.lockd: out of lock.\n");
			nlm_resp->lstat = nlm_denied_nolocks;
		} else {
			perror("fcntl");
			printf("rpc.lockd: unable to test a lock. \n");
			nlm_resp->lstat = nlm_denied;
		}
	} else {
		if (ld.l_type == F_UNLCK) {
			nlm_resp->lstat = nlm_granted;
			a->lck.lox.type = ld.l_type;
		} else {
			nlm_resp->lstat = nlm_denied;
			a->lck.lox.type = ld.l_type;
			a->lck.lox.base = ld.l_start;
			a->lck.lox.length = ld.l_len;
			a->lck.lox.pid = ld.l_pid;
			a->lck.lox.rpid = ld.l_rpid;
			a->lck.lox.rsys = ld.l_rsys;
		}
	}

	if (debug) {
		printf("ld.l_start=%d ld.l_len=%d ld.l_rpid=%d ld.l_rsys=%x\n",
			ld.l_start, ld.l_len, ld.l_rpid, ld.l_rsys);
	}

	/*
	 * It is a new fd, so we close it. Or we leave it open.
	 */
	if (nofd < used_fd) {
		remove_fd(fd);
		close(fd);
	}

	return (nlm_resp);
}

remote_result *
remote_test(a, choice)
	struct reclock *a;
	int choice;
{
	if (debug)
		printf("enter remote_test\n");

	if (choice == MSG) {
		if (nlm_call(NLM_TEST_MSG, a, 0) == -1)
			a->rel = 1;
	} else {
		printf("rpc not supported\n");
		a->rel = 1;
	}
	return (NULL);
}

remote_result *
remote_cancel(a, choice)
	struct reclock *a;
	int choice;
{
	msg_entry *msgp;

	if (debug)
		printf("enter remote_cancel(%x)\n", a);

	if (choice == MSG){
		if (nlm_call(NLM_CANCEL_MSG, a, 0) == -1)
			a->rel = 1;
	} else { /* rpc case */
		printf("rpc not supported\n");
		a->rel = 1;
	}

	if ((msgp = retransmitted(a, KLM_LOCK)) != NULL) {
		/* msg is being processed */
		if (debug)
				printf("remove msg (%x) due to remote cancel\n",
				msgp->req);

		/* don't free the reclock here as remove_req_in_me() will do */
		/* that.						     */
		msgp->req->rel = 0;
		dequeue(msgp);
	}
	remove_req_in_me(a);

	return (NULL);
}

remote_result *
local_grant(a)
	struct reclock *a;
{
	msg_entry *msgp;
	remote_result *resp;
 
        if (debug)
                printf("enter local_grant(%x)...\n", a);
	msgp = msg_q;
        while (msgp != NULL) {

                if (same_bound(&(msgp->req->alock.lox), &(a->alock.lox)) && 
			same_type(&(msgp->req->alock.lox), &(a->alock.lox)) &&
			(msgp->req->alock.lox.pid == a->alock.lox.pid)) {

			/* upgrade request from pending to granted in */
			/* monitoring list for recovery		      */
			(void) upgrade_req_in_me(msgp->req);
			/*
			 * if the reply is for older request, set the
			 * reply result so that the nxt poll by the kernel
			 * will get this result
			 */
			if (msgp->reply != NULL) {
               			msgp->reply->lstat = klm_granted;
			/* if no reply is set before, lets save this reply */
			} else if ((resp = get_res()) != NULL) {
				resp->lstat = klm_granted;
				msgp->reply = resp;
			}
			break;
                }
                msgp = msgp->nxt;
        }
        nlm_resp->lstat = nlm_granted;
        return (nlm_resp);
}

remote_result *
remote_grant(a, choice)
	struct reclock *a;
	int choice;
{
	struct reclock *req;

	if (debug)
		printf("enter remote_grant...\n");

	/* reply to the granted lock that was queued in our blocking lock */
	/* list								  */
	if ((req = find_block_req(a)) != NULL) {
		if (choice == MSG) {
			if (nlm_call(NLM_GRANTED_MSG, req, 0) != -1)
				dequeue_block_req(req);
		} else {
			printf("rpc not supported\n");
		}
	}
	return (NULL);
}

remote_result *
cont_lock(a, resp)
	struct reclock *a;
	remote_result *resp;
{
	if (debug) {
		printf("enter cont_lock (%x) ID=%d \n", a, a->lck.lox.LockID);
	}

	switch (resp->lstat) {
	case nlm_granted:
		a->rel = 0;
		if (add_mon(a, 1) == -1)
			printf("rpc.lockd: add_mon failed in cont_lock.\n");
		return (resp);
	case denied:
	case nolocks:
		a->rel = 1;
		a->block = FALSE;
		a->lck.lox.granted = 0;
		return (resp);
	case deadlck:
		a->rel = 1;
		a->block = TRUE;
		a->lck.lox.granted = 0;
		return (resp);
	case blocking:
		a->rel = 0;
		a->w_flag = 1;
		a->block = TRUE;
		return (resp);
	case grace:
		a->rel = 0;
		release_res(resp);
		return (NULL);
	default:
		a->rel = 1;
		release_res(resp);
		printf("unknown lock return: %d\n", resp->lstat);
		return (NULL);
	}
}


remote_result *
cont_unlock(a, resp)
	struct reclock *a;
	remote_result *resp;
{
	if (debug)
		printf("enter cont_unlock\n");

	a->rel = 1;
	switch (resp->lstat) {
		case nlm_granted:
			return (resp);
		case denied:		/* impossible */
		case nolocks:
			return (resp);
		case blocking:		/* impossible */
			a->w_flag = 1;
			return (resp);
		case grace:
			a->rel = 0;
			release_res(resp);
			return (NULL);
		default:
			a->rel = 0;
			release_res(resp);
			fprintf(stderr,
				"rpc.lockd: unkown rpc_unlock return: %d\n",
				resp->lstat);
			return (NULL);
		}
}

remote_result *
cont_test(a, resp)
	struct reclock *a;
	remote_result *resp;
{
	if (debug)
		printf("enter cont_test\n");

	a->rel = 1;
	switch (resp->lstat) {
	case grace:
		a->rel = 0;
		release_res(resp);
		return (NULL);
	case nlm_granted:
	case denied:
		if (debug)
			printf("lock blocked by %d, (%d, %d)\n",
				resp->lholder.svid, resp->lholder.l_offset,
				resp->lholder.l_len);
		return (resp);
	case nolocks:
		return (resp);
	case blocking:
		a->w_flag = 1;
		return (resp);
	default:
		fprintf(stderr, "rpc.lockd: cont_test: unknown return: %d\n",
			resp->lstat);
		release_res(resp);
		return (NULL);
	}
}

remote_result *
cont_cancel(a, resp)
	struct reclock *a;
	remote_result *resp;
{
	if (debug)
		printf("enter cont_cancel\n");

	return(cont_unlock(a, resp));
}

remote_result *
cont_reclaim(a, resp)
	struct reclock *a;
	remote_result *resp;
{
	remote_result *local;

	if (debug)
		printf("enter cont_reclaim\n");
	switch (resp->lstat) {
	case nlm_granted:
	case denied:
	case nolocks:
	case blocking:
		local = resp;
		break;
	case grace:
		if (a->reclaim)
			fprintf(stderr, "rpc.lockd: reclaim lock req(%x) is returned due to grace period, impossible\n", a);
		local = NULL;
		break;
	default:
		printf("unknown cont_reclaim return: %d\n", resp->lstat);
		local = NULL;
		break;
	}

	if (local == NULL)
		release_res(resp);
	return (local);
}

remote_result *
cont_grant(a, resp)
        struct reclock *a;
        remote_result *resp;
{
        if (debug)
                printf("enter cont_grant...\n");
 
        return (resp);
}

