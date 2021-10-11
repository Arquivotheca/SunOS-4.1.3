
#ifndef lint
static  char sccsid[] = "@(#)catch_signals.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <signal.h>
#include <esd.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */

extern char *program_name;
extern char *sprintf();

char	*sig_names[] = {"",
			"SIGHUP",
			"SIGINT",
			"SIGQUIT",
			"SIGILL",
			"SIGTRAP",
			"SIGIOT",
			"SIGEMT",
			"SIGFPE",
			"SIGKILL",
			"SIGBUS",
			"SIGSEGV",
			"SIGSYS",
			"SIGPIPE",
			"SIGALRM",
			"SIGTERM",
			"SIGURG",
			"SIGSTOP",
			"SIGTSTP",
			"SIGCONT",
			"SIGCHLD",
			"SIGTTIN",
			"SIGTTOU",
			"SIGIO",
			"SIGXCPU",
			"SIGXFSZ",
			"SIGVTALRM",
			"SIGPROF",
			"SIGWINCH",
			"SIGLOST",
			"SIGUSR1",
			"SIGUSR2", };

/**********************************************************************/
catch_signals()
/**********************************************************************/

{

    extern void signals_caught();

    func_name = "catch_signals";
    TRACE_IN

    /* catch signals when test dies */
    (void)signal(SIGILL, signals_caught);
    (void)signal(SIGTRAP, signals_caught);
    (void)signal(SIGEMT, signals_caught);
    (void)signal(SIGFPE, signals_caught);
    (void)signal(SIGBUS, signals_caught);
    (void)signal(SIGSEGV, signals_caught);
    (void)signal(SIGSYS, signals_caught);

    TRACE_OUT
}

/*ARGSUSED*/
/**********************************************************************/
void
signals_caught(sig, code, scp, addr)
/**********************************************************************/
int sig, code;
struct sigcontext *scp;
char *addr;

{

    char errtxt[512];


    func_name = "signals_caught";
    TRACE_IN

    if (sig > 0 && sig < 32) {
	(void)sprintf(errtxt, errmsg_list[2], program_name, sig_names[sig], code);
    } else {
	(void)sprintf(errtxt, errmsg_list[3], program_name, sig, code);
    }

    perror(errtxt);
    fatal_error_exit(errtxt);

    TRACE_OUT
}
