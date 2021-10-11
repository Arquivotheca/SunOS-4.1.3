#ifndef lint
static char sccsid[] = "@(#)prot_libr.c 1.1 92/07/30 SMI";
#endif

	/*
	 * Copyright (c) 1988 by Sun Microsystems, Inc.
	 */

	/*
	 * prot_libr.c
	 * consists of routines used for initialization, mapping and debugging
	 */

#include <stdio.h>
#include <sys/file.h>
#include <sys/param.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "prot_lock.h"
#include "prot_time.h"

char hostname[MAXHOSTNAMELEN];		/* for generating oh */
int pid;				/* id for monitor usage */
int host_len;				/* for generating oh */
int lock_len;
int res_len;
int msg_len;
int local_state;
int grace_period;
remote_result res_nolock;
remote_result res_working;
remote_result res_grace;

int cookie;				/* monitonically increasing # */

extern msg_entry *msg_q;
extern int debug;
extern int HASH_SIZE;
extern char *strcpy();
extern struct process_locks *proc_locks;

char *xmalloc();
extern void print_lock();

int	maxfd;
struct fd_table	*fd_table;

init()
{
	struct rlimit rl;

	(void) gethostname(hostname, MAXHOSTNAMELEN);
	/* used to generate owner handle */
	host_len = strlen(hostname) +1;
	msg_len = sizeof (msg_entry);
	lock_len = sizeof (struct reclock);
	res_len = sizeof (remote_result);
	pid = getpid(); /* used to generate return id for status monitor */
	res_nolock.lstat = nolocks;
	res_working.lstat = blocking;
	res_grace.lstat = grace;
	grace_period = LM_GRACE;
	free_locks= NULL;

	/* 
	 * try to set to max fd that can be used in the system
	 * if failed, just ignore
	 */
	(void) getrlimit(RLIMIT_NOFILE, &rl);
	rl.rlim_cur= rl.rlim_max;
	if (setrlimit(RLIMIT_NOFILE, &rl) < 0)
		fprintf(stderr, 
		"cannot increase fd limit to max %d (ignored)\n", rl.rlim_max);

	maxfd = rl.rlim_max;
	fd_table = (struct fd_table *) xmalloc(sizeof(struct fd_table) * maxfd);
	if (fd_table == NULL)
		exit(-1);
	bzero((char *) fd_table, sizeof(struct fd_table) * maxfd);
}

/*
 * map input (from kenel) to lock manager internal structure
 * returns -1 if cannot allocate memory;
 * returns 0 otherwise
 */
int
map_kernel_klm(a)
	reclock *a;
{
	/*
	 * common code shared between map_kernel_klm and map_klm_nlm
	 * generate op
	 */
	if (a->lck.lox.type == F_WRLCK) {
		a->lck.op = LOCK_EX;
	} else {
		a->lck.op = LOCK_SH;
	}
	if (a->block == FALSE)
		a->lck.op = a->lck.op | LOCK_NB;
	if (a->lck.lox.length > MAXLEN) {
		fprintf(stderr, " len(%d) greater than max len(%d)\n",
			a->lck.lox.length, MAXLEN);
		a->lck.lox.length = MAXLEN;
	}

	/*
	 * generate svid holder
	 */
	if (!a->lck.lox.pid)
		a->lck.lox.pid = getpid();
	a->lck.svid = a->lck.lox.pid;

	/*
	 * owner handle == (hostname, pid);
	 * cannot generate owner handle use obj_alloc
	 * because additioanl pid attached at the end
	 */
	a->lck.oh_len = host_len + sizeof (int);
	if ((a->lck.oh_bytes = xmalloc(a->lck.oh_len) ) == NULL)
		return (-1);
	(void) strcpy(a->lck.oh_bytes, hostname);
	bcopy((char *) &a->lck.lox.pid, &a->lck.oh_bytes[host_len],
		sizeof (int));

	/*
	 * generate cookie
	 * cookie is generated from monitonically increasing #
	 */
	cookie++;
	if (obj_alloc(&a->cookie, (char *) &cookie, sizeof (int))== -1)
		return (-1);

	/*
	 * generate clnt_name
	 */
	if ((a->lck.clnt= xmalloc(host_len)) == NULL)
		return (-1);
	(void) strcpy(a->lck.clnt, hostname);
	a->lck.caller_name = a->lck.clnt; 	/* ptr to same area */
	return (0);
}


