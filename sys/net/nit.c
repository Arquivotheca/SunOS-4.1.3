#ifndef lint
static	char sccsid[] = "@(#)nit.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifdef NITDEBUG
int nit_debug = 0;
#endif NITDEBUG

/*
 * The socket based implementation of nit precludes a large simple
 * circular buffer for reading, since data must be waiting at the
 * socket in mbufs.  The choices I see to implement such are to (1)
 * add an ioctl to trigger the ferrying of the data to the socket, and
 * use it before the read;  (2) have the buffer attached to the
 * socket, and muck with it in place; or (3) look to see if a select
 * or a read is in progress during filling or timeout processing
 * (timeouts would engender an artificial delay); all are disgusting.
 * I now wonder if the special character device which used ioctls to
 * select the interface might not be a better way to go.
 *
 * We must use rawintr to deliver the packets, since we must avoid
 * mucking with the sockbuf at splimp.
 */
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/mbuf.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/route.h>
#include <net/raw_cb.h>
#include <net/nit.h>

/*
 * NIT protocol family.
 */
extern	int nit_output();
extern	int nit_init();
extern	int nit_slowtimo();
extern	int nit_usrreq();

extern	struct domain nitdomain;

struct protosw nitsw[] = {
{ 0,		&nitdomain,	0,		0,
  0,		0,		0,		0,
  0,
  nit_init,	0,		0,		0,
},
{ SOCK_RAW,	&nitdomain,	NITPROTO_RAW,	PR_ATOMIC|PR_WANTRCVD,
  0,		nit_output,	0,		0,
  nit_usrreq,
  0,		0,		nit_slowtimo,	0,
},
};

struct domain nitdomain =
    { AF_NIT, "nit", 0, 0, 0, nitsw, &nitsw[sizeof(nitsw)/sizeof(nitsw[0])] };

/*
 * Nit Routines
 *	provide the protocol to support nit
 * XXX:
 *	code knows that cluster mbufs are used to collect packets
 *	should the storing (vice filtering) be done at splnet?
 *	buffer sizes are rounded up to multiples of MCLBYTES
 *
 *	Users can set both the buffer space (maximum total data to
 *	buffer internally) and the chunksize (size of chunk to ship
 *	to a socket as a single record).  Returned packets are not
 *	restricted to be less than MCLBYTES of data.  This means we
 *	must handle chains of mbufs.  We do so with ncb_head and
 *	ncb_tail and ncb_filled.
 *
 *	To avoid wasting space in an mcluster when ferrying data to a
 *	socket, say if the space remaining in the mcluster were large
 *	or the packet to be buffered would fit in one less mcluster if
 *	we used the remaining space, we could clone the mcluster and
 *	ferry one copy (which held the end of the previous packet) to
 *	the socket and continue filling the other copy (with the start
 *	of the new packet).  Ncb_skipped could indicate the number of
 *	bytes unavailable for storage (since it was in use in the
 *	ferried mbuf).  But this would further complicate the code,
 *	and handling more than MCLBYTES of data with packets packed
 *	buffers is tough enough for now.
 */

/* Internal control block -- one per bound socket */
struct nit_cb {
	struct	nit_cb *ncb_next;	/* next nit_cb */
	struct	nit_cb *ncb_prior;	/* previous nit_cb */
	struct	rawcb *ncb_rcb;		/* backpointer to associated rcb */
	struct	ifnet *ncb_if;		/* backpointer to bound interface */
	struct	mbuf *ncb_head;		/* first mbuf for returned packets */
	struct	mbuf *ncb_tail;		/* last mbuf for returned packets */
	u_int	ncb_filled;		/* amount of buffer filled so far */
	int	ncb_state;		/* current state of tap */
	int	ncb_seqno;		/* current buffer sequence number */
	int	ncb_dropped;		/* count of dropped packets */
	u_int	ncb_typetomatch;	/* packet type field to match */
	int	ncb_snaplen;		/* length of data to capture */
	int	ncb_timodelay;		/* periodic flush delay in slowtimo */
	int	ncb_timoleft;		/* timeouts to go before flush */
	int	ncb_flags;		/* set from nioc_flags */
	u_int	ncb_chunksize;		/* set from nioc_chunksize */
	u_int	ncb_bufalign;		/* set from nioc_bufalign */
	u_int	ncb_bufoffset;		/* set from nioc_bufoffset */
};

