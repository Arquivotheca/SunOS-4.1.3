/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)callers.c 1.1 92/07/30"	/* from SVR3.2 uucp:callers.c 2.23 */

#include "uucp.h"

#ifdef BSD4_2
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif
#ifdef UNET
#include  "UNET/unetio.h"
#include  "UNET/tcp.h"
#endif


int alarmtr();
extern jmp_buf Sjbuf;
extern char *fdig();
extern int	Modemctrl;

/*
 *	to add a new caller:
 *	declare the function that knows how to call on the device,
 *	add a line to the callers table giving the name of the device
 *	(from Devices file) and the name of the function
 *	add the function to the end of this file
 */

#ifdef DIAL801
int	dial801();
#endif

#ifdef DATAKIT
int	dkcall();
#endif /* DATAKIT */

#ifdef V8
int	Dialout();
#endif

#ifdef TCP
int	unetcall();
int	tcpcall();
#endif /* TCP */

#ifdef SYTEK
int	sytcall();
#endif /* SYTEK */

#ifdef TLI
int	tlicall();
#endif /* TLI */

struct caller Caller[] = {

#ifdef DIAL801
	{"801",		dial801},
	{"212",		dial801},
#endif /* DIAL801 */

#ifdef V8
	{"Dialout",	Dialout},	/* ditto but using dialout(III) */
#endif

#ifdef TCP
#ifdef BSD4_2
	{"TCP",		tcpcall},	/* 4.2BSD sockets */
#else /* !BSD4_2 */
#ifdef UNET
	{"TCP",		unetcall},	/* 3com implementation of tcp */
	{"Unetserver",	unetcall},
#endif /* UNET */
#endif /* BSD4_2 */
#endif /* TCP */

#ifdef DATAKIT
	{"DK",		dkcall},	/* standard AT&T DATAKIT VCS caller */
#endif /* DATAKIT */

#ifdef SYTEK
	{"Sytek",	sytcall},	/* untested but should work */
#endif /* SYTEK */

#ifdef TLI
	{"TLI",		tlicall},	/* AT&T Transport Layer Interface */
#ifdef TLIS
	{"TLIS",	tlicall},	/* AT&T Transport Layer Interface */
#endif /*  TLIS  */
#endif /* TLI */

	{NULL, 		NULL}		/* this line must be last */
};

/***
 *	exphone - expand phone number for given prefix and number
 *
 *	return code - none
 */

static void
exphone(in, out)
char *in, *out;
{
	FILE *fn;
	char pre[MAXPH], npart[MAXPH], tpre[MAXPH], p[MAXPH];
	char buf[BUFSIZ];
	char *s1;

	if (!isalpha(*in)) {
		(void) strcpy(out, in);
		return;
	}

	s1=pre;
	while (isalpha(*in))
		*s1++ = *in++;
	*s1 = NULLCHAR;
	s1 = npart;
	while (*in != NULLCHAR)
		*s1++ = *in++;
	*s1 = NULLCHAR;

	tpre[0] = NULLCHAR;
	fn = fopen(DIALCODES, "r");
	if (fn != NULL) {
		while (fgets(buf, BUFSIZ, fn)) {
			if ( sscanf(buf, "%s%s", p, tpre) < 1)
				continue;
			if (EQUALS(p, pre))
				break;
			tpre[0] = NULLCHAR;
		}
		fclose(fn);
	}

	(void) strcpy(out, tpre);
	(void) strcat(out, npart);
	return;
}

/*
 * repphone - Replace \D and \T sequences in arg with phone
 * expanding and translating as appropriate.
 */
static char *
repphone(arg, phone, trstr)
register char *arg, *phone, *trstr;
{
	extern void translate();
	static char pbuf[2*(MAXPH+2)];
	register char *fp, *tp;

	for (tp=pbuf; *arg; arg++) {
		if (*arg != '\\') {
			*tp++ = *arg;
			continue;
		} else {
			switch (*(arg+1)) {
			case 'T':
				exphone(phone, tp);
				translate(trstr, tp);
				for(; *tp; tp++)
				    ;
				arg++;
				break;
			case 'D':
				for(fp=phone; *tp = *fp++; tp++)
				    ;
				arg++;
				break;
			default:
				*tp++ = *arg;
				break;
			}
		}
	}
	*tp = '\0';
	return(pbuf);
}

/*
 * processdev - Process a line from the Devices file
 *
 * return codes:
 *	file descriptor  -  succeeded
 *	FAIL  -  failed
 */

