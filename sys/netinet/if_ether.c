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
 *	@(#)if_ether.c 1.1 92/07/30 SMI; from UCB 7.6 12/7/87
 */

/*
 * Ethernet address resolution protocol.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>

#define	ARPTAB_BSIZ	32		/* bucket size */
#define	ARPTAB_NB	16		/* number of buckets */
#define	ARPTAB_SIZE	(ARPTAB_BSIZ * ARPTAB_NB)
struct	arptab arptab[ARPTAB_SIZE];
int	arptab_size = ARPTAB_SIZE;	/* for arp command */

#define	ARPTAB_HASH(a) \
	((u_long)(a) % ARPTAB_NB)

#define	ARPTAB_LOOK(at,addr) { \
	register n; \
	at = &arptab[ARPTAB_HASH(addr) * ARPTAB_BSIZ]; \
	for (n = 0 ; n < ARPTAB_BSIZ ; n++,at++) \
		if (at->at_iaddr.s_addr == addr) \
			break; \
	if (n >= ARPTAB_BSIZ) \
		at = 0; \
}

/* timer values */
#define	ARPT_AGE	(60*1)	/* aging timer, 1 min. */
#define	ARPT_KILLC	20	/* kill completed entry in 20 mins. */
#define	ARPT_KILLI	3	/* kill incomplete entry in 3 minutes */

#ifndef lint
struct ether_addr etherbroadcastaddr = {{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }};
#endif lint
extern struct ifnet loif;

/*
 * Timeout routine.  Age arp_tab entries once a minute.
 */
arptimer()
{
	register struct arptab *at;
	register i;

	timeout(arptimer, (caddr_t)0, ARPT_AGE * hz);
	at = &arptab[0];
	for (i = 0; i < ARPTAB_SIZE; i++, at++) {
		if (at->at_flags == 0 || (at->at_flags & ATF_PERM))
			continue;
		if (in_broadcast(at->at_iaddr)) {
			arptfree(at);
			continue;
		}
		if (++at->at_timer < ((at->at_flags&ATF_COM) ?
		    ARPT_KILLC : ARPT_KILLI))
			continue;
		/* timer has expired, clear entry */
		arptfree(at);
	}
#ifdef lint
	arptab_size = arptab_size;
#endif
}

/*
 * Broadcast an ARP packet, asking who has addr on interface ac.
 */
arpwhohas(ac, addr)
	register struct arpcom *ac;
	struct in_addr *addr;
{
	register struct mbuf *m;
	register struct ether_header *eh;
	register struct ether_arp *ea;
	struct sockaddr sa;

	if ((m = m_get(M_DONTWAIT, MT_DATA)) == NULL)
		return (1);
	m->m_len = sizeof *ea;
	m->m_off = MMAXOFF - m->m_len;
	ea = mtod(m, struct ether_arp *);
	eh = (struct ether_header *)sa.sa_data;
	bzero((caddr_t)ea, sizeof (*ea));
	ether_copy( &etherbroadcastaddr, &eh->ether_dhost);
	eh->ether_type = htons(ETHERTYPE_ARP);		/* Network order */
	ea->arp_hrd = htons(ARPHRD_ETHER);
	ea->arp_pro = htons(ETHERTYPE_IP);
	ea->arp_hln = sizeof(ea->arp_sha);	/* hardware address length */
	ea->arp_pln = sizeof(ea->arp_spa);	/* protocol address length */
	ea->arp_op = htons(ARPOP_REQUEST);
	ether_copy(&ac->ac_enaddr, &ea->arp_sha);
	ether_copy(&ac->ac_ipaddr, ea->arp_spa);
	ip_copy(addr, ea->arp_tpa);
	sa.sa_family = AF_UNSPEC;
	return ((*ac->ac_if.if_output)(&ac->ac_if, m, &sa));
}

/*
 * Resolve an IP address into an ethernet address.  If success, 
 * desten is filled in.  If there is no entry in arptab,
 * set one up and broadcast a request for the IP address.
 * Hold onto this mbuf and resend it once the address
 * is finally resolved.  A return value of 1 indicates
 * that desten has been filled in and the packet should be sent
 * normally; a 0 return indicates that the packet has been
 * taken over here, either now or for later transmission.
 *
 * We do some (conservative) locking here at splimp, since
 * arptab is also altered from input interrupt service (ecintr/ilintr
 * calls arpinput when ETHERTYPE_ARP packets come in).
 */
