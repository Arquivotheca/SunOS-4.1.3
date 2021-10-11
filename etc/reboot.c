#ifndef lint
static char sccsid[] = "@(#)reboot.c 1.1 92/07/30 SMI"; /* from UCB 5.4 5/26/86 */
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
 * Reboot
 */
#include <stdio.h>
#include <sys/reboot.h>
#include <errno.h>
#include <signal.h>
#include <pwd.h>
#include <strings.h>
#include <syscall.h>
#include <sys/types.h>
#include <sys/label.h>
#include <sys/audit.h>
#include <sys/time.h>
#include <sys/syslog.h>
#include <sys/file.h>

void dingdong();

main(argc, argv)
	int argc;
	char **argv;
{
	int howto = 0;
	char *audit_argv[6];
	register int qflag = 0;
	int needlog = 1;
	char *user, *getlogin();
	struct passwd *pw, *getpwuid();
	extern char *optarg;
	extern int optind, opterr, errno;
	char c;

	openlog("reboot", 0, LOG_AUTH);

	audit_argv[0] = argv[0];
	while ((c = getopt(argc, argv, "qndl")) != EOF)
		switch((char)c) {
		case 'q':
			qflag++;
			break;
		case 'n':
			howto |= RB_NOSYNC;
			break;
		case 'd':
			howto |= RB_DUMP;
			break;
		case 'l':
			needlog = 0;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc == 1)
		howto |= RB_STRING;
	else if (argc != 0) 
		usage();

	audit_argv[1] = qflag ? "quick" : "not quick";
	audit_argv[2] = howto & RB_NOSYNC ? "not synced" : "synced";
	audit_argv[3] = howto & RB_DUMP ? "dump" : "no dump";
	audit_argv[4] = access("/fastboot", F_OK) ? "fastboot" : "do fsck";
	audit_argv[5] = howto & RB_STRING ? argv[0] : " ";

	(void)signal(SIGHUP, SIG_IGN);	/* for remote connections */
	if (kill(1, SIGTSTP) == -1) {
		(void)fprintf(stderr, "reboot: can't idle init\n");
		audit_text(AU_MAJPRIV, 1, 1, 6, audit_argv);
		exit(1);
	}

	if (needlog) {
		user = getlogin();
		if (user == NULL && (pw = getpwuid(getuid())) != NULL)
			user = pw->pw_name;
		if (user == NULL)
			user = "root";
		syslog(LOG_CRIT, "rebooted by %s", user);
	}

	sleep(1);
	(void)kill(-1, SIGTERM);	/* one chance to catch it */
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
			audit_text(AU_MAJPRIV, 2, 2, 6, audit_argv);
			exit(1);
		}

		if ((howto & RB_NOSYNC) == 0) {
			markdown();
			sync();
			(void)alarm(5);
			(void)pause();
		}
	}
	syscall(SYS_reboot, howto, argv[0]);
	audit_args(AU_MAJPRIV, 6, audit_argv);
	perror("reboot");
	audit_text(AU_MAJPRIV, 3, 3, 6, audit_argv);
	(void)kill(1, SIGHUP);
	exit(1);
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

usage()
{
	(void)fprintf(stderr,
	    "usage: reboot [ -dnql ] [ boot args ]\n");
	exit(1);
}
