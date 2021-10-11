#ifndef	lint
static char sccsid[] = "@(#)nit_buf.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif	lint

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * NIT buffering module
 *
 * This streams module collects incoming messages from modules below
 * it on the stream and buffers them up into a smaller number of
 * aggregated messages.  Its main purpose is to reduce overhead by
 * cutting down on the number of read (or getmsg) calls its client
 * user process makes.
 */

/* #define NBDEBUG */

#include "nbuf.h"

#if	NNBUF > 0

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/debug.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/user.h>		/* only for u.u_error */

#include <net/nit_buf.h>

int	nbclosechunk();

/*
 * Stereotyped streams glue.
 */

int	nbopen();
int	nbclose();		/* really void, not int */
int	nbrput();		/* really void, not int */
int	nbwput();		/* really void, not int */

struct module_info	nb_minfo = {
	21,			/* module id XXX: define real value */
	"nitbuf",
	0,
	INFPSZ,			/* max msg size */
	STRHIGH,		/* high water XXX: make reasonable */
	STRLOW			/* low water XXX: make reasonable */
};

struct qinit	nb_rinit = {
	nbrput,
	NULL,
	nbopen,
	nbclose,
	NULL,
	&nb_minfo,
	NULL
};

struct qinit	nb_winit = {
	nbwput,
	NULL,
	NULL,
	NULL,
	NULL,
	&nb_minfo,
	NULL
};

struct streamtab	nb_info = {
	&nb_rinit,
	&nb_winit,
	NULL,
	NULL
};

/*
 * Per-module instance state information.
 *
 * Each instance is dynamically allocated from the dblk pool.
 * nb_soft cross-references back to the mblk whose associated dblk
 * contains the instance.
 *
 * If nb_ticks is negative, we don't deliver chunks until they're
 * full.  If it's zero, we deliver every packet as it arrives.  (In
 * this case we force nb_chunk to zero, to make the implementation
 * easier.)  Otherwise, nb_ticks gives the number of ticks from
 * the last delivery time to the next one.
 */
typedef struct nb_softc {
	mblk_t		*nb_soft;	/* cross-link to containing mblk */
	queue_t		*nb_rq;		/* cross-link to read queue */
	mblk_t		*nb_mp;		/* partially accumulated aggregate */
	u_int		nb_mlen;	/* nb_mp length */
	u_int		nb_chunk;	/* max aggregate packet data size */
	int		nb_ticks;	/* timeout interval */
} nb_softc_t;


/* ARGSUSED */
int
nbopen(q, dev, oflag, sflag)
	struct queue	*q;
	dev_t		dev;
	int		oflag,
			sflag;
{
	register nb_softc_t	*nbp;
	register mblk_t		*bp;

#ifdef	NBDEBUG
	printf("nbopen: q: %x, WR(q): %x, dev: %x, oflag: %x, sflag: %x\n",
		q, WR(q), dev, oflag, sflag);
#endif	NBDEBUG

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
	bp = allocb((int) sizeof *nbp, BPRI_MED);
	if (! bp) {
		u.u_error = ENOMEM;		/* gag */
		return (OPENFAIL);
	}
	bp->b_wptr += sizeof *nbp;
	nbp = (nb_softc_t *)bp->b_rptr;
	nbp->nb_soft = bp;

	/*
	 * Set up cross-links between queues and the softc structure.
	 */
	nbp->nb_rq = q;
	q->q_ptr = WR(q)->q_ptr = (char *)nbp;

	/*
	 * Set default buffering values.
	 */
	nbp->nb_ticks = -1;
	nbp->nb_chunk = NB_DFLT_CHUNK;

	/*
	 * Initialize the first chunk.
	 */
	nbp->nb_mp = NULL;
	nbp->nb_mlen = 0;
	nbstartchunk(nbp);

	return (0);
}

