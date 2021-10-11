/*
 * This source code is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify this source code without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * THIS PROGRAM CONTAINS SOURCE CODE COPYRIGHTED BY SUN MICROSYSTEMS, INC.
 * AND IS LICENSED TO SUNSOFT, INC., A SUBSIDIARY OF SUN MICROSYSTEMS, INC.
 * SUN MICROSYSTEMS, INC., MAKES NO REPRESENTATIONS ABOUT THE SUITABLITY
 * OF SUCH SOURCE CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT
 * EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  SUN MICROSYSTEMS, INC. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO SUCH SOURCE CODE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN
 * NO EVENT SHALL SUN MICROSYSTEMS, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT,
 * INCIDENTAL, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM USE OF SUCH SOURCE CODE, REGARDLESS OF THE THEORY OF LIABILITY.
 * 
 * This source code is provided with no support and without any obligation on
 * the part of Sun Microsystems, Inc. to assist in its use, correction, 
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS
 * SOURCE CODE OR ANY PART THEREOF.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California 94043
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <rpc/types.h>
#include <lwp/lwp.h>
#include <lwp/lwperror.h>
#include <lwp/lwpmachdep.h>
#include <lwp/stackdep.h>
#include <syscall.h>
#include "../queue.h"

extern int errno;

/* @(#) nb.c 1.1 92/07/30 Copyr 1987 Sun Micro */

/*
 * UNIX functions that can block.
 * This library maps all potentially blocking system calls
 * into non-blocking ones. This provides some temporary help
 * for threads that use system calls until kernel thread support is available.
 * Because underlying descriptors are marked NB and ASYNCH in UNIX,
 * this library doesn't work well with other unix processes
 * sharing descriptors. Also, problems with kill -9
 * since you can't catch this to reset a descriptor.
 * We don't do exit processing (see on_exit) if a default signal action
 * kills the pod either.
 * We mark C library special context for threads using system calls.
 * This preserves errno for these threads.
 * File descriptors are treated as global state: if two threads
 * try to open/close the same descriptor, no guarantees hold.
 * XXX We could improve things by using special contexts to hold
 * the per-thread info. The only disadvantage is that this could
 * slow down scheduling slightly for threads that don't use special contexts.
 */

/*
 * we don't handle flock(2), or sigpause(2).
 * readv and writev are rarely used so we don't bother with them.
 * These are the guys we support:
 *
 * open(), socket(), pipe(), close(),
 * read(), write(),
 * send(), sendto(), sendmsg(), recv(), recvfom(), recvmsg(),
 * accept(), connect(),
 * select();
 * wait();
 */

#define	READ	0
#define	WRITE	1
#define	EXCEPT  2

/* info about a descriptor */
typedef	struct nb_t {
	int issocket;	/* TRUE if descriptor is a socket */
	int old_flag;	/* previous value of descriptor flags */
} nb_t;

/* info about an agent used to monitor a descriptor */
typedef	struct agent_t {
	struct agent_t *agt_next;
	thread_t agt_lwpid;
	int agt_event;
	caddr_t agt_mem;
	thread_t agt_aid;
} agent_t;

#define	AGT_TYPES 2
#define	AGT_SIGIO 0
#define	AGT_SIGCHLD 1

static nb_t NbDesc[NOFILE];
static qheader_t NbAgt[AGT_TYPES];
static NbInit = FALSE;
static int NbSet[NOFILE];
static mon_t AgtMon;

#define	INIT() {if (!NbInit) nbio_init(); }

static int MyPid;	/* for adding signals to process group */
static int NDesc;	/* Number of file descriptors UNIX supports */

/*
 * Initialize non-blocking lwp library.
 */
static void
nbio_init()
{
	int i;
	extern void nbio_lastrites();
	extern void fdreset();
	static stkalign_t stack[2000];

	for (i = 0; i< NOFILE; i++) {
		NbDesc[i].issocket = FALSE;
		NbDesc[i].old_flag = 0;
		NbSet[i] = FALSE;
	}
	for (i = 0; i < AGT_TYPES; i++)
		INIT_QUEUE(&(NbAgt[i]));
	MyPid = -getpid();
	(void) lwp_create((thread_t *)0, nbio_lastrites, MINPRIO,
	    (LWPSERVER | LWPNOLASTRITES), STKTOP(stack), 0);
	mon_create(&AgtMon);
	NDesc = getdtablesize();
	NbInit = TRUE;
	on_exit(fdreset, 0);	/* XX should catch signals too? */
}

