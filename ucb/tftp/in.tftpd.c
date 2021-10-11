#ifndef lint
static  char *sccsid = "@(#)in.tftpd.c 1.1 92/07/30 SMI; from UCB 5.6";    /* 86/05/13 */
#endif

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

/*
 * Trivial file transfer protocol server.
 *
 * This version includes many modifications by Jim Guyton <guyton@rand-unix>
 * merged with Sun features for tftp booting, security, and PNP.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <arpa/tftp.h>

#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <setjmp.h>
#include <syslog.h>


#define	TIMEOUT		5
#define	PKTSIZE		SEGSIZE+4
#define	DELAY_SECS	3
#define DALLYSECS 60

extern	int optind, getopt();
extern	char *optarg;
extern	int errno;

struct	sockaddr_in sin = { AF_INET };
int	peer;
int	rexmtval = TIMEOUT;
int	maxtimeout = 5*TIMEOUT;
char	buf[PKTSIZE];
char	ackbuf[PKTSIZE];
struct	sockaddr_in from;
int	fromlen;
int	child;			/* pid of child handling delayed replys */
int	delay_fd [2];		/* pipe for communicating with child */
FILE	*file;
struct	delay_info {
	long	timestamp;		/* time request received */
	int	ecode;			/* error code to return */
	struct	sockaddr_in from;	/* address of client */
};

int	securetftp = 0;
int	debug = 0;
int	disable_pnp = 0;

/*
 * Default directory for unqualified names
 * Used by TFTP boot procedures
 */
char	*homedir = "/tftpboot";

childcleanup ()
{
	int ret;

	wait3 ((union wait *) 0, WNOHANG, (struct rusage *) 0);
}

