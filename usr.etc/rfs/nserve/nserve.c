/*	@(#)nserve.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nserve:nserve.c	1.26.1.1"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/utsname.h>
#include <errno.h>
#include <fcntl.h>
#include "nsdb.h"
#include "nslog.h"
#include <tiuser.h>
#include <rfs/nsaddr.h>
#include "stdns.h"
#include <rfs/nserve.h>
#include "nsports.h"
#include "nsrec.h"
#include <unistd.h>
#include <rfs/rfsys.h>

#ifdef LOGGING
#define USAGE "Usage: %s [-krv][-l level][-f logfile][-n master][-t secs][-p addr] net_spec\n\
	level = one or more of 'abcdmnopst'\n"
#define GETOPT_ARG "t:n:l:f:p:krvN"
#else
#define USAGE "Usage: %s [-kv][-p address] net_spec\n"
#define GETOPT_ARG "p:kv"
#endif

char	Dname[MAXDNAME];	/* machine's full domain name	*/
int	Done=FALSE;		/* set before exit		*/
char	*Netmaster=NETMASTER;	/* master file for name server	*/
int	Primary=FALSE;		/* is this machine primary?	*/
int	Oedipus=FALSE;		/* kill parent after startup?	*/
int	Verify=FALSE;		/* verify incoming requests?	*/
int	Recover=TRUE;		/* Recovery enabled		*/
struct address	*Paddress[MAXNS]; /* address(es) of primaries	*/
struct address	*Myaddress=NULL;  /* address of this machine	*/
struct address	*getmyaddr();	/* just what you would think	*/
static	int	sigterm();	/* function for SIGTERM		*/
static	int	waitcld();	/* function for SIGUSR2		*/
static	int	savecld();	/* save info about children	*/
static	int	setnslock();	/* put pid in NSPID, lock.	*/
int		r_init();	/* set up recovery and ns addrs	*/
void	(*Alarmsig)()=SIG_IGN;	/* current value for alarm	*/
char	*dompart();
char	*aatos();
struct address	*astoa();

static struct childinfo {	/* struct to keep active children */
	int		c_pid;
	struct request  *c_req;
} Children[MAXCHILDREN];

extern char	*Net_spec;	/* listener's network id	*/
extern int	Polltime;	/* polling interval for backup	*/
extern char	*optarg;
extern int	optind, opterr;
extern int	sys_nerr;
extern char	*sys_errlist[];
#define CHILDTIME	300	/* length of sanity timer for child process */
#define CHILDWAIT	10	/* maximum time to wait for child to exit   */

