#ifndef	lint
static char sccsid[] = "@(#)nit_if.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif	lint

/*
 * Streams driver interface to Ethernet
 *
 * This code is experimental, but becoming less so as time goes on.
 * Its intent is to supply a hook for experimenting with the streams
 * networking interface.
 *
 * The design is intended to support a packet-filtering streams
 * module layered immediately on top of this driver.  This setup
 * has the consequence that many packets potentially enter the
 * read queue that are destined to be thrown away immediately.
 * It would be nice to find ways to reduce or remove this inefficiency.
 */

/* #define	SNITDEBUG */

#include "snit.h"

#if	NSNIT > 0

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/debug.h>
#include <sys/user.h>
#include <sys/mbuf.h>			/* incoming packets are in mbufs */
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>

#include <net/nit.h>			/* for nit_ii */
#include <net/if.h>
#include <net/if_arp.h>
#include <net/nit_if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

struct mbuf	*cp_mblks_to_mbufs();

/*
 * Stereotyped streams glue.
 */

int	snit_open();
int	snit_close();		/* really void, not int */
int	snit_put();		/* really void, not int */

/*
 * Set high and low water marks.
 *
 * Since we don't supply service procedures, the high and low
 * water values given in snit_minfo are never examined.  However,
 * we do send these values to the stream head in an M_SETOPTS
 * message to alter its buffering behavior.  We send these messages
 * at open time and after getting a new snapshot length to use.
 *
 * One can think of these values as being composed from three pieces.
 * - The number of outstanding messages desired at high/low water.
 * - An estimate of the message size. (Using ETHERMTU here is actually
 *   overstating things.)
 * - A correction factor to balance number of packets against packet
 *   size.  This value grows as packet size decreases, but more slowly.
 *   It's really a seat-of-the-pants kludge.
 */
#define	SNIT_HIWAT(msgsize, fudge)	(32 * msgsize * fudge)
#define	SNIT_LOWAT(msgsize, fudge)	( 8 * msgsize * fudge)

#define	SNIT_PRE_PAD	16

struct module_info	snit_minfo = {
	19,			/* module id XXX: define real value */
	"snit",
	0,
	ETHERMTU + sizeof (struct ether_header),
	SNIT_HIWAT(ETHERMTU, 1),
	SNIT_LOWAT(ETHERMTU, 1)
};

struct qinit	snit_rinit = {
	NULL,		/* putp (only the "hardware" below us) */
	NULL,		/* srvp (snit_intr performs this role) */
	snit_open,
	snit_close,
	NULL,
	&snit_minfo,
	NULL
};

struct qinit	snit_winit = {
	snit_put,
	NULL,		/* srvp (everything handled in put proc) */
	NULL,
	NULL,
	NULL,
	&snit_minfo,
	NULL
};

struct streamtab	snit_info = {
	&snit_rinit,
	&snit_winit,
	NULL,
	NULL
};

/*
 * Per-device instance state information.
 *
 * Each instance is dynamically allocated from the dblk pool.
 * ni_mb cross-references back to the mblk whose associated dblk
 * contains the instance.  Open instances are linked together
 * through the ni_next field into a list ordered by the ni_unit
 * field.
 */
typedef struct ni_softc {
	struct ni_softc	*ni_next;	/* link to next open instance */
	mblk_t		*ni_mb;		/* cross-link to containing mblk */
	queue_t		*ni_rq;		/* cross-link to read queue */
	struct ifnet	*ni_ifp;	/* interface to which we're bound */
	u_long		ni_snap;	/* snapshot length */
	u_long		ni_flags;	/* as defined in <net/nit_if.h> */
	u_int		ni_drops;	/* packets dropped due to !canput */
	int		ni_unit;	/* minor dev # of this instance */
} ni_softc;

ni_softc	*nisoftc;		/* head of list of open instances */


/*
 * XXX: verify permissions for writing.
 *	(Or are the vnode's permissions good enough?)
 */
