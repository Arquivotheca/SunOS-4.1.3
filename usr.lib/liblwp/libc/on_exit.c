/* Copyright (C) 1986 Sun Microsystems Inc. */
#include "common.h"
#include "queue.h"
#include "asynch.h"
#include "machsig.h"
#include "machdep.h"
#include "lwperror.h"
#include "cntxt.h"
#include "message.h"
#include "process.h"
#include "schedule.h"
#include "alloc.h"
#include "condvar.h"
#include "monitor.h"
#include "agent.h"
#include "libc.h"
#ifndef lint
SCCSID(@(#) on_exit.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

/*
 * PRIMITIVES contained herein:
 * pod_exit(status)
 */

/*
 * Storage for procedures and arguments invoked at pod termination.
 */
typedef struct exithan_t {
    void      (*exit_handler)();	/* exit handler procedure */
    caddr_t	exit_arg;		/* argument to handler procedure */
} exithan_t;

STATIC int NumExitHandlers = 0;		/* Number of exit handlers set */

extern void _cleanup();			/* from C library */

/* the list of exit handlers */
STATIC exithan_t ExitActs[NEXITHANDLERS] = {{ _cleanup, 0}};

/*
 * on_exit -- replacement for C library on_exit.
 * Establish an exit handler for pod termination.
 * The exit handlers are only invoked when the pod terminates;
 * exit(3) now just terminates the calling thread.
 */
int
on_exit(procp, arg)
	int (*procp)();		/* procedure to be invoked on pod termination */
	caddr_t arg;		/* argument to procedure */
{
	LWPINIT();
	LOCK();
	NumExitHandlers++;
	ERROR((NumExitHandlers >= NEXITHANDLERS), LE_NOROOM);
	ExitActs[NumExitHandlers].exit_handler = (void (*)())procp;
	ExitActs[NumExitHandlers].exit_arg = arg;
	UNLOCK();
	return (0);
}

/*
 * exit -- replacement for C library exit.
 * Terminate the thread and set the exit status for the pod.
 */
void
exit(code)
	int code;
{
	LWPINIT();
	pod_setexit(code);
	(void)lwp_destroy(SELF);
}

/*
 * __do_exithand -- INTERFACE to lwp library.
 * Invoke exit handlers just prior to pod termination.
 * Called by idle thread when it terminates.
 */
void
__do_exithand()
{
	register int i;

	for (i = NumExitHandlers; i >= 0; i--) {
		ExitActs[i].exit_handler(ExitActs[i].exit_arg);
	}
}

/*
 * pod_exit -- PRIMITIVE.
 * Replacement for exit(3) if pod termination is desired.
 * Terminate entire pod forcefully.
 */
void
pod_exit(status)
	int status;
{
	pod_setexit(status);
	__do_exithand();
	_exit(pod_getexit());
	/* NOTREACHED */
}
