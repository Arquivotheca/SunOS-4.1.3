#ifndef lint
static        char sccsid[] = "@(#)subr_kudp.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif


/*
 * subr_kudp.c
 * Subroutines to do UDP/IP sendto and recvfrom in the kernel
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <mon/sunromvec.h>

struct mbuf     *mclgetx();

/*
 * General kernel udp stuff.
 * The routines below are used by both the client and the server side
 * rpc code.
 */

/*
 * Kernel recvfrom.
 * Pull address mbuf and data mbuf chain off socket receive queue.
 */
struct mbuf *
ku_recvfrom(so, from)
	register struct socket *so;
	struct sockaddr_in *from;
{
	register struct mbuf	*m;
	register struct mbuf	*m0;
	int		len = 0;

#ifdef RPCDEBUG1
	rpc_debug(6, "ku_recvfrom so=0x%x\n", so);
#endif
	m = so->so_rcv.sb_mb;
	if (m == NULL) {
		return (m);
	}

	*from = *mtod(m, struct sockaddr_in *);
	sbfree(&so->so_rcv, m);
	MFREE(m, m0);
	if (m0 == NULL) {
		printf("cku_recvfrom: no body!\n");
		so->so_rcv.sb_mb = m0;
		return (m0);
	}

	/*
	 * Walk down mbuf chain till m_act set (end of packet) or
	 * end of chain freeing socket buffer space as we go.
	 * After the loop m points to the last mbuf in the packet.
	 */
	m = m0;
	for (;;) {
		sbfree(&so->so_rcv, m);
		len += m->m_len;
		if (m->m_act || m->m_next == NULL) {
			break;
		}
		m = m->m_next;
	}

	so->so_rcv.sb_mb = m->m_next;
	m->m_next = NULL;
	if (len > UDPMSGSIZE) {
		printf("ku_recvfrom: len = %d\n", len);
	}

#ifdef RPCDEBUG1
	rpc_debug(6, "ku_recvfrom %d from 0x%x\n", len, from->sin_addr.s_addr);
#endif
	return (m0);
}


int Sendtries = 0;
int Sendok = 0;

/*
 * Kernel sendto.
 * Set addr and send off via UDP.
 * Use ku_fastsend if possible.
 */
int
ku_sendto_mbuf(so, m, addr)
	struct socket *so;
	struct mbuf *m;
	struct sockaddr_in *addr;
{
#ifdef SLOWSEND
	register struct inpcb *inp = sotoinpcb(so);
	int s;
#endif
	int error;

#ifdef RPCDEBUG1
	rpc_debug(6, "ku_sendto_mbuf(so 0x%x m0x%x addr 0x%x)\n",
		so, m, addr);
#endif
	Sendtries++;
#ifdef SLOWSEND
	s = splnet();
	if (error = in_pcbsetaddr(inp, addr)) {
		printf("pcbsetaddr failed %d\n", error);
		(void) splx(s);
		m_freem(m);
		return (error);
	}
	error = udp_output(inp, m);
	in_pcbdisconnect(inp);
	(void) splx(s);
#else
	error = ku_fastsend(so, m, addr);
#endif
	Sendok++;
#ifdef RPCDEBUG1
	rpc_debug(6, "ku_sendto_mbuf: ku_sendto returning %d\n", error);
#endif
	return (error);
}

#ifdef RPCDEBUG
int rpcdebug = 5;

/*VARARGS2*/
rpc_debug(level, str, a1, a2, a3, a4, a5, a6, a7, a8, a9)
        int level;
        char *str;
        int a1, a2, a3, a4, a5, a6, a7, a8, a9;
{

        if (level <= rpcdebug)
                printf(str, a1, a2, a3, a4, a5, a6, a7, a8, a9);
}
#else
rpc_debug(level, str, a1, a2, a3, a4, a5, a6, a7, a8, a9) {}
#endif
