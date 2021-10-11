#ifndef lint
static  char sccsid[] = "@(#)tcp_tliaux.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */


/*
 * The auxiliary process half of the streams interface
 * to the socket-based TCP implementation.
 */

#define	TLIDEBUG

#include "tcptli.h"

#if	NTCPTLI > 0

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

#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#include <nettli/tihdr.h>
#include <nettli/tiuser.h>
#include <nettli/tcp_tli.h>
#include <nettli/tcp_tlivar.h>


/*
 * Round up to machine alignment boundary.
 *
 * XXX:	belongs in <machine/param.h>.
 */
#ifndef	align
#define	align(x)	(((x) + 3) & ~3)
#endif	align

int	tcptli_auxproc_running = 0;

/*
 * Auxiliary process main loop.  This consists of waiting until
 * the stream bottom code requests some processing, then doing
 * whatever can be done.
 */
tcptli_main()
{
	register int			s;
	register struct tt_softc	*ttp;
	extern	struct proc		*tcptli_auxprocp;
	extern	struct tt_softc		*tcptli_ttp;
	extern	int			tcptli_ttpcount;
	extern	int			tcptli_auxproc_running;
	struct	tt_softc		*tcptli_remq();

	/*
	 * Initialization of global veriables
	 */
	tcptli_ttp = NULL;
	tcptli_auxprocp = u.u_procp;
	tcptli_memfree();
	bcopy("tcptli_auxproc", (caddr_t)u.u_comm, (u_int)15);
	(void) setjmp(&u.u_qsave);

	for (;;) {
		/*
		 * Wait for the next processing request.
		 *
		 */

		s = splnet();
		while ((ttp = tcptli_remq(tcptli_ttp)) == NULL &&
			tcptli_ttpcount != 0) {
			tcptli_auxproc_running = 0;
			(void) sleep((caddr_t)&tcptli_ttp, PZERO + 1);
		}
		(void) splx(s);


		/*
		 * If no more open ttp, terminate this aux proc
		 */
		if (ttp == NULL && tcptli_ttpcount == 0) {
			TCPTLI_PRINTF("tcptli_auxproc: Exiting ...\n");
			exit(0);
		}
		/*
		 * If the ttp already marked TT_CLOSE, then no
		 * processing is done.
		 */
		if (ttp->tt_flags & TT_CLOSE)
			continue;


		/*
		 * Process the read side first ...
		 *
		 * If the read queue is full, do nothing.
		 *
		 * Otherwise check each possible event to see whether
		 * it's occurred, doing the appropriate processing for
		 * each that has.
		 *
		 * The code below is complicated by the need to observe
		 * tli state discipline.  Each event is legal only from
		 * certain tli states.  Moreover, at a given instant, the
		 * set of possible events is a function of the socket (and
		 * underlying TCP) states.
		 *
		 **********
		 *  NOTE  *
		 **********
		 * The read side is performed first because read events
		 * are asynchronous. In the event that a socket
		 * is blown away unexpectedly, the repair will be
		 * done before any other operation (which has no knowledge
		 * of possible damage to the ttp struct) is attempted.
		 */
		tcptli_auxproc_running = 1;
		tcptli_rdevent(ttp);

		/*
		 * Process the write side.
		 *
		 * Tcptli_usrreq takes the entire queue and does
		 * getq within it.
		 *
		 */
		(void) tcptli_usrreq(ttp);
	}
}


/*
 * Process receive events.
 *
 */
tcptli_rdevent (ttp)
	struct tt_softc	*ttp;
{
	int			processedevent;

	/*
	 * Search for new events until there are none or we can't
	 * handle any more for the moment.  The latter circumstance
	 * amounts to running out of resources -- mblks, room in the
	 * read queue, etc.  In this case we'll break from the loop.
	 *
	 *
	 ********
	 * NOTE *
	 ********
	 * The 2 Rcv-disconnet events must be checked first
	 * in the case the socket is blown away due to an
	 * unexpected remote connection refusal/shutdown
	 * that causes the underlying TCP to remove all the
	 * tcp-associations.
	 */
	do {
		processedevent = 0;

		processedevent |= tcptli_Ercvdis1(ttp);
		processedevent |= tcptli_Ercvdis23(ttp);
		processedevent |= tcptli_Ercvconnect(ttp);
		processedevent |= tcptli_Elisten(ttp);
		processedevent |= tcptli_Ercv(ttp);

	} while (processedevent);
}