processdev(flds, dev)
register char *flds[], *dev[];
{
	int dcf = -1;
	register struct caller	*ca;
	char *args[D_MAX+1], dcname[20];
	register char **sdev;
	extern int (*Setup)();
	extern void translate(), setdevcfg();
	register nullfd;
	char *phonecl;			/* clear phone string */
	char phoneex[2*(MAXPH+2)];	/* expanded phone string */
	extern ttygenbrk();

	sdev = dev;
/*	set up default "break" routine */
	genbrk = ttygenbrk;

	for (ca = Caller; ca->CA_type != NULL; ca++) {
		/* This will find built-in caller functions */
		if (EQUALS(ca->CA_type, dev[D_CALLER])) {
			DEBUG(5, "Internal caller type %s\n", dev[D_CALLER]);
			if (dev[D_ARG] == NULL) {
				/* if NULL - assume translate */
				dev[D_ARG+1] = NULL;	/* needed for for loop later to mark the end */
				dev[D_ARG] = "\\T";
			}
			dev[D_ARG] = repphone(dev[D_ARG], flds[F_PHONE], "");
			if ((dcf = (*(ca->CA_caller))(flds, dev)) < 0)
				return(dcf) ;
			if ( interface( ca->CA_type ) ) {
				DEBUG(5, "interface(%s) failed", ca->CA_type);
				Uerror = SS_DEVICE_FAILED;
				/*	restore vanilla unix interface	*/
				(void)interface("UNIX");
				return(FAIL);
			}
			dev += 2; /* Skip to next CALLER and ARG */
			break;
		}
	}
	if (dcf == -1) {
		/* Here if not a built-in caller function	*/

		/* If "DIAL" is defined, the mlock will probably fail	*/
		/* (/usr/spool/locks is usually 755 owned by uucp), but	*/
		/* since are using advisory file locks anyway let it	*/
		/* slide.						*/
		if (mlock(dev[D_LINE]) == FAIL) { /* Lock the line */
#ifdef DIAL
			;
		}
#else /* !DIAL */
			DEBUG(5, "mlock %s failed\n", dev[D_LINE]);
			Uerror = SS_LOCKED_DEVICE;
			return(FAIL);
		}
		DEBUG(5, "mlock %s succeeded\n", dev[D_LINE]);
#endif /* DIAL */
		/*
		 * Open the line
		 */
		if ( *dev[D_LINE] != '/' ) {
			(void) sprintf(dcname, "/dev/%s", dev[D_LINE]);
		} else {
			(void) strcpy(dcname, dev[D_LINE] );
		}
		/* take care of the possible partial open fd */
		(void) close(nullfd = open("/", 0));
		if (setjmp(Sjbuf)) {
			(void) close(nullfd);
			DEBUG(1, "generic open timeout\n", 0);
			logent("generic open", "TIMEOUT");
			Uerror = SS_CANT_ACCESS_DEVICE;
			goto bad;
		}
		(void) signal(SIGALRM, alarmtr);
		(void) alarm(10);
		if ( Modemctrl ) {
			DEBUG(7, "opening with O_NDELAY set\n", 0);
			dcf = open(dcname, (O_RDWR | O_NDELAY) );
		} else
			dcf = open(dcname, O_RDWR );
		(void) alarm(0);
		if (dcf < 0) {
			DEBUG(1, "generic open failed, errno = %d\n", errno);
			logent("generic open", "FAILED");
			Uerror = SS_CANT_ACCESS_DEVICE;
			(void) close(nullfd);
			goto bad;
		}
		if ( Modemctrl ) {
			DEBUG(7, "clear O_NDELAY\n", 0);
			if ( fcntl(dcf, F_SETFL,
				(fcntl(dcf, F_GETFL, 0) & ~O_NDELAY)) < 0 ) {
				DEBUG( 7, "clear O_NDELAY failed, errno %d\n", errno);
				(void)close(dcf);
				Uerror = SS_DEVICE_FAILED;
				goto bad;
			}
		}
#ifdef ATTSV
		if ( filelock(dcf) != SUCCESS ) {
			(void)close(dcf);
			DEBUG(1, "failed to lock device %s\n", dcname);
			Uerror = SS_LOCKED_DEVICE;
			goto bad;
		}
#endif /* ATTSV */
		fixline(dcf, atoi(fdig(dev[D_CLASS])), D_DIRECT);
	}

	/*	init device config info	*/
	DEBUG(5, "processdev: calling setdevcfg(%s, ", Progname);
	DEBUG(5, "%s)\n", flds[F_TYPE]);
	setdevcfg(Progname, flds[F_TYPE]);

	if ( (*Setup)( MASTER, &dcf, &dcf ) ) {
		/*	any device|system lock files we should remove?	*/
		DEBUG(5, "MASTER Setup failed", 0);
		Uerror = SS_DEVICE_FAILED;
		goto bad;
	}
	/*
	 * Now loop through the remaining callers and chat
	 * according to scripts in dialers file.
	 */
	for (; dev[D_CALLER] != NULL; dev += 2) {
		register int w;
		/*
		 * Scan Dialers file to find an entry
		 */
		if ((w = gdial(dev[D_CALLER], args, D_MAX)) < 1) {
			DEBUG(1, "%s not found in Dialers file\n", dev[D_CALLER]);
			logent("generic call to gdial", "FAILED");
			Uerror = SS_CANT_ACCESS_DEVICE;
			goto bad;
		}
		if (w <= 2)	/* do nothing - no chat */
			break;
		/*
		 * Translate the phone number
		 */
		if (dev[D_ARG] == NULL) {
			/* if NULL - assume no translation */
			dev[D_ARG+1] = NULL; /* needed for for loop to mark the end */
			dev[D_ARG] = "\\D";
		}
		
		phonecl = repphone(dev[D_ARG], flds[F_PHONE], args[1]);
		exphone(phonecl, phoneex);
		translate(args[1], phoneex);
		/*
		 * Chat
		 */
		if (chat(w-2, &args[2], dcf, phonecl, phoneex) != SUCCESS) {
			Uerror = SS_CHAT_FAILED;
			goto bad;
		}
	}
	/*
	 * Success at last!
	 */
	strcpy(Dc, sdev[D_LINE]);
	return(dcf);
