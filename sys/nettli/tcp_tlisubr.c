#ifndef lint
static  char sccsid[] = "@(#)tcp_tlisubr.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

#define	TLIDEBUG

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/debug.h>
#include <sys/user.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/uio.h>
#include <sys/protosw.h>
#include <sys/kmem_alloc.h>

#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_pcb.h>

#include <nettli/tihdr.h>
#include <nettli/tiuser.h>
#include <nettli/tcp_tli.h>
#include <nettli/tcp_tlivar.h>


/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * If tlierr == NULL, then issues M_ERROR msg
 */
int
tcptli_error(q, mrp, prim, tlierr, unixerr)
	queue_t	*q;
	mblk_t	*mrp;
	long	prim;
	int	tlierr;
	int	unixerr;
{
	register struct T_error_ack	*ap;
	register struct tt_softc	*ttp;

	ttp = (struct tt_softc *)q->q_ptr;

	/*
	 * If mrp is NULL or too small for response, then use
	 * the reserve tt_errack buffer allocated
	 * earlier during open. Buffer tt_merror is not
	 * used unless tt_errack does not exist (for whatever
	 * reason). If we are forced to use tt_merror, then we
	 * assume we're in deep yogurt and generate the dreaded M_ERROR
	 * response.
	 */
	if (!mrp || (mrp->b_wptr - mrp->b_rptr < sizeof (struct T_error_ack))) {
		if (mrp)
			freemsg(mrp);
		if (!ttp->tt_errack) {
			mrp = ttp->tt_merror;
			ttp->tt_merror = NULL;
			tlierr = 0;
		} else {
			mrp = ttp->tt_errack;
			ttp->tt_errack =
				allocb(sizeof (struct T_error_ack), BPRI_HI);
		}
	}
	if (tlierr == 0) {
		mrp->b_datap->db_type = M_ERROR;
		*(mrp->b_wptr) = (char) unixerr;
		mrp->b_wptr++;
	} else {
		mrp->b_datap->db_type = M_PCPROTO;
		mrp->b_wptr = mrp->b_rptr + sizeof (struct T_error_ack);
		ap = (struct T_error_ack *)mrp->b_rptr;
		ap->PRIM_type = T_ERROR_ACK;

		ap->ERROR_prim = prim;
		ap->TLI_error = tlierr;
		ap->UNIX_error = unixerr;
	}
	/*
	 * Always issue on the read queue.
	 */
	putnext(ttp->tt_rq, mrp);

	return (0);
}
/*
 * tcptli_sockargs() is the same as sockargs(),
 * except this one copies data within the
 * kernel address space
 */
tcptli_sockargs(aname, name, namelen, type)
	struct mbuf	**aname;
	caddr_t		name;
	int 		namelen;
	int		type;
{
	register struct mbuf *m;

	if (namelen > MLEN)
		return (EINVAL);
	m = m_get(M_WAIT, type);
	if (m == NULL)
		return (ENOBUFS);
	m->m_len = namelen;
	bcopy(name, mtod(m, caddr_t), (u_int)namelen);
	*aname = m;
	return (0);
}
/*
 * This routine sets up the uio vector and pass it
 * onto sosend().
 */
