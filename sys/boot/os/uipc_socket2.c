#ifndef lint
static        char sccsid[] = "@(#)uipc_socket2.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif


/*
 * Copyright (c) 1984 Sun Microsystems, Inc.
 */
#include <sys/param.h>
#include "boot/systm.h"
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/mbuf.h>
#include "boot/protosw.h"
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <netinet/in.h>
#include <mon/sunromvec.h>

#define SB_MAXCOUNT	32767

/*
 * Primitive routines for operating on sockets and socket buffers
 */

/*
 * Procedures to manipulate state flags of socket
 * and do appropriate wakeups.  Normal sequence from the
 * active (originating) side is that soisconnecting() is
 * called during processing of connect() call,
 * resulting in an eventual call to soisconnected() if/when the
 * connection is established.  When the connection is torn down
 * soisdisconnecting() is called during processing of disconnect() call,
 * and soisdisconnected() is called when the connection to the peer
 * is totally severed.  The semantics of these routines are such that
 * connectionless protocols can call soisconnected() and soisdisconnected()
 * only, bypassing the in-progress calls when setting up a ``connection''
 * takes no time.
 *
 * From the passive side, a socket is created with
 * two queues of sockets: so_q0 for connections in progress
 * and so_q for connections already made and awaiting user acceptance.
 * As a protocol is preparing incoming connections, it creates a socket
 * structure queued on so_q0 by calling sonewconn().  When the connection
 * is established, soisconnected() is called, and transfers the
 * socket structure to so_q, making it available to accept().
 * 
 * If a socket is closed with sockets on either
 * so_q0 or so_q, these sockets are dropped.
 *
 * If higher level protocols are implemented in
 * the kernel, the wakeups done here will sometimes
 * cause software-interrupt process scheduling.
 */

#ifdef	NEVER
soisconnecting(so)
	register struct socket *so;
{

	so->so_state &= ~(SS_ISCONNECTED|SS_ISDISCONNECTING);
	so->so_state |= SS_ISCONNECTING;
	wakeup((caddr_t)&so->so_timeo);
}

soisconnected(so)
	register struct socket *so;
{
	register struct socket *head = so->so_head;

	if (head) {
		if (soqremque(so, 0) == 0)
			panic("soisconnected");
		soqinsque(head, so, 1);
		sorwakeup(head);
		wakeup((caddr_t)&head->so_timeo);
	}
	so->so_state &= ~(SS_ISCONNECTING|SS_ISDISCONNECTING);
	so->so_state |= SS_ISCONNECTED;
	wakeup((caddr_t)&so->so_timeo);
	sorwakeup(so);
	sowwakeup(so);
}

soisdisconnecting(so)
	register struct socket *so;
{

	so->so_state &= ~SS_ISCONNECTING;
	so->so_state |= (SS_ISDISCONNECTING|SS_CANTRCVMORE|SS_CANTSENDMORE);
	wakeup((caddr_t)&so->so_timeo);
	sowwakeup(so);
	sorwakeup(so);
}

soisdisconnected(so)
	register struct socket *so;
{

	so->so_state &= ~(SS_ISCONNECTING|SS_ISCONNECTED|SS_ISDISCONNECTING);
	so->so_state |= (SS_CANTRCVMORE|SS_CANTSENDMORE);
	wakeup((caddr_t)&so->so_timeo);
	sowwakeup(so);
	sorwakeup(so);
}

/*
 * When an attempt at a new connection is noted on a socket
 * which accepts connections, sonewconn is called.  If the
 * connection is possible (subject to space constraints, etc.)
 * then we allocate a new structure, propoerly linked into the
 * data structure of the original socket, and return this.
 */
struct socket *
sonewconn(head)
	register struct socket *head;
{
	register struct socket *so;
	register struct mbuf *m;

	if (head->so_qlen + head->so_q0len > 3 * head->so_qlimit / 2)
		goto bad;
	m = m_getclr(M_DONTWAIT, MT_SOCKET);
	if (m == NULL)
		goto bad;
	so = mtod(m, struct socket *);
	so->so_type = head->so_type;
	so->so_options = head->so_options &~ SO_ACCEPTCONN;
	so->so_linger = head->so_linger;
	so->so_state = head->so_state | SS_NOFDREF;
	so->so_proto = head->so_proto;
	so->so_timeo = head->so_timeo;
	so->so_pgrp = head->so_pgrp;
	soqinsque(head, so, 0);
	if ((*so->so_proto->pr_usrreq)(so, PRU_ATTACH,
	    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0)) {
		(void) soqremque(so, 0);
		(void) m_free(m);
		goto bad;
	}
	return (so);
bad:
	return ((struct socket *)0);
}

soqinsque(head, so, q)
	register struct socket *head, *so;
	int q;
{

	so->so_head = head;
	if (q == 0) {
		head->so_q0len++;
		so->so_q0 = head->so_q0;
		head->so_q0 = so;
	} else {
		head->so_qlen++;
		so->so_q = head->so_q;
		head->so_q = so;
	}
}