arpresolve(ac, m)
	register struct arpcom *ac;
	struct mbuf *m;
{
	register struct arptab *at;
	struct in_ifaddr *ia;
	struct sockaddr_in sin;
	int s;

	s = splimp();
	ARPTAB_LOOK(at, ac->ac_lastip.s_addr);
	if (at == 0) {			/* not found */
		if (in_broadcast(ac->ac_lastip)) {
			/* 
			 * broadcast address 
			 */
			(void) splx(s);
			ether_copy(&etherbroadcastaddr, &ac->ac_lastarp);
			return (1);
		}
		for (ia = in_ifaddr; ia; ia = ia->ia_next) {
		  /*
		   * if for us, use software loopback driver if desired
		   * BSD only checked this interface's address, but we 
		   * go through all of them to handle the race condition 
		   * where two interfaces on different subnets have been
		   * turned on but not had their netmasks set yet.
		   */
		    if (ac->ac_lastip.s_addr == IA_SIN(ia)->sin_addr.s_addr) {
			(void) splx(s);
			sin.sin_family = AF_INET;
			sin.sin_addr = ac->ac_lastip;
			(void) looutput(&loif, m, (struct sockaddr *)&sin);
			/*
			 * The packet has already been sent and freed.
			 */
			return (0);
		    }
		}
		at = arptnew(&ac->ac_lastip);
		at->at_hold = m;
		at->at_tvsec = time.tv_sec;
		(void) splx(s);
		(void) arpwhohas(ac, &ac->ac_lastip);
		return (0);
	}
	at->at_timer = 0;		/* restart the timer */
	if (at->at_flags & ATF_COM) {	/* entry IS complete */
		ether_copy(&at->at_enaddr, &ac->ac_lastarp);
		(void) splx(s);
		return (1);
	}
	/*
	 * There is an arptab entry, but no ethernet address
	 * response yet.  Replace the held mbuf with this
	 * latest one.
	 * Only send one ARP request per second to avoid storms.
	 */
	if (at->at_hold)
		m_freem(at->at_hold);
	at->at_hold = m;
	if (at->at_tvsec != time.tv_sec)
		(void) arpwhohas(ac, &ac->ac_lastip);	/* ask again */
	at->at_tvsec = time.tv_sec;
	(void) splx(s);
	return (0);
}

/*
 * Called from 10 Mb/s Ethernet interrupt handlers
 * when ether packet type ETHERTYPE_ARP
 * is received.  Common length and type checks are done here,
 * then the protocol-specific routine is called.
 */
arpinput(ac, m)
	struct arpcom *ac;
	struct mbuf *m;
{
	register struct arphdr *ar;

	if (ac->ac_if.if_flags & IFF_NOARP)
		goto out;
	IF_ADJ(m);
	if (m->m_len < sizeof(struct arphdr))
		goto out;
	ar = mtod(m, struct arphdr *);
	if (m->m_len < sizeof(struct arphdr) + 2 * ar->ar_hln + 2 * ar->ar_pln)
		goto out;

	switch (ntohs(ar->ar_pro)) {

	case ETHERTYPE_IP:
		in_arpinput(ac, m);
		return;

	default:
		break;
	}
out:
	m_freem(m);
}

/*
 * ARP for Internet protocols on 10 Mb/s Ethernet.
 * Algorithm is that given in RFC 826.
 * In addition, a sanity check is performed on the sender
 * protocol address, to catch impersonators.
 * We also used to handle negotiations for use of trailer protocol,
 * but now just always use normal packets.
 */
in_arpinput(ac, m)
	register struct arpcom *ac;
	struct mbuf *m;
{
	register struct ether_arp *ea;
	struct ether_header *eh;
	register struct arptab *at;  /* same as "merge" flag */
	struct sockaddr_in sin;
	struct sockaddr sa;
	struct in_addr isaddr, itaddr, myaddr;
	int op, s;

