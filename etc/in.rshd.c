/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)in.rshd.c 1.1 92/07/30 SMI"; /* from UCB 5.11 4/18/88 */
#endif not lint

/*
 * remote shell server:
 *	remuser\0
 *	locuser\0
 *	command\0
 *	data
 */
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/label.h>
#include <sys/audit.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <pwdadj.h>
#include <signal.h>
#include <netdb.h>
#include <syslog.h>

#define AUDITNUM sizeof(audit_argv) / sizeof(char *)

int	errno;
char	*index(), *rindex(), *strncat();
char	*audit_argv[] = {"in.rshd", 0, 0, 0, 0, 0};
/*VARARGS1*/
int	error();

/*ARGSUSED*/
main(argc, argv)
	int argc;
	char **argv;
{
	struct linger linger;
	int on = 1, fromlen;
	struct sockaddr_in from;

	openlog("rsh", LOG_PID | LOG_ODELAY, LOG_DAEMON);
	fromlen = sizeof (from);
	if (getpeername(0, &from, &fromlen) < 0) {
		fprintf(stderr, "%s: ", argv[0]);
		perror("getpeername");
		_exit(1);
	}
	if (setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, (char *)&on,
	    sizeof (on)) < 0)
		syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");
	linger.l_onoff = 1;
	linger.l_linger = 60;			/* XXX */
	if (setsockopt(0, SOL_SOCKET, SO_LINGER, (char *)&linger,
	    sizeof (linger)) < 0)
		syslog(LOG_WARNING, "setsockopt (SO_LINGER): %m");
	doit(dup(0), &from);
	/* NOTREACHED */
}

char	username[20] = "USER=";
char	homedir[64] = "HOME=";
char	shell[64] = "SHELL=";
char	*envinit[] =
	    {homedir, shell, "PATH=:/usr/ucb:/bin:/usr/bin", username, 0};
char	**environ;

doit(f, fromp)
	int f;
	struct sockaddr_in *fromp;
{
	char *cp;
	char *locuser, *remuser, *cmdbuf;
	char *getstr();

	struct passwd *pwd;
	struct passwd_adjunct *apw, *getpwanam();
	audit_state_t astate;

	int s;
	struct hostent *hp;
	char *hostname;
	short port;
	int pv[2], pid, ready, readfrom, cc;
	char buf[BUFSIZ], sig;
	int one = 1;

	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTERM, SIG_DFL);
#ifdef DEBUG
	{ int t = open("/dev/tty", 2);
	  if (t >= 0) {
		ioctl(t, TIOCNOTTY, (char *)0);
		(void) close(t);
	  }
	}
