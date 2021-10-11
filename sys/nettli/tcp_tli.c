#ifndef lint
static  char sccsid[] = "@(#)tcp_tli.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Streams interface to the socket-based TCP implementation.
 * Compatible with the AT&T Transport Level Interface.
 *
 * General notes:
 * 1)	This module's notion of an address is a struct sockaddr_in.
 *	However, at one point, AT&T defined a "short" address format
 *	as the std TLI addressing format known as taddr_in:
 *		struct taddr_in == (struct sockaddr_in) - (char zero[8])
 *	which might exist in some streams TCP implementations. Both
 *	formats are supported here.
 *
 * 2)   TCPTLI_PRINTF debug statements are switched by the global
 *	variable tcptli_debug, defined in nettli/tcp_tlivar.h.
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
#include <sys/file.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/wait.h>
#include <sys/vnode.h>
#include <sys/protosw.h>

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
 * Stereotyped streams glue.
 */

int	tcptli_open();
int	tcptli_close();
int	tcptli_wput();
int	tcptli_wsrv();

struct module_info	tcptli_minfo = {
	0,			/* module id XXX: define real value */
	"tcptli",
	0,
	INFPSZ,			/* max msg size */
	6144,			/* high water: 6 x RFS's */
	STRLOW			/* low water XXX: make reasonable */
};

struct qinit	tcptli_rinit = {
	NULL,
	NULL,
	tcptli_open,
	tcptli_close,
	NULL,
	&tcptli_minfo,
	NULL
};

/*
 * Write-side queue info.
 *
 * Although the driver has the equivalent of a write-side service procedure,
 * it's not visible to the streams framework.  The auxiliary process is
 * responsible for processing entries on the write queue.
 *
 * tcptli_wsrv() does not really exist. The write-service routine
 * is handled in the auxiliary process. The existence of the
 * entry point declaration is to fake the streams code such that
 * flow control will be checked by canput().
 */

struct qinit	tcptli_winit = {
	tcptli_wput,
	tcptli_wsrv,
	tcptli_open,
	tcptli_close,
	NULL,
	&tcptli_minfo,
	NULL
};

struct streamtab	tcptli_info = {
	&tcptli_rinit,
	&tcptli_winit,
	NULL,
	NULL
};

/*
 * Global variables:
 *
 * 1) Per-device instance state information.
 * This private data structure plays the key role in this
 * TCPTLI implementation.
 *
 * 2) Debug flag
 *
 * 3) Counter of total opened ttp
 *
 * 4) Head pointer to the pending ttp-queue
 *
 */
extern	struct	tt_softc	tt_softc[];	/* declared in param.c */
extern	int		tcptli_limit;		/* from config's NTCPTLI */
int			tcptli_debug = 0;	/* default=no debugging */
int			tcptli_ttpcount = 0;	/* opened ttp counter */
struct	tt_softc	*tcptli_ttp = NULL;	/* hd ptr to pend'g ttp list */
struct	proc		*tcptli_auxprocp = NULL;	/* aux proc ptr */



#define	CANPUSHBACK	1	/* symbolic val for 3rd arg to tcptli_usrreq */


/* ARGSUSED */
int
tcptli_open(q, dev, oflag, sflag)
	struct queue	*q;
	dev_t		dev;
	int		oflag;
	int		sflag;
{
	register int			unit;
	register struct tt_softc	*ttp;
	struct tt_sockinfo		*tsp, *tcptli_mksockinfo();
	struct socket			*so;
	int 				tcptli_wakeup();
	int				tcptli_sendsize = TT_SENDSIZE;
	int				tcptli_recvsize = TT_RECVSIZE;
	extern int 			tcptli_main();

	TCPTLI_PRINTF(
		"tcptli_open: q:%x, WR(q):%x, dev:%x, oflag:%x, sflag:%x\n",
		q, WR(q), dev, oflag, sflag);

	/*
	 * Get our minor device number, verifying while we're at it
	 * that nobody's trying to open us as a module (as opposed
	 * to a device).
	 */
	if (sflag == CLONEOPEN) {
		/*
		 * Find an unused minor device instance.
		 * XXX -- Skip returning instance 0, until
		 * strclose() code is fixed
		 */
		for (unit = 1; unit < tcptli_limit; unit++)
			if (!(tt_softc[unit].tt_flags & (TT_OPEN | TT_CLOSE)))
				break;
	} else if (sflag)
		return (OPENFAIL);
	else
		unit = minor(dev);

	if (unit < 0 || unit >= tcptli_limit) {
		u.u_error = ENXIO;
		return (OPENFAIL);
	}

	ttp = &tt_softc[unit];
	TCPTLI_PRINTF("tcptli_open: ttp: %x tt_softc: %x\n", ttp, tt_softc);
	if (ttp->tt_flags & TT_OPEN) {
		u.u_error = EBUSY;		/* bletch! */
		return (OPENFAIL);
	}

	/*
	 * Pre-allocate space for the last-resort M_ERROR
	 * message, forcing the open to fail if the allocation
	 * fails.
	 */
	if (!(ttp->tt_merror = allocb(sizeof (struct T_error_ack), BPRI_HI)))
		return (OPENFAIL);
	/*
	 * Grab space for an error acknowledgement.
	 * Failure here is acceptable.
	 */
	ttp->tt_errack = allocb(sizeof (struct T_error_ack), BPRI_HI);

	/*
	 * Opening the device corresponds to performing a
	 * socket call.  Grab a socket for our internal use.
	 * Set the socket options: SO_REUSEADDR & SO_KEEPALIVE
	 * and mark flag SS_NBIO.
	 */
	if (u.u_error = socreate(AF_INET, &so, SOCK_STREAM, 0))
		return (OPENFAIL);
	so->so_state |= SS_NBIO;

	/*
	 * Set the socket options: SO_REUSEADDR & SO_KEEPALIVE
	 * and send/recv socket buffer sizes for RFS applications.
	 * Criteria for setting socket buffer sizes:
	 * size < SM_MAX * MCLBYTES / (2 * MSIZE + MCLBYTES)
	 * taken from sbreserve(), but not checked here to
	 * imporve performance.
	 */

	so->so_options |= SO_REUSEADDR;
	so->so_options |= SO_KEEPALIVE;

	so->so_rcv.sb_hiwat = tcptli_recvsize;
	so->so_rcv.sb_mbmax = MIN(tcptli_recvsize * 2, SB_MAX);
	so->so_snd.sb_hiwat = tcptli_sendsize;
	so->so_snd.sb_mbmax = MIN(tcptli_sendsize * 2, SB_MAX);
	/*
	 * Setup the special socket wakeup hooks here
	 */
	tsp = tcptli_mksockinfo(ttp);
	if (!tsp)
		goto release;
	tsp->ts_seqnum = 0;
	tsp->ts_flags = TT_TS_NOTUSED;
	so->so_wupalt = &tsp->ts_sowakeup;
	ttp->tt_so = so;

	/*
	 * Set up cross-links between device and queues.
	 */
	ttp->tt_rq = q;
	q->q_ptr = (char *)ttp;
	WR(q)->q_ptr = (char *)ttp;

	/*
	 * Perform tli automaton state transition to the initial state.
	 */
	if (ttp->tt_state != TL_UNINIT) {
		/*
		 * This check counts on TL_UNINIT == 0.
		 */
		panic("tcptli_open: bogus tli state at open\n");
	}
	ttp->tt_state = TL_UNBND;

	ttp->tt_flags = TT_OPEN;

	ttp->tt_event = 0;

	/*
	 * Start up the auxiliary process if this is the
	 * first instance.
	 * Otherwise just increment the open ttp count.
	 */
	if (!tcptli_ttpcount++) {
		tcptli_ttp = NULL;
		kern_proc((void (*)())tcptli_main, (int)NULL);
		/*
		 * parent returns here -- child cannot, eventually exits
		 */
		TCPTLI_PRINTF("tcptli_open: parent returning\n");
	}

	return (unit);

release:
	ttp->tt_flags &= ~TT_OPEN;	/* mark it not used */
	if (ttp->tt_so) {		/* release socket */
		(void) soclose(ttp->tt_so);
		ttp->tt_so = NULL;
	}
	/*
	 * Free pre-allocated resources
	 */
	if (ttp->tt_merror) {
		freeb(ttp->tt_merror);
		ttp->tt_merror = NULL;
	}
	if (ttp->tt_errack) {
		freeb(ttp->tt_errack);
		ttp->tt_errack = NULL;
	}
	ttp->tt_rq = NULL;		/* break the queue */
	q->q_ptr = NULL;
	WR(q)->q_ptr = NULL;
	ttp->tt_state = TL_UNINIT;	/* set tli transition state */
	return (OPENFAIL);
}