/* ARGSUSED */
int
snit_open(q, dev, oflag, sflag)
	struct queue	*q;
	dev_t		dev;
	int		oflag,
			sflag;
{
	register int		unit;
	register ni_softc	*nip;
	register ni_softc	**inspp;
	register mblk_t		*mp;

#ifdef	SNITDEBUG
	printf("snit_open: q: %x, WR(q): %x, dev: %x, oflag: %x, sflag: %x\n",
		q, WR(q), dev, oflag, sflag);
#endif	SNITDEBUG

	/*
	 * This is an exclusive open device, so a valid open request
	 * will require inserting a new device instance into the list
	 * rooted at nisoftc.  This list is kept sorted on the ni_unit
	 * field.  The code below finds an unused minor device number
	 * or verifies that the one we're given us free, depending on
	 * whether or not the open is a clone open.  While doing so,
	 * it records the list insertion point and checks for various
	 * error conditions.
	 */
	if (sflag == CLONEOPEN) {
		/*
		 * Clone open: find an unused minor device instance.
		 */
		unit = 0;
		for (inspp = &nisoftc; nip = *inspp; inspp = &nip->ni_next) {
			if (unit < nip->ni_unit)
				break;
			unit++;
		}
		if (unit == minor(0xffff) + 1) {
			/*
			 * All minor devices in use.
			 */
			u.u_error = ENXIO;		/* gag */
			return (OPENFAIL);
		}
	}
	else if (sflag) {
		/*
		 * Opening as a module is illegal.
		 */
		return (OPENFAIL);
	}
	else {
		/*
		 * Regular open: verify that the indicated minor
		 * device instance is free.
		 */
		unit = minor(dev);
		for (inspp = &nisoftc; nip = *inspp; inspp = &nip->ni_next) {
			if (unit < nip->ni_unit)
				break;
			if (unit > nip->ni_unit)
				continue;
			u.u_error = EBUSY;		/* gag */
			return (OPENFAIL);
		}
	}
	/*
	 * ASSERT: unit is in range and is currently unused, and
	 * inspp points to the proper list insertion point for
	 * unit's softc structure.
	 */

	/*
	 * Allocate space for per-open-instance information
	 * and set up internal cross-links.
	 *
	 * XXX:	If the allocation fails, I suppose we could
	 *	try a bufcall...
	 */
	mp = allocb((int) sizeof *nip, BPRI_MED);
	if (! mp) {
		u.u_error = ENOMEM;			/* gag */
		return (OPENFAIL);
	}
	mp->b_wptr += sizeof *nip;
	nip = (ni_softc *)mp->b_rptr;
	nip->ni_mb = mp;
	/*
	 * Link it into the list of open instances.
	 */
	nip->ni_next = *inspp;
	*inspp = nip;

	/*
	 * Set up cross-links between device and queues.
	 *	The streams framework uses (at least to the extent
	 *	of demanding non-garbage) the links from queues
	 *	back to device.
	 */
	nip->ni_rq = q;
	q->q_ptr = (char *)nip;
	WR(q)->q_ptr = (char *)nip;

	nip->ni_drops = 0;
	nip->ni_snap = 0;
	nip->ni_unit = unit;
	nip->ni_flags = 0;
	nip->ni_ifp = NULL;

	/*
	 * Send our notion of reasonable high and low water
	 * marks upstream to whichever module is willing to
	 * look at it.  If the allocation fails, we give up
	 * on the idea out of laziness.
	 *
	 * XXX:	Timing issue -- really, we would like this
	 *	message to reach the next module upstream
	 *	that supplies a service procedure.  However,
	 *	we have no guarantee that this module has
	 *	been pushed in place yet.  What to do?
	 *	(If we are to believe AT&T's documentation,
	 *	the issue is moot, since the documentation
	 *	claims that this message "alters some
	 *	characteristics of the stream head".)
	 */
	if (mp = allocb(sizeof (struct stroptions), BPRI_LO)) {
		register struct stroptions	*sop;

		mp->b_datap->db_type = M_SETOPTS;
		sop = (struct stroptions *)mp->b_rptr;
		sop->so_flags = SO_HIWAT | SO_LOWAT;
		sop->so_hiwat = SNIT_HIWAT(ETHERMTU, 1);
		sop->so_lowat = SNIT_LOWAT(ETHERMTU, 1);
		mp->b_wptr += sizeof (struct stroptions);

		putnext(q, mp);
	}

#ifdef	SNITDEBUG
	snitdump(nisoftc);
#endif	SNITDEBUG

	return (unit);
}

