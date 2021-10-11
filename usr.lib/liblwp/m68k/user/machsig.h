/* @(#) machsig.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */
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
#ifndef _lwp_machsig_h
#define _lwp_machsig_h

#ifndef STAND

#ifndef _lwp_wait
#define _lwp_wait
#include <sys/wait.h>
#endif _lwp_wait

#include <sys/time.h>
#include <sys/resource.h>

#endif STAND

#include <signal.h>

/* the event which corresponds to a clock tick */
#define ALARMSIGNAL	SIGALRM

/* resolution of clock in us */
#define	CLOCKRES	10000

/* Number of events: add 1 for LASTRITES "soft" events */
#define	NUMSIG		(NSIG+1)

typedef enum sigdstat_t {
	SIG_ERROR	=0,	/* error in processing event */
	SIG_CONT	=1,	/* try to get more events from this signal */
	SIG_DONE	=2,	/* no more events from this signal */
} sigdstat_t;
#define	TRAPEVENT	((sigdstat_t (*)()) 0)	/* trap (vs. interrupt) */

typedef struct sigdesc_t {
	sigdstat_t (*int_start)();	/* interrupt info gathering routine */
	int int_cookie;			/* info descriptor size */
} sigdesc_t;

/*
 * Interrupt info storage. Each MUST have an event_t as the
 * first element.
 */

/*
 * storage for SIGCHLD events
 */
typedef struct sigchld_t {
	struct event_t sigchld_event;		/* must be first */
	int sigchld_pid;			/* pid of process that changed */
	union wait sigchld_status;		/* status from wait3 */
	struct rusage sigchld_rusage;		/* rusage from wait3 */
} sigchld_t;

extern void __init_sig();
extern sigdesc_t __Eventinfo[];
extern void __setclock(/* timeout */);
extern void __update(/* expire */);

/*
 * Decrement the reference count of an event and free it if no longer in use
 * by an agent. Must be done while disasbled since event memory is allocated
 * at interrupt time. (AGTMSG memory may be allocated while DISABLED but NOT at
 * interrupt time, so LOCK can protect it).
 */
#define	RECLAIM(ev) {								\
	DISABLE();								\
	if (((ev) != EVENTNULL) && (--(ev)->event_refcnt == 0))			\
		FREECHUNK(__Eventinfo[(ev)->event_signum].int_cookie, (ev));	\
	ENABLE();								\
}

#endif /*!_lwp_machsig_h*/