/*
 * Process a pending "rcvconnect" event, returning 1 if there
 * is one and it was processed successfully and 0 otherwise.
 *
 * This event corresponds to the local events occurring when a
 * foreign peer accepts (in the 4.3 sense) our request to connect.
 */
tcptli_Ercvconnect(ttp)
	register struct tt_softc	*ttp;
{
	struct socket			*so = ttp->tt_so;
	register mblk_t			*mp;
	register struct T_conn_con	*ccp;
	register int			offset;
	struct sockaddr_in		sa;

	if (!(ttp->tt_state & TL_OUTCON) || !(so->so_state & SS_ISCONNECTED))
		return (0);

	if (!canput(ttp->tt_rq)) {
		tcptli_schedwakeup(ttp, 100);
		return (0);
	}

	TCPTLI_PRINTF("Ercvconnect ... [tcptli-%d] ttp: %x tt_so %x\n",
		ttp - tt_softc, ttp, so);

	/*
	 * Gather resources before committing to observable state change;
	 * get storage for message to transport user.  This message must
	 * be large enough to hold the peer's address.
	 */
	offset = align(sizeof (struct T_conn_con));
	if (!(mp = allocb(offset + sizeof (struct sockaddr_in), BPRI_MED)))
		return (0);

	/*
	 * Notify the transport user of the pending
	 * connection.
	 */
	mp->b_datap->db_type = M_PROTO;
	mp->b_wptr += offset + sizeof (struct sockaddr_in);
	ccp = (struct T_conn_con *)mp->b_rptr;
	ccp->PRIM_type = T_CONN_CON;
	ccp->RES_length = sizeof (struct sockaddr_in);
	ccp->RES_offset = offset;
	ccp->OPT_length = 0;
	ccp->OPT_offset = 0;

	/*
	 * Stuff the sockaddr_in info into mblk
	 *
	 */
	bzero((caddr_t)&sa, sizeof (sa));
	sa.sin_family = AF_INET;
	sa.sin_port = sotoinpcb(so)->inp_fport;
	sa.sin_addr = sotoinpcb(so)->inp_faddr;
	bcopy((caddr_t)&sa, (caddr_t)(mp->b_rptr + offset), sizeof (sa));

	putnext(ttp->tt_rq, mp);

	/*
	 * Turn on the TF_NODELAY option
	 */
	{
		struct inpcb *inp = sotoinpcb(ttp->tt_so);
		struct tcpcb *tp  = intotcpcb(inp);

		tp->t_flags |= TF_NODELAY;

	}
	/*
	 * Perform tli automaton state transition.
	 */

	ttp->tt_state = TL_DATAXFER;

	return (1);
}

/*
 * Process the eldest pending "listen" event, returning 1 if there
 * is one and it was processed successfully and 0 otherwise.
 *
 * This event corresponds to the local events occurring when a
 * foreign peer attempts to connect (in the 4.3 sense) to us.
 */