	myaddr = ac->ac_ipaddr;
	ea = mtod(m, struct ether_arp *);
	op = ntohs(ea->arp_op);
	ip_copy( ea->arp_spa, &isaddr);
	ip_copy( ea->arp_tpa, &itaddr);
	if (!ether_cmp(&ea->arp_sha, &ac->ac_enaddr))
		goto out;	/* it's from me, ignore it. */
	if (!ether_cmp(&ea->arp_sha, &etherbroadcastaddr)) {
		log(LOG_ERR,
		    "arp: ether address is broadcast for IP address %x!\n",
		    ntohl(isaddr.s_addr));
		goto out;
	}
	if (isaddr.s_addr == myaddr.s_addr) {
		log(LOG_ERR, "%s: %s\n",
			"duplicate IP address!! sent from ethernet address",
			ether_sprintf(&ea->arp_sha));
		itaddr = myaddr;
		if (op == ARPOP_REQUEST)
			goto reply;
		goto out;
	}
	s = splimp();
	ARPTAB_LOOK(at, isaddr.s_addr);
	if (at) {
		ether_copy(&ea->arp_sha, &at->at_enaddr);
		at->at_flags |= ATF_COM;
		if (at->at_hold) {
			sin.sin_family = AF_INET;
			sin.sin_addr = isaddr;
			(*ac->ac_if.if_output)(&ac->ac_if, 
			    at->at_hold, (struct sockaddr *)&sin);
			at->at_hold = 0;
		}
	}
	if (at == 0 && itaddr.s_addr == myaddr.s_addr) {
		/* ensure we have a new table entry */
		if (at = arptnew(&isaddr)) {
			ether_copy(&ea->arp_sha, &at->at_enaddr);
			at->at_flags |= ATF_COM;
		}
	}
	(void) splx(s);
reply:
		/*
		 * Reply if this is an IP request,
		 * or if we want to send a trailer response.
		 */
	if (op != ARPOP_REQUEST)
		goto out;
	if (itaddr.s_addr == myaddr.s_addr) {
		/* I am the target */
		ether_copy(&ea->arp_sha, &ea->arp_tha);
		ether_copy(&ac->ac_enaddr, &ea->arp_sha);
	} else {
		ARPTAB_LOOK(at, itaddr.s_addr);
		if (at == NULL || (at->at_flags & ATF_PUBL) == 0)
			goto out;
		ether_copy(&ea->arp_sha, &ea->arp_tha);
		ether_copy(&at->at_enaddr, &ea->arp_sha);
	}

	ip_copy(ea->arp_spa, ea->arp_tpa);
	ip_copy(&itaddr, ea->arp_spa);
	ea->arp_op = htons(ARPOP_REPLY); 
	eh = (struct ether_header *)sa.sa_data;
	ether_copy(&ea->arp_tha, &eh->ether_dhost);
	eh->ether_type = htons(ETHERTYPE_ARP);
	sa.sa_family = AF_UNSPEC;
	(*ac->ac_if.if_output)(&ac->ac_if, m, &sa);
	return;
out:
	m_freem(m);
	return;
}

/*
 * Free an arptab entry.
 */
arptfree(at)
	register struct arptab *at;
{
	int s = splimp();

	if (at->at_hold)
		m_freem(at->at_hold);
	at->at_hold = 0;
	at->at_timer = at->at_flags = 0;
	at->at_iaddr.s_addr = 0;
	(void) splx(s);
}

/*
 * Enter a new address in arptab, pushing out the oldest entry 
 * from the bucket if there is no room.
 * This always succeeds since no bucket can be completely filled
 * with permanent entries (except from arpioctl when testing whether
 * another permanent entry will fit).
 */
struct arptab *
arptnew(addr)
	struct in_addr *addr;
{
	register n;
	int oldest = -1;
	register struct arptab *at, *ato = NULL;
	static int first = 1;

