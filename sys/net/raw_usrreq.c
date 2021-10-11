/*
 * Copyright (c) 1980, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)raw_usrreq.c 1.1 92/07/30 SMI; from UCB 7.1
 */

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>

#include <net/if.h>
#include <net/route.h>
#include <net/netisr.h>
#include <net/raw_cb.h>

#ifdef vax
#include <vax/mtpr.h>
#endif

/*
 * Initialize raw connection block q.
 */
raw_init()
{

	rawcb.rcb_next = rawcb.rcb_prev = &rawcb;
	rawintrq.ifq_maxlen = IFQ_MAXLEN;
}

/*
 * Raw protocol interface.
 */
raw_input(m0, proto, src, dst)
	struct mbuf *m0;
	struct sockproto *proto;
	struct sockaddr *src, *dst;
{
	register struct mbuf *m;
	struct raw_header *rh;
	int s;

	/*
	 * Rip off an mbuf for a generic header.
	 */
	m = m_get(M_DONTWAIT, MT_HEADER);
	if (m == 0) {
		m_freem(m0);
		return;
	}
	m->m_next = m0;
	m->m_len = sizeof(struct raw_header);
	rh = mtod(m, struct raw_header *);
	rh->raw_dst = *dst;
	rh->raw_src = *src;
	rh->raw_proto = *proto;

	/*
	 * Header now contains enough info to decide
	 * which socket to place packet in (if any).
	 * Queue it up for the raw protocol process
	 * running at software interrupt level.
	 */
	s = splimp();
	if (IF_QFULL(&rawintrq)) {
		IF_DROP(&rawintrq);
		m_freem(m);
	} else
		IF_ENQUEUE(&rawintrq, m);
	(void) splx(s);
	schednetisr(NETISR_RAW);
}

/*
 * Raw protocol input routine.  Process packets entered
 * into the queue at interrupt time.  Find the socket
 * associated with the packet(s) and move them over.  If
 * nothing exists for this packet, drop it.  Depending on the
 * requirements of the protocol associated with the receiving
 * socket, we use one of sbappend{,addr,record} to accomplish
 * the transfer.  The convoluted replicated code is to avoid
 * unnecessary m_copies.
 */
rawintr()
{
	int s;
	struct mbuf *m;
	register struct rawcb *rp;
	register struct raw_header *rh;
	struct socket *lastso;
	struct rawcb *lastrcb;
	register short prflags;

next:
	s = splimp();
	IF_DEQUEUE(&rawintrq, m);
	(void) splx(s);
	if (m == 0)
		return;
	rh = mtod(m, struct raw_header *);
	lastso = 0;
#ifdef lint
	lastrcb = 0;
#endif lint
	for (rp = rawcb.rcb_next; rp != &rawcb; rp = rp->rcb_next) {
		if (rp->rcb_proto.sp_family != rh->raw_proto.sp_family)
			continue;
		if (rp->rcb_proto.sp_protocol  &&
		    rp->rcb_proto.sp_protocol != rh->raw_proto.sp_protocol)
			continue;
		/*
		 * We assume the lower level routines have
		 * placed the address in a canonical format
		 * suitable for a structure comparison.
		 */
#define equal(a1, a2) \
	(bcmp((caddr_t)&(a1), (caddr_t)&(a2), sizeof (struct sockaddr)) == 0)
		if ((rp->rcb_flags & RAW_LADDR) &&
		    !equal(rp->rcb_laddr, rh->raw_dst))
			continue;
		if ((rp->rcb_flags & RAW_FADDR) &&
		    !equal(rp->rcb_faddr, rh->raw_src))
			continue;
		if (lastso) {
			/*
			 * Must fan this packet out to a second (or later)
			 * recipient.  Do special processing for the
			 * previous recipient and produce a copy for
			 * the current recipient.
			 */
			register struct mbuf *n;
			register int result;

			/* Copy body only -- header only used locally. */
			if ((n = m_copy(m->m_next, 0, (int)M_COPYALL)) == 0)
				goto nospace;
			if (lastrcb->rcb_flags & RAW_TALLY) {
				s = splimp();
				m_tally(n, -1,
				    &lastrcb->rcb_cc, &lastrcb->rcb_mbcnt);
				(void) splx(s);
			}
			if ((prflags = lastso->so_proto->pr_flags) & PR_ADDR) {
				result = sbappendaddr(&lastso->so_rcv,
				    &rh->raw_src, n, (struct mbuf *)0);
			} else if (prflags & PR_ATOMIC) {
				result = 1;
				(void) sbappendrecord(&lastso->so_rcv, n);
			} else {
				result = 1;
				(void) sbappend(&lastso->so_rcv, n);
			}
			if (result == 0) {
				/* should notify about lost packet */
				m_freem(n);
				goto nospace;
			}
			sorwakeup(lastso);
		}
nospace:
		lastso = rp->rcb_socket;
		lastrcb = rp;
	}
	if (lastso) {
		register int result;

		if (lastrcb->rcb_flags & RAW_TALLY) {
			s = splimp();
			m_tally(m->m_next, -1,
			    &lastrcb->rcb_cc, &lastrcb->rcb_mbcnt);
			(void) splx(s);
		}
		if ((prflags = lastso->so_proto->pr_flags) & PR_ADDR) {
			result = sbappendaddr(&lastso->so_rcv,
			    &rh->raw_src, m->m_next, (struct mbuf *)0);
		} else if (prflags & PR_ATOMIC) {
			result = 1;
			(void) sbappendrecord(&lastso->so_rcv, m->m_next);
		} else {
			result = 1;
			(void) sbappend(&lastso->so_rcv, m->m_next);
		}
		if (result == 0)
			goto drop;
		m = m_free(m);		/* header */
		sorwakeup(lastso);
		goto next;
	}
drop:
	m_freem(m);
	goto next;
}