/*
 * Get an agent for a thread to receive unblocking signals on.
 * There is one agent per event per lwp - agents are stored on a linked list -
 * probably should be a hash table.
 * We don't create a new agent each time for two reasons: 1) if we create
 * and destroy an agent, we may drop signals before having a chance to
 * create a new one for the new IO, and 2) for efficiency.
 * Threads must check that the event they receive was really for them: e.g.,
 * see if some IO is really available.
 * We also mark this thread as a C-library thread so it doesn't drop errno.
 */
static void
get_agent(agent, agt_type)
	thread_t *agent;
	int agt_type;
{
	agent_t *agt;
	thread_t self;

	MONITOR(AgtMon);
	lwp_self(&self);
	for (agt = (agent_t *)FIRSTQ(&(NbAgt[agt_type]));
	    agt != (agent_t *)0; agt = agt->agt_next) {
		if (SAMETHREAD(agt->agt_lwpid, self)) {
			*agent = agt->agt_aid;
			return;
		}
	}
	lwp_libcset(self);
	agt = (agent_t *)malloc(sizeof (agent_t));
	agt->agt_lwpid = self;
	agt->agt_event = agt_type;
	agt->agt_mem = (agt_type == AGT_SIGIO) ?
	    malloc(sizeof (eventinfo_t)) :
	    malloc(sizeof (sigchldev_t));
	agt_create(&agt->agt_aid, agt_type == AGT_SIGIO ? SIGIO : SIGCHLD,
	    agt->agt_mem);
	INS_QUEUE(&(NbAgt[agt_type]), agt);
	*agent = agt->agt_aid;
}

/*
 * Called each time a client thread dies.
 * Reclaim its nbio resources.
 */
static void
nbio_lastrites() {

	thread_t aid;
	eventinfo_t *event;
	int argsz;
	eventinfo_t lastrites;
	thread_t tid;	/* dying thread */
	register agent_t *agt;

	(void) agt_create(&aid, LASTRITES, (caddr_t)&lastrites);
	for (;;) {
		(void) msg_recv(&aid, (caddr_t *)&event, &argsz,
		    (caddr_t *)0, (int *)0, INFINITY);
		tid = event->eventinfo_victimid;
		msg_reply(aid);

		/*
		 * Search for all nbio agents belonging to tid
		 * and remove them and their agent memory.
		 * The REMX_ITEM is inefficient: should
		 * do the search and removal together.
		 */
		mon_enter(AgtMon);
		for (agt = (agent_t *)FIRSTQ(&(NbAgt[AGT_SIGIO]));
		    agt != (struct agent_t *)0; agt = agt->agt_next) {
			if (SAMETHREAD(agt->agt_lwpid, tid)) {
				free(agt->agt_mem);
				REMX_ITEM(&(NbAgt[AGT_SIGIO]), agt);
				free((caddr_t)agt);
				break;
			}
		}
		for (agt = (agent_t *)FIRSTQ(&(NbAgt[AGT_SIGCHLD]));
		    agt != (struct agent_t *)0; agt = agt->agt_next) {
			if (SAMETHREAD(agt->agt_lwpid, tid)) {
				free(agt->agt_mem);
				REMX_ITEM(&(NbAgt[AGT_SIGCHLD]), agt);
				free((caddr_t)agt);
				break;
			}
		}
		mon_exit(AgtMon);
	}
}

/*
 * mark a descriptor non-blocking.
 * don't reset descriptor after each IO since another thread
 * may be using the descriptor. Monitor protection won't solve
 * the problem since there is no way to atomically release the monitor
 * upon entry into the kernel.
 */