bad:
	if ( dcf >= 0 )
		(void)close(dcf);
	delock(sdev[D_LINE]);
	/*	restore vanilla unix interface	*/
	(void)interface("UNIX");
	return(FAIL);
}

/*
 * translate the pairs of characters present in the first
 * string whenever the first of the pair appears in the second
 * string.
 */
static void
translate(ttab, str)
register char *ttab, *str;
{
	register char *s;

	for(;*ttab && *(ttab+1); ttab += 2)
		for(s=str;*s;s++)
			if(*ttab == *s)
				*s = *(ttab+1);
}

#define MAXLINE	512
/*
 * Get the information about the dialer.
 * gdial(type, arps, narps)
 *	type	-> type of dialer (e.g., penril)
 *	arps	-> array of pointers returned by gdial
 *	narps	-> number of elements in array returned by gdial
 * Return value:
 *	-1	-> Can't open DIALERFILE
 *	0	-> requested type not found
 *	>0	-> success - number of fields filled in
 */
static
gdial(type, arps, narps)
register char *type, *arps[];
register int narps;
{
	static char info[MAXLINE];
	register na;
	extern void dialreset();

	DEBUG(2, "gdial(%s) called\n", type);
	while (getdialline(info, sizeof(info))) {
		if ((info[0] == '#') || (info[0] == ' ') ||
		    (info[0] == '\t') || (info[0] == '\n'))
			continue;
		if ((na = getargs(info, arps, narps)) == 0)
			continue;
		if (EQUALS(arps[0], type)) {
			dialreset();
			bsfix(arps);
			return(na);
		}
	}
	dialreset();
	return(0);
}


#ifdef DATAKIT

/***
 *	dkcall(flds, dev)	make a DATAKIT VCS connection
 *				  DATAKIT VCS is a trademark of AT&T
 *
 *	return codes:
 *		>0 - file number - ok
 *		FAIL - failed
 */

#include "dk.h"

dkcall(flds, dev)
char *flds[], *dev[];
{
	register fd;
#ifdef V8
	extern int cdkp_ld;
#endif

	char	dialstring[64];
	extern  dkbreak();

	strcpy(dialstring, dev[D_ARG]);

#ifndef STANDALONE
	if(*flds[F_CLASS] < '0' || *flds[F_CLASS] > '9')
		sprintf(dialstring, "%s.%s", dev[D_ARG], flds[F_CLASS]);
#endif

	DEBUG(4, "dkcall(%s)\n", dialstring);


#ifdef V8
	if (setjmp(Sjbuf)) {
		Uerror = SS_DIAL_FAILED;
		return(FAIL);
	}

	(void) signal(SIGALRM, alarmtr);
	(void) alarm(15);
	DEBUG(4, "tdkdial(%s", flds[F_PHONE]);
	DEBUG(4, ", %d)\n", atoi(dev[D_CLASS]));
    	if ((fd = tdkdial(flds[F_PHONE], atoi(dev[D_CLASS]))) >= 0)
	    if (dkproto(fd, cdkp_ld) < 0)
	       {
	    	close(fd);
	    	fd = -1;
	       }
	(void) alarm(0);
#else
	fd = dkdial(dialstring);
#endif

	(void) strcpy(Dc, "DK");
	if (fd < 0) {
		Uerror = SS_DIAL_FAILED;
		return(FAIL);
	}
	else {
		genbrk = dkbreak;
		return(fd);
	}
}

