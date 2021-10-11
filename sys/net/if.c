/*
 * Copyright (c) 1980, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)if.c 1.1 92/07/30 SMI; from UCB 7.3 4/7/88
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/mbuf.h>

#include <net/if.h>
#include <net/af.h>
#include <net/if_arp.h>

#ifdef	OPENPROMS
#include <mon/obpdefs.h>
#else	OPENPROMS
#include <mon/sunromvec.h>
#endif	OPENPROMS
#include <sun/autoconf.h>
#include <sun/openprom.h>

#include "ether.h"

#ifdef OPENPROMS
#ifdef	MULTIPROCESSOR
/*
 * We can't risk user processes on other cpus bashing the output device
 * while the boot prom is also bashing it, so go through the code
 * that coordinates the output device.
 */
#define	prom_printf	mpprom_printf
#endif	MULTIPROCESSOR
/*
 * Undefine DPRINTF to remove compiled debug code.
 * #define DPRINTF		if (bootpath_debug) prom_printf
 */
#ifdef	DPRINTF
static	int bootpath_debug = 1;
#endif	DPRINTF
#endif	OPENPROMS

/*LINTLIBRARY*/

int	ifqmaxlen = IFQ_MAXLEN;

/*
 * Network interface utility routines.
 *
 * Routines with ifa_ifwith* names typically take sockaddr *'s as
 * parameters.
 */

ifinit()
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if (ifp->if_snd.ifq_maxlen == 0)
			ifp->if_snd.ifq_maxlen = ifqmaxlen;
	if_slowtimo();
}

#ifdef vax
/*
 * Call each interface on a Unibus reset.
 */
ifubareset(uban)
	int uban;
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if (ifp->if_reset)
			(*ifp->if_reset)(ifp->if_unit, uban);
}
#endif

/*
 * Attach an interface to the
 * list of "active" interfaces.
 */
if_attach(ifp)
	struct ifnet *ifp;
{
	register struct ifnet **p = &ifnet;

	while (*p)
		p = &((*p)->if_next);
	*p = ifp;
}

/*
 * Locate an interface based on a complete address.
 */
/*ARGSUSED*/
struct ifaddr *
ifa_ifwithaddr(addr)
	struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

#define	equal(a1, a2) \
	(bcmp((caddr_t)((a1)->sa_data), (caddr_t)((a2)->sa_data), 14) == 0)
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr.sa_family != addr->sa_family)
			continue;
		if (equal(&ifa->ifa_addr, addr))
			return (ifa);
		if ((ifp->if_flags & IFF_BROADCAST) &&
		    equal(&ifa->ifa_broadaddr, addr))
			return (ifa);
	}
	return ((struct ifaddr *)0);
}

/*
 * Locate the point to point interface with a given destination address.
 */
/*ARGSUSED*/
struct ifaddr *
ifa_ifwithdstaddr(addr)
	struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    if (ifp->if_flags & IFF_POINTOPOINT)
		for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr.sa_family != addr->sa_family)
				continue;
			if (equal(&ifa->ifa_dstaddr, addr))
				return (ifa);
	}
	return ((struct ifaddr *)0);
}

/*
 * Find an interface on a specific network.  If many, choice
 * is first found.
 */
struct ifaddr *
ifa_ifwithnet(addr)
	register struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;
	register u_int af = addr->sa_family;
	register int (*netmatch)();

	if (af >= AF_MAX)
		return (0);
	netmatch = afswitch[af].af_netmatch;
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr.sa_family != addr->sa_family)
			continue;
		if ((*netmatch)(&ifa->ifa_addr, addr))
			return (ifa);
	}
	return ((struct ifaddr *)0);
}

/*
 * Find an interface using a specific address family.
 * First try for one with the UP bit set, then accept anyone.
 */
struct ifaddr *
ifa_ifwithaf(af)
	register int af;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    if (ifp->if_flags & IFF_UP)
		for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
		    if (ifa->ifa_addr.sa_family == af)
			return (ifa);
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
		if (ifa->ifa_addr.sa_family == af)
			return (ifa);
	return ((struct ifaddr *)0);
}

/*
 * At this point, we have not initialised any interface (except
 * loopback!), so we choose an arbitrary interface on which to
 * try a revarp.
 */