struct nit_cb nitcb;			/* master nit_cb */
#define nitcb_promisc nitcb.ncb_state	/* count of promiscuous sockets */

/*
 * Return the next alignment point at or beyond old,
 * according to the alignment information carried in *ncb.
 */
u_int
nit_align(old, ncb)
	register u_int		old;
	register struct nit_cb	*ncb;
{
	register u_int 	align = ncb->ncb_bufalign;
	register u_int	new;

	/* Get last alignment multiple at or before bp, then bias by offset. */
	new = old - (old % align) + ncb->ncb_bufoffset;

	/* Adjust it to point at or beyond bp. */
	if (new < old)
		new += align;

	return (new);
}

/*
 * The nit tap routine, invoked by the interfaces.
 * Assumes called at splimp.
 */
nit_tap(ifp, m, nii)
	struct ifnet *ifp;
	struct mbuf *m;
	struct nit_ii *nii;
{
	register struct nit_cb *ncb;
	register int datalen;

	for (ncb = nitcb.ncb_next; ncb != &nitcb; ncb = ncb->ncb_next) {
		if (ncb->ncb_state == NIT_QUIET)
			continue;
		if (nii->nii_promisc && (ncb->ncb_flags&NF_PROMISC) == 0)
			continue;
		if (ncb->ncb_if != ifp)
			continue;

		datalen = nii->nii_hdrlen + nii->nii_datalen;

		switch (ncb->ncb_typetomatch) {
		case NT_ALLTYPES:
			/* capture some of every packet */
			datalen = MIN(datalen, ncb->ncb_snaplen);
			break;
		default:
			if (ncb->ncb_typetomatch != nii->nii_type)
				continue;
			/* capture all of some packets */
			break;
		}

		/*
		 * See whether adding this packet will push the current
		 * record over the chunksize threshold.  If so, close
		 * off the current record and start a new one.  In the
		 * worst case, we need room for two nit headers.
		 */
		if (ncb->ncb_state == NIT_CATCH) {
			register u_int start1;

			start1 = nit_align(ncb->ncb_filled, ncb) +
				sizeof (struct nit_hdr);
			if (nit_align(start1, ncb) + sizeof (struct nit_hdr) +
			    datalen > ncb->ncb_chunksize)
				nit_refill(ncb);
		}

		/* retest, since nit_refill could fail */
		if (ncb->ncb_state != NIT_CATCH ||
		    nit_fillbuf(ncb, nii, m, datalen))
			ncb->ncb_dropped++;
	}
}

/*
 * Fill the buffer; return 0 iff success.
 * Assumes called with enough room left in the current chunk
 * to hold the new packet and its associated headers.
 * (However, after adding new headers from state changes, etc.,
 * there might _not_ be enough room in the current chunk.)
 */