/* Release various resources when starting auxiliary process */
tcptli_memfree()
{
	int 			i;
	/*
	 * root vnode set during system init
	 */
	extern	struct vnode	*rootdir;

	VN_RELE(u.u_cdir);
	u.u_cdir = rootdir;
	VN_HOLD(u.u_cdir);
	if (u.u_rdir) {
		VN_RELE(u.u_rdir);
	}
	u.u_rdir = NULL;
	/* Close all open files, including the partially open tcptli device */
	for (i = 0; i < u.u_lastfile + 1; i++)
		if (u.u_ofile[i] != NULL) {
			closef(u.u_ofile[i]);
			u.u_ofile[i] = NULL;
			u.u_pofile[i] = 0;
		}
}


/*
 * When the TT_CLOSE flag is set, it means the ttp is in the
 * process of closing/clean-up. All requests will be ignored.
 *
 * When the flag (ttp->tt_flag = 0) is 0, that means it is
 * no longer possible to have any event outstanding and
 * there should be no ttp pending (i.e. on the pending queue).
 */
tcptli_close(q)
	register queue_t	*q;
{
	register struct tt_softc	*ttp = (struct tt_softc *)q->q_ptr;
	register int			error = 0;
	register int			s;
	int				tcptli_timeout();

	TCPTLI_PRINTF("tcptli_close-%d: q: %x, WR(q): %x \n",
		ttp - tt_softc, q, WR(q));

	/*
	 * Flush outstanding traffic on its way
	 * downstream to us.
	 */
	flushq(WR(q), FLUSHALL);

	/*
	 * If a timeout has been issued, cancel it.
	 */
	if (ttp->tt_flags & TT_TIMER)
		untimeout((int (*)())tcptli_timeout, (caddr_t)ttp);

	ttp->tt_flags = TT_CLOSE;

	/*
	 * Close the socket associated with this device instance.
	 */
	if (ttp->tt_so) {
		error = soclose(ttp->tt_so);
		ttp->tt_so = NULL;
	}

	/*
	 * Perform tli automaton state transition.
	 *
	 */
	ttp->tt_state = TL_UNINIT;

	/*
	 * Free unused pre-allocated resources.
	 */
	if (ttp->tt_merror) {
		freeb(ttp->tt_merror);
		ttp->tt_merror = NULL;
	}
	if (ttp->tt_errack) {
		freeb(ttp->tt_errack);
		ttp->tt_errack = NULL;
	}


	/*
	 * Clear the ttp->tt_flags to indicate this ttp is
	 * not in use any more.
	 * Decrement the count on opened ttp, wake up
	 * aux proc if this is the last one
	 */
	if (--tcptli_ttpcount == 0) {
		struct	wait4_args	args;

		/*
		 * If we get here, we are ready to shutdown the
		 * aux proc.
		 */
		(void) wakeup((caddr_t)&tcptli_ttp);

		/*
		 * Wait for the aux proc to exit
		 * If the aux_proc is not our child, then
		 * the parent does not wait. The ttp->tt_flags
		 * is cleared by the aux_proc before
		 * it exits, so we are ok here.
		 */
		args.pid = tcptli_auxprocp->p_pid;
		args.status = (union wait *) NULL;
		args.options = 0;
		args.rusage = (struct rusage *) NULL;

		wait4(&args);

	}

	/*
	 * More cleanup: remove any outstanding
	 * event on the pending queue.
	 *
	 * Do the search only if the ttp has been put on the queue.
	 *
	 * If there is only 1 event on the queue, just remove it.
	 * Otherwise, march down the singlely-linked list to
	 * find the match.
	 */
	s = splnet();
	if ((ttp->tt_event & TTE_ONQUEUE) && (tcptli_ttp != NULL)) {
		register struct tt_softc	*tp;	/* current ptr */
		register struct tt_softc	*btp;	/* previous ptr */

		if (tcptli_ttp->tt_next == NULL) {
			tcptli_ttp = NULL;
		} else {
			for (btp = tp = tcptli_ttp; tp; btp = tp,
				tp = tp->tt_next) {
				if (tp == ttp) {
					if (btp == tp) /* 1st in queue */
						tcptli_ttp =
							tcptli_ttp->tt_next;
					else
						btp->tt_next = tp->tt_next;
					break;
				}
			}
		}
	}
	(void) splx(s);
	/*
	 * Break association between queue and driver instance.
	 */
	TCPTLI_PRINTF("ttp-%d: %x, ttp->tt_rq: %x closed\n",
		ttp - tt_softc, ttp, ttp->tt_rq);
	ttp->tt_rq = NULL;
	ttp->tt_flags = 0;

	if (error)
		u.u_error = error;
}
/*
 * Wake up routine to be used by the main process
 * to wake up the aux proc so it will process messages
 * queued on the WR(q).
 *
 */
/*ARGSUSED*/
int
tcptli_wakeup(so, info, warg)
	struct	socket		*so;
	caddr_t			info,
				warg;
{
	register struct	tt_sockinfo	*tsp = (struct tt_sockinfo *)warg;
	register struct	tt_softc	*ttp = tsp->ts_ttp;
	register int			s;

	TCPTLI_PRINTF("tcptli_wakeup: ttp-%d : %x socket: %x info: %d\n",
		ttp - tt_softc, ttp, so, info);

	/*
	 * If info == 0, then it is not a real wakeup, but
	 * an indication that we are called from sofree.
	 * So we free the tt_sockinfo memory, and also
	 * check to see if the freeing socket is the same
	 * kept by ttp. If yes, then set ttp->tt_so = NULL
	 * because when the aux proc wakes up, we can check
	 * to see if the socket has gone away.
	 */
	if (info == 0) {
		if (so == tsp->ts_ttp->tt_so)
			tsp->ts_ttp->tt_so = NULL;
		so->so_wupalt = NULL;
		kmem_free((caddr_t)tsp, sizeof (*tsp));
	} else if (ttp->tt_flags & TT_CLOSE || ttp->tt_flags == 0) {
		return;
	} else {
		/*
		 * Check if read is available, if so
		 * calls tcptli_Ercv() directly. TT_DIRECT_READ
		 * is a C-macro defined in nettli/tcp_tlivar.h.
		 *
		 * If this ttp is already on the queue pending
		 * for processing, then no insertion of ttp is needed.
		 */
		TT_DIRECT_READ(ttp, so);

		s = splnet();
		if (!(ttp->tt_event & TTE_ONQUEUE)) {
			ttp->tt_event |= TTE_EVENT;
			tcptli_insq(ttp);
		}
		(void) splx(s);
		(void) wakeup((caddr_t)&tcptli_ttp);
	}
	return;
}
/*
 * Insert to the tail of the tcptli_ttp queue.
 * When an ttp is inserted in the queue, the ttp->tt_event's
 * TTE_ONQUEUE flag is set. It is cleared when it is removed
 * from the list.
 * Both tcptli_insq() and tcptli_remq() are called at splnet
 * level.
 */
tcptli_insq(ttp)
register struct tt_softc	*ttp;
{
	register struct tt_softc	*tp = tcptli_ttp;

	TCPTLI_PRINTF("Tcptli_insq: ttp-%d : %x\n", ttp - tt_softc, ttp);
	if (tp) {
		while (tp->tt_next)
			tp = tp->tt_next;
		tp->tt_next = ttp;
	} else {			/* first time */
		tcptli_ttp = ttp;
	}
	ttp->tt_event |= TTE_ONQUEUE;	/* mark it's on the queue */
	ttp->tt_next = NULL;		/* mark as the last in queue */
}

/*
 * Remove from the head of the tcptli_ttp queue
 */
struct tt_softc *
tcptli_remq(tp)
	register struct tt_softc	*tp;
{

	if (tp == NULL) {
		TCPTLI_PRINTF("Tcptli_remq: returning NULL\n");
		return ((struct tt_softc *)NULL);
	} else {
		tcptli_ttp = tp->tt_next;
		tp->tt_event &= ~TTE_ONQUEUE;
		TCPTLI_PRINTF("Tcptli_remq: returning ttp-%d : %x\n",
			tp - tt_softc, tp);
		return (tp);
	}
}
/*
 * Write-side put procedure.
 *
 * Streams-specific messages are processed immediately.  All other
 * (tli-related) messages are put onto the write queue for processing
 * by the auxiliary process.
 *
 * Stated differently, we maintain the following invariant on the write
 * queue: the write queue contains only messages that require action
 * by the auxiliary process and none relating only to the streams framework.
 */
