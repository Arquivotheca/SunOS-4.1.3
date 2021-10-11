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
SCCSID(@(#) message.c 1.1 92/07/30 Copyr 1986 Sun Micro);
#endif lint

qheader_t __ReceiveQ;			/* list of all blocked receivers */
qheader_t __SendQ;			/* list of all blocked senders */

/*
 * PRIMITIVES contained herein:
 * msg_send(destid, tag)
 * msg_recv(id, timeout)
 * msg_reply(sender)
 * msg_enumsend(vec, maxsize)
 * msg_enumrecv(vec, maxsize)
 */

/* awaken a sleeping process (called from alarm routine only) */
void
__lwp_rwakeup(pid)
	register lwp_t *pid;
{
	REMX_ITEM(&__ReceiveQ, pid);
	UNBLOCK(pid);
	SET_ERROR(pid, LE_TIMEOUT);
}

/*
 * msg_send -- PRIMITIVE.
 * Send a message (described by "tag") to process "destid".
 * Block unconditionally until a reply is sent.
 * The link field in "tag" is not used once the message is
 * received. Since msg_send() is blocking, the client
 * can't do any harm by messing up a tag.
 * XXX should redo message stuff: add to each thread a
 * queue of waiting senders, a queue of senders awaiting a reply,
 * and a queue of receivers. Also, a hashed queue of the universal
 * receiver (msg_recv(ALL, ...)). This eliminates the need for
 * the sender id in the tag, and obviates need to search sendq
 * or receiveq, a big expense if lots of messages in the system.
 * It also allows more accurate reporting of state (e.g., received, not replied).
 */
int
msg_send(dest, arg, argsize, res, ressize)
	thread_t dest;		/* destination thread */
	caddr_t arg;		/* argument buffer */
	int argsize;		/* size in bytes of argument buffer */
	caddr_t res;		/* result buffer */
	int ressize;		/* size in bytes of result buffer */
{
	register lwp_t *destid = (lwp_t *)dest.thread_id;
	register int err;
	register lwp_t *me = __Curproc;

	CLR_ERROR();
	LOCK();
	ERROR((dest.thread_key != destid->lwp_lock), LE_NONEXIST);
	me->lwp_arg = arg;
	me->lwp_res = res;
	me->lwp_argsize = argsize;
	me->lwp_ressize = ressize;
	err = __do_send((caddr_t)me, destid, &me->lwp_tag);
	if (err != 0) {
		UNLOCK();
		return (-1);
	}
	BLOCK(__Curproc, RS_SEND, destid);
	INS_QUEUE(&__SendQ, __Curproc);
	WAIT();
	if (IS_ERROR())
		return (-1);
	return (0);
}

/*
 * Remove first message that contains a matching sender id.
 * If a thread dies, it removes all unreceived messages (or remains
 * blocked until an unreplied message is replied to) so
 * no need to match lock and key.
 */
#define	REM_MSG(queue, id, val)					\
{								\
	register __queue_t *q = FIRSTQ(queue);			\
	register __queue_t *prev = QNULL;				\
								\
	while (q != QNULL) {					\
		if (id == (((msg_t *)q)->msg_sender.thread_id))	\
			break;					\
		prev = q;					\
		q = q->q_next;					\
	}							\
	if (q != QNULL) {					\
		if (prev == QNULL) {				\
			(queue)->q_head = q->q_next;		\
		} else {					\
			prev->q_next = q->q_next;		\
		}						\
		if ((queue)->q_tail == q)			\
			(queue)->q_tail = prev;			\
	}							\
	val = ((msg_t *)q);					\
}

#define	CHECKSENDER(send_id, is_agt, send_key)				\
	ERROR((								\
	    (!is_agt && ((lwp_t *)send_id)->lwp_lock != send_key)	\
	    ||								\
	    (is_agt && ((agent_t *)send_id)->agt_lock != send_key)	\
	  ), LE_NONEXIST);

/*
 * msg_recv -- PRIMITIVE.
 * Rendezvous with a sending process and receive a message
 * from it. If a message is waiting, do not block; otherwise
 * block until a message is sent.
 * If *sender is THREADNULL, a message from any
 * process or agent will be accepted.
 * Otherwise, only a message from the specific agent or process will
 * be accepted. If timeout is not INFINITY, msg_recv() will only
 * block for "timeout" usec.
 * msg_recv() modifies the buffer arguments to indicate the message received.
 */
int
msg_recv(sender, arg, argsize, res, ressize, timeout)
	thread_t *sender;	/* VALUE-RESULT: agt id, process id, THREADNULL */
	register struct timeval *timeout;	/* time to wait for message */
	caddr_t *arg;		/* argument buffer */
	int *argsize;		/* argument size */
	caddr_t *res;		/* result buffer */
	int *ressize;		/* result size */
{
	register msg_t *item;
	register bool_t clockset = (timeout != INFINITY);
	register caddr_t who = sender->thread_id;
	bool_t takeall = (sender->thread_id == CHARNULL);
	bool_t isagent;
	int key = sender->thread_key;
	int lock;

/*
 * Put message back on agent queue to be deallocated later.
 * No new agent messages will be delivered
 * until this message is replied to and dequeued.
 */
#define	FILL_MSG() {								\
	caddr_t from = (item->msg_sender).thread_id;				\
										\
	if (TYPE(from) == AGENT_TYPE) {						\
		FRONT_QUEUE( &(((agent_t *)from)->agt_msgs), item);		\
		lock = ((agent_t *)from)->agt_lock;				\
	} else {								\
		lock = ((lwp_t *)from)->lwp_lock;				\
	}									\
	sender->thread_id = from;						\
	sender->thread_key = lock;						\
	if (arg != CPTRNULL) *arg = item->msg_argbuf;				\
	if (argsize != 0) *argsize = item->msg_argsize;				\
	if (res != CPTRNULL) *res = item->msg_resbuf;				\
	if (ressize != 0) *ressize = item->msg_ressize;				\
}

	CLR_ERROR();
	LOCK();
	ERROR((sender == &__ThreadNull), LE_INVALIDARG);
	if (!takeall) {		/* be selective */
		isagent = (TYPE(who) == AGENT_TYPE);
		CHECKSENDER(who, isagent, key);
		REM_MSG(&__Curproc->lwp_messages, who, item);
	} else {		/* take anything */
		REM_QUEUE((msg_t *), &__Curproc->lwp_messages, item);
	}
	if (item != MESSAGENULL) {	/* found a waiting message */
		LWPTRACE(tr_MESSAGE, 1, ("immed. msg\n"));
		FILL_MSG();
		UNLOCK();
		return (0);
	}
	ERROR((timeout == POLL), LE_TIMEOUT);
	if (clockset && (__setalarm(timeout, __lwp_rwakeup, (int)__Curproc)== -1))
		TERROR(LE_INVALIDARG);
	BLOCK(__Curproc, RS_RECEIVE, sender->thread_id);
	INS_QUEUE(&__ReceiveQ, __Curproc);
	WAIT(); /* return here as result of timeout or message arrival */
	LOCK();
	if (clockset && (__Curproc->lwp_lwperrno != LE_TIMEOUT))
		(void) __cancelalarm(__lwp_rwakeup, (int)__Curproc);
	if (IS_ERROR()) {
		UNLOCK();
		return (-1);
	}
	/* desired message must be first in queue: msg/agt-send sees to it */
	REM_QUEUE((msg_t *), &__Curproc->lwp_messages, item);
	ASSERT(item != MESSAGENULL);
	FILL_MSG();
	UNLOCK();
	return (0);
#undef FILL_MSG()
}

/*
 * msg_reply() -- PRIMITIVE.
 * Reply to (and unblock) the sender.
 * Free agent message memory if an agent is replied to.
 */
int
msg_reply(who)
	thread_t who;
{
	register lwp_t *sender = (lwp_t *)(who.thread_id);
	register int err = 0;
	bool_t isagent = (TYPE(sender) == AGENT_TYPE);
	int key = who.thread_key;

	CLR_ERROR();
	LOCK();
	CHECKSENDER(sender, isagent, key);
	if (isagent) {
		/*
		 * deallocate the message and enable a new message
		 * to be delivered by agent.
		 */
		err = __reply_agent((agent_t *)sender);
		UNLOCK();
		if (err < 0)
			return (-1);
		return (0);
	} else {
		ERROR((sender->lwp_blockedon != (caddr_t)__Curproc),
		  LE_INVALIDARG);
		if (sender->lwp_run == RS_ZOMBIE) {

			/*
			 * sender killed (a zombie), but stack wasn't freed yet,
			 * to avoid having replier trash the possibly-reallocated
			 * stack memory. __freeproc() frees zombie stack.
			 * Must remove from __ZombieQ FIRST (can't be on freelist
			 * and __ZombieQ simultaneously).
			 */
			REMX_ITEM(&__ZombieQ, sender);
			__freeproc(sender);
			TERROR(LE_NONEXIST);
		}
		REMX_ITEM(&__SendQ, sender);
		UNBLOCK(sender);
		YIELD_CPU(sender->lwp_prio);
	}
	return (0);
}

#define	ADDLIST(p) {						\
	if (i < maxsize) {					\
		vec[i].thread_id = (caddr_t)p;			\
		vec[i].thread_key = ((lwp_t *)p)->lwp_lock;	\
	}							\
	i++;							\
}

