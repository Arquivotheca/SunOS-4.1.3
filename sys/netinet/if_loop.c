/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)if_loop.c 1.1 92/07/30 SMI; from UCB 7.1 6/5/86
 */

/*
 * Loopback interface driver for protocol testing and timing.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/netisr.h>
#include <net/route.h>

#ifdef	INET
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_var.h>

struct ether_family *ether_families;

#endif

#ifdef vax
#include <vax/mtpr.h>
#endif

#define	LOMTU	(1024+512)

struct	ifnet loif;
int	looutput(), loioctl();

loattach()
{
	register struct ifnet *ifp = &loif;
	struct ifreq ifr;	

	ifp->if_name = "lo";
	ifp->if_mtu = LOMTU;
	ifp->if_flags = IFF_LOOPBACK;
	ifp->if_ioctl = loioctl;
	ifp->if_output = looutput;
	if_attach(ifp);
#ifndef BERK
	  /*
	   * For "temporary" backward compatibility with older
	   * Sun releases we set an address and turn the loopback
	   * interface on automagically.
	   */
	bzero((caddr_t)&ifr, sizeof(ifr) );
	IN_SET_LOOPBACK_ADDR((struct sockaddr_in*)&ifr.ifr_addr );
	(void)in_control((struct socket *)0, SIOCSIFADDR, (caddr_t)&ifr, ifp);
	ifp->if_flags |= IFF_UP;
#endif BERK
	ifp->if_flags |= IFF_RUNNING;
}

looutput(ifp, m0, dst)
	struct ifnet *ifp;
	register struct mbuf *m0;
	struct sockaddr *dst;
{
	int s;
	register struct ifqueue *ifq;
	struct mbuf *m;
	struct ether_family *efp;

	/*
	 * Place interface pointer before the data
	 * for the receiving protocol.
	 */
	if (m0->m_off <= MMAXOFF &&
	    m0->m_off >= MMINOFF + sizeof(struct ifnet *)) {
		m0->m_off -= sizeof(struct ifnet *);
		m0->m_len += sizeof(struct ifnet *);
	} else {
		MGET(m, M_DONTWAIT, MT_HEADER);
		if (m == (struct mbuf *)0)
			return (ENOBUFS);
		m->m_len = sizeof(struct ifnet *);
		m->m_next = m0;
		m0 = m;
	}
	*(mtod(m0, struct ifnet **)) = ifp;
	s = splimp();
	ifp->if_opackets++;
	switch (dst->sa_family) {

#ifdef INET
	case AF_INET:
		ifq = &ipintrq;
		if (IF_QFULL(ifq)) {
			IF_DROP(ifq);
			m_freem(m0);
			(void) splx(s);
			return (ENOBUFS);
		}
		IF_ENQUEUE(ifq, m0);
		schednetisr(NETISR_IP);
		break;
#endif

	default:
		/*
		 * If it was not one of our "native" address families,
		 * check the extended family table to see if anyone else
		 * wants it, and let them have it.
		 */
		for (efp = ether_families; efp != NULL; efp = efp->ef_next) {
			if (efp->ef_family == dst->sa_family)
				break;
		}
		if (efp != NULL && efp->ef_infunc != NULL) {
			ifq =  (*efp->ef_infunc)(ifp, m0);
			if (ifq == NULL)
				break;
			if (efp->ef_netisr != NULL)
				schednetisr(efp->ef_netisr);
			if (IF_QFULL(ifq)) {
				IF_DROP(ifq);
				m_freem(m0);
			}
			else IF_ENQUEUE(ifq, m0);
			break;
		}
		(void) splx(s);
		printf("lo%d: can't handle af%d\n", ifp->if_unit,
			dst->sa_family);
		m_freem(m0);
		return (EAFNOSUPPORT);
	}
	ifp->if_ipackets++;
	(void) splx(s);
	return (0);
}

/*
 * Process an ioctl request.
 */
/* ARGSUSED */
loioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	int error = 0;

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		/*
		 * Everything else is done at a higher level.
		 */
		break;

	default:
		error = EINVAL;
	}
	return (error);
}