snit_close(q)
	register queue_t	*q;
{
	register ni_softc	*nip,
				*np;
	ni_softc		**delpp;
	register struct ifnet	*ifp;

#ifdef	SNITDEBUG
	printf("snit_close: q: %x, WR(q): %x ", q, WR(q));
#endif	SNITDEBUG

	/*
	 * Flush outstanding traffic on its way
	 * downstream to us.
	 */
	flushq(WR(q), FLUSHALL);

	nip = (ni_softc *)q->q_ptr;
	ifp = nip->ni_ifp;
#ifdef	SNITDEBUG
	printf("nip: %x, nip->ni_rq: %x\n", nip, nip->ni_rq);
	printf("     %d packets dropped to satisfy flow control\n",
		nip->ni_drops);
#endif	SNITDEBUG

	/*
	 * Undo our request for promiscuous mode if necessary.
	 */
	if ((nip->ni_flags & NI_PROMISC) && ifp)
		(void) ifpromisc(ifp, 0);

	/*
	 * Unhook the softc structure from the things that point
	 * to it and then free it.
	 */
	for (delpp = &nisoftc; np = *delpp; delpp = &np->ni_next)
		if (np == nip)
			break;
	if (!np)
		panic("snit_close nonexistent instance");
	*delpp = nip->ni_next;
	q->q_ptr = NULL;
	freeb(nip->ni_mb);

#ifdef	SNITDEBUG
	snitdump(nisoftc);
#endif	SNITDEBUG
}

/*
 * Write-side put procedure.  It handles the usual streams stuff and:
 *  - 	the NIOCBIND ioctl to bind us to a given interface, and
 *  - 	M_PROTO messages, whose bodies are handed to the interface
 *	for transmission, after stripping off the first mblk to
 *	obtain the destination address.
 */
snit_put(q, mp)
	queue_t	*q;
	mblk_t	*mp;
{
	switch (mp->b_datap->db_type) {

	case M_FLUSH:
#ifdef	SNITDEBUG
		printf("snit_put: M_FLUSH q: %x, mp: %x\n", q, mp);
#endif	SNITDEBUG
		if (*mp->b_rptr & FLUSHW) {
			/*
			 * Question: the lp example passed FLUSHDATA
			 * as arg to flushq; the emd driver passes
			 * FLUSHALL.  What are the implications of
			 * this difference (which amounts to preserving/
			 * not preserving control messages)?
			 */
			flushq(q, FLUSHALL);
			/*
			 * If we actually had a write in progress,
			 * we would want to flush local copies of
			 * pending outgoing messages as well.
			 */
			*mp->b_rptr &= ~FLUSHW;
		}
		if (*mp->b_rptr & FLUSHR) {
			/*
			 * N.B.: FLUSHW has been turned off by the
			 * time we get here.  (This is necessary to
			 * prevent a FLUSHW from circling back through
			 * the stream head to hit us a second time.)
			 */
			qreply(q, mp);
		}
		else
			freemsg(mp);
		break;

	case M_IOCTL:
		snit_ioctl(q, mp);
		break;

	case M_PROTO: {
		/*
		 * Transmit a packet.  Obtain the destination address
		 * from the M_PROTO block in front, and the body from
		 * the M_DATA blocks that follow.  If not in this format,
		 * return M_ERROR (EINVAL).
		 */
		struct sockaddr	*saddr = (struct sockaddr *)mp->b_rptr;
		mblk_t		*mnp = mp->b_cont;
		struct mbuf	*m;
		struct ifnet	*ifp;

		ifp = ((ni_softc *)q->q_ptr)->ni_ifp;

		if (!mnp || mnp->b_datap->db_type != M_DATA ||
		    (mp->b_wptr - mp->b_rptr) < sizeof (struct sockaddr) ||
		    ifp == NULL || saddr->sa_family != AF_UNSPEC) {
			mp->b_datap->db_type = M_ERROR;
			mp->b_rptr = mp->b_wptr = mp->b_datap->db_base;
			*mp->b_wptr++ = EINVAL;
			qreply(q, mp);
			break;
		}

		/*
		 * Convert body into the form the rest of the system expects.
		 */
		if (!(m = cp_mblks_to_mbufs(mnp))) {
			/*
			 * cp_mblks_to_mbufs has already freed
			 * the chain starting at mnp.
			 */
			freeb(mp);
			break;
		}
		/*
		 * Keep the data-link header fields in network order.
		 */

		/*
		 * Would like to report errors back up, but can't do it
		 * synchronously and sending up an M_ERROR is overkill.
		 */
		(void) (*ifp->if_output)(ifp, m, saddr);

		/*
		 * The interface output routine will (eventually)
		 * free the chain starting at mnp.
		 */
		freeb(mp);
		break;
	    }

	case M_DATA:
		/*
		 * Erroneous: we require an M_PROTO header at
		 * the front, so we know where to send it.
		 *
		 * Fall through...
		 */
	default:
		/*
		 * Is there anything else we should handle here?
		 */
		freemsg(mp);	/* Wing it. */
		break;
	}
}