main (argc, argv)
	int argc;
	char **argv;
{
	register struct tftphdr *tp;
	register int n;
	int on = 1;
	int c;

	openlog("tftpd", LOG_PID, LOG_DAEMON);
	if (ioctl(0, FIONBIO, &on) < 0) {
		syslog(LOG_ERR, "ioctl(FIONBIO): %m\n");
		exit(1);
	}

	while ((c = getopt (argc, argv, "dsp")) != EOF)
	    switch (c) {
		case 'd':		/* enable debug */
		    debug++;
		    continue;
		case 's':		/* secure daemon */
		    securetftp = 1;
		    continue;
		case 'p':		/* disable name pnp mapping */
		    disable_pnp = 1;
		    continue;
		case '?':
		default:
usage:
		    fprintf (stderr, "usage:  %s [-spd] [home-directory]\n",
			    argv[0]);
		    for ( ; optind < argc; optind++)
			syslog (LOG_ERR, "bad argument %s", argv [optind]);
		    exit (1);
	    }

	if (optind < argc)
	    if (optind == argc - 1 && *argv [optind] == '/')
		homedir = argv [optind];
	    else
		goto usage;
	
	if (securetftp) {
		if (chroot(homedir) < 0) {
			syslog(LOG_ERR, 
			       "tftpd: cannot chroot to directory %s: %m\n", 
			       homedir);
			exit(1);
		}
		(void) chdir("/");  /* cd to  new root */
	} else {
		(void) chdir(homedir); /* don't care if this works */
	}

#define UID_NOBODY 65534
#define GID_NOBODY 65534

	if (getuid() == 0) {
		/*
		 * Root access is too risky for tftp because the protocol
		 * has no provision for authentication.  If the system 
		 * administrator specified root in the inetd.conf file,
		 * it was probably a mistake.  In this case, we lower 
		 * ourself down to a uid/gid with only "public" file
		 * access.
		 */
		if (setgid((gid_t)GID_NOBODY) == -1) {
			syslog (LOG_ERR, "setgid(%d): %m", GID_NOBODY);
			exit(1);
		}
		if (setuid((uid_t)UID_NOBODY) == -1) {
			syslog (LOG_ERR, "setuid(%d): %m", UID_NOBODY);
			exit(1);
		}
	}

	if (pipe (delay_fd) < 0) {
		syslog (LOG_ERR, "pipe (main): %m");
		exit (1);
	}

	(void) signal (SIGCHLD, childcleanup);

	if ((child = fork ()) < 0) {
		syslog (LOG_ERR, "fork (main): %m");
		exit (1);
	}

	if (child == 0) {
		struct delay_info dinfo;
		long now;

		/* we don't use the descriptors passed in to the parent */
		(void) close (0);
		(void) close (1);

		/* close write side of pipe */
		(void) close (delay_fd[1]);

		for (;;) {
			if (read (delay_fd [0], (char *) &dinfo, 
				  sizeof (dinfo)) != sizeof (dinfo)) {
				if (errno == EINTR)
					continue;
				syslog (LOG_ERR, "read from pipe: %m");
				exit (1);
			}

			peer = socket(AF_INET, SOCK_DGRAM, 0);
			if (peer < 0) {
				syslog(LOG_ERR, "socket (delay): %m");
				exit(1);
			}

			bzero ((char *) &sin, sizeof (sin));
			sin.sin_family = AF_INET;
			if (bind(peer, (struct sockaddr *) &sin, 
				 sizeof (sin)) < 0) {
				syslog(LOG_ERR, "bind (delay): %m");
				exit(1);
			      }

			dinfo.from.sin_family = AF_INET;
			if (connect(peer, (struct sockaddr *) &dinfo.from,
				    sizeof(dinfo.from)) < 0) {
				syslog(LOG_ERR, "connect (delay): %m");
				exit(1);
				break;
			}

			/*
			 * only sleep if DELAY_SECS has not elapsed since 
			 * original request was received.  Ensure that `now' 
			 * is not earlier than `dinfo.timestamp'
			 */
			now = time(0);
			if ((u_int)(now - dinfo.timestamp) < DELAY_SECS)
				sleep (DELAY_SECS - (now - dinfo.timestamp));
			nak (dinfo.ecode);
			(void) close (peer);
		} /* for */
		/*NOTREACHED*/
		exit (0);
	} /* child */

	/* close read side of pipe */
	(void) close (delay_fd[0]);


	/*
	 * Top level handling of incomming tftp requests.  Read a request
	 * and pass it off to be handled.  If request is valid, handling
	 * forks off and parent returns to this loop.  If no new requests
	 * are received for DALLYSECS, exit and return to inetd.
	 */

	for (;;) {
		int readfds;
		struct timeval dally;

		readfds = 0x1;
		dally.tv_sec = DALLYSECS;
		dally.tv_usec = 0;

		n = select (sizeof (int), &readfds, (int *) NULL, 
			    (int *) NULL, &dally);
		if (n < 0) {
			if (errno == EINTR)
			  continue;
			syslog (LOG_ERR, "select: %m");
		  	(void) kill (child, SIGKILL);
			exit (1);
		}
		if (n == 0) {
		  	(void) kill (child, SIGKILL);
			exit (0);
		}

		fromlen = sizeof (from);
		n = recvfrom(0, buf, sizeof (buf), 0,
			     (struct sockaddr *) &from, &fromlen);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			syslog(LOG_ERR, "recvfrom: %m");
			(void) kill (child, SIGKILL);
			exit(1);
		}

		(void) alarm(0);

		peer = socket(AF_INET, SOCK_DGRAM, 0);
		if (peer < 0) {
			syslog(LOG_ERR, "socket (main): %m");
			(void) kill (child, SIGKILL);
			exit(1);
		}

		bzero ((char *) &sin, sizeof (sin));
		sin.sin_family = AF_INET;
		if (bind(peer, (struct sockaddr *)&sin, sizeof (sin)) < 0) {
			syslog(LOG_ERR, "bind (main): %m");
			(void) kill (child, SIGKILL);
			exit(1);
		}

		from.sin_family = AF_INET;
		if (connect(peer, (struct sockaddr *)&from, sizeof(from)) 
		    < 0) {
			syslog(LOG_ERR, "connect (main): %m");
			(void) kill (child, SIGKILL);
			exit(1);
		}

		tp = (struct tftphdr *)buf;
		tp->th_opcode = ntohs((u_short) tp->th_opcode);
		if (tp->th_opcode == RRQ || tp->th_opcode == WRQ)
			tftp(tp, n);

		(void) close (peer);
		(void) fclose (file);
	}
}

int	validate_access();
int	sendfile(), recvfile();

struct formats {
	char	*f_mode;
	int	(*f_validate)();
	int	(*f_send)();
	int	(*f_recv)();
	int	f_convert;
} formats[] = {
	{ "netascii",	validate_access,	sendfile,	recvfile, 1 },
	{ "octet",	validate_access,	sendfile,	recvfile, 0 },
#ifdef notdef
	{ "mail",	validate_user,		sendmail,	recvmail, 1 },
#endif
	{ 0 }
};