/*
 * msg_enumsend() -- PRIMITIVE.
 * enumerate all processes blocked on a send awaiting a reply.
 * return the number of processes on the queue.
 * MIN(maxsize, msg_enumsend()) pids will be filled in.
 */
int
msg_enumsend(vec, maxsize)
	thread_t vec[];	/* array to be filled in with tids */
	int maxsize;	/* size of vec */
{
	register __queue_t *pid;
	register int i = 0;

	CLR_ERROR();
	LOCK();
	for (pid = FIRSTQ(&__SendQ); pid != QNULL; pid = pid->q_next)
		ADDLIST(pid);
	UNLOCK();
	return (i);
}

/*
 * msg_enumrecv() -- PRIMITIVE.
 * enumerate all processes blocked on a receive awaiting a send.
 * return the number of processes on the queue.
 * MIN(maxsize, msg_enumrecv()) pids will be filled in.
 */
int
msg_enumrecv(vec, maxsize)
	thread_t vec[];	/* array to be filled in with pids */
	int maxsize;	/* size of vec */
{
	register __queue_t *pid;
	register int i = 0;

	CLR_ERROR();
	LOCK();
	for (pid = FIRSTQ(&__ReceiveQ); pid != QNULL; pid = pid->q_next)
		ADDLIST(pid);
	UNLOCK();
	return (i);
}
#undef ADDLIST