main(argc,argv)
int	argc;
char	*argv[];
{
	int	c;
	int	ret;
	int	errflg=0;
	FILE	*fopen();
	int	pd;
	int	ppid;
	char	tmpbuf[BUFSIZ];
	int	nofork = 0;

	while ((c = getopt(argc,argv,GETOPT_ARG)) != EOF)
		switch (c) {
		case 'n':	/* net master file	*/
			Netmaster = optarg;
			break;
		case 'l':	/* log level	*/
			Loglevel = setlog(optarg);
			break;
		case 'f':	/* log file	*/
			if ((Logfd = fopen(optarg,"w")) == NULL) {
			    Logfd = stderr;
			    fprintf(stderr,
				"nserve: WARNING can't open logfile '%s'\n",optarg);
			}
			else {
				fclose(Logfd);
				Logfd = freopen(optarg,"w",stderr);
			}
			break;
		case 'p':	/* here is the primary's address */
			sprintf(tmpbuf,"%s",optarg);
			LOG3(L_ALL,"(%5d) nserve: set Paddress to %s\n",
				Logstamp,tmpbuf);
			if ((Paddress[0] = astoa(tmpbuf,NULL)) == NULL) {
			    fprintf(stderr,
				"nserve: -p option given bad primary address=%s\n",
				tmpbuf);
			    errflg++;
			}
			break;
		case 'k':	/* kill parent	*/
			Oedipus = TRUE;
			break;
		case 'v':	/* enable verification. */
			Verify = TRUE;
			break;
		case 'r':	/* turn recovery off	*/
			Recover = FALSE;
			break;
		case 't':	/* set poll time	*/
			if ((Polltime = atoi(optarg)) < 0)
				Polltime = POLLTIME;
			break;
		case 'N':
			nofork = 1;
			break;
		case '?':
			errflg++;
			break;
		}

	ppid = getppid();

	if (errflg || optind >= argc) {
		if (Oedipus)
			kill(ppid, SIGUSR2);
		else
			fprintf(stderr,USAGE,argv[0]);
		exit(2);
	}

	Net_spec = argv[optind];

	if (netname(Dname) == -1) {
		if (Oedipus) kill(ppid, SIGUSR2);
		exit(2);
	}

	/* need to put this in background if not started by rfstart */
	if (!nofork) {
		if (!Oedipus && fork())
			exit(0);
	}

	sigset(SIGHUP,SIG_IGN);
	sigset(SIGINT,SIG_IGN);
	sigset(SIGQUIT,SIG_IGN);
	sigset(SIGTERM,SIG_IGN);
	sigset(SIGALRM,SIG_IGN);

	if (setnslock()) {
		PLOG1("RFS Name server already running\n");
		if (Oedipus) kill(ppid, SIGUSR2);
		exit(2);
	}

	/* initialize the database	*/

	if (Paddress[0]) {
		Paddress[0]->protocol = copystr(Net_spec);
		if (putprime(Paddress[0]) == RFS_FAILURE) {
			if (Oedipus) kill(ppid, SIGUSR2);
			clrnslock();
			exit(2);
		}
	}
	else if ((errflg = readdb(Netmaster)) <= 0) {
		if (errflg == 0)
			PLOG2("RFS name server: %s is empty\n",Netmaster);
		else
			PLOG3("RFS name server: can't open %s, error %s\n",
			    Netmaster,
			    (errno > sys_nerr)?"unknown error":sys_errlist[errno]);

		if (Oedipus) kill(ppid, SIGUSR2);
		clrnslock();
		exit(2);
	}
	if (merge(DOMMASTER,FALSE) == 0)
		LOG2(L_OVER,"nserve: warning, %s file empty or missing\n",
			DOMMASTER);

	if (nslisten() == -1) {
		if (Oedipus) kill(ppid, SIGUSR2);
		clrnslock();
		exit(2);
	}
	LOG1(L_OVER,"nserve: listen port opened\n");

	sigset(SIGTERM,sigterm);

	if (r_init() == RFS_FAILURE) {
		if (Oedipus) kill(ppid, SIGUSR2);
		exit(2);
	}

	/* alarm signal may have been set by r_init, so hold it quick	*/
	sigset(SIGALRM,SIG_HOLD);

	/* notify rfstart of successful start up	*/
	if (Oedipus)
		kill(ppid, SIGUSR1);

	setpgrp();
	fclose(stdin);
	fclose(stdout);

	if (Verify)
		rfsys(RF_VFLAG,V_SET);
	else
		rfsys(RF_VFLAG,V_CLEAR);

	for (Logstamp=0; Done == FALSE; Logstamp++) {
		/* only allow these signals in poll	*/
		sigset(SIGUSR2,waitcld);
		sigset(SIGALRM,Alarmsig);
		ret = nswait(&pd);
		sigset(SIGALRM,SIG_HOLD);
		sigset(SIGUSR2,SIG_HOLD);
		LOG3(L_ALL,"(%5d) nserve: got req %d\n",Logstamp, ret);

		switch (ret) {
		case LOC_REQ:
			nsreq(pd,LOCAL);
			break;
		case REC_CON:
		case REC_HUP:
		case REC_ACC:
			if (nsrecover(pd,ret) == RFS_FAILURE)
				Done = TRUE;
			break;
		case REM_REQ:
		case REC_IN:
			nsreq(pd,REMOTE);
			break;
		case NON_FATAL:
			break;
		case FATAL:
			Done = TRUE;
			break;
		}
		LOG3(L_ALL,"(%5d) nserve: processed req %d\n",Logstamp, ret);
		/* if we got sigterm or fatal, we will exit, else continue */
#ifdef LOGGING
		fflush(Logfd);
#endif
	}

	LOG2(L_ALL,"(%5d) nserve: normal exit\n",Logstamp);
	sigterm(0);	/* sigterm cleans up and exits */
	exit(0);
	/* NOTREACHED */
}
static int
nsreq(pd,mode)
int	pd;	/* port descriptor for incoming request	*/
int	mode;	/* source of request: REMOTE or LOCAL	*/
{
	char	*block=NULL;
	struct request	*req;
	struct request	*nsfunc();
	struct request	*nreq;
	struct header	*hp;
	int	cwait=FALSE;	/* set if we need to wait for child	*/
	int	size;

	LOG4(L_TRANS | L_OVER,"(%5d) nsreq: rcvd %s request on port %d\n",
		Logstamp,(mode==LOCAL)?"LOCAL":"REMOTE",pd);

	if ((size = nsread(pd,&block,0)) == -1) {
		LOG2(L_ALL,"(%5d) nsreq: nsread failed\n",Logstamp);
		nsrclose(pd);
		return;
	}

	if ((req = btoreq(block,size)) == NULL) {
		LOG2(L_ALL,"(%5d) nsreq: btoreq failed\n",Logstamp);
		if (sndback(pd,R_FORMAT) == RFS_FAILURE)
			nsrclose(pd);
		if (block)
			free(block);
		return;
	}
	if ((nreq = nsfunc(req,pd)) == NULL) {
		LOG2(L_ALL,"(%5d) nsreq: nsfunc returns NULL\n",Logstamp);
		if (req->rq_head->h_flags & QUERY) {
			if (sndback(pd,R_NSFAIL) == RFS_FAILURE)
				nsrclose(pd);
		}
		free(block);
		freerp(req);
		return;
	}
	hp = nreq->rq_head;
	hp->h_flags &= ~QUERY;
	if (mode == REMOTE || (hp->h_rcode != R_NOERR && hp->h_rcode != R_NONAME) ||
	    req->rq_head->h_opcode == NS_REL ||
	    (Primary && domauth(dompart(req->rq_qd[0]->q_name)))) {

		LOG3(L_TRANS | L_OVER,"(%5d) nsreq: request returns %s\n",
			Logstamp,prtype(hp->h_rcode));
		free(block);
		block = NULL;
		if (req->rq_head->h_flags & QUERY) {
			block = reqtob(nreq,block,&size);
			if (nswrite(pd,block,size) == -1) {
				LOG2(L_ALL,"(%5d) nsreq: nswrite failed\n",
					Logstamp);
				nsrclose(pd);
			}
			if (block) free(block);
		}
		freerp(req);
		freerp(nreq);
		return;
	}
	/* we now have good response, check for any remote operations	*/

	switch ((int) req->rq_head->h_opcode) {
	/* anything going to the primary name server ONLY, goes here	*/
	case NS_ADV:
	case NS_UNADV:
	case NS_MODADV:
	case NS_INIT:
	case NS_VERIFY:
	case NS_SENDPASS:
	case NS_FINDP:
		if (!Primary) {
			/* remsend takes care of reply	*/
			remsend(pd,block,size,nreq,req);
			if (req->rq_head->h_opcode == NS_INIT) {
				waitcld(); /* NS_INIT child does not signal */
				if (!Primary && alarm(0) != 0)
					alarm(10);  /* make first poll immediate */
			}
			cwait=TRUE; /* wait for child */
			break;
		}
		LOG3(L_TRANS | L_OVER,"(%5d) nsreq: ADV/UNADV returns rcode = %s\n",
			Logstamp,prtype(hp->h_rcode));
		if (block) {
			free(block);
			block = NULL;
		}
		block = reqtob(nreq,block,&size);
		if (nswrite(pd,block,size) == -1) {
			LOG2(L_ALL,"(%5d) nsreq: nswrite failed\n",Logstamp);
			nsrclose(pd);
		}
		break;	
	case NS_BYMACHINE:
	case NS_QUERY:
		/*
		 * check to see if response was answer or indirect.
		 * The presence of an answer section means that there
		 * is a response to return.  But, if we are not primary,
		 * go to primary for authoritative answer.  If we are
		 * primary, check out any indirection.
		 */
		if (!Primary) {
			remsend(pd,block,size,nreq,NULL);
			break;
		}
		if (hp->h_ancnt != 0) {
			LOG4(L_TRANS | L_OVER,
				"(%5d) nsreq: returns %d records rcode = %s\n",
				Logstamp,hp->h_ancnt,prtype(hp->h_rcode));
			if (block) {
				free(block);
				block = NULL;
			}
			block = reqtob(nreq,block,&size);
			if (nswrite(pd,block,size) == -1) {
				LOG2(L_ALL,"(%5d) nsreq: nswrite failed\n",
					Logstamp);
				nsrclose(pd);
			}
		}
		else if (hp->h_nscnt == 0 || hp->h_arcnt == 0) {
			LOG2(L_ALL,"(%5d) nsreq: can't make indirect ref.\n",
				Logstamp);
			if (sndback(pd,R_RCV) == RFS_FAILURE)
				nsrclose(pd);
		}
		else {	/* we have some indirect references to try	*/
			remsend(pd,block,size,nreq,NULL);
		}
		break;
	default:	/* unknown type	*/
		LOG3(L_ALL,"(%5d) nsreq: unknown req type = %d\n",
			Logstamp,req->rq_head->h_opcode);
		if (sndback(pd,R_FORMAT) == RFS_FAILURE)
			nsrclose(pd);
		break;
	}
	/* now just clean up and continue	*/
	if (!cwait)
		freerp(req);
	if (block)
		free(block);
	freerp(nreq);
	return;
}
/*
 *
 *	remsend assigns a new port, and forks.
 *	The parent continues as a normal name server.
 *	The child completes the remote part of the transaction.
 *
 *	When oreq is set, the request will be stored for backout if the
 *	operation fails.  Oreq should only be set for those functions
 *	that go to the primary domain name server, because remsend relies
 *	on this to return the correct failure message when the primary
 *	cannot be reached.
 *
 */