soqremque(so, q)
	register struct socket *so;
	int q;
{
	register struct socket *head, *prev, *next;

	head = so->so_head;
	prev = head;
	for (;;) {
		next = q ? prev->so_q : prev->so_q0;
		if (next == so)
			break;
		if (next == head)
			return (0);
		prev = next;
	}
	if (q == 0) {
		prev->so_q0 = next->so_q0;
		head->so_q0len--;
	} else {
		prev->so_q = next->so_q;
		head->so_qlen--;
	}
	next->so_q0 = next->so_q = 0;
	next->so_head = 0;
	return (1);
}

/*
 * Socantsendmore indicates that no more data will be sent on the
 * socket; it would normally be applied to a socket when the user
 * informs the system that no more data is to be sent, by the protocol
 * code (in case PRU_SHUTDOWN).  Socantrcvmore indicates that no more data
 * will be received, and will normally be applied to the socket by a
 * protocol when it detects that the peer will send no more data.
 * Data queued for reading in the socket may yet be read.
 */

socantsendmore(so)
	struct socket *so;
{

	so->so_state |= SS_CANTSENDMORE;
	sowwakeup(so);
}

socantrcvmore(so)
	struct socket *so;
{

	so->so_state |= SS_CANTRCVMORE;
	sorwakeup(so);
}

/*
 * Socket select/wakeup routines.
 */

/*
 * Queue a process for a select on a socket buffer.
 */
sbselqueue(sb)
	struct sockbuf *sb;
{
	register struct proc *p;

	if ((p = sb->sb_sel) && p->p_wchan == (caddr_t)&selwait)
		sb->sb_flags |= SB_COLL;
	else
		sb->sb_sel = u.u_procp;
}

/*
 * Wait for data to arrive at/drain from a socket buffer.
 */
sbwait(sb)
	struct sockbuf *sb;
{

	sb->sb_flags |= SB_WAIT;
	(void) sleep((caddr_t)&sb->sb_cc, PZERO+1);
}

/*
 * Wakeup processes waiting on a socket buffer.
 */
sbwakeup(sb)
	register struct sockbuf *sb;
{

	if (sb->sb_sel) {
		selwakeup(sb->sb_sel, sb->sb_flags & SB_COLL);
		sb->sb_sel = 0;
		sb->sb_flags &= ~SB_COLL;
	}
	if (sb->sb_flags & SB_WAIT) {
		sb->sb_flags &= ~SB_WAIT;
		wakeup((caddr_t)&sb->sb_cc);
	}
}

/*
 * Wakeup socket readers and writers.
 * Do asynchronous notification via SIGIO
 * if the socket has the SS_ASYNC flag set.
 */
sowakeup(so, sb)
	register struct socket *so;
	struct sockbuf *sb;
{
	register struct proc *p;

	sbwakeup(sb);
	if (so->so_state & SS_ASYNC) {
		if (so->so_pgrp == 0)
			return;
		else if (so->so_pgrp > 0)
			gsignal(so->so_pgrp, SIGIO);
		else if ((p = pfind(-so->so_pgrp)) != 0)
			psignal(p, SIGIO);
	}
}
#endif	/* NEVER */

/*
 * Socket buffer (struct sockbuf) utility routines.
 *
 * Each socket contains two socket buffers: one for sending data and
 * one for receiving data.  Each buffer contains a queue of mbufs,
 * information about the number of mbufs and amount of data in the
 * queue, and other fields allowing select() statements and notification
 * on data availability to be implemented.
 *
 * Before using a new socket structure it is first necessary to reserve
 * buffer space to the socket, by calling sbreserve.  This commits
 * some of the available buffer space in the system buffer pool for the
 * socket.  The space should be released by calling sbrelease when the
 * socket is destroyed.
 *
 * The routine sbappend() is normally called to append new mbufs
 * to a socket buffer, after checking that adequate space is available
 * comparing the function sbspace() with the amount of data to be added.
 * Data is normally removed from a socket buffer in a protocol by
 * first calling m_copy on the socket buffer mbuf chain and sending this
 * to a peer, and then removing the data from the socket buffer with
 * sbdrop when the data is acknowledged by the peer (or immediately
 * in the case of unreliable protocols.)
 *
 * Protocols which do not require connections place both source address
 * and data information in socket buffer queues.  The source addresses
 * are stored in single mbufs before each data item;
 * the data items are all marked with end of record markers.  The
 * sbappendaddr() routine stores a datum and associated address in
 * a socket buffer.  Note that, unlike sbappend(), this routine checks
 * for the caller that there will be enough space to store the data.
 * It fails if there is not enough space, or if it cannot find
 * a mbuf to store the address in.
 *
 * The higher-level routines sosend and soreceive (in socket.c)
 * also add data to, and remove data from socket buffers repectively.
 */

soreserve(so, sndcc, rcvcc)
	register struct socket *so;
	int sndcc, rcvcc;
{

	if (sbreserve(&so->so_snd, sndcc) == 0)
		goto bad;
	if (sbreserve(&so->so_rcv, rcvcc) == 0)
		goto bad2;
	return (0);
bad2:
	sbrelease(&so->so_snd);
bad:
	return (ENOBUFS);
}