tcptli_send(ttp, mp, flags, alreadysent)
	struct	tt_softc *ttp;
	mblk_t	*mp;
	int	flags;
	int	*alreadysent;
{
	register mblk_t	*mbp;
	struct	mbuf	*nam = 0;
	int		error = 0;
	int		numblks = 0;
	int		total = 0;
	struct	iovec	iov[TT_MAXUIO];
	struct	iovec	*moreiov = NULL;
	struct uio	uio;

	uio.uio_iov = iov;
	uio.uio_iovcnt = 0;
	uio.uio_offset = 0;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_fmode = 0;
	uio.uio_resid = 0;
	/*
	 * Determine number of mblks in data chain
	 * and check if existing iovec can hold. If
	 * not, then kmem_alloc() enough before
	 * moving on.
	 */
	for (mbp = mp; mbp; mbp = mbp->b_cont)
		numblks++;

	if (numblks > TT_MAXUIO) {
		moreiov = (struct iovec *)new_kmem_alloc(
			(u_int)(sizeof (struct iovec) * numblks), KMEM_SLEEP);
		uio.uio_iov = moreiov;
	}
	for (mbp = mp; mbp; mbp = mbp->b_cont) {
		uio.uio_iov->iov_base = (caddr_t)mbp->b_rptr;
		uio.uio_iov->iov_len = (int)(mbp->b_wptr - mbp->b_rptr);
		uio.uio_resid += uio.uio_iov->iov_len;
		uio.uio_iovcnt++;
		uio.uio_iov++;

	}
	/*
	 * Reset uio.uio_iov to beginning of iov[] or
	 * moreiov[]
	 */
	uio.uio_iov = numblks > TT_MAXUIO ? moreiov : iov;

	TCPTLI_PRINTF("tcptli_sendexdata: Sending %d bytes data\n",
		uio.uio_resid);
	total = uio.uio_resid;	/* save the total count for later checking */
	error = sosend(ttp->tt_so, nam, &uio, flags,
			(struct mbuf *)0);
	if (error)
		goto quit;

	TCPTLI_PRINTF("tcptli_send: uio_resid = %d after sosend\n",
		uio.uio_resid);
	/*
	 * Check to see if there are unsent data. If so,
	 * return TT_INCOMPLETESEND error code
	 */
	if (uio.uio_resid > 0)	{
		*alreadysent = total - uio.uio_resid;
		error = TT_INCOMPLETESEND;
	}
quit:
	if (moreiov)
		kmem_free((caddr_t)moreiov,
			(u_int)(sizeof (struct iovec) * numblks));
	return (error);
}

/*
 * Routine to create a new tt_sockinfo block. The kmem_free
 * is done in sofree() i.e. when the associated socket is
 * closed.
 * This routine does **NOT** set the sequence number and
 * the flags. The caller is responsible for how to use
 * those 2 fields.
 */
struct tt_sockinfo *
tcptli_mksockinfo(ttp)
	struct	tt_softc	*ttp;
{
	register struct tt_sockinfo *tsp;
	int	tcptli_wakeup();

	tsp = (struct tt_sockinfo *)new_kmem_alloc(
				sizeof (struct tt_sockinfo), KMEM_SLEEP);
	tsp->ts_sowakeup.wup_func = tcptli_wakeup;
	tsp->ts_sowakeup.wup_arg = (caddr_t) tsp;
	tsp->ts_ttp = ttp;
	return (tsp);
}
/*
 * When a service was attempted but failed, requiring a wakeup
 * at a later time. The scheme here is to check to see if a
 * timer is set already (possibly by another service failure) and
 * then issues a timer to call tcptli_timeout() [see below], which
 * will clear the TT_TIMER flag when the timer goes off, thus
 * the TT_TIMER flag is set/clear only by these two routines.
 */
tcptli_schedwakeup(ttp, hz)
	struct	tt_softc	*ttp;
	int			hz;	/* clock ticks */
{
	int	tcptli_timeout();

	TCPTLI_PRINTF("tcptli_schedwakeup: for ttp = %x\n", ttp);
	if (! (ttp->tt_flags & TT_TIMER)) {
		ttp->tt_flags |= TT_TIMER;
		timeout((int (*)()) tcptli_timeout, (caddr_t)ttp, hz);
	}
	return;
}
/*
 * When timer goes off, the TT_TIMER flag is clears
 * and a real wakeup is issued
 */
tcptli_timeout(ttp)
	struct	tt_softc	*ttp;
{
	register int		s;
	extern struct tt_softc	*tcptli_ttp;

	ttp->tt_flags &= ~TT_TIMER;
	ttp->tt_event |= TTE_EVENT;
	TCPTLI_PRINTF("tcptli_timer: ttp: %x\n", ttp);
	s = splnet();
	if (!(ttp->tt_event & TTE_ONQUEUE)) {
		tcptli_insq(ttp);
	}
	(void) splx(s);
	(void) wakeup((caddr_t)&tcptli_ttp);
	return;
}
