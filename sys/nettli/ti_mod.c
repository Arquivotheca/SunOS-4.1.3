#ifndef lint
static  char sccsid[] = "@(#)ti_mod.c 1.1 92/07/30 SMI"; /* from S5R3 io/timod.c   10.4.1.1*/
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* #ident	"@(#)kern-port:io/timod.c	10.4.1.1" */
/*
 * Transport Interface Library cooperating module - issue 1
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <nettli/tihdr.h>
#include <nettli/timod.h>
#include <nettli/tiuser.h>
#include <sys/debug.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/errno.h>
#include "tim.h"

struct tim_tim {
	long 	 tim_flags;
	queue_t	*tim_rdq;
	mblk_t  *tim_iocsave;
};

extern struct tim_tim	tim_tim[];	/* declared in param.c */
extern int		tim_cnt;	/* defined in param.c */
extern nulldev();


#define TIMOD_ID	3


/* stream data structure definitions */
int timodopen(), timodclose(), timodrput(), timodwput();
static struct module_info timod_info = {TIMOD_ID, "timod", 0, INFPSZ, 4096, 1024};
static struct qinit timodrinit = { timodrput, NULL, timodopen, timodclose, nulldev, &timod_info, NULL};
static struct qinit timodwinit = { timodwput, NULL, timodopen, timodclose, nulldev, &timod_info, NULL};
struct streamtab timinfo = { &timodrinit, &timodwinit, NULL, NULL };



/*
 * state transition table for TI interface 
 */
#define nr	127		/* not reachable */

#ifndef lint
char ti_statetbl[TE_NOEVENTS][TS_NOSTATES] = {
				/* STATES */
/* 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16 */

 { 1, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr},
 {nr, nr, nr,  2, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr},
 {nr, nr, nr,  4, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr},
 {nr,  3, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr},
 {nr, nr, nr, nr,  3, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr},
 {nr,  0,  3, nr,  3,  3, nr, nr,  7, nr, nr, nr,  6,  7,  9, 10, 11},
 {nr, nr,  0, nr, nr,  6, nr, nr, nr, nr, nr, nr,  3, nr,  3,  3,  3},
 {nr, nr, nr, nr, nr, nr, nr, nr,  9, nr, nr, nr, nr,  3, nr, nr, nr},
 {nr, nr, nr, nr, nr, nr, nr, nr,  3, nr, nr, nr, nr,  3, nr, nr, nr},
 {nr, nr, nr, nr, nr, nr, nr, nr,  7, nr, nr, nr, nr,  7, nr, nr, nr},
 {nr, nr, nr,  5, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr},
 {nr, nr, nr, nr, nr, nr, nr,  8, nr, nr, nr, nr, nr, nr, nr, nr, nr},
 {nr, nr, nr, nr, nr, nr, 12, 13, nr, 14, 15, 16, nr, nr, nr, nr, nr},
 {nr, nr, nr, nr, nr, nr, nr, nr, nr,  9, nr, 11, nr, nr, nr, nr, nr},
 {nr, nr, nr, nr, nr, nr, nr, nr, nr,  9, nr, 11, nr, nr, nr, nr, nr},
 {nr, nr, nr, nr, nr, nr, nr, nr, nr, 10, nr,  3, nr, nr, nr, nr, nr},
 {nr, nr, nr,  7, nr, nr, nr,  7, nr, nr, nr, nr, nr, nr, nr, nr, nr},
 {nr, nr, nr, nr, nr, nr,  9, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr},
 {nr, nr, nr, nr, nr, nr, nr, nr, nr,  9, 10, nr, nr, nr, nr, nr, nr},
 {nr, nr, nr, nr, nr, nr, nr, nr, nr,  9, 10, nr, nr, nr, nr, nr, nr},
 {nr, nr, nr, nr, nr, nr, nr, nr, nr, 11,  3, nr, nr, nr, nr, nr, nr},
 {nr, nr, nr, nr, nr, nr,  3, nr, nr,  3,  3,  3, nr, nr, nr, nr, nr},
 {nr, nr, nr, nr, nr, nr, nr,  3, nr, nr, nr, nr, nr, nr, nr, nr, nr},
 {nr, nr, nr, nr, nr, nr, nr,  7, nr, nr, nr, nr, nr, nr, nr, nr, nr},
 {nr, nr, nr,  9, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr},
 {nr, nr, nr,  3, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr},
 {nr, nr, nr,  3, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr},
 {nr, nr, nr,  3, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr, nr},
};
#endif lint

