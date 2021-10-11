/* Copyright (C) 1986 Sun Microsystems Inc. */
/*
 * This source code is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify this source code without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * THIS PROGRAM CONTAINS SOURCE CODE COPYRIGHTED BY SUN MICROSYSTEMS, INC.
 * AND IS LICENSED TO SUNSOFT, INC., A SUBSIDIARY OF SUN MICROSYSTEMS, INC.
 * SUN MICROSYSTEMS, INC., MAKES NO REPRESENTATIONS ABOUT THE SUITABLITY
 * OF SUCH SOURCE CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT
 * EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  SUN MICROSYSTEMS, INC. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO SUCH SOURCE CODE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN
 * NO EVENT SHALL SUN MICROSYSTEMS, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT,
 * INCIDENTAL, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM USE OF SUCH SOURCE CODE, REGARDLESS OF THE THEORY OF LIABILITY.
 * 
 * This source code is provided with no support and without any obligation on
 * the part of Sun Microsystems, Inc. to assist in its use, correction, 
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS
 * SOURCE CODE OR ANY PART THEREOF.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California 94043
 */
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include "common.h"
#include "queue.h"
#include "asynch.h"
#include "machsig.h"
#include "machdep.h"
#include "cntxt.h"
#include "lwperror.h"
#include "message.h"
#include "process.h"
#include "schedule.h"
#include "agent.h"
#include "alloc.h"
#include "monitor.h"
#include "clock.h"
#include <syscall.h>
#ifndef lint
SCCSID(@(#) machsig.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

/*
 * This file contains machine-specific details about mapping asynchronous
 * events into agents.
 * Because the machine is never allowed to block, we need a special
 * mechanism to do this (the agent takes care of holding the "thread"
 * caused by the interrupt).
 * "soft" interrupts should be provided by using helper lwps to provide
 * the effect of non-blocking messages.
 * For UNIX userland, the asynchronous events are the UNIX signals.
 * On a bare machine, the asynchronous events are the device interrupts.
 * Each environment will supply a different version of this file and supply
 * appropriate versions of
 * __Eventinfo[], __enable(), disable(), and init_sig().
 * ALARMSIGNAL should also be defined as the event which happens when
 * the clock ticks so __do_agent knows when to call __timesup().
 * Finally, __setclock and __update manipulate the hardware clock.
 * INTERRUPTS DO NOT NEST.
 * If the interrupt handler wishes to delay notification, it may choose
 * to return -1.
 * Also, if an interrupt handler wishes to avoid notification when
 * a pending notification exists, should add code to agt_create
 * to register the agent with the handler.
 * The handler can then look (at interrupt level) at the message queue for
 * that agent.
 */

/*
 * This version supplies 4.2 BSD signal mapping
 * CAVEAT: UNIX signals can be "folded" in that if a given signal arrives
 * while another one of the same type is still pending, a second
 * interrupt will not be generated. The client should be prepared
 * for this possibility (e.g., on SIGIO, check that no data is
 * available before going to sleep).
 */

extern	void __interrupt();

/* handler for enabled UNIX signals */
STATIC struct sigvec Svec = {
	(void (*)())__interrupt,	/* signal handler */
	-1,				/* no nested signals */
	SV_ONSTACK,			/* use alternate stack */
};

/*
 * signal handler for resetting UNIX signals.
 * We use SIG_DFL for this; arguably SIG_IGN is correct.
 */
STATIC struct sigvec Dvec = {
	SIG_DFL,	/* signal handler */
	-1,		/* no nested signals */
	SV_ONSTACK,	/* use alternate stack */
};

STATIC int DefsigType;		/* cookie for default event caches */
STATIC int SigchldType;		/* cookie for SIGCHLD events */
STATIC int LastRitesType;	/* cookie for LASTRITES events */

extern sigdstat_t __sigdefault();
extern sigdstat_t __errsig();
extern sigdstat_t __sigchldinfo();

/*
 * Some devices (e.g., intel ethernet) may have chained dma interrupts
 * such that the return to low prio enables the next buffer
 * to be filled. If the driver rearranges these buffers
 * at interrupt time, it will assume that no more interrupts
 * are happening. To achieve this effect,
 * a given int_start routine may need to disable a device.
 * A client can delay replying until it has explicitly reenabled the device.
 */

/*
 * Table of interrupt handling info.
 * The second entry will hold the cookie for the event cache
 * information upon interrupt; the first entry specifies how
 * to get that information.
 * If the first entry is TRAPEVENT, this is a synchronous trap.
 * SIGSEGV is considered asynchronous because the stack may be garbaged
 * and recovery may only be possible by another thread.
 * In some cases, the boundary between trap and interrupt is vague
 * in that you may not be able to tell exactly which thread was
 * running when the fault happened (e.g., FP coprocessor may be behind you).
 * SIGALRM will not be delivered to client: lwps need it to mux the clock.
 * Events which can occur during an lwp pimitive invocation must
 * be considered asynchronous.
 */
sigdesc_t __Eventinfo[] =
{
	__errsig,	0,	/* NOTHING	0 */
	__sigdefault,	0,	/* SIGHUP	1 */
	__sigdefault,	0,	/* SIGINT	2 */
	__sigdefault,	0,	/* SIGQUIT	3 */
	__sigdefault,	0,	/* SIGILL	4 */
	__sigdefault,	0,	/* SIGTRAP	5 */
	__errsig,	0,	/* SIGIOT	6 */
	__sigdefault,	0,	/* SIGEMT	7 */
	__sigdefault,	0,	/* SIGFPE	8 */
	__errsig,	0,	/* SIGKILL	9 */
	__sigdefault,	0,	/* SIGBUS	10 */
	__sigdefault,	0,	/* SIGSEGV	11 */
	__sigdefault,	0,	/* SIGSYS	12 */
	__sigdefault,	0,	/* SIGPIPE	13 */
	__sigdefault,	0,	/* SIGALRM	14 */
	__sigdefault,	0,	/* SIGTERM	15 */
	__sigdefault,	0,	/* SIGURG	16 */
	__errsig,	0,	/* SIGSTOP	17 */
	__sigdefault,	0,	/* SIGTSTP	18 */
	__sigdefault,	0,	/* SIGCONT	19 */
	__sigchldinfo,	0,	/* SIGCHLD	20 */
	__sigdefault,	0,	/* SIGTTIN	21 */
	__sigdefault,	0,	/* SIGTTOU	22 */
	__sigdefault,	0,	/* SIGIO	23 */
	__sigdefault,	0,	/* SIGXCPU	24 */
	__sigdefault,	0,	/* SIGXFSZ	25 */
	__sigdefault,	0,	/* SIGVTALRM 	26 */
	__sigdefault,	0,	/* SIGPROF	27 */
	__sigdefault,	0,	/* SIGWINCH 	28 */
	__sigdefault,	0,	/* SIGLOST 	29 */
	__sigdefault,	0,	/* SIGUSR1 	30 */
	__sigdefault,	0,	/* SIGUSR2 	31 */
	__sigdefault,	0,	/* LASTRITES	32 */
};

/*
 * handle illegal interrupt
 */
sigdstat_t
__errsig(event)
	event_t *event;
{
#ifdef lint
	event = event;
#endif lint

	__panic("bad signal??");
	/* NOTREACHED */
}

/*
 * null routine to save no info about an interrupt (i.e., just the kind
 * of interrupt is sufficient info).
 */
sigdstat_t
__sigdefault(event)
	event_t *event;
{
#ifdef lint
	event = event;
#endif lint

	return (SIG_DONE);
}

/*
 * routine to gather info about a process that changed status (SIGCHLD)
 */
sigdstat_t
__sigchldinfo(event)
	event_t *event;
{
	register int pid;
	union wait status;
	struct rusage rusage;
	register sigchld_t *s = (sigchld_t *)event;
	int olderrno;

	olderrno = errno;
	pid = wait3(&status, WNOHANG|WUNTRACED, &rusage);
	errno = olderrno;
	if ((pid == 0) || (pid == -1)) {
		return (SIG_ERROR);
	}
	s->sigchld_pid = pid;
	STRUCTASSIGN(&status, &s->sigchld_status);
	STRUCTASSIGN(&rusage, &s->sigchld_rusage);
	return (SIG_CONT);
}

/*
 * enable interrupts for a particular event
 */
int
__enable(event)
	int event;
{
	register int olderrno;

	olderrno = errno;
	(void) _sigvec(event, &Svec, (struct sigvec *)0);
	errno = olderrno;
}

/*
 * Reset signal handler (in UNIX, to default action; in kernel, disable
 * device interrupts for the specified device).
 * This is called when the last agent for an interrupt goes away.
 */
int
__disable(event)
	int event;
{
	register int olderrno;

	olderrno = errno;
	(void) _sigvec(event, &Dvec, (struct sigvec *)0);
	errno = olderrno;
}

/*
 * clock manipulation routines
 */

/*
 * reset the clock. Make sure that no stale clock events remain
 * as could happen, for example, when cancelling the current alarm.
 */
void
__setclock(timeout)
	struct timeval *timeout;
{
	struct itimerval it, oitv;
	register struct itimerval *itp = &it;
	register int olderrno = errno;
	int mask;

	/*
	 * UNIX may store a signal while we're disabled.
	 * This interrupt will be delivered
	 * as a result of returning from a system call for example.
	 * So, we ensure that no alarms other than the one we are setting
	 * will be delivered by disabling alarms, which has the side effect
	 * of causing any signals to be delivered, then removing all alarms
	 * from the event queue.
	 * (In an LWP world, killing all threads with agents for the signal
	 * (or alternatively, all agents)
	 * removes the signal from the address space since pending events are
	 * destroyed when there are no agents for it).
	 * This routine may be called while DISABLED or LOCKED or both.
	 * If not LOCKED, we want to ensure that we don't try to schedule
	 * via UNLOCK after ENABLE so we explicitly set/clear __AsynchLock.
	 * During the ENABLED time, events can enter the event queue
	 * via save_event, but no scheduling happens.
	 */
	__AsynchLock++;		/* only save interrupts */

	/* lock all interrupts BUT clock */
	if ((mask = sigsetmask(~sigmask(SIGALRM))) == -1)
		__panic("sigsetmask");
	timerclear(&itp->it_value);
	timerclear(&itp->it_interval);

	/* cancel all alarms and give UNIX a chance to deliver any old ones */
	if (setitimer(ITIMER_REAL, itp, &oitv)  < 0)	/* cancel all alarms */
		__panic("bad timer");
	/*
	 * No more alarms remain in the OS, so we can lock out clock too.
	 * We will be in DISABLED state; set it so macro is correct.
	 * Note that __rem_event requires ALL interrupts to be masked.
	 */
	if (sigsetmask(-1) == -1)
		__panic("sigsetmask");
	if (__SigLocked == 0) {
		__Omask = mask;
	}
	__SigLocked++;

	/* at this point, all alarms must be on the event queue */
	__AsynchLock--;	/* reset critical section state but don't unlock */
	while (__rem_event(SIGALRM))	/* must be called at high prio */
		continue;
	timerclear(&itp->it_interval);
	if (timeout == (struct timeval *)0)
		timerclear(&itp->it_value);
	else
		itp->it_value = *timeout;
	if (setitimer(ITIMER_REAL, itp, &oitv)  < 0)
		__panic("bad timer");
	ENABLE();	/* no races possible now */
	errno = olderrno;
}

/*
 * Update the expiration of the alarm due to go off so that
 * we accurately locate place and expiration for a new alarm.
 * Note that UNIX clock resolution is typically at the ms, not us
 * level, so we must be careful to assign the min to avoid
 * excess roundup errors.
 */
void
__update(expire)
	struct timeval *expire;	/* when alarm is due to go off */
{
	struct itimerval value;
	register int olderrno;

	olderrno = errno;
	if (getitimer(ITIMER_REAL, &value) < 0)
		__panic("getitimer");
	if (timercmp(&value.it_value, expire, <))
		*expire = value.it_value;
	LWPTRACE(tr_CLOCK,
	  6, ("update %d %d\n", expire->tv_sec, expire->tv_usec));
	errno = olderrno;
}

/*
 * Environment-specific initialization code. Be sure that
 * interrupt memory is used.
 * True interrupt memory must be preallocated since interrupts
 * could interrupt malloc.
 */
void
__init_sig() {
	register int i;

	DefsigType =
	  __allocinit(sizeof (event_t), AGENTMEMORY, IFUNCNULL, TRUE);
	for (i = 0; i < NUMSIG; i++) {
		__Eventinfo[i].int_cookie = DefsigType;
	}
	SigchldType =
	  __allocinit(sizeof (sigchld_t), AGENTMEMORY, IFUNCNULL, TRUE);
	__Eventinfo[SIGCHLD].int_cookie = SigchldType;

	/* No need for interrupt memory for pseudo-events */
	LastRitesType =
	  __allocinit(sizeof (event_t), NUMLASTRITES, IFUNCNULL, FALSE);
	__Eventinfo[LASTRITES].int_cookie = LastRitesType;
}