#endif /* DATAKIT */

#ifdef TCP

/***
 *	tcpcall(flds, dev)	make ethernet/socket connection
 *
 *	return codes:
 *		>0 - file number - ok
 *		FAIL - failed
 */

#ifndef BSD4_2
/*ARGSUSED*/
tcpcall(flds, dev)
char	*flds[], *dev[];
{
	Uerror = SS_NO_DEVICE;
	return(FAIL);
}
#else /* BSD4_2 */
tcpcall(flds, dev)
char *flds[], *dev[];
{
	int ret;
	short port;
	extern int	errno, sys_nerr;
	extern char *sys_errlist[];
	struct servent *sp;
	struct hostent *hp;
	struct sockaddr_in sin;

	if (EQUALS(flds[F_CLASS], "-")) {
		/*
		 * Use standard UUCP port number.
		 */
		sp = getservbyname("uucp", "tcp");
		endservent();
		ASSERT(sp != NULL, "No uucp service number", 0, 0);
		port = sp->s_port;
	} else {
		/*
		 * Systems file specifies a port number.
		 */
		sp = getservbyname(flds[F_CLASS], "tcp");
		endservent();
		if (sp == NULL) {
			port = htons(atoi(flds[F_CLASS]));
			if (port == 0) {
				logent("tcpopen", "unknown port number");
				Uerror = SS_NO_DEVICE;
				return(FAIL);
			}
		} else
			port = sp->s_port;
	}
	if (EQUALS(flds[F_PHONE], "-")) {
		/*
		 * Use UUCP name as host name.
		 */
		hp = gethostbyname(flds[F_NAME]);
	} else {
		/*
		 * Systems file specifies a host name different from the UUCP
		 * host name.
		 */
		hp = gethostbyname(flds[F_PHONE]);
	}
	endhostent();
	if (hp == NULL) {
		logent("tcpopen", "no such host");
		Uerror = SS_NO_DEVICE;
		return(FAIL);
	}
	DEBUG(4, "tcpdial host %s, ", hp->h_name);
	DEBUG(4, "port %d\n", ntohs(port));

	ret = socket(AF_INET, SOCK_STREAM, 0);
	if (ret < 0) {
		if (errno < sys_nerr) {
			DEBUG(5, "no socket: %s\n", sys_errlist[errno]);
			logent("no socket", sys_errlist[errno]);
		}
		else {
			DEBUG(5, "no socket, errno %d\n", errno);
			logent("tcpopen", "NO SOCKET");
		}
		Uerror = SS_NO_DEVICE;
		return(FAIL);
	}
	sin.sin_family = hp->h_addrtype;
	bcopy(hp->h_addr, (caddr_t)&sin.sin_addr, hp->h_length);
	sin.sin_port = port;
	if (setjmp(Sjbuf)) {
		DEBUG(4, "timeout tcpopen\n", 0);
		logent("tcpopen", "TIMEOUT");
		Uerror = SS_NO_DEVICE;
		return(FAIL);
	}
	(void) signal(SIGALRM, alarmtr);
	(void) alarm(30);
	DEBUG(7, "family: %d\n", sin.sin_family);
	DEBUG(7, "port: %d\n", sin.sin_port);
	DEBUG(7, "addr: %08x\n",*((int *) &sin.sin_addr));
	if (connect(ret, (caddr_t)&sin, sizeof (sin)) < 0) {
		(void) alarm(0);
		(void) close(ret);
		if (errno < sys_nerr) {
			DEBUG(5, "connect failed: %s\n", sys_errlist[errno]);
			logent("connect failed", sys_errlist[errno]);
		}
		else {
			DEBUG(5, "connect failed, errno %d\n", errno);
			logent("tcpopen", "CONNECT FAILED");
		}
		Uerror = SS_NO_DEVICE;
		return(FAIL);
	}
	(void) signal(SIGPIPE, SIG_IGN);  /* watch out for broken ipc link...*/
	(void) alarm(0);
	(void) strcpy(Dc, "IPC");
	return(ret);
}

#endif /* BSD4_2 */