/*
 * timodopen - open routine gets called when the
 *	       module gets pushed onto the stream.
 */

/*ARGSUSED*/
timodopen(q, dev, flag, sflag)
register queue_t *q;
{

	register struct tim_tim *timptr;

	ASSERT(q != NULL);

	if (q->q_ptr)
		return(1);

	for (timptr=tim_tim; timptr < &tim_tim[tim_cnt]; timptr++)
		if (!(timptr->tim_flags & USED))
			break;

	if (timptr >= &tim_tim[tim_cnt]) {
		u.u_error = ENOSPC;
		return(OPENFAIL);
	}


	timptr->tim_flags = USED;
	timptr->tim_rdq = q;
	timptr->tim_iocsave = NULL;
	q->q_ptr = (caddr_t)timptr;
	WR(q)->q_ptr = (caddr_t)timptr;

	return(1);
}


/*
 * timodclose - This routine gets called when the module
 *              gets popped off of the stream.
 */

timodclose(q)
register queue_t *q;
{
	register struct tim_tim *timptr;


	ASSERT(q != NULL);

	timptr = (struct tim_tim *)q->q_ptr;


	ASSERT(timptr != NULL);
	 
	if (timptr->tim_iocsave)
		freemsg(timptr->tim_iocsave);


	timptr->tim_flags = 0;
}


/*
 * timodrput - Module read queue put procedure.
 *             This is called from the module or
 *	       driver downstream.
 */

timodrput(q, mp)
register queue_t *q;
register mblk_t *mp;
{
	register union T_primitives *pptr;
	register struct tim_tim *timptr;
	register struct iocblk *iocbp;

	ASSERT(q != NULL);

	timptr = (struct tim_tim *)q->q_ptr;

	ASSERT(timptr != NULL);

	switch(mp->b_datap->db_type) {

	default:
		putnext(q, mp);
		return;

	case M_PROTO:
	case M_PCPROTO:
		/* asset checks if there is enough data to determine type */
		ASSERT((mp->b_wptr - mp->b_rptr) >= sizeof(long));

		pptr = (union T_primitives *)mp->b_rptr;

		switch (pptr->type) {

		default:
			putnext(q, mp);
			return;

		case T_ERROR_ACK:
			ASSERT((mp->b_wptr - mp->b_rptr) == sizeof(struct T_error_ack));
		        if (timptr->tim_flags&WAITIOCACK) {

				ASSERT(timptr->tim_iocsave != NULL);
				if (pptr->error_ack.ERROR_prim != *(long *)timptr->tim_iocsave->b_cont->b_rptr) {
					putnext(q, mp);
					return;
				}

				switch (pptr->error_ack.ERROR_prim) {

				case T_INFO_REQ:
				case T_OPTMGMT_REQ:
				case T_BIND_REQ:
				case T_UNBIND_REQ:
					/* get saved ioctl msg and set values */
					iocbp = (struct iocblk *)timptr->tim_iocsave->b_rptr;
					iocbp->ioc_error = 0;
					iocbp->ioc_rval = pptr->error_ack.TLI_error;
					if (iocbp->ioc_rval == TSYSERR)
						iocbp->ioc_rval |= pptr->error_ack.UNIX_error << 8;
					timptr->tim_iocsave->b_datap->db_type = M_IOCACK;
					putnext(q, timptr->tim_iocsave);

					timptr->tim_iocsave = NULL;
					timptr->tim_flags &= ~WAITIOCACK;
					freemsg(mp);
					return;
				}
			} 
			putnext(q, mp);
			return;

		case T_OK_ACK:
			if (timptr->tim_flags & WAITIOCACK) {
				ASSERT(timptr->tim_iocsave != NULL);
				if (pptr->ok_ack.CORRECT_prim != *(long *)timptr->tim_iocsave->b_cont->b_rptr) {
					putnext(q, mp);
					return;
				 }
				 goto out;
			}
			putnext(q, mp);
			return;
		case T_BIND_ACK:
			if (timptr->tim_flags & WAITIOCACK) {
				ASSERT(timptr->tim_iocsave != NULL);
				if (*(long *)timptr->tim_iocsave->b_cont->b_rptr != T_BIND_REQ) {
					putnext(q, mp);
					return;
				 }
				 goto out;
			}
			putnext(q, mp);
			return;
		case T_OPTMGMT_ACK:
			if (timptr->tim_flags & WAITIOCACK) {
				ASSERT(timptr->tim_iocsave != NULL);
				if (*(long *)timptr->tim_iocsave->b_cont->b_rptr != T_OPTMGMT_REQ) {
					putnext(q, mp);
					return;
			         }
				 goto out;
			}
			putnext(q, mp);
			return;
		case T_INFO_ACK:
			if (timptr->tim_flags & WAITIOCACK) {
				ASSERT(timptr->tim_iocsave != NULL);
				if (*(long *)timptr->tim_iocsave->b_cont->b_rptr != T_INFO_REQ) {
					putnext(q, mp);
					return;
				 }
				 q->q_maxpsz = pptr->info_ack.TIDU_size;
				 OTHERQ(q)->q_maxpsz = pptr->info_ack.TIDU_size;
				 goto out;
			}
			putnext(q, mp);
			return;
					
out:
			iocbp = (struct iocblk *)timptr->tim_iocsave->b_rptr;
			ASSERT(timptr->tim_iocsave->b_datap != NULL);
			timptr->tim_iocsave->b_datap->db_type = M_IOCACK;
			mp->b_datap->db_type = M_DATA;
			freemsg(timptr->tim_iocsave->b_cont);
			timptr->tim_iocsave->b_cont = mp;
			iocbp->ioc_error = 0;
			iocbp->ioc_rval = 0;
			iocbp->ioc_count = mp->b_wptr - mp->b_rptr;
	
			putnext(q, timptr->tim_iocsave);

			timptr->tim_iocsave = NULL;
			timptr->tim_flags &= ~WAITIOCACK;
			return;
		}

	}
}