int
tcptli_Elisten(ttp)
	register struct tt_softc	*ttp;
{
	register struct socket		*soq;
	register struct socket		*so = ttp->tt_so;
	mblk_t				*mp;
	register struct T_conn_ind	*cip;
	register int			offset;
	struct sockaddr_in		sa;
	int				s;
	struct tt_sockinfo		*tsp, *tcptli_mksockinfo();

	if (!(ttp->tt_state & (TL_IDLE | TL_INCON)))
		return (0);

	if (!canput(ttp->tt_rq)) {
		tcptli_schedwakeup(ttp, 100);
		return (0);
	}

	/*
	 * Gather resources before committing to observable state change;
	 * get storage for internal record of the pending connection and
	 * for message to transport user; the latter must be large enough
	 * to hold the peer's address.
	 */
	offset = align(sizeof (struct T_conn_ind));
	if (!(mp = allocb(offset + sizeof (struct sockaddr_in), BPRI_MED)))
		return (0);

	/*
	 * We check if there are new pending connection in the
	 * so_q pointed by the head socket, if so we allocate
	 * a tt_sockinfo block to store the sequence number for
	 * future reference. If not, we simply return.
	 */

	/*
	 * Find eldest new pending connection.
	 * This is done by searching the socket (tt_so->so_q) list
	 * and find the one that matches in soq (established by
	 * soisconnected()).
	 */
	s = splnet();
	soq = so->so_q;
	/*
	 * soq == NULL		==>  has not done an solisten() yet
	 * soq == so		==>  has done an solisten(), but no pending
	 * soq->so_wupalt != NULL ==> the tt_sockinfo has been setup, i.e.
	 *				we are at the head of the connected
	 *				socket queue list
	 */
	if (soq == NULL || soq == so || soq->so_wupalt != NULL) {
		(void) splx(s);
		freeb(mp);
		return (0);
	}
	for (soq = so->so_q; soq != so; soq = soq->so_q)
		if (soq->so_wupalt == NULL) 	/* new pending connection */
			break;
	(void) splx(s);

	ASSERT(soq != so);
	/*
	 * Create a new tt_sockinfo block and record the found
	 * new pending connection with a sequence number
	 */
	tsp = tcptli_mksockinfo(ttp);
	tsp->ts_seqnum = ttp->tt_seqnext++;
	tsp->ts_flags |= TT_TS_INUSE;
	soq->so_wupalt = &tsp->ts_sowakeup;


	TCPTLI_PRINTF("Elisten ... [tcptli-%d] ttp: %x tt_so %x\n",
		ttp - tt_softc, ttp, ttp->tt_so);
	/*
	 * Notify the transport user of the pending
	 * connection.
	 */
	mp->b_datap->db_type = M_PROTO;
	mp->b_wptr += offset + sizeof (struct sockaddr_in);
	cip = (struct T_conn_ind *)mp->b_rptr;
	cip->PRIM_type = T_CONN_IND;
	cip->SRC_length = sizeof (struct sockaddr_in);
	cip->SRC_offset = offset;
	cip->OPT_length = 0;
	cip->OPT_offset = 0;
	cip->SEQ_number = tsp->ts_seqnum;

	bzero((caddr_t)&sa, sizeof (sa));
	sa.sin_family = AF_INET;
	sa.sin_port = sotoinpcb(soq)->inp_fport;
	sa.sin_addr = sotoinpcb(soq)->inp_faddr;
	bcopy((caddr_t)&sa, (caddr_t)(mp->b_rptr + offset), sizeof (sa));

	TCPTLI_PRINTF("Elisten: Recv'd conn-req from %x Port %x\n",
		sa.sin_addr.s_addr, sa.sin_port);
	putnext(ttp->tt_rq, mp);

	/*
	 * Perform tli automaton state transition.
	 */
	ttp->tt_state = TL_INCON;

	return (1);
}


/*
 * Process a pending "rcv" event, returning 1 if there
 * is one and it was processed successfully and 0 otherwise.
 *
 * This event corresponds to the local events occurring when a
 * foreign peer sends us data, either normal or expedited.
 *
 */