/***
 *	unetcall(flds, dev)	make ethernet connection
 *
 *	return codes:
 *		>0 - file number - ok
 *		FAIL - failed
 */

#ifndef UNET
unetcall(flds, dev)
char	*flds[], *dev[];
{
	Uerror = SS_NO_DEVICE;
	return(FAIL);
}
#else /* UNET */
unetcall(flds, dev)
char *flds[], *dev[];
{
	int ret;
	int port;
	extern int	errno;

	port = atoi(dev[D_ARG]);
	DEBUG(4, "unetdial host %s, ", flds[F_PHONE]);
	DEBUG(4, "port %d\n", port);
	(void) alarm(30);
	ret = tcpopen(flds[F_PHONE], port, 0, TO_ACTIVE, "rw");
	(void) alarm(0);
	endhnent();
	if (ret < 0) {
		DEBUG(5, "tcpopen failed: errno %d\n", errno);
		Uerror = SS_DIAL_FAILED;
		return(FAIL);
	}
	(void) strcpy(Dc, "UNET");
	return(ret);
}
#endif /* UNET */

#endif /* TCP */

#ifdef SYTEK

/****
 *	sytcall(flds, dev)	make a sytek connection
 *
 *	return codes:
 *		>0 - file number - ok
 *		FAIL - failed
 */

/*ARGSUSED*/
sytcall(flds, dev)
char *flds[], *dev[];
{
	extern int errno;
	int dcr, dcr2, nullfd, ret;
	char dcname[20], command[BUFSIZ];

	(void) sprintf(dcname, "/dev/%s", dev[D_LINE]);
	DEBUG(4, "dc - %s, ", dcname);
	if (mlock(dev[D_LINE]) == FAIL) { /* Lock the line */
		DEBUG(5, "mlock %s failed\n", dev[D_LINE]);
		Uerror = SS_LOCKED_DEVICE;
		return(FAIL);
	}
	dcr = open(dcname, O_WRONLY|O_NDELAY);
	if (dcr < 0) {
		Uerror = SS_DIAL_FAILED;
		DEBUG(4, "OPEN FAILED %s\n", dcname);
		delock(dev[D_LINE]);
		return(FAIL);
	}
	if ( filelock(dcr) != SUCCESS ) {
		(void)close(dcr);
		DEBUG(1, "failed to lock device %s\n", dcname);
		Uerror = SS_LOCKED_DEVICE;
	}

	sytfixline(dcr, atoi(fdig(dev[D_CLASS])), D_DIRECT);
	(void) sleep(2);
	DEBUG(4, "Calling Sytek unit %s\n", dev[D_ARG]);
	(void) sprintf(command,"\r\rcall %s\r", dev[D_ARG]);
	ret = (*Write)(dcr, command, strlen(command));
	(void) sleep(1);
	DEBUG(4, "COM1 return = %d\n", ret);
	sytfix2line(dcr);
	(void) close(nullfd = open("/", 0));
	(void) signal(SIGALRM, alarmtr);
	if (setjmp(Sjbuf)) {
		DEBUG(4, "timeout sytek open\n", 0);
		(void) close(nullfd);
		(void) close(dcr2);
		(void) close(dcr);
		Uerror = SS_DIAL_FAILED;
		delock(dev[D_LINE]);
		return(FAIL);
	}
	(void) alarm(10);
	dcr2 = open(dcname,O_RDWR);
	(void) alarm(0);
	(void) close(dcr);
	if (dcr2 < 0) {
		DEBUG(4, "OPEN 2 FAILED %s\n", dcname);
		Uerror = SS_DIAL_FAILED;
		(void) close(nullfd);	/* kernel might think dc2 is open */
		delock(dev[D_LINE]);
		return(FAIL);
	}
	return(dcr2);
}

#endif /* SYTEK */

#ifdef DIAL801

/***
 *	dial801(flds, dev)	dial remote machine on 801/801
 *	char *flds[], *dev[];
 *
 *	return codes:
 *		file descriptor  -  succeeded
 *		FAIL  -  failed
 *
 *	unfortunately, open801() is different for usg and non-usg
 */