/*
 * Handle write-side M_IOCTL messages.
 */
snit_ioctl(q, mp)
	queue_t	*q;
	mblk_t	*mp;
{
	register ni_softc	*nip = (ni_softc *)q->q_ptr;
	register struct iocblk	*iocp = (struct iocblk *)mp->b_rptr;
	register int		cmd = iocp->ioc_cmd;

#ifdef	SNITDEBUG
	printf("snit%d M_IOCTL, cmd: %x\n", nip->ni_unit, iocp->ioc_cmd);
#endif	SNITDEBUG

	/*
	 * If the ioctl requires that the stream already
	 * be bound to an interface, verify that it is.
	 */
	switch (cmd) {

	case SIOCGIFADDR:
	case SIOCADDMULTI:
	case SIOCDELMULTI:
	case NIOCSFLAGS:
		/*
		 * Return an error if we're not currently
		 * bound to an interface.
		 */
		if (!nip->ni_ifp) {
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = ENOTCONN;
			qreply(q, mp);
			return;
		}

	default:
		break;
	}

	/*
	 * Process the command.
	 */
	switch (cmd) {

	/*
	 * Ioctls bridging between this stream
	 * and the underlying 4.2 networking code.
	 */

	case SIOCGIFADDR:
	case SIOCADDMULTI:
	case SIOCDELMULTI: {
		register struct ifnet	*ifp = nip->ni_ifp;
		register struct ifreq	*ifr;

		/*
		 * Error if bad arg length or no ifp ioctl routine.
		 */
		if ((iocp->ioc_count < sizeof (struct ifreq)) ||
			(ifp->if_ioctl == NULL)) {
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			break;
		}
		
		ifr = (struct ifreq *)mp->b_cont->b_rptr;

		/*
		 * Have the interface fill in the info.
		 */
		iocp->ioc_error = (*ifp->if_ioctl)(ifp, cmd, ifr);
		if (iocp->ioc_error)
			mp->b_datap->db_type = M_IOCNAK;
		else {
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = sizeof *ifr;
		}

		break;
	    }

	/*
	 * Ioctls specific to this driver.
	 */

	case NIOCBIND: {
		/*
		 * Bind to a particular network interface.
		 *
		 * XXX:	Disallow changing after bound?  If not,
		 *	should consider serializing wrt incoming
		 *	packets, or else reporting interface id
		 *	of each packet (supposing that headers
		 *	are turned on).
		 *
		 *	Alternatively (and this sounds better),
		 *	expect the caller to flush the read queue
		 *	after changing binding.
		 */
		register struct ifnet	*ifp;
		register struct ifreq	*ifr;

		/*
		 * Verify argument length.
		 */
		if (iocp->ioc_count < sizeof (struct ifreq)) {
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			break;
		}

		ifr = (struct ifreq *)mp->b_cont->b_rptr;
		ifp = ifunit(ifr->ifr_name, sizeof ifr->ifr_name);
		if (!ifp) {
			/*
			 * Bogus interface.  Leave previous
			 * binding intact.
			 */
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = ENXIO;
			break;
		}

		/*
		 * Install the new binding and
		 * prepare to acknowledge the ioctl.
		 *
		 * XXX:	Flush previously queued packets?
		 *	Probably caller's responsibility.
		 */
		nip->ni_ifp = ifp;

		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_count = 0;

		break;
	    }

	/*
	 * Set new flag values.  Most of the work here is concerned
	 * with handling transitions into and out of promiscuous mode.
	 */
	case NIOCSFLAGS: {
		u_long		flags;
		register int	error = 0;

		/*
		 * Verify argument length.
		 */
		if (iocp->ioc_count < sizeof (long)) {
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			break;
		}

		flags = *(u_long *)mp->b_cont->b_rptr;

		if (flags & NI_PROMISC) {
			if (iocp->ioc_uid) {
				/*
				 * Insufficient privilege: reject.
				 */
				error = EPERM;
			}
			/*
			 * If promiscuous mode is being requested, ideally
			 * we should set u.u_acflag here, but can't.
			 */

			else if (!(nip->ni_flags & NI_PROMISC))
				error = ifpromisc(nip->ni_ifp, 1);

			if (error) {
				mp->b_datap->db_type = M_IOCNAK;
				iocp->ioc_error = error;
				break;
			}
		}
		else if (nip->ni_flags & NI_PROMISC) {
			/*
			 * Transition out of promiscuous mode.
			 */
			error = ifpromisc(nip->ni_ifp, 0);

			if (error) {
				mp->b_datap->db_type = M_IOCNAK;
				iocp->ioc_error = error;
				break;
			}
		}

		nip->ni_flags &= ~NI_USERBITS;
		nip->ni_flags |= (flags & NI_USERBITS);
#ifdef	SNITDEBUG
		printf("NIOCSFLAGS: new flags %x\n", nip->ni_flags);
#endif	SNITDEBUG
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_count = 0;
		break;
	    }

	case NIOCGFLAGS:

		/*
		 * Verify argument length.
		 */
		if (iocp->ioc_count < sizeof (long)) {
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			break;
		}

		*(u_long *)mp->b_cont->b_rptr = nip->ni_flags & NI_USERBITS;
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_count = sizeof (u_long);
		break;

	/*
	 * Set new snapshot length.  If we're not requested to pass
	 * everything up (snaplen == 0), we force it to be at least
	 * sizeof (struct ether_header), which simplifies life elsewhere.
	 *
	 * Since the snapshot length dramatically affects flow control
	 * characteristics, we reset the high and low water mark values
	 * based on the new snaplen.  This is a bit tacky.  A better way
	 * to proceed might be to define a new ioctl specifically for
	 * setting high and low water mark values.  This would have the
	 * advantage of letting the user take into account the entire
	 * set of modules pushed onto the stream.  (The facilities for
	 * setting flow control values properly for a given stream are
	 * really quite unsatisfactory, especially when trying to handle
	 * multiple modules with service procedures on the same stream.)
	 */
	case NIOCSSNAP: {
		mblk_t	*msp;

		/*
		 * Verify argument length.
		 */
		if (iocp->ioc_count < sizeof (long)) {
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			break;
		}

		nip->ni_snap = *(u_long *)mp->b_cont->b_rptr;
		if (nip->ni_snap != 0 &&
		    nip->ni_snap < sizeof (struct ether_header))
			nip->ni_snap = sizeof (struct ether_header);
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_count = 0;
		/*
		 * Build an M_SETOPTS message with revised flow control
		 * values and fire it upstream.  If allocation fails,
		 * give up in disgust.
		 */
		if (msp = allocb(sizeof (struct stroptions), BPRI_MED)) {
			register struct stroptions	*sop;
			int				snap,
							fudge;

			/*
			 * XXX Heuristic: Set fudge to allow more packets
			 * in the system for smaller packet sizes.  This
			 * is ugly and should be replaced by a better
			 * method of setting flow control characteristics.
			 */
			snap = nip->ni_snap != 0 ? nip->ni_snap : ETHERMTU;
			fudge = snap <= 100 ?	4 :
				snap <= 400 ?	2 :
						1;
			msp->b_datap->db_type = M_SETOPTS;
			sop = (struct stroptions *)msp->b_rptr;
			sop->so_flags = SO_HIWAT | SO_LOWAT;
			sop->so_hiwat = SNIT_HIWAT(snap, fudge);
			sop->so_lowat = SNIT_LOWAT(snap, fudge);
			msp->b_wptr += sizeof (struct stroptions);

			qreply(q, msp);
		}
		break;
	    }

	case NIOCGSNAP:
		/*
		 * Verify argument length.
		 */
		if (iocp->ioc_count < sizeof (long)) {
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			break;
		}

		*(u_long *)mp->b_cont->b_rptr = nip->ni_snap;
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_count = sizeof (u_long);
		break;

	default:
		/*
		 * We're at the end of the line and
		 * must NACK all unrecognized ioctls.
		 */
		mp->b_datap->db_type = M_IOCNAK;
		iocp->ioc_count = 0;
		break;
	}

	/*
	 * Send the response back upstream.
	 */
	qreply(q, mp);
}


