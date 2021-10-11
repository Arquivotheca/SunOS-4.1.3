/* @(#)queue.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1986. Sun Microsystems, Inc. */

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
	int __s;							\
									\
	__s = splclock();						\
	((__queue_t *)(item))->q_next = QNULL;				\
	if ((q)->q_head == QNULL) {					\
		(q)->q_head = (__queue_t *)(item);			\
	} else {							\
		(q)->q_tail->q_next = (__queue_t *)(item);		\
	}								\
	(q)->q_tail = (__queue_t *)(item);				\
	(void) splx(__s);						\
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
	int __s;							\
									\
	__s = splclock();						\
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
	(void) splx(__s);						\
}

/* Same as REM_ITEM but don't bother returning the value */
#define	REMX_ITEM(queue, element)					\
{									\
	register __queue_t *q = FIRSTQ(queue);				\
	register __queue_t *prev = QNULL;					\
	int rmq_ps;							\
									\
	rmq_ps = splclock();						\
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
	(void) splx(rmq_ps);						\
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
	register int __s;					\
								\
	__s = splclock();					\
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
	(void) splx(__s);					\
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