nit_fillbuf(ncb, nii, m, datalen)
	register struct nit_cb *ncb;
	struct nit_ii *nii;
	register struct mbuf *m;
	register int datalen;
{
	struct nit_hdr nh;
	struct mbuf *tail = ncb->ncb_tail;
	int mlen = tail->m_len;
	u_int filled = ncb->ncb_filled;
	int timoleft = ncb->ncb_timoleft;
	register u_int len;

	nit_fillhdr(ncb, &nh, nii);
	nh.nh_datalen = datalen;

	if (nit_addhdr(ncb, &nh))
		goto drop;

	/* Transcribe the packet's header. */
	len = MIN(datalen, nii->nii_hdrlen);
	if (len > 0) {
		if (nit_cp(ncb, (caddr_t) nii->nii_header, len))
			goto drop;
		datalen -= len;
	}

	/*
	 * Transcribe the packet's body.
	 */
	while (m && datalen > 0) {
		len = MIN(datalen, m->m_len);
		if (nit_cp(ncb, mtod(m, caddr_t), len))
			goto drop;
		m = m->m_next;
		datalen -= len;
	}

	ncb->ncb_flags |= NF_BUSY;
#ifdef NITDEBUG
	if (ncb->ncb_filled > ncb->ncb_chunksize)
		printf("nit: overfilled chunk: %d (max should be %d)\n",
			ncb->ncb_filled, ncb->ncb_chunksize);
#endif NITDEBUG
	if (ncb->ncb_filled >= ncb->ncb_chunksize ||
	    (ncb->ncb_flags&NF_TIMEOUT && ncb->ncb_timodelay == 0) )
		nit_refill(ncb);
	return (0);

drop:
#ifdef NITDEBUG
	if (nit_debug > 2) {
		printf("nit drop, filled %d -> %d\n", ncb->ncb_filled, filled);
	}
#endif NITDEBUG
	m_freem(tail->m_next);
	tail->m_next = 0;
	tail->m_len = mlen;
	ncb->ncb_tail = tail;
	ncb->ncb_filled = filled;
	ncb->ncb_timoleft = timoleft;
	(void) nit_ferry(ncb);			/* since state change */
	return (1);
}

/*
 * Copy data into the mbuf chain holding an accumulating nit chunk.
 * Assumes that there is some storage already allocated for the chunk
 * and that nit_getbuf allocates storage in units of MCLBYTES.  Also
 * assumes that if nit_getbug is called, it doesn't induce another
 * header.  (This means that when nit_cp is called to add a header,
 * there must already be enough room to hold it.)
 * If the routine fails due to allocation failure the caller is
 * responsible for freeing storage for the partially copied data.
 * Returns 0 on success, the error code from nit_getbuf on failure.
 */
nit_cp (ncb, buf, totlen)
	struct nit_cb	*ncb;
	caddr_t		buf;
	u_int		totlen;
{
	register struct mbuf	*m = ncb->ncb_tail;
	register caddr_t	cp = buf;
	register u_int		len;

	while (totlen > 0) {
		/*
		 * If we've totally exhausted our currently
		 * allocated space, get more.
		 */
		if ((len = MCLBYTES - m->m_len) == 0) {
			register int	retcode = nit_getbuf(ncb);

			if (retcode)
				return (retcode);
			if (! (m = m->m_next))
				panic("nit_cp");
			/*
			 * Sanity check: verify that the space just
			 * allocated is unoccupied (i.e., that nit_getbuf
			 * didn't add a nit header to it).
			 */
			if (m->m_len != 0)
				panic("nit_cp extra header");
			len = MCLBYTES;
		}
		/* Copy as much additional as we can. */
		len = MIN(totlen, len);
		bcopy(cp, mtod(m, caddr_t) + m->m_len, len);
		m->m_len += len;
		cp += len;
		ncb->ncb_filled += len;
		totlen -= len;
	}
	return (0);
}

/* Fill in the nit_hdr structure and update timer */
nit_fillhdr(ncb, nh, nii)
	register struct nit_cb *ncb;
	struct nit_hdr *nh;
	struct nit_ii *nii;
{
	nh->nh_timestamp = time;
	nh->nh_state = ncb->ncb_state;
	switch (ncb->ncb_state) {
	case NIT_CATCH:
		nh->nh_wirelen = nii->nii_hdrlen + nii->nii_datalen;
		if (ncb->ncb_timoleft == 0) /* first packet in block */
			ncb->ncb_timoleft = ncb->ncb_timodelay;
		break;
	case NIT_NOSPACE:
	case NIT_NOMBUF:
	case NIT_NOCLUSTER:
		nh->nh_dropped = ncb->ncb_dropped;
		break;
	case NIT_SEQNO:
		nh->nh_seqno = ncb->ncb_seqno++;
		break;
	default:
		panic("nit_fillhdr");
	}
#ifdef NITDEBUG
	if (nit_debug > 2 && ncb->ncb_state != NIT_CATCH)
		printf("nit_hdr: state=%d\n", ncb->ncb_state);
#endif NITDEBUG
}