static int
nblk_io(desc, issock)
	int desc;
	int issock;
{
	int res;

	if (NbSet[desc])
		return (0);

	/* must set process group for sock io explicitly */
	if (issock) {
		NbDesc[desc].issocket = TRUE;
		if (ioctl(desc, SIOCSPGRP, &MyPid) == -1)
			return (-1);
	}
	if ((res = fcntl(desc, F_GETFL, 0)) == -1) {
		return (-1);
	} else {
		NbDesc[desc].old_flag = res;
	}
	if (fcntl(desc, F_SETFL, FASYNC|FNDELAY) == -1)
		return (-1);
	NbSet[desc] = TRUE;
	return (0);
}

/*
 * Reset all descriptors upon pod termination.
 */
static void
fdreset()
{
	register int fd;

	for (fd = 0; fd < NOFILE; fd++) {
		if (NbSet[fd]) {
			(void)fcntl(fd, F_SETFL, NbDesc[fd].old_flag);
			NbSet[fd] = FALSE;
		}
	}
}


/*
 * Await IO on a descriptor.
 */
static void
block_io(desc, operation)
	int desc;
	int operation;
{
	fd_set rmask, wmask, emask;
	register int nready;

	FD_ZERO(&rmask); FD_ZERO(&wmask); FD_ZERO(&emask);
	switch (operation){
	    case READ :
		FD_SET(desc, &rmask);
		break;
	    case WRITE :
		FD_SET(desc, &wmask);
		break;
	}
	FD_SET(desc, &emask);	/* always catch exceptions */
	errno = 0;
	nready =
	    block_until_notified(NDesc, &rmask, &wmask, &emask, INFINITY);
	if (nready <= 0) {
		if (lwp_geterr() != LE_NOERROR)
			lwp_perror("block");
		if (errno != 0)
			perror("block");
	}
}

/* Copy masks for select (which alters its arguments) */
static void
set_masks(rmask, wmask, emask, c_rmask, c_wmask, c_emask)
	fd_set *rmask, *wmask, *emask, *c_rmask, *c_wmask, *c_emask;
{
	if (c_rmask != (fd_set *)NULL) {
		if (rmask == (fd_set *)NULL) {
			FD_ZERO(c_rmask);
		} else {
			bcopy((caddr_t)rmask, (caddr_t)c_rmask,
			    sizeof (fd_set));
		}
	}
	if (c_wmask != (fd_set *)NULL) {
		if (wmask == (fd_set *)NULL) {
			FD_ZERO(c_wmask);
		} else {
			bcopy((caddr_t)wmask, (caddr_t)c_wmask,
			    sizeof (fd_set));
		}
	}
	if (c_emask != (fd_set *)NULL) {
		if (emask == (fd_set *)NULL) {
			FD_ZERO(c_emask);
		} else {
			bcopy((caddr_t)emask, (caddr_t)c_emask,
			    sizeof (fd_set));
		}
	}
}

/* 'or' together a set of masks into "mask" */
static void
or_masks(mask, rmask, wmask, emask)
	fd_set *mask, *rmask, *wmask, *emask;
{
	register int i;
	register caddr_t m = (caddr_t)mask;
	fd_set temp;

	FD_ZERO(&temp);
	m = (caddr_t)mask;
	if (rmask == (fd_set *)NULL)
		rmask = &temp;
	if (wmask == (fd_set *)NULL)
		wmask = &temp;
	if (emask == (fd_set *)NULL)
		emask = &temp;
	for (i = 0; i < sizeof (fd_set); i++) {
		m[i] =
		    ((caddr_t)rmask)[i] | ((caddr_t)wmask)[i] |
		    ((caddr_t)emask)[i];
	}
}

/* return TRUE if any descriptors in src appear in dst */
int
mask_match(src, dst)
	fd_set *src;
	fd_set *dst;
{
	register int i;

	for (i = 0; i < sizeof (fd_set); i++) {
		if (((caddr_t)src)[i] & ((caddr_t)dst)[i]) {
			return (TRUE);
		}
	}
	return (FALSE);
}

/*
 *  Count the number of bits set in a mask.
 */
