/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static  char sccsid[] = "@(#)in.rlogind.c 1.1 92/07/30 SMI"; /* from UCB 5.11 5/23/86 */
#endif

/*
 * remote login server:
 *	remuser\0
 *	locuser\0
 *	terminal info\0
 *	data
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/file.h>

#include <netinet/in.h>

#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <sgtty.h>
#include <stdio.h>
#include <netdb.h>
#include <syslog.h>
#include <strings.h>

# ifndef TIOCPKT_WINDOW
# define TIOCPKT_WINDOW 0x80
# endif TIOCPKT_WINDOW

extern	errno;
int	reapchild();
struct	passwd *getpwnam();
char	*malloc();

main(argc, argv)
	int argc;
	char **argv;
{
	int on = 1, options = 0, fromlen;
	struct sockaddr_in from;

	openlog("rlogind", LOG_PID | LOG_AUTH, LOG_AUTH);
	fromlen = sizeof (from);
	if (getpeername(0, &from, &fromlen) < 0) {
		fprintf(stderr, "%s: ", argv[0]);
		perror("getpeername");
		_exit(1);
	}
	if (setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof (on)) < 0) {
		syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");
	}
	doit(0, &from);
	/* NOTREACHED */
}

int	child;
int	cleanup();
int	netf;
extern	errno;
char	*line;
extern	char	*inet_ntoa();

struct winsize win = { 0, 0, 0, 0 };

doit(f, fromp)
	int f;
	struct sockaddr_in *fromp;
{
	int i, p, t, pid, on = 1;
	register struct hostent *hp;
	struct hostent hostent;
	char c, *cp;

	alarm(60);
	read(f, &c, 1);
	if (c != 0)
		exit(1);
	alarm(0);
	fromp->sin_port = ntohs((u_short)fromp->sin_port);
	hp = gethostbyaddr(&fromp->sin_addr, sizeof (struct in_addr),
		fromp->sin_family);
	if (hp == 0) {
		/*
		 * Only the name is used below.
		 */
		hp = &hostent;
		hp->h_name = inet_ntoa(fromp->sin_addr);
	}
	if (fromp->sin_family != AF_INET ||
	    fromp->sin_port >= IPPORT_RESERVED ||
	    fromp->sin_port < IPPORT_RESERVED/2)
		fatal(f, "Permission denied");
	write(f, "", 1);
	/*
	 * Find an unused pty.  A security hole exists if we
	 * allow any other process to hold the slave side of the
	 * pty open while we are setting up.  The other process
	 * could read from the slave side at the time that in.rlogind
	 * is forwarding the "rlogin authentication" string from the 
	 * rlogin client through the pty to the login program.  If the
	 * other process grabs this string, the user at the rlogin
	 * client program can forge an authentication string.
	 * This problem would not exist if the pty subsystem
	 * would fail the open of the master side when there are
	 * existing open instances of the slave side.
	 * Until the pty driver is changed to work this way, we use
	 * a side effect of one particular ioctl to test whether
	 * any process has the slave open.
	 */

	for (cp = "pqrstuvwxyzPQRST"; *cp; cp++) {
		struct stat stb;
		int pgrp;

		 /* make sure this "bank" of ptys exists */
		line = "/dev/ptyXX";
		line[strlen("/dev/pty")] = *cp;
		line[strlen("/dev/ptyp")] = '0';
		if (stat(line, &stb) < 0)
			break;
		for (i = 0; i < 16; i++) {
			line[strlen("/dev/ptyp")] = "0123456789abcdef"[i];
			line[strlen("/dev/")] = 'p';
			
			/*
			 * Test to see if this pty is unused.  The open
			 * on the master side of the pty will fail if
			 * any other process has it open.
			 */
			if ((p = open(line, O_RDWR | O_NOCTTY)) == -1)
				continue;

			/* 
			 * Lock the slave side so that no one else can 
			 * open it after this.
			 */
			line[strlen("/dev/")] = 't';
			if (chmod (line, 0600) == -1) {
				(void) close (p);
				continue;
			}

			/*
			 * XXX - Use a side effect of TIOCGPGRP on ptys
			 * to test whether any process is holding the
			 * slave side open.  May not work as we expect 
			 * in anything other than SunOS 4.1.
			 */
			if ((ioctl(p, TIOCGPGRP, &pgrp) == -1) &&
			    (errno == EIO))
				goto gotpty;
			else {
				(void) chmod(line, 0666);
				(void) close(p);
			}
		}
	}
	fatal(f, "Out of ptys");
	/*NOTREACHED*/
gotpty:
	(void) ioctl(p, TIOCSWINSZ, &win);
	netf = f;
#ifdef DEBUG
	{ 
		int tt = open("/dev/tty", 2);
		if (tt > 0) {
			ioctl(tt, TIOCNOTTY, 0);
			close(tt);
		}
	}
#endif
	line[strlen("/dev/")] = 't';
	t = open(line, O_RDWR | O_NOCTTY);
	if (t < 0)
		fatalperror(f, line, errno);
	{
		/*
		 * XXX - These should be converted to termios ioctls.
		 */
		struct sgttyb b;
		int zero = 0;  
	
		if (gtty(t, &b) == -1)
			perror("gtty");  
		b.sg_flags = RAW|ANYP; 
		/* XXX - ispeed and ospeed must be non-zero */
		b.sg_ispeed = B38400;
		b.sg_ospeed = B38400;
		if (stty(t, &b) == -1)
			perror("stty");
		/*
		 * Turn off PASS8 mode, since "login" no longer does so.
		 */
		if (ioctl(t, TIOCLSET, &zero) == -1)
			perror ("ioctl TIOCLSET"); 
	}
	pid = fork();
	if (pid < 0)
		fatalperror(f, "", errno);
	if (pid == 0) {
		int tt;

		/* 
		 * The child process needs to be the session leader
		 * and have the pty as its controlling tty.
		 */
		(void) setpgrp(0, 0);		/* setsid */
		tt = open(line, O_RDWR);
		if (tt < 0)
			fatalperror(f, line, errno);
		(void) close(f);
		(void) close(p);
		(void) close(t);
		if (tt != 0)
			(void) dup2(tt, 0);
		if (tt != 1)
			(void) dup2(tt, 1);
		if (tt != 2)
			(void) dup2(tt, 2);
		if (tt > 2)
			(void) close(tt);
		execl("/bin/login", "login", "-r", hp->h_name, (char *)0);
		fatalperror(2, "/bin/login", errno);
		/*NOTREACHED*/
	}
	close(t);
	ioctl(f, FIONBIO, &on);
	ioctl(p, FIONBIO, &on);
	ioctl(p, TIOCPKT, &on);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGCHLD, cleanup);
	setpgrp(0, 0);
	protocol(f, p);
	cleanup();
}