static int
remsend(pd,block,size,req,oreq)
int	pd;		/* local port descriptor			*/
char	*block;
int	size;
struct request	*req;	/* request, if non-NULL, contains addresses	*/
struct request	*oreq;	/* request, if non-NULL, contains undo info	*/
{
	int	npd;
	int	chg = 0;	/* return from mkalist, no. of new addresses */
	int	i;
	char	*nblock;
	int	nsize;
	int	child;
	int	ret=R_NOERR;
	struct address	*addrlist[MAX_RETRY];
	struct request  *nreq;
	struct header	*hp=NULL;
	static int	sane_cld();	/* sanity check for child process */

	LOG2(L_OVER,"(%5d) remsend: send request to remote ns\n",Logstamp);

	/* now fork to handle the remote part	*/
	switch(child = fork()) {
	case (-1):	/* error	*/
		LOG2(L_ALL,"(%5d) remsend: fork() failed\n",Logstamp);
		if (sndback(pd,R_SYS) == RFS_FAILURE)
			nsrclose(pd);
		return;
	default:	/* parent	*/
		/* if request was passed in, save for possible backout */
		if (oreq)
			savecld(child,oreq);
		nsrclose(pd);
		return;
	case 0:		/* child	*/
#ifdef LOGGING
		Logstamp += 1000;	/* differentiate child in output */
#endif
		alarm(CHILDTIME);
		sigset(SIGALRM,sane_cld);
		sigset(SIGTERM,SIG_IGN);
		break;
	}
	/*
	 * only get here if child, now need to handle remote stuff
	 * and exit.
	 */