static int
countbits(mask, width)
	fd_set *mask;
	int width;
{
	register int i;
	register int total = 0;
	register int nbytes;

	nbytes = (width - 1 + NBBY) / NBBY;
	for (i = 0; i < sizeof (fd_set); i++) {
		total += count(((caddr_t)mask)[i]);
	}
	return (total);
}

/* count the number of bits in a byte. should really be a table */
static int
count(n)
	char n;
{
	register int tocount = (n & 0xff);
	register int bits = 0;

	while (tocount) {
		bits++;
		tocount &= tocount - 1;
	}
	return (bits);
}

static struct timeval SelPol = {0, 0};

/*
 * See if IO is immediately ready on a set of descriptors.
 * Be careful to not have side effects on descriptors since
 * the intent is to block until something really happens.
 * True polled select does have the side effect on descriptors,
 * and is handled in select itself.
 */
static int
poll_select(width, rmask, wmask, emask)
	int width;
	fd_set *rmask, *wmask, *emask;
{
	fd_set c_rmask, c_wmask, c_emask;
	int nready = 0;

	set_masks(rmask, wmask, emask, &c_rmask, &c_wmask, &c_emask);
	do {
		timerclear(&SelPol);
		nready =
		    syscall(SYS_select, width, rmask, wmask, emask, &SelPol);
	} while ((nready == -1) && (errno == EINTR));
	if (nready <= 0)
		set_masks(&c_rmask, &c_wmask, &c_emask, rmask, wmask, emask);
	return (nready);
}

/*
 * Wait until IO may be available on a set of descriptors.
 */
static int
block_until_notified(width, rmask, wmask, emask, timeout)
	fd_set *rmask, *wmask, *emask;
	int width;
	struct timeval *timeout;
{
	fd_set c_rmask, c_wmask, c_emask;
	thread_t aid;
	fd_set ret_mask;
	fd_set req_mask;
	fd_set countmask;
	int err;

	get_agent(&aid, AGT_SIGIO);
	set_masks(rmask, wmask, emask, &c_rmask, &c_wmask, &c_emask);
	or_masks(&req_mask, &c_rmask, &c_wmask, &c_emask);
again:
	if (!(poll_select(width, &c_rmask, &c_wmask, &c_emask))) {
		err = msg_recv(&aid,
		    (caddr_t *)0, (int *)0, (caddr_t *)0, (int *)0, timeout);
		if ((err == -1) && (lwp_geterr() == LE_TIMEOUT)) {
			return (0);
		}
		msg_reply(aid);
	}
	or_masks(&ret_mask, &c_rmask, &c_wmask, &c_emask);
	if (!mask_match(&ret_mask, &req_mask)) {
		set_masks(rmask, wmask, emask, &c_rmask, &c_wmask, &c_emask);
		goto again;
	}
	set_masks(&c_rmask, &c_wmask, &c_emask, rmask, wmask, emask);
	or_masks(&countmask, emask, rmask, wmask);
	return (countbits(&countmask, width));
}

/*
 * -----------------------------------------------------------------------
 * 			SYSTEM CALLS
 * -----------------------------------------------------------------------
 */


int
socket(af, type, protocol)
	int af;
	int type;
	int protocol;
{
	int s;

	INIT();
	s = syscall(SYS_socket, af, type, protocol);
	if (s == -1)
		return (-1);
	if (nblk_io(s, TRUE) == -1)
		return (-1);
	return (s);
}

int
accept(s, addr, addrlen)
	int s;
	struct sockaddr *addr;
	int *addrlen;
{
	register int ns;
	register int saved_len = *addrlen;

	INIT();
	if (nblk_io(s, TRUE) == -1)
		return (-1);
	ns = syscall(SYS_accept, s, addr, addrlen);
	while ((ns == -1) && ((errno == EWOULDBLOCK) || (errno == EINTR))) {
		*addrlen = saved_len;
		block_io(s, READ);
		ns = syscall(SYS_accept, s, addr, addrlen);
	}
	return (ns);
}