/*ARGSUSED*/
struct ifnet *
ifb_ifwithaf(af)
	int af;
{
#ifndef OPENPROMS
	register struct bootparam *bp;
#endif
	register struct ifnet *ifp;
#ifdef	OPENPROMS
	char	name[OBP_MAXDEVNAME];
#else	OPENPROMS
	char	name[32];
#endif	OPENPROMS
	short unit;
#ifdef DPRINTF
	int	i;
#endif  DPRINTF
	extern char	*strcpy(), *strncpy();
	struct ifnet	*ifname();

#ifdef	OPENPROMS
	extern char	*prom_bootpath();	/* For debug only! */

#ifdef	DPRINTF
	char	*path;

	path = prom_bootpath();
	DPRINTF("ifb_ifwithaf: prom bootpath %s --> ", path ? path : "<NONE>");
#endif	DPRINTF

	name[0] = (char)0;		/* In case of error getting name */
#ifdef DPRINTF
	i = prom_get_boot_dev_name(name, sizeof name);
#else
        (void) prom_get_boot_dev_name(name, sizeof name);
#endif


#ifdef	DPRINTF
	if (i == 0)
		DPRINTF("device %s, ", name);
	else
		DPRINTF("device <NONE?>, ");
#endif	DPRINTF

	unit = (short)prom_get_boot_dev_unit();

#ifdef	DPRINTF
	DPRINTF("unit %d\n", (int)unit);
#endif	DPRINTF

#else	OPENPROMS
	bp = *romp->v_bootparam;
	name[0] = bp->bp_dev[0];
	name[1] = bp->bp_dev[1];
	name[2] = '\0';
	unit = (short) bp->bp_ctlr;
#endif

	/*
	 * Translate "gn" to "fddi"
	 * -- this hack for the Narya VME FDDI/DX interface.
	 */
	if (!strcmp(name, "gn"))
		(void) strcpy(name, "fddi");

	/*
	 * Look for matching name/unit in the ifnet linked list.
	 */
	if (ifp = ifname(name, unit))
		return (ifp);

	/*
	 * Otherwise, default to first found non-point-to-point,
	 * non-loopback interface.
	 */
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if (!(ifp->if_flags & IFF_POINTOPOINT) &&
			!(ifp->if_flags & IFF_LOOPBACK))
			return (ifp);
	return (NULL);
}

/*
 * Mark an interface down and notify protocols of
 * the transition.
 * NOTE: must be called at splnet or eqivalent.
 */
if_down(ifp)
	register struct ifnet *ifp;
{
	register struct ifaddr *ifa;

	ifp->if_flags &= ~IFF_UP;
	for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
		pfctlinput(PRC_IFDOWN, &ifa->ifa_addr);
	if_qflush(&ifp->if_snd);
}

/*
 * Flush an interface queue.
 */
if_qflush(ifq)
	register struct ifqueue *ifq;
{
	register struct mbuf *m, *n;

	n = ifq->ifq_head;
	while (m = n) {
		n = m->m_act;
		m_freem(m);
	}
	ifq->ifq_head = 0;
	ifq->ifq_tail = 0;
	ifq->ifq_len = 0;
}

/*
 * Handle interface watchdog timer routines.  Called
 * from softclock, we decrement timers (if set) and
 * call the appropriate interface routine on expiration.
 */
if_slowtimo()
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_timer == 0 || --ifp->if_timer)
			continue;
		if (ifp->if_watchdog)
			(*ifp->if_watchdog)(ifp->if_unit);
	}
	timeout(if_slowtimo, (caddr_t)0, hz / IFNET_SLOWHZ);
}

/*
 * Map interface name to
 * interface structure pointer.
 */
struct ifnet *
ifunit(name, size)
	register char *name;
	int size;
{
	register char *cp;
	register struct ifnet *ifp;
	int unit;
	int namlen = 0;

	for (cp = name; cp < name + size && *cp; cp++) {
		if (*cp >= '0' && *cp <= '9')
			break;
		namlen++;
	}
	if (*cp == '\0' || cp == name + size || cp == name)
		return ((struct ifnet *)0);
	unit = 0;
	while (*cp && cp < name + size)
		unit = 10*unit + (*cp++ - '0');
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if (unit == ifp->if_unit && namlen == strlen(ifp->if_name) &&
		    bcmp(ifp->if_name, name, namlen) == 0)
			return (ifp);
	return ((struct ifnet *)0);
}

/*
 * Map interface name and unit
 * to interface structure pointer.
 * Return NULL if not found.
 */
struct ifnet *
ifname(name, unit)
char    *name;
short   unit;
{
	register	struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if (!strcmp(ifp->if_name, name) && (ifp->if_unit == unit))
			return (ifp);

	return (NULL);
}

/*
 * Interface ioctls.
 */
