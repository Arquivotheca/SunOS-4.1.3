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

#ifndef lint
static	char *sccsid = "@(#)in.rexecd.c 1.1 92/07/30 SMI"; /* from UCB 5.4 5/9/86 */
#endif not lint

#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/label.h>
#include <sys/audit.h>

#include <netinet/in.h>

#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <pwdadj.h>
#include <signal.h>
#include <netdb.h>

extern	errno;
struct	passwd *getpwnam();
char	*crypt(), *rindex(), *strncat(), *sprintf();
struct  passwd_adjunct *getpwanam();
char	*audit_argv[] = { "in.rexecd", 0, 0, 0, 0 };
/*VARARGS1*/
int	error();

/*
 * remote execute server:
 *	username\0
 *	password\0
 *	command\0
 *	data
 */
/*ARGSUSED*/
main(argc, argv)
	int argc;
	char **argv;
{
	struct sockaddr_in from;
	int fromlen;

	fromlen = sizeof (from);
	if (getpeername(0, &from, &fromlen) < 0) {
		fprintf(stderr, "%s: ", argv[0]);
		perror("getpeername");
		exit(1);
	}
	doit(0, &from);
	/* NOTREACHED */
}

char	username[20] = "USER=";
char	homedir[64] = "HOME=";
char	shell[64] = "SHELL=";
char	*envinit[] =
	    {homedir, shell, "PATH=:/usr/ucb:/bin:/usr/bin", username, 0};
char	**environ;

struct	sockaddr_in asin = { AF_INET };

/*
 * Berkeley uses NCARGS + 1 for this, but on SunOS that is too big.
 */
static char cmdbuf[BUFSIZ * 8];

doit(f, fromp)
	int f;
	struct sockaddr_in *fromp;
{
	char *cp, *namep;
	char user[16], pass[16];
	struct	hostent *chostp;
	struct passwd *pwd;
	struct  passwd_adjunct *apw;
	audit_state_t audit_state;
	int s;
	u_short port;
	int pv[2], pid, ready, readfrom, cc;
	char buf[BUFSIZ], sig;
	int one = 1;

	/*
	 * Set up the audit user information so that auditing failures
	 * can occur.
	 */
	if (issecure()) {
		setauid(0);
		audit_state.as_success = AU_LOGIN;
		audit_state.as_failure = AU_LOGIN;
        	setaudit(&audit_state);
	}

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
        /*
	 * store common info. for audit record 
         */
	chostp = gethostbyaddr(&fromp->sin_addr, sizeof(struct in_addr),
				fromp->sin_family);
	audit_argv[2] = chostp->h_name; 
	dup2(f, 0);
	dup2(f, 1);
	dup2(f, 2);
	(void) alarm(60);
	port = 0;
	for (;;) {
		char c;
		if (read(f, &c, 1) != 1)
			exit(1);
		if (c == 0)
			break;
		port = port * 10 + c - '0';
	}
	(void) alarm(0);
	if (port != 0) {
		s = socket(AF_INET, SOCK_STREAM, 0);
		if (s < 0)
			exit(1);
		if (bind(s, &asin, sizeof (asin)) < 0)
			exit(1);
		(void) alarm(60);
		fromp->sin_port = htons((u_short)port);
		if (connect(s, fromp, sizeof (*fromp)) < 0)
			exit(1);
		(void) alarm(0);
	}
	getstr(user, sizeof(user), "username");
	getstr(pass, sizeof(pass), "password");
	getstr(cmdbuf, sizeof(cmdbuf), "command");
	audit_argv[1] = user; 
	audit_argv[3] = cmdbuf; 
	setpwent();
	pwd = getpwnam(user);
	if (pwd == NULL) {
		audit_note(1, 1, "Login incorrect");
		error("Login incorrect.\n");
		exit(1);
	}
	endpwent();

	/* 
	 * set audit uid 
	 */
	setauid(pwd->pw_uid);

	/*
	 * get audit flags for user and set for this process
	 */
	if ((apw = getpwanam(user)) != NULL) {
		audit_state.as_success = 0;
		audit_state.as_failure = 0;

        	if ((getfauditflags(&apw->pwa_au_always,
			&apw->pwa_au_never, &audit_state))!=0) {
              		/*
               	 	 * if we can't tell how to audit from the 
			 * flags, audit everything that's not never 
			 * for this user.
               	 	 */
              		audit_state.as_success =
			    apw->pwa_au_never.as_success ^ (-1);
              		audit_state.as_failure =
			    apw->pwa_au_never.as_success ^ (-1);
		}

	} else {
		/* user entry in the passwd file but not the 
		 * passwd.adjunct file.  Audit everything.  
		 * If this isn't a secure system nothing will 
		 * be audited
		 */
                audit_state.as_success = -1; 
                audit_state.as_failure = -1; 
	}
	if (issecure())
        	setaudit(&audit_state);

	if (*pwd->pw_passwd != '\0') {
		namep = crypt(pass, pwd->pw_passwd);
		if (strcmp(namep, pwd->pw_passwd)) {
			audit_note(1, 1, "Password incorrect");
			error("Password incorrect.\n");
			exit(1);
		}
	}
	if (chdir(pwd->pw_dir) < 0) {
		audit_note(1, 1, "No remote directory");
		error("No remote directory.\n");
		exit(1);
	}
	(void) write(2, "\0", 1);
	if (port) {
		(void) pipe(pv);
		pid = fork();
		if (pid == -1)  {
			error("Try again.\n");
			exit(1);
		}
		if (pid) {
			(void) close(0); (void) close(1); (void) close(2);
			(void) close(f); (void) close(pv[1]);
			readfrom = (1<<s) | (1<<pv[0]);
			ioctl(pv[1], FIONBIO, (char *)&one);
			/* should set s nbio! */
			do {
				ready = readfrom;
				(void) select(16, &ready, (fd_set *)0,
				    (fd_set *)0, (struct timeval *)0);
				if (ready & (1<<s)) {
					if (read(s, &sig, 1) <= 0)
						readfrom &= ~(1<<s);
					else
						killpg(pid, sig);
				}
				if (ready & (1<<pv[0])) {
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
		(void) close(s); (void)close(pv[0]);
		dup2(pv[1], 2);
	}
	/* 
	 * output audit record for successful operation 
	 */
	audit_note(0, 0, "user authenticated");

	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = "/bin/sh";
	if (f > 2)
		(void) close(f);
	if (setgid((gid_t)pwd->pw_gid) == -1) {
		error("Invalid gid.\n");
		exit(1);
	}
	initgroups(pwd->pw_name, pwd->pw_gid);
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
	audit_note(1, 1, "can't exec");
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

getstr(buf, cnt, err)
	char *buf;
	int cnt;
	char *err;
{
	char c;

	do {
		if (read(0, &c, 1) != 1)
			exit(1);
		*buf++ = c;
		if (--cnt == 0) {
			error("%s too long\n", err);
			exit(1);
		}
	} while (c != 0);
}

audit_note(err, retcode, s)
	int err;
	int retcode;
	char *s;
{
	audit_argv[4] = s;
	audit_text(AU_LOGIN, err, retcode, 5, audit_argv);
}