/*ARGSUSED*/
dial801(flds, dev)
char *flds[], *dev[];
{
	char dcname[20], dnname[20], phone[MAXPH+2], *fdig();
	int dcf = -1, speed;

	if (mlock(dev[D_LINE]) == FAIL) {
		DEBUG(5, "mlock %s failed\n", dev[D_LINE]);
		Uerror = SS_LOCKED_DEVICE;
		return(FAIL);
	}
	(void) sprintf(dnname, "/dev/%s", dev[D_CALLDEV]);
	(void) sprintf(phone, "%s%s", dev[D_ARG]   , ACULAST);
	(void) sprintf(dcname, "/dev/%s", dev[D_LINE]);
	CDEBUG(1, "Use Port %s, ", dcname);
	DEBUG(4, "acu - %s, ", dnname);
	CDEBUG(1, "Phone Number  %s\n", phone);
	VERBOSE("Trying modem - %s, ", dcname);	/* for cu */
	VERBOSE("acu - %s, ", dnname);	/* for cu */
	VERBOSE("calling  %s:  ", phone);	/* for cu */
	speed = atoi(fdig(dev[D_CLASS]));
	dcf = open801(dcname, dnname, phone, speed);
	if (dcf >= 0) {
		fixline(dcf, speed, D_ACU);
		(void) strcpy(Dc, dev[D_LINE]);	/* for later unlock() */
		VERBOSE("SUCCEEDED\n", 0);
	} else {
		delock(dev[D_LINE]);
		VERBOSE("FAILED\n", 0);
	}
	return(dcf);
}


#ifndef ATTSV
/*ARGSUSED*/
open801(dcname, dnname, phone, speed)
char *dcname, *dnname, *phone;
{
	int nw, lt, pid = -1, dcf = -1, nullfd, dnf = -1;
	unsigned timelim;

	if ((dnf = open(dnname, 1)) < 0) {
		DEBUG(5, "can't open %s\n", dnname);
		Uerror = SS_CANT_ACCESS_DEVICE;
		return(FAIL);
	}
	DEBUG(5, "%s is open\n", dnname);

	(void) close(nullfd = open("/dev/null", 0));	/* partial open hack */
	if (setjmp(Sjbuf)) {
		DEBUG(4, "timeout modem open\n", 0);
		logent("801 open", "TIMEOUT");
		(void) close(nullfd);
		(void) close(dcf);
		(void) close(dnf);
		if (pid > 0) {
			kill(pid, 9);
			wait((int *) 0);
		}
		Uerror = SS_DIAL_FAILED;
		return(FAIL);
	}
	(void) signal(SIGALRM, alarmtr);
	timelim = 5 * strlen(phone);
	(void) alarm(timelim < 30 ? 30 : timelim);
	if ((pid = fork()) == 0) {
		sleep(2);
		nw = (*Write)(dnf, phone, lt = strlen(phone));
		if (nw != lt) {
			DEBUG(4, "ACU write error %d\n", errno);
			logent("ACU write", "FAILED");
			exit(1);
		}
		DEBUG(4, "ACU write ok\n", 0);
		exit(0);
	}
	/*  open line - will return on carrier */
	dcf = open(dcname, 2);

	DEBUG(4, "dcf is %d\n", dcf);
	if (dcf < 0) {	/* handle like a timeout */
		(void) alarm(0);
		longjmp(Sjbuf, 1);
	}

	/* modem is open */
	while ((nw = wait(&lt)) != pid && nw != -1)
		;
	(void) alarm(0);

	(void) close(dnf);	/* no reason to keep the 801 open */
	if (lt != 0) {
		DEBUG(4, "Fork Stat %o\n", lt);
		(void) close(dcf);
		Uerror = SS_DIAL_FAILED;
		return(FAIL);
	}
	return(dcf);
}

#else /* ATTSV */