#endif
	fromp->sin_port = ntohs((u_short)fromp->sin_port);
	if (fromp->sin_family != AF_INET) {
		syslog(LOG_ERR, "malformed from address\n");
		exit(1);
	}
	if (fromp->sin_port >= IPPORT_RESERVED ||
	    fromp->sin_port < IPPORT_RESERVED/2) {
		syslog(LOG_NOTICE, "connection from bad port\n");
		exit(1);
	}
	(void) alarm(60);
	port = 0;
	for (;;) {
		char c;
		if ((cc = read(f, &c, 1)) != 1) {
			if (cc < 0)
				syslog(LOG_NOTICE, "read: %m");
			shutdown(f, 1+1);
			exit(1);
		}
		if (c == 0)
			break;
		port = port * 10 + c - '0';
	}
	(void) alarm(0);
	if (port != 0) {
		int lport = IPPORT_RESERVED - 1;
		s = rresvport(&lport);
		if (s < 0) {
			syslog(LOG_ERR, "can't get stderr port: %m");
			exit(1);
		}
		if (port >= IPPORT_RESERVED) {
			syslog(LOG_ERR, "2nd port not reserved\n");
			exit(1);
		}
		fromp->sin_port = htons((u_short)port);
		if (connect(s, fromp, sizeof (*fromp)) < 0) {
			syslog(LOG_INFO, "connect second port: %m");
			exit(1);
		}
	}
	dup2(f, 0);
	dup2(f, 1);
	dup2(f, 2);
	hp = gethostbyaddr((char *)&fromp->sin_addr, sizeof (struct in_addr),
		fromp->sin_family);
	if (hp)
		hostname = hp->h_name;
	else
		hostname = inet_ntoa(fromp->sin_addr);

	remuser = getstr(16, "remuser");
	locuser = getstr(16, "locuser");
	cmdbuf = getstr(4096, "command");

    	/*
	 * store common info. for audit record 
     	 */
	audit_argv[1] = remuser; 
	audit_argv[2] = locuser; 
	audit_argv[3] = hostname; 
	audit_argv[4] = cmdbuf; 

	setpwent();
	pwd = getpwnam(locuser);
	if (pwd == NULL) {
		audit_write(1, "Login incorrect"); 
		error("Login incorrect.\n");
		exit(1);
	}
	endpwent();
	setauid(pwd->pw_uid);
	/*
	 * get audit flags for user and set for all 
	 * processes owned by this uid 
	 */
	if ((apw = getpwanam(locuser)) != NULL) {
		astate.as_success = 0;
		astate.as_failure = 0;

		if ((getfauditflags(&apw->pwa_au_always, 
		  &apw->pwa_au_never, &astate)) != 0) {
			/*
             		* if we can't tell how to audit from the flags, audit
             		* everything that's not never for this user.
			*/
            		astate.as_success = apw->pwa_au_never.as_success ^ (-1);
            		astate.as_failure = apw->pwa_au_never.as_success ^ (-1);
		}
	}
	else {
		astate.as_success = -1;
		astate.as_failure = -1;
	}

	if (issecure())
		setuseraudit(pwd->pw_uid, &astate);

	if (chdir(pwd->pw_dir) < 0) {
		(void) chdir("/");
#ifdef notdef
		error("No remote directory.\n");
		exit(1);
#endif
	}
	if (pwd->pw_passwd != 0 && *pwd->pw_passwd != '\0' &&
	    ruserok(hostname, pwd->pw_uid == 0, remuser, locuser) < 0) {
		audit_write(1, "Permission denied"); 
		error("Permission denied.\n");
		exit(1);
	}
	(void) write(2, "\0", 1);
	if (port) {
		if (pipe(pv) < 0) {
			audit_write(1, "Can't make pipe."); 
			error("Can't make pipe.\n");
			exit(1);
		}
		pid = fork();
		if (pid == -1)  {
			audit_write(1, "Error in fork()."); 
			error("Try again.\n");
			exit(1);
		}
		if (pid) {
			(void) close(0); (void) close(1); (void) close(2);
			(void) close(f); (void) close(pv[1]);
			readfrom = (1<<s) | (1<<pv[0]);
			ioctl(pv[0], FIONBIO, (char *)&one);
			/* should set s nbio! */
			do {
				ready = readfrom;
				if (select(16, &ready, (fd_set *)0,
				    (fd_set *)0, (struct timeval *)0) < 0)
					break;
				if (ready & (1<<s)) {
					if (read(s, &sig, 1) <= 0)
						readfrom &= ~(1<<s);
					else
						killpg(pid, sig);
				}
				if (ready & (1<<pv[0])) {
					errno = 0;
					cc = read(pv[0], buf, sizeof (buf));
					if (cc <= 0) {
						shutdown(s, 1+1);
						readfrom &= ~(1<<pv[0]);
					} else
						(void) write(s, buf, cc);
				}
			} while (readfrom);
			exit(0);
		}
		setpgrp(0, getpid());
		(void) close(s); (void) close(pv[0]);
		dup2(pv[1], 2);
	}
	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = "/bin/sh";
	(void) close(f);
	if (setgid((gid_t)pwd->pw_gid) == -1) {
		error("Invalid gid.\n");
		exit(1);
	}
	initgroups(pwd->pw_name, pwd->pw_gid);

	/*
	 * write audit record before making uid switch  
	 */
	audit_write(0, "authorization successful"); 

	if (setuid((uid_t)pwd->pw_uid) == -1) {
		error("Invalid uid.\n");
		exit(1);
	}
	environ = envinit;
	strncat(homedir, pwd->pw_dir, sizeof(homedir)-6);
	strncat(shell, pwd->pw_shell, sizeof(shell)-7);
	strncat(username, pwd->pw_name, sizeof(username)-6);
	cp = rindex(pwd->pw_shell, '/');
	if (cp)
		cp++;
	else
		cp = pwd->pw_shell;
	execl(pwd->pw_shell, cp, "-c", cmdbuf, (char *)0);
	perror(pwd->pw_shell);
	audit_write(1, "can't exec");
	exit(1);
}

/*VARARGS1*/
error(fmt, a1, a2, a3)
	char *fmt;
	int a1, a2, a3;
{
	char buf[BUFSIZ];

	buf[0] = 1;
	(void) sprintf(buf+1, fmt, a1, a2, a3);
	(void) write(2, buf, strlen(buf));
}

char *
getstr(inc, err)
	int inc;
	char *err;
{
	char *buf, *malloc(), *realloc();
	unsigned bufsize;
	char c;
	int i = 0;

	if (buf = malloc(bufsize = inc))
		while (1) {
			if (read(0, &c, 1) != 1)
				break;

			buf[i] = c;

			if (c == 0)
				return buf;

			if (++i >= bufsize &&
				(buf = realloc(buf, bufsize += inc)) == 0)
				break;
		}

	error("%s too long\n", err);
	exit(1);
}

audit_write(val, message)
int val;
char *message;
{
	audit_argv[5] = message;
	audit_text(AU_LOGIN, val, val, AUDITNUM, audit_argv);
}