	addrlist[0] = NULL;	/* list ended by null (unless full) */

	chg = mkalist(addrlist,req,NULL);
	if (!Primary)
		mkalist(addrlist,NULL,Paddress);

	for (i=0; i < MAX_RETRY && addrlist[i] != NULL; i++) {

		if (eqaddr(Myaddress,addrlist[i]))
			continue;

		LOG3(L_ALL,"(%5d) remsend: try to connect to %s\n",
			Logstamp,aatos(Logbuf,addrlist[i],KEEP | HEX));

		if ((npd = rconnect(addrlist[i],REMOTE)) == -1) {
			LOG3(L_ALL,"(%5d) remsend: can't connect to %s\n",
				Logstamp,aatos(Logbuf,addrlist[i],KEEP | HEX));
			continue;
		}
		if (nswrite(npd,block,size) == -1) {
			LOG2(L_ALL,"(%5d) remsend: nswrite failed\n",Logstamp);
			nsrclose(npd);
			continue;
		}
		nblock = NULL;
		if ((nsize=nsread(npd,&nblock,0)) == -1) {
			LOG2(L_ALL,"(%5d) remsend: nsread failed\n",Logstamp);
			nsrclose(npd);
			continue;
		}
		if ((nreq = btoreq(nblock,nsize)) == NULL) {
			LOG4(L_ALL,"(%5d) remsend: btoreq(%x,%d) failed\n",
				Logstamp,nblock,nsize);
			free(nblock);
			nsrclose(npd);
			continue;
		}
		hp = nreq->rq_head;

		/*
		 * if response was authoritative, break and return it.
		 */
		if (hp->h_flags & AUTHORITY) {
			ret = hp->h_rcode;
			nsrclose(npd);
			break;
		}

		/*
		 * Otherwise check for indirection.
		 */
		if (hp->h_rcode == R_NOERR && hp->h_nscnt != 0 && hp->h_arcnt != 0)
			chg += mkalist(addrlist,nreq,NULL);

		freerp(nreq);
		free(nblock);
		hp = NULL;
		nsrclose(npd);
	}
	/*
	 * Done. if hp is NULL, no response was found, otherwise
	 *       return nblock to sender.
	 */
	if (hp == NULL) {
		LOG2(L_ALL,"(%5d) remsend: return R_NSFAIL\n",Logstamp);
		/* if chg was set, we got an indirection, but no answer */
		/* if oreq is set, we are going to the primary only, so */
		/* no indirection is expected.				*/
		if (sndback(pd,(chg || oreq)?R_SEND:R_RCV) == RFS_FAILURE)
			nsrclose(pd);

		ret = R_NSFAIL;	/* this sets up backout if necessary	*/
	}
	else {
		if (nswrite(pd,nblock,nsize) == -1) {
			LOG2(L_ALL,"(%5d) remsend: nswrite of resp failed\n",
			   Logstamp);
			nsrclose(pd);
		}
		if (oreq && oreq->rq_head && 
			oreq->rq_head->h_opcode == NS_INIT) {
			exit(setmaster(nreq)); /* NS_INIT does not signal parent */
		}
	}