open801(dcname, dnname, phone, speed)
char *dcname, *dnname, *phone;
{
	int nw, lt, dcf = -1, nullfd, dnf = -1, ret;
	unsigned timelim;

	(void) close(nullfd = open("/", 0));	/* partial open hack */
	if (setjmp(Sjbuf)) {
		DEBUG(4, "DN write %s\n", "timeout");
		(void) close(dnf);
		(void) close(dcf);
		(void) close(nullfd);
		Uerror = SS_DIAL_FAILED;
		return(FAIL);
	}
	(void) signal(SIGALRM, alarmtr);
	timelim = 5 * strlen(phone);
	(void) alarm(timelim < 30 ? 30 : timelim);

	if ((dnf = open(dnname, O_WRONLY)) < 0 ) {
		DEBUG(5, "can't open %s\n", dnname);
		Uerror = SS_CANT_ACCESS_DEVICE;
		return(FAIL);
	}
	DEBUG(5, "%s is open\n", dnname);
	if ( filelock(dnf) != SUCCESS ) {
		(void)close(dnf);
		DEBUG(1, "failed to lock device %s\n", dnname);
		Uerror = SS_LOCKED_DEVICE;
	}
	if (  (dcf = open(dcname, O_RDWR | O_NDELAY)) < 0 ) {
		DEBUG(5, "can't open %s\n", dcname);
		Uerror = SS_CANT_ACCESS_DEVICE;
		return(FAIL);
	}
	if ( filelock(dcf) != SUCCESS ) {
		(void)close(dcf);
		DEBUG(1, "failed to lock device %s\n", dcname);
		Uerror = SS_LOCKED_DEVICE;
	}

	DEBUG(4, "dcf is %d\n", dcf);
	fixline(dcf, speed, D_ACU);
	nw = (*Write)(dnf, phone, lt = strlen(phone));
	if (nw != lt) {
		(void) alarm(0);
		DEBUG(4, "ACU write error %d\n", errno);
		(void) close(dnf);
		(void) close(dcf);
		Uerror = SS_DIAL_FAILED;
		return(FAIL);
	} else 
		DEBUG(4, "ACU write ok\n", 0);

	(void) close(dnf);
	(void) close(nullfd = open("/", 0));	/* partial open hack */
	ret = open(dcname, 2);  /* wait for carrier  */
	(void) alarm(0);
	(void) close(ret);	/* close 2nd modem open() */
	if (ret < 0) {		/* open() interrupted by alarm */
		DEBUG(4, "Line open %s\n", "failed");
		Uerror = SS_DIAL_FAILED;
		(void) close(nullfd);		/* close partially opened modem */
		return(FAIL);
	}
	(void) fcntl(dcf,F_SETFL, fcntl(dcf, F_GETFL, 0) & ~O_NDELAY);
	return(dcf);
}
#endif /* ATTSV */

#endif /* DIAL801 */

#ifdef V8
Dialout(flds)
char *flds[];
{
    int fd;
    char phone[MAXPH+2];

    exphone(flds[F_PHONE], phone);

    DEBUG(4, "call dialout(%s", phone);
    DEBUG(4, ", %s)\n", dev[D_CLASS]);
    fd = dialout(phone, dev[D_CLASS]);
    if (fd == -1)
	Uerror = SS_NO_DEVICE;
    if (fd == -3)
	Uerror = SS_DIAL_FAILED;
    if (fd == -9)
	Uerror = SS_DEVICE_FAILED;

    (void) strcpy(Dc, "Dialout");

    return(fd);
}
#endif /* V8 */

#ifdef TLI
/*
 *
 * AT&T Transport Layer Interface
 *
 * expected in Devices
 *	TLI line1 - - TLI 
 * or
 *	TLIS line1 - - TLIS
 *
 */

#include <sys/tiuser.h>

#define	CONNECT_ATTEMPTS	3
#define	TFREE(p)	if ( (p) ) t_free( (p) )

/*
 * returns fd to remote uucp daemon
 */