nbclose(q)
	register queue_t	*q;
{
	register nb_softc_t	*nbp = (nb_softc_t *)q->q_ptr;
	register int		s = splstr();

#ifdef	NBDEBUG
	printf("nbclose: q: %x, WR(q): %x ", q, WR(q));
#endif	NBDEBUG

	/*
	 * Flush outstanding traffic on its way
	 * downstream to us.
	 *	Is this appropriate for this module?
	 */
	flushq(WR(q), FLUSHALL);

	/*
	 * Clean up state: free partially accumulated chunk
	 * and cancel pending timeout.
	 */
	if (nbp->nb_mp)
		freemsg(nbp->nb_mp);
	nbp->nb_mp = NULL;
	untimeout(nbclosechunk, (caddr_t)nbp);

	/*
	 * Free the softc structure itself, then unlink it.
	 */
	freeb(nbp->nb_soft);
	q->q_ptr = NULL;

	(void) splx(s);
}

/*
 * Write-side put procedure.  Its main task is to detect ioctls
 * for manipulating the buffering state and hand them to nbioctl.
 * Other message types are passed on through.
 */
nbwput(q, mp)
	queue_t	*q;
	mblk_t	*mp;
{
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
		nbioctl(q,mp);
		break;

	default:
		putnext(q, mp);
		break;
	}
}


/*
 * Read-side put procedure.  It's responsible for buffering up incoming
 * messages and grouping them into aggregates according to the current
 * buffering parameters.
 */
nbrput(q, mp)
	queue_t	*q;
	mblk_t	*mp;
{
	register mblk_t		*mnp;
	register nb_softc_t	*nbp;

	/*
	 * Sanity check: make sure our corresponding device
	 * instance is still open.
	 */
	nbp = (nb_softc_t *)q->q_ptr;
	if (!nbp) {
		printf("nbrput: called on closed device instance\n");
		freemsg(mp);
		return;
	}

	switch (mp->b_datap->db_type) {

	case M_PROTO:
		/*
		 * Convert M_PROTO header to M_DATA header.
		 */
		for (mnp = mp; mnp && mnp->b_datap->db_type == M_PROTO;
		    mnp = mnp->b_cont)
			mnp->b_datap->db_type = M_DATA;
		/* Fall through... */

	case M_DATA:
		nbaddmsg(q, mp);
		break;

	case M_FLUSH:
		/*
		 * Symmetrically to M_FLUSH in nbwput,
		 * we only worry about FLUSHR here.
		 */
		if (*mp->b_rptr & FLUSHR) {
			/*
			 * Prevent interference from new messages
			 * arriving from below.
			 */
			register int	s = splstr();

			flushq(q, FLUSHDATA);
			/*
			 * Reset timeout, flush the chunk currently in
			 * progress, and start a new chunk.
			 */
			untimeout(nbclosechunk, (caddr_t)nbp);
			if (nbp->nb_mp) {
				freemsg(nbp->nb_mp);
				nbp->nb_mp = NULL;
				nbp->nb_mlen = 0;
				nbstartchunk(nbp);
			}
			if (nbp->nb_ticks > 0) {
				/*
				 * XXX:	Set new timeout or does it suffice
				 *	merely to start a new chunk?
				 */
				timeout(nbclosechunk, (caddr_t)nbp,
					nbp->nb_ticks);
			}
			(void) splx(s);
		}
		putnext(q, mp);
		break;

	default:
		putnext(q, mp);
		break;
	}
}

/*
 * Handle write-side M_IOCTL messages.
 */
