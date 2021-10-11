/*	@(#)listen.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)listen:listen.c	1.19.4.4"

/*
 * Network Listener Process
 *
 *		data base version:
 *
 *		accepts the following message to start servers:
 *
 *		NLPS:000:001:svc_code	where svc_code is a null terminated 
 *					char string ( <= SVC_CODE_SZ bytes)
 *
 *		command line:
 *
 *		listen [ -n netspec ] [ -r external_addr ] [ -l external_addr ]
 *		all options take appropriate defaults for STARLAN NETWORK.
 *		-r: address to listen for remote login requests.
 *		-l: address to listen for listener service requests.
 *
 */

/* system include files	*/

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <varargs.h>
#include <string.h>
#include <errno.h>
#include <memory.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <tiuser.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <values.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <sys/poll.h>
#include <sys/dir.h>
#include <sys/stropts.h>

/* S4 status manager interface					*/
#ifdef	S4
#include <status.h>
#endif

/* listener include files */

#include "lsparam.h"		/* listener parameters		*/
#include "lsfiles.h"		/* listener files info		*/
#include "lserror.h"		/* listener error codes		*/
#include "lsnlsmsg.h"		/* NLPS listener protocol	*/
#include "lssmbmsg.h"		/* MS_NET identifier		*/
#include "lsdbf.h"		/* data base file stuff		*/
#include "nlsenv.h"		/* environ variable names	*/
#include "listen.h"

/* defines	*/

#define NAMESIZE	(NAMEBUFSZ-1)

#define SPLhi()		Splflag = 1
#define SPLlo()		Splflag = 0

#define GEN	1
#define LOGIN	0

/* global variables	*/

int	Child;		/* set if process is a listener child (worker bee) */

int	Pid;		/* listener's process ID (original listener's child) */

char	*Progname;		/* listener's basename (from argv[0])	*/

static char Provbuf[PATHSIZE];
char	*Provider = Provbuf;	/* name of transport provider		*/
char	*Netspec = NETSPEC;

int	Nfd1 = -1;		/* Net fd's, init to illegal		*/
int	Nfd2 = -1;
int	Nfd3 = -1;

char	_rlbuf[NAMEBUFSZ];		/* remote login name		*/
char	_lsbuf[NAMEBUFSZ];		/* listener service name	*/

char	*Nodename = _rlbuf;		/* my normalized name (login svc)*/
char	*Pnodename = _lsbuf;		/* my permuted name (listener svc) */

char	*Rxname = NULL;			/* remote login external name	*/
char	*Lxname = NULL;			/* listener svc external name	*/

int	Rlen = NAMEBUFSZ;
int	Llen = NAMEBUFSZ;

struct	call_list	Lfree;		/* network call save free list */
struct	call_list	Lpend;		/* network call save pending list */
unsigned int	Lmaxcon;		/* maximum # of connections for gen. listen */

struct	call_list	Rfree;		/* rem login call save free list */
struct	call_list	Rpend;		/* rem login call save pending list */
unsigned int	Rmaxcon;		/* maximum # of connections for remote login */

char	*Basedir = BASEDIR;		/* base dir */
char	*Home;				/* listener's home directory	*/
char	Homebuf[BUFSIZ];
FILE	*Debugfp = stderr;
FILE	*Logfp;
int	Pidfd;

int	Background;			/* are we in background?	*/

char	Lastmsg[BUFSIZ];		/* contains last msg logged (by stampbuf) */

int	Logmax = LOGMAX;

int	Validate;		/* from -v flag: validate data base only */

int	Lckfd;

int	Sigusr1;		/* data base file changed		*/

int	Splflag;		/* logfile critical region flag		*/


static char *badnspmsg = "Bad netspec on command line ( Pathname too long )\n";
static char *badstart  = "Listener failed to start properly\n";


