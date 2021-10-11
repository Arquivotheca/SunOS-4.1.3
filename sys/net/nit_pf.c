#ifndef	lint
static char sccsid[] = "@(#)nit_pf.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif	lint

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Streams NIT packet filter module
 *
 * This module applies a filter to messages arriving on its read
 * queue, passing on messages that the filter accepts and dropping
 * the others.  It supports ioctls for setting and (eventually)
 * getting the filter.
 *
 * On the write side, the module simply passes everything through
 * unchanged.
 *
 * The module is intended to sit on top of a network interface driver
 * or pseudo-driver such as the one defined in nit_if.c.
 *
 * XXX: Make sure that correct processor levels are observed throughout.
 */

/* #define PFDEBUG */

#include "pf.h"

#if	NPF > 0

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/debug.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/user.h>		/* for u.u_error */

#include <net/packetfilt.h>
#include <net/nit_pf.h>

/*
 * Stereotyped streams glue.
 */

int	pfopen();
int	pfclose();		/* really void, not int */
int	pfrput();		/* really void, not int */
int	pfwput();		/* really void, not int */

struct module_info	pf_minfo = {
	20,			/* module id XXX: define real value */
	"pf",
	0,
	32767,			/* max msg size XXX: make reasonable */
	STRHIGH,		/* high water XXX: make reasonable */
	STRLOW			/* low water XXX: make reasonable */
};

struct qinit	pf_rinit = {
	pfrput,
	NULL,		/* srvp (XXX: can we do without one?) */
	pfopen,
	pfclose,
	NULL,
	&pf_minfo,
	NULL
};

struct qinit	pf_winit = {
	pfwput,
	NULL,		/* srvp (XXX: can we do without one?) */
	NULL,
	NULL,
	NULL,
	&pf_minfo,
	NULL
};

struct streamtab	pf_info = {
	&pf_rinit,
	&pf_winit,
	NULL,
	NULL
};

/*
 * Per-module instance state information.
 *
 * Each instance is dynamically allocated from the dblk pool.
 * p_mb cross-references back to the mblk whose associated dblk
 * contains the instance.
 */
typedef struct pf_softc {
	mblk_t			*p_mb;	/* cross-link to containing mblk */
	queue_t			*p_rq;	/* cross-link to read queue */
	dev_t			p_dev;	/* used for error printouts only */
	struct epacketfilt	p_pf;	/* filter to apply */
} pf_softc_t;


/* ARGSUSED */
int
pfopen(q, dev, oflag, sflag)
	struct queue	*q;
	dev_t		dev;
	int		oflag,
			sflag;
{
	register pf_softc_t		*pp;
	register mblk_t			*bp;
	register struct epacketfilt	*pfp;

#ifdef	PFDEBUG
	printf("pfopen: q: %x, WR(q): %x, dev: %x, oflag: %x, sflag: %x\n",
		q, WR(q), dev, oflag, sflag);
#endif	PFDEBUG

	/* We must be called with MODOPEN. */
	if (sflag != MODOPEN)
		return (OPENFAIL);

	/*
	 * If someone else already has us open, there's
	 * nothing further to do.
	 */
	if (q->q_ptr)
		return (0);

	/*
	 * Allocate space for per-open-instance information
	 * and set up internal cross-links.
	 *
	 * XXX:	If the allocation fails, I suppose we could
	 *	try a bufcall...
	 */
	bp = allocb((int) sizeof *pp, BPRI_MED);
	if (! bp) {
		u.u_error = ENOMEM;		/* gag */
		return (OPENFAIL);
	}
	bp->b_wptr += sizeof *pp;
	pp = (pf_softc_t *)bp->b_rptr;
	pp->p_mb = bp;

	/*
	 * Set up cross-links between queues and the softc structure.
	 */
	pp->p_rq = q;
	q->q_ptr = (char *)pp;
	WR(q)->q_ptr = (char *)pp;

	/*
	 * Record dev for later use in error messages.
	 */
	pp->p_dev = dev;

	/*
	 * Initialize to zero-length filter.
	 */
	pfp = &pp->p_pf;
	pfp->pf_FilterLen = 0;
	pfp->pf_FilterEnd = &pfp->pf_Filter[0];

	return (0);
}

