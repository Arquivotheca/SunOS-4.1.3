/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)uipc_socket.c 1.1 92/07/30 SMI; from UCB 7.8 1/20/88
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/mbuf.h>
#include <sys/un.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/route.h>
#include <netinet/in.h>
#include <net/if.h>

/*
 * Socket operation routines.
 * These routines are called by the routines in
 * sys_socket.c or from a system process, and
 * implement the semantics of socket operations by
 * switching out to the protocol specific routines.
 *
 * TODO:
 *	test socketpair
 *	clean up async
 *	out-of-band is a kludge
 */
/*ARGSUSED*/
socreate(dom, aso, type, proto)
	struct socket **aso;
	register int type;
	int proto;
{
	register struct protosw *prp;
	register struct socket *so;
	register struct mbuf *m;
	register int error;

	if (proto)
		prp = pffindproto(dom, proto, type);
	else
		prp = pffindtype(dom, type);
	if (prp == 0)
		return (EPROTONOSUPPORT);
	if (prp->pr_type != type)
		return (EPROTOTYPE);
	m = m_getclr(M_WAIT, MT_SOCKET);
	so = mtod(m, struct socket *);
	so->so_options = 0;
	so->so_state = 0;
	so->so_type = type;
	if (u.u_uid == 0)
		so->so_state = SS_PRIV;
	so->so_proto = prp;
	error =
	    (*prp->pr_usrreq)(so, PRU_ATTACH,
		(struct mbuf *)0, (struct mbuf *)proto, (struct mbuf *)0);
	if (error) {
		so->so_state |= SS_NOFDREF;
		sofree(so);
		return (error);
	}
	*aso = so;
	return (0);
}

sobind(so, nam)
	struct socket *so;
	struct mbuf *nam;
{
	int s = splnet();
	int error;

	error =
	    (*so->so_proto->pr_usrreq)(so, PRU_BIND,
		(struct mbuf *)0, nam, (struct mbuf *)0);
	(void) splx(s);
	return (error);
}

