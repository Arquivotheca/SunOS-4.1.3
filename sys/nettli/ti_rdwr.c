#ifndef lint
static  char sccsid[] = "@(#)ti_rdwr.c 1.1 92/07/30 SMI"; /* from S5R3 io/tirdwr.c  10.4 */
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* #ident	"@(#)kern-port:io/tirdwr.c	10.4" */
/*
 * Transport Interface Library read/write module - issue 1
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <nettli/tihdr.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/debug.h>
#include <sys/errno.h>
#include "tirw.h"

struct trw_trw {
	long 	 trw_flags;
	queue_t	*trw_rdq;
	mblk_t  *trw_mp;
};

#define USED 		001
#define ORDREL 		002
#define DISCON  	004
#define FATAL		010
#define WAITACK 	020

#define TIRDWRPRI 	PZERO+3

extern struct trw_trw	trw_trw[];	/* declared in param.c */
extern int		trw_cnt;	/* defined in param.c */
extern nulldev();

#define TIRDWR_ID	4


/* stream data structure definitions */
int tirdwropen(), tirdwrclose(), tirdwrrput(), tirdwrwput();
static struct module_info tirdwr_info = {TIRDWR_ID, "tirdwr", 0, INFPSZ, 4096, 1024};
static struct qinit tirdwrrinit = { tirdwrrput, NULL, tirdwropen, tirdwrclose, nulldev, &tirdwr_info, NULL};
static struct qinit tirdwrwinit = { tirdwrwput, NULL, tirdwropen, tirdwrclose, nulldev, &tirdwr_info, NULL};
struct streamtab trwinfo = { &tirdwrrinit, &tirdwrwinit, NULL, NULL };



/*
 * tirdwropen - open routine gets called when the
 *	       module gets pushed onto the stream.
 */

/*ARGSUSED*/
tirdwropen(q, dev, flag, sflag)
register queue_t *q;
{

	register struct trw_trw *trwptr;

	ASSERT(q != NULL);

	/* check if already open */
	if (q->q_ptr)
		return(1);

	/* find free data structure */
	for (trwptr=trw_trw; trwptr < &trw_trw[trw_cnt]; trwptr++)
		if (!(trwptr->trw_flags & USED))
			break;
	if (trwptr >= &trw_trw[trw_cnt]) {
		u.u_error = ENOSPC;
		return(OPENFAIL);
	}

	if (!check_strhead(q)) {
		u.u_error = EPROTO;
		return(OPENFAIL);
	}

	if ((trwptr->trw_mp = (mblk_t *)allocb(sizeof(struct T_discon_req), BPRI_LO)) == NULL) {
		u.u_error = EAGAIN;
		return(OPENFAIL);
	}

	strip_strhead(q);

	/* initialize data structure */
	trwptr->trw_flags = USED;
	trwptr->trw_rdq = q;
	q->q_ptr = (caddr_t)trwptr;
	WR(q)->q_ptr = (caddr_t)trwptr;
	WR(q)->q_maxpsz = WR(q)->q_next->q_maxpsz;
	q->q_maxpsz = q->q_next->q_maxpsz;

	return(1);
}


/*
 * tirdwrclose - This routine gets called when the module
 *              gets popped off of the stream.
 */

tirdwrclose(q)
register queue_t *q;
{
	register struct trw_trw *trwptr;
	register mblk_t *mp;
	register union T_primitives *pptr;


	ASSERT(q != NULL);

	/* assign saved data structure from queue */
	trwptr = (struct trw_trw *)q->q_ptr;


	ASSERT(trwptr != NULL);
	 

	if ((trwptr->trw_flags & ORDREL) && !(trwptr->trw_flags & FATAL)) {
		mp = trwptr->trw_mp;
		pptr = (union T_primitives *)mp->b_rptr;
		mp->b_wptr = mp->b_rptr + sizeof(struct T_ordrel_req);
		pptr->type = T_ORDREL_REQ;
		mp->b_datap->db_type = M_PROTO;
		putnext(WR(q), mp);
		trwptr->trw_mp = NULL;
	}

	freemsg(trwptr->trw_mp);
	trwptr->trw_flags = 0;
	trwptr->trw_rdq = NULL;
	trwptr->trw_mp = NULL;
}


/*
 * tirdwrrput - Module read queue put procedure.
 *             This is called from the module or
 *	       driver downstream.
 */

