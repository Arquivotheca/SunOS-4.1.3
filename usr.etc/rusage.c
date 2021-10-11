#ifndef lint
static  char sccsid[] = "@(#)rusage.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * rusage
 */

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

main(argc, argv)
	int argc;
	char **argv;
{
	union wait status;
	int options=0;
	register int p;
	struct timeval before, after;
	struct rusage ru;
	struct timezone tz;

	if (argc<=1)
		exit(0);
	(void) gettimeofday(&before, &tz);

	/* fork a child process to run the command */

	p = fork();
	if (p < 0) {
		perror("rusage");
		exit(1);
	}

	if (p == 0) {

		/* exec the command specified */

		execvp(argv[1], &argv[1]);
		perror(argv[1]);
		exit(1);
	}

	/* parent code - wait for command to complete */

	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
	while (wait3(&status, options, &ru) != p)
		;

	/* get closing time of day */
	(void) gettimeofday(&after, &tz);

	/* check for exit status of command */

	if ((status.w_termsig) != 0)
		(void) fprintf(stderr, "Command terminated abnormally.\n");

	/* print an accounting summary line */

	after.tv_sec -= before.tv_sec;
	after.tv_usec -= before.tv_usec;
	if (after.tv_usec < 0) {
		after.tv_sec--;
		after.tv_usec += 1000000;
	}
	fprintt("real", &after);
	fprintt("user", &ru.ru_utime);
	fprintt("sys", &ru.ru_stime);
	(void) fprintf(stderr,"%D pf %D pr %D sw",
		ru.ru_majflt,
		ru.ru_minflt,
		ru.ru_nswap);
	(void) fprintf(stderr," %D rb %D wb %D vcx %D icx",
		ru.ru_inblock,
		ru.ru_oublock,
		ru.ru_nvcsw,
		ru.ru_nivcsw);
	(void) fprintf(stderr, " %D mx %D ix %D id %D is",
		ru.ru_maxrss,
		ru.ru_ixrss,
		ru.ru_idrss,
		ru.ru_isrss);

	(void) fprintf(stderr, "\n");
	exit((int)status.w_retcode);
	/*NOTREACHED*/
}

fprintt(s, tv)
	char *s;
	struct timeval *tv;
{

	(void) fprintf(stderr, "%d.%02d %s ", tv->tv_sec, tv->tv_usec/10000, s);
}