/*
 * Allot mbufs to a sockbuf.
 */
sbreserve(sb, cc)
	struct sockbuf *sb;
{

	/* someday maybe this routine will fail... */
	sb->sb_hiwat = cc;
	/* * 2 implies names can be no more than 1 mbuf each */
	sb->sb_mbmax = cc<<1;
	/* XXX Handle small sockbuf counters */
	if (sb->sb_hiwat <= 0)
		sb->sb_hiwat = SB_MAXCOUNT;
	if (sb->sb_mbmax <= 0)
		sb->sb_mbmax = SB_MAXCOUNT;
	return (1);
}

/*
 * Free mbufs held by a socket, and reserved mbuf space.
 */
sbrelease(sb)
	struct sockbuf *sb;
{

	sbflush(sb);
	sb->sb_hiwat = sb->sb_mbmax = 0;
}

/*
 * Routines to add (at the end) and remove (from the beginning)
 * data from a mbuf queue.
 */

/*
 * Append mbuf queue m to sockbuf sb.
 * Note: does not set "endofrecord" indicator.
 */
sbappend(sb, m)
	register struct mbuf *m;
	register struct sockbuf *sb;
{
	register struct mbuf *n;


	n = sb->sb_mb;
	if (n)
		while (n->m_next)
			n = n->m_next;
	while (m) {
		if (m->m_len == 0 && (int)m->m_act == 0) {
			m = m_free(m);
			continue;
		}
		if (n && n->m_off <= MMAXOFF && m->m_off <= MMAXOFF &&
		   (int)n->m_act == 0 && (int)m->m_act == 0 &&
		   (n->m_off + n->m_len + m->m_len) <= MMAXOFF) {
			bcopy(mtod(m, caddr_t), mtod(n, caddr_t) + n->m_len,
			    (unsigned)m->m_len);
			n->m_len += m->m_len;
			sb->sb_cc += m->m_len;
			m = m_free(m);
			continue;
		}
		sballoc(sb, m);
		if (n == 0)
			sb->sb_mb = m;
		else
			n->m_next = m;
		n = m;
		m = m->m_next;
		n->m_next = 0;
	}
}

/*
 * Append data and address.
 * Return 0 if no space in sockbuf or if
 * can't get mbuf to stuff address in.
 */
sbappendaddr(sb, asa, m0, rights0, rightsflag)
	struct sockbuf *sb;
	struct sockaddr *asa;
	struct mbuf *m0, *rights0;
	int rightsflag;
{
	register struct mbuf *m;
	register int len = sizeof (struct sockaddr);
	register struct mbuf *rights;


	if (rights0)
		len += rights0->m_len;
	m = m0;
	if (m == 0)
		panic("sbappendaddr");
	for (;;) {
		len += m->m_len;
		if (m->m_next == 0) {
			m->m_act = (struct mbuf *)1;
			break;
		}
		m = m->m_next;
	}
	if (len > sbspace(sb))
		return (0);
	m = m_get(M_DONTWAIT, MT_SONAME);
	if (m == NULL)
		return (0);
	m->m_len = sizeof (struct sockaddr);
	m->m_act = (struct mbuf *)1;
	*mtod(m, struct sockaddr *) = *asa;
	if (rightsflag) {
		if (rights0 == 0 || rights0->m_len == 0) {
			rights = m_get(M_DONTWAIT, MT_SONAME);
			if (rights)
				rights->m_len = 0;
		} else
			rights = m_copy(rights0, 0, rights0->m_len);
		if (rights == 0) {
			m_freem(m);
			return (0);
		}
		rights->m_act = (struct mbuf *)1;
		m->m_next = rights;
		rights->m_next = m0;
	} else {
		m->m_next = m0;
	}
	sbappend(sb, m);
	return (1);
}


/*
 * Free all mbufs on a sockbuf mbuf chain.
 * Check that resource allocations return to 0.
 */
sbflush(sb)
	register struct sockbuf *sb;
{

	if (sb->sb_flags & SB_LOCK)
		panic("sbflush");
	if (sb->sb_cc)
		sbdrop(sb, (int)sb->sb_cc);
	if (sb->sb_cc || sb->sb_mbcnt || sb->sb_mb)
		panic("sbflush 2");
}

/*
 * Drop data from (the front of) a sockbuf chain.
 */
sbdrop(sb, len)
	register struct sockbuf *sb;
	register int len;
{
	register struct mbuf *m = sb->sb_mb, *mn;

	while (len > 0 || (m && m->m_len == 0)) {
		if (m == 0)
			panic("sbdrop");
		if (m->m_len > len) {
			m->m_len -= len;
			m->m_off += len;
			sb->sb_cc -= len;
			break;
		}
		len -= m->m_len;
		sbfree(sb, m);
		MFREE(m, mn);
		m = mn;
	}
	sb->sb_mb = m;
}