/*
 * timodwput - Module write queue put procedure.
 *             This is called from the module or
 *	       stream head upstream.
 */

timodwput(q, mp)
register queue_t *q;
register mblk_t *mp;
{
	register struct tim_tim *timptr;
 	struct iocblk *iocbp = (struct iocblk *)mp->b_rptr;
	register mblk_t *tmp;

	ASSERT(q != NULL);

	timptr = (struct tim_tim *)q->q_ptr;

	ASSERT(timptr != NULL);


	switch(mp->b_datap->db_type) {

	default:
		putnext(q, mp);
		return;
	
	case M_IOCTL:
		ASSERT((mp->b_wptr - mp->b_rptr) == sizeof(struct iocblk));

		if (timptr->tim_flags & WAITIOCACK) {
			mp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = EPROTO;
			qreply(q, mp);
			return;
		}

		switch (iocbp->ioc_cmd) {

		default:
			putnext(q, mp);
			return;

		case TI_BIND:
		case TI_UNBIND:
		case TI_GETINFO:
		case TI_OPTMGMT:

			if (mp->b_cont == NULL) {
				mp->b_datap->db_type = M_IOCNAK;
				iocbp->ioc_error = EINVAL;
			 	qreply(q, mp);
				return;
			}
			if (!pullupmsg(mp->b_cont, -1)) {
				mp->b_datap->db_type = M_IOCNAK;
				iocbp->ioc_error = EAGAIN;
			 	qreply(q, mp);
				return;
			}	

			if ((tmp = copymsg(mp->b_cont)) == NULL) {
				mp->b_datap->db_type = M_IOCNAK;
				iocbp->ioc_error = EAGAIN;
			 	qreply(q, mp);
				return;
			}

			timptr->tim_iocsave = mp;
			timptr->tim_flags |= WAITIOCACK;

			if (iocbp->ioc_cmd == TI_GETINFO)
				tmp->b_datap->db_type = M_PCPROTO;
			else
				tmp->b_datap->db_type = M_PROTO;
	
			putnext(q, tmp);
			return;
		}
	}
}
