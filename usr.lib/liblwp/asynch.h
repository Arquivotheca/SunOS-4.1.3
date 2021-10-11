/* @(#) asynch.h 1.1 92/07/30 Copyr 1987 Sun Micro */
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
#ifndef _lwp_asynch_h
#define _lwp_asynch_h

/*
 * eventinfo_t's (see lwp.h) contain information about an interrupt as
 * the header for each interrupt type.
 * Following the last field of an event_t will be the
 * specific data peculiar to a given interrupt (e.g., character from a tty
 * interrupt). The address of event_info will be passed to the client
 * as the message buffer.
 * The event is reference-counted for each agent that uses it
 * (several agents can catch one event).
 * When the message is replied to, the reference count is
 * adjusted, and the event (see asynch.h) is freed if necessary.
 */

typedef struct event_t {
	__queue_t event_next;		/* next event in queue (must be 1st) */
	int event_refcnt;		/* number of agents using this info */
	struct eventinfo_t event_info;	/* identity of interrupt */
} event_t;
#define	event_signum	event_info.eventinfo_signum
#define event_code	event_info.eventinfo_code
#define event_addr	event_info.eventinfo_addr
#define	event_victimid	event_info.eventinfo_victimid
#define	EVENTEXCESS	(sizeof (event_t) - sizeof (eventinfo_t))
#define	EVENTNULL ((event_t *)0)

extern int __SigLocked;			/* 0 iff interrupts are disabled */
extern volatile int __AsynchLock;	/* non-zero iff in a critical section */
extern bool_t __EventsPending;	/* TRUE iff unprocessed events exist */
extern int __Omask;		/* Mask for saving/restoring previous mask */

extern void	__save_event(/*victim, signum, code, addr*/);
extern caddr_t	__rem_event(/*signum*/);
extern void	__fake_sig();
extern void	__do_agent();
extern void	__init_asynch();
extern int	__AgtmsgType;

/*
 * Lock operations for nugget critical sections.
 * These nest (LOCK, LOCK, UNLOCK, UNLOCK has no race).
 * However, it is safe to YIELD to somebody when locked. When a process
 * YIELD's, we resume in an UNLOCKED state (locked status is not part of
 * lwp state). The basic way these work is that if an interrupt occurs while
 * __AsynchLock is non-zero, the interrupt info is saved away but nothing
 * else (affecting global state) is done.
 * At the end of the critical section, we check
 * EventsPending. If any unprocessed events exist, we
 * lock out interrupts and process them. If an interrupt
 * occurs after we check EventsPending and find it FALSE, there
 * is no race because we simply process the interrupt (AsynchLock is cleared
 * prior to testing EventsPending).
 * Note that there is almost no overhead if (as should be often the case)
 * there are no pending events.
 */
#define	LOCK()		{__AsynchLock++;}
#define	UNLOCK() 	{ if (__AsynchLock == 1) __unlock(); else __AsynchLock--;}
#define	IS_LOCKED()	(__AsynchLock != 0)	/* must not affect registers */

/*
 * Interrupt enabling-disabling mechanisms.
 * Not used much; we use LOCK-UNLOCK for critical sections.
 * Note that __SigLocked is guaranteed to be == 1 just after an interrupt occurs
 * because we set things up to prohibit nested interrupts.
 * Also note that __SigLocked is never set unless interrupts are disabled:
 *	a) it's set after a real interrupt arrives
 *	b) it's set if DISABLING and it's previously 0
 * if (__SigLocked++ == 0) mask(); has a race in that an interrupt arriving
 * after the increment resets __SigLocked to 0 upon rti.
 */
#define	DISABLE()							\
		{							\
			if (__SigLocked == 0) {				\
				__Omask = sigsetmask(-1);		\
				__SigLocked++;				\
			} else {					\
				__SigLocked++;				\
			}						\
		}
#define	ENABLE()	{if (--__SigLocked == 0) (void) sigsetmask(__Omask); }
#define ENABLEALL()	{__SigLocked = 1; __Omask = 0; ENABLE(); }

/* Force all software lock control off */
#define	LOCK_OFF()	{__AsynchLock = 0; __SigLocked = 0;}
#define	DISABLED()	(__SigLocked != 0)

#endif /*!_lwp_asynch_h*/