/*
 * Feed-in from do_protocol().  Invoked at splimp.  From the
 * viewpoint of the streams subsystem, this routine acts like
 * a device interrupt routine.  (If this were a streams module
 * as opposed to driver, this would be the read-side service
 * procedure.)
 *
 * For now, at least, we use the same interface as that to
 * nit_tap.  Since the packet has already been stuffed into
 * an mbuf by this point, using this interface implies
 * extra copying.
 */

/*
 * Find next device instance to get a copy of the packet.
 * It must be open, on the same interface as the packet,
 * and have room for the packet in its queue.  The packet
 * must also be directed to this host or this device instance
 * must be in promiscuous mode.
 *
 * The macro also sneaks in some statistics gathering.
 */
#define	nextrecipient(nip, ifp, nif)	{ \
	for ( ; (nip); (nip) = (nip)->ni_next) \
		if ((nip)->ni_ifp == (ifp) && \
		    (!(nif)->nif_promisc || ((nip)->ni_flags & NI_PROMISC)) && \
		    (canput((nip)->ni_rq) ? 1 : ((nip)->ni_drops++, 0))) \
			break; \
}

snit_intr(ifp, m, nif)
	struct ifnet	*ifp;
	struct mbuf	*m;
	struct nit_if	*nif;
{
	mblk_t			*snit_cpmsg();
	register mblk_t		*mp,
				*mnp;
	mblk_t			*mhp = NULL;
	register ni_softc	*nip,
				*nnip;
	register u_long		cursnap;