main(argc, argv)
int argc;
char **argv;
{
	register char **av;
	struct stat buf;
	int ret;
	extern void exit(), logexit();

	/*
	 * pre-initialization:
	 */

	if (geteuid() != 0) {
		fprintf(stderr, "Must be root to start listener\n");
		logexit(1, badstart);
	}

	/*
	 * quickly, find -n [ and -v ] argument on command line	
	 */

	av = argv; ++av;		/* skip argv[0]	*/

	while(*av)  {
		if (!strncmp(*av,"-n", 2)) {
			if (*(*av + 2) == NULL)
				Netspec = *++av;
			else
				Netspec = (*av + 2);
		} else {
			if (!strcmp(*av, "-v"))
				++Validate;
		}
		++av;
	}

	if (strlen(Netspec) > PATHSIZE)  {
		fprintf(stderr, badnspmsg);
		logexit(1, badstart);
	}

	strcat(Homebuf,Basedir);
	strcat(Homebuf,"/");
	strcat(Homebuf,Netspec);
	Home = Homebuf;

	strcpy(Provbuf, "/dev/");
	strcat(Provbuf, Netspec);

	if (chdir(Home))  {
		fprintf(stderr,"Cannot change directory to %s\n",Home);
		logexit(1, badstart);
	}

	/*
	 * -v flag tells listener to validate the data base only
	 */

	if (Validate)  {
		Logfp = stderr;
#ifdef	DEBUGMODE
		umask(022);
		if (!(Debugfp = fopen(VDBGNAME, "w")))  {
			fprintf(stderr,"Can't create debug file: %s in %s\n",
				VDBGNAME, Home);
			exit(1); /* don't log validation exits */
		}
#endif
		ret = init_dbf();
		logmessage(ret ? "Bad data base file": "Good data base file");
		exit(ret); /* don't log validation exits */
	}

	umask(0);

	if (((Lckfd = open(LCKNAME, O_WRONLY | O_CREAT, 0666 )) == -1) 
		|| (locking(Lckfd, 2, 0L) == -1)) {
		fprintf(stderr, "Errno %d opening lock file %s/%s.\n", 
			errno, Home, LCKNAME);
		fprintf(stderr, "Listener may already be running!\n");
		logexit(1, badstart);
	}

	write(Lckfd, "lock", 4);

	if (locking(Lckfd, 1, 0L) == -1)  {
		fprintf(stderr, "Errno %d while locking lock file\n", errno);
		logexit(1, badstart);
	}

	umask(022);

	if (stat(LOGNAME, &buf) == 0) {
		/* file exists, try and save it but if we can't don't worry */
		unlink(OLOGNAME);
		if (link(LOGNAME, OLOGNAME)) {
			fprintf(stderr, "Lost old log file\n");
		}
		unlink(LOGNAME);
	}

/*
 * At this point, either it didn't exist, we successfully saved it
 * or we couldn't save it.  In any case, just open and continue.
 */

	Logfp = fopen(LOGNAME, "w");

#ifdef	DEBUGMODE
	Debugfp = fopen(DBGNAME, "w");
#endif


#ifdef	DEBUGMODE
	if ((!Logfp) || (!Debugfp))  {
#else
	if (!Logfp) {
#endif
		fprintf(stderr,"listener cannot create file(s) in %s\n",Home);
		logexit(1, badstart);
	}

	logmessage("@(#)listen:listen.c	1.19.4.4");

#ifdef	DEBUGMODE
	logmessage("Listener process with DEBUG capability");
#endif

	initialize(argv, argc);
	listen();
}



initialize(argv, argc)
int argc;
char **argv;
{

	/* try to get my "basename"		*/

	DEBUG((9,"in initialize"));
	Progname = strrchr(argv[0], '/');
	if (Progname && Progname[1])
		++Progname;
	else
		Progname = argv[0];

	cmd_line_args(argv,argc);
	init_dbf();
	pid_open();			/* create pid file		*/
	net_open();			/* init, open, bind names 	*/
	catch_signals();
	divorce_parent();
	logmessage("Initialization Complete");
}


/*
 * cmd_line_args uses getopt(3) to read command line arguments.
 */

static char *Optflags = "n:r:l:L:";


cmd_line_args(argv,argc)
int argc;
char **argv;
{
	register i;
	extern char *optarg;
	extern int optind, opterr;
	extern int isdigits();
	int errflag = 0;

	DEBUG((9,"in cmd_line_args"));

#ifdef	DEBUGMODE
	DEBUG((9,"cmd_line_args: argc = %d",argc));
	for (i=0;  i < argc; i++)
		DEBUG((9,"argv[%d] = %s",i,argv[i]));
#else
	opterr = 1;		/* disable getopt's stderr output */
#endif	/* DEBUGMODE */


	while ( (i = getopt(argc, argv, Optflags)) != EOF )
		switch (i)  {
		case 'n':		/* netspec			*/
			break;		/* (read previously)		*/

		case 'r':		/* remote login external addr	*/
			Rxname = optarg;
			break;

		case 'l':		/* listener svc external addr	*/
			Lxname = optarg;
			break;

		case 'L':		/* max log entries		*/
			if (isdigits(optarg))  {
				Logmax = atoi( optarg );
				if (Logmax < LOGMIN)
					Logmax = LOGMIN;
				DEBUG((5, "Logmax = %d", Logmax));
			} else
				++errflag;
			break;

		case '?':
			++errflag;
			break;
		}

#ifndef S4

		if (!Lxname)
			++errflag;

#endif

	if (errflag)
		error(E_CMDLINE, EXIT | NOCORE);

}


/*
 * pid_open:
 *
 * open pidfile with specified oflags and modes
 *
 */

static char *pidopenmsg ="Can't create process ID file in home directory";

pid_open()
{
	int pid;
	int ret;
	unsigned i;
	char pidstring[20];

	DEBUG((9,"in pid_open()"));

	if ((Pidfd = open(PIDNAME, PIDOFLAG, PIDMODE)) == -1)  {
		logmessage(pidopenmsg);
		error(E_CREAT, EXIT | NOCORE | NO_MSG);
	}
	pid = getpid();
	i = sprintf(pidstring, "%d", pid) + 1;
	if ((ret = write(Pidfd, pidstring, i)) != i) {
		if (ret < 0)
			sys_error(E_PIDWRITE, EXIT);
		else
			error(E_PIDWRITE, EXIT);
	}
	(void) lseek(Pidfd, 0L, 0);
}


/*
 * net_open: open and bind communications channels
 *		The name generation code in net_open, open_bind and bind is, 
 * 		for the	most part, specific to STARLAN NETWORK.  
 *		This name generation code is included in the listener
 *		as a developer debugging aid.
 */

net_open()
{
	extern char *memcpy();
	extern char *t_alloc();
	extern int nlsc2addr();
	void queue();
#ifdef	S4
	register char *p;
	extern char *getnodename();
	extern char *nlsname();		/* STARLAN specific	*/
#endif
#ifdef	CHARADDR
	char pbuf[NAMEBUFSZ + 1];
#endif
	register int i;
	register struct callsave *tmp;
	char scratch[BUFSIZ];

	DEBUG((9,"in net_open"));

	DEBUG((3, "Rxname = %s", Rxname));
	DEBUG((3, "Lxname = %s", Lxname));

	if (Rxname)  {			/* -r cmd line argument		*/
		DEBUG((3, "Decoding remote login address"));
		if ((Rlen = nlsc2addr(Nodename, NAMEBUFSZ, Rxname)) < 0)  {
			logmessage("Error decoding remote login name");
			error(E_CMDLINE, EXIT | NOCORE | NO_MSG);
		}
	}

#ifdef	S4
	else  {
		p = getnodename();
		DEBUG((3, "machine nodename = %s", p));
		Rlen = strlen(p);
		(void)memcpy(Nodename, p, Rlen);
	}
#endif	/* S4 */


	if (Lxname)  {			/* -l cmd line argument		*/
		DEBUG((3, "Decoding listener service address"));
		if ((Llen = nlsc2addr(Pnodename, NAMEBUFSZ, Lxname)) < 0)  {
			logmessage("Error decoding listener service name");
			error(E_CMDLINE, EXIT | NOCORE | NO_MSG);
		}
	}

#ifdef	S4
	else  {
		p = getnodename();
		DEBUG((3, "machine nodename = %s", p));
		p = nlsname(p);
		Llen = strlen(p);
		(void)memcpy(Pnodename,p,Llen);
		DEBUG((3,"nlsname = %s, Pnodename = %s, Llen = %d",
			p, Pnodename, Llen));
	}
#endif /* S4 */

#ifdef	CHARADDR
	/* this debug code assumes addresses are printable characters */
	/* not necessarily null terminated.	*/

	if (Rxname) {
		(void)memcpy(pbuf, Nodename, Rlen);
		*(pbuf+Rlen) = (char)0;
		DEBUG((3, "Nodename = %s, length = %d", pbuf, Rlen));
	}
	(void)memcpy(pbuf, Pnodename, Llen);
	*(pbuf+Llen) = (char)0;
	DEBUG((3, "Pnodename = %s, length = %d", pbuf, Llen));
#endif	/* CHARADDR	*/

	if (Rxname) {
		Nfd1 = open_bind(Nodename, MAXCON, Rlen, &Rmaxcon);
		sprintf(scratch, "Rmaxcon is %d", Rmaxcon);
		logmessage(scratch);
	}

/*
 * set up call save list for remote login service
 */

	if (Rxname) {
		Rfree.cl_head = (struct callsave *) NULL;
		Rfree.cl_tail = (struct callsave *) NULL;
		for (i = 0; i < Rmaxcon; ++i) {
			if ((tmp = (struct callsave *) malloc(sizeof(struct callsave))) == NULL) {
				error(E_MALLOC, NOCORE | EXIT);
			}
			if ((tmp->c_cp = (struct t_call *) t_alloc(Nfd1, T_CALL, T_ALL)) == NULL) {
				tli_error(E_T_ALLOC, EXIT);
			}
			queue(&Rfree, tmp);
		}
	}

	Nfd2 = open_bind(Pnodename, MAXCON, Llen, &Lmaxcon);
	sprintf(scratch, "Lmaxcon is %d", Lmaxcon);
	logmessage(scratch);

/*
 * set up call save list for general network listen service
 */

	Lfree.cl_head = (struct callsave *) NULL;
	Lfree.cl_tail = (struct callsave *) NULL;
	for (i = 0; i < Lmaxcon; ++i) {
		if ((tmp = (struct callsave *) malloc(sizeof(struct callsave))) == NULL) {
			error(E_MALLOC, NOCORE | EXIT);
		}
		if ((tmp->c_cp = (struct t_call *) t_alloc(Nfd2, T_CALL, T_ALL)) == NULL) {
			tli_error(E_T_ALLOC, EXIT);
		}
		queue(&Lfree, tmp);
	}

	if ( ((Nfd1 == -1) && Rxname) || (Nfd2 == -1) )
		error(E_OPENBIND, EXIT);

	logmessage("Net opened, names bound");
}


/*
 * Following are some general queueing routines.  The call list head contains
 * a pointer to the head of the queue and to the tail of the queue.  Normally,
 * calls are added to the tail and removed from the head to ensure they are
 * processed in the order received, however, because of the possible interruption
 * of an acceptance with the resulting requeueing, it is necessary to have a
 * way to do a "priority queueing" which inserts at the head of the queue for
 * immediate processing
 */

/*
 * queue:
 *
 * add calls to tail of queue
 */


void
queue(head, cp)
register struct call_list *head;
register struct callsave *cp;
{
	if (head->cl_tail == (struct callsave *) NULL) {
		cp->c_np = (struct callsave *) NULL;
		head->cl_head = head->cl_tail = cp;
	}
	else {
		cp->c_np = head->cl_tail->c_np;
		head->cl_tail->c_np = cp;
		head->cl_tail = cp;
	}
}


/*
 * pqueue:
 *
 * priority queuer, add calls to head of queue
 */

void
pqueue(head, cp)
register struct call_list *head;
register struct callsave *cp;
{
	if (head->cl_head == (struct callsave *) NULL) {
		cp->c_np = (struct callsave *) NULL;
		head->cl_head = head->cl_tail = cp;
	}
	else {
		cp->c_np = head->cl_head;
		head->cl_head = cp;
	}
}


/*
 * dequeue:
 *
 * remove a call from the head of queue
 */


struct callsave *
dequeue(head)
register struct call_list *head;
{
	register struct callsave *ret;

	if (head->cl_head == (struct callsave *) NULL)
		error(E_CANT_HAPPEN, EXIT);
	ret = head->cl_head;
	head->cl_head = ret->c_np;
	if (head->cl_head == (struct callsave *) NULL)
		head->cl_tail = (struct callsave *) NULL;
	return(ret);
}


/*
 * open_bind:
 *
 * open the network and bind the endpoint to 'name'
 * this routine is also used by listen(), so it can't exit
 * under all error conditions: specifically, if there are
 * no minor devices avaliable in the network driver, open_bind
 * returns -1.  (error message will be logged).  All other errors
 * cause an exit.
 *
 * If clen is zero, transport provider picks the name and these
 * routines (open_bind and bind) ignore name and qlen -- 
 * this option is used when binding a name for accepting a connection 
 * (not for listening.)  You MUST supply a name, qlen and clen when
 * opening/binding a name for listening.
 *
 * Assumptions: driver returns ENXIO when all devices are allocated.
 */

int
open_bind(name, qlen, clen, conp)
char *name;
int qlen;
int clen;
unsigned int *conp;
{
	register fd;
	struct t_info t_info;
	extern int t_errno, errno;
	unsigned int ret;
	extern unsigned int bind();

	fd = t_open(Provider, NETOFLAG, &t_info);
	if (fd < 0)  {
		if ( (t_errno == TSYSERR) && ((errno == ENXIO) ||
		    (errno == ENOSR) || (errno == EAGAIN) || (errno == ENOSPC)) )  {
			tli_error(E_FD1OPEN, CONTINUE);
			logmessage("No network minor devices (ENXIO/ENOSR)");
			return(-1);
		}
		tli_error(E_FD1OPEN, EXIT);
	}

	DEBUG((7,"fd %d opened",fd));

	ret = bind(fd, name, qlen, clen);
	if (conp)
		*conp = ret;
	return(fd);
}


unsigned int
bind(fd, name, qlen, clen)
register fd;
char *name;
int qlen;
register int clen;
{
	register struct t_bind *req = (struct t_bind *)0;
	register struct t_bind *ret = (struct t_bind *)0;
	register char *p, *q;
	unsigned int retval;
	extern char *t_alloc();
	extern void nlsaddr2c();
	extern int memcmp();
	extern int errno;
	extern char *memcpy();

#ifdef S4
	extern struct netbuf *lname2addr();
#endif /* S4 */

#ifdef	CHARADDR
	char pbuf[NAMEBUFSZ + 1];
#endif
	char scratch[BUFSIZ];

	DEBUG((9,"in bind, clen = %d", clen));

	if (clen)  {
		errno = t_errno = 0;
		while (!(req = (struct t_bind *)t_alloc(fd,T_BIND,T_ALL)) ) {
			if ((t_errno != TSYSERR) || (errno != EAGAIN))
				tli_error( E_T_ALLOC, EXIT);
			else
				tli_error( E_T_ALLOC, CONTINUE);
		}

		errno = t_errno = 0;
		while (!(ret = (struct t_bind *)t_alloc(fd,T_BIND,T_ALL)) ) {
			if ((t_errno != TSYSERR) || (errno != EAGAIN))
				tli_error( E_T_ALLOC, EXIT);
			else
				tli_error( E_T_ALLOC, CONTINUE);
		}

		if (clen > req->addr.maxlen)  {
			sprintf(scratch,"Truncating name size from %d to %d", 
				clen, req->addr.maxlen);
			logmessage(scratch);
			clen = req->addr.maxlen;
		}

		(void)memcpy(req->addr.buf, name, clen);
		req->addr.len = clen;
		req->qlen = qlen;

#if defined(CHARADDR) && defined(DEBUGMODE)
		(void)memcpy(pbuf, req->addr.buf, req->addr.len);
		pbuf[req->addr.len] = (char)0;
		DEBUG((3,"bind: fd=%d, logical name=%c%s%c, len=%d",
			fd, '\"',pbuf, '\"', req->addr.len));
#endif	/* CHARADDR  && DEBUGMODE */


#ifdef S4
		if (!lname2addr(fd, &(req->addr)))  {
			sprintf(scratch, "lname2addr failed, errno %d", errno);
			logmessage(scratch);
			error(E_SYS_ERROR, EXIT | NO_MSG);
		}
#endif /* S4 */

#if defined(CHARADDR) && defined(DEBUGMODE)
		(void)memcpy(pbuf, req->addr.buf, req->addr.len);
		pbuf[req->addr.len] = (char)0;
		DEBUG((3,"bind: fd=%d, address=%c%s%c, len=%d",
			fd, '\"',pbuf, '\"', req->addr.len));
#endif	/* CHARADDR  && DEBUGMODE */


	}

	if (t_bind(fd, req, ret))  {
		DEBUG((1,"t_bind failed; t_errno %d errno %d", t_errno, errno));
		if (qlen)	/* starup only */
			tli_error(E_T_BIND, EXIT | NOCORE);
		/* here during normal service */
		if ((t_errno == TNOADDR) || ((t_errno == TSYSERR) && (errno == EAGAIN))) {
			/* our name space is all used up */
			tli_error(E_T_BIND, CONTINUE);
			t_close(fd);
			return(-1);
		}
		/* otherwise, irrecoverable error */
		tli_error(E_T_BIND, EXIT | NOCORE);
	}

#if defined(S4) && defined(DEBUGMODE)
	t_dump(Debugfp);  /* show TLI internal name structures on Debugfp */
#endif


	if (clen)  {
		retval = ret->qlen;
		if ( (ret->addr.len != req->addr.len) ||
		     (memcmp( req->addr.buf, ret->addr.buf, req->addr.len)) )  {
			p = (char *) malloc(((ret->addr.len) << 1) + 1);
			q = (char *) malloc(((req->addr.len) << 1) + 1);
			if (p && q) {
				nlsaddr2c(p, ret->addr.buf, ret->addr.len);
				nlsaddr2c(q, req->addr.buf, req->addr.len);
				sprintf(scratch, "attempted to bind address \\x%s", q);
				logmessage(scratch);
				sprintf(scratch, "actually bound address \\x%s", p);
				logmessage(scratch);
			}
			error(E_BIND_REQ, EXIT | NOCORE);
		}

		if ( t_free(req, T_BIND) )
			tli_error(E_T_FREE, EXIT);

		if ( t_free(ret, T_BIND) )
			tli_error(E_T_FREE, EXIT);
		return(retval);
	}
	return((unsigned int) 0);
}


/* 
 * Clean-up server children corpses. Invoked on receiving the SIGCHLD signal.
 */
void 
reap_child()
{
	wait(0);
}


/*
 * divorce_parent: fork a child, make child independent of parent.
 *		   parent writes child's pid to process id file
 *		   and exits.  If it can't create or write pidfile,
 *		   child must be killed.
 *
 *	Assumptions: directory previously changed to '/'.
 *
 *	Notes: define FOREGROUND to inhibit the listener from running
 *		in the background.  Useful with DEBUGMODE.
 */

divorce_parent()
{
	char pidstring[50];	/* holds childs pid in ascii decimal */
	char scratch[128];
	register ret, i;
	extern void exit();
	struct sigvec sv;

	DEBUG((9,"in divorce_parent"));

	setpgrp();		/* listener + kids in own p-group	*/

	if ( (Pid = fork()) < 0 )
		sys_error(E_LSFORK, EXIT);

	if (Pid)  {

		/* parent: open pid output file and
		 *	   write childs process ID to it.
		 *	   If it works, exit 0.
		 *	   If it doesn't, kill the child and exit non-zero.
		 */

		i = sprintf(pidstring,"%d",Pid) + 1; /* add null */
		if ( (ret = write(Pidfd,pidstring,(unsigned)i)) != i ) {

			if (ret < 0)
				sys_error(E_PIDWRITE, CONTINUE);

			signal(SIGCHLD, SIG_IGN);
			kill(Pid, SIGTERM);
			error(E_PIDWRITE, EXIT); /* exit with proper code */
		}

	/*
	 * Lock file will be unlocked when parent exits.
	 */

		exit( 0 );	/* parent exits -- child does the work */
	}

	/*
	 * child: close everything but the network fd's
	 * but first, lock the lock file -- blocking lock will wait
	 * for parent to exit.
	 */
	close(Lckfd);
	if ((Lckfd = open(LCKNAME, O_WRONLY | O_CREAT, 0666 )) == -1) {
		
		sprintf(scratch, "Error (%d) re-locking the lock file", errno);
		logmessage(scratch);
		error(E_SYS_ERROR, EXIT | NOCORE | NO_MSG);
	}


	if (locking(Lckfd, 1, 0L) == -1)  {
		sprintf(scratch, "Error (%d) re-locking the lock file", errno);
		logmessage(scratch);
		error(E_SYS_ERROR, EXIT | NOCORE | NO_MSG);
	}

	/* Clean up server children corpses -- using sigvec like this
 	   prevents handler from getting reset when caught, whereas 
	   system V signal() routine resets handler when caught */
	sv.sv_handler = (void (*)())reap_child;
	sv.sv_mask = 0;
	sv.sv_flags = SV_INTERRUPT;
	sigvec(SIGCHLD, &sv, (struct sigvec *) 0);
	Background = 1;
	close_files();

}


/*
 * close_files:
 *		Close all files except what we opened explicitly
 *		Including stdout, stderr and anything else
 *		that may be open by whatever exec'ed me.
 *		We also set the close on exec flag on all fd's
 *		Note, that we keep fd's 0, 1, and 2 reserved for
 *		servers by opening them to "/dev/null".
 */

close_files()
{
	register int i;
	register int ndesc;

	fclose(stdin);
	fclose(stdout);
	fclose(stderr);

	if (((i = open("/dev/null", O_RDWR)) != 0) || (dup(i) != 1) ||
	    (dup(i) != 2))  {
		logmessage("Trouble opening/duping /dev/null");
		sys_error(E_SYS_ERROR, EXIT | NOCORE);
	}

	ndesc = ulimit(4, 0L);	/* get value for NOFILE */
	for (i = 3; i < ndesc; i++)  {	/* leave stdout, stderr open	*/
		fcntl(i, F_SETFD, 1);	/* set close on exec flag*/
	}
}


/*
 * catch_signals:
 *		Ignore some, catch the rest. Use SIGTERM to kill me.
 */

catch_signals()
{
	extern void sigterm();
	register int i;
	struct sigvec sv;

	for (i = 1; i < SIGKILL; i++)
		signal(i, SIG_IGN);
	for (i = SIGKILL + 1; i < SIGTERM; i++)
		signal(i, SIG_IGN);

	sv.sv_handler = (void (*)())sigterm;
	sv.sv_mask = 0;
	sv.sv_flags = SV_INTERRUPT;
	sigvec(SIGTERM, &sv, (struct sigvec *) 0);

	for (i = SIGTERM + 1; i < NSIG; i++)
		signal(i, SIG_IGN);
	errno = 0;	/* SIGWIND and SIGPHONE only on UNIX PC */
}


/*
 * rst_signals:
 *		After forking but before exec'ing a server,
 *		reset all signals to defaults.
 *		exec automatically resets 'caught' sigs to SIG_DFL.
 *		(This is quick and dirty for now.)
 */

rst_signals()
{
	register int i;

	for (i = 1; i < SIGKILL; i++)
		signal(i, SIG_DFL);
	for (i = SIGKILL + 1; i < SIGTERM; i++)
		signal(i, SIG_DFL);
	for (i = SIGTERM + 1; i < NSIG; i++)
		signal(i, SIG_DFL);
	errno = 0;	/* SIGWIND and SIGPHONE only on UNIX PC */
}


/*
 * sigterm:	Clean up and exit.
 */

void
sigterm()
{
	signal(SIGTERM, SIG_IGN);
	error(E_SIGTERM, EXIT | NORMAL | NOCORE);	/* calls cleanup */
}


/*
 * listen:	listen for and process connection requests.
 */

static struct pollfd pollfds[2];
static struct t_discon *disc;

listen()
{
	register fd, i;
	int count;
	register struct pollfd *sp;
	struct call_list *fhead, *phead; /* free and pending heads */

	DEBUG((9,"in listen"));
	if (Rxname) {
		count = 2;	/* how many fd's are we listening on */
		pollfds[0].fd = Nfd1;	/* tells poll() what to do		*/
		pollfds[1].fd = Nfd2;
		pollfds[0].events = pollfds[1].events = POLLIN;
	}
	else {
		count = 1;
		pollfds[0].fd = Nfd2;
		pollfds[0].events = POLLIN;
	}
	errno = t_errno = 0;

/*
 * for receiving disconnects allocate one and be done with it
 */

	while (!(disc = (struct t_discon *)t_alloc(Nfd2,T_DIS,T_ALL)) ) {
		if ((t_errno != TSYSERR) || (errno != EAGAIN))
			tli_error(E_T_ALLOC, EXIT);
		else
			tli_error(E_T_ALLOC, CONTINUE);
	}

	for (;;) {
		DEBUG((9,"listen(): TOP of loop"));
		if (Rxname) {
			pollfds[0].revents = pollfds[1].revents = 0;
		}
		else {
			pollfds[0].revents = 0;
		}
		DEBUG((7,"waiting for a request (via poll)"));

		if (poll(pollfds, count, -1) == -1) {
			/* Assume due to a child server exiting, so just continue */
			if (errno == EINTR)
				continue;
			else
				sys_error(E_POLL, EXIT);
		}

		for (i = 0, sp = pollfds; i < count; i++, sp++) {
			switch (sp->revents) {
			case POLLIN:
				fd = sp->fd;
				if (fd == Nfd1) {
					/* This can't happen if no remote login is
					   available (Rxname == NULL, count == 1,
					   Nfd1 == -1) */
					fhead = &Rfree;
					phead = &Rpend;
				}
				else {
					fhead = &Lfree;
					phead = &Lpend;
				}
				doevent(fhead, phead, fd);
				trycon(fhead, phead, fd);
				break;
			case 0:
				break;
			/* distinguish the various errors for the user */
			case POLLERR:
				logmessage("poll() returned POLLERR");
				error(E_SYS_ERROR, EXIT | NO_MSG);
			case POLLHUP:
				logmessage("poll() returned POLLHUP");
				error(E_SYS_ERROR, EXIT | NO_MSG);
			case POLLNVAL:
				logmessage("poll() returned POLLNVAL");
				error(E_SYS_ERROR, EXIT | NO_MSG);
			case POLLPRI:
				logmessage("poll() returned POLLPRI");
				error(E_SYS_ERROR, EXIT | NO_MSG);
			case POLLOUT:
				logmessage("poll() returned POLLOUT");
				error(E_SYS_ERROR, EXIT | NO_MSG);
			default:
				logmessage("poll() returned unrecognized event");
				error(E_SYS_ERROR, EXIT | NO_MSG);
			}
		}
	}
}


/*
 * doevent:	handle an asynchronous event
 */

doevent(fhead, phead, fd)
struct call_list *fhead;
struct call_list *phead;
int fd;
{
	register struct callsave *current;
	register struct t_call *call;
	char scratch[BUFSIZ];

	DEBUG((9, "in doevent"));
	switch (t_look(fd)) {
	case 0:
		sys_error(E_POLL, EXIT);
		/* no return */
	case T_LISTEN:
		current = dequeue(fhead);
		call = current->c_cp;
		if (t_listen(fd, call) < 0) {
			tli_error(E_T_LISTEN, CONTINUE);
			return;
		}
		queue(phead, current);
		sprintf(scratch, "Connect pending on fd %d, seq # %d", fd, call->sequence);
		logmessage(scratch);
		DEBUG((9, "incoming call seq # %d", call->sequence));
		break;
	case T_DISCONNECT:
		if (t_rcvdis(fd, disc) < 0) {
			tli_error(E_T_RCVDIS, EXIT);
			/* no return */
		}
		sprintf(scratch, "Disconnect on fd %d, seq # %d", fd, disc->sequence);
		logmessage(scratch);
		DEBUG((9, "incoming disconnect seq # %d", disc->sequence));
		pitchcall(fhead, phead, disc);
		break;
	default:
		tli_error(E_T_LOOK, CONTINUE);
		break;
	}
}


/*
 * trycon:	try to accept a connection
 */

trycon(fhead, phead, fd)
struct call_list *fhead;
struct call_list *phead;
int fd;
{
	register struct callsave *current;
	register struct t_call *call;
	int pid;
	char scratch[BUFSIZ];

	DEBUG((9, "in trycon"));
	while (!EMPTY(phead)) {
		current = dequeue(phead);
		call = current->c_cp;
		DEBUG((9, "try to accept #%d", call->sequence));
		close(0);
		SPLhi();
		if ((Nfd3 = open_bind(NULL, 0, 0, (unsigned int *) 0)) != 0) {
			error(E_OPENBIND, CONTINUE);
			clr_call(call);
			queue(fhead, current);
			continue;	/* let transport provider generate disconnect */
		}
		SPLlo();
		if (t_accept(fd, Nfd3, call) < 0) {
			if (t_errno == TLOOK) {
				t_close(Nfd3);
				SPLhi();
				if (dup(1) != 0)
					logmessage("Trouble duping fd 0");
				SPLlo();
				logmessage("t_accept collision");
				DEBUG((9, "save call #%d", call->sequence));
				pqueue(phead, current);
				return;
			}
			else {
				t_close(Nfd3);
				SPLhi();
				if (dup(1) != 0)
					logmessage("Trouble duping fd 0");
				SPLlo();
				tli_error(E_T_ACCEPT, CONTINUE);
				clr_call(call);
				queue(fhead, current);
				return;
			}
		}
		sprintf(scratch, "Accepted request on fd %d, seq # %d", Nfd3, call->sequence);
		logmessage(scratch);
		DEBUG((9, "Accepted call %d", call->sequence));
		check_dbf();
		if ((pid = fork()) < 0)
			log(E_FORK_SERVICE);
		else if (!pid) {
			startit(Nfd3, call, (fd == Nfd1 ? LOGIN : GEN));
			/* no return */
		}
		clr_call(call);
		t_close(Nfd3);
		queue(fhead, current);
		SPLhi();
		if (dup(1) != 0)
			logmessage("Trouble duping fd 0");
		SPLlo();
	}
}


/*
 * startit:	start up a server
 */

startit(fd, call, servtype)
int fd;
struct t_call *call;
int servtype;
{
	DEBUG((9, "in startit"));
	setpgrp();
	Child = 1;
	if (servtype == LOGIN)
		login(fd, call);
	else
		process(fd, call);

/*
 * if login() or process() returns, an error
 * was encountered.  Errors should be logged
 * by the subroutines -- exit done here.
 * (Subroutines shouldn't care if they are
 * a parent/child/whatever.)
 *
 * NOTE: timeout() is an exception; 
 * it does an exit.
 * There can be no TLI calls after this point.
 * (non-tli modules may have been pushed)
 *
 */

	DEBUG((9, "return in startit"));
#ifdef	COREDUMP
	abort();
#endif
	exit(1); /* server failed, don't log */
}


/*
 * process:	process an accepted connection request
 *		for ms-net or NLP service.
 */

process(fd, call)
register fd;
struct t_call *call;
{
	register size;
	char buf[RCVBUFSZ];
	char **argv;
	register char *bp = buf;
	register dbf_t *dbp;
	extern dbf_t *getdbfentry();
	extern char **mkdbfargv();

	DEBUG((9,"in process (NLPS/SMB message)"));

	if ((size = getrequest(fd, bp)) <= 0)  {
		logmessage("process(): No/bad service request received");
		return(-1);
	}

	if (size < MINMSGSZ)  {
		DEBUG((7,"process(): msg size (%d) too small",size));
		logmessage("process(): Message size too small");
		return(-1);
	}

	/*
	 * if message is NLPS protocol...
	 */

	if ((!strncmp(bp,NLPSIDSTR,NLPSIDSZ))  && 	/* NLPS request	*/
	    (*(bp + NLPSIDSZ) == NLPSSEPCHAR)) {
		nls_service(fd, bp, size, call);
		(void)sleep(10);	/* if returned to here, then 
				 * must sleep for a short period of time to
				 * insure that the client received any possible
				 * exit response message from the listener.
					 */

	/*
	 * else if message is for the MS-NET file server...
	 */

	} else if ( (*bp == (char)0xff) && (!strncmp(bp+1,SMBIDSTR,SMBIDSZ)) )  {
		if (dbp = getdbfentry(DBF_SMB_CODE))
		    if (dbp->dbf_flags & DBF_OFF)
			logmessage("SMB message, server disabled in data base");
		    else    {
			argv = mkdbfargv(dbp);
			smbservice(fd, bp, size, call, argv);
		    }
		else
			logmessage("SMB message, no data base entry");

	/*
	 * else, message type is unknown...
	 */

	} else  {
		logmessage("Process(): Unknown service request (ignored)");
		DEBUG((7,"msg size: %d; 1st four chars (hex) %x %x %x %x",
			*bp, *(bp+1), *(bp+2), *(bp+3)));
	}

	/*
	 * the routines that start servers return only if there was an error
	 * and will have logged their own errors.
	 */

	return(-1);
}


/*
 * getrequest:	read in a full message.  Timeout, in case the client died.
 *		returns: -1 = timeout or other error.
 *			 positive number = message size.
 */

int
getrequest(fd, bp)
register fd;
register char *bp;
{
	register size;
	register char *tmp = bp;
	int flags;
	extern void timeout();
	extern unsigned alarm();
	unsigned short cnt;

	DEBUG((9,"in getrequest"));

	signal(SIGALRM, timeout);
	(void)alarm(ALARMTIME);

	/* read in MINMSGSZ to determine type of msg */
	if ((size = l_rcv(fd, bp, MINMSGSZ, &flags)) != MINMSGSZ) {
		DEBUG((9, "getrequest: l_rcv returned %d", size));
		tli_error(E_RCV_MSG, CONTINUE);
		return(-1);
	}
	tmp += size;

	/*
	 * if message is NLPS protocol...
	 */

	if ((!strncmp(bp,NLPSIDSTR,NLPSIDSZ))  && 	/* NLPS request	*/
	    (*(bp + NLPSIDSZ) == NLPSSEPCHAR)) {

		do {
			if (++size > RCVBUFSZ) {
				logmessage("Getrequest(): recieve buffer not large enough");
				return(-1);
			}

			if (t_rcv(fd, tmp, sizeof(char), &flags) != sizeof(char)) {
				tli_error(E_RCV_MSG, CONTINUE);
				return(-1);
			}

		} while (*tmp++ != '\0');



	/*
	 * else if message is for the MS-NET file server...
	 */

	} else if ( (*bp == (char)0xff) && (!strncmp(bp+1,SMBIDSTR,SMBIDSZ)) )  {

		/* read in 28 more bytes to get count of paramter words */
		if (l_rcv(fd, tmp, 28, &flags) != 28) {
			tli_error(E_RCV_MSG, CONTINUE);
			return(-1);
		}
		tmp += 28;
		size += 28;

		/*
		 * read amount of paramater words plus word for
                 * the number of data bytes to follow (2 bytes/word)
                 */
		cnt = (int)*(tmp - 1) * 2 + 2;

		if ((size += cnt) > RCVBUFSZ) {
			logmessage("Getrequest(): recieve buffer not large enough");
			return(-1);
		}

		if (l_rcv(fd, tmp, cnt, &flags) != cnt) {
			tli_error(E_RCV_MSG, CONTINUE);
			return(-1);
		}
		tmp += cnt;

		getword(tmp - 2, &cnt);

		if ((size += cnt) > RCVBUFSZ) {
			logmessage("Getrequest(): recieve buffer not large enough");
			return(-1);
		}

		if (l_rcv(fd, tmp, cnt, &flags) != cnt) {
			tli_error(E_RCV_MSG, CONTINUE);
			return(-1);
		}

		nullfix(fd);

	/*
	 * else, message type is unknown...
	 */

	} else  {
		logmessage("Getrequest(): Unknown service request (ignored)");
		DEBUG((7,"msg size: %d; 1st four chars (hex) %x %x %x %x",
			*bp, *(bp+1), *(bp+2), *(bp+3)));
		return(-1);
	}

	(void)alarm(0);
	signal(SIGALRM, SIG_IGN);

	DEBUG((7,"t_rcv returned %d, flags: %x",size,flags));

	return(size);
}


/*
 * The following code is for patching a 6300 side bug.  The original
 * message that comes over may contain 2 null bytes which aren't
 * part of the message, and if left on the stream, will poison the
 * server.  Peek into the stream and snarf up those bytes if they
 * are there.  If anything goes wrong with the I_PEEK, just continue,
 * if the nulls weren't there, it'll work, and if they were, all that
 * will happen is that the server will fail.  Just note what happened
 * in the log file.
 */

nullfix(fd)
int fd;
{
	struct strpeek peek;
	register struct strpeek *peekp;
	char scratch[BUFSIZ];
	char junk[2];
	int flags;
	int ret;

	peekp = &peek;
	peekp->flags = 0;
	/* need to ask for ctl info to avoid bug in I_PEEK code */
	peekp->ctlbuf.maxlen = 1;
	peekp->ctlbuf.buf = junk;
	peekp->databuf.maxlen = 2;
	peekp->databuf.buf = junk;
	ret = ioctl(fd, I_PEEK, &peek);
	if (ret == -1) {
		sprintf(scratch, "nullfix(): unable to PEEK, errno is %d", errno);
		DEBUG((9, "nullfix(): I_PEEK failed, errno is %d", errno));
		logmessage(scratch);
	}
	else if (ret == 0) {
		DEBUG((9, "nullfix(): no messages on stream to PEEK"));
	}
	else {
		if (peekp->databuf.len == 2) {
			/* Note: junk contains "peeked" data */
			DEBUG((9, "peeked <%x> <%x>", junk[0], junk[1]));
			if ((junk[0] == 0) && (junk[1] == 0)) {
				/* pitch the nulls */
				DEBUG((9, "pitching 2 nulls from first peek"));
				l_rcv(fd, junk, 2, &flags);
			}
		}

		/*
		 * this represents a somewhat pathological case where
		 * the "2 nulls" are broken across message boundaries.
		 * Pitch the first and hope the next one is there
		 */

		else if (peekp->databuf.len == 1) {
			DEBUG((9, "peeked <%x>", junk[0]));
			if (junk[0] == 0) {
				/* pitch the first */
				DEBUG((9, "split nulls, pitching first"));
				l_rcv(fd, junk, 1, &flags);
				peekp->databuf.maxlen = 1;
				ret = ioctl(fd, I_PEEK, &peek);
				if (ret == -1) {
					sprintf(scratch, "nullfix(): unable to PEEK second time, errno is %d", errno);
					DEBUG((9, "second peek failed, errno %d", errno));
					logmessage(scratch);
				}
				else if (ret == 0) {
					DEBUG((9, "no messages for 2nd peek"));
				}
				else {
					if (peekp->databuf.len == 1) {
						DEBUG((9, "2nd peek <%x>", junk[0]));
						if (junk[0] == 0) {
							/* pitch the second */
							DEBUG((9, "pitching 2nd single null"));
							l_rcv(fd, junk, 1, &flags);
						}
						else {
							/* uh oh, server will most likely fail */
							DEBUG((9, "2nd null not found"));
							logmessage("nullfix(): threw away a valid null byte");
						}
					}
				}
			}
		}
	}
}


/*
 * timeout:	SIGALRM signal handler.  Invoked if t_rcv timed out.
 *		See comments about 'exit' in process().
 */


void
timeout()
{
	DEBUG((9, "TIMEOUT"));
	error(E_RCV_TMO, EXIT | NOCORE);
}



/*
 * nls_service:	Validate and start a server requested via the NLPS protocol
 *
 *		version 0:1 -- expect "NLPS:000:001:service_code".
 *
 *	returns only if there was an error (either msg format, or couldn't exec)
 */

static char *badversion =
	"Unknown version of an NLPS service request: %d:%d";
static char *disabledmsg =
	"Request for service code <%s> denied, service is disabled";
static char *nlsunknown =
	"Request for service code <%s> denied, unknown service code";


/*
 * Nlsversion can be used as a NLPS flag (< 0 == not nls service)
 * and when >= 0, indicates the version of the NLPS protocol used
 */

static int Nlsversion = -1;	/* protocol version	*/

int
nls_service(fd, bp, size, call)
int fd, size;
char *bp;
struct t_call *call;
{
	int low, high;
	char svc_buf[64];
	register char *svc_code_p = svc_buf;
	char scratch[256];
	register dbf_t *dbp;
	extern dbf_t *getdbfentry();
	extern char **mkdbfargv();


	if (nls_chkmsg(bp, size, &low, &high, svc_code_p))  {
		if ((low == 0) || (low == 2))
			Nlsversion = low;
		else  {
			sprintf(scratch, badversion, low, high);
			logmessage(scratch);
			error(E_BAD_VERSION, CONTINUE);
			return(-1);
		}

		DEBUG((9,"nls_service: protocol version %d", Nlsversion));

		/*
		 * common code for protocol version 0 or 2
		 * version 0 allows no answerback message
		 * version 2 allows exactly 1 answerback message
		 */

		if (dbp = getdbfentry(svc_code_p))
		    if (dbp->dbf_flags & DBF_OFF)  {
			sprintf(scratch, disabledmsg, svc_code_p);
			logmessage(scratch);
			nls_reply(NLSDISABLED, scratch);
		    }  else
			start_server(fd, dbp, (char **)0, call);
			/* return is an error	*/
		else  {
			sprintf(scratch, nlsunknown, svc_code_p);
			logmessage(scratch);
			nls_reply(NLSUNKNOWN, scratch);
		}

	}  else
		error(E_BAD_FORMAT, CONTINUE);

	/* if we're still here, server didn't get exec'ed	*/

	return(-1);
}



/*
 * nls_chkmsg:	validate message and return fields to caller.
 *		returns: TRUE == good format
 *			 FALSE== bad format
 */

nls_chkmsg(bp, size, lowp, highp, svc_code_p)
char *bp, *svc_code_p;
int size, *lowp, *highp;
{

	/* first, make sure bp is null terminated */

	if ((*(bp + size - 1)) != (char)0)
		return(0);

	/* scanf returns number of "matched and assigned items"	*/

	return(sscanf(bp, "%*4c:%3d:%3d:%s", lowp, highp, svc_code_p) == 3);

}


/*
 * nls_reply:	send the "service request response message"
 *		when appropriate.  (Valid if running version 2 or greater).
 *		Must use write(2) since unknown modules may be pushed.
 *
 *		Message format:
 *		protocol_verion_# : message_code_# : message_text
 */

static char *srrpprot = "%d:%d:%s";

nls_reply(code, text)
register code;
register char *text;
{
	char scratch[256];

	/* Nlsversion = -1 for login service */

	if (Nlsversion >= 2)  {
		DEBUG((7, "nls_reply: sending response message"));
		sprintf(scratch, srrpprot, Nlsversion, code, text);
		t_snd(0, scratch, strlen(scratch)+1, 0);
	}
}


/*
 * common code to  start a server process (for any service)
 * if optional argv is given, info comes from o_argv, else pointer
 * to dbf struct is used.  In either case, first argument in argv is
 * full pathname of server. Before exec-ing the server, the caller's
 * logical address, opt and udata are addded to the environment. 
 */

start_server(netfd, dbp, o_argv, call)
int netfd;
register dbf_t *dbp;
register char **o_argv;
struct t_call *call;
{
	char *path;
	char **argvp;
	extern char **environ;
	extern struct passwd *getpwnam();
	extern void endpwent();
	extern struct group *getgrgid();
	register struct passwd *pwdp;
	struct group *grpp;
	char msgbuf[256];
	register dbf_t *wdbp = dbp;

	/*
	 * o_argv is set during SMB service setup only, in
	 * which case dbp is NULL.
	 */

	if (o_argv) {
		argvp = o_argv;
		if ((wdbp = getdbfentry(DBF_SMB_CODE)) == NULL) {
			/* this shouldn't happen because at this point we've
			   already found it once */
			logmessage("SMB message, missing data base entry");
			exit(2); /* server, don't log */
		}
	}
	else
		argvp = mkdbfargv(dbp);
	path = *argvp;

	/* set up stdout and stderr before pushing optional modules */

	(void) close(1);
	(void) close(2);

	if (dup(0) != 1 || dup(0) != 2) {
		logmessage("Dup of fd 0 failed");
		exit(2); /* server, don't log */
	}

	sprintf(msgbuf,"Starting server (%s)",path);
	nls_reply(NLSSTART, msgbuf);
	logmessage(msgbuf);

	/* after pushmod, tli calls are questionable?		*/

	if (dbp && pushmod(netfd, dbp->dbf_modules)) {
		logmessage("Can't push server's modules: exit");
		exit(2); /* server, don't log */
	}

	rst_signals();

	if (wdbp == NULL) {
		logmessage("No database entry");
		exit(2); /* server, don't log */
	}

/*
	if ((pwdp = getpwnam(wdbp->dbf_id)) == NULL) {
		sprintf(msgbuf, "Missing or bad passwd entry for <%s>", wdbp->dbf_id);
		logmessage(msgbuf);
		exit(2); */ /* server, don't log */
/*
	}

	if (setgid(pwdp->pw_gid)) {
		if ((grpp = getgrgid(pwdp->pw_gid)) == NULL) {
			sprintf(msgbuf, "No group entry for %d", pwdp->pw_gid);
			logmessage(msgbuf);
			exit(2); */ /* server, don't log */
/*
		}
		sprintf(msgbuf, "Cannot set group id to %s", grpp->gr_name);
		logmessage(msgbuf);
		exit(2); */ /* server, don't log */
/*
	}

	if (setuid(pwdp->pw_uid)) {
		sprintf(msgbuf, "Cannot set user id to %s", wdbp->dbf_id);
		logmessage(msgbuf);
		exit(2); */ /* server, don't log */
/*
	}

	if (chdir(pwdp->pw_dir)) {
		sprintf(msgbuf, "Cannot chdir to %s", pwdp->pw_dir);
		logmessage(msgbuf);
		exit(2); */ /* server, don't log */
/*
	}

	DEBUG((9, "New uid %d New gid %d", getuid(), getgid()));
	if (senviron(call, pwdp->pw_dir))  {
		logmessage("Can't expand server's environment");
	}
	endpwent();
*/

	execve(path, argvp, environ);

	/* exec returns only on failure!		*/

	logmessage("listener could not exec server");
	sys_error(E_SYS_ERROR, CONTINUE);
	return(-1);
}


/*
 * senviron:	Update environment before exec-ing the server:
 *		The callers logical address is placed in the
 *		environment in hex/ascii character representation.
 *
 * Note:	no need to free the malloc'ed buffers since this process
 *		will either exec or exit.
 */

static char provenv[2*PATHSIZE];
static char homeenv[BUFSIZ];
static char tzenv[BUFSIZ];

#define TIMEZONE	"/etc/TIMEZONE"
#define TZSTR	"TZ="

int
senviron(call, home)
register struct t_call *call;
register char *home;
{
	register char *p;
	char scratch[BUFSIZ];
	extern void nlsaddr2c();
	extern char *getenv();
	char *parse();
	FILE *fp;

	sprintf(homeenv, "HOME=%s", home);
	putenv(homeenv);

/*
 * The following code handles the case where the listener was started with
 * no environment.  If so, supply a reasonable default path and find out
 * what timezone we're in so the log file is consistent.
 */

	if (getenv("PATH") == NULL)
		putenv("PATH=/bin:/etc:/usr/bin");
	if (getenv("TZ") == NULL) {
		fp = fopen(TIMEZONE, "r");
		if (fp) {
			while (fgets(tzenv, BUFSIZ, fp)) {
				if (scratch[strlen(tzenv) - 1] == '\n')
					tzenv[strlen(tzenv) - 1] = '\0';
				if (!strncmp(TZSTR, tzenv, strlen(TZSTR))) {
					putenv(parse(tzenv));
					break;
				}
			}
			fclose(fp);
		}
		else {
			sprintf(scratch, "couldn't open %s, default to GMT", TIMEZONE);
			logmessage(scratch);
		}
	}

	if ((p = (char *)malloc(((call->addr.len)<<1) + 18)) == NULL)
		return(-1);
	strcpy(p, NLSADDR);
	strcat(p, "=");
	nlsaddr2c(p + strlen(p), call->addr.buf, call->addr.len);
	DEBUG((7, "Adding %s to server's environment", p));
	putenv(p);

	if ((p = (char *)malloc(((call->opt.len)<<1) + 16)) == NULL)
		return(-1);
	strcpy(p, NLSOPT);
	strcat(p, "=");
	nlsaddr2c(p + strlen(p), call->opt.buf, call->opt.len);
	DEBUG((7, "Adding %s to server's environment", p));
	putenv(p);

	p = provenv;
	strcpy(p, NLSPROVIDER);
	strcat(p, "=");
	strcat(p, Netspec);
	DEBUG((7, "Adding %s to environment", p));
	putenv(p);

	if ((p = (char *)malloc(((call->udata.len)<<1) + 20)) == NULL)
		return(-1);
	strcpy(p, NLSUDATA);
	strcat(p, "=");
	if ((int)call->udata.len >= 0)
		nlsaddr2c(p + strlen(p), call->udata.buf, call->udata.len);
	DEBUG((7, "Adding %s to server's environment", p));
	putenv(p);
	return (0);
}


/*
 * parse:	Parse TZ= string like init does for consistency
 *		Work on string in place since result will
 *		either be the same or shorter.
 */

char *
parse(s)
char *s;
{
	register char *p;
	register char *tp;
	char scratch[BUFSIZ];
	int delim;

	tp = p = s + strlen("TZ=");	/* skip TZ= in parsing */
	if ((*p == '"') || (*p == '\'')) {
		/* it is quoted */
		delim = *p++;
		for (;;) {
			if (*p == '\0') {
				/* etc/TIMEZONE ill-formed, go without TZ */
				sprintf(scratch, "%s ill-formed", TIMEZONE);
				logmessage(scratch);
				strcpy(s, "TZ=");
				return(s);
			}
			if (*p == delim) {
				*tp = '\0';
				return(s);
			}
			else {
				*tp++ = *p++;
			}
		}
	}
	else { /* look for comment or trailing whitespace */
		for ( ; *p && !isspace(*p) && *p != '#'; ++p)
			;
		/* if a comment or trailing whitespace, trash it */
		if (*p) {
			*p = '\0';
		}
		return(s);
	}
}


/*
 * login:	Start the intermediary process that handles
 *		pseudo-tty getty/login service.
 */

login(fd, call)
register fd;
struct t_call *call;
{
	register dbf_t *dbp;

	DEBUG((9,"in login (request for intermediary)"));

	if (!(dbp = getdbfentry(DBF_INT_CODE)) || (dbp->dbf_flags & DBF_OFF))  {
		log( E_NOINTERMEDIARY );
		return(-1);
	}

	start_server(fd, dbp, (char **)0, call);

	/* returns only on failure!			*/

	logmessage("listener could not exec the intermediary process");
	sys_error(E_SYS_ERROR, CONTINUE);

	return(-1);
}


/*
 * isdigits:	string version of isdigit.  (See ctype(3))
 */

int
isdigits(p)
register char *p;
{
	register int flag = 1;

	if (!strlen(p))
		return(0);

	while (*p)
		if (!isdigit(*p++))
			flag = 0;
	return(flag);
}


/*
 * pushmod:	push modules if defined in the data base entry.
 *
 *		NOTE: there are no modules to push in WTLI.
 *		so this code is a noop on the S4.  However,
 *		the data base file is compatible.
 *
 *		WARNING: This routine writes into the in-memory copy
 *		of the database file.  Therefore, after it is called,
 *		the incore copy of the database file will no longer be valid.
 */

int
pushmod(fd, mp)
int fd;
register char *mp;
{
#ifndef	S4
	register char *cp = mp;
	register int pflag = 0;
	char name[32];

	DEBUG((9,"in pushmod:"));
	if (!mp) {
		DEBUG((9,"NULL list: exiting pushmod"));
		return(0);
	}
	while (*cp) {
	    if (*cp == ',') {
	    	*cp = (char)0;
		if (*mp && strcmp(mp, "NULL")) {

	/*
	 * if first time thru, pop off TIMOD if it is on top of stream
	 */

		    if  (!pflag) {
			pflag++;
			if (ioctl(fd, I_LOOK, name) >= 0) {
			    if (strcmp(name, "timod") == 0) {
				if (ioctl(fd, I_POP) < 0)
				    DEBUG((9,"pushmod: I_POP failed"));
			    }
			}
		    }

		    DEBUG((9,"pushmod: about to push %s",mp));
		    if (ioctl(fd, I_PUSH, mp) < 0) {
			DEBUG((9,"pushmod: ioctl failed, errno = %d",errno));
			return(1);
		    }

		} /* if */
		mp = ++cp;
		continue;
	    } /* if */
	    cp++;
	} /* while */
	DEBUG((9,"exiting pushmod:"));
#endif
	return(0);
}


int
l_rcv(fd, bufp, bytes, flagp)
int fd;
char *bufp;
int bytes;
int *flagp;
{
	register int n;
	register int count = bytes;
	register char *bp = bufp;

	DEBUG((9, "in l_rcv"));

	do {
		*flagp = 0;
		n = t_rcv(fd, bp, count, flagp);
		if (n < 0) {
			DEBUG((9, "l_rcv, t_errno is %d", t_errno));
#ifdef DEBUGMODE
			if (t_errno == TLOOK) {
				DEBUG((9, "l_rcv, t_look returns %d", t_look(fd)));
			}
#endif
			return(n);
		}
		count -= n;
		bp += n;
	} while (count > 0);

	return(bp - bufp);
}


/*
 * clr_call:	clear out a call structure
 */

clr_call(call)
struct t_call *call;
{
	call->sequence = 0;
	call->addr.len = 0;
	call->opt.len = 0;
	call->udata.len = 0;
	memset(call->addr.buf, 0, call->addr.maxlen);
	memset(call->opt.buf, 0, call->opt.maxlen);
	memset(call->udata.buf, 0, call->udata.maxlen);
}


/*
 * pitchcall: remove call from pending list
 */

pitchcall(free, pending, disc)
struct call_list *free;
struct call_list *pending;
struct t_discon *disc;
{
	register struct callsave *p, *oldp;

	DEBUG((9, "pitching call, sequence # is %d", disc->sequence));
	if (EMPTY(pending)) {
		disc->sequence = -1;
		return;
	}
	p = pending->cl_head;
	oldp = (struct callsave *) NULL;
	while (p) {
		if (p->c_cp->sequence == disc->sequence) {
			if (oldp == (struct callsave *) NULL) {
				pending->cl_head = p->c_np;
				if (pending->cl_head == (struct callsave *) NULL) {
					pending->cl_tail = (struct callsave *) NULL;
				}
			}
			else if (p == pending->cl_tail) {
				oldp->c_np = p->c_np;
				pending->cl_tail = oldp;
			}
			else {
				oldp->c_np = p->c_np;
			}
			clr_call(p->c_cp);
			queue(free, p);
			disc->sequence = -1;
			return;
		}
		oldp = p;
		p = p->c_np;
	}
	logmessage("received disconnect with no pending call");
	disc->sequence = -1;
	return;
}
