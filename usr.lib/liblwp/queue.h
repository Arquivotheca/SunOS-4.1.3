/* @(#) queue.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1986. Sun Microsystems, Inc. */
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
#ifndef _lwp_queue_h
#define _lwp_queue_h

/*
 * next-link (MUST BE FIRST in a structure) for singly-linked queues
 * USAGE:
 *	struct foo {
 *		__queue_t next;	(* MUST BE FIRST *)
 *		<other fields>;
 *	};
 */
typedef struct __queue_t {
	struct __queue_t *q_next;	/* hack since recursive typedefs not allowed */
} __queue_t;
#define QNULL	((__queue_t *) 0)

/* general queue header */
typedef struct qheader {
	__queue_t hq_head;	/* front of queue */
	__queue_t hq_tail;	/* back of queue */
} qheader_t;
#define	QHNULL	((qheader_t *)0)

/* easy access to head and tail from a queue header */
#define	q_head	hq_head.q_next
#define	q_tail	hq_tail.q_next

/*
 * All queue macros take pointers to qheaders to simplify
 * using with register variables.
 */

/* initialize a queue */
#define	INIT_QUEUE(q) {(q)->q_tail = (q)->q_head = QNULL; }

/* return first object in a queue */
#define	FIRSTQ(q) ((q)->q_head)

/* true iff the queue is empty */
#define	ISEMPTYQ(q) ((FIRSTQ(q) == QNULL) ? TRUE : FALSE)

/* insert an item at the end of a queue */
#define INS_QUEUE(queue, item)						\
{									\
	register qheader_t *q = queue;					\
									\
	((__queue_t *)(item))->q_next = QNULL;				\
	if ((q)->q_head == QNULL) {					\
		(q)->q_head = (__queue_t *)(item);			\
	} else {							\
		(q)->q_tail->q_next = (__queue_t *)(item);		\
	}								\
	(q)->q_tail = (__queue_t *)(item);				\
}

/*
 * Remove first item in a queue and return it in "val".
 * The item returned in val should be of type "cast".
 */
#define	REM_QUEUE(cast, queue, val)						\
{										\
	register __queue_t *front = FIRSTQ(queue);				\
										\
	if ((front != QNULL) && (((queue)->q_head = front->q_next) == QNULL)) {	\
		(queue)->q_tail = QNULL;					\
	}									\
	val = cast front;							\
}

/* same as REM_QUEUE but don't bother returning a value */
#define	REMX_QUEUE(queue)						\
{									\
	register __queue_t *front = FIRSTQ(queue);			\
									\
	if (front != QNULL) {						\
		if (((queue)->q_head = front->q_next) == QNULL)		\
			(queue)->q_tail = QNULL;			\
		front->q_next = QNULL;					\
	}								\
}

/* Insert an item at the front of a queue (stack discipline) */
#define FRONT_QUEUE(q, item)						\
{									\
	if ((q)->q_head == QNULL) {					\
		(q)->q_tail = (__queue_t *)(item);			\
	}								\
	((__queue_t *)(item))->q_next = (q)->q_head;			\
	(q)->q_head = (__queue_t *)(item);				\
}

/*
 * Remove a particular element from a queue and return it
 * in val with the appropriate cast.
 */
#define	REM_ITEM(cast, queue, element, val) 				\
{									\
	register __queue_t *q = FIRSTQ(queue);				\
	register __queue_t *prev = QNULL;					\
									\
	while ((q != QNULL) && (q != (__queue_t *)(element))) {		\
		prev = q;						\
		q = q->q_next;						\
	}								\
	if (q != QNULL) {						\
		if (prev == QNULL) {					\
			(queue)->q_head = q->q_next;			\
		} else {						\
			prev->q_next = q->q_next;			\
		}							\
		if ((queue)->q_tail == q)				\
			(queue)->q_tail = prev;				\
	}								\
	val = cast q;							\
}

/* Same as REM_ITEM but don't bother returning the value */
#define	REMX_ITEM(queue, element)					\
{									\
	register __queue_t *q = FIRSTQ(queue);				\
	register __queue_t *prev = QNULL;					\
									\
	while ((q != QNULL) && (q != (__queue_t *)(element))) {		\
		prev = q;						\
		q = q->q_next;						\
	}								\
	if (q != QNULL) {						\
		if (prev == QNULL) {					\
			(queue)->q_head = q->q_next;			\
		} else {						\
			prev->q_next = q->q_next;			\
		}							\
		if ((queue)->q_tail == q)				\
			(queue)->q_tail = prev;				\
	}								\
}

/*
 * Move the front of the queue to the back of the same queue.
 * Return the NEW front value in val with the appropriate cast.
 */
#define	BACK_QUEUE(cast, q, val)					\
{									\
	register __queue_t *front = FIRSTQ(q);				\
									\
	if (front == QNULL)						\
		val = cast CHARNULL;					\
	else if (front->q_next == QNULL)				\
		val = cast front;					\
	else {								\
		(q)->q_head = front->q_next;				\
		front->q_next = QNULL;					\
		((q)->q_tail)->q_next = front;				\
		(q)->q_tail = front;					\
		val = cast (q)->q_head;					\
	}								\
}

/*
 * Same as BACK_QUEUE but no value returned. NB: No test for front == QNULL
 * for speed since nobody (currently) relies on this.
 */
#define	BACKX_QUEUE(q)							\
{									\
	register __queue_t *front = FIRSTQ(q);				\
									\
	if (front->q_next != QNULL) {					\
		(q)->q_head = front->q_next;				\
		front->q_next = QNULL;					\
		((q)->q_tail)->q_next = front;				\
		(q)->q_tail = front;					\
	}								\
}

/*
 * insert proc in queue by priority (higher prio towards front).
 */
#define PRIO_QUEUE(queue, proc, curprio)			\
{								\
	register __queue_t *qu, *prevq;				\
								\
	for (prevq = QNULL, qu = (queue)->q_head; qu != QNULL;	\
	  prevq = qu, qu = qu->q_next) {			\
		if ((curprio) > ((lwp_t *)qu)->lwp_prio)	\
			break;					\
	}							\
	((__queue_t *)(proc))->q_next = qu;			\
	if (prevq == QNULL) {					\
		(queue)->q_head = (__queue_t *)(proc);		\
	} else {						\
		prevq->q_next = (__queue_t *)(proc);		\
	}							\
	if (qu == QNULL) {					\
		(queue)->q_tail = (__queue_t *)(proc);		\
	}							\
}

/*
 * Special identifying info for queued objects (lwp's and agents in particular).
 * ALWAYS at offset 4 from the start of an object (immediately after
 * the queueing field).
 * To the client, an agent and a lwp are the same, but the nugget needs
 * to know which is which.
 */
typedef enum object_type_t {
	BADOBJECT_TYPE		= 0,	/* unintialized, thus a stale object */
	PROCESS_TYPE		= 1,	/* lwp */
	AGENT_TYPE		= 2,	/* agent */
} object_type_t;
#define	TYPE(p)	((*(object_type_t *)((caddr_t)p + (sizeof (struct __queue_t)))))
#endif /* !_lwp_queue_h */