int
tcptli_Ercv(ttp)
	register struct tt_softc	*ttp;
{
	struct socket			*so = ttp->tt_so;
	register struct	T_data_ind	*dip;
	struct	iovec	iov[TT_MAXUIO];
	struct	uio	uio;
	struct	mbuf	*nam;
	mblk_t		*mp;
	int		s;
	int		error;

	if (!(ttp->tt_state & (TL_DATAXFER | TL_OUTREL)))
		return (0);

	/*
	 * Need to mark the SB_WAIT flag in the socket-recv
	 * buffer. The socket will never get waken up if
	 * this flag is not set. Look at sbwakeup() for
	 * details.
	 */
	s = splnet();
	so->so_rcv.sb_flags |= SB_WAIT;
	(void) splx(s);
	/*
	 *
	 *	Basic outline:
	 *	while (socket nonempty && canput) {
	 *		transcribe as big a chunk as we can, respecting
	 *		urgent data boundaries.
	 *	}
	 *
	 *	Urgent data detected by:
	 *		so->so_oobmark || (so->so_state & SS_RCVATMARK)
	 *	Normal data detected by:
	 *		so->so_rcv.sb_cc > 0
	 */

	if (!canput(ttp->tt_rq)) {
		tcptli_schedwakeup(ttp, 100);
		return (0);
	}
	/*
	 * We could take soreceive and let its uiomove to
	 * copy data to mblk's. If we use m_cpytoc(), then
	 * we need to free mbufs. So by using soreceive(),
	 * we don't have to worry about freeing mbufs.
	 */

	if (so->so_rcv.sb_cc != 0) {
		int	size,		/* amount of data in socket queue */
			flags = 0;	/* EXDATA flag */
		mblk_t *smp = NULL;
		mblk_t *tmp = NULL;

		TCPTLI_PRINTF("Ercv ... [tcptli-%d] ttp: %x tt_so %x\n",
			ttp - tt_softc, ttp, so);
		uio.uio_iov = iov;
		uio.uio_iovcnt = 0;
		uio.uio_offset = 0;
		uio.uio_segflg = UIO_SYSSPACE;
		uio.uio_fmode = 0;
		uio.uio_resid = 0;

		size = so->so_rcv.sb_cc;
		mp = allocb(sizeof (struct T_data_ind), BPRI_MED);
		if (!mp) {
			return (0);
		}
		mp->b_datap->db_type = M_PROTO;
		mp->b_wptr += sizeof (struct T_data_ind);
		dip = (struct T_data_ind *)mp->b_rptr;
		dip->PRIM_type = T_DATA_IND;
		dip->MORE_flag = 0;

		if (so->so_oobmark || (so->so_state & SS_RCVATMARK)) {
			flags |= MSG_OOB;
			mp->b_cont = allocb(sizeof (char), BPRI_MED);
			uio.uio_iovcnt++;
			uio.uio_resid = 1;
			uio.uio_iov->iov_base = (caddr_t)mp->b_cont->b_rptr;
			error = soreceive(so, &nam, &uio, flags,
				(struct mbuf **)0);
			mp->b_cont->b_wptr +=  1 - uio.uio_resid;
			if (error) {
				TCPTLI_PRINTF(
"tcptli_Ercv: Error (%x) returned by soreceive, reading OOB data\n", error);
				freemsg(mp);
				return (0);
			}
		} else {		/* non-urgent data */

			uio.uio_resid = size;
			for (smp = mp; size > 0; smp = smp->b_cont) {
				tmp = allocb(MIN(size, TT_BUFSIZE), BPRI_MED);
				/*
				 * Allocb fails, if got some buffers, then
				 * do the sorecive() on what are available
				 * If not, just quit.
				 */
				if (!tmp) {
					TCPTLI_PRINTF(
					    "tcptli_Ercv: allocb failed\n");
					break;
				}
				uio.uio_iov->iov_base = (caddr_t)tmp->b_rptr;
				tmp->b_wptr += MIN(size, TT_BUFSIZE);
				uio.uio_iov->iov_len = MIN(size, TT_BUFSIZE);
				smp->b_cont = tmp;
				size -= uio.uio_iov->iov_len;
				uio.uio_iov++;
				uio.uio_iovcnt++;
				if (uio.uio_iovcnt >= TT_MAXUIO)
					break;
			}
			if (size > 0)
				tcptli_schedwakeup(ttp, 100);

			TCPTLI_PRINTF(
			    "tcptli_Ercv: size = %d, uio_resid = %d\n",
			    size, uio.uio_resid);
			uio.uio_resid -= size;
			if (uio.uio_resid == 0) {
				freemsg(mp);
				return (0);
			}
			uio.uio_iov = iov;	/* reset to iov[0] */
			/*
			 * start the soreceive
			 */
			TCPTLI_PRINTF("tcptli_Ercv: Receiving %d bytes\n",
			    uio.uio_resid);
			error = soreceive(so, &nam, &uio, flags,
				(struct mbuf **)0);
			TCPTLI_PRINTF(
			    "tcptli_Ercv: uio_resid = %d after soreceive\n",
			    uio.uio_resid);
			if (error) {
				TCPTLI_PRINTF(
			"tcptli_Ercv: unexpected error from soreceive\n");
				freemsg(mp);
				return (0);
			}
		}
		putnext(ttp->tt_rq, mp);
		return (1);
	} else
		return (0);
}

