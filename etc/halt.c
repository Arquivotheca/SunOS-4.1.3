#ifndef lint
static char sccsid[] = "@(#)halt.c 1.1 92/07/30 SMI"; /* from UCB 5.4 5/26/86 */
#endif

/*
 * Copyright (c) 1980,1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980,1986 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

/*
 * Halt
 */
#include <stdio.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/label.h>
#include <sys/audit.h>
#include <sys/time.h>
#include <sys/syslog.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <syscall.h>
#include <signal.h>
#include <pwd.h>
#include <strings.h>

void dingdong();

main(argc, argv)
	int argc;
	char **argv;
{
	int howto;
	char *ttyname(), *ttyn = ttyname(2);
	char *audit_argv[6];
	register int qflag = 0;
	int needlog = 1;
	char *user, *getlogin();
	struct passwd *pw, *getpwuid();
	extern int errno;

	openlog("halt", 0, LOG_AUTH);
	howto = RB_HALT;
	audit_argv[0] = *argv++;
	audit_argv[1] = "not quick";
	audit_argv[2] = "synced";
	audit_argv[3] = access("/fastboot", F_OK) ? "fasthalt" : "do fsck";
	audit_argv[4] = (ttyn == NULL) ? "(no tty)" : ttyn;
	audit_argv[5] = "logged";
	while (--argc != 0) {
		if (strcmp(*argv, "-n") == 0) {
			audit_argv[2] = "not synced";
			howto |= RB_NOSYNC;
		} else if (strcmp(*argv, "-y") == 0) {
			audit_argv[4] = "dialup ( -y )";
			ttyn = 0;
		} else if (strcmp(*argv, "-q") == 0) {
			audit_argv[1] = "quick";
			qflag++;
		} else if (strcmp(*argv, "-l") == 0) {
			audit_argv[5] = "not logged";
			needlog = 0;
		} else {
			(void)fprintf(stderr, "usage: halt [ -n ]\n");
			audit_text(AU_MAJPRIV, 1, 1, 6, audit_argv);
			exit(1);
		}
		argv++;
	}

	if (ttyn && !strncmp(ttyn, "/dev/ttyd", strlen("/dev/ttyd"))) {
		(void)fprintf(stderr, 
"halt: dangerous on a dialup; use ``halt -y'' if you are really sure\n");
		audit_text(AU_MAJPRIV, 1, 1, 6, audit_argv);
		exit(1);
	}

	(void)signal(SIGHUP, SIG_IGN);		/* for network connections */
	if (kill(1, SIGTSTP) == -1) {
		(void)fprintf(stderr, "halt: can't idle init\n");
		audit_text(AU_MAJPRIV, 1, 1, 6, audit_argv);
		exit(1);
	}

	if (needlog) {
		user = getlogin();
		if (user == NULL && (pw = getpwuid(getuid())) != NULL)
			user = pw->pw_name;
		if (user == NULL)
			user = "root";
		syslog(LOG_CRIT, "halted by %s", user);
	}
	sleep(1);
	(void) kill(-1, SIGTERM);	/* one chance to catch it */
	sleep(5);

	if (!qflag)  {
		register unsigned i = 0;
		(void)signal(SIGALRM, dingdong);
		while (kill(-1, SIGKILL) == 0) {
			if (++i == 6) {
				(void)fprintf(stderr,
			 "CAUTION: some process(es) wouldn't die\n");
				errno = 0;
				break;
			}
			(void)alarm(2 * i);
			(void)pause();
		}

		if (errno != 0 && errno != ESRCH) {
			perror("reboot: kill");
			(void)kill(1, SIGHUP);
			audit_text(AU_MAJPRIV, 1, 1, 6, audit_argv);
			exit(1);
		}

		if ((howto & RB_NOSYNC) == 0) {
			markdown();
			sync();
			(void)alarm(5);
			(void)pause();
		}
	}
	audit_args(AU_MAJPRIV, 6, audit_argv);
	syscall(SYS_reboot, howto);
	audit_text(AU_MAJPRIV, 1, 1, 6, audit_argv);
	perror("reboot");
	exit(0);
	/* NOTREACHED */
}

void
dingdong()
{
	/* RRRIIINNNGGG RRRIIINNNGGG */
}

#include <utmp.h>
#define SCPYN(a, b)	(void)strncpy(a, b, sizeof (a))
char wtmpf[] = "/var/adm/wtmp";

markdown()
{
	off_t lseek();
	time_t time();
	struct utmp wtmp;
	register int f = open(wtmpf, O_WRONLY, 0);
	if (f >= 0) {
		(void)lseek(f, 0L, L_XTND);
		SCPYN(wtmp.ut_line, "~");
		SCPYN(wtmp.ut_name, "shutdown");
		SCPYN(wtmp.ut_host, "");
		wtmp.ut_time = time((time_t *)0);
		(void)write(f, (char *)&wtmp, sizeof wtmp);
		(void)close(f);
	}
}
