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
#include <lwp/monitor.h>

#ifndef lint
SCCSID(@(#) agent.c 1.1 92/07/30 Copyr 1986 Sun Micro);
#endif lint

/*
 * PRIMITIVES contained herein:
 * agt_create(agt, event, memory)
 * agt_enumerate(vec, maxsize)
 * agt_trap(event)
 */

qheader_t __Agents[NUMSIG]; 	/* list of agents set for each signal */
STATIC int SigRefCnt[NUMSIG];	/* number of agents for each signal */
STATIC int AgtType;		/* cookie for agent descriptor caches */

/* release signals set by an agent (if no other agents need the signals) */
STATIC void
sig_unlock(agt)
	register agent_t *agt;
{
	register int event = agt->agt_signum;

	if (--SigRefCnt[event] == 0) {	/* turn off signals */
		__disable(event);
		LWPTRACE(tr_AGENT, 2, ("Disabling %d\n", event));
	}
	LWPTRACE(tr_AGENT, 2, ("-SigRefCnt[%d] %d\n", event, SigRefCnt[event]));
}

/*
 * Remove an agent from existence.
 * Unblock agent owner if necessary (lwp blocked on a receive from the agent
 * if a different process destroys the agent) and remove the agent itself.
 * Note that an undelivered agent message may still be on the owner's
 * message queue.
 * AgtTypes are allocated at do_agent time but NOT interrupt time so LOCK
 * protects this type of memory.
 */
STATIC void
rem_agent(agt)
	register agent_t *agt;
{
	register int event = agt->agt_signum;
	register event_t *ev = agt->agt_event;

	/* Free any process waiting on this agent by searching ReceiveQ. */
	__rem_corpse(&__ReceiveQ, (caddr_t)agt, LE_NONEXIST);

	/* Free the agent itself */
	agt->agt_type = BADOBJECT_TYPE;
	REMX_ITEM(&__Agents[event], agt);
	RECLAIM(ev);
	FREECHUNK(AgtType, agt);
}

/*
 * agt_create() -- PRIMITIVE.
 * Create an agent for the current process.
 * An agent is owned by the lwp that creates it.
 * The agent will catch all signals matching "event"
 * and map them into messages to the lwp.
 * A lwp will be unable to get the next message from an agent until
 * it replies to the previous agent message. Thus, an agent can be conceived of
 * as having the same semantics as another lwp sending messages.
 */
int
agt_create(agt, event, memory)
	thread_t *agt;			/* id of agent being created */
	int event;			/* signal being handled */
	caddr_t memory;			/* place for agent to store message */
{
	register agent_t *desc;

	LWPINIT();
	CLR_ERROR();
	LOCK();
	ERROR((event == ALARMSIGNAL), LE_INVALIDARG);
	ERROR((__Eventinfo[event].int_start == TRAPEVENT), LE_INUSE);
	GETCHUNK((agent_t *), desc, AgtType);
	desc->agt_replied = TRUE;	/* can deliver first message */
	desc->agt_signum = event;
	desc->agt_type = AGENT_TYPE;
	desc->agt_dest = __Curproc;
	desc->agt_event = EVENTNULL;
	desc->agt_memory = memory;
	INIT_QUEUE(&desc->agt_msgs);
	if (SigRefCnt[event]++ == 0) {
		__enable(event);
	}
	LWPTRACE(tr_SELECT, 2, ("+SigRefCnt[%d] %d\n", event, SigRefCnt[event]));
	INS_QUEUE(&__Agents[event], desc);
	agt->thread_id = (caddr_t)desc;
	agt->thread_key = desc->agt_lock = UNIQUEID();
	UNLOCK();
	return (0);
}

/*
 * agt_trap -- PRIMITIVE.
 * map event into trap, not message
 */
int
agt_trap(event)
	int event;
{
	LWPINIT();
	LOCK();
	ERROR((event >= LASTRITES), LE_INVALIDARG);
	ERROR((SigRefCnt[event] != 0), LE_INUSE);
	if (__Eventinfo[event].int_start == TRAPEVENT) {
		UNLOCK();
		return (0);
	}
	__Eventinfo[event].int_start = TRAPEVENT;
	__enable(event);
	UNLOCK();
	return (0);
}

/*
 * agt_destroy().
 * Destroy an individual agent.
 */
int
__agt_destroy(agt, key)
	register agent_t *agt;
	int key;
{
	register msg_t *item;
	register event_t *ev;

	CLR_ERROR();
	LOCK();
	ERROR((agt->agt_lock != key), LE_NONEXIST);
	LWPTRACE(tr_SELECT, 2, ("destroy disabling %x\n", (int)agt));
	sig_unlock(agt);
	while (!ISEMPTYQ(&agt->agt_msgs)) {
		REM_QUEUE((msg_t *), &agt->agt_msgs, item);
		ev = (event_t *)item->msg_argbuf;
		RECLAIM(ev);
		FREECHUNK(__AgtmsgType, item);
	}
	rem_agent(agt);
	YIELD_CPU(__HighPrio);
	return (0);
}

/*
 * agt_enumerate() -- PRIMITIVE.
 * Fill in list with all agent id's.
 * Return total number of these agents.
 * MIN(maxsize, agt_enumerate()) pids will be filled in.
 */
int
agt_enumerate(vec, maxsize)
	thread_t vec[];	/* array to be filled in with agt ids */
	int maxsize;	/* size of vec */
{
	register agent_t *agt;
	register int i = 0;
	register int k;

#define ADDLIST(a) {                                    \
	if (i < maxsize) {                              \
		vec[i].thread_id = (caddr_t)a;          \
		vec[i].thread_key = a->agt_lock;        \
	}                                               \
	i++;                                            \
}

	LWPINIT();
	CLR_ERROR();
	LOCK();
	for (k = 0; k < NUMSIG; k++) {
		for (agt = (agent_t *)FIRSTQ(&__Agents[k]); agt != AGENTNULL;
		  agt = agt->agt_next) {
			ADDLIST(agt);
		}
	}
	UNLOCK();
	return (i);
#undef ADDLIST
}

/*
 * agt_msgs() -- UNDOCUMENTED.
 * Fill in list with messages pending for a given agent.
 * Return total number of these messages.
 * MIN(maxsize, # msgs) messages (tag addresses) will be filled in.
 * N.B. This primitive is for debugging only and is not documented as
 * an available facility.
 */
int
agt_msgs(agt, vec, maxsize)
	thread_t agt;		/* agent being examined */
	char *vec[];		/* vector being filled in with tags */
	int maxsize;		/* size of vec */
{
	register agent_t *aid = (agent_t *)(agt.thread_id);
	register int i = 0;
	register msg_t *m;

#define ADDLIST(m) {if (i < maxsize) vec[i] = (caddr_t)m; i++; }

	LWPINIT();
	CLR_ERROR();
	LOCK();
	for (m = (msg_t *)FIRSTQ(&aid->agt_msgs);
	  m != MESSAGENULL; m = m->msg_next) {
		ADDLIST((caddr_t)m);
	}
	UNLOCK();
	return (i);

#undef ADDLIST
}

/*
 * deliver next message from this agent to a process and
 * free the memory from the previous agent message.
 */
int
__reply_agent(agt)
	register agent_t *agt;	/* agent being replied to */
{
	register caddr_t item;
	register msg_t *newmsg;
	register event_t *ev;

	if ((agt == AGENTNULL) || (TYPE(agt) != AGENT_TYPE)) {
		SET_ERROR(__Curproc, LE_NONEXIST);
		return (-1);
	}
	LWPTRACE(tr_AGENT, 1, ("free agent %x %x\n", (int)&agt->agt_msgs, (int)agt));
	ev = agt->agt_event;
	RECLAIM(ev);
	agt->agt_replied = TRUE;	/* enable next message to be received */
	REM_QUEUE((caddr_t), &agt->agt_msgs, item);/* remove now-replied-to msg */
	LWPTRACE(tr_AGENT, 1, ("free agent item %x\n", item));

	/*
	 * attempt to reply when no message is there.
	 * Can happen, e.g., if msg_recv times out and doesn't
	 * check for error
	 */
	if (item == CHARNULL) {
		SET_ERROR(__Curproc, LE_NOWAIT);
		return (-1);
	}
	FREECHUNK(__AgtmsgType, item);
	REM_QUEUE((msg_t *), &agt->agt_msgs, newmsg);
	if (newmsg != MESSAGENULL) {
		ev = (event_t *)newmsg->msg_argbuf;
		return (__deliveragt(newmsg, agt, ev));
	}
	agt->agt_event = EVENTNULL;
	return (0);
}

/*
 * do the work of delivering an agent message
 */
int
__deliveragt(newmsg, agt, ev)
	register msg_t *newmsg;	/* agent message info */
	register agent_t *agt;	/* agent sending message */
	register event_t *ev;	/* event being delivered */
{
	newmsg->msg_argbuf = agt->agt_memory;
	bcopy((caddr_t)&ev->event_info, newmsg->msg_argbuf,
	  (u_int)newmsg->msg_argsize);
	agt->agt_event = ev;
	agt->agt_replied = FALSE;
	return (__do_send((caddr_t)agt, agt->agt_dest, newmsg));
}

/* called when a process dies to clean up after its agents. */
void
__agentcleanup(corpse)
	lwp_t *corpse;	/* the dying process */
{
	register int i;
	register agent_t *agtq;
	register agent_t *agt;
	register msg_t *m;
	msg_t *tmp;
	event_t *ev;

	/*
	 * search message queue for an agent message (can be at most 1 per agent).
	 * Must do before call to rem_agent (which will make TYPE test fail).
	 * rem_agent will remove any unreplied-to event.
	 */
	for (m = (msg_t *)FIRSTQ(&corpse->lwp_messages); m != MESSAGENULL;) {
		if (TYPE(m->msg_sender.thread_id) == AGENT_TYPE) {
			tmp = m->msg_next;
			REMX_ITEM(&corpse->lwp_messages, m);
			FREECHUNK(__AgtmsgType, m);
			m = tmp;
		} else
			m = m->msg_next;
	}

	for (i = 0; i < NUMSIG; i++) {
		for (agtq = (agent_t *)FIRSTQ(&__Agents[i]); agtq != AGENTNULL;) {

			/*
			 * need temp as rem_agent can nail the queue header
			 * and screw up the agtq=agtq->agt_next if it were
			 * calculated at the end of the loop.
			 */
			agt = agtq;
			agtq = agtq->agt_next;
			if (agt->agt_dest == corpse) {
				while (!ISEMPTYQ(&agt->agt_msgs)) {
					REM_QUEUE((msg_t *), &agt->agt_msgs, m);
					ev = (event_t *)m->msg_argbuf;
					RECLAIM(ev);
					FREECHUNK(__AgtmsgType, m);
				}
				sig_unlock(agt);
				rem_agent(agt);
				LWPTRACE(tr_AGENT, 1, ("kill agt %x\n", (int)agt));
			}
		}
	}
}

/* initialize all agent code */
void
__init_agent()
{
	register int i;

	for (i = 0; i < NUMSIG; i++) {
		INIT_QUEUE(&__Agents[i]);
		SigRefCnt[i] = 0;
	}
	__enable(ALARMSIGNAL);
	AgtType = __allocinit(sizeof (agent_t), NUMAGENTS, IFUNCNULL, FALSE);
}