solisten(so, backlog)
	register struct socket *so;
	int backlog;
{
	int s = splnet(), error;

	error =
	    (*so->so_proto->pr_usrreq)(so, PRU_LISTEN,
		(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
	if (error) {
		(void) splx(s);
		return (error);
	}
	if (so->so_q == 0) {
		so->so_q = so;
		so->so_q0 = so;
		so->so_options |= SO_ACCEPTCONN;
	}
	if (backlog < 0)
		backlog = 0;
	so->so_qlimit = MIN(backlog, SOMAXCONN);
	(void) splx(s);
	return (0);
}

sofree(so)
	register struct socket *so;
{

	if (so->so_pcb || (so->so_state & SS_NOFDREF) == 0)
		return;
	if (so->so_head) {
		if (!soqremque(so, 0) && !soqremque(so, 1))
			panic("sofree dq");
		so->so_head = 0;
	}

	/*
	 * call the special socket wakeup function (if set)
	 * to notify the user that the socket is being freed and
	 * to allow him to free any associated resources.
	 * XXX -- 0 in 2nd arg signifies to the callee that he
	 * should free resources at this point.
	 */
	if (so->so_wupalt)
		(*so->so_wupalt->wup_func)(so, (caddr_t)0,
			so->so_wupalt->wup_arg);

	sbrelease(&so->so_snd);
	sorflush(so);
	(void) m_free(dtom(so));
}

/*
 * Close a socket on last file table reference removal.
 * Initiate disconnect if connected.
 * Free socket when disconnect complete.
 */
soclose(so)
	register struct socket *so;
{
	int s = splnet();		/* conservative */
	int error = 0;

	if (so->so_options & SO_ACCEPTCONN) {
		while (so->so_q0 != so)
			(void) soabort(so->so_q0);
		while (so->so_q != so)
			(void) soabort(so->so_q);
	}
	if (so->so_pcb == 0)
		goto discard;
	if (so->so_state & SS_ISCONNECTED) {
		if ((so->so_state & SS_ISDISCONNECTING) == 0) {
			error = sodisconnect(so);
			if (error)
				goto drop;
		}
		if (so->so_options & SO_LINGER) {
			if ((so->so_state & SS_ISDISCONNECTING) &&
			    (so->so_state & SS_NBIO))
				goto drop;
			while (so->so_state & SS_ISCONNECTED)
				(void) sleep((caddr_t)&so->so_timeo, PZERO+1);
		}
	}
drop:
	if (so->so_pcb) {
		int error2 =
		    (*so->so_proto->pr_usrreq)(so, PRU_DETACH,
			(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
		if (error == 0)
			error = error2;
	}
discard:
	if (so->so_state & SS_NOFDREF)
		panic("soclose: NOFDREF");
	so->so_state |= SS_NOFDREF;
	sofree(so);
	(void) splx(s);
	return (error);
}

/*
 * Must be called at splnet...
 */
soabort(so)
	struct socket *so;
{

	return (
	    (*so->so_proto->pr_usrreq)(so, PRU_ABORT,
		(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0));
}

soaccept(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	int s = splnet();
	int error;

	if ((so->so_state & SS_NOFDREF) == 0)
		panic("soaccept: !NOFDREF");
	so->so_state &= ~SS_NOFDREF;
	error = (*so->so_proto->pr_usrreq)(so, PRU_ACCEPT,
	    (struct mbuf *)0, nam, (struct mbuf *)0);
	(void) splx(s);
	return (error);
}

soconnect(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	int s;
	int error;

	if (so->so_options & SO_ACCEPTCONN)
		return (EOPNOTSUPP);
	s = splnet();
	/*
	 * If protocol is connection-based, can only connect once.
	 * Otherwise, if connected, try to disconnect first.
	 * This allows user to disconnect by connecting to, e.g.,
	 * a null address.
	 */
	if (so->so_state & (SS_ISCONNECTED|SS_ISCONNECTING) &&
	    ((so->so_proto->pr_flags & PR_CONNREQUIRED) ||
	    (error = sodisconnect(so))))
		error = EISCONN;
	else
		error = (*so->so_proto->pr_usrreq)(so, PRU_CONNECT,
		    (struct mbuf *)0, nam, (struct mbuf *)0);
	(void) splx(s);
	return (error);
}

soconnect2(so1, so2)
	register struct socket *so1;
	struct socket *so2;
{
	int s = splnet();
	int error;

	error = (*so1->so_proto->pr_usrreq)(so1, PRU_CONNECT2,
	    (struct mbuf *)0, (struct mbuf *)so2, (struct mbuf *)0);
	(void) splx(s);
	return (error);
}

sodisconnect(so)
	register struct socket *so;
{
	int s = splnet();
	int error;

	if ((so->so_state & SS_ISCONNECTED) == 0) {
		error = ENOTCONN;
		goto bad;
	}
	if (so->so_state & SS_ISDISCONNECTING) {
		error = EALREADY;
		goto bad;
	}
	error = (*so->so_proto->pr_usrreq)(so, PRU_DISCONNECT,
	    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
bad:
	(void) splx(s);
	return (error);
}

#define	SEND_CHUNK	(MCLBYTES * 4)	/* Limit before protocol call */
#define	SEND_THRESH	(MCLBYTES/2)	/* Threshold to use a cluster */

/*
 * Send on a socket.
 * If send must go all at once and message is larger than
 * send buffering, then hard error.
 * Lock against other senders.
 * If must go all at once and not enough room now, then
 * inform user that this would block and do nothing.
 * Otherwise, if nonblocking, send as much as possible.
 */
sosend(so, nam, uio, flags, rights)
	register struct socket *so;
	struct mbuf *nam;
	register struct uio *uio;
	int flags;
	struct mbuf *rights;
{
	struct mbuf *top = 0;
	register struct mbuf *m, **mp;
	register int space;
	int len, rlen = 0, error = 0, s, dontroute, first = 1;

	if (sosendallatonce(so) && uio->uio_resid > so->so_snd.sb_hiwat)
		return (EMSGSIZE);
	dontroute =
	    (flags & MSG_DONTROUTE) && (so->so_options & SO_DONTROUTE) == 0 &&
	    (so->so_proto->pr_flags & PR_ATOMIC);
	u.u_ru.ru_msgsnd++;
	if (rights)
		rlen = rights->m_len;
#define	snderr(errno)	{ error = errno; (void) splx(s); goto release; }

restart:
	sblock(so, &so->so_snd);
	do {
		s = splnet();
		if (so->so_state & SS_CANTSENDMORE)
			snderr(EPIPE);
		if (so->so_error) {
			error = so->so_error;
			so->so_error = 0;			/* ??? */
			(void) splx(s);
			goto release;
		}
		if ((so->so_state & SS_ISCONNECTED) == 0) {
			if (so->so_proto->pr_flags & PR_CONNREQUIRED)
				snderr(ENOTCONN);
			if (nam == 0)
				snderr(EDESTADDRREQ);
		}
		if (flags & MSG_OOB)
			space = 1024;
		else {
			register int	resid = uio->uio_resid + rlen;
			int		ispipe = (so->so_state & SS_PIPE);

			space = sbspace(&so->so_snd);
			/*
			 * POSIX/SVID behavior on pipes
			 * if (NDELAY && nbytes <= PIPE_BUF && space < nbytes)
			 *	return POSIX ? EAGAIN : 0
			 */
			if ((uio->uio_fmode & (FNONBIO | FNBIO)) &&
			    ispipe && resid <= PIPE_BUF && space < resid) {
				if (uio->uio_fmode & FNONBIO && first)
					error = EAGAIN;
				(void) splx(s);
				goto release;
			}
			/*
			 * More POSIX: writes to a pipe of <= PIPE_BUF must
			 * be atomic; the third disjunct of the if below
			 * enforces this.  Moreover, for requests larger than
			 * PIPE_BUF, if the pipe is empty at the outset, we
			 * must transfer at least PIPE_BUF bytes; the fourth
			 * disjunct enforces this condition.
			 */
			if (space <= rlen ||
			    (sosendallatonce(so) && space < resid) ||
			    (ispipe && resid <= PIPE_BUF && space < resid) ||
			    (ispipe && resid > PIPE_BUF && space < PIPE_BUF &&
				so->so_snd.sb_cc == 0) ||
			    (uio->uio_resid >= MCLBYTES && space < MCLBYTES &&
				so->so_snd.sb_cc >= MCLBYTES &&
				(so->so_state & SS_NBIO) == 0 &&
				(uio->uio_fmode & (FNBIO | FNONBIO)) == 0)
			    ) {
				/*
				 * Don't have enough space available to send
				 * everthing that must go out as a single
				 * unit.
				 */
				if (uio->uio_fmode & FNONBIO) {
					/* POSIX semantics apply. */
					if (first)
						error = EAGAIN;
					(void) splx(s);
					goto release;
				} else if (so->so_state & SS_NBIO) {
					/* 4bsd semantics apply. */
					if (first)
						error = EWOULDBLOCK;
					(void) splx(s);
					goto release;
				} else if (uio->uio_fmode & FNBIO) {
					/* SVID semantics apply. */
					(void) splx(s);
					goto release;
				}
				sbunlock(so, &so->so_snd);
				sbwait(&so->so_snd);
				(void) splx(s);
				goto restart;
			}
		}
		(void) splx(s);
		mp = &top;
		space -= rlen;
		if ((so->so_proto->pr_flags & PR_ATOMIC) == 0) {
			/*
			 * Limit size done in one send to a couple clusters
			 * in order to get better parallelism
			 */
			space = MIN(space, SEND_CHUNK);
		}

		while (space > 0) {
			/*
			 * Process another mbuf's worth of outgoing
			 * data.  Note that a zero-length write will
			 * turn into a zero-length mbuf.
			 */
			MGET(m, M_WAIT, MT_DATA);
			/*
			 * See whether to use a cluster mbuf.  The first
			 * clause checks that there's enough data to make
			 * it worthwhile (MCLBYTES/<small power of 2> might
			 * be better) and the second checks whether doing
			 * so would exceed the amount of allowable buffer
			 * space.
			 */
			if (uio->uio_resid >= SEND_THRESH &&
			    space >= SEND_THRESH) {
				MCLGET(m);
				if (m->m_len != MCLBYTES)
					goto nomclusters;
				len = MIN(MCLBYTES, uio->uio_resid);
				space -= MCLBYTES;
			} else {
nomclusters:
				len = MIN(MIN(MLEN, uio->uio_resid), space);
				space -= len;
			}
			error = uiomove(mtod(m, caddr_t), len, UIO_WRITE, uio);
			m->m_len = len;
			*mp = m;
			if (error)
				goto release;
			mp = &m->m_next;
			if (uio->uio_resid <= 0)
				break;
		}
		if (dontroute)
			so->so_options |= SO_DONTROUTE;
		s = splnet();					/* XXX */
		error = (*so->so_proto->pr_usrreq)(so,
		    (flags & MSG_OOB) ? PRU_SENDOOB : PRU_SEND,
		    top, (caddr_t)nam, rights);
		(void) splx(s);
		if (dontroute)
			so->so_options &= ~SO_DONTROUTE;
		rights = 0;
		rlen = 0;
		top = 0;
		first = 0;
		if (error)
			break;
	} while (uio->uio_resid);

release:
	sbunlock(so, &so->so_snd);
	if (top)
		m_freem(top);
	if (error == EPIPE)
		psignal(u.u_procp, SIGPIPE);
	return (error);
}

/*
 * Implement receive operations on a socket.
 * We depend on the way that records are added to the sockbuf
 * by sbappend*.  In particular, each record (mbufs linked through m_next)
 * must begin with an address if the protocol so specifies,
 * followed by an optional mbuf containing access rights if supported
 * by the protocol, and then zero or more mbufs of data.
 * In order to avoid blocking network interrupts for the entire time here,
 * we splx() while doing the actual copy to user space.
 * Although the sockbuf is locked, new data may still be appended,
 * and thus we must maintain consistency of the sockbuf during that time.
 */
soreceive(so, aname, uio, flags, rightsp)
	register struct socket *so;
	struct mbuf **aname;
	register struct uio *uio;
	int flags;
	struct mbuf **rightsp;
{
	register struct mbuf *m;
	register int len, error = 0, s, offset;
	struct protosw *pr = so->so_proto;
	struct mbuf *nextrecord;
	int moff;

	if (rightsp)
		*rightsp = 0;
	if (aname)
		*aname = 0;
	if (flags & MSG_OOB) {
		m = m_get(M_WAIT, MT_DATA);
		error = (*pr->pr_usrreq)(so, PRU_RCVOOB,
		    m, (struct mbuf *)(flags & MSG_PEEK), (struct mbuf *)0);
		if (error)
			goto bad;
		if (pr->pr_flags & PR_OOB_ADDR) {
			*aname = m;
			m = m->m_next;
			(*aname)->m_next = (struct mbuf *)0;
			if (!m)
				return (error);
		}
		do {
			len = uio->uio_resid;
			if (len > m->m_len)
				len = m->m_len;
			error = uiomove(mtod(m, caddr_t), (int)len, UIO_READ,
					uio);
			m = m_free(m);
		} while (uio->uio_resid && error == 0 && m);
bad:
		if (m)
			m_freem(m);
		return (error);
	}

restart:
	sblock(so, &so->so_rcv);
	s = splnet();

	if (so->so_rcv.sb_cc == 0) {
		if (so->so_error) {
			error = so->so_error;
			so->so_error = 0;
			goto release;
		}
		if (so->so_state & SS_CANTRCVMORE)
			goto release;
		if ((so->so_state & SS_ISCONNECTED) == 0 &&
		    (so->so_proto->pr_flags & PR_CONNREQUIRED)) {
			error = ENOTCONN;
			goto release;
		}
		if (uio->uio_resid == 0)
			goto release;
		if (uio->uio_fmode & FNONBIO) {
			error = EAGAIN;
			goto release;
		}
		if (so->so_state & SS_NBIO) {
			error = EWOULDBLOCK;
			goto release;
		}
		if (uio->uio_fmode & FNBIO)
			goto release;
		sbunlock(so, &so->so_rcv);
		sbwait(&so->so_rcv);
		(void) splx(s);
		goto restart;
	}
	u.u_ru.ru_msgrcv++;
	m = so->so_rcv.sb_mb;
	if (m == 0)
		panic("receive 1");
	nextrecord = m->m_act;
	if (pr->pr_flags & PR_ADDR) {
		register struct mbuf *n;

		if (m->m_type != MT_SONAME)
			panic("receive 1a");
		if (flags & MSG_PEEK) {
			/*
			 * Advance past address subchain,
			 * copying it into *aname if needed.
			 */
			register int alen = 0;

			for (n = m; n && n->m_type == MT_SONAME; n = n->m_next)
				alen += n->m_len;
			if (aname)
				*aname = m_copy(m, 0, alen);
			m = n;
		} else {
			/*
			 * Dispose of address subchain, possibly
			 * by transferring it over to *aname.
			 */
			for ( ; m && m->m_type == MT_SONAME;
			    n = m, m = m->m_next)
				sbfree(&so->so_rcv, m);
			/* Break it off. */
			n->m_next = 0;
			if (aname)
				*aname = so->so_rcv.sb_mb;
			else
				m_freem(so->so_rcv.sb_mb);
			so->so_rcv.sb_mb = m;
			if (m)
				m->m_act = nextrecord;
		}
	}
	if (m && m->m_type == MT_RIGHTS) {
		register struct mbuf *n;

		if ((pr->pr_flags & PR_RIGHTS) == 0)
			panic("receive 2");
		if (flags & MSG_PEEK) {
			/*
			 * Advance past rights subchain,
			 * copying it into *rightsp if needed.
			 */
			register int rlen = 0;

			for (n = m; n && n->m_type == MT_RIGHTS; n = n->m_next)
				rlen += n->m_len;
			if (rightsp)
				*rightsp = m_copy(m, 0, rlen);
			m = n;
		} else {
			/*
			 * Dispose of rights subchain, possibly
			 * by transferring it over to *rightsp.
			 */
			for ( ; m && m->m_type == MT_RIGHTS;
			    n = m, m = m->m_next)
				sbfree(&so->so_rcv, m);
			/* Break it off. */
			n->m_next = 0;
			if (rightsp)
				*rightsp = so->so_rcv.sb_mb;
			else
				m_freem(so->so_rcv.sb_mb);
			so->so_rcv.sb_mb = m;
			if (m)
				m->m_act = nextrecord;
		}
	}
	moff = 0;
	offset = 0;
	while (m && uio->uio_resid > 0 && error == 0) {
		if (m->m_type != MT_DATA && m->m_type != MT_HEADER)
			panic("receive 3");
		len = uio->uio_resid;
		so->so_state &= ~SS_RCVATMARK;
		if (so->so_oobmark && len > so->so_oobmark - offset)
			len = so->so_oobmark - offset;
		if (len > m->m_len - moff)
			len = m->m_len - moff;
		(void) splx(s);
		if (uiomove(mtod(m, caddr_t) + moff, (int)len, UIO_READ, uio))
			error = EFAULT;
		s = splnet();
		if (len == m->m_len - moff) {
			/* Have consumed all of current mbuf. */
			if (flags & MSG_PEEK) {
				m = m->m_next;
				moff = 0;
			} else {
				nextrecord = m->m_act;
				sbfree(&so->so_rcv, m);
				MFREE(m, so->so_rcv.sb_mb);
				m = so->so_rcv.sb_mb;
				if (m)
					m->m_act = nextrecord;
			}
		} else {
			if (flags & MSG_PEEK)
				moff += len;
			else {
				m->m_off += len;
				m->m_len -= len;
				so->so_rcv.sb_cc -= len;
			}
		}
		if (so->so_oobmark) {
			if ((flags & MSG_PEEK) == 0) {
				so->so_oobmark -= len;
				if (so->so_oobmark == 0) {
					so->so_state |= SS_RCVATMARK;
					break;
				}
			} else
				offset += len;
		}
	}
	if ((flags & MSG_PEEK) == 0) {
		if (m == 0)
			so->so_rcv.sb_mb = nextrecord;
		else if (pr->pr_flags & PR_ATOMIC)
			(void) sbdroprecord(&so->so_rcv);
		if (pr->pr_flags & PR_WANTRCVD && so->so_pcb)
			(*pr->pr_usrreq)(so, PRU_RCVD, (struct mbuf *)0,
			    (struct mbuf *)0, (struct mbuf *)0);
		if (error == 0 && rightsp && *rightsp &&
		    pr->pr_domain->dom_externalize)
			error = (*pr->pr_domain->dom_externalize)(*rightsp);
	}
release:
	sbunlock(so, &so->so_rcv);
	(void) splx(s);
	return (error);
}

soshutdown(so, how)
	register struct socket *so;
	register int how;
{
	register struct protosw *pr = so->so_proto;

	how++;
	if (how & FREAD)
		sorflush(so);
	if (how & FWRITE)
		return ((*pr->pr_usrreq)(so, PRU_SHUTDOWN,
		    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0));
	return (0);
}

sorflush(so)
	register struct socket *so;
{
	register struct sockbuf *sb = &so->so_rcv;
	register struct protosw *pr = so->so_proto;
	register int s;
	struct sockbuf asb;

	sblock(so, sb);
	s = splimp();
	socantrcvmore(so);
	sbunlock(so, sb);
	asb = *sb;
	bzero((caddr_t)sb, sizeof (*sb));
	(void) splx(s);
	if (pr->pr_flags & PR_RIGHTS && pr->pr_domain->dom_dispose)
		(*pr->pr_domain->dom_dispose)(asb.sb_mb);
	sbrelease(&asb);
}

sosetopt(so, level, optname, m0)
	register struct socket *so;
	int level, optname;
	struct mbuf *m0;
{
	int error = 0;
	register struct mbuf *m = m0;

	if (level != SOL_SOCKET) {
		if (so->so_proto && so->so_proto->pr_ctloutput)
			return ((*so->so_proto->pr_ctloutput)
				(PRCO_SETOPT, so, level, optname, &m0));
		error = ENOPROTOOPT;
	} else {
		switch (optname) {

#ifndef notyet
		/*
		 * 4.2 compatibility code.
		 *	THIS SHOULD GO AWAY WHEN WE NO LONGER REQUIRE
		 *	BINARY COMPATIBILITY WITH RELEASE 3.0.
		 */
#ifndef	SO_DONTLINGER
#define	SO_DONTLINGER	(~SO_LINGER)
#endif	SO_DONTLINGER
		case SO_DONTLINGER:
			so->so_options &= ~SO_LINGER;
			so->so_linger = 0;
			break;
#endif notyet

		case SO_LINGER:
#ifndef notyet
			/*
			 * 4.2 compatibility code.
			 *	THIS SHOULD GO AWAY WHEN WE NO LONGER REQUIRE
			 *	BINARY COMPATIBILITY WITH RELEASE 3.0.
			 *
			 * Convert 4.2 option representation to the 4.3
			 * representation.  The code counts on the fact
			 * that sizeof (struct linger) != sizeof (int).
			 */
			if (m && m->m_len == sizeof (int)) {
				register int ling = *mtod(m, int *);
				register struct linger *l;

				(void) m_free(m);
				MGET(m, M_WAIT, MT_SOOPTS);
				m->m_len = sizeof (struct linger);
				l = mtod(m, struct linger *);
				l->l_linger = ling;
				l->l_onoff = 1;
			}
#endif notyet
			if (m == NULL || m->m_len != sizeof (struct linger)) {
				error = EINVAL;
				goto bad;
			}
			so->so_linger = mtod(m, struct linger *)->l_linger;
			if (mtod(m, struct linger *)->l_onoff)
				so->so_options |= SO_LINGER;
			else
				so->so_options &= ~SO_LINGER;
			break;

		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_USELOOPBACK:
		case SO_REUSEADDR:
#ifndef notyet
			/*
			 * 4.2 compatibility code.
			 *	THIS SHOULD GO AWAY WHEN WE NO LONGER REQUIRE
			 *	BINARY COMPATIBILITY WITH RELEASE 3.0.
			 *
			 * Convert to 4.3 representation by cobbling up an
			 * mbuf with a nonzero option value.
			 */
			if (m == NULL) {
				MGET(m, M_WAIT, MT_SOOPTS);
				m->m_len = sizeof (int);
				*mtod(m, int *) = 1;
			}
			/* fall through... */
#endif notyet
		case SO_BROADCAST:
		case SO_OOBINLINE:
			if (m == NULL || m->m_len < sizeof (int)) {
				error = EINVAL;
				goto bad;
			}
			if (*mtod(m, int *))
				so->so_options |= optname;
			else
				so->so_options &= ~optname;
			break;

		case SO_SNDBUF:
		case SO_RCVBUF:
		case SO_SNDLOWAT:
		case SO_RCVLOWAT:
		case SO_SNDTIMEO:
		case SO_RCVTIMEO:
			if (m == NULL || m->m_len < sizeof (int)) {
				error = EINVAL;
				goto bad;
			}
			switch (optname) {

			case SO_SNDBUF:
			case SO_RCVBUF:
				if (sbreserve(optname == SO_SNDBUF ?
				    &so->so_snd : &so->so_rcv,
				    *mtod(m, int *)) == 0) {
					error = ENOBUFS;
					goto bad;
				}
				break;

			case SO_SNDLOWAT:
				so->so_snd.sb_lowat = *mtod(m, int *);
				break;
			case SO_RCVLOWAT:
				so->so_rcv.sb_lowat = *mtod(m, int *);
				break;
			case SO_SNDTIMEO:
				so->so_snd.sb_timeo = *mtod(m, int *);
				break;
			case SO_RCVTIMEO:
				so->so_rcv.sb_timeo = *mtod(m, int *);
				break;
			}
			break;

		default:
			error = ENOPROTOOPT;
			break;
		}
	}
bad:
	if (m)
		(void) m_free(m);
	return (error);
}

sogetopt(so, level, optname, mp)
	register struct socket *so;
	int level, optname;
	struct mbuf **mp;
{
	register struct mbuf *m;

	if (level != SOL_SOCKET) {
		if (so->so_proto && so->so_proto->pr_ctloutput) {
			return ((*so->so_proto->pr_ctloutput)
					(PRCO_GETOPT, so, level, optname, mp));
		} else
			return (ENOPROTOOPT);
	} else {
		m = m_get(M_WAIT, MT_SOOPTS);
		m->m_len = sizeof (int);

		switch (optname) {

		case SO_LINGER:
			m->m_len = sizeof (struct linger);
			mtod(m, struct linger *)->l_onoff =
				so->so_options & SO_LINGER;
			mtod(m, struct linger *)->l_linger = so->so_linger;
			break;

		case SO_USELOOPBACK:
		case SO_DONTROUTE:
		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_REUSEADDR:
		case SO_BROADCAST:
		case SO_OOBINLINE:
			*mtod(m, int *) = so->so_options & optname;
			break;

		case SO_TYPE:
			*mtod(m, int *) = so->so_type;
			break;

		case SO_ERROR:
			*mtod(m, int *) = so->so_error;
			so->so_error = 0;
			break;

		case SO_SNDBUF:
			*mtod(m, int *) = so->so_snd.sb_hiwat;
			break;

		case SO_RCVBUF:
			*mtod(m, int *) = so->so_rcv.sb_hiwat;
			break;

		case SO_SNDLOWAT:
			*mtod(m, int *) = so->so_snd.sb_lowat;
			break;

		case SO_RCVLOWAT:
			*mtod(m, int *) = so->so_rcv.sb_lowat;
			break;

		case SO_SNDTIMEO:
			*mtod(m, int *) = so->so_snd.sb_timeo;
			break;

		case SO_RCVTIMEO:
			*mtod(m, int *) = so->so_rcv.sb_timeo;
			break;

		default:
			(void) m_free(m);
			return (ENOPROTOOPT);
		}
		*mp = m;
		return (0);
	}
}

sohasoutofband(so)
	register struct socket *so;
{
	struct proc *p;

	if (so->so_pgrp < 0)
		gsignal(-so->so_pgrp, SIGURG);
	else if (so->so_pgrp > 0 && (p = pfind(so->so_pgrp)) != 0)
		psignal(p, SIGURG);
	if (so->so_rcv.sb_sel) {
		selwakeup(so->so_rcv.sb_sel, so->so_rcv.sb_flags & SB_COLL);
		so->so_rcv.sb_sel = 0;
		so->so_rcv.sb_flags &= ~SB_COLL;
	}
}