tcptli_wput (q, mp)
	queue_t	*q;
	mblk_t	*mp;
{
	register struct tt_softc	*ttp = (struct tt_softc *)q->q_ptr;
	register int			s;

	/*
	 * If we've issued an M_ERROR on this stream,
	 * we can accept no new messages.
	 */
	if (ttp->tt_flags & TT_ERROR) {
		freemsg(mp);
		return;
	}

	switch (mp->b_datap->db_type) {

	case M_FLUSH:
		TCPTLI_PRINTF("tcptli_wput-%d: M_FLUSH q: %x, mp: %x\n",
			ttp - tt_softc, q, mp);
		if (*mp->b_rptr & FLUSHW) {

			flushq(q, FLUSHALL);
			/*
			 * Besides flushing the stream write queue,
			 * we must flush our associated socket's write
			 * queue as well.
			 *
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
		} else
			freemsg(mp);
		return;

	case M_PCPROTO:
		/*
		 * Priority request: validity check the message type,
		 * putting it onto the write queue if it's a legitimate
		 * tli message.
		 */
		TCPTLI_PRINTF("tcptli_wput-%d: M_PCPROTO q: %x, mp: %x\n",
			ttp - tt_softc, q, mp);
		switch (((union T_primitives *)mp->b_rptr)->type) {

		case T_INFO_REQ:
			putq(q, mp);
			s = splnet();
			if (!(ttp->tt_event & TTE_ONQUEUE)) {
				ttp->tt_event |= TTE_EVENT;
				tcptli_insq(ttp);
			}
			(void) splx(s);
			(void) wakeup((caddr_t)&tcptli_ttp);
			return;

		default:
			/* Unknown message type. */
			break;
		}

		break;

	case M_PROTO:
		/*
		 * In-band request: validity check the message type,
		 * putting it onto the write queue if it's a legitimate
		 * tli message.
		 */
		TCPTLI_PRINTF("tcptli_wput-%d: M_PROTO q: %x, mp: %x\n",
			ttp -tt_softc, q, mp);
		switch (((union T_primitives *)mp->b_rptr)->type) {

		case T_CONN_REQ:
		case T_BIND_REQ:
		case T_DISCON_REQ:
		case T_CONN_RES:
		case T_DATA_REQ:
		case T_EXDATA_REQ:
		case T_UNBIND_REQ:
		case T_OPTMGMT_REQ:
		case T_ORDREL_REQ:
			putq(q, mp);
			s = splnet();
			if (!(ttp->tt_event & TTE_ONQUEUE)) {
				ttp->tt_event |= TTE_EVENT;
				tcptli_insq(ttp);
			}
			(void) splx(s);
			(void) wakeup((caddr_t)&tcptli_ttp);
			return;

		default:
			/* Unknown message type. */
			break;
		}

		break;

	case M_DATA:
		/*
		 * M_DATA msgs are procssed in tcptli_usrreq()
		 * as it may sleep while getting mbufs, hence
		 * we need context.
		 */
		TCPTLI_PRINTF("tcptli_wput-%d: M_DATA q: %x, mp: %x\n",
			ttp - tt_softc, q, mp);
		putq(q, mp);
		s = splnet();
		if (!(ttp->tt_event & TTE_ONQUEUE)) {
			ttp->tt_event |= TTE_EVENT;
			tcptli_insq(ttp);
		}
		(void) splx(s);
		(void) wakeup((caddr_t)&tcptli_ttp);
		return;

	default:
		/* Unknown message type. */
		break;
	}

	/*
	 * We should probably generate an error message
	 * here rather than dropping the message.
	 */
	freemsg(mp);
}


/*
 * N.B.: tcptli_wsrv appears in tcp_tliaux.c, not here, since the
 * auxiliary process invokes it, not the streams framework code.
 * This is just a null procedure but the existence of the
 * function call must exist otherwise flow control routines
 * such as canput() will not work.
 */
int
tcptli_wsrv()
{
}


/*
 * Process tli primitive originated by the transport user.  Return
 * zero iff there were insufficient resources to process the primitive.
 *
 * Since we have no guarantees about the size of the request message,
 * we can't count on it being large enough to contain the corresponding
 * reply.  So in each processing of tli event, we allocate the
 * buffer for error msg so we don't have to risk the
 * possibility of allocation failure.  The canputback argument is useful
 * in this regard, being nonzero when it's allowable to push the incoming
 * message back onto the write queue, thereby avoiding the necessity of
 * having to deal with the failure.
 */
int
tcptli_usrreq(ttp)
	struct	tt_softc	*ttp;
{
	/*
	 * The following variables are used in almost all cases
	 * of the switch below, so are declared here.
	 */
	register queue_t		*q = (OTHERQ(ttp->tt_rq));
	register mblk_t			*mp;
	int				error = 0;
	long				type;
	struct tt_sockinfo		*tcptli_mksockinfo();

	while ((mp = getq(q))) {
		/*
		 * M_DATA msg are processed here as it can sleep
		 * while getting mbufs down below.
		 */
		if (mp->b_datap->db_type == M_DATA) {
			/*
			 * Since this msg does not require an ack, so the
			 * reply msg buffer is allocated by tcptli_senddata()
			 * so it decides to use it for error msg, or free
			 * it if no error.
			 */
			error = tcptli_senddata(ttp, mp, 0);
			/*
			 * Error is an indication if the underlying
			 * code has encountered difficulties in sending
			 * data. If error is set, we return here. Otherwise,
			 * we go to the beginning of the while loop and
			 * get the next msg off the queue.
			 */
			if (error)
				return;
			else
				continue;
		}
		/*
		 * Big case machine.
		 */

		type = ((union T_primitives *)mp->b_rptr)->type;
		switch (type) {

			case T_CONN_REQ: {
				(void) tcptli_t_conn_req(q, mp, ttp, type);
				break;
				}

			case T_CONN_RES: {
				(void) tcptli_t_conn_res(q, mp, ttp, type);
				break;
				}

			case T_DISCON_REQ: {
				(void) tcptli_t_discon_req(q, mp, ttp, type);
				break;
				}

			case T_DATA_REQ: {
				(void) tcptli_t_data_req(q, mp, ttp, type);
				break;
				}

			case T_EXDATA_REQ: {
				(void) tcptli_t_exdata_req(q, mp, ttp, type);
				break;
				}

			case T_INFO_REQ: {
				(void) tcptli_t_info_req(q, mp, ttp, type);
				break;
			}

			case T_BIND_REQ: {
				(void) tcptli_t_bind_req(q, mp, ttp, type);
				break;
			}

			case T_UNBIND_REQ: {
				(void) tcptli_t_unbind_req(q, mp, ttp, type);
				break;
			}

			case T_OPTMGMT_REQ: {
				(void) tcptli_t_optmgmt_req(q, mp, ttp, type);
				break;
			}

			case T_ORDREL_REQ: {
				(void) tcptli_t_ordrel_req(q, mp, ttp);
				break;
			}

			default: {
				(void) tcptli_error(q, mp, type, TBADDATA, 0);
			}
		}	/* end of switch */
	}		/* end of while */
	return;
}

/*
 * This primitive has no direct 4.3 equivalent.  Its semantics
 * are to deny a pending connection request (even one we initiated)
 * or to disconnect an existing connection.
 */
tcptli_t_discon_req(q, mp, ttp, type)
	queue_t	*q;
	mblk_t	*mp;
	struct	tt_softc *ttp;
	long	type;
{
	register struct T_discon_req	*rp;
	register struct T_ok_ack	*rap;
	mblk_t				*mfp = NULL;
	mblk_t				*mrp;
	struct	socket			*so = ttp->tt_so;
	struct	socket			*soq;
	int				fatalerror = 0;
	int				error = 0;
	int				tcptli_sendsize = TT_SENDSIZE;
	int				tcptli_recvsize = TT_RECVSIZE;

	TCPTLI_PRINTF("tcptli-%d: ttp: %x tt_so: %x T_DISCON_REQ\n",
		ttp - tt_softc, ttp, ttp->tt_so);
	rp = (struct T_discon_req *)mp->b_rptr;

	/*
	 * Do preliminary validity-checking on the request,
	 * starting by verifying that we're not in a state
	 * from which this request can't be valid.
	 */
	if (ttp->tt_state & (TL_IDLE | TL_UNBND | TL_UNINIT)) {
		(void) tcptli_error(q, mp, type, TOUTSTATE, 0);
		return;
	}
	/*
	 * Since we're doing TCP, which has no concept of
	 * connection-time data, there can be no attached
	 * M_DATA mblks.
	 */
	if (mp->b_cont) {
		(void) tcptli_error(q, mp, type, TBADDATA, 0);
		return;
	}
	/*
	 * Check both cases when SEQ_number == -1 and
	 * SEQ_number != 1 against ttp->state to
	 * catch if a bogus SEQ_number is received.
	 */
	if (rp->SEQ_number == -1 &&
	    !(ttp->tt_state & (TL_DATAXFER | TL_INREL | TL_OUTREL))) {
		(void) tcptli_error(q, mp, type, TBADSEQ, 0);
		return;
	}
	if (rp->SEQ_number != -1 &&
	    !(ttp->tt_state & (TL_INCON | TL_OUTCON))) {
		(void) tcptli_error(q, mp, type, TBADSEQ, 0);
		return;
	}
	/*
	 * Before proceeding further, latch onto enough message
	 * resources to satisfy our requirements below.
	 */
	mrp = allocb(sizeof (union T_primitives), BPRI_HI);
	if (!mrp) {
		(void) tcptli_error(q, mp, type, TSYSERR, ENOBUFS);
		return;
	}
	/*
	 * The sequence number must match an outstanding
	 * connection request, or must be -1 if already
	 * connected.
	 */
	if (rp->SEQ_number != -1) {	/* for the pending side */
		/*
		 * Find matching pending connection.
		 */
		for (soq = so->so_q; soq != so; soq = soq->so_q) {
			if (soq->so_wupalt &&
				((struct tt_sockinfo *)soq->so_wupalt->
				wup_arg)->ts_seqnum == rp->SEQ_number)
				break;
		}
		if (soq == so) {
			(void) tcptli_error(q, mrp, type, TBADSEQ, 0);
			freemsg(mp);
			return;
		}
		/*
		 * Zap the connection here ... before calling
		 * soclose(), clear the SS_NOFDREF flag. Just
		 * some socket terminalogy we have to follow (Gross)
		 */
		soq->so_state &= ~SS_NOFDREF;
		(void) soclose(soq);
		ttp->tt_state = so->so_q == so ? TL_IDLE : TL_INCON;
	} else {			/* for the connected side */
		/*
		 * Save the sockaddr info from the socket to be dropped
		 * into a tmp buffer. Close the requested drop socket, create
		 * a new one, and bind with the saved sockaddr.
		 */
		struct	sockaddr_in	sa;
		struct	inpcb		*inp = sotoinpcb(so);
		struct	socket		*newso;
		struct	tt_sockinfo	*tsp;
		struct	tt_sockinfo	*tcptli_mksockinfo();
		struct	mbuf		*nam = NULL;

		bzero((caddr_t)&sa, sizeof (sa));
		sa.sin_family = AF_INET;
		sa.sin_port = inp->inp_lport;
		sa.sin_addr = inp->inp_laddr;
		(void) soclose(so);
		ttp->tt_so = NULL;
		if (error = socreate(AF_INET, &newso, SOCK_STREAM, 0)) {
			fatalerror = 1;
			goto reply;
		}

		newso->so_state |= SS_NBIO;

		/*
		 * Set the socket options and send/recv socket buffer.
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
			fatalerror = 1;
			goto reply;
		}
		tsp->ts_seqnum = 0;
		tsp->ts_flags = TT_TS_NOTUSED;
		newso->so_wupalt = &tsp->ts_sowakeup;

		(void) tcptli_sockargs(&nam, (caddr_t)&sa, sizeof (sa),
			MT_SONAME);
		if (error = sobind(newso, nam)) {
			fatalerror = 1;
			goto reply;
		}
		if (nam)
			m_freem(nam);

		inp = sotoinpcb(newso);
		ttp->tt_laddr = inp->inp_laddr;	/* save the port and addr */
		ttp->tt_lport = inp->inp_lport;
		ttp->tt_so = newso;		/* attach new socket */
		ttp->tt_state = TL_IDLE;	/* set TLI state */
	}

	/*
	 * If we're in the TL_DATAXFER, TL_OUTREL, or TL_INREL
	 * state, we must issue M_FLUSH for both the read
	 * and write queues before sending back the ack.
	 * (We must also do so before sending back an
	 * unsolicited T_DISCONN_IND.)  Allocate here, and
	 * send after we've gathered the rest of our resources.
	 */
	mfp = allocb(sizeof (char), BPRI_HI);
	if (!mfp) {
		(void) tcptli_error(q, mrp, type, TSYSERR, ENOBUFS);
		freemsg(mp);
		return;
	}
	/*
	 * Flush if required.
	 */
	if (mfp) {
		mfp->b_datap->db_type = M_FLUSH;
		mfp->b_wptr++;
		*mfp->b_rptr = FLUSHR | FLUSHW;

		qreply(q, mfp);
	}

reply:
	/*
	 * Acknowledge success.
	 */
	mrp->b_datap->db_type = M_PCPROTO;
	mrp->b_wptr += sizeof (struct T_ok_ack);
	rap = (struct T_ok_ack *)mrp->b_rptr;
	rap->PRIM_type = T_OK_ACK;

	rap->CORRECT_prim = T_DISCON_REQ;
	qreply(q, mrp);

	if (fatalerror) {
		(void) tcptli_error(q, mp, type, NULL, error);
		ttp->tt_state = TL_UNBND;
	} else
		freemsg(mp);
	return;
}
/*
 * This primitive corresponds to the first half of the 4.3 connect
 * call; i.e., the transport user has expressed desire to connect
 * to a foreign peer.  (The second half corresponds to the
 * T_CONN_CON primitive.)
 */
tcptli_t_conn_req(q, mp, ttp, type)
	queue_t	*q;
	mblk_t	*mp;
	struct	tt_softc *ttp;
	long	type;
{
	register struct T_conn_req	*rp;
	register mblk_t			*mrp;	/* reply message */
	register int			error;
	struct T_ok_ack			*rap;
	struct mbuf			*nam;
	int				offset;
	struct	sockaddr_in		sa;

	TCPTLI_PRINTF("tcptli-%d: ttp: %x tt_so: %x T_CONN_REQ\n",
		ttp - tt_softc, ttp, ttp->tt_so);

	rp = (struct T_conn_req *)mp->b_rptr;
	offset = rp->DEST_offset;

	if (ttp->tt_state != TL_IDLE) {
		(void) tcptli_error(q, mp, type, TOUTSTATE, 0);
		return;
	}

	/*
	 * Do preliminary validity-checking on the request.
	 * Since we're doing TCP, which has no concept of
	 * connection-time data, there can be no attached
	 * M_DATA mblks.
	 */
	if (mp->b_cont) {
		(void) tcptli_error(q, mp, type, TBADDATA, 0);
		return;
	}
	/*
	 * The (initial?) implementation doesn't support
	 * connection-time options, so verify that OPT_length == 0.
	 * (If it ever does support such options, they'll only
	 * be setsockopt-style options, not ones that go across
	 * the wire.)
	 */
	if (rp->OPT_length != 0) {
		(void) tcptli_error(q, mp, type, TBADOPT, 0);
		return;
	}

	/*
	 * Before proceeding further, latch onto enough message
	 * resources to satisfy our requirements below.
	 */
	mrp = allocb(sizeof (union T_primitives), BPRI_HI);
	if (!mrp) {
		(void) tcptli_error(q, mp, type, TSYSERR, ENOBUFS);
		return;
	}

	/*
	 * Stuff the user supplied address into an mbuf in preparation
	 * for the soconnect.
	 */
	(void) bzero((caddr_t)sa.sin_zero, 8);
	(void) bcopy((caddr_t)(mp->b_rptr + offset), (caddr_t)&sa,
		sizeof (struct taddr_in));
	error = tcptli_sockargs(&nam, (caddr_t)&sa,
		sizeof (struct sockaddr_in), MT_SONAME);
	if (error) {
		freemsg(mp);
		(void) tcptli_error(q, mrp, type, TSYSERR, error);
		return;
	}

	error = soconnect(ttp->tt_so, nam);
	if (nam)
		m_freem(nam);
	if (error) {
		int	te;

		freemsg(mp);
		if (error == EACCES)
			te = TACCES;
		else
			te = TSYSERR;
		(void) tcptli_error(q, mrp, type, te,
			te == TSYSERR ? error : 0);
		return;
	}

	/*
	 * Perform tli automaton state transition.
	 */
	ttp->tt_state = TL_OUTCON;

	/*
	 * Acknowledge success.
	 */
	freemsg(mp);
	mrp->b_datap->db_type = M_PCPROTO;
	mrp->b_wptr += sizeof (struct T_ok_ack);
	rap = (struct T_ok_ack *)mrp->b_rptr;
	rap->PRIM_type = T_OK_ACK;

	rap->CORRECT_prim = T_CONN_REQ;
	qreply(q, mrp);

	return;
}
/*
 * This primitive corresponds to the 4.3 accept call.
 *
 * Semantic differences: 1) It's acceptable to accept on the
 * listening stream; 2) The transport user must pass in a reference
 * to a correctly bound stream on which to accept the connection.
 */
tcptli_t_conn_res(q, mp, ttp, type)
	queue_t	*q;
	mblk_t	*mp;
	struct	tt_softc *ttp;
	long	type;
{
	struct T_conn_res		*rp;
	struct T_ok_ack			*rap;
	register struct tt_softc	*ttap;
	register mblk_t			*mrp;	/* reply message */
	struct mbuf			*nam;
	register int			acceptonlisten;
	register int			numpend = 0;
	register struct	socket		*so = ttp->tt_so;
	register struct socket		*soq;
	struct	tt_sockinfo		*tsp;
	struct	sockbuf			*ssb;
	struct	sockbuf			*srb;
	int				error;

	TCPTLI_PRINTF("tcptli-%d: ttp: %x tt_so: %x T_CONN_RES\n",
		ttp - tt_softc, ttp, ttp->tt_so);

	rp = (struct T_conn_res *)mp->b_rptr;

	/*
	 * Do preliminary validity-checking on the request,
	 * first verifying that we're starting from the right state.
	 */
	if (ttp->tt_state != TL_INCON) {
		(void) tcptli_error(q, mp, type, TOUTSTATE, 0);
		return;
	}
	/*
	 * Since we're doing TCP, which has no concept of
	 * connection-time data, there can be no attached
	 * M_DATA mblks.
	 */
	if (mp->b_cont) {
		(void) tcptli_error(q, mp, type, TBADDATA, 0);
		return;
	}
	/*
	 * The (initial?) implementation doesn't support
	 * connection-time options, so verify that OPT_length == 0.
	 * (If it ever does support such options, they'll only
	 * be setsockopt-style options, not ones that go across
	 * the wire.)
	 */
	if (rp->OPT_length != 0) {
		(void) tcptli_error(q, mp, type, TBADOPT, 0);
		return;
	}

	/*
	 * Before proceeding further, latch onto enough message
	 * resources to satisfy our requirements below.
	 */
	mrp = allocb(sizeof (union T_primitives), BPRI_HI);
	if (!mrp) {
		(void) tcptli_error(q, mp, type, TSYSERR, ENOBUFS);
		return;
	}

	/*
	 * Find the device instance that matches the read queue
	 */
	for (ttap = tt_softc; ttap < &tt_softc[tcptli_limit]; ttap++) {
		if (ttap->tt_rq == rp->QUEUE_ptr)
			break;
	}
	if (ttap == &tt_softc[tcptli_limit]) {	/* no match */
		(void) tcptli_error(q, mrp, type, TBADF, 0);
		freemsg(mp);
		return;
	}

	/*
	 * Find the total number of pending connections,
	 * ignoring those without wupalt setup
	 */
	for (soq = so->so_q; soq != so; soq = soq->so_q) {
		if (soq->so_wupalt != NULL)
			numpend++;
	}
	acceptonlisten = ttap == ttp;
	TCPTLI_PRINTF("CONN_RES: ttp[%x] ttap[%x] numpend[%d]\n",
		ttp, ttap, numpend);
	/*
	 * Check if excess pending connections. If accept
	 * on the same fd as the one we listen, there should
	 * be only 1 pending.
	 */
	if (acceptonlisten && numpend != 1) {
		(void) tcptli_error(q, mrp, type, TOUTSTATE, 0);
		freemsg(mp);
		return;
	}

	/*
	 * If not accepting on the same fd, then the new fd
	 * should not have done a bind/listen on it.
	 */
	if (!acceptonlisten && ttap->tt_state != TL_IDLE) {
		(void) tcptli_error(q, mrp, type, TOUTSTATE, 0);
		freemsg(mp);
		return;
	}
	/*
	 * Determine the pending socket that matches the
	 * sequence number supplied by the user
	 */
	for (soq = so->so_q; soq != so; soq = soq->so_q) {
		if (soq->so_wupalt &&
			((struct tt_sockinfo *)soq->so_wupalt->wup_arg)
			->ts_seqnum == rp->SEQ_number)
			break;
	}

	TCPTLI_PRINTF("CONN_RES: soq [%x] so [%x]\n", soq, so);
	TCPTLI_PRINTF("CONN_RES: Seq No = %d\n", rp->SEQ_number);
	if (soq == so) {	/* end of list, no match */
		(void) tcptli_error(q, mrp, type, TBADSEQ, 0);
		freemsg(mp);
		return;
	}

	/*
	 * Remove the pending connection's associated socket from
	 * the listening socket's queue and pass it to soaccept
	 * to complete the connection.
	 */
	if (soqremque(soq, 1) == 0)
		panic("tcptli: inconsistent queues");
	nam = m_get(M_WAIT, MT_SONAME);
	if (error = soaccept(soq, nam)) {
		(void) tcptli_error(q, mrp, type, NULL, error);
		freemsg(mp);
		m_freem(nam);
		return;
	}

	/*
	 * Mark the Sequence number -1 since it is now connected
	 */
	tsp = (struct tt_sockinfo *)soq->so_wupalt->wup_arg;
	tsp->ts_seqnum = -1;

	/*
	 * Update the ttp ptr in tt_sockinfo, and copy the
	 * sockbuf options from the head scoket to this new one
	 */
	tsp->ts_ttp = ttap;
	ssb = &ttap->tt_so->so_snd;
	srb = &ttap->tt_so->so_rcv;
	soq->so_snd.sb_hiwat = ssb->sb_hiwat;
	soq->so_snd.sb_mbmax = ssb->sb_mbmax;
	soq->so_rcv.sb_hiwat = srb->sb_hiwat;
	soq->so_rcv.sb_mbmax = srb->sb_mbmax;

	/*
	 * Close the head socket and replace by the new one
	 */
	if (ttap->tt_so)
		(void) soclose(ttap->tt_so);
	ttap->tt_so = soq;

	/*
	 * Turn on the TF_NODELAY option
	 */
	{
		struct inpcb *inp = sotoinpcb(ttap->tt_so);
		struct tcpcb *tp  = intotcpcb(inp);

		tp->t_flags |= TF_NODELAY;

	}
	/*
	 * Perform tli automaton state transitions for both this
	 * stream and the one that has gained the connection.
	 */
	ttap->tt_state = TL_DATAXFER;
	m_freem(nam);
	/*
	 * Acknowledge success.
	 */
	freemsg(mp);
	mrp->b_datap->db_type = M_PCPROTO;
	mrp->b_wptr += sizeof (struct T_ok_ack);
	rap = (struct T_ok_ack *)mrp->b_rptr;
	rap->PRIM_type = T_OK_ACK;

	rap->CORRECT_prim = T_CONN_RES;
	qreply(q, mrp);

	return;
}

/*
 * The processing is similar to M_DATA, except in this
 * case, we skip over the TI header. Note, no reply
 * msg buffer is allocated here, as it is not required
 * to issue an acknowledgement back to the user.
 * We call the same tcptli_senddata() as M_DATA case,
 * which takes care of issue error msg to the user if
 * error encountered.
 */
tcptli_t_data_req(q, mp, ttp, type)
	queue_t	*q;
	mblk_t	*mp;
	struct	tt_softc *ttp;
	long	type;
{
	register mblk_t			*mbp;

	TCPTLI_PRINTF("tcptli-%d: ttp: %x tt_so: %x T_DATA_REQ\n",
		ttp - tt_softc, ttp, ttp->tt_so);

	/*
	 * Verify that we're in the proper state for the request.
	 */
	if (!(ttp->tt_state & (TL_DATAXFER | TL_INREL))) {
		(void) tcptli_error(q, mp, type, TOUTSTATE, 0);
		return (EINVAL);
	}
	/* Legal message must have a data block after the TI header block */
	mbp = mp->b_cont;
	if (!mbp || mbp->b_datap->db_type != M_DATA) {
		freemsg(mp);
		(void) tcptli_error(q, (mblk_t *)NULL, type, TBADDATA, 0);
		return (EINVAL);
	}

	/* Get rid of the TI header block and send the real data */
	mp->b_cont = NULL;
	freeb(mp);
	/*
	 * Data is freed by tcptli_senddata()
	 */
	(void) tcptli_senddata(ttp, mbp, 0);
	return (0);
}

tcptli_t_exdata_req(q, mp, ttp, type)
	queue_t	*q;
	mblk_t	*mp;
	struct	tt_softc *ttp;
	long	type;
{
	register struct T_exdata_req	*rp;
	register mblk_t			*mbp;

	TCPTLI_PRINTF("tcptli-%d: ttp: %x tt_so: %x T_EXDATA_REQ\n",
		ttp - tt_softc, ttp, ttp->tt_so);
	rp = (struct T_exdata_req *)mp->b_rptr;

	/*
	 * Verify that we're in the proper state for the request.
	 */
	if (!(ttp->tt_state & (TL_DATAXFER | TL_INREL))) {
		(void) tcptli_error(q, mp, type, TOUTSTATE, 0);
		return (0);
	}
	/*
	 * Chain (MORE_flag > 0) currently not supported for TCP
	 * Expedited data. Assuming urgent data can't exceed 4k.
	 */
	if (rp->MORE_flag > 0) {
		freemsg(mp);
		(void) tcptli_error(q, (mblk_t *)NULL, type, TNOTSUPPORT, 0);
		return (0);
	}
	mbp = mp->b_cont;
	if (!mbp) {
		freemsg(mp);
		(void) tcptli_error(q, (mblk_t *)NULL, type, TBADDATA, 0);
		return (0);
	}
	/* Get rid of the TI header block and send the real data */
	mp->b_cont = NULL;
	freeb(mp);
	/*
	 * Data is freed by tcptli_send()
	 */
	(void) tcptli_senddata(ttp, mbp, MSG_OOB);
	return (0);
}



tcptli_t_info_req(q, mp, ttp, type)
	queue_t	*q;
	mblk_t	*mp;
	struct	tt_softc *ttp;
	long	type;
{
	register struct T_info_ack	*ap;
	register mblk_t			*mrp;	/* reply message */

	TCPTLI_PRINTF("tcptli-%d: ttp: %x tt_so: %x T_INFO_REQ\n",
		ttp - tt_softc, ttp, ttp->tt_so);

	mrp = allocb(sizeof (union T_primitives), BPRI_HI);
	if (!mrp) {
		(void) tcptli_error(q, mp, type, TSYSERR, ENOBUFS);
		return;
	}
	freemsg(mp);
	mrp->b_datap->db_type = M_PCPROTO;
	mrp->b_wptr += sizeof (struct T_info_ack);
	ap = (struct T_info_ack *)mrp->b_rptr;
	ap->PRIM_type = T_INFO_ACK;

	/*
	 *
	 * Questionable choices:
	 *   TSDU_size:
	 *	If this concept is supported at all,
	 *	it probably maps to the tcp segment size.
	 *   ETSDU_size:
	 *	Really this is a bit stating that there's
	 *	urgent data coming further along.
	 *   OPT_size:
	 *	It's still unknown which of the possible
	 *	protocol options make sense for tcp and
	 *	how we can make them correspond to those
	 *	of other streams tcp implementations.
	 *   SERV_type:
	 *	It remains to be seen whether tcp supports
	 *	orderly disconnect in the ISO sense.
	 */
	ap->TSDU_size = 0;
	ap->ETSDU_size = 1;
	ap->CDATA_size = -2;			/* not supported */
	ap->DDATA_size = -2;			/* not supported */
	ap->ADDR_size = sizeof (struct sockaddr_in);
	ap->OPT_size = 0;			/* TBD */
	ap->TIDU_size = strmsgsz;
	ap->SERV_type = T_COTS;		/* ISO orderly rel not supported */
	ap->CURRENT_state =
		tcptli_stateconv(((struct tt_softc *)q->q_ptr)->tt_state);

	qreply(q, mrp);
	return;
}
/*
 * This primitive roughly corresponds to paired 4.3 bind/listen calls.
 */
tcptli_t_bind_req(q, mp, ttp, type)
	queue_t	*q;
	mblk_t	*mp;
	struct	tt_softc *ttp;
	long	type;
{
	register struct T_bind_req	*rp;
	register struct T_bind_ack	*rap;
	register mblk_t			*mrp;	/* reply message */
	register int			error;
	int				offset;
	int				addrlength;
	struct mbuf			*nam = NULL;
	struct sockaddr_in		*sin;

	TCPTLI_PRINTF("tcptli-%d: ttp: %x tt_so: %x T_BIND_REQ\n",
		ttp - tt_softc, ttp, ttp->tt_so);

	/*
	 * Verify that the request is allowable.
	 */
	if (ttp->tt_state != TL_UNBND) {
		(void) tcptli_error(q, mp, type, TOUTSTATE, 0);
		return;
	}

	rp = (struct T_bind_req *)mp->b_rptr;
	offset = rp->ADDR_offset;

	/*
	 * Alloc a reply buffer
	 */
	mrp = allocb(MAX(sizeof (union T_primitives),
			offset + sizeof (struct sockaddr_in)), BPRI_HI);
	if (!mrp) {
		(void) tcptli_error(q, mp, type, TSYSERR, ENOBUFS);
		return;
	}

	/*
	 * Check to see if the user supplied address is either
	 * sockaddr_in type or the once valid (now obsoleted) taddr_in type.
	 * Save the address length because we'll need it in our
	 * reply msg.
	 *
	 * Stuff the address to bind to into an mbuf.  (If one
	 * wasn't supplied, we're supposed to pick one, which
	 * in_pcbbind will do during the call below to sobind.)
	 *
	 */
	if (rp->ADDR_length > 0) {
		struct	sockaddr_in	sa;

		if (rp->ADDR_length == sizeof (struct taddr_in) ||
			rp->ADDR_length == sizeof (struct sockaddr_in)) {
			addrlength = rp->ADDR_length;
			(void) bzero((caddr_t)sa.sin_zero, 8);
			(void) bcopy((caddr_t)(mp->b_rptr + offset),
				(caddr_t)&sa, sizeof (struct taddr_in));
		} else {
			freemsg(mp);
			(void) tcptli_error(q, mrp, type, TBADADDR, 0);
			return;
		}

		error = tcptli_sockargs(&nam, (caddr_t)&sa,
			sizeof (struct sockaddr_in), MT_SONAME);
		if (error) {
			freemsg(mp);
			(void) tcptli_error(q, mrp, type, TSYSERR, error);
			return;
		}
	}

	/*
	 * If CONIND_number > 0, we must verify that no other
	 * socket is listening (in the 4.3 sense) on the same
	 * address.
	 */
	if (nam) {
		extern struct in_addr	zeroin_addr;

		sin = mtod(nam, struct sockaddr_in *);
		if (sin->sin_port && in_pcblookup(
		    sotoinpcb(ttp->tt_so)->inp_head, zeroin_addr,
		    0, sin->sin_addr, sin->sin_port, 0)) {
			freemsg(mp);
			m_freem(nam);
			(void) tcptli_error(q, mrp, type, TNOADDR, 0);
			return;
		}
	}

	/*
	 * Bind the socket, translating unix errors into tli errors.
	 * Also save the port and address in ttp struct for
	 * future reference (a backup) in the case the inpcb
	 * is blown away.
	 */
	error = sobind(ttp->tt_so, nam);
	if (nam)
		m_freem(nam);
	if (error) {
		int	te;

		freemsg(mp);
		if (error == EADDRINUSE || error == EADDRNOTAVAIL)
			te = TNOADDR;
		else if (error == EACCES)
			te = TACCES;
		else
			te = TSYSERR;
		(void) tcptli_error(q, mrp, type, te,
			te == TSYSERR ? error : 0);
		return;
	}

	/*
	 * If CONIND_number != 0, issue a listen call as well.
	 *
	 */
	if (rp->CONIND_number != 0) {
		if (error = solisten(ttp->tt_so, (int)rp->CONIND_number)) {
			if (tcptli_error(q, mrp, type, TSYSERR, error)) {
				(void) soclose(ttp->tt_so);
				ttp->tt_so = NULL;
			}
			freemsg(mp);
			return;
		}
	}

	/*
	 * Generate acknowledgement message by digging resulting
	 * values out of socket and in_pcb structures.
	 */
	offset = sizeof (struct T_bind_ack);
	mrp->b_datap->db_type = M_PCPROTO;
	mrp->b_wptr += offset + sizeof (struct sockaddr_in);
	rap = (struct T_bind_ack *)mrp->b_rptr;
	rap->PRIM_type = T_BIND_ACK;

	/*
	 * If user specifies a zero address length, then
	 * return the default size, which is sockaddr_in
	 */
	if (addrlength == 0)
		addrlength = sizeof (struct sockaddr_in);

	rap->ADDR_length = addrlength;
	rap->ADDR_offset = offset;
	if (rp->CONIND_number != 0)
		rap->CONIND_number = ttp->tt_so->so_qlimit;
	else
		rap->CONIND_number = 0;
	freemsg(mp);
	/*
	 * Copy family, port, and address into mblk to be
	 * sent up to the Transport User.
	 */
	{
		struct sockaddr_in	sa;
		struct inpcb		*inp = sotoinpcb(ttp->tt_so);

		bzero((caddr_t)&sa, sizeof (sa));
		sa.sin_family = AF_INET;
		sa.sin_port = inp->inp_lport;
		sa.sin_addr = inp->inp_laddr;
		ttp->tt_laddr = inp->inp_laddr; /* save the local port/addr */
		ttp->tt_lport = inp->inp_lport;
		bcopy((caddr_t)&sa, (caddr_t)(mrp->b_rptr + offset),
			sizeof (sa));
	}
	qreply(q, mrp);

	/*
	 * Perform tli automaton state transition.
	 */
	ttp->tt_state = TL_IDLE;

	return;
}
/*
 *
 */
tcptli_t_unbind_req(q, mp, ttp, type)
	queue_t	*q;
	mblk_t	*mp;
	struct	tt_softc *ttp;
	long	type;
{
	register mblk_t			*mrp;	/* reply message */
	register struct T_ok_ack	*rap;
	mblk_t				*mfp = NULL;
	struct	tt_sockinfo		*tsp, *tcptli_mksockinfo();
	struct socket 			*so;
	int				error;
	int				fatalerror = 0;
	int				tcptli_sendsize = TT_SENDSIZE;
	int				tcptli_recvsize = TT_RECVSIZE;

	TCPTLI_PRINTF("tcptli-%d: ttp: %x tt_so: %x T_UNBIND_REQ\n",
		ttp - tt_softc, ttp, ttp->tt_so);

	/*
	 * Verify that the request is allowable.
	 */
	if (ttp->tt_state != TL_IDLE) {
		(void) tcptli_error(q, mp, type, TOUTSTATE, 0);
		return;
	}

	/*
	 * Must issue M_FLUSH for both read and write queues
	 * before sending back the ack.
	 */
	mfp = allocb(sizeof (struct T_ok_ack), BPRI_HI);
	if (!mfp) {
		(void) tcptli_error(q, mp, type, TSYSERR, ENOBUFS);
		return;
	}
	mfp->b_datap->db_type = M_FLUSH;
	*(mfp->b_rptr) |= FLUSHRW;
	qreply(q, mfp);

	/*
	 * allocate a reply msg buffer
	 */
	mrp = allocb(sizeof (union T_primitives), BPRI_HI);
	if (!mrp) {
		(void) tcptli_error(q, mp, type, TSYSERR, ENOBUFS);
		return;
	}
	/*
	 * There's some difficulty with leaving the socket in a state
	 * such that it can later be reused.  In_pcbbind refuses to
	 * allow rebinding once already bound.  The only
	 * way is to do this is to issue an soclose
	 * followed by an socreate.
	 */
	if (ttp->tt_so) {
		error = soclose(ttp->tt_so);
		ttp->tt_so = NULL;
	}
	if (error) {
		freemsg(mp);
		(void) tcptli_error(q, mrp, type, TSYSERR, error);
		return;
	}
	error = socreate(AF_INET, &so, SOCK_STREAM, 0);
	if (error) {
		fatalerror = 1;
		goto reply;
	}

	/*
	 * Set the socket options and send/recv socket buffer.
	 * Criteria for setting socket buffer sizes:
	 * size < SM_MAX * MCLBYTES / (2 * MSIZE + MCLBYTES)
	 * taken from sbreserve(), but not checked here to
	 * imporve performance.
	 */

	so->so_options |= SO_REUSEADDR;
	so->so_options |= SO_KEEPALIVE;

	so->so_rcv.sb_hiwat = tcptli_recvsize;
	so->so_rcv.sb_mbmax = MIN(tcptli_recvsize * 2, SB_MAX);
	so->so_snd.sb_hiwat = tcptli_sendsize;
	so->so_snd.sb_mbmax = MIN(tcptli_sendsize * 2, SB_MAX);
	/*
	 * Setup the special socket wakeup hooks here
	 */
	tsp = tcptli_mksockinfo(ttp);
	if (tsp == NULL) {
		fatalerror = 1;
		goto reply;
	}
	tsp->ts_seqnum = 0;
	tsp->ts_flags = TT_TS_NOTUSED;
	so->so_wupalt = &tsp->ts_sowakeup;

	so->so_state |= SS_NBIO;
	ttp->tt_so = so;

reply:
	/*
	 * Acknowledge success.
	 */
	mrp->b_datap->db_type = M_PCPROTO;
	mrp->b_wptr += sizeof (struct T_ok_ack);
	rap = (struct T_ok_ack *)mrp->b_rptr;
	rap->PRIM_type = T_OK_ACK;

	rap->CORRECT_prim = T_UNBIND_REQ;
	qreply(q, mrp);

	/*
	 * Perform tli automaton state transition.
	 */
	ttp->tt_state = TL_UNBND;

	if (fatalerror)
		(void) tcptli_error(q, mp, type, NULL, error);
	else
		freemsg(mp);

	return;
}
tcptli_t_optmgmt_req(q, mp, ttp, type)
	queue_t	*q;
	mblk_t	*mp;
	struct	tt_softc *ttp;
	long	type;
{
	register struct T_optmgmt_req	*rp;
	register struct socket		*so = ttp->tt_so;
	struct	T_optmgmt_ack 		*rap;
	int				offset;

	TCPTLI_PRINTF("tcptli-%d: ttp: %x tt_so: %x T_OPTMGMT_REQ\n",
		ttp - tt_softc, ttp, ttp->tt_so);
	rp = (struct T_optmgmt_req *)mp->b_rptr;
	offset = rp->OPT_offset;

	/*
	 * Verify that the request is allowable; options
	 * may be negotiated only from the unbound state.
	 */
	if (ttp->tt_state != TL_IDLE) {
		(void) tcptli_error(q, mp, type, TOUTSTATE, 0);
		return;
	}
	switch (rp->MGMT_flags) {

	case T_DEFAULT: {
		register mblk_t	*mrp;	/* reply message */
		struct	tt_soopt	*optp;

		mrp = allocb(sizeof (struct T_optmgmt_ack)
			+ sizeof (struct tt_soopt), BPRI_HI);
		if (!mrp) {
			(void) tcptli_error(q, mp, type, TSYSERR, ENOBUFS);
			return;
		}

		freemsg(mp);
		mrp->b_datap->db_type = M_PCPROTO;
		mrp->b_wptr = mrp->b_rptr + sizeof (struct T_optmgmt_ack)
				+ sizeof (struct tt_soopt);
		rap = (struct T_optmgmt_ack *)mp->b_rptr;
		rap->MGMT_flags = T_DEFAULT;
		rap->PRIM_type = T_OPTMGMT_ACK;
		rap->OPT_length = sizeof (struct tt_soopt);
		rap->OPT_offset = sizeof (struct T_optmgmt_ack);
		optp = (struct tt_soopt *)(mrp->b_rptr
			+ sizeof (struct T_optmgmt_ack));
		optp->tts_reuseaddr = TTS_DFLT_REUSEADDR;
		optp->tts_keepalive = TTS_DFLT_KEEPALIVE;
		optp->tts_sendsize = TTS_BUFSIZE;
		optp->tts_recvsize = TTS_BUFSIZE;
		qreply(q, mrp);
		return;

	}
	case T_CHECK: {
		if ((rp->OPT_length != sizeof (struct tt_soopt)) ||
			(rp->OPT_offset < sizeof (struct T_optmgmt_req))) {

			(void) tcptli_error(q, mp, type, TBADOPT, 0);
			return;
		}

		rap = (struct T_optmgmt_ack *)mp->b_rptr;
		rap->PRIM_type = T_OPTMGMT_ACK;
		rap->MGMT_flags |= T_SUCCESS;
		mp->b_datap->db_type = M_PCPROTO;
		qreply(q, mp);
			return;
	}
	case T_NEGOTIATE: {
		struct	tt_soopt	opt,
					*optp;
		struct	mbuf		*m = NULL;
		int			error;


		if ((rp->OPT_length != sizeof (struct tt_soopt)) ||
			(rp->OPT_offset < sizeof (struct T_optmgmt_req))) {

			(void) tcptli_error(q, mp, type, TBADOPT, 0);
			return;
		}

		bcopy((char *)(mp->b_rptr + offset), (char *)&opt,
			sizeof (struct tt_soopt));

		if (opt.tts_reuseaddr) {
			error = sosetopt(so, SOL_SOCKET, SO_REUSEADDR,
				(struct mbuf *)0);
			if (error < 0) {
				(void) tcptli_error(q, mp,
					type, TSYSERR, error);
				return;
			}
		}
		if (opt.tts_keepalive) {
			error = sosetopt(so, SOL_SOCKET, SO_KEEPALIVE,
				(struct mbuf *)0);
			if (error < 0) {
				(void) tcptli_error(q, mp,
					type, TSYSERR, error);
				return;
			}
		}
		if (opt.tts_sendsize > 0) {
			/*
			 * mbuf is freed at the end of sosetopt()
			 */
			m = m_get(M_WAIT, MT_SOOPTS);
			if (m == NULL) {
				(void) tcptli_error(q, mp,
					type, TSYSERR, ENOBUFS);
				return;
			}
			bcopy((caddr_t)&opt.tts_sendsize,
				mtod(m, caddr_t), sizeof (int));
			m->m_len = sizeof (int);
			error = sosetopt(so, SOL_SOCKET, SO_SNDBUF, m);
			if (error < 0) {
				(void) tcptli_error(q, mp,
					type, TSYSERR, error);
				return;
			}
		}
		if (opt.tts_recvsize > 0) {
			m = m_get(M_WAIT, MT_SOOPTS);
			if (m == NULL) {
				(void) tcptli_error(q, mp,
					type, TSYSERR, ENOBUFS);
				return;
			}
			bcopy((caddr_t)&opt.tts_recvsize,
				mtod(m, caddr_t), sizeof (int));
			m->m_len = sizeof (int);
			error = sosetopt(so, SOL_SOCKET, SO_RCVBUF, m);
			if (error < 0) {
				(void) tcptli_error(q, mp, type, TSYSERR,
					error);
				return;
			}
		}

		mp->b_datap->db_type = M_PCPROTO;
		rap = (struct T_optmgmt_ack *)mp->b_rptr;
		rap->PRIM_type = T_OPTMGMT_ACK;
		rap->MGMT_flags = T_NEGOTIATE;
		rap->OPT_length = sizeof (struct tt_soopt);
		rap->OPT_offset = sizeof (struct T_optmgmt_ack);
		optp = (struct tt_soopt *)(mp->b_rptr
			+ sizeof (struct T_optmgmt_ack));
		optp->tts_reuseaddr = opt.tts_reuseaddr;
		optp->tts_keepalive = opt.tts_keepalive;
		optp->tts_sendsize = so->so_snd.sb_hiwat;
		optp->tts_recvsize = so->so_rcv.sb_hiwat;

		qreply(q, mp);
		return;
	}
	default:
		(void) tcptli_error(q, mp, type, TBADFLAG, 0);
		return;
	}
}
tcptli_t_ordrel_req(q, mp, ttp)
	queue_t	*q;
	mblk_t	*mp;
	struct	tt_softc *ttp;
{

	TCPTLI_PRINTF("tcptli-%d: ttp: %x tt_so: %x T_ORDREL_REQ\n",
		ttp - tt_softc, ttp, ttp->tt_so);

	/*
	 * Verify that the request is allowable.  Since this
	 * request has no corresponding acknowledgement, errors
	 * can only be reported by putting the stream into
	 * M_ERROR mode.
	 */
	if (!(ttp->tt_state & (TL_DATAXFER | TL_INREL))) {
		/*
		 * Issue an M_ERROR up and mark this ttp unusable
		 */
		(void) tcptli_error(q, (mblk_t *)NULL, (long)0, 0, EPROTO);
		ttp->tt_flags |= TT_ERROR;
		return;
	}

	/*
	 * Shut down our half of the socket.  The tli spec makes
	 * no provision for handling errors that arise here.
	 */
	(void) soshutdown(ttp->tt_so, FWRITE);

	/*
	 * Free the msg that get passed down to us
	 */
	if (mp)
		freemsg(mp);
	/*
	 * Perform tli automaton state transition.
	 */
	ttp->tt_state = ttp->tt_state == TL_DATAXFER ? TL_OUTREL : TL_IDLE;

	return;
}


/*
 * Conversion between the state value used by TCPTLI in the
 * kernel (i.e. TL_XXX, defined in nettli/tcp_tli.h) to the
 * set of values used in the TLI library (i.e. TS_XXX, defined
 * in nettli/tihdr.h).
 */
tcptli_stateconv(ts)
	u_int	ts;
{
	switch (ts) {

	case TL_UNBND:
		return (TS_UNBND);

	case TL_IDLE:
		return (TS_IDLE);

	case TL_DATAXFER:
		return (TS_DATA_XFER);

	case TL_OUTCON:
		return (TS_WCON_CREQ);

	case TL_INCON:
		return (TS_WRES_CIND);

	case TL_OUTREL:
		return (TS_WREQ_ORDREL);

	case TL_INREL:
		return (TS_WIND_ORDREL);

	case TL_UNINIT:
	case TL_ERROR:
	default:
		return (TS_NOSTATES);
	}
}

/*
 * Common code for processing M_DATA and T_DATA_REQ
 * with recovery.
 * There may be 3 kinds of errors:
 *
 *	EWOULDBLOCK	from sosend(). We just put the msg back on
 *			the queue and try again later.
 *
 *	TT_INCOMPLETESEND
 *			in this case, we walk thru the mblk chain
 *			and figure out what part of the data got
 *			sent. We still use the mblk structure, but
 *			just tells this mblk contains zero data, i.e.
 *			set the b_wptr with the b_rptr.
 *
 *	other error	Just free the msg and issue an error up
 *
 * This routine returns error to the call, as an indication
 * whether some error processing has been done here already.
 */
tcptli_senddata(ttp, mp, flags)
	struct	tt_softc	*ttp;
	mblk_t			*mp;
	int	flags;			/* normal or expedited */
{
	int	error = 0;
	int	alreadysent;
	mblk_t		*mrp;
	queue_t		*q = ttp->tt_rq;

	mrp = allocb(sizeof (struct T_error_ack), BPRI_HI);
	if (!mrp) {
		(void) tcptli_error(ttp->tt_rq, (mblk_t *)NULL,
			(long)M_DATA, TSYSERR, ENOBUFS);
		return (0);
	}
	if (error = tcptli_send(ttp, mp, flags, &alreadysent)) {
		if (error == EWOULDBLOCK) {
			/*  error got from sosend */
			tcptli_schedwakeup(ttp, 20);
			putbq(OTHERQ(q), mp);
			freeb(mrp);
			return (error);
		} else if (error == TT_INCOMPLETESEND) {
			/*
			 * Partial data got sent, walk thru
			 * the mblk chain and throw away
			 * those that got sent, via
			 * setting b_wptr = b_rptr;
			 */
			register int	mblklen;
			register int	offset = alreadysent;
			register mblk_t	*mbp;

			for (mbp = mp; mbp; mbp = mbp->b_cont) {
				mblklen = mbp->b_wptr - mbp->b_rptr;
				if (offset > mblklen) {
					mbp->b_rptr = mbp->b_wptr;
					offset -= mblklen;
				} else {
					mbp->b_rptr += offset;
					break;
				}
			}
			if (!mbp)
				/*
				 * Issue a fatal msg M_ERROR up
				 */
				(void) tcptli_error(q, (mblk_t *)NULL, (long)0,
					0, 0);
			tcptli_schedwakeup(ttp, 20);
			putbq(OTHERQ(q), mp);
			freeb(mrp);
			return (error);
		} else {
			/* some unknown err, quit */
			mrp->b_datap->db_type = M_ERROR;
			*(mrp->b_wptr) = (char)EPROTO;
			mrp->b_wptr++;
			qreply(q, mrp);
			freemsg(mp);
			return (error);
		}
	} else {
		/* normal case */
		freemsg(mp);
		freeb(mrp);
		return (0);
	}
	/* NOTREACHED */
}

#endif	NTCPTLI > 0