	/*
	 * Get the first target device instance.  If there isn't
	 * one, we're done.
	 */
	nip = nisoftc;
	nextrecipient(nip, ifp, nif);
	if (!nip)
		return;

	/*
	 * Make the first message copy, recording how much of it we
	 * actually transcribe.  We use this value below to determine
	 * whether the next device instance needs a fresh copy or can
	 * share this one -- if the snaplens are the same, we can share.
	 *
	 * This strategy could be improved at the price of examining
	 * device instances in snaplen order.  We could also consider
	 * unconditional sharing with b_[rw]ptr values adjusted to
	 * reflect snaplens, but this could lead to excessive storage
	 * committment.  (The tradeoffs are unclear.  Consider a couple
	 * rarpd streams followed by an etherfind stream.  The rarpd
	 * instances need entire packets (snaplen == 0), but immediately
	 * throw most away.  If packet storage is shared with the etherfind
	 * instance, the upstream buffering module will gather together
	 * lots of big buffers with little of each used.)
	 */
	cursnap = nip->ni_snap;
	mp = snit_cpmsg(nif, m, cursnap);
	if (!mp) {
		nip->ni_drops++;
#ifdef	SNITDEBUG
		printf("snit%d: snit_cpmsg failed\n", nip->ni_unit);
#endif	SNITDEBUG
		return;
	}

