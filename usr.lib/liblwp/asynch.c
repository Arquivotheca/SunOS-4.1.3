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
#include <lwp/clock.h>
#include <lwp/monitor.h>
#ifndef lint
SCCSID(@(#) asynch.c 1.1 92/07/30 Copyr 1986 Sun Micro);
#endif lint

volatile int __AsynchLock;	/* non-zero iff in critical section */
int __SigLocked;		/* non-zero iff interrupts masked */
bool_t __EventsPending;		/* TRUE iff unprocessed interrupts exist */
int __Omask;			/* mask for saving/restoring interrupt mask */
STATIC qheader_t EventList;	/* list of unprocessed events */
int __AgtmsgType;		/* cookie for agent messages cache */

/*
 * Called when an interrupt occurred to save away event info.
 */
void
__save_event(victim, signum, code, addr)
	lwp_t *victim;		/* thread running at time of event */
	int signum;		/* signal identity */
	int code;		/* any code associated with the signal */
	caddr_t addr;		/* address of fault */
{
	register event_t *ev;
	register sigdesc_t *siginfo = &__Eventinfo[signum];
	register sigdstat_t status;

	__EventsPending = TRUE;	/* There is a signal to be dealt with */

	for (;;) {
		DISABLE();
		GETCHUNK((event_t *), ev, siginfo->int_cookie);
		ENABLE();
		if (ev == EVENTNULL)	/* overflow -- we lost an event */
			break;

		/* get volatile device state */
		status = siginfo->int_start(ev);
		if (status == SIG_ERROR) {
			DISABLE();
			FREECHUNK(siginfo->int_cookie, ev);
			ENABLE();
			return;
		}

		ev->event_signum = signum;
		ev->event_victimid.thread_id = (caddr_t)victim;
		ev->event_victimid.thread_key = (victim ? victim->lwp_lock : 0);
		ev->event_refcnt = 0;
		ev->event_code = code;
		ev->event_addr = addr;
		INS_QUEUE(&EventList, ev);
		if (status == SIG_DONE)
			break;
		/* else, may be able to extract another message (SIG_CONT) */
	}
}

/*
 * Remove an event (must be called at high priority (DISABLE, not LOCK)).
 * Needed when cancelling alarms. At most one event (matching the signal)
 * is removed.
 */
caddr_t
__rem_event(signum)
	register int signum;
{
	register int cmp;
	register __queue_t *q = EventList.q_head;
	register __queue_t *prev = QNULL;

	ASSERT(DISABLED());
	while (q != QNULL) {
		cmp = ((event_t *)q)->event_signum;
		if (cmp == signum)
			break;
		prev = q;
		q = q->q_next;
	}
	if (q == QNULL)
		return (CHARNULL);
	if (prev == QNULL) {	/* first guy in queue */
		EventList.q_head = q->q_next;
	} else {
		prev->q_next = q->q_next;
	}
	if (EventList.q_tail == q)
		EventList.q_tail = prev;
	FREECHUNK(__Eventinfo[signum].int_cookie, q);
	return ((caddr_t)q);
}

/*
 * Called when about to return from a normal context switch (see low.s)
 * (AFTER the general registers have been restored) if
 * events have happened while the process was switching.
 * At this point, interrupts are enabled but the context of
 * the process accurately reflects what state is to
 * be restored. We do not set __AsynchLocked to be 0 (in CSWTCH()) until
 * we guarantee that the current state can be accurately saved
 * in case of a real interrupt.  Also called from UNLOCK.
 * fake_sig is always called while LOCKED and operates on the nugget stack.
 */
void
__fake_sig()
{
	/*
	 * make it look like a real signal (i.e., non-interruptable) so
	 * we can call __do_agent transparently.
	 * Also, guarantee that we return via full_swtch since
	 * fake_sig can't return to the caller.
	 * Kernel lastrites currently only agent, but no full switch yet.
	 */
	DISABLE();
	ASSERT(IS_LOCKED());

	/*
	 * it's possible there's no events to process (e.g., interrupt
	 * after clearing AsynchLock and before setting it again).
	 * However, if we just return, there would be a race
	 * on EventsPending since we will have to UNLOCK
	 * before the final rts
	 */

	LWPTRACE(tr_ASYNCH, 3, ("fake sig calling agent\n"));
	__do_agent();
	/* NOTREACHED */
}

/*
 * Map signals (from EventList) to messages. Because 1 signal may cause >1
 * message, need to allocate separate messasge headers that point into
 * the event descriptor.
 * Invoked at high prio always.
 * For each saved event, make it appear that the message was sent by the
 * agent (agentid == sender).
 * For UNIX agents, there is a problem with signals
 * such as SIGIO in that a SIGIO can be generated for a descriptor, but
 * further IO comes along before the descriptor is read and the
 * resultant SIGIO gets "folded" into the previous one, resulting in
 * fewer signals than expected.
 * Thus, clients should guarantee read fails before sleeping on a SIGIO agent.
 * As a result, spurious agent messages may be generated and the client
 * should be prepared to deal with this (by ignoring reads that return nothing).
 * In the kernel, DISABLE is cheap, so as long as we are LOCKED, it
 * is safe to allow interrupts. We need to remain LOCKED to avoid
 * reentering do_agent before it completes (we don't have infinite stack space).
 * We also must DISABLE before scheduling since deciding that there are
 * no more events to process is a critical section.
 * The event queue must be protected because it is accessed at interrupt time.
 */

void
__do_agent() {
	register agent_t *agnt;
	register event_t *ev;
	register msg_t *amesg;
	register int sig;

	ASSERT(DISABLED());
	LWPTRACE(tr_SELECT, 2, ("__do_agent\n"));
	for(;;) {
		if (ISEMPTYQ(&EventList)) {
			break;
		}
		REM_QUEUE((event_t *), &EventList, ev);
		sig = ev->event_signum;
		LWPTRACE(tr_SELECT, 1, ("processing signal %d\n", sig));
		if (sig == ALARMSIGNAL) { /* process timeouts */
			FREECHUNK(__Eventinfo[sig].int_cookie, ev);
			(void) __timesup();
			continue;
		}

		if (ISEMPTYQ(&__Agents[sig])) {
			FREECHUNK(__Eventinfo[sig].int_cookie, ev);
			continue;
		}

		for (agnt = (agent_t *)FIRSTQ(&__Agents[sig]); agnt != AGENTNULL;
		  agnt = agnt->agt_next) {
			GETCHUNK((msg_t *), amesg, __AgtmsgType);
			LWPTRACE(tr_ASYNCH, 1, ("agt allocated %x\n", (int)amesg));
			ev->event_refcnt++;
			amesg->msg_argsize =
			  MEMSIZE(__Eventinfo[sig].int_cookie) - EVENTEXCESS;
			amesg->msg_ressize = 0;
			amesg->msg_resbuf = CHARNULL;
			amesg->msg_sender.thread_id = (caddr_t)agnt;
			amesg->msg_sender.thread_key = agnt->agt_lock;
			LWPTRACE(tr_ASYNCH,
			  3, ("agt snd to %x\n", agnt->agt_dest));
			if (agnt->agt_replied) { /* queue msg iff replied to */
				LWPTRACE(tr_ASYNCH, 1, ("immed agt %x %x\n",
				  &agnt->agt_msgs, amesg));
				if (__deliveragt(amesg, agnt, ev) == -1) {
					LWPTRACE(tr_ASYNCH,
					  4, ("agt killed\n"));
					FREECHUNK(__AgtmsgType, amesg);
				}
			} else { /* queue for next msg. upon reply */

				/*
				 * adjust later to point to info upon delivery.
				 * Meanwhile, keep track of the event.
				 */
				amesg->msg_argbuf = (caddr_t)ev;
				LWPTRACE(tr_ASYNCH,
				  1, ("delayed agt msg %x %x\n",
				  &agnt->agt_msgs, amesg));
				INS_QUEUE(&agnt->agt_msgs, amesg);
			}
		} /* for */
	 } /* while */

	 /*
	  * May not be strictly necessary to __schedule() as lower-priority
	  * process than the current one for example could have been enabled.
	  * However, we didn't know this at the time the signal came in
	  * so we preserved all the state anyway.
	  * Imperative that interrupts are disabled from the time
	  * we decided that there were no more events to the time
	  * we complete the context switch via schedule().
	  */
	
	ASSERT(DISABLED());

	 __EventsPending = FALSE;	/* must be set while disabled
					 * otherwise, full_swtch can find events,
					 * call do_agent which can schedule,
					 * etc. forever, with no stack bound)
					 */

	 __schedule();
}

/* initialize agent code for nugget */
void
__init_asynch() {

	__AsynchLock = 0;
	__SigLocked = 0;
	__EventsPending = FALSE;
	__AgtmsgType =
	  __allocinit(sizeof (msg_t), NUMAGTMSGS, IFUNCNULL, FALSE);
	INIT_QUEUE(&EventList);
}
