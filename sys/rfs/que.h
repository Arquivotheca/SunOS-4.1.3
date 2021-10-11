/*	@(#)que.h 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)kern-port:sys/que.h	10.3" */

/*
 *	Declare the data structures used for queue manipulation.
 */

#ifndef _rfs_que_h
#define _rfs_que_h

/*
 *	Queues consist simply of one-way list.  Each queue is
 *	defined by a 'queue control element' made up of head
 *	and tail pointers.  Each item held on a queue contains
 *	a pointer word used to link successive item together.
 *	The head pointer contains the address of the first
 *	item in the queue and the tail pointer contains the
 *	address of the last item.
 */



/*
 *	Queue Item
 */

#define	QITEM	struct qitem_st

struct	qitem_st
	{
	QITEM	*qi_next;	/* next queue item pointer */
	char	data[1];	/* data */
	};



/*
 *	Queue Control Element
 */

#define	QCTRL	struct qctrl_st

struct	qctrl_st
	{
	QITEM	*qc_head;		/* head pointer */
	QITEM	*qc_tail;		/* tail pointer */
	};

#endif /*!_rfs_que_h*/