	/*
	 * Loop through all open device instances, passing
	 * a copy of the packet to each.  At each iteration,
	 * if there's still another instance to go, produce
	 * a new copy before passing the current one on.
	 * This convolution is necessary to prevent the
	 * next module up in line from destroying the packet
	 * before we can copy it.
	 */
	for ( ; nip && mp; nip = nnip, mp = mnp) {
		nnip = nip->ni_next;
		nextrecipient(nnip, ifp, nif);

		if (nnip) {
			/* Make the next copy. */
			if (nnip->ni_snap == cursnap) {
				/* Sharable. */
				mnp = dupmsg(mp);
				if (!mnp) {
					nnip->ni_drops++;
#ifdef	SNITDEBUG
					printf("snit%d: dupmsg failed\n",
						nnip->ni_unit);
#endif	SNITDEBUG
				}
			}
			else {
				/* Need distinct copy (but see above). */
				mnp = snit_cpmsg(nif, m, nnip->ni_snap);
				if (!mnp) {
					nnip->ni_drops++;
#ifdef	SNITDEBUG
					printf("snit%d: snit_cpmsg failed\n",
						nnip->ni_unit);
#endif	SNITDEBUG
				}
				else
					cursnap = nnip->ni_snap;
			}
		}

		/*
		 * If the current instance expects any of the nit headers,
		 * allocate space for them and fill them in.  Unfortunately,
		 * the header can't share the dblk holding the packet itself,
		 * since the message types differ.  Since each instance can
		 * request different headers (and the drops header can have
		 * a different value for each instance), it's not feasible
		 * to share headers among instances either.
		 */
		if (nip->ni_flags & (NI_TIMESTAMP|NI_LEN|NI_DROPS)) {
			/*
			 * Calculate space required for header
			 * then allocate it.
			 */
			register int	hlen = 0;

			if (nip->ni_flags & NI_TIMESTAMP)
				hlen += sizeof (struct nit_iftime);
			if (nip->ni_flags & NI_DROPS)
				hlen += sizeof (struct nit_ifdrops);;
			if (nip->ni_flags & NI_LEN)
				hlen += sizeof (struct nit_iflen);
			hlen += SNIT_PRE_PAD;
			mhp = allocb(hlen, BPRI_MED);
			if (!mhp) {
				freemsg(mp);
				nip->ni_drops++;
#ifdef	SNITDEBUG
				printf("snit%d: len %d (ifhdr) allocb failed\n",
					nip->ni_unit, hlen);
#endif	SNITDEBUG
				continue;
			}
			mhp->b_rptr += SNIT_PRE_PAD;
			mhp->b_wptr += SNIT_PRE_PAD;

			/*
			 * Fill it in.  Ordering of the individual
			 * headers is important and must correspond
			 * to the commentary in nit_if.h.
			 */
			mhp->b_datap->db_type = M_PROTO;
			if (nip->ni_flags & NI_TIMESTAMP) {
				struct nit_iftime	*ntp;

				ntp = (struct nit_iftime *)mhp->b_wptr;
				uniqtime(&ntp->nh_timestamp);
				mhp->b_wptr += sizeof (struct nit_iftime);
			}
			if (nip->ni_flags & NI_DROPS) {
				struct nit_ifdrops	*ndp;

				ndp = (struct nit_ifdrops *)mhp->b_wptr;
				ndp->nh_drops = nip->ni_drops;
				mhp->b_wptr += sizeof (struct nit_ifdrops);
			}
			if (nip->ni_flags & NI_LEN) {
				struct nit_iflen	*nlp;

				nlp = (struct nit_iflen *)mhp->b_wptr;
				nlp->nh_pktlen =
					nif->nif_hdrlen + nif->nif_bodylen;
				mhp->b_wptr += sizeof (struct nit_iflen);
			}

			/*
			 * Paste the headers on to the front of the packet.
			 */
			linkb(mhp, mp);
			mp = mhp;
		}

		/*
		 * Shove the packet into the next module's read queue.
		 */
		putnext(nip->ni_rq, mp);
	}
}

#undef	nextrecipient


/*
 * Copy the first snaplen bytes of the packet denoted by nif (both
 * header and body) into mblks.  See the comments below for more
 * details on the semantics of snaplen.
 *
 * We attempt to use buffer space as efficiently as possible
 * by using a single dblk for the header and body, placing them
 * at the end of the dblk (but leaving enough room at the end
 * to maintain correct alignment for the start of the header).
 * This placement maximizes the chance that there's sufficient
 * space at the front to prepend headers.
 *
 * This strategy is based on knowledge of expected use (it's
 * likely that additional headers will be pasted on at the
 * beginning, as opposed to the more usual case of having them
 * be stripped off; the packet body should be contiguous to make
 * life easier for the packet filtering module that's likely to
 * be next in line; etc.) and observations drawn from previous
 * versions that exposed trouble with running out of buffer
 * resources.
 */