int
tlicall(flds, dev)
char *flds[];
char *dev[];
{
	char		addrbuf[ BUFSIZ ];
	char		devname[MAXNAMESIZE];
	int		fd, service;
	register int	i, j;
	struct t_bind	*bind_ret = 0;
	struct t_info	tinfo;
	struct t_call	*sndcall = 0, *rcvcall = 0;
	extern int	errno, t_errno;
	extern char	*sys_errlist[], *t_errlist[];

	extern struct netbuf	*stoa();

	if ( dev[D_LINE][0] != '/' ) {
		/*	dev holds device name relative to /dev	*/
		sprintf(devname, "/dev/%s", dev[D_LINE]);
	} else {
		/*	dev holds full path name of device	*/
		strcpy(devname, dev[D_LINE]);
	}
	/* gimme local transport endpoint */
	errno = t_errno = 0;
	if (setjmp(Sjbuf)) {
		DEBUG(1, "t_open timeout\n", 0);
		logent("t_open", "TIMEOUT");
		Uerror = SS_NO_DEVICE;
		return(FAIL);
	}
	(void) signal(SIGALRM, alarmtr);
	(void) alarm(5);
	fd = t_open(devname, O_RDWR, &tinfo);
	(void) alarm(0);
	if (fd < 0) {
		tfaillog(fd, "t_open" );
		Uerror = SS_NO_DEVICE;
		return(FAIL);
	}
	if ( filelock(fd) != SUCCESS ) {
		(void)t_close(fd);
		DEBUG(1, "tlicall: failed to lock device %s\n", devname);
		Uerror = SS_LOCKED_DEVICE;
		return(FAIL);
	}

	/* allocate tli structures	*/
	errno = t_errno = 0;
	if ( (bind_ret = (struct t_bind *)t_alloc(fd, T_BIND, T_ALL)) == 
	    (struct t_bind *)NULL
	|| (sndcall = (struct t_call *)t_alloc(fd, T_CALL, T_ALL)) == 
	    (struct t_call *)NULL
	|| (rcvcall = (struct t_call *)t_alloc(fd, T_CALL, T_ALL)) ==
	    (struct t_call *)NULL ) {
		tfaillog(fd, "t_alloc" );
		TFREE(bind_ret);TFREE(sndcall);TFREE(rcvcall);
		Uerror = SS_NO_DEVICE;
		return(FAIL);
	}

	/* bind */
	errno = t_errno = 0;
	if (t_bind(fd, (struct t_bind *) 0, bind_ret ) < 0) {
		tfaillog(fd, "t_bind" );
		TFREE(bind_ret);TFREE(sndcall);TFREE(rcvcall);
		Uerror = SS_NO_DEVICE;
		(void) t_close(fd);
		return(FAIL);
	}
	DEBUG(5, "tlicall: bound to %s\n", bind_ret->addr.buf);

	/*
	 * Prepare to connect.
	 *
	 * If address begins with "\x", "\X", "\o", or "\O",
	 * assume is hexadecimal or octal address and use stoa()
	 * to convert it.
	 *
	 * Else is usual uucico address -- only \N's left to process.
	 * Walk thru connection address, changing \N's to NULLCHARs.
	 * Note:  If a NULLCHAR must be part of the connection address,
	 * it must be overtly included in the address.  One recommended
	 * way is to do it in the Devices file, thusly:
	 *		Netname /dev/netport - - TLI \D\000
	 * bsfix() turns \000 into \N and then the loop below makes it a
	 * real, included-in-the-length null-byte.
	 *
	 * The DEBUG must print the strecpy'd address (so that
	 * non-printables will have been replaced with C escapes).
	 */

	DEBUG(5, "t_connect to addr \"%s\"\n",
		strecpy( addrbuf, dev[D_ARG], "\\" ) );

	if ( dev[D_ARG][0] == '\\' &&
	( dev[D_ARG][1] == 'x' || dev[D_ARG][1] == 'X'
	|| dev[D_ARG][1] == 'o' || dev[D_ARG][1] == 'O' ) ) {
		if ( stoa(dev[D_ARG], &(sndcall->addr)) == (struct netbuf *)NULL ) {
			DEBUG(5, "tlicall: stoa failed\n","");
			logent("tlicall", "string-to-address failed");
			Uerror = SS_NO_DEVICE;
			(void) t_close(fd);
			return(FAIL);
		}
	} else {
		for( i = j = 0; i < BUFSIZ && dev[D_ARG][i] != NULLCHAR;
		++i, ++j ) {
			if( dev[D_ARG][i] == '\\'  &&  dev[D_ARG][i+1] == 'N' ) {
				addrbuf[j] = NULLCHAR;
				++i;
			}
			else {
				addrbuf[j] = dev[D_ARG][i];
			}
		}
		sndcall->addr.buf = addrbuf;
		sndcall->addr.len = j;
	}

	if (setjmp(Sjbuf)) {
		DEBUG(4, "timeout tlicall\n", 0);
		logent("tlicall", "TIMEOUT");
		TFREE(bind_ret);TFREE(sndcall);TFREE(rcvcall);
		Uerror = SS_NO_DEVICE;
		(void) t_close(fd);
		return(FAIL);
	}
	(void) signal(SIGALRM, alarmtr);
	(void) alarm(30);

	/* connect to the service -- some listeners can't handle */
	/* multiple connect requests, so try it a few times */
	errno = t_errno = 0;
	for ( i = 0; i < CONNECT_ATTEMPTS; ++i ) {
		if (t_connect(fd, sndcall, rcvcall) == 0)
			break;
		if ( (t_errno == TLOOK) && (t_look(fd) == T_DISCONNECT)) {
			t_rcvdis(fd,NULL);
			(void) alarm(0);
		} else {
			(void) alarm(0);
			tfaillog(fd, "t_connect");
			TFREE(bind_ret);TFREE(sndcall);TFREE(rcvcall);
			Uerror = SS_DIAL_FAILED;
			(void) t_close(fd);
			return(FAIL);
		}
	}
	(void) alarm(0);
	TFREE(bind_ret);TFREE(sndcall);TFREE(rcvcall);
	if ( i == CONNECT_ATTEMPTS ) {
		tfaillog(fd, "t_connect");
		Uerror = SS_DIAL_FAILED;
		(void) t_close(fd);
		return(FAIL);
	}
	errno = t_errno = 0;
	return(fd);
}


#endif /* TLI */