/*
 * Handle initial connection protocol.
 */
tftp(tp, size)
	struct tftphdr *tp;
	int size;
{
	register char *cp;
	int first = 1, ecode;
	register struct formats *pf;
	char *filename, *mode;
	int pid;
	struct delay_info dinfo;

	filename = cp = tp->th_stuff;
again:
	while (cp < buf + size) {
		if (*cp == '\0')
			break;
		cp++;
	}
	if (*cp != '\0') {
		nak(EBADOP);
		exit(1);
	}
	if (first) {
		mode = ++cp;
		first = 0;
		goto again;
	}
	for (cp = mode; *cp; cp++)
		if (isupper(*cp))
			*cp = tolower(*cp);
	for (pf = formats; pf->f_mode; pf++)
		if (strcmp(pf->f_mode, mode) == 0)
			break;
	if (pf->f_mode == 0) {
		nak(EBADOP);
		exit(1);
	}
	ecode = (*pf->f_validate)(filename, tp->th_opcode);

	if (ecode) {
		/*
                 * The most likely cause of an error here is that
                 * someone has broadcast an RRQ packet because he's
                 * trying to boot and doesn't know who his server his.
                 * Rather then sending an ERROR packet immediately, we
                 * wait a while so that the real server has a better chance
                 * of getting through (in case client has lousy Ethernet
                 * interface).  We write to a child that handles delayed
		 * ERROR packets to avoid delaying service to new
		 * requests.
                 */
		dinfo.timestamp = time(0);
		dinfo.ecode = ecode;
		dinfo.from = from;
		if (write (delay_fd [1], (char *) &dinfo, sizeof (dinfo)) !=
		    sizeof (dinfo)) {
		  syslog (LOG_ERR, "delayed write failed.");
		  (void) kill (child, SIGKILL);
		  exit (1);
		}
		return;
	}
					        
	/* 
	 * fork a new process only after we have determined that we can 
	 * handle this request.
	 */

	pid = fork();
	if (pid < 0) {
		syslog(LOG_ERR, "fork (tftp): %m");
		return;
	}

	if (pid == 0) {
		/* we don't use the descriptors passed in to the parent */
		(void) close (0);
		(void) close (1);
		
		if (tp->th_opcode == WRQ)
			(*pf->f_recv)(pf);
		else
			(*pf->f_send)(pf);
		exit (0);
	}

	return;
}

/*
 *	Maybe map filename into another one.
 *
 *	For PNP, we get TFTP boot requests for filenames like 
 *	<Unknown Hex IP Addr>.<Sun Architecture Name>.   We must
 *	map these to 'pnp.<Sun Architecture Name>'.  Note that
 *	uppercase is mapped to lowercase in the architecture names.
 *
 *	For names <Hex IP Addr> there are two cases.  First,
 *	it may be a buggy prom that omits the architecture code.
 *	So first check if <Hex IP Addr>.<arch> is on the filesystem.
 *	Second, this is how most Sun3s work; assume <arch> is sun3.
 */

char *
pnp_check (filename)
    char *filename;
{
    static char buf [MAXNAMLEN + 1];
    char *arch, *s;
    long ipaddr;
    int len = (filename ? strlen (filename) : 0);
    struct hostent *hp;
    DIR *dir;
    struct dirent *dp;
    struct stat statb;

    if (securetftp || disable_pnp || len < 8 || len > 14)
	return (char *) NULL;

	/* XXX see if this cable allows pnp; if not, return NULL
	 * Requires NIS support for determining this!
	 */

    ipaddr = htonl (strtol (filename, &arch, 16));
    if (!arch || (len > 8 && *arch != '.'))
	return (char *) NULL;
    if (len == 8)
	arch = "SUN3";
    else
	arch++;

	/* allow <Hex IP Addr>* filename request to to be
	 * satisfied by <Hex IP Addr><Any Suffix> rather
	 * than enforcing this to be Sun3 systems.  Also serves
	 * to make case of suffix a don't-care.
	 */
    if ((dir = opendir ("/tftpboot")) == (DIR *) NULL)
	return (char *) NULL;
    while ((dp = readdir (dir)) != (struct dirent *)NULL) {
	if (strncmp (filename, dp->d_name, 8) == 0) {
	    strcpy (buf, dp->d_name);
	    closedir (dir);
	    return buf;
	}
    }
    closedir (dir);

	/* XXX maybe call NIS master for most current data iff
	 * pnp is enabled.
	 */

    hp = gethostbyaddr (&ipaddr, sizeof (ipaddr), AF_INET);

	/* map to PNP boot file name
	 */
    if (!hp) {
	strcpy (buf, "pnp.");
	for (s = &buf [4]; *arch; )
	    if (isupper (*arch))
		*s++ = tolower (*arch++);
	    else
		*s++ = *arch++;
	return buf;
    } else {
	return (char *) NULL;
    }
}



