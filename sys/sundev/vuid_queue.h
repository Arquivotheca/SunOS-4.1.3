/*	@(#)vuid_queue.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * This file describes a virtual user input device (vuid) event queue
 * maintainence package (see ../sundev/vuid_event.h for a description
 * of what vuid is).  This header file defines the interface that a
 * client of this package sees.  This package is used to maintain queues
 * of firm events awaiting deliver to some consumer.
 */

#ifndef _sundev_vuid_queue_h
#define _sundev_vuid_queue_h

/*
 * Vuid input queue structure.
 */
typedef	struct	vuid_queue {
	struct	vuid_q_node *top;	/* input queue head (first in line) */
	struct	vuid_q_node *bottom;	/* input queue head (last in line) */
	struct	vuid_q_node *free;	/* input queue free list */
	int	num;			/* number of items currently on queue */
	int	size;			/* number of items allowed on queue */
} Vuid_queue;
#define	VUID_QUEUE_NULL	((Vuid_queue *)0)
#define	vq_used(vq) ((vq)->num)
#define	vq_avail(vq) ((vq)->size - (vq)->num)
#define	vq_size(vq) ((vq)->size)
#define	vq_is_empty(vq) ((vq)->top == VUID_Q_NODE_NULL)
#define	vq_is_full(vq) ((vq)->num == (vq)->size)

/*
 * Vuid input queue node structure.
 */
typedef	struct	vuid_q_node {
	struct	vuid_q_node *next;	/* Next item in queue */
	struct	vuid_q_node *prev;	/* Previous item in queue */
	Firm_event firm_event;		/* Firm event */
} Vuid_q_node;
#define	VUID_Q_NODE_NULL	((Vuid_q_node *)0)

/*
 * Vuid input queue status codes.
 */
typedef	enum	vuid_q_code {
	VUID_Q_OK=0,		/* OK */
	VUID_Q_OVERFLOW=1,	/* overflow */
	VUID_Q_EMPTY=2,		/* empty */
} Vuid_q_code;

extern	void vq_initialize();	/* (Vuid_queue *vq, caddr_t data, u_int bytes)
				   Client allocates bytes worth of storage
				   and pass in a data.  Client destroys the q
				   simply by releasing data. */
extern	Vuid_q_code vq_put();	/* (Vuid_queue *vq, Firm_event *firm_event)
				   Place firm_event on queue, position is
				   dependent on the firm event's time.  Can
				   return VUID_Q_OVERFLOW if no more room. */
extern	Vuid_q_code vq_get();	/* (Vuid_queue *vq, Firm_event *firm_event)
				   Place event on top of queue into firm_event.
				   Can return VUID_Q_EMPTY if no more events */
extern	Vuid_q_code vq_peek();	/* Like vq_get but doesn't remove from queue */
extern	Vuid_q_code vq_putback();/* (Vuid_queue *vq, Firm_event *firm_event)
				   Push firm_event on top of queue.  Can
				   return VUID_Q_OVERFLOW if no more room. */

extern	int vq_compress();	/* (Vuid_queue *vq, factor)  Try to
				   collapse the queue to a size of 1/factor
				   by squeezing like valuator events together.
				   Returns number collapsed */
extern	int vq_is_valuator();	/* (Vuid_q_node *vqn) if value is not 0 or 1 ||
				   pair_type is FE_PAIR_DELTA or
				   FE_PAIR_ABSOLUTE */
extern	void vq_delete_node();	/* (Vuid_queue *vq, Vuid_q_node *vqn)
				   Deletes vqn from vq */


#endif /*!_sundev_vuid_queue_h*/
