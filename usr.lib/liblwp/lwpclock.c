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
/*
 * Clock manipulation routines.
 * Because an alarm can cause manipulation of nugget data structures
 * via wakeups, it is impractical to implement the clock as a lwp
 * with an agent for ALARMSIGNAL.
 */

#include <lwp/common.h>
#ifndef lint
SCCSID(@(#) lwpclock.c 1.1 92/07/30 Copyr 1986 Sun Micro);
#endif lint

#include <lwp/queue.h>
#include <lwp/asynch.h>
#include <machlwp/machsig.h>
#include <machlwp/machdep.h>
#include <lwp/queue.h>
#include <lwp/cntxt.h>
#include <lwp/lwperror.h>
#include <lwp/message.h>
#include <lwp/process.h>
#include <lwp/alloc.h>
#include <lwp/clock.h>
#include <lwp/monitor.h>

STATIC alarm_t *Pending;		/* first pending alarm */
STATIC int AlarmType;			/* cookie for alarm cache */
static struct timeval poll = {0, 0};
struct timeval *__Poll = &poll;		/* for polling */

/*
 * Add two timeval structs:  t2 += t1;
 */
STATIC void
add_time(t1, t2)
	register struct timeval *t1, *t2;
{
	t2->tv_sec += t1->tv_sec;
	if ((t2->tv_usec += t1->tv_usec) >= 1000000L) {
		t2->tv_sec++;
		t2->tv_usec -= 1000000L;
	}
}

/*
 * Subtract two timeval's:  t2 -= t1;
 * It is guaranteed that t2 >= t1.
 */
STATIC void
subtract_time(t1, t2)
	register struct timeval *t1, *t2;
{
	if (t2->tv_usec < t1->tv_usec) {
		t2->tv_sec = t2->tv_sec - t1->tv_sec - 1;
		t2->tv_usec = t2->tv_usec + 1000000L - t1->tv_usec;
	} else {
		t2->tv_sec -= t1->tv_sec;
		t2->tv_usec -= t1->tv_usec;
	}
}

/*
 * This routine is used to make all alarms equivalent modulo
 * the clock resolution.
 * Thus, two alarms 1/100 quantum apart expire at the same time
 * as opposed to having the first expiring, and then setting
 * an alarm for the next alarm which is due at a time far less than
 * the clock resolution. Even if such an alarm expires immediately,
 * we still save the overhead of the system call and interrupt.
 */
STATIC void
roundup_timer(t)
	register struct timeval *t;
{
	if ((t->tv_usec % CLOCKRES) == 0)
		return;
	t->tv_usec = ((t->tv_usec / CLOCKRES) + 1) * CLOCKRES;
	if (t->tv_usec >= 1000000L) {
		t->tv_sec += 1;
		t->tv_usec -= 1000000L;
	}
}

/*
 * Called when an alarm (ALARMSIGNAL) goes off, indicating that
 * the first guy in alarms is due to expire.
 * Must be called either while LOCKED or DISABLED.
 * Ok for proc to call setalarm, etc. as the queue is consistent before the call.
 */
void
__timesup() {
	register alarm_t *tocall;

	ASSERT(IS_LOCKED() || DISABLED());
	if (Pending != CLOCKNULL)
		timerclear(&Pending->alarm_tv);
	while ((Pending != CLOCKNULL) && !timerisset(&Pending->alarm_tv)) {
		tocall = Pending;		/* alarm going off now */
		Pending = tocall->alarm_next;

		/* proc must return */
		(*tocall->alarm_proc) (tocall->alarm_arg);
		FREECHUNK(AlarmType, tocall);
	}
	if (Pending != CLOCKNULL) {
		__setclock(&Pending->alarm_tv);
	} else {
		__setclock((struct timeval *)0);
	}
}

/*
 * Set an alarm to call proc in "interval" ticks. "interval" is >0. 
 * The alarm is identified by proc and arg. Duplicates are not removed.
 * We always make the new alarm's expiration time
 * relative only to the node in front. The effect of adding a new
 * alarm is thus localized to altering one node's expiration time.
 * Note: the internal clock resolution is 1/100 second.
 * Immediate alarms (0 timeout) are not allowed.
 * We copy the delay to avoid side effects.
 */
int
__setalarm(delay, proc, arg)
	struct timeval *delay;
	void (*proc)();
	int arg;
{
	register alarm_t *current;	
	register alarm_t *apt;
	register alarm_t *prev;
	struct timeval time;
	register struct timeval *interval = &time;

	ASSERT(IS_LOCKED());
	time.tv_sec = delay->tv_sec;
	time.tv_usec = delay->tv_usec;
	if (interval->tv_sec < 0 ||
	    ((interval->tv_sec == 0) && (interval->tv_usec <= 0))) {
		return (-1); /* bad timeval */
	}
	roundup_timer(interval); /* don't resolve finer than system clock does */
	if (Pending != CLOCKNULL) { /* make Pending accurate as of NOW. */
		__update(&Pending->alarm_tv);
	}

	/* for pending alarms due to go off before me */
	for (current = Pending, prev = CLOCKNULL;
		(current != CLOCKNULL) &&
		timercmp(&current->alarm_tv, interval, <);
		prev = current, current = current->alarm_next) {

		/*
		 * find interval relative to guy in front .
		 * interval -= &current->alarm_tv;
		 */
		subtract_time(&current->alarm_tv, interval);
	}

	/*
	 * Update the guy I butted in front of.
	 * In some cases, this may become the clock resolution.
	 * Suppose a short timer is set frequently and a long timer
	 * has been set in the past. The long timer is not affected
	 * by the update code above because the new timer is set immediately.
	 * Thus, the long timer is decremented in "interval" units.
	 */
	if (current != CLOCKNULL) {
		subtract_time(interval, &current->alarm_tv);
		LWPTRACE(tr_CLOCK, 2, ("clock %d, %d\n", current->alarm_tv.tv_sec,
		    current->alarm_tv.tv_usec));
	}
	GETCHUNK((alarm_t *), apt, AlarmType);
	apt->alarm_tv = *interval;
	apt->alarm_proc = proc;
	apt->alarm_arg = arg;
	apt->alarm_next = current;

	/* new alarm will be the next to occur */
	if (prev == CLOCKNULL) {
		__setclock(interval);
		Pending = apt;
	} else {
		prev->alarm_next = apt;
	}
	return (0);
}

/*
 * Cancel an alarm. If not found, return FALSE else return TRUE
 * The implementation could be made simpler by using a well-known
 * proc to represent a cancelled alarm instead of actually
 * removing it, but this increases the number of interrupts.
 */
int
__cancelalarm(proc, arg)
	void (*proc)();
	int arg;
{
	register alarm_t *current;
	register alarm_t *foll;
	register alarm_t *prev;

	current = Pending;
	prev = CLOCKNULL;

	/* make 1st alarm accurate in case I delete 2nd guy */
	if (Pending != CLOCKNULL) { /* make expiration accurate as of NOW */
		__update(&Pending->alarm_tv);
	}
	while (current != CLOCKNULL) {
		foll = current->alarm_next;
		if ((current->alarm_proc == proc) &&
		  (arg == current->alarm_arg)) {
			if (current == Pending) { /* Remove first alarm */
				if (foll == CLOCKNULL) {
					/* No more pending */
					__setclock((struct timeval *)0);
				} else { /* Update the 2nd guy */
					add_time(&current->alarm_tv,
					    &foll->alarm_tv);
					__setclock(&foll->alarm_tv);
				}
			} else if (foll != CLOCKNULL)
				/* Remove a middle alarm */
				add_time(&current->alarm_tv, &foll->alarm_tv);
			if (prev == CLOCKNULL)
				Pending = foll;
			else
				prev->alarm_next = foll;
			FREECHUNK(AlarmType, current);
			return (TRUE);
		} /* Found the guy to delete */
		prev = current;
		current = foll;
	}
	return (FALSE);
}

/* initialize clock code */
void
__init_clock() {

	__setclock((struct timeval *)0);
	Pending = CLOCKNULL;
	AlarmType = __allocinit(sizeof (alarm_t), MAXALARMS, IFUNCNULL, FALSE);
}