int
connect(s, name, namelen)
	int s;
	struct sockaddr *name;
	int namelen;
{
	register int err;

	INIT();
	if (nblk_io(s, TRUE) == -1)
		return (-1);
	err = syscall(SYS_connect, s, name, namelen);
	while ((err == -1) && ((errno == EINPROGRESS) ||
	    (errno == EWOULDBLOCK) || (errno == EINTR))) {
		block_io(s, WRITE);
		err = syscall(SYS_connect, s, name, namelen);
	}
	if ((err == -1) && (errno == EISCONN))
		err = 0;
	return (err);
}

int
pipe(fds)
	int fds[];
{
	INIT();
	if (mpipe(fds) == -1)
		return (-1);
	if (nblk_io(fds[0], TRUE) == -1)
		return (-1);
	if (nblk_io(fds[1], TRUE) == -1)
		return (-1);
	return (0);
}

int
open(path, flags, mode)
	char *path;
	int flags;
	int mode;
{
	register int fd;

	INIT();
	fd = syscall(SYS_open, path, flags, mode);
	if (fd == -1)
		return (-1);
	if (nblk_io(fd, FALSE) == -1)
		return (-1);
	return (fd);
}

int
close(fd)
{

	INIT();
	if (fcntl(fd, F_SETFL, NbDesc[fd].old_flag) == -1) {
		return (-1);
	}
	if (syscall(SYS_close, fd) == -1)
		return (-1);
	NbDesc[fd].issocket = FALSE;
	NbSet[fd] = FALSE;
	return (0);
}

int
write(fd, buf, nbytes)
	int fd;
	caddr_t buf;
	int nbytes;
{
	register int nwrote;
	register caddr_t next = buf;
	register int total_written = 0;
	register int l_nbytes = nbytes;

	INIT();
	if (nblk_io(fd, FALSE) == -1)
		return (-1);
	do {
		nwrote = syscall(SYS_write, fd, next, l_nbytes);
		while ((nwrote == -1) &&
		    ((errno == EWOULDBLOCK) || (errno == EINTR))) {
			block_io(fd, WRITE);
			nwrote = syscall(SYS_write, fd, next, l_nbytes);
		}
		if (nwrote == -1)
			break;
		else {
			total_written += nwrote;
			if (total_written < nbytes)
				next += (l_nbytes = (nbytes - total_written));
		}
	} while (total_written < nbytes);
	if (nwrote == -1)
		return (-1);
	return (total_written);
}

int
read(fd, buf, nbytes)
	int fd;
	char *buf;
	int nbytes;
{
	register int nread;

	INIT();
	if (nblk_io(fd, FALSE) == -1)
		return (-1);
	nread = syscall(SYS_read, fd, buf, nbytes);
	while ((nread == -1) && ((errno == EWOULDBLOCK) || (errno == EINTR))) {
		block_io(fd, READ);
		nread = syscall(SYS_read, fd, buf, nbytes);
	}
	return (nread);
}

int
select(width, rmask, wmask, emask, timeout)
	int width;
	fd_set *rmask;
	fd_set *wmask;
	fd_set *emask;
	struct timeval *timeout;
{

	register int err;
	register int nready;
	register int i;
	fd_set mask;	/* composite mask of fds to be marked ASYNC */

	INIT();
	or_masks(&mask, rmask, wmask, emask);
	if ((timeout != NULL) &&
	    (timeout->tv_sec == 0) && (timeout->tv_usec == 0)) {
		err = syscall(SYS_select, width, rmask, wmask, emask, timeout);
		return (err);
	}
	for (i = 0; i < NOFILE; i++) {
		if (FD_ISSET(i, &mask)) {
			if (nblk_io(i, FALSE) == -1) {
				return (-1);
			}
		}
	}
	nready = block_until_notified(width, rmask, wmask, emask, timeout);
	return (nready);
}

int
send(s, msg, len, flags)
	int s;
	char *msg;
	int len;
	int flags;
{
	register int nsent;

	INIT();
	if (nblk_io(s, TRUE) == -1)
		return (-1);
	nsent = syscall(SYS_send, s, msg, len, flags);
	while ((nsent == -1) && ((errno == EWOULDBLOCK) || (errno == EINTR))) {
		block_io(s, WRITE);
		nsent = syscall(SYS_send, s, msg, len, flags);
	}
	return (nsent);
}