	if (first) {
		first = 0;
		timeout(arptimer, (caddr_t)0, hz);
	}
	at = &arptab[ARPTAB_HASH(addr->s_addr) * ARPTAB_BSIZ];
	for (n = 0; n < ARPTAB_BSIZ; n++,at++) {
		if (at->at_flags == 0)
			goto out;	 /* found an empty entry */
		if (at->at_flags & ATF_PERM)
			continue;
		if ((int)at->at_timer > oldest) {
			oldest = at->at_timer;
			ato = at;
		}
	}
	if (ato == NULL)
		return (NULL);
	at = ato;
	arptfree(at);
out:
	at->at_iaddr = *addr;
	at->at_flags = ATF_INUSE;
	return (at);
}

arpioctl(cmd, data)
	int cmd;
	caddr_t data;
{
	register struct arpreq *ar = (struct arpreq *)data;
	register struct arptab *at;
	register struct sockaddr_in *sin;
	int s;

	if (ar->arp_pa.sa_family != AF_INET ||
	    ar->arp_ha.sa_family != AF_UNSPEC)
		return (EAFNOSUPPORT);
	sin = (struct sockaddr_in *)&ar->arp_pa;

	s = splimp();
	ARPTAB_LOOK(at, sin->sin_addr.s_addr);
	if (at == NULL) {		/* not found */
		if (cmd != SIOCSARP) {
			(void) splx(s);
			return (ENXIO);
		}
		if (ifa_ifwithnet(&ar->arp_pa) == NULL) {
			(void) splx(s);
			return (ENETUNREACH);
		}
	}

	switch (cmd) {

	case SIOCSARP:		/* set entry */
		if (at == NULL) {
			at = arptnew(&sin->sin_addr);
			if (at == NULL) {
				(void) splx(s);
				return (EADDRNOTAVAIL);
			}
			if (ar->arp_flags & ATF_PERM) {
			/* never make all entries in a bucket permanent */
				register struct arptab *tat;
				
				/* try to re-allocate */
				tat = arptnew(&sin->sin_addr);
				if (tat == NULL) {
					arptfree(at);
					(void) splx(s);
					return (EADDRNOTAVAIL);
				}
				arptfree(tat);
			}
		}
		ether_copy(ar->arp_ha.sa_data, &at->at_enaddr);
		at->at_flags = ATF_COM | ATF_INUSE |
			(ar->arp_flags & (ATF_PERM|ATF_PUBL));
		at->at_timer = 0;
		arpcomflush(sin);
		break;

	case SIOCDARP:		/* delete entry */
		arptfree(at);
		arpcomflush(sin);
		break;

	case SIOCGARP:		/* get entry */
		ether_copy(&at->at_enaddr, ar->arp_ha.sa_data);
		ar->arp_flags = at->at_flags;
		break;
	}
	(void) splx(s);
	return (0);
}

static
arpcomflush(sin)
struct	sockaddr_in	*sin;
{
	struct	ifaddr	*ifa;

	if (sin->sin_family == AF_INET)
		if (ifa = ifa_ifwithnet((struct sockaddr *)sin))
			((struct arpcom *)(ifa->ifa_ifp))->ac_lastip.s_addr =
				INADDR_ANY;
}

/*
 * Flush the entire ARP table. For example, this needs to be done
 * when changing the netmask, for example.
 */
arpflush()
{
	register struct arptab *at;
	
	for (at = arptab; at != &arptab[ARPTAB_SIZE]; at++)
		if ((at->at_flags & ATF_PERM)==0)
			arptfree(at);
}


/*
 * REVerse Address Resolution Protocol (revarp) is used by a diskless
 * client to find out its IP address when all it knows is its Ethernet address.
 */
struct in_addr myaddr;

revarp_myaddr(ifp)
	register struct ifnet *ifp;
{
	register struct sockaddr_in *sin;
	struct ifreq ifr;
	int s;

	/*
	 * We used to give the interface a temporary address,
	 * but now all we need to do is call the init routine
	 * for the interface before we can send or receive packets.
	 */
	bzero((caddr_t)&ifr, sizeof(ifr));
	sin = (struct sockaddr_in *)&ifr.ifr_addr;
	sin->sin_family = AF_INET;
	ifp->if_flags |= IFF_NOTRAILERS | IFF_BROADCAST | IFF_UP;
	s = splimp();
	(*ifp->if_init)(ifp->if_unit);
	(void) splx(s);

	myaddr.s_addr = 0;
	revarp_start(ifp);
	s = splimp();
	while (myaddr.s_addr == 0)
		(void) sleep((caddr_t)&myaddr, PZERO-1);
	(void) splx(s);
	sin->sin_addr = myaddr;
	if (in_control((struct socket*)0, SIOCSIFADDR, (caddr_t)&ifr, ifp))
		printf("revarp: can't set perm inet addr\n");
}