nbioctl(q, mp)
	queue_t	*q;
	mblk_t	*mp;
{
	register nb_softc_t	*nbp = (nb_softc_t *)q->q_ptr;
	register struct iocblk	*iocp = (struct iocblk *)mp->b_rptr;
	register int		cmd = iocp->ioc_cmd;

#ifdef	NBDEBUG
	printf("nb %x M_IOCTL, cmd: %x\n", nbp, iocp->ioc_cmd);
#endif	NBDEBUG

	switch (cmd) {

	case NIOCSTIME: {
		register struct timeval *t;
		register long		ticks;

		/*
		 * Verify argument length.
		 */
		if (iocp->ioc_count < sizeof (struct timeval)) {
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			break;
		}

		t = (struct timeval *)mp->b_cont->b_rptr;
		if (t->tv_sec < 0 || t->tv_usec < 0) {
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			break;
		}
		ticks = (1000000 * t->tv_sec + t->tv_usec) * hz;
		ticks /= 1000000;
		nbp->nb_ticks = ticks;
		if (ticks == 0)
			nbp->nb_chunk = 0;
#ifdef	NBDEBUG
		printf("NIOCSTIME: ticks %d, sec %d, usec %d\n",
			nbp->nb_ticks, t->tv_sec, t->tv_usec);
#endif	NBDEBUG
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_count = 0;
		break;
	    }

	case NIOCGTIME: {
		register struct timeval *t;
		register long		usec;

		/*
		 * Verify argument length.
		 */
		if (iocp->ioc_count < sizeof (struct timeval)) {
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			break;
		}

		/*
		 * If infinite timeout, return range error
		 * for the ioctl.
		 */
		if (nbp->nb_ticks < 0) {
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = ERANGE;
			break;
		}

		usec = 1000000 * nbp->nb_ticks;
		usec /= hz;
		t = (struct timeval *)mp->b_cont->b_rptr;
		t->tv_usec = usec % 1000000;
		t->tv_sec = usec / 1000000;
#ifdef	NBDEBUG
		printf("NIOCGTIME: ticks %d, sec %d, usec %d\n",
			nbp->nb_ticks, t->tv_sec, t->tv_usec);
#endif	NBDEBUG
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_count = sizeof (struct timeval);
		break;
	    }

	case NIOCCTIME:
		nbp->nb_ticks = -1;
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_count = 0;
		break;

	case NIOCSCHUNK:
		/*
		 * Verify argument length.
		 */
		if (iocp->ioc_count < sizeof (int)) {
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			break;
		}

		nbp->nb_chunk = *(u_int *)mp->b_cont->b_rptr;
#ifdef	NBDEBUG
		printf("NIOCSCHUNK: chunk size %d\n", nbp->nb_chunk);
#endif	NBDEBUG
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_count = 0;
		break;

	case NIOCGCHUNK:
		/*
		 * Verify argument length.
		 */
		if (iocp->ioc_count < sizeof (int)) {
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			break;
		}

		*(u_int *)mp->b_cont->b_rptr = nbp->nb_chunk;
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_count = sizeof (u_int);
		break;

	default:
		/*
		 * Pass unrecognized ioctls through to
		 * give modules below us a chance at them.
		 */
		putnext(q, mp);
		return;
	}

	/*
	 * Send the response back upstream.
	 */
	qreply(q,mp);
}

/*
 * Given a length l, calculate the amount of extra storage
 * required to round it up to the next multiple of the alignment a.
 */
#define RoundUpAmt(l, a)	((l) % (a) ? (a) - ((l) % (a)) : 0)
/*
 * Calculate additional amount of space required for alignment.
 */
#define Align(l)		RoundUpAmt(l, sizeof (u_long))

/*
 * Augment the message given as argument with a nit_bufhdr header
 * and with enough trailing padding to satisfy alignment constraints.
 * Return a pointer to the augmented message, or NULL if allocation
 * failed.
 */