	kill(getppid(),SIGUSR2);	/* notify parent	*/
	exit(ret);
}
/*
 * This function is a sanity timer for the child process.
 * if the child takes longer than CHILDTIME to complete, it will be
 * interrupted, and the operation will fail.  CHILDTIME should
 * be set to several minutes to guarantee that it only goes
 * off when things are clearly hung (i.e., > 200).  In particular,
 * it should be at least double R_TIMEOUT, which is the recovery
 * timeout.
 */
static int
sane_cld()
{
	LOG3(L_OVER,"(%5d) sane_cld: Child %d timed out\n",Logstamp,getpid());
	kill(getppid(),SIGUSR2);
	exit(R_NSFAIL);
}
int
sndback(pd,rcode)
int	pd;
int	rcode;
{
	struct	request	rq;
	struct	header	hd;
	char	*block=NULL;
	int	size;

	LOG4(L_TRANS,"(%5d) sndback: pd=%d, rcode=%d\n",Logstamp,pd,rcode);
	rq.rq_head = &hd;
	hd.h_version = NSVERSION;
	hd.h_flags = RESPONSE | NOT_AUTHORITY;
	hd.h_opcode = NS_SNDBACK;
	hd.h_rcode = rcode;
	hd.h_dname = Dname;
	hd.h_qdcnt = hd.h_ancnt = hd.h_nscnt = hd.h_arcnt = 0;
	if ((block = reqtob(&rq,block,&size)) == NULL) {
		LOG1(L_TRANS,"(%5d) sndback: block is NULL\n");
		return(RFS_FAILURE);
	}
	if (nswrite(pd,block,size) == -1) {
		LOG2(L_TRANS,"(%5d) sndback: nswrite failed\n",Logstamp);
		free(block);
		return(RFS_FAILURE);
	}

	free(block);
	return(RFS_SUCCESS);
}
/*
 * This function makes a temporary netmaster file
 * from an address, and reads it in using readfile.
 */
