/*	@(#)str_sp.c 1.1 92/07/30 SMI; from S5R3 io/sp.c 10.2	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * SP - Stream "pipe" device.  Any two minor devices may
 * be opened and connected to each other so that each user
 * is at the end of a single stream.  This provides a full
 * duplex communications path and allows for the passing
 * of file descriptors as well.
 *
 * WARNING - an interprocess stream does not have the same
 *	     semantics as a pipe, and this does not replace
 *	     pipes.
 */



#include "sp.h"

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/stropts.h>
#include <sys/stream.h>

int	spopen(),
	spclose(),
	spput();
static struct module_info	spm_info = {
	1111, "sp", 0, INFPSZ, 5120, 1024
};
static struct qinit	sprinit = {
	NULL, NULL, spopen, spclose, NULL, &spm_info, NULL
};
static struct qinit	spwinit = {
	spput, NULL, NULL, NULL, NULL, &spm_info, NULL
};
struct streamtab	spinfo = {
	&sprinit, &spwinit, NULL, NULL
};

struct sp {
	queue_t *sp_rdq;		/* this stream's read queue */
	queue_t *sp_ordq;		/* other stream's read queue */
} sp_sp[NSP*32];

int spcnt = NSP*32;

/*ARGSUSED*/
spopen(q, dev, flag, sflag)
register queue_t *q;
register int dev;
{
	register struct sp *spp;

	dev = minor(dev);
	switch (sflag) {
	case MODOPEN:
		dev = (struct sp *)q->q_ptr - sp_sp;
		break;

	case CLONEOPEN:
		for (dev = 0, spp = sp_sp; dev < spcnt && spp->sp_rdq;
		    dev++, spp++)
			;
		break;
	}
	if (dev < 0 || dev >= spcnt)
		return (OPENFAIL);
	spp = &sp_sp[dev];
	if (!spp->sp_rdq) {
		spp->sp_rdq = q;
		q->q_ptr = WR(q)->q_ptr = (caddr_t)spp;
	}
	return (dev);
}

spclose(q)
register queue_t *q;
{
	register struct sp *spp, *osp;
	register queue_t *orq;
	register mblk_t *mp;

	spp = (struct sp *)q->q_ptr;
	spp->sp_rdq = NULL;
	if (orq = spp->sp_ordq) {
		osp = (struct sp *)orq->q_ptr;
		osp->sp_ordq = NULL;
		spp->sp_ordq = NULL;
		WR(orq)->q_next = NULL;
		WR(q)->q_next = NULL;
		if (mp = allocb(0, BPRI_MED)) {
			mp->b_datap->db_type = M_HANGUP;
			putnext(orq, mp);
		} else
			printf("SP: WARNING - close could not allocate block\n");
	}
	q->q_ptr = WR(q)->q_ptr = NULL;
}

spput(q, bp)
register queue_t *q;
register mblk_t *bp;
{
	register btype;

	switch (btype = bp->b_datap->db_type) {


	case M_IOCTL:
		bp->b_datap->db_type = M_IOCNAK;
		qreply(q, bp);
		return;

	case M_FLUSH:
		/*
		 * The meaning of read and write sides must be reversed
		 * for the destination stream head.
		 */
		if (!q->q_next) {
			*bp->b_rptr &= ~FLUSHW;
			if (*bp->b_rptr & FLUSHR) qreply(q, bp);
			return;
		}
		switch (*bp->b_rptr) {
		case FLUSHR: *bp->b_rptr = FLUSHW; break;
		case FLUSHW: *bp->b_rptr = FLUSHR; break;
		}
		putnext(q, bp);
		return;

	default:
		if (q->q_next) {
			putnext(q, bp);
			return;
		} else if (btype == M_PROTO) {

			register queue_t *oq;
			register struct sp *spp, *osp;
			register int i;

			if (bp->b_cont)
				goto errout;
			if ((bp->b_wptr - bp->b_rptr) != sizeof (queue_t *))
				goto errout;
			oq = *((queue_t **)bp->b_rptr);
			for (i = 0, osp = sp_sp;
			    i < spcnt && oq != osp->sp_rdq; i++, osp++)
				;
			if (i == spcnt)
				goto errout;
			if (osp->sp_ordq)
				goto errout;

			spp = (struct sp *)q->q_ptr;
			spp->sp_ordq = oq;
			osp->sp_ordq = RD(q);
			WR(oq)->q_next = RD(q)->q_next;
			q->q_next = oq->q_next;
			freemsg(bp);
			return;
		}
		break;
	}

errout:
	/*
	 * The stream has not been connected yet.
	 */
	bp->b_datap->db_type = M_ERROR;
	bp->b_wptr = bp->b_rptr = bp->b_datap->db_base;
	*bp->b_wptr++ = EIO;
	qreply(q, bp);
	return;
}