mblk_t *
nbaugmsg(mp)
	register mblk_t	*mp;
{
	register struct nit_bufhdr	*hp;
	register u_int			len = msgdsize(mp);
	register int			pad;

	/*
	 * Get space for the header.  If there's room for it in the
	 * message's leading dblk and it would be correctly aligned,
	 * stash it there; otherwise hook in a fresh one.
	 *
	 * Some slight sleaze: the last clause of the test relies on
	 * sizeof (struct nit_bufhdr) being a multiple of sizeof (u_int).
	 */
	if (mp->b_rptr - mp->b_datap->db_base >= sizeof (struct nit_bufhdr) &&
	    mp->b_datap->db_ref == 1 &&
	    ((u_int)mp->b_rptr & (sizeof (u_int) - 1)) == 0) {
		/*
		 * Adjust data pointers to precede old beginning.
		 */
		mp->b_rptr -= sizeof (struct nit_bufhdr);
	}
	else {
		register mblk_t	*mpnew;

		mpnew = allocb(sizeof (struct nit_bufhdr), BPRI_MED);
		if (!mpnew) {
			freemsg(mp);
			return ((mblk_t *)NULL);
		}
		mpnew->b_datap->db_type = M_DATA;
		mpnew->b_wptr += sizeof (struct nit_bufhdr);

		/*
		 * Link it in place.
		 */
		linkb(mpnew, mp);
		mp = mpnew;
	}

	/*
	 * Fill it in.
	 *
	 * We maintain the invariant: nb_m_len % sizeof (u_long) == 0.
	 * Since x % a == 0 && y % a == 0 ==> (x + y) % a == 0, we can
	 * pad the message out to the correct alignment boundary without
	 * having to worry about how long the currently accumulating
	 * chunk is already.
	 */
	hp = (struct nit_bufhdr *)mp->b_rptr;
	hp->nhb_msglen = len;
	len += sizeof (struct nit_bufhdr);
	pad = Align(len);
	hp->nhb_totlen = len + pad;

	/*
	 * Add padding.  If there's room at the end of the message
	 * (which there always should be so long as we retain the
	 * power-of-2 dblk allocation scheme), simply extend the wptr
	 * for the last mblk.  Otherwise allocate a new mblk and
	 * tack it on.
	 */
	if (pad) {
		register mblk_t	*mep;

		/*
		 * Find the message's last mblk.
		 */
		for (mep = mp; mep->b_cont; mep = mep->b_cont)
			continue;

		if (mep->b_datap->db_lim - mep->b_wptr >= pad &&
		    mep->b_datap->db_ref == 1)
			mep->b_wptr += pad;
		else {
			register mblk_t	*mpnew = allocb(pad, BPRI_MED);

			if (!mpnew) {
				freemsg(mp);
				return ((mblk_t *) NULL);
			}
			mpnew->b_datap->db_type = M_DATA;
			mpnew->b_wptr += pad;
			linkb(mep, mpnew);
		}
	}

	return (mp);
}

/*
 * Process a read-side M_DATA message.
 *
 * First condition the message for inclusion in a chunk, by adding
 * a nit_bufhdr header and trailing alignment padding.
 *
 * If the currently accumulating chunk doesn't have enough room
 * for the message, close off the chunk, pass it upward, and start
 * a new one.  Then add the message to the current chunk, taking
 * account of the possibility that the message's size exceeds the
 * chunk size.
 */
nbaddmsg(q, mp)
	queue_t	*q;
	mblk_t	*mp;
{
	register nb_softc_t	*nbp = (nb_softc_t *)q->q_ptr;
	register u_int		mlen,
				mnew;

	/*
	 * Add header and padding to the message.
	 */
	if (!(mp = nbaugmsg(mp))) {
#ifdef	NBDEBUG
		printf("nb %x: out of space for header or padding\n",
			nbp);
#endif	NBDEBUG
		return;
	}

	/*
	 * If the padded message won't fit in the current chunk,
	 * close the chunk off and start a new one.  The second
	 * clause of the test compensates for allocation failures
	 * in nbstartchunk the last time around.
	 */
	mlen = ((struct nit_bufhdr *)mp->b_rptr)->nhb_totlen;
	mnew = nbp->nb_mlen + mlen;
	if (mnew > nbp->nb_chunk || !nbp->nb_mp)
		nbclosechunk((caddr_t)nbp);

	/*
	 * If it still doesn't fit, pass it on directly, bypassing
	 * the chunking mechanism.  Note that if we're not chunking
	 * things up at all (chunk == 0), we'll pass the message
	 * upward here.
	 */
	mnew = nbp->nb_mlen + mlen;
	if (mnew > nbp->nb_chunk || !nbp->nb_mp) {
		putnext(q, mp);
		return;
	}

	/*
	 * We now know that there's a chunk in progress
	 * and that the message will fit in it.  Glue it
	 * in place.
	 */
	linkb(nbp->nb_mp, mp);
	nbp->nb_mlen = mnew;
}