char	magic[2] = { 0377, 0377 };
char	oobdata[] = {TIOCPKT_WINDOW};

/*
 * Handle a "control" request (signaled by magic being present)
 * in the data stream.  For now, we are only willing to handle
 * window size changes.
 */
control(pty, cp, n)
	int pty;
	char *cp;
	int n;
{
	struct winsize w;
	int pgrp;

	if (n < 4+sizeof (w) || cp[2] != 's' || cp[3] != 's')
		return (0);
	oobdata[0] &= ~TIOCPKT_WINDOW;	/* we know he heard */

	bcopy(cp+4, (char *)&w, sizeof(w));
	w.ws_row = ntohs(w.ws_row);
	w.ws_col = ntohs(w.ws_col);
	w.ws_xpixel = ntohs(w.ws_xpixel);
	w.ws_ypixel = ntohs(w.ws_ypixel);
	(void)ioctl(pty, TIOCSWINSZ, &w);
	return (4+sizeof (w));
}

/*
 * rlogin "protocol" machine.
 */
protocol(f, p)
	int f, p;
{
	char pibuf[1024], fibuf[1024], *pbp, *fbp;
	register pcc = 0, fcc = 0;
	int cc, wsize;
	char cntl;

	/*
	 * Must ignore SIGTTOU, otherwise we'll stop
	 * when we try and set slave pty's window shape
	 * (our controlling tty is the master pty).
	 */
	(void) signal(SIGTTOU, SIG_IGN);
	send(f, oobdata, 1, MSG_OOB);	/* indicate new rlogin */
	for (;;) {
		int ibits, obits, ebits;

		ibits = 0;
		obits = 0;
		if (fcc)
			obits |= (1<<p);
		else
			ibits |= (1<<f);
		if (pcc >= 0)
			if (pcc)
				obits |= (1<<f);
			else
				ibits |= (1<<p);
		ebits = (1<<p);
		if (select(16, &ibits, &obits, &ebits, 0) < 0) {
			if (errno == EINTR)
				continue;
			fatalperror(f, "select", errno);
		}
		if (ibits == 0 && obits == 0 && ebits == 0) {
			/* shouldn't happen... */
			sleep(5);
			continue;
		}
#define	pkcontrol(c)	((c)&(TIOCPKT_FLUSHWRITE|TIOCPKT_NOSTOP|TIOCPKT_DOSTOP))
		if (ebits & (1<<p)) {
			cc = read(p, &cntl, 1);
			if (cc == 1 && pkcontrol(cntl)) {
				cntl |= oobdata[0];
				send(f, &cntl, 1, MSG_OOB);
				if (cntl & TIOCPKT_FLUSHWRITE) {
					pcc = 0;
					ibits &= ~(1<<p);
				}
			}
		}
		if (ibits & (1<<f)) {
			fcc = read(f, fibuf, sizeof (fibuf));
			if (fcc < 0 && (
				(errno == EWOULDBLOCK)||
				(errno == ENETUNREACH)||
				(errno == EHOSTUNREACH)
					))
				fcc = 0;
			else {
				register char *cp;
				int left, n;

				if (fcc <= 0)
					break;
				fbp = fibuf;

			top:
				for (cp = fibuf; cp < fibuf+fcc-1; cp++)
					if (cp[0] == magic[0] &&
					    cp[1] == magic[1]) {
						left = fcc - (cp-fibuf);
						n = control(p, cp, left);
						if (n) {
							left -= n;
							if (left > 0)
								bcopy(cp+n, cp, left);
							fcc -= n;
							goto top; /* n^2 */
						} /* if (n) */
					} /* for (cp = ) */
				} /* else */
		} /* if (ibits & (1<<f)) */

		if ((obits & (1<<p)) && fcc > 0) {
			wsize = fcc;
			do {
				cc = write(p, fbp, wsize);
				wsize /= 2;
			} while (cc<0 && errno==EWOULDBLOCK && wsize);
			if (cc > 0) {
				fcc -= cc;
				fbp += cc;
			}
		}

		if (ibits & (1<<p)) {
			pcc = read(p, pibuf, sizeof (pibuf));
			pbp = pibuf;
			if (pcc < 0 && errno == EWOULDBLOCK)
				pcc = 0;
			
			else if (pcc == 0)
				break;
			else if (pcc < 0) {
				/*
				 * Errors EIO (in BSD) and EINVAL (in System V)
				 * on the master side of a pty mean that
				 * the slave side has gone away.  Treat
				 * these the same as EOF.
				 */
				if ((errno == EIO) || (errno == EINVAL))
					break;
				else
					fatalperror(f, line, errno);
			}
			else if (pibuf[0] == 0)
				pbp++, pcc--;
			else {
				if (pkcontrol(pibuf[0])) {
					pibuf[0] |= oobdata[0];
					send(f, &pibuf[0], 1, MSG_OOB);

				}
				pcc = 0;
			}
		}
		if ((obits & (1<<f)) && pcc > 0) {
			wsize = pcc;
			do {
				cc = write(f, pbp, wsize);
				wsize /= 2;
			} while (cc<0 && errno==EWOULDBLOCK && wsize);
			if (cc > 0) {
				pcc -= cc;
				pbp += cc;
			}
		}
	}
}