/*
 * Copy a nit header into the internal buffer associated with ncb,
 * taking alignment constraints into account.  Return zero on success,
 * nonzero otherwise.
 */
nit_addhdr(ncb, nhp)
	register struct nit_cb	*ncb;
	struct nit_hdr		*nhp;
{
	register struct mbuf	*m,
				*mnext;
	caddr_t			cp;
	register u_int		skip,
				remlen,
				align_to;

recalc:
	cp = (caddr_t) nhp;
	/*
	 * See whether we have enough space left to hold the
	 * header, taking alignment constraints into account.
	 * If there's not enough and we have to grab more space,
	 * recalculate, because nit_getbuf may have added a
	 * header of its own.  If the alignment values aren't
	 * constrained correctly, there's potential here for
	 * infinite loops and recursion (between this routine
	 * and nit_getbuf).
	 */
	align_to = nit_align(ncb->ncb_filled, ncb);
	skip = align_to - ncb->ncb_filled;
	m = ncb->ncb_tail;
	if ((u_int) (m->m_len) + skip + sizeof *nhp > MCLBYTES) {
		/* Insufficient space -- get more. */
		if (nit_getbuf(ncb))
			return (1);	/* no space available */
		/* There should now be exactly one new mbuf on the chain. */
		if (! (mnext = m->m_next) || mnext != ncb->ncb_tail)
			panic("nit_addhdr");
		/* See whether another header's slipped in. */
		if (mnext->m_len > 0)
			goto recalc;
		/*
		 * Skip over remaining part of original mbuf and
		 * set up for the next one.
		 */
		remlen = MCLBYTES - m->m_len;
		if (remlen >= skip) {
			/*
			 * The original mbuf had enough room left in it
			 * to soak up the amount to skip for alignment.
			 * Move over the skip amount.  Any space then left
			 * in m will be unused -- the copy itself will
			 * start at the beginning of mnext.
			 */
			m->m_len += skip;
			ncb->ncb_filled += skip;
			skip = 0;
		}
		else {
			/*
			 * The amount to skip spills over into
			 * the new mbuf.
			 */
			ncb->ncb_filled += remlen;
			m->m_len = MCLBYTES;
			skip -= remlen;
		}
		m = mnext;
	}

	/* Copy the rest. */
	m->m_len += skip;
	ncb->ncb_filled += skip;
	if (nit_cp(ncb, cp, sizeof *nhp))
		return (1);

	return (0);
}

nit_refill(ncb)
	struct nit_cb *ncb;
{
#ifdef NITDEBUG
	extern int (*caller())();

	if (nit_debug > 1)
	    printf("nit_refill caller=0x%x chunksize=%d filled=%d\n",
	        caller(), ncb->ncb_chunksize, ncb->ncb_filled);
#endif NITDEBUG

	if (nit_ferry(ncb))
		(void) nit_getbuf(ncb);
}

/*
 * Move the data-in-the-buffer toward the socket.
 * Returns 1 iff no buffer remains.
 */
nit_ferry(ncb)
	register struct nit_cb *ncb;
{
	struct sockproto sp;
	struct rawcb *rcb = ncb->ncb_rcb;

	if (! (ncb->ncb_flags&NF_BUSY)) {
#ifdef NITDEBUG
		if (nit_debug)
			printf("nit_ferry: void\n");
#endif NITDEBUG
		return (ncb->ncb_head == NULL);
	}

	if (ncb->ncb_head == NULL)
		panic("nit_ferry");

	sp.sp_family = PF_NIT;
	sp.sp_protocol = NITPROTO_RAW;
#ifdef NITDEBUG
	/* Verify that the tail pointer points where it should. */
	if (ncb->ncb_tail->m_next)
		panic("nit_ferry next");
	else {
		register struct mbuf *m;

		for (m = ncb->ncb_head; m->m_next; m = m->m_next)
			continue;
		if (m != ncb->ncb_tail)
			panic("nit_ferry tail");
	}
	/* Verify that the chunksize hasn't been exceeded. */
	if (nit_debug && ncb->ncb_filled > ncb->ncb_chunksize) {
		printf("nit_ferry: filled: %d, chunksize: %d\n",
			ncb->ncb_filled, ncb->ncb_chunksize);
	}
#endif NITDEBUG
	m_tally(ncb->ncb_head, 1, &rcb->rcb_cc, &rcb->rcb_mbcnt);
	raw_input(ncb->ncb_head, &sp, &rcb->rcb_faddr, &rcb->rcb_laddr);
	ncb->ncb_head = ncb->ncb_tail = 0;
	ncb->ncb_filled = 0;
	ncb->ncb_flags &= ~NF_BUSY;
	ncb->ncb_timoleft = 0;

	return (1);
}