/*
 * Validate file access.  Since we
 * have no uid or gid, for now require
 * file to exist and be publicly
 * readable/writable.
 * Note also, full path name must be
 * given as we have no login directory.
 */
validate_access(filename, mode)
	char *filename;
	int mode;
{
	struct stat stbuf;
	int	fd;
	char *origfile;

	if (stat(filename, &stbuf) < 0) {
	    if (errno != ENOENT)
		return EACCESS;
	    origfile = filename;
	    if (mode != RRQ
		    || (filename = pnp_check (origfile)) == (char *) NULL
		    || stat (filename, &stbuf) < 0)
		return (errno == ENOENT ? ENOTFOUND : EACCESS);
	    syslog (LOG_NOTICE, "%s -> %s\n", origfile, filename);
	}

	if (mode == RRQ) {
		if ((stbuf.st_mode&(S_IREAD >> 6)) == 0)
			return (EACCESS);
	} else {
		if ((stbuf.st_mode&(S_IWRITE >> 6)) == 0)
			return (EACCESS);
	}
	if ((stbuf.st_mode & S_IFMT) != S_IFREG)
                return (EACCESS);
	fd = open(filename, mode == RRQ ? 0 : 1);
	if (fd < 0)
		return (errno + 100);
	file = fdopen(fd, (mode == RRQ)? "r":"w");
	if (file == NULL) {
		return errno+100;
	}
	return (0);
}

int	timeout;
jmp_buf	timeoutbuf;

timer()
{

	timeout += rexmtval;
	if (timeout >= maxtimeout)
		exit(1);
	longjmp(timeoutbuf, 1);
}

/*
 * Send the requested file.
 */
sendfile(pf)
	struct formats *pf;
{
	struct tftphdr *dp, *r_init();
	struct tftphdr *ap;    /* ack packet */
	int block = 1, size, n;

	signal(SIGALRM, timer);
	dp = r_init();
	ap = (struct tftphdr *)ackbuf;
	do {
		size = readit(file, &dp, pf->f_convert);
		if (size < 0) {
			nak(errno + 100);
			goto abort;
		}
		dp->th_opcode = htons((u_short)DATA);
		dp->th_block = htons((u_short)block);
		timeout = 0;
		(void) setjmp(timeoutbuf);

send_data:
		if (send(peer, dp, size + 4, 0) != size + 4) {
			if ((errno == ENETUNREACH) ||
			    (errno == EHOSTUNREACH) ||
			    (errno == ECONNREFUSED))
				syslog(LOG_WARNING, "send (data): %m");
			else
				syslog(LOG_ERR, "send (data): %m");
			goto abort;
		}
		read_ahead(file, pf->f_convert);
		for ( ; ; ) {
			alarm(rexmtval);        /* read the ack */
			n = recv(peer, ackbuf, sizeof (ackbuf), 0);
			alarm(0);
			if (n < 0) {
				if (errno == EINTR)
					continue;
				if ((errno == ENETUNREACH) ||
				    (errno == EHOSTUNREACH) ||
				    (errno == ECONNREFUSED))
					syslog(LOG_WARNING, "recv (ack): %m");
				else
					syslog(LOG_ERR, "recv (ack): %m");
				goto abort;
			}
			ap->th_opcode = ntohs((u_short)ap->th_opcode);
			ap->th_block = ntohs((u_short)ap->th_block);

			if (ap->th_opcode == ERROR)
				goto abort;
			
			if (ap->th_opcode == ACK) {
				if (ap->th_block == block) {
					break;
				}
				/* Re-synchronize with the other side */
				(void) synchnet(peer);
				if (ap->th_block == (block -1)) {
					goto send_data;
				}
			}

		}
		block++;
	} while (size == SEGSIZE);
abort:
	(void) fclose(file);
}