cleanup()
{

	rmut();
	vhangup();		/* XXX */
	shutdown(netf, 2);
	exit(1);
}

fatal(f, msg)
	int f;
	char *msg;
{
	char buf[BUFSIZ];

	buf[0] = '\01';		/* error indicator */
	(void) sprintf(buf + 1, "rlogind: %s.\r\n", msg);
	(void) write(f, buf, strlen(buf));
	exit(1);
}

fatalperror(f, msg, errno)
	int f;
	char *msg;
	int errno;
{
	char buf[BUFSIZ];
	extern int sys_nerr;
	extern char *sys_errlist[];

	if ((unsigned)errno < sys_nerr)
		(void) sprintf(buf, "%s: %s", msg, sys_errlist[errno]);
	else
		(void) sprintf(buf, "%s: Error %d", msg, errno);
	fatal(f, buf);
}

#include <utmp.h>

struct	utmp wtmp;
char	wtmpf[]	= "/var/adm/wtmp";
char	utmpf[] = "/etc/utmp";
#define SCPYN(a, b)	strncpy(a, b, sizeof(a))
#define SCMPN(a, b)	strncmp(a, b, sizeof(a))

rmut()
{
	register f;
	int found = 0;
	struct utmp *u, *utmp;
	int nutmp;
	struct stat statbf;

	signal(SIGCHLD, SIG_IGN); /* while cleaning up don't allow for disruption */
	f = open(utmpf, O_RDWR);
	if (f >= 0) {
		fstat(f, &statbf);
		utmp = (struct utmp *)malloc(statbf.st_size);
		if (!utmp)
			syslog(LOG_ERR, "utmp malloc failed");
		if (statbf.st_size && utmp) {
			nutmp = read(f, utmp, statbf.st_size);
			nutmp /= sizeof(struct utmp);
		
			for (u = utmp ; u < &utmp[nutmp] ; u++) {
				if (SCMPN(u->ut_line, line+5) ||
				    u->ut_name[0]==0)
					continue;
				lseek(f, ((long)u)-((long)utmp), L_SET);
				SCPYN(u->ut_name, "");
				SCPYN(u->ut_host, "");
				time(&u->ut_time);
				write(f, (char *)u, sizeof(wtmp));
				found++;
			}
		}
		close(f);
	}
	if (found) {
		f = open(wtmpf, O_WRONLY|O_APPEND);
		if (f >= 0) {
			SCPYN(wtmp.ut_line, line+5);
			SCPYN(wtmp.ut_name, "");
			SCPYN(wtmp.ut_host, "");
			time(&wtmp.ut_time);
			write(f, (char *)&wtmp, sizeof(wtmp));
			close(f);
		}
	}
	chmod(line, 0666);
	chown(line, 0, 0);
	line[strlen("/dev/")] = 'p';
	chmod(line, 0666);
	chown(line, 0, 0);
	signal(SIGCHLD, cleanup);
}