/*
 * Attempt to acquire an additional MCL_STATIC cluster mbuf for packets.
 * Returns 0 on success, UNIX errno on failure.  Note that the net amount
 * of new space acquired may be less than MCLBYTES due to nit headers
 * added from state transitions detected herein.
 * XXX pass in wait/dontwait?
 */
nit_getbuf(ncb)
	register struct nit_cb *ncb;
{
	register struct rawcb *rcb = ncb->ncb_rcb;
	struct socket *so = rcb->rcb_socket;
	register struct mbuf *m = 0;
	struct nit_hdr nh;
	register int state;

	if (sbspace(&so->so_rcv) <
	    ncb->ncb_filled + MCLBYTES + MAX(rcb->rcb_mbcnt, rcb->rcb_cc))
		state = NIT_NOSPACE;
	else {
		m = m_get(M_DONTWAIT, MT_DATA);
		if (m == NULL)
			state = NIT_NOMBUF;
		else
			if (mclget(m)) {
				m->m_len = 0; /* NO data yet! */
				if (ncb->ncb_tail) {
					ncb->ncb_tail->m_next = m;
					ncb->ncb_tail = m;
				} else {
					/* Start of a new chunk */
					ncb->ncb_head = ncb->ncb_tail = m;
					state = ncb->ncb_state;
					ncb->ncb_state = NIT_SEQNO;
					nit_fillhdr(ncb, &nh,
					    (struct nit_ii *)0);
					ncb->ncb_state = state;
					if (nit_addhdr(ncb, &nh))
						panic("nit_getbuf");
				}
				state = NIT_CATCH;
			} else {
				m_freem(m);
				m = NULL;
				state = NIT_NOCLUSTER;
			}
	}
	/*
	 * At this point, the nit control block still records the
	 * old state, and the variable state the new.
	 */

	if (state == NIT_CATCH && ncb->ncb_state != NIT_CATCH) {
		/*
		 * Transition to CATCH state.  Add a header indicating
		 * the change.  Note that it's important that we be on
		 * a packet boundary whenever this transition occurs,
		 * since there's no mechanism for going back to adjust
		 * the datalen indicated in the packet's nit header.
		 */
#ifdef NITDEBUG
		if (nit_debug)
			printf("nit state change from %d to %d\n",
				ncb->ncb_state, state);
#endif NITDEBUG
	    	if (ncb->ncb_state != NIT_QUIET) {
			/* indicate state transition to user */
			nit_fillhdr(ncb, &nh, (struct nit_ii *)0);
			if (nit_addhdr(ncb, &nh))
				panic("nit_getbuf 2");
			ncb->ncb_flags |= NF_BUSY;
		}
		ncb->ncb_state = state;
	}

	if (state != NIT_CATCH && ncb->ncb_state == NIT_CATCH) {
		/* Transition from CATCH state. */
#ifdef NITDEBUG
		if (nit_debug)
			printf("nit state change from %d to %d\n",
				ncb->ncb_state, state);
#endif NITDEBUG
		ncb->ncb_state = state;
		ncb->ncb_dropped = 0;
	}
#ifdef NITDEBUG
	if (nit_debug > 2)
		printf("nit_getbuf() = %d\n", m == NULL);
#endif NITDEBUG

	return (m == NULL ? ENOBUFS : 0);
}