tirdwrrput(q, mp)
register queue_t *q;
register mblk_t *mp;
{
	register union T_primitives *pptr;
	register struct trw_trw *trwptr;
	register mblk_t *tmp;

	ASSERT(q != NULL);

	trwptr = (struct trw_trw *)q->q_ptr;

	ASSERT(trwptr != NULL);

	if ((trwptr->trw_flags & FATAL) && !(trwptr->trw_flags & WAITACK)) {
		freemsg(mp);
		return;
	}

	switch(mp->b_datap->db_type) {

	default:
		putnext(q, mp);
		return;

	case M_DATA:
		if (msgdsize(mp) == 0) {
			freemsg(mp);
			return;
		}
		putnext(q, mp);
		return;

	case M_PCPROTO:
	case M_PROTO:
		/* is there enough data to check type */
		ASSERT((mp->b_wptr - mp->b_rptr) >= sizeof(long));

		pptr = (union T_primitives *)mp->b_rptr;

		switch (pptr->type) {

		case T_EXDATA_IND:
			send_fatal(q, mp);
			return;
		case T_DATA_IND:
			if (msgdsize(mp) == 0) {
				freemsg(mp);
				return;
			}

			tmp = (mblk_t *)unlinkb(mp);
			freemsg(mp);
			putnext(q, tmp);
			return;

		case T_ORDREL_IND:
			trwptr->trw_flags |= ORDREL;
			mp->b_datap->db_type = M_DATA;
			mp->b_wptr = mp->b_rptr;
			putnext(q, mp);
			return;

		case T_DISCON_IND:
			trwptr->trw_flags |= DISCON;
			trwptr->trw_flags &= ~ORDREL;
			if (msgdsize(mp) != 0) {
				tmp = (mblk_t *)unlinkb(mp);
				putnext(q, tmp);
			}
			mp->b_datap->db_type = M_HANGUP;
			mp->b_wptr = mp->b_rptr;
			putnext(q, mp);
			return;

		case T_ERROR_ACK:
		case T_OK_ACK:
			if (trwptr->trw_flags & WAITACK) {
				trwptr->trw_flags &= ~WAITACK;
				wakeup((caddr_t)trwptr);
				if (pptr->type == T_ERROR_ACK) {
					send_fatal(q, mp);
					return;
				}
				freemsg(mp);
				return;
			}
			/* flow thru */
		default:
			send_fatal(q, mp);
			return;
		}
	}
}


/*
 * tirdwrwput - Module write queue put procedure.
 *             This is called from the module or
 *	       stream head upstream.
 */

tirdwrwput(q, mp)
register queue_t *q;
register mblk_t *mp;
{
	register struct trw_trw *trwptr;

	ASSERT(q != NULL);

	trwptr = (struct trw_trw *)q->q_ptr;

	ASSERT(trwptr != NULL);

	if (trwptr->trw_flags & FATAL) {
		freemsg(mp);
		return;
	}

	switch(mp->b_datap->db_type) {

	default:
		putnext(q, mp);
		return;

	case M_DATA:
		if (msgdsize(mp) == 0) {
			freemsg(mp);
			return;
		}
		putnext(q, mp);
		return;

	case M_PROTO:
	case M_PCPROTO:
		send_fatal(q, mp);
		return;
	}
}


static
send_fatal(q, mp)
register queue_t *q;
register mblk_t *mp;
{
	register struct trw_trw *trwptr;

	trwptr = (struct trw_trw *)q->q_ptr;

	trwptr->trw_flags |= FATAL;
	mp->b_datap->db_type = M_ERROR;
	*mp->b_datap->db_base = EPROTO;
	mp->b_rptr = mp->b_datap->db_base;
	mp->b_wptr = mp->b_datap->db_base + sizeof(char);
	freemsg(unlinkb(mp));
	if (q->q_flag&QREADR)
		putnext(q, mp);
	else
		qreply(q, mp);
}


static
check_strhead(q)
queue_t *q;
{
	register mblk_t *mp;
	register union T_primitives *pptr;

	for (mp = q->q_next->q_first; mp != NULL; mp = mp->b_next) {

		switch(mp->b_datap->db_type) {
		case M_PROTO:
			pptr = (union T_primitives *)mp->b_rptr;
			if ((mp->b_wptr - mp->b_rptr) < sizeof(long))
				return(0);
			switch (pptr->type) {

				case T_EXDATA_IND:
					return(0);
				case T_DATA_IND:
					if (mp->b_cont &&
					    (mp->b_cont->b_datap->db_type != M_DATA))
						return(0);
					break;
				default:
					return(0);
			}
			break;

		case M_PCPROTO:
			return(0);

		case M_DATA:
		case M_SIG:
			break;
		default:
			return(0);
		}
	}
	return(1);
}



static
strip_strhead(q)
queue_t *q;
{
	register mblk_t *mp;
	register mblk_t *emp;
	register mblk_t *tmp;
	register union T_primitives *pptr;

	q = q->q_next;
	for (mp = q->q_first; mp != NULL;) {

		switch(mp->b_datap->db_type) {
		case M_PROTO:
			pptr = (union T_primitives *)mp->b_rptr;
			switch (pptr->type) {

				case T_DATA_IND:
					if (msgdsize(mp) == 0) {
strip0:
						tmp = mp->b_next;
						rmvq(q, mp);
						freemsg(mp);
						mp = tmp;
						break;
					}
					emp = mp->b_next;
					rmvq(q, mp);
					tmp = (mblk_t *)unlinkb(mp);
					freeb(mp);
					insq(q, emp, tmp);
					mp = emp;
					break;
			}
			break;

		case M_DATA:
			if (msgdsize(mp) == 0)
				goto strip0;
			mp = mp->b_next;
			break;

		case M_SIG:
			mp = mp->b_next;
			break;
		}
	}
	return;
}