/*
 * msg_status() -- UNDOCUMENTED.
 * List all the messages (tag addresses) pending for a given process.
 * for use in debugging only, not documented (since the messages not
 * yet received are supposedly hidden from the client).
 */
msg_status(tid, vec, maxsize)
	thread_t tid;
	caddr_t vec[];
	int maxsize;
{
	lwp_t *pid = (lwp_t *)(tid.thread_id);
	msg_t *m;
	int i = 0;

#define ADDLIST(m) {if (i < maxsize) vec[i] = (caddr_t)m; i++; }

	for (m = (msg_t*)FIRSTQ(&pid->lwp_messages);
	  m != MESSAGENULL; m = m->msg_next)
		ADDLIST(m);
#undef ADDLIST
}

/*
 * Cause a message (described by "tag") to be enqueued
 * for the process "destid" on behalf of sending process
 * or agent "sender". If the destination is already blocked,
 * then put the message first in the message queue
 * iff it's the expected message and unblock the
 * destination (complete the rendezvous). Otherwise, put the
 * message at the end of the queue.
 */
int
__do_send(sender, dest, tag)
	register caddr_t sender;	/* process or agent sending message */
	register lwp_t *dest;		/* process receiving message */
	msg_t *tag;			/* message descriptor */
{
	if ((dest == LWPNULL) ||
	  (TYPE(dest) != PROCESS_TYPE) || (dest->lwp_run == RS_ZOMBIE)) {
		LWPTRACE(tr_MESSAGE, 1,
		  ("sending to dead process %x\n", (int)dest));
		if (TYPE(sender) == PROCESS_TYPE) {
			SET_ERROR((lwp_t *)sender, LE_NONEXIST);
		} /* else, error from agent, not a process so don't set error */
		return (-1);
	}
	LWPTRACE(tr_MESSAGE, 1, ("sending to %x\n", (int)dest));
	if (sender == (caddr_t)dest) {
		SET_ERROR(__Curproc, LE_INVALIDARG);
		return (-1);
	}
	tag->msg_sender.thread_id = sender; /* msg_recv will fill in lock field */
	if ((dest->lwp_run == RS_RECEIVE) &&
	  ((dest->lwp_blockedon == sender) ||
	    (dest->lwp_blockedon == NOBODY))) {
		LWPTRACE(tr_MESSAGE, 1, ("immediate send\n"));
		FRONT_QUEUE(&dest->lwp_messages, tag);
		REMX_ITEM(&__ReceiveQ, dest);
		UNBLOCK(dest);
	} else { /* the receiver isn't waiting yet, or not the right msg */
		INS_QUEUE(&dest->lwp_messages, tag);
		LWPTRACE(tr_MESSAGE, 1, ("delayed send\n"));
	}
	return (0);
}

/*
 * See if anybody is expecting a message or reply from "corpse".
 * If so, terminate their wait with an error.
 * Called when "corpse" is in the process of dying.
 */
void
__msgcleanup(corpse)
	lwp_t *corpse;
{
	__rem_corpse(&__SendQ, (caddr_t)corpse, LE_NONEXIST);
	__rem_corpse(&__ReceiveQ, (caddr_t)corpse, LE_NONEXIST);
}

/* initialize message queues */
void
__init_msg()
{

	INIT_QUEUE(&__SendQ);
	INIT_QUEUE(&__ReceiveQ);
}