/*
 * Nit_cb allocator and initializer;
 * returns 0 on success, else UNIX error code.
 */
nit_cballoc(rcb)
	register struct rawcb *rcb;
{
	register struct nit_cb *ncb;
	register struct mbuf *m;
	register int s;

#ifndef lint
	/* Sanity check... */
	if (sizeof(struct nit_cb) > MLEN || NITBUFSIZ != MCLBYTES)
		return (ENOPROTOOPT);
#endif lint

	m = m_getclr(M_WAIT, MT_PCB);
	ncb = mtod(m, struct nit_cb *);
	ncb->ncb_rcb = rcb;
	ncb->ncb_if = nit_ifwithaddr(&rcb->rcb_laddr);
	ncb->ncb_chunksize = MCLBYTES;
	ncb->ncb_bufalign = 1;
	ncb->ncb_bufoffset = 0;

	rcb->rcb_flags |= RAW_DONTROUTE|RAW_TALLY;
	rcb->rcb_pcb = (caddr_t)ncb;
	((struct sockaddr_nit *)&rcb->rcb_laddr)->snit_cookie = (caddr_t)ncb;

	s = splimp();
	insque(ncb, &nitcb);
	(void) splx(s);

	return (0);
}

nit_cbfree(ncb)
	register struct nit_cb *ncb;
{
	register int	s;
	register int	error;

	/*
	 * If a previous bind call failed, it's possible that we
	 * never made it to the point of allocating a nit_cb.
	 */
	if (!ncb)
		return;

	s = splimp();
	remque(ncb);
	(void) splx(s);
	m_freem(ncb->ncb_head);
	error = nit_promisc(ncb, ncb->ncb_flags, 0);
	if (error)
		printf("nit_promisc error %d\n", error);
	m_freem(dtom(ncb));
}

nit_slowtimo()
{
	register struct nit_cb *ncb;
	int s = splimp();

	for (ncb = nitcb.ncb_next; ncb != &nitcb; ncb = ncb->ncb_next) {
		switch (ncb->ncb_state) {
		case NIT_NOMBUF:
		case NIT_NOCLUSTER:
			break;
		case NIT_CATCH:
			/* vice NF_BUSY, since only doing honest data packets */
			if (ncb->ncb_timoleft > 0 && --ncb->ncb_timoleft == 0)
				break;
			/* FALL THRU */
		default:
			continue;
		}

#ifdef NITDEBUG
		if (nit_debug > 2)
			printf("nit_slowtimo with 0x%x\n", ncb);
#endif NITDEBUG
		nit_refill(ncb);
	}
	(void) splx(s);
}

/* Initialize the nit world */
nit_init()
{
	nitcb.ncb_next = nitcb.ncb_prior = &nitcb;
}

/*
 * Perform various control functions.
 * Returns 0 on success, UNIX errnos on failure.
 *
 * XXX:	Does the whole thing really have to run at splimp?
 */
nit_usrreq(so, req, m, nam, opt)
	struct socket	*so;
	int		req;
	struct mbuf	*m,
			*nam,
			*opt;
{
	register struct rawcb	*rcb = sotorawcb(so);
	register struct nit_cb	*ncb = NULL;
	register int		error = 0;
	register int		s;

	if (rcb)
		ncb = (struct nit_cb *)rcb->rcb_pcb;

	s = splimp();

	/*
	 * We handle most requests by passing them directly to
	 * raw_usrreq.  We deal with most of the rest by augmenting
	 * the raw action with a bit of special-purpose nit stuff;
	 * this is a bit distasteful, as we must keep this code
	 * in sync with the corresponding code in raw_usrreq().
	 * PRU_CONTROL is the only request that has no raw action.
	 */
	switch (req) {
	case PRU_DETACH:
		/*
		 * Tear down the raw and nit-specific parts
		 * of the socket's control block.
		 */
		if (rcb == NULL) {
			error = ENOTCONN;
			break;
		}
		nit_cbfree(ncb);
		raw_detach(rcb);
		break;

	case PRU_BIND:
		if (rcb->rcb_flags & RAW_LADDR) {
			/* Already bound. */
			error = EINVAL;			/* XXX */
			break;
		}
		if (error = raw_bind(so, nam))
			break;
		error = nit_cballoc(rcb);
		break;

	case PRU_RCVD:
		if (ncb->ncb_state != NIT_CATCH)
			error = nit_getbuf(ncb);
		break;

	case PRU_CONTROL:
		/*
		 * m really an int, so we shouldn't
		 * attempt to free it later.
		 */
		error = nit_control(so, (int)m, (caddr_t)nam);
		m = NULL;
		break;

	default:
		/*
		 * raw_usrreq will free m.
		 */
		error = raw_usrreq(so, req, m, nam, opt);
		m = NULL;
		break;
	}

	if (m != NULL)
		m_freem(m);

	(void) splx(s);
	return (error);
}