/*ARGSUSED*/
raw_ctlinput(cmd, arg)
	int cmd;
	struct sockaddr *arg;
{

	if (cmd < 0 || cmd > PRC_NCMDS)
		return;
	/* INCOMPLETE */
}

/*ARGSUSED*/
raw_usrreq(so, req, m, nam, rights)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *rights;
{
	register struct rawcb *rp = sotorawcb(so);
	register int error = 0;

	if (req == PRU_CONTROL)
		return (EOPNOTSUPP);
	if (rights && rights->m_len) {
		error = EOPNOTSUPP;
		goto release;
	}
	if (rp == 0 && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}
	switch (req) {

	/*
	 * Allocate a raw control block and fill in the
	 * necessary info to allow packets to be routed to
	 * the appropriate raw interface routine.
	 */
	case PRU_ATTACH:
		if ((so->so_state & SS_PRIV) == 0) {
			error = EACCES;
			break;
		}
		if (rp) {
			error = EINVAL;
			break;
		}
		error = raw_attach(so, (int)nam);
		break;

	/*
	 * Destroy state just before socket deallocation.
	 * Flush data or not depending on the options.
	 */
	case PRU_DETACH:
		if (rp == 0) {
			error = ENOTCONN;
			break;
		}
		raw_detach(rp);
		break;

	/*
	 * If a socket isn't bound to a single address,
	 * the raw input routine will hand it anything
	 * within that protocol family (assuming there's
	 * nothing else around it should go to). 
	 */
	case PRU_CONNECT:
		if (rp->rcb_flags & RAW_FADDR) {
			error = EISCONN;
			break;
		}
		raw_connaddr(rp, nam);
		soisconnected(so);
		break;

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		goto release;

	case PRU_BIND:
		if (rp->rcb_flags & RAW_LADDR) {
			error = EINVAL;			/* XXX */
			break;
		}
		error = raw_bind(so, nam);
		break;

	case PRU_DISCONNECT:
		if ((rp->rcb_flags & RAW_FADDR) == 0) {
			error = ENOTCONN;
			break;
		}
		raw_disconnect(rp);
		soisdisconnected(so);
		break;

	/*
	 * Mark the connection as being incapable of further input.
	 */
	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	/*
	 * Ship a packet out.  The appropriate raw output
	 * routine handles any massaging necessary.
	 */
	case PRU_SEND:
		if (nam) {
			if (rp->rcb_flags & RAW_FADDR) {
				error = EISCONN;
				break;
			}
			raw_connaddr(rp, nam);
			/*
			 * clear RAW_FADDR to enable raw receive
			 * semantics by avoiding race between
			 * rawintr inception and pr_output completion
			 */
			rp->rcb_flags &= ~RAW_FADDR;
		} else if ((rp->rcb_flags & RAW_FADDR) == 0) {
			error = ENOTCONN;
			break;
		}
		error = (*so->so_proto->pr_output)(m, so);
		m = NULL;
		break;

	case PRU_ABORT:
		raw_disconnect(rp);
		sofree(so);
		soisdisconnected(so);
		break;

	case PRU_SENSE:
		/*
		 * stat: don't bother with a blocksize.
		 */
		return (0);

	/*
	 * Not supported.
	 */
	case PRU_RCVOOB:
	case PRU_RCVD:
		return(EOPNOTSUPP);

	case PRU_LISTEN:
	case PRU_ACCEPT:
	case PRU_SENDOOB:
		error = EOPNOTSUPP;
		break;

	case PRU_SOCKADDR:
		bcopy((caddr_t)&rp->rcb_laddr, mtod(nam, caddr_t),
		    sizeof (struct sockaddr));
		nam->m_len = sizeof (struct sockaddr);
		break;

	case PRU_PEERADDR:
		bcopy((caddr_t)&rp->rcb_faddr, mtod(nam, caddr_t),
		    sizeof (struct sockaddr));
		nam->m_len = sizeof (struct sockaddr);
		break;

	default:
		panic("raw_usrreq");
	}
release:
	if (m != NULL)
		m_freem(m);
	return (error);
}