justquit()
{
	exit(0);
}


/*
 * Receive a file.
 */
recvfile(pf)
	struct formats *pf;
{
	struct tftphdr *dp, *w_init();
	struct tftphdr *ap;    /* ack buffer */
	int block = 0, n, size;

	signal(SIGALRM, timer);
	dp = w_init();
	ap = (struct tftphdr *)ackbuf;
	do {
		timeout = 0;
		ap->th_opcode = htons((u_short)ACK);
		ap->th_block = htons((u_short)block);
		block++;
		(void) setjmp(timeoutbuf);
send_ack:
		if (send(peer, ackbuf, 4, 0) != 4) {
			syslog(LOG_ERR, "send (ack): %m\n");
			goto abort;
		}
		write_behind(file, pf->f_convert);
		for ( ; ; ) {
			alarm(rexmtval);
			n = recv(peer, dp, PKTSIZE, 0);
			alarm(0);
			if (n < 0) {            /*really? */
				if (errno == EINTR)
					continue;
				syslog(LOG_ERR, "recv (data): %m");
				goto abort;
			}
			dp->th_opcode = ntohs((u_short)dp->th_opcode);
			dp->th_block = ntohs((u_short)dp->th_block);
			if (dp->th_opcode == ERROR)
				goto abort;
			if (dp->th_opcode == DATA) {
				if (dp->th_block == block) {
					break;   /* normal */
				}
				/* Re-synchronize with the other side */
				(void) synchnet(peer);
				if (dp->th_block == (block-1))
					goto send_ack;          /* rexmit */
			}
		}
		/*  size = write(file, dp->th_data, n - 4); */
		size = writeit(file, &dp, n - 4, pf->f_convert);
		if (size != (n-4)) {                    /* ahem */
			if (size < 0) nak(errno + 100);
			else nak(ENOSPACE);
			goto abort;
		}
	} while (size == SEGSIZE);
	write_behind(file, pf->f_convert);
	(void) fclose(file);            /* close data file */

	ap->th_opcode = htons((u_short)ACK);    /* send the "final" ack */
	ap->th_block = htons((u_short)(block));
	(void) send(peer, ackbuf, 4, 0);

	signal(SIGALRM, justquit);      /* just quit on timeout */
	alarm(rexmtval);
	n = recv(peer, buf, sizeof (buf), 0); /* normally times out and quits */
	alarm(0);
	if (n >= 4 &&                   /* if read some data */
	    dp->th_opcode == DATA &&    /* and got a data block */
	    block == dp->th_block) {	/* then my last ack was lost */
		(void) send(peer, ackbuf, 4, 0);     /* resend final ack */
	}
abort:
	return;
}

struct errmsg {
	int	e_code;
	char	*e_msg;
} errmsgs[] = {
	{ EUNDEF,	"Undefined error code" },
	{ ENOTFOUND,	"File not found" },
	{ EACCESS,	"Access violation" },
	{ ENOSPACE,	"Disk full or allocation exceeded" },
	{ EBADOP,	"Illegal TFTP operation" },
	{ EBADID,	"Unknown transfer ID" },
	{ EEXISTS,	"File already exists" },
	{ ENOUSER,	"No such user" },
	{ -1,		0 }
};

/*
 * Send a nak packet (error message).
 * Error code passed in is one of the
 * standard TFTP codes, or a UNIX errno
 * offset by 100.
 */
nak(error)
	int error;
{
	register struct tftphdr *tp;
	int length;
	register struct errmsg *pe;
	extern char *sys_errlist[];

	tp = (struct tftphdr *)buf;
	tp->th_opcode = htons((u_short)ERROR);
	tp->th_code = htons((u_short)error);
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			break;
	if (pe->e_code < 0) {
		pe->e_msg = sys_errlist[error - 100];
		tp->th_code = EUNDEF;   /* set 'undef' errorcode */
	}
	strcpy(tp->th_msg, pe->e_msg);
	length = strlen(pe->e_msg);
	tp->th_msg[length] = '\0';
	length += 5;
	if (send(peer, buf, length, 0) != length)
		syslog(LOG_ERR, "tftpd: nak: %m\n");
}
