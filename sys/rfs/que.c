/*	@(#)que.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*****

NAME		enque()		deque()
PURPOSE
	To provide procedures for queue manipulation.

DESCRIPTION
	Queues consist simply of one-way list.  Each queue is
	defined by a 'queue control element' made up of head
	and tail pointers.  Each item held on a queue contains
	a pointer word used to link successive item together.
	The head pointer field contains the address of the
	first item in the queue and the tail pointer field
	contains the address of the last item.  In addition,
	the pointer word of the last item in a queue is set
	to NULL.  When a queue is empty, the value of head
	pointer is NULL (and that of tail pointer is undefined).

FUNCTIONS
	enque(qctrl, qiptr)
		qctrl is the address of the queue control element
		and qiptr is the address of a queue item.
		The function of enque() is to link the given item
		onto the tail of the specified queue.

	deque(qctrl)
		qctrl is the address of the queue control element.
		The function of deque() is to unlink an item from
		the head of the specified queue and return the
		address of the item to the calling program.
		Zero is returned if queue is empty.

NOTES
	The advantage of this technique for queue handling is
	that any address can be used for a queue control element
	and it is possible to build up more complex data structures
	which use these control elements as components.  Also
	the size and format of items held on queues is immaterial
	since the procedures operate only on the pointer fields.
	Queue items may therefore be complex data structures
	possibly containing other pointer fields.

	In some cases more complex queue organizations may be
	required.  If a dynamic priority scheme is employed, it
	may be necessary to move items from one queue to another
	when item priorities change.  In such case items may have
	to be extracted from the middle of queues, which is
	facilitated better by using both backward and forward
	pointers for each item.

*****/





#include <sys/types.h>
#include <sys/stream.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <rfs/rfs_misc.h>
#include <rfs/sema.h>
#include <rfs/comm.h>

#ifndef	NULL
#define	NULL	0
#endif

enque(qctrl, qiptr)
struct bqctrl_st *qctrl;		/* queue control element */
mblk_t	*qiptr;				/* queue item pointer */
{
	if (qctrl->qc_head == NULL) {

		/*
		 *	queue is empty
		 */

		qctrl->qc_head = qiptr;
		qctrl->qc_tail = qiptr;
	}
	else {
		qctrl->qc_tail->b_next = qiptr;
		qctrl->qc_tail = qiptr;
	}
	qiptr->b_next = NULL;
}





mblk_t *deque(qctrl)
struct bqctrl_st *qctrl;		/* queue control element */
{
	mblk_t	*qiptr;		/* queue item pointer */

	qiptr = qctrl->qc_head;
	if (qiptr != NULL)
		qctrl->qc_head = qctrl->qc_head->b_next;
	return qiptr;
}