pfclose(q)
	register queue_t	*q;
{
	register pf_softc_t	*pp = (pf_softc_t *)q->q_ptr;

#ifdef	PFDEBUG
	printf("pfclose: q: %x, WR(q): %x ", q, WR(q));
#endif	PFDEBUG

	/*
	 * Flush outstanding traffic on its way
	 * downstream to us.
	 *	Is this appropriate for this module?
	 */
	flushq(WR(q), FLUSHALL);

	/*
	 * Free the softc structure itself, then unlink it.
	 */
	freeb(pp->p_mb);
	q->q_ptr = NULL;
}

/*
 * Write-side put procedure.  Its main task is to handle the
 * ioctls for manipulating the packet filter.  Other message
 * types are passed on through.
 */
pfwput(q, mp)
	queue_t	*q;
	mblk_t	*mp;
{
	register pf_softc_t		*pp;
	register struct iocblk		*iocp;

	switch (mp->b_datap->db_type) {

	case M_FLUSH:
		/*
		 * Only worry about FLUSHW here.  We'll catch FLUSHR
		 * on the way back up.  We don't flush control messages,
		 * although it's not clear that this is correct.
		 */
		if (*mp->b_rptr & FLUSHW)
			flushq(q, FLUSHDATA);
		putnext(q, mp);
		break;

	case M_IOCTL:
		pp = (pf_softc_t *)q->q_ptr;
		iocp = (struct iocblk *)mp->b_rptr;
#ifdef	PFDEBUG
		printf("pf M_IOCTL, cmd: %x\n", iocp->ioc_cmd);
#endif	PFDEBUG

		switch (iocp->ioc_cmd) {

		case NIOCSETF: {
			register struct epacketfilt	*pfp;
			register struct packetfilt	*upfp;
			register u_short		*fwp;
			register int			maxoff;
			int				s;

			pfp = &pp->p_pf;

			/*
			 * Verify argument length.
			 */
			if (iocp->ioc_count < sizeof (struct packetfilt)) {
				mp->b_datap->db_type = M_IOCNAK;
				iocp->ioc_error = EINVAL;
				qreply(q, mp);
				break;
			}

			/*
			 * Do a bit of validity checking before
			 * committing to the new filter.
			 */
			upfp = (struct packetfilt *)mp->b_cont->b_rptr;
			if (upfp->Pf_FilterLen > ENMAXFILTERS) {
				mp->b_datap->db_type = M_IOCNAK;
				iocp->ioc_error = EINVAL;
				qreply(q, mp);
				break;
			}

			/*
			 * Install the new filter atomically.
			 *
			 * We move to splstr to prevent inconsistencies
			 * from scheduling the read side while we're
			 * part way through setting up the filter.
			 */
			s = splstr();
			bcopy((caddr_t)upfp, (caddr_t)pfp, (u_int)sizeof *upfp);
			pfp->pf_FilterEnd = &pfp->pf_Filter[pfp->pf_FilterLen];
			/*
			 * Find and record maximum byte offset that the
			 * filter uses.  We use this when executing the
			 * filter to determine how much of the packet
			 * body to pull up.  This code depends on the
			 * filter encoding.
			 */
			maxoff = 0;
			for (fwp = pfp->pf_Filter; fwp < pfp->pf_FilterEnd; fwp++) {
				register u_int	arg;

				arg = *fwp & ((1 << ENF_NBPA) - 1);
				switch (arg) {

				default:
					if ((arg -= ENF_PUSHWORD) > maxoff)
						maxoff = arg;
					break;

				case ENF_PUSHLIT:
					/* Skip over the literal. */
					fwp++;
					break;

				case ENF_PUSHZERO:
				case ENF_NOPUSH:
					break;
					
				}
			}
			/* Convert word offset to length in bytes. */
			pfp->pf_PByteLen = (maxoff + 1) * sizeof (u_short);
			(void) splx(s);

#ifdef	PFDEBUG
			printf("pf NIOCSETF installed at %x, len %d, PByteLen %d\n",
				pfp, pfp->pf_FilterLen, pfp->pf_PByteLen);
#endif	PFDEBUG

			/* Acknowledge with no data returned. */
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = 0;
			qreply(q,mp);
			break;
		    }

#ifdef	notdef
		case PF_GETF:
			/* I suppose we should support this someday... */
#endif	notdef


		default:
			/*
			 * Pass unrecognized ioctls through to
			 * give modules below us a chance at them.
			 */
			putnext(q, mp);
			break;
		}
		break;

	default:
		putnext(q, mp);
		break;
	}
}