static int
putprime(addr)
struct address	*addr;
{
	char	*tmpnam();
	char	*tmpfile = tmpnam(NULL);
	char	*p_name = PRIMENAME;
	char	addbuf[BUFSIZ];
	FILE	*fopen();
	FILE	*fd;
	int	ret;

	LOG3(L_TRANS,"(%5d) putprime: addr=%x\n",Logstamp,addr);

	if (!addr || aatos(addbuf,addr,HEX) == NULL) {
		LOG2(L_TRANS,"(%5d) putprime: Can't translate address\n",Logstamp);
		return(RFS_FAILURE);
	}

	LOG3(L_TRANS | L_OVER,"(%5d) putprime: set primary address to %s\n",
		Logstamp,addbuf);

	if (eqaddr(addr,getmyaddr()))
		p_name = Dname;

	if ((fd = fopen(tmpfile,"w")) == NULL) {
		perror("RFS name server: putprime temporary file");
		return(RFS_FAILURE);
	}

	fprintf(fd,"%s p %s\n%s a %s\n",
		dompart(Dname),p_name,p_name,addbuf);

	fclose(fd);

	ret = readdb(tmpfile);
	unlink(tmpfile);

	if (ret <= 0)
		PLOG1("RFS name server: RFS_FAILURE reading primary address\n");

	return((ret <= 0)?RFS_FAILURE:RFS_SUCCESS);
}
int
freerp(req)
struct request	*req;
{
	long	i;
	struct header	*hp;

	hp = req->rq_head;
	/* remove resource records	*/
	for (i=0; i < hp->h_ancnt; i++)
		freerec(req->rq_an[i]);
	for (i=0; i < hp->h_nscnt; i++)
		freerec(req->rq_ns[i]);
	for (i=0; i < hp->h_arcnt; i++)
		freerec(req->rq_ar[i]);

	if (hp->h_ancnt) free(req->rq_an);
	if (hp->h_nscnt) free(req->rq_ns);
	if (hp->h_arcnt) free(req->rq_ar);

	/* now do the query section	*/
	for (i=0; i < hp->h_qdcnt; i++) {
		free(req->rq_qd[i]->q_name);
		free(req->rq_qd[i]);
	}
	if (hp->h_qdcnt) free(req->rq_qd);

	if (hp->h_dname)
		free(hp->h_dname);

	free(hp);
	free(req);
	return;
}

static int
sigterm()
{
	LOG2(L_ALL,"(%5d): sigterm called\n",Logstamp);
	clrnslock();
	Done = TRUE;
}
/* wait for child	*/
static int
waitcld()
{
	register int	i;
	int	stat_loc;
	int	child;
	struct request	*req;
	int	alrm_tim;
	static int	alrm_fn();

	sigset(SIGUSR2, SIG_IGN);
	alrm_tim = alarm(0);
	sigset(SIGALRM,alrm_fn);
	alarm(CHILDWAIT);

	LOG2(L_COMM,"(%5d) waitcld: entering\n", Logstamp);
	while ((child = wait(&stat_loc)) != -1) {
	    alarm(0);
	    LOG4(L_COMM,"(%5d) waitcld: child %d exits with code 0x%x\n",
		Logstamp,child,stat_loc);
	    /* see if child is in table	*/
	    for (i=0; i < MAXCHILDREN; i++)
		if (Children[i].c_pid == child) {
			Children[i].c_pid = 0;
			req = Children[i].c_req;
			Children[i].c_req = NULL;
			if ((stat_loc & 0xFF) == 0 && stat_loc)
				ns_undo(req);
			else {
				if (req->rq_head->h_opcode == NS_INIT) {
					if (re_read() == RFS_FAILURE) 
					    Done = TRUE;
					else if (!Primary && r_init() == RFS_FAILURE) {
					    PLOG1("RFS Name Server: r_init after merge FATAL\n");
					    Done = TRUE;
					}
				}
				freerp(req);
			}
			break;
		}
		sigset(SIGALRM,alrm_fn);
		alarm(CHILDWAIT);
	}

	alarm(0);
	sigset(SIGALRM,Alarmsig);
	if (Alarmsig != SIG_IGN)
		alarm((alrm_tim)?alrm_tim:Polltime);

	sigset(SIGUSR2,waitcld);
	return;
}
static int
alrm_fn()
{
	sigset(SIGALRM,alrm_fn);
	LOG2(L_OVER,"(%5d) waitcld: wait sanity alarm\n",Logstamp);
}