/*
 * Process a pending "rcvdis1" event, returning 1 if there
 * is one and it was processed successfully and 0 otherwise.
 *
 * This event corresponds to the local events occurring when a
 * foreign peer refuses our connect request or drops an existing
 * connection.
 */
int
tcptli_Ercvdis1(ttp)
	register struct tt_softc	*ttp;
{
	struct socket			*so = ttp->tt_so;
	register mblk_t			*mp;
	mblk_t				*mfp = NULL;
	register struct T_discon_ind	*dip;

	if (!(ttp->tt_state & (TL_OUTCON | TL_DATAXFER | TL_OUTREL | TL_INREL)))
		return (0);

	if (!(so->so_state  & SS_CANTRCVMORE))
		return (0);

	if (!canput(ttp->tt_rq)) {
		tcptli_schedwakeup(ttp, 100);
		return (0);
	}

	/*
	 * If we're in the TL_DATAXFER, TL_OUTREL, or TL_INREL state,
	 * we must issue M_FLUSH for both the read and write queues
	 * before sending up the disconnect notification. Allocate
	 * here, and send after we've gathered the rest of our resources.
	 */
	if (ttp->tt_state & (TL_DATAXFER | TL_INREL | TL_OUTREL)) {
		if (!(mfp = allocb(sizeof (char), BPRI_HI)))
			return (0);
	}

	/*
	 * Gather resources before committing to observable state change;
	 * get storage for the message to the transport user.
	 */
	if (!(mp = allocb(sizeof (struct T_discon_ind), BPRI_MED))) {
		if (mfp)
			freeb(mfp);
		return (0);
	}

	TCPTLI_PRINTF("Ercvdis1 ... [tcptli-%d] ttp: %x tt_so %x\n",
		ttp - tt_softc, ttp, so);
	/*
	 * Flush if required.
	 */
	if (mfp) {
		mfp->b_datap->db_type = M_FLUSH;
		mfp->b_wptr++;
		*mfp->b_rptr = FLUSHR | FLUSHW;

		putnext(ttp->tt_rq, mfp);
	}

	/*
	 * Save the sockaddr info from the socket to be dropped
	 * into a mbuf. Close the requested drop socket. Create
	 * a new one, and bind with the saved sockaddr.
	 */
	{
		struct	sockaddr_in	sa;
		struct	socket		*newso;
		struct	tt_sockinfo	*tsp, *tcptli_mksockinfo();
		struct	mbuf		*nam = NULL;
		int			error;
		int			tcptli_sendsize = TT_SENDSIZE;
		int			tcptli_recvsize = TT_RECVSIZE;

		bzero((caddr_t)&sa, sizeof (sa));
		sa.sin_family = AF_INET;
		sa.sin_port = ttp->tt_lport;
		sa.sin_addr = ttp->tt_laddr;
		(void) soclose(so);
		ttp->tt_so = NULL;
		if (error = socreate(AF_INET, &newso, SOCK_STREAM, 0)) {
			(void) tcptli_error(
				ttp->tt_rq, mp, (long)0, NULL, error);
			goto out;
		}
		newso->so_state |= SS_NBIO;

		/*
		 * Set the socket options and send/recv socket buffer
		 * sizes to 8K for RFS applications.
		 * Criteria for setting socket buffer sizes:
		 * size < SM_MAX * MCLBYTES / (2 * MSIZE + MCLBYTES)
		 * taken from sbreserve(), but not checked here to
		 * imporve performance.
		 */

		newso->so_options |= SO_REUSEADDR;
		newso->so_options |= SO_KEEPALIVE;

		newso->so_rcv.sb_hiwat = tcptli_recvsize;
		newso->so_rcv.sb_mbmax = MIN(tcptli_recvsize * 2, SB_MAX);
		newso->so_snd.sb_hiwat = tcptli_sendsize;
		newso->so_snd.sb_mbmax = MIN(tcptli_sendsize * 2, SB_MAX);
		/*
		 * Setup the special socket wakeup hooks here
		 */
		tsp = tcptli_mksockinfo(ttp);
		if (tsp == NULL) {
			(void) tcptli_error(
				ttp->tt_rq, mp, (long)0, NULL, error);
			(void) soclose(newso);
			goto out;
		}
		tsp->ts_seqnum = 0;
		tsp->ts_flags = TT_TS_NOTUSED;
		newso->so_wupalt = &tsp->ts_sowakeup;

		(void) tcptli_sockargs(&nam, (caddr_t)&sa, sizeof (sa),
			MT_SONAME);
		error = sobind(newso, nam);
		if (nam)
			m_freem(nam);
		if (error) {
			(void) tcptli_error(
				ttp->tt_rq, mp, (long)0, NULL, error);
			(void) soclose(newso);
			goto out;
		}
		ttp->tt_so = newso;
	}
	/*
	 * Notify the transport user of our peer's disconnection.
	 */
	mp->b_datap->db_type = M_PROTO;
	mp->b_wptr += sizeof (struct T_discon_ind);
	dip = (struct T_discon_ind *)mp->b_rptr;
	dip->PRIM_type = T_DISCON_IND;
	dip->DISCON_reason = (long)so->so_error;		/* ??? */
	dip->SEQ_number = -1;

	putnext(ttp->tt_rq, mp);

	/*
	 * Perform tli automaton state transition.
	 */
	ttp->tt_state = TL_IDLE;

	return (1);
out:
	ttp->tt_state = TL_UNINIT;
	return (0);
}