ifioctl(so, cmd, data)
	struct socket *so;
	int cmd;
	caddr_t data;
{
	register struct ifnet *ifp, *oifp;
	register struct ifreq *ifr;

	switch (cmd) {

	case SIOCGIFCONF:
		return (ifconf(cmd, data));

#if defined(INET) && NETHER > 0
	case SIOCSARP:
	case SIOCDARP:
		if (!suser())
			return (u.u_error);
		/* FALL THROUGH */
	case SIOCGARP:
		return (arpioctl(cmd, data));
#endif
	}

	ifr = (struct ifreq *)data;
	ifp = ifunit(ifr->ifr_name, IFNAMSIZ);
	if (ifp == 0)
		return (ENXIO);

	switch (cmd) {

	case SIOCGIFFLAGS:
		ifr->ifr_flags = ifp->if_flags;
		break;

	case SIOCGIFMETRIC:
		ifr->ifr_metric = ifp->if_metric;
		break;

	case SIOCSIFFLAGS:
		if (!suser())
			return (u.u_error);
		if (ifp->if_flags & IFF_UP && (ifr->ifr_flags & IFF_UP) == 0) {
			int s = splimp();
			if_down(ifp);
			(void) splx(s);
		}
#ifdef VDDRV
		/*
		 * For a loadable driver, we need to set ifqmaxlen here,
		 * since it wasn't done in ifinit.
		 */
		if (ifp->if_snd.ifq_maxlen == 0)
			ifp->if_snd.ifq_maxlen = ifqmaxlen;
#endif
		ifp->if_flags = (ifp->if_flags & IFF_CANTCHANGE) |
			(ifr->ifr_flags &~ IFF_CANTCHANGE);
		if (ifp->if_ioctl)
			(void) (*ifp->if_ioctl)(ifp, cmd, data);
		break;

	case SIOCSIFMETRIC:
		if (!suser())
			return (u.u_error);
		ifp->if_metric = ifr->ifr_metric;
		break;

	case SIOCUPPER:
		oifp = ifunit(ifr->ifr_oname, IFNAMSIZ);
		if (oifp == 0)
			return (ENXIO);
		if (oifp->if_input == 0)
			return (EINVAL);
		ifp->if_upper = oifp;
		break;

	case SIOCLOWER:
		oifp = ifunit(ifr->ifr_oname, IFNAMSIZ);
		if (oifp == 0)
			return (ENXIO);
		if (oifp->if_output == 0)
			return (EINVAL);
		ifp->if_lower = oifp;
		break;

	case SIOCSPROMISC:
		if (!suser())
			return (u.u_error);
		return (ifpromisc(ifp, *(int *)data));

	/*
	 * Set/zap multicast address.  We demand root privilege,
	 * but otherwise leave it to the protocol to decide what
	 * to do with it.  This strategy has the same coordination
	 * problems among multiple protocols that setting the
	 * individual address does...
	 */
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		if (!suser())
			return (u.u_error);
		/* FALL THROUGH */

	default:
		if (so->so_proto == 0)
			return (EOPNOTSUPP);
		return ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL,
			cmd, data, ifp));
	}
	return (0);
}

/*
 * Handle transitions into and out of promiscuous mode.  Keep track
 * of how many outstanding requests there are for the mode and call
 * the interface to change mode on transitions to and from zero.
 */
int
ifpromisc(ifp, on)
	register struct ifnet	*ifp;
	register int		on;
{
	register int	change;

	if (on)
		change = ifp->if_promisc++ == 0;
	else
		change = (ifp->if_promisc != 0) ? (--ifp->if_promisc == 0) : 0;
	if (!change)
		return (0);

	/*
	 * Force the IFF_PROMISC bit to match the
	 * value implied by the if_promisc field.
	 */
	if (on)
		ifp->if_flags |= IFF_PROMISC;
	else
		ifp->if_flags &= ~IFF_PROMISC;

	return ((*ifp->if_ioctl)(ifp, SIOCSIFFLAGS, (caddr_t) 0));
}

/*
 * Return interface configuration
 * of system.  List may be used
 * in later ioctl's (above) to get
 * other information.
 */
/*ARGSUSED*/
ifconf(cmd, data)
	int cmd;
	caddr_t data;
{
	register struct ifconf *ifc = (struct ifconf *)data;
	register struct ifnet *ifp = ifnet;
	register struct ifaddr *ifa;
	register char *cp, *ep;
	struct ifreq ifr, *ifrp;
	int space = ifc->ifc_len, error = 0;

	ifrp = ifc->ifc_req;
	ep = ifr.ifr_name + sizeof (ifr.ifr_name) - 4;
	for (; space >= sizeof (ifr) && ifp; ifp = ifp->if_next) {
		bcopy(ifp->if_name, ifr.ifr_name, sizeof (ifr.ifr_name) - 2);
		for (cp = ifr.ifr_name; cp < ep && *cp; cp++)
			;
		if (ifp->if_unit > 99)
			*cp++ = '0' + (ifp->if_unit / 100);
		if (ifp->if_unit > 9)
			*cp++ = '0' + (ifp->if_unit % 100)/10;
		*cp++ = '0' + (ifp->if_unit % 10);
		*cp = '\0';
		if ((ifa = ifp->if_addrlist) == 0) {
			bzero((caddr_t)&ifr.ifr_addr, sizeof (ifr.ifr_addr));
			error = copyout((caddr_t)&ifr, (caddr_t)ifrp,
						(u_int)(sizeof ifr));
			if (error)
				break;
			space -= sizeof (ifr), ifrp++;
		} else
		    for (; space > sizeof (ifr) && ifa; ifa = ifa->ifa_next) {
			ifr.ifr_addr = ifa->ifa_addr;
			error = copyout((caddr_t)&ifr, (caddr_t)ifrp,
			    sizeof (ifr));
			if (error)
				break;
			space -= sizeof (ifr), ifrp++;
		}
	}
	ifc->ifc_len -= space;
	return (error);
}