revarp_start(ifp)
	register struct ifnet *ifp;
{
	register struct mbuf *m;
	register struct ether_arp *ea;
	register struct ether_header *eh;
	static int retries = 0;
	struct ether_addr myether;
	struct sockaddr sa;

	if (myaddr.s_addr != 0) {
		if (retries >= 2)
			printf("Found Internet address %x\n", myaddr.s_addr);
		retries = 0;
		return;
	}
	(void) localetheraddr((struct ether_addr *)NULL, &myether);
	if (++retries == 2) {
		printf("revarp: Requesting Internet address for %s\n",
		    ether_sprintf(&myether));
	}
	if ((m = m_get(M_DONTWAIT, MT_DATA)) == NULL)
		panic("revarp: no mbufs");
	m->m_len = sizeof(struct ether_arp);
	m->m_off = MMAXOFF - m->m_len;
	ea = mtod(m, struct ether_arp *);
	bzero((caddr_t)ea, sizeof (*ea));

	sa.sa_family = AF_UNSPEC;
	eh = (struct ether_header *)sa.sa_data;
	ether_copy(&etherbroadcastaddr, &eh->ether_dhost);
	ether_copy(&myether, &eh->ether_shost);
	eh->ether_type = htons(ETHERTYPE_REVARP);

	ea->arp_hrd = htons(ARPHRD_ETHER);
	ea->arp_pro = htons(ETHERTYPE_IP);
	ea->arp_hln = sizeof(ea->arp_sha);	/* hardware address length */
	ea->arp_pln = sizeof(ea->arp_spa);	/* protocol address length */
	ea->arp_op = htons(REVARP_REQUEST);
	ether_copy(&myether, &ea->arp_sha);
	ether_copy(&myether, &ea->arp_tha);
	(*ifp->if_output)(ifp, m, &sa);
	timeout(revarp_start, (caddr_t)ifp, 3*hz);
}

/*
 * Client side Reverse-ARP input
 * Server side is handled by user level server
 */
revarpinput(ac, m)
	register struct arpcom *ac;
	struct mbuf *m;
{
	register struct ether_arp *ea;
	struct ether_addr myether;

	IF_ADJ(m);
	ea = mtod(m, struct ether_arp *);
	if (m->m_len < sizeof *ea)
		goto out;
	if (ac->ac_if.if_flags & IFF_NOARP)
		goto out;
	if (ntohs(ea->arp_pro) != ETHERTYPE_IP)
		goto out;
	if (ntohs(ea->arp_op) != REVARP_REPLY)
		goto out;
	(void) localetheraddr((struct ether_addr *)NULL, &myether);
	if (!ether_cmp(&ea->arp_tha, &myether)) {
		ip_copy(ea->arp_tpa, &myaddr);
		wakeup((caddr_t)&myaddr);
	}
out:
	m_freem(m);
	return;
}

localetheraddr(hint, result)
	struct ether_addr *hint, *result;
{
	static int found = 0;
	static struct ether_addr addr;

	if (!found) {
		found = 1;
		if (hint == NULL)
			return (0);
		addr = *hint;
		printf("Ethernet address = %s\n", ether_sprintf(&addr) );
	}
	if (result != NULL)
		*result = addr;
	return (1);
}

/*
 * Convert Ethernet address to printable (loggable) representation.
 */
char *
ether_sprintf(addr)
	struct ether_addr *addr;
{
        static char etherbuf[18];
	extern char *sprintf();

	return (sprintf(etherbuf, "%x:%x:%x:%x:%x:%x",
	    addr->ether_addr_octet[0], addr->ether_addr_octet[1],
	    addr->ether_addr_octet[2], addr->ether_addr_octet[3],
	    addr->ether_addr_octet[4], addr->ether_addr_octet[5]));
}