static int
re_read()
{
	int	errflg;

	if ((errflg = readfile(Netmaster,TRUE,FALSE)) <= 0) {
		if (errflg == 0)
			PLOG2("RFS name server: %s is empty after INIT\n",Netmaster);
		else
			PLOG3("RFS name server: can't open %s, error %s after INIT\n",
			    Netmaster,
			    (errno > sys_nerr)?"unknown error":sys_errlist[errno]);

		return(RFS_FAILURE);
	}
	merge(DOMMASTER,FALSE);
	return(RFS_SUCCESS);
}
static int
savecld(child,req)
int	child;
struct request	*req;
{
	int	i;
	for (i=0; i < MAXCHILDREN; i++)
		if (Children[i].c_pid == NULL) {
			Children[i].c_pid = child;
			Children[i].c_req = req;
			return(RFS_SUCCESS);
		}
	/* ran out of room, warn but continue	*/
	PLOG1("RFS name server: WARNING exceeded maximum outstanding children\n");
	freerp(req);
	return(RFS_FAILURE);
}
static int
mkalist(addrlist,req,addr)
struct address	**addrlist;
struct request	*req;
struct address	**addr;
{
	int	i,j;
	int	istart;		/* initial length of addrlist	*/
	struct header	*hp;
	struct address	*ap;

	LOG5(L_TRANS,"(%5d) mkalist(addrlist=%x,req=%x,addr[0]=%s\n",
		Logstamp,addrlist,req,(addr && addr[0])?
			aatos(Logbuf,addr[0],KEEP | HEX):"NULL");

	/* find end of addrlist	*/
	for (i=0; addrlist[i] != NULL && i < MAX_RETRY; i++)
		;

	if (i == MAX_RETRY)
		return(0);

	istart = i;

	if (addr)
	    for (j=0; j < MAXNS; j++)
		if (addr[j] && notinlist(addrlist,addr[j])) {
			addrlist[i++] = addr[j];
			if (i < MAX_RETRY) {
				LOG3(L_TRANS,"(%5d) mkalist: add Paddress %s\n",
				    Logstamp,aatos(Logbuf,addr[j],KEEP|HEX));
				addrlist[i] = NULL;
			}
		}

	if (req) {
		hp = req->rq_head;
		for (j=0; j < hp->h_arcnt && i < MAX_RETRY; j++) {
			if (ap = astoa(req->rq_ar[j]->rr_a,NULL)) {
			    if (notinlist(addrlist,ap)) {
				addrlist[i++] = ap;
				if (i < MAX_RETRY) {
					LOG3(L_TRANS,
					   "(%5d) mkalist: add address %s\n",
				           Logstamp,aatos(Logbuf,ap,KEEP|HEX));
					addrlist[i] = NULL;
				}
			    }
			    else
				free(ap);
			}
		}
	}
	return(i - istart);
}
static int
notinlist(addrlist,ap)
register struct	address	**addrlist;
register struct	address	*ap;
{
	register int	i;

	for (i=0; i < MAX_RETRY && *addrlist != NULL; i++, addrlist++)
		if (eqaddr(*addrlist,ap))
			return(FALSE);

	return(TRUE);
}
static int
eqaddr(a,b)
register struct address *a;
register struct address *b;
{
	if (!a || !b)
		return(FALSE);

	if (a->addbuf.len != b->addbuf.len)
		return(FALSE);
	if (a->protocol && b->protocol && strcmp(a->protocol,b->protocol) != NULL)
		return(FALSE);

	if (memcmp(a->addbuf.buf,b->addbuf.buf,a->addbuf.len))
		return(FALSE);

	return(TRUE);
}
static int
ns_undo(req)
struct request	*req;
{
	struct header	*hp;
	struct request	*nreq;