/*
 * Process a pending "rcvdis2" or "rcvdis3" event, returning 1 if
 * there is one and it was processed successfully and 0 otherwise.
 *
 * This event corresponds to the local events occurring when a
 * foreign peer abandons a connect request it has made to us.
 */
int
tcptli_Ercvdis23(ttp)
	register struct tt_softc	*ttp;
{
	struct socket			*so = ttp->tt_so,
					*soq;
	register mblk_t			*mp;
	struct T_discon_ind		*dip;
	long				seqno;
	struct	tt_sockinfo		*tsp;

	if (!(ttp->tt_state & TL_INCON))
		return (0);


	soq = so->so_q;
	if (soq == NULL || soq == so) {
		return (0);
	}
	for (soq = so->so_q; soq != so; soq = soq->so_q) {
		if (!(soq->so_state & SS_ISCONNECTED)) {
			break;
		}
	}

	if (soq == so)		/* no match */
		return (0);

	TCPTLI_PRINTF("Ercvdis23 ... [tcptli-%d] ttp: %x tt_so %x soq: %x\n",
		ttp - tt_softc, ttp, so, soq);
	/*
	 * Gather resources before committing to observable state change;
	 * get storage for the message to the transport user.
	 */
	if (!(mp = allocb(sizeof (struct T_discon_ind), BPRI_MED)))
		return (0);
	tsp = (struct tt_sockinfo *)soq->so_wupalt->wup_arg;
	seqno = tsp->ts_seqnum;
	tsp->ts_flags = TT_TS_NOTUSED;
	/*
	 * Check if the SS_NOFDREF is set, if so, clears
	 * it before calling soclose, otherwise, soclose
	 * will panic. Normally, the flag is cleared by
	 * soaccept.
	 */
	if (soq->so_state & SS_NOFDREF)
		soq->so_state &= ~SS_NOFDREF;
	{
		register struct inpcb *inp;
		register struct tcpcb *tp;

		inp = sotoinpcb(soq);
		if (inp) {
			tp = intotcpcb(inp);
			tp = tcp_close(tp);
		}
	}
	(void) soclose(soq);

	/*
	 * Notify the transport user of our peer's abandonment.
	 */
	mp->b_datap->db_type = M_PROTO;
	mp->b_wptr += sizeof (struct T_discon_ind);
	dip = (struct T_discon_ind *)mp->b_rptr;
	dip->PRIM_type = T_DISCON_IND;
	dip->DISCON_reason = (long)so->so_error;		/* ??? */
	dip->SEQ_number = seqno;

	putnext(ttp->tt_rq, mp);

	/*
	 * Perform tli automaton state transition.  If there's
	 * another pending connection remaining, stay in the
	 * TL_INCON state.
	 */
	ttp->tt_state = so->so_q == so ? TL_IDLE : TL_INCON;

	return (1);
}

#endif	NTCPTLI > 0