/*
 * nlm map input from klm to lock manager internal structure
 * return -1, if cannot allocate memory!
 * returns 0, otherwise
 */
int
map_klm_nlm(a)
	reclock *a;
{
	/*
	 * common code shared between map_kernel_klm and map_klm_nlm
	 * generate op
	 */
	if (a->lck.lox.type == F_WRLCK) {
		a->lck.op = LOCK_EX;
	} else {
		a->lck.op = LOCK_SH;
	}
	if (a->block == FALSE)
		a->lck.op = a->lck.op | LOCK_NB;

	/*
	 * generate svid holder
	 */
	if (!a->lck.lox.pid)
		a->lck.lox.pid = getpid();
	a->lck.svid = a->lck.lox.pid;

	a->lck.l_offset = a->lck.lox.base;
	a->lck.l_len = a->lck.lox.length;

 	/*
	 * normal klm to nlm calls
	 */
	a->lck.clnt = a->lck.caller_name;
	if ((a->lck.svr = xmalloc(host_len)) == NULL) {
		return (-1);
	}
	(void) strcpy(a->lck.svr, hostname);
	return (0);
}

pr_oh(a)
	netobj *a;
{
	int i;
	int j;
	unsigned p = 0;

	if (a->n_len - sizeof (int) > 4 )
		j = 4;
	else
		j = a->n_len - sizeof (int);

	/*
	 * only print out part of oh
	 */
	for (i = 0; i< j; i++) {
		printf("%c", a->n_bytes[i]);
	}
	for (i = a->n_len - sizeof (int); i< a->n_len; i++) {
		p = (p << 8) | (((unsigned)a->n_bytes[i]) & 0xff);
	}
	printf("%u", p);
}

pr_fh(a)
	netobj *a;
{
	int i;

	for (i = 0; i< a->n_len; i++) {
		printf("%02x", (a->n_bytes[i] & 0xff));
	}
}


pr_lock(a)
	reclock *a;
{
	if (a != NULL) {
		printf("(%x), oh= ", a);
		pr_oh(&a->lck.oh);
		printf(", svr= %s, fh = ", a->lck.svr);
		pr_fh(&a->lck.fh);
		if (a->block)
			printf(" block=TRUE ");
		else
			printf(" block=FALSE ");
		if (a->exclusive)
			printf(" exclusive=TRUE ");
		else
			printf(" exclusive=FALSE ");
		printf(" rel=%d w_flag=%d type=%d color=%d pid=%d class=%d granted=%d rsys=%x rpid=%d LockID=%d ",
			a->rel, a->w_flag, a->lck.lox.type, a->lck.lox.color,
			a->lck.lox.pid, a->lck.lox.class, a->lck.lox.granted,
			a->lck.lox.rsys, a->lck.lox.rpid, a->lck.lox.LockID);
		printf(", op=%d, ranges= [%d, %d)\n",
 			a->lck.op,
			a->lck.lox.base, a->lck.lox.base + a->lck.lox.length);
	} else {
		printf("RECLOCK is NULL.\n");
	}
}

pr_all()
{
	msg_entry *msgp;

	if (debug < 2)
		return;

	/*
	 * print msg queue
	 */
	if (msg_q != NULL) {
		printf("***** MSG QUEUE *****\n");
		msgp= msg_q;
		while (msgp != NULL) {
			printf("(%x) : ", msgp->req);
			printf(" (%x, ", msgp->req);
			if (msgp->reply != NULL)
				printf(" lstat =%d), ", msgp->reply->lstat);
			else
				printf(" NULL), ");
			msgp = msgp->nxt;
		}
		printf("\n");
	}
	else
		printf("***** NO MSG IN MSG QUEUE *****\n");

	(void) fflush(stdout);
}

up(x)
	int x;
{
	return ((x % 2 == 1) || (x %2 == -1));
}

kill_process(a)
	reclock *a;
{
	fprintf(stderr, "kill process (%d)\n", a->lck.lox.pid);
	(void) kill(a->lck.lox.pid, SIGLOST);
}
