#ifndef lint
static char sccsid[] = "@(#)selection_svc.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1985, 1989 Sun Microsystems, Inc.
 */

#include <signal.h>
#include <sys/types.h>
#include  <sunwindow/notify.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

extern void seln_init_service();

#ifdef STANDALONE
main(argc, argv)
#else
selection_svc_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	int debug = 0;

	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'd')
		debug = 1;

	siginit();
	seln_init_service(debug);
	(void) notify_start();
	(void) seln_svc_unmap();

	EXIT(0);
}

static void
catch(sig)
	int sig;
{
	(void) seln_svc_unmap();

	if (sig != SIGTERM) {
		psignal(sig, "selection_svc exiting");
		exit(1);
	}

	exit(0);
}

static
siginit()
{
	static int siglist[] = {
		SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGABRT,
		SIGEMT, SIGFPE, SIGBUS, SIGSEGV, SIGSYS, SIGTERM,
		SIGXCPU, SIGXFSZ,
		0
	};

	register int *p;

	for (p = siglist; *p; p++)
		(void) signal(*p, catch);
}