	LOG2(L_COMM,"ns_undo: req=0X%x\n",req);

	hp = req->rq_head;

	/* for now, only undo advertisements	*/
	if (hp->h_opcode == NS_ADV) {
	    hp->h_opcode = NS_UNADV;
	    if ((nreq = nsfunc(req,-1)) == NULL)
		LOG1(L_ALL,"ns_undo: can't backout request\n");
	    else
		freerp(nreq);
	}
	freerp(req);
	return;
}
/*
 * setnslock sets a lock file to make sure that two name servers
 * are not allowed to run at once.  It also writes this name server's
 * pid into the lock file so that rfstop can use it.
 */
static int	Lockfd = -1;	/* current lock fd	*/
static int
setnslock()
{
	char	buf[BUFSIZ];

	if ((Lockfd = open(NSPID, O_RDWR|O_CREAT, 0600)) == -1) {
		perror(NSPID);
		return(-1);
	}
	if (lockf(Lockfd,F_TLOCK,0L) == -1) {
		perror(NSPID);
		return(-1);
	}
	sprintf(buf,"%-7d",getpid());
	write(Lockfd,buf,strlen(buf)+1);
	return(0);
}
static int
clrnslock()
{
	if (Lockfd == -1)
		return(-1);

	close(Lockfd);
	unlink(NSPID);
	Lockfd = -1;
	return(0);
}

/* 4.1 BSD signalling functions used by the name server emulated
 * with 4.2 BSD calls. 
 */

int
sighold(sig)
	int sig;
{

	if (sig == SIGKILL) {
		errno = EINVAL;
		return (-1);	/* sigblock quietly disallows SIGKILL */
	}
	(void) sigblock(sigmask(sig));
	return (0);		/* SVID specifies 0 return on success */
}

int
sigrelse(sig)
	int sig;
{

	if (sig == SIGKILL) {
		errno = EINVAL;
		return (-1);	/* sigsetmask quietly disallows SIGKILL */
	}
	(void) sigsetmask(sigblock(0) & ~sigmask(sig));
	return (0);		/* SVID specifies 0 return on success */
}

int
sigignore(sig)
	int sig;
{
	struct sigvec vec;

	if (sig == SIGKILL) {
		errno = EINVAL;
		return (-1);	/* sigsetmask quietly disallows SIGKILL */
	}
	if (sigvec(sig, (struct sigvec *)0, &vec) < 0)
		return (-1);
	vec.sv_handler = SIG_IGN;
	if (sigvec(sig, &vec, (struct sigvec *)0) < 0)
		return (-1);
	(void) sigsetmask(sigblock(0) & ~sigmask(sig));
	return (0);		/* SVID specifies 0 return on success */
}

void (*
sigset(sig, func))()
	int sig;
	void (*func)();
{
	struct sigvec newvec;
	int newmask;
	struct sigvec oldvec;
	int oldmask;

	if (sigvec(sig, (struct sigvec *)0, &oldvec) < 0)
		return (SIG_ERR);
	oldmask = sigblock(0);
	newvec = oldvec;
	newvec.sv_flags |= SV_INTERRUPT;
	newvec.sv_flags &= ~SV_RESETHAND;
	newvec.sv_mask = 0;
	newmask = oldmask;
	if (func == SIG_HOLD) {
		/*
		 * Signal will be held.  Set the bit for that
		 * signal in the signal mask.  Leave the action
		 * alone.
		 */
		newmask |= sigmask(sig);
	} else {
		/*
		 * Signal will not be held.  Clear the bit
		 * for it in the signal mask.  Set the action
		 * for it.
		 */
		newmask &= ~sigmask(sig);
		newvec.sv_handler = func;
	}
	if (sigvec(sig, &newvec, (struct sigvec *)0) < 0)
		return (SIG_ERR);
	if (sigsetmask(newmask) < 0)
		return (SIG_ERR);
	if (oldmask & sigmask(sig))
		return (SIG_HOLD);      /* signal was held */
	else
		return (oldvec.sv_handler);
}