/*
 * Process PRU_CONTROL operations on a nit socket.
 */
nit_control(so, cmd, data)
	struct socket *so;
	int cmd;
	caddr_t data;
{
	struct rawcb *rcb = sotorawcb(so);
	register struct nit_cb *ncb = (struct nit_cb *)rcb->rcb_pcb;
	int bufsize;
	register int error = 0;

#ifdef	NITDEBUG
	if (nit_debug > 1)
		printf("nit_control(%x, %x, %x)\n", so, cmd, data);
#endif	NITDEBUG

	switch (cmd) {
	case SIOCSNIT: {
		register struct nit_ioc *nioc = (struct nit_ioc *)data;

		if (ncb->ncb_state == NIT_CATCH)
			(void) nit_ferry(ncb);

		/* Use old value if no change requested. */
		if (nioc->nioc_bufalign < 0)
			nioc->nioc_bufalign = ncb->ncb_bufalign;
		if (nioc->nioc_bufoffset < 0)
			nioc->nioc_bufoffset = ncb->ncb_bufoffset;

		/*
		 * Do some sanity checks before committing
		 * to change anything.  The code for allocating
		 * and filling buffers depends on being able to
		 * stash at least two headers into a buffer of
		 * size MCLBYTES.  Enforce this constraint here.
		 */
		nioc->nioc_bufalign = MAX(nioc->nioc_bufalign, 1);
		if (nioc->nioc_bufalign >=
		    ((MCLBYTES >> 1) - sizeof (struct nit_hdr)) >> 1
		   ) {
			error = EINVAL;
			break;
		}
		if (nioc->nioc_bufoffset >= nioc->nioc_bufalign) {
			error = EINVAL;
			break;
		}

		if ((bufsize = nioc->nioc_bufspace) > 0) {
			struct sockbuf *sb = &so->so_rcv;

			/* round up to MCLBYTES multiple */
			bufsize += MCLBYTES - 1;
			bufsize &= ~(MCLBYTES - 1);
			if (sbreserve(sb, bufsize) == 0) {
				error = ENOBUFS;
				break;
			}
		}

		if (nioc->nioc_chunksize > 0) {
			ncb->ncb_chunksize = nioc->nioc_chunksize;
			if (ncb->ncb_chunksize > so->so_rcv.sb_hiwat)
				ncb->ncb_chunksize = so->so_rcv.sb_hiwat;
		}

		if (nioc->nioc_flags != -1) {
			error = nit_promisc(ncb, ncb->ncb_flags,
						nioc->nioc_flags);
			if (error)
				break;
			ncb->ncb_flags = nioc->nioc_flags & ~NF_BUSY;
		}

		if (nioc->nioc_typetomatch != NT_NOTYPES)
			ncb->ncb_typetomatch = nioc->nioc_typetomatch;

		if (nioc->nioc_snaplen > -1)
			ncb->ncb_snaplen = nioc->nioc_snaplen;

		ncb->ncb_bufalign = nioc->nioc_bufalign;
		ncb->ncb_bufoffset = nioc->nioc_bufoffset;

		if (nioc->nioc_flags & NF_TIMEOUT) {
			register p, q;
			
			q = 1000000 / PR_SLOWHZ;
			/* round up */
			p = nioc->nioc_timeout.tv_usec + q - 1;
			p /= q;
			p += nioc->nioc_timeout.tv_sec * PR_SLOWHZ;
			ncb->ncb_timodelay = p;
		} else
			ncb->ncb_timodelay = 0;

		(void) nit_getbuf(ncb);
		break;
	    }

	case SIOCGNIT: {
		register struct nit_ioc *nioc = (struct nit_ioc *)data;

		nioc->nioc_bufspace = so->so_rcv.sb_hiwat;
		nioc->nioc_chunksize = ncb->ncb_chunksize;
		nioc->nioc_typetomatch = ncb->ncb_typetomatch;
		nioc->nioc_snaplen = ncb->ncb_snaplen;
		nioc->nioc_bufalign = ncb->ncb_bufalign;
		nioc->nioc_bufoffset = ncb->ncb_bufoffset;
		if (ncb->ncb_flags & NF_TIMEOUT) {
			register int	p,
					q;

			p = ncb->ncb_timodelay;
			q = p / PR_SLOWHZ;
			nioc->nioc_timeout.tv_sec = q;
			p -= q * PR_SLOWHZ;
			p = (p * 1000000) / PR_SLOWHZ;
			nioc->nioc_timeout.tv_usec = p;
		}
		break;
	    }

	/*
	 * Nit special cases this ioctl to return the link-level
	 * address rather than a higher-level "protocol" address.
	 * It uses a back door to get the address.  The interface's
	 * ioctl routine provides the link-level address, even
	 * though there's no way to get to that case of the routine
	 * directly.
	 */
	case SIOCGIFADDR: {
		register struct ifnet	*ifp = ncb->ncb_if;
		error = (*ifp->if_ioctl)(ifp, cmd, data);
		break;
	    }

	default:
		error = EOPNOTSUPP;
		break;
	}

#ifdef	NITDEBUG
	if (nit_debug > 1)
		printf("nit_control() = %d\n", error);
#endif	NITDEBUG

	return (error);
}