mblk_t *
snit_cpmsg(nif, m, snaplen)
	register struct nit_if	*nif;
	struct mbuf		*m;
	register u_long		snaplen;
{
	register mblk_t	*mp;
	register int	blen,
			rlen;

	/*
	 * Get rounded length, taking the snapshot length into account.
	 * Note that a snapshot length of zero implies "deliver the whole
	 * thing".  Nonzero values are constrained elsewhere to be large
	 * enough that the entire link level header is always included.
	 *
	 * precondition: snaplen >= sizeof (struct ether_header)
	 */
	blen = nif->nif_bodylen;
	if (snaplen != 0) {
		if (blen > snaplen - sizeof (struct ether_header))
			blen = snaplen - sizeof (struct ether_header);
	}
	rlen = roundup(blen + nif->nif_hdrlen, sizeof (u_long));

	/*
	 * Get a dblk and arrange to copy to its end.
	 */
	if (!(mp = allocb(rlen, BPRI_MED)))
		return (NULL);
	mp->b_datap->db_type = M_DATA;
	mp->b_rptr = mp->b_wptr = mp->b_datap->db_lim - rlen;

	/*
	 * Transcribe packet header.
	 */
	bcopy(nif->nif_header, (caddr_t)mp->b_wptr,
		nif->nif_hdrlen);
	mp->b_wptr += nif->nif_hdrlen;

	/*
	 * Transcribe packet body.
	 */
	if (blen > 0) {
		(void) m_cpytoc(m, 0, blen, (caddr_t)mp->b_wptr);
		mp->b_wptr += blen;
	}

	return (mp);
}


/*
 * XXX:	move these routines to a place where other drivers/modules
 *	can take advantage of them.
 */


/*
 * Free the contents of an MCL_LOANED mbuf constructed
 * with cp_mblks_to_mbufs below; i.e., free the underlying
 * mblk.
 *
 * N.B.: only the mblk itself is freed; anything it points
 * to is left intact.
 */
free_mbuffed_mblk(arg)
	int	arg;
{
	freeb((mblk_t *)arg);
}

/*
 * Copy the mblk chain headed at mp into mbufs by wrapping each
 * mblk into an MCL_LOANED mbuf.  Ignore the mblk b_next field,
 * following only the b_cont link.  Set the type of all the resulting
 * mbufs to MT_DATA.
 *
 * N.B.: it is the responsibility of the ultimate consumer of the
 * resulting mbuf chain to free it.  Since the chain consists of
 * MCL_LOANED mbufs, freeing it will free the original mblk chain
 * as well, so the caller should _not_ free the original chain.
 *
 * If resources aren't immediately available, return NULL.
 */
struct mbuf *
cp_mblks_to_mbufs(mp)
	mblk_t	*mp;
{
	struct mbuf		*head = NULL;
	register struct mbuf	*m,
				**mm;
	register mblk_t		*mnp;

	mm = &head;
	for (mnp = mp; mnp; mnp = mnp->b_cont) {
		m = mclgetx(free_mbuffed_mblk, (int)mnp,
			(caddr_t)(mnp->b_rptr), mnp->b_wptr - mnp->b_rptr,
			M_DONTWAIT);
		if (!m) {
			/*
			 * Must free the mbuf chain we have so far
			 * (and by extension, the mblks embedded in
			 * it), and the unconsumed part of the mblk
			 * chain.
			 */
			if (head)
				m_freem(head);
			freemsg(mnp);
			return (NULL);
		}

		*mm = m;
		mm = &m->m_next;
	}
	return (head);
}

#ifdef	SNITDEBUG
/*
 * Dump the addresses and unit numbers of ni_softc instances
 * present on the list rooted at nip.
 */
snitdump(nip)
	register ni_softc	*nip;
{
	printf("  snitdump:\n");
	while (nip) {
		printf("  %x\t%d\n", nip, nip->ni_unit);
		nip = nip->ni_next;
	}
}
#endif	SNITDEBUG

#endif	NSNIT > 0
