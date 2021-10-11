/* Copyright (C) 1986 Sun Microsystems Inc. */

#include <lwp/common.h>
#include <lwp/queue.h>
#include <lwp/asynch.h>
#include <machlwp/machsig.h>
#include <machlwp/machdep.h>
#include <lwp/cntxt.h>
#include <lwp/lwperror.h>
#include <lwp/message.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/agent.h>
#include <lwp/alloc.h>
#include <lwp/monitor.h>
#include <lwp/clock.h>
#ifndef lint
SCCSID(@(#) machsig.c 1.1 92/07/30);
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

#ifndef KERNEL
STATIC int DefsigType;		/* cookie for default event caches */
STATIC int LastRitesType;	/* cookie for LASTRITES events */
#endif KERNEL

extern sigdstat_t __sigdefault();

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
	__sigdefault,	0,	/* LASTRITES	0 */
	__sigdefault,	0,	/* ??		1 */
};

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
 * enable interrupts for a particular event
 */
/* ARGSUSED */
int
__enable(event)
	int event;
{
#ifdef lint
	event = event;
#endif lint
	/* XXX */
}

/*
 * Reset signal handler (in UNIX, to default action; in kernel, disable
 * device interrupts for the specified device).
 * This is called when the last agent for an interrupt goes away.
 */
/* ARGSUSED */
int
__disable(event)
	int event;
{
	/* XXX */
}

/*
 * Environment-specific initialization code. Be sure that
 * interrupt memory is used.
 * True interrupt memory must be preallocated since interrupts
 * could interrupt malloc.
 */
void
__init_sig() {
#ifndef KERNEL

	register int i;
	DefsigType =
	  __allocinit(sizeof (event_t), AGENTMEMORY, IFUNCNULL, TRUE);
	for (i = 0; i < NUMSIG; i++) {
		__Eventinfo[i].int_cookie = DefsigType;
	}

	/* No need for interrupt memory for pseudo-events */
	LastRitesType =
	  __allocinit(sizeof (event_t), NUMLASTRITES, IFUNCNULL, FALSE);
	__Eventinfo[LASTRITES].int_cookie = LastRitesType;
#endif KERNEL
}