/* Find the interface matching the sockaddr_nit */
struct ifnet *
nit_ifwithaddr(addr)
	struct sockaddr *addr;
{
	register struct sockaddr_nit *snit =
		(struct sockaddr_nit *)addr;

	return (ifunit(snit->snit_ifname, NITIFSIZ));
}

/* Pump a raw packet onto the network */
nit_output(m, so)
	struct mbuf *m;
	struct socket *so;
{
	struct rawcb *rcb = sotorawcb(so);
	struct ifnet *ifp;

	ifp = ((struct nit_cb *)rcb->rcb_pcb)->ncb_if;
	if (ifp)
		return ((*ifp->if_output)(ifp, m, &rcb->rcb_faddr));
	else
		return (ENXIO);
}

/*
 * Handle promiscuous mode.
 * Return 0 on success UNIX errno on failure.
 */
nit_promisc(ncb, old, new)
	struct nit_cb *ncb;
	int old, new;
{
	int delta;
	int error = 0;
	int orig = nitcb_promisc;
	short change;
	struct ifnet *ifp = ncb->ncb_if;
	int (*rtn)() = ifp->if_ioctl;

	old = (old&NF_PROMISC) ? 1 : 0;
	new = (new&NF_PROMISC) ? 1 : 0;
	delta = new - old;

	change = 0;
	switch (delta) {
	case -1:
		if (--nitcb_promisc == 0)
			change = delta;
		break;
	case  1:
		if (nitcb_promisc++ == 0)
			change = delta;
		break;
	}

	if (change) {
		if (rtn) {
			if (change < 0)
				ifp->if_flags &= ~IFF_PROMISC;
			else
				ifp->if_flags |= IFF_PROMISC;
			error = (*rtn)(ifp, SIOCSIFFLAGS, (caddr_t)NULL);
		} else
			error = EOPNOTSUPP;
	}

#ifdef NITDEBUG
	if (nit_debug)
		printf("nit_promisc() = %d delta=%d\n", error, delta);
#endif NITDEBUG
	if (error)
		nitcb_promisc = orig;
	return (error);
}