/*
 * Start a fresh chunk.  If the chunk size is positive,
 * add a zero-length message at the beginning, to make
 * sure that nbclosechunk has something to deliver when
 * the timeout period expires with nothing having arrived.
 */
nbstartchunk(nbp)
	register nb_softc_t	*nbp;
{
	register mblk_t	*mp;

	if (nbp->nb_mp) {
		/*
		 * This should probably be a panic.
		 */
		printf("nb %x: nbstartchunk called with existent chunk\n",
			nbp);
		freemsg(nbp->nb_mp);
	}
	nbp->nb_mp = NULL;
	nbp->nb_mlen = 0;
	if (nbp->nb_chunk) {
		/*
		 * Set up the leading zero-length message.  If this
		 * fails, we'll be unable to deliver chunks until
		 * allocation once more succeeds, but then will revert
		 * back to accumulating chunks, since control will
		 * flow through nbclosechunk and from there to here.
		 */
		if (mp = allocb(0, BPRI_MED)) {
			mp->b_datap->db_type = M_DATA;
			nbp->nb_mp = mp;
		}
#ifdef	NBDEBUG
		else
			printf("nb %x: null chunk allocation failed\n",
				nbp);
#endif	NBDEBUG
	}
}

/*
 * Close off the currently accumulating chunk and pass
 * it upward.  Takes care of resetting timers as well.
 *
 * This routine is called both directly and as a result
 * of the chunk timeout expiring.
 */
nbclosechunk(arg)
	caddr_t	arg;
{
	register nb_softc_t	*nbp;
	register mblk_t		*mp;
	register queue_t	*q;
	register int		s;

	/*
	 * Prevent interference from timeouts expiring
	 * and from new messages being passed up from below.
	 */
	s = splstr();

	untimeout(nbclosechunk, arg);
	nbp = (nb_softc_t *)arg;
	mp = nbp->nb_mp;

	/*
	 * Guard against being in the middle of closing the stream
	 * when a timeout goes off and brings us here.
	 */
	q =  nbp->nb_rq;
	if (!q) {
		if (mp)
			freemsg(mp);
		nbp->nb_mp = NULL;
		(void) splx(s);
		return;
	}

	/*
	 * If there's currently a chunk in progress, close it off
	 * and send it up, provided that we won't violate flow control
	 * constraints by doing so.  The hard case here is handling
	 * data that we generate as opposed to data that come to us
	 * from below.  The only thing we generate is the zero-length
	 * message that heads a chunk, so if that's all there is, flow
	 * control applies.  If not, the module(s) feeding us from below
	 * should have checked canput, so we won't.  (If we checked
	 * as well, we could end up exceeding the chunk size.)
	 */
	if (mp && (nbp->nb_mlen != 0 || canput(q))) {
		putnext(q, mp);
		mp = nbp->nb_mp = NULL;
	}

	/*
	 * Re-establish the desired invariant that there's
	 * always a chunk in progress (subject to resource
	 * availability).
	 */
	if (!mp)
		nbstartchunk(nbp);

	/*
	 * Establish timeout until next message delivery time,
	 * but only if the user has requested timeouts.
	 */
	if (nbp->nb_ticks > 0)
		timeout(nbclosechunk, arg, nbp->nb_ticks);

	(void) splx(s);
}

#endif	NNBUF > 0