/*
 * Read-side put procedure.  It's responsible for applying the
 * packet filter and passing an incoming message on or discarding
 * it depending on the results.
 *
 * Incoming messages can start with zero or more M_PROTO mblks.
 * Such header information does not participate in the filtering
 * process.
 *
 * Design question: How should M_PROTO-only messages be handled?
 * The current implementation passes them through, but it's not
 * clear that this is the right thing to do.
 */
pfrput(q, mp)
	queue_t	*q;
	mblk_t	*mp;
{
	register struct epacketfilt	*pfp;
	register mblk_t			*mbp,
					*mpp;
	struct packdesc			pd;

	ASSERT(mp && mp->b_datap);

	switch (mp->b_datap->db_type) {

	case M_PROTO:
	case M_DATA: {
		register int	need;

		ASSERT(q->q_ptr);
		pfp = &((pf_softc_t *)q->q_ptr)->p_pf;

		/*
		 * Skip over protocol information and find the start
		 * of the message body, saving the overall message
		 * start in mpp.
		 */
		mpp = mp;
		while (mp && mp->b_datap->db_type == M_PROTO)
			mp = mp->b_cont;

		/*
		 * Null body (exclusive of M_PROTO blocks) ==> accept.
		 * Note that a null body is not the same as an empty body.
		 */
		if (!mp) {
			putnext(q, mpp);
			break;
		}

		/*
		 * Pull the packet up to the length required by
		 * the filter.  Note that doing so destroys sharing
		 * relationships, which is unfortunate, since the
		 * results of pulling up here are likely to be useful
		 * for shared messages applied to a filter on a sibling
		 * stream.
		 *
		 * Most packet sources will provide the packet in two
		 * logical pieces: an initial header in a single mblk,
		 * and a body in a sequence of mblks hooked to the
		 * header.  We're prepared to deal with variant forms,
		 * but in any case, the pullup applies only to the body
		 * part.
		 */
		mbp = mp->b_cont;
		need = pfp->pf_PByteLen;
		if (mbp && mbp->b_wptr - mbp->b_rptr < need) {
			register int	len = msgdsize(mbp);

			if (pullupmsg(mbp, MIN(need, len)) == 0) {
				dev_t	dev;

				dev = ((pf_softc_t *)q->q_ptr)->p_dev;
				printf("pf<%d,%d>: pullupmsg failed\n",
					major(dev), minor(dev));
				freemsg(mpp);
				break;
			}
		}

		/*
		 * Misaliagnment (not on short boundary) ==> reject.
		 */
		if (((u_int)mp->b_rptr & (sizeof (u_short) - 1)) ||
		    (mbp && ((u_int)mbp->b_rptr & (sizeof (u_short) - 1)))) {
			dev_t	dev = ((pf_softc_t *)q->q_ptr)->p_dev;

			printf ("pf<%d,%d>: misaligned packet\n",
				major(dev), minor(dev));
		    	freemsg(mpp);
			break;
		}

		/*
		 * These assignments are distasteful, but necessary,
		 * since the packet filter wants to work in terms of
		 * shorts.  Odd bytes at the end of header or data can't
		 * participate in the filtering operation.
		 */
		pd.pd_hdr = (u_short *)mp->b_rptr;
		pd.pd_hdrlen = (mp->b_wptr - mp->b_rptr) / sizeof (u_short);
		if (mbp) {
			pd.pd_body = (u_short *)mbp->b_rptr;
			pd.pd_bodylen = (mbp->b_wptr - mbp->b_rptr) /
							sizeof (u_short);
		}
		else {
			pd.pd_body = NULL;
			pd.pd_bodylen = 0;
		}

		/*
		 * Apply the filter.
		 */
		if (FilterPacket(&pd, pfp))
			putnext(q, mpp);
		else
			freemsg(mpp);

		break;
	    }

	case M_FLUSH:
		/*
		 * Symmetrically to M_FLUSH in pfwput,
		 * we only worry about FLUSHR here.
		 */
		if (*mp->b_rptr & FLUSHR)
			flushq(q, FLUSHDATA);
		putnext(q, mp);
		break;

	default:
		putnext(q, mp);
		break;
	}
}

#endif	NPF > 0