int
sendto(s, msg, len, flags, to, tolen)
	int s;
	char *msg;
	int len;
	int flags;
	struct sockaddr *to;
	int tolen;
{
	register int nsent;

	INIT();
	if (nblk_io(s, TRUE) == -1)
		return (-1);
	nsent = syscall(SYS_sendto, s, msg, len, flags, to, tolen);
	while ((nsent == -1) && ((errno == EWOULDBLOCK) || (errno == EINTR))) {
		block_io(s, WRITE);
		nsent = syscall(SYS_sendto, s, msg, len, flags, to, tolen);
	}
	return (nsent);
}

int
sendmsg(s, msg, flags)
	int s;
	struct msghdr msg[];
	int flags;
{
	register int nsent;

	INIT();
	if (nblk_io(s, TRUE) == -1)
		return (-1);
	nsent = syscall(SYS_sendmsg, s, msg, flags);
	while ((nsent == -1) && ((errno == EWOULDBLOCK) || (errno == EINTR))) {
		block_io(s, WRITE);
		nsent = syscall(SYS_sendmsg, s, msg, flags);
	}
	return (nsent);
}

int
recvfrom(s, buf, len, flags, from, fromlen)
	int s;
	char *buf;
	int len;
	int flags;
	struct sockaddr *from;
	int *fromlen;
{
	register int nrecvd;
	register int saved_len = *fromlen; /* if nrecvd=-1; reset fromlen */

	INIT();
	if (nblk_io(s, TRUE) == -1)
		return (-1);
	nrecvd = syscall(SYS_recvfrom, s, buf, len, flags, from, fromlen);
	while ((nrecvd == -1) && ((errno == EWOULDBLOCK) || (errno == EINTR))){
		block_io(s, READ);
		*fromlen = saved_len;
		nrecvd = syscall(SYS_recvfrom, s, buf, len,
		    flags, from, fromlen);
	}
	return (nrecvd);
}

int
recvmsg(s, msg, flags)
	int s;
	struct  msghdr msg[];
	int flags;
{
	register int nrecvd;

	INIT();
	if (nblk_io(s, TRUE) == -1)
		return (-1);
	nrecvd = syscall(SYS_recvmsg, s, msg, flags);
	while ((nrecvd == -1) && ((errno == EWOULDBLOCK) || (errno == EINTR))) {
		block_io(s, READ);
		nrecvd = syscall(SYS_recvmsg, s, msg, flags);
	}
	return (nrecvd);
}

/*
 * Should have only a single thread gathering the hwp's
 * since each thread using a wait agent will see signals for ALL hwp's.
 * Could do IO like this too: each read (say) registers with a SIGIO agent
 * thread that wakes up the registerer only when HIS IO is ready.
 * SIGIO has this same property of being able to report 1 signal for multiple
 * IO however so this method could deadlock.
 * This is a general problem sith signals: ons signal for many events is
 * possible. With real lwps, if you want to be non-blocking,
 * you simply *block* on the thing you want: wait(pid). This doesn't
 * affect the rest of the pod. Hwps aren't an answer here because
 * you don't want to fork a hwp to wait for a hwp. lwp's give you
 * the semantics of sharing descriptors that hwp's don't.
 * In addition to performance, this is a big difference.
 */
pid_t
wait(status)
	union wait *status;
{
	thread_t aid;
	int chld_pid;
	int err;
	sigchldev_t *agtinfo;
	struct rusage rusage;

	INIT();
	chld_pid = wait3(status, WNOHANG|WUNTRACED, &rusage);
	if (chld_pid < 0) {	/* no children */
		return (-1);
	}
	if (chld_pid > 0) {	/* reaped a child right away */
		return (chld_pid);
	}

	/* Have children, but none have exited yet */
	get_agent(&aid, AGT_SIGCHLD);
	err = msg_recv(&aid,
	    (caddr_t *)&agtinfo, (int *)0, (caddr_t *)0, (int *)0, INFINITY);
	if (err == -1) {
		return (-1);
	}
	chld_pid = agtinfo->sigchld_pid;
	*status = agtinfo->sigchld_status;
	msg_reply(aid);
	return (chld_pid);
}
