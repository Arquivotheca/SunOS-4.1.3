/*LINTLIBRARY*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 *
 * @(#)if_subr.c 1.1 92/07/30
 */

/*
 * Device independent subroutines used by Ethernet device drivers.
 * Mostly this code takes care of protocols and sockets and
 * stuff.  Knows about Ethernet in general, but is ignorant of the
 * details of any particular Ethernet chip or board.
 */

#include "snit.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <net/netisr.h>
#include <net/if_ieee802.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <net/nit_if.h>

extern	struct ifnet loif;
struct ether_family *ether_families = NULL;

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 * Any hardware-specific initialization is done by the caller.
 */
ether_attach(ifp, unit, name, init, ioctl, output, reset)
	register struct ifnet *ifp;
	int unit;
	char *name;
	int (*init)(), (*ioctl)(), (*output)(), (*reset)();
{

	ifp->if_unit = unit;
	ifp->if_name = name;
	ifp->if_mtu = ETHERMTU;

	ifp->if_init = init;
	ifp->if_ioctl = ioctl;
	ifp->if_output = output;
	ifp->if_reset = reset;
	ifp->if_upper = 0;
	ifp->if_lower = 0;
	if_attach(ifp);
}

address_known(ifp)
	struct ifnet *ifp;
{

	return (ifp->if_addrlist != NULL);
}

/*
 * Add a new ether_family to the list of "interesting" types.
 * The trick with lint is needed because the normal systems never call
 * this routine.  It is only included in binary releases so that other
 * protocols (e.g. XNS) can be configured into a binary release.
 */
ether_register(efp)
	struct ether_family *efp;
{
# ifdef lint
	if (efp != efp) ether_register(efp);
# endif lint
	efp->ef_next = ether_families;
	ether_families = efp;
}

/*
 * This is used from within an ioctl service routine to process the
 * SIOCSIFADDR request.  This should be called at interrupt priority
 * level to prevent races.
 */
set_if_addr(ifp, sa, enaddr)
	register struct ifnet *ifp;
	struct sockaddr *sa;
	struct ether_addr *enaddr;
{
	struct sockaddr_in *sin = (struct sockaddr_in *)sa;

	switch (sa->sa_family) {
	    case AF_UNSPEC:
		/*
		 * Reset Ethernet address: broad or multi-cast not allowed.
		 * Then send an ARP so caches can be flushed.
		 */
		if (sa->sa_data[0] & 1)
			return (EINVAL);

		ether_copy(sa->sa_data, enaddr);
		break;

#ifdef INET
	    case AF_INET:
		((struct arpcom *)ifp)->ac_ipaddr = sin->sin_addr;
		break;
#endif INET
	}

	/*
	 * The "IFF_UP" flag is included here only for backward
	 * compatibility.  That is, people assumed that once you
	 * set the address of an Ethernet controller, it should
	 * be "up".
	 */
	ifp->if_flags |= IFF_UP|IFF_BROADCAST;
	(*ifp->if_init)(ifp->if_unit);
	if (sin->sin_addr.s_addr) {
		(void) arpwhohas((struct arpcom *)ifp,
			&(((struct arpcom *)ifp)->ac_ipaddr));
	}
	return (0);
}

/*
 * Ethernet output routine.
 * Encapsulate a packet of type family for the local net.
 * If destination is this address or broadcast, send packet to
 * input side to kludge around the fact that interfaces can't
 * talk to themselves.
 */
ether_output(ifp, m0, dst, startout)
	register struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
	int (*startout)(); /* Hardware-specific routine to start output */
{
	int type, s;
	register struct mbuf *m = m0;
	register struct ether_header *header;
	register struct ether_family *efp;
	struct mbuf *mcopy = (struct mbuf *)0;		/* Null */
	struct ether_addr edst;

#define	ap ((struct arpcom *)ifp)

	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING)) {
		m_freem(m0);
		return (ENETDOWN);
	}

	switch (dst->sa_family) {

#ifdef	INET
	case AF_INET:
		s = splimp();
		if (ap->ac_lastip.s_addr !=
		    ((struct sockaddr_in *)dst)->sin_addr.s_addr) {
			ap->ac_lastip = ((struct sockaddr_in *)dst)->sin_addr;
			if (!arpresolve(ap, m)) {
				ap->ac_lastip.s_addr = INADDR_ANY;
				(void) splx(s);
				return (0);	/* if not yet resolved */
			}
		}
		ether_copy(&ap->ac_lastarp, &edst);
		type = htons(ETHERTYPE_IP);
		(void) splx(s);
		break;
#endif	INET

	case AF_UNSPEC:
		header = (struct ether_header *)dst->sa_data;
		ether_copy(&header->ether_dhost, &edst);
		type = header->ether_type;	/* already in net order */
		break;

	default:
		/*
		 * If it was not one of our "native" address families,
		 * check the extended family table to see if anyone else
		 * knows how to send it, and let them tell us.
		 */
		for (efp = ether_families; efp != NULL; efp = efp->ef_next) {
			if (efp->ef_family == dst->sa_family)
				break;
		}
		if (efp != NULL && efp->ef_outfunc != NULL) {
			if ((*efp->ef_outfunc)(dst, m, ap, &edst))
				return (0);
			if (efp->ef_ethertype == EF_8023_TYPE) {
				register struct mbuf *m2;

				type = 0;
				for (m2 = m; m2; m2 = m2->m_next)
					type += m2->m_len;
				type = htons((u_short) type);
			} else
				type = htons(efp->ef_ethertype);
			break;
		}
		identify(ifp);
		printf("can't handle AF 0x%x\n", dst->sa_family);
		m_freem(m0);
		return (EAFNOSUPPORT);
	}

	/*
	 * Most Ethernet devices cannot hear themselves.  So we make
	 * a local copy to send back to ourselves if broadcasting.
	 */
	if (!ether_cmp(&edst, &etherbroadcastaddr))
		mcopy = m_copy(m, 0, M_COPYALL);
	/*
	 * Add local net header.  If no space in first mbuf,
	 * allocate another.
	 */
	if (m->m_off > MMAXOFF ||
	    MMINOFF + sizeof (struct ether_header) > m->m_off) {
		m = m_get(M_DONTWAIT, MT_HEADER);
#ifdef DEBUG
		chkmbuf(m);
#endif DEBUG
		if (m == 0) {
			m_freem(m0);
			m_freem(mcopy);
			ether_error(ifp, "WARNING: no mbufs\n");
			return (ENOBUFS);
		}
		m->m_next = m0;
		m->m_len = sizeof (struct ether_header);
	} else {
		m->m_off -= sizeof (struct ether_header);
		m->m_len += sizeof (struct ether_header);
	}
	header = mtod(m, struct ether_header *);
	ether_copy(&edst, &header->ether_dhost);
	header->ether_type = type;

	/*
	 * Send a copy of broadcasts to our input side, now that we have
	 * the header, but before it is sent away to the hardware.
	 */
	if (mcopy) {
		struct mbuf *m1;
		int wirelen;

		for (m1 = mcopy, wirelen = 0; m1; m1 = m1->m_next)
			wirelen += m1->m_len;
		ether_copy(&ap->ac_enaddr, &header->ether_shost);
		do_protocol(header, mcopy, ap, wirelen);
		mcopy = (struct mbuf *) 0;
	}

	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */
	s = splimp();
	if (IF_QFULL(&ifp->if_snd)) {
#ifdef DEBUG
		chkmbuf(m);
#endif DEBUG
		IF_DROP(&ifp->if_snd);
		random_drop(ifp);
	}
	IF_ENQUEUE(&ifp->if_snd, m);
	(*startout)(ifp->if_unit);
	(void) splx(s);
	return (0);
#undef ap
}

/*
 * Drop a random packet when congestion noticed
 * (Instead of the current one)
 */
random_drop(ifp)
	register struct ifnet *ifp;
{
	register int victim;
	register struct mbuf *mm, *m;

	victim = time.tv_usec;
	victim ^= time.tv_sec;
	victim %= ifp->if_snd.ifq_maxlen;
	if (victim == 0) {
		/*
		 * Easy - drop head of the queue
		 */
		IF_DEQUEUE(&ifp->if_snd, m);
		m_freem(m);
		return;
	}
	victim--;
	/*
	 * The subtraction of one above is because we want mm to point
	 * to one packet BEFORE the one that we want to drop.
	 */
	mm = ifp->if_snd.ifq_head;
	while (mm != NULL && (victim-- > 0))
			mm = mm->m_act;
	if (mm == NULL)
		return;
	m = mm->m_act;
	if (m == NULL)
		return;
	mm->m_act = m->m_act;
	if (m == ifp->if_snd.ifq_tail)
		ifp->if_snd.ifq_tail = mm;
	m_freem(m);
	ifp->if_snd.ifq_len--;
}

/*
 * Prints the name of the interface, i.e. ec0.  Doing this in a
 * subroutine instead of in-line makes several error printf's
 * not have to have the name of the interface hardwire into the
 * printf format string.  Since errors should be infrequent,
 * speed is not an issue.
 */
identify(ifp)
	struct ifnet *ifp;
{

	printf("%s%d: ", ifp->if_name, ifp->if_unit);
}

/*
 * Print an error message, but not too often
 */
/*VARARGS1*/
ether_error(ifp, fmt, a1, a2, a3, a4, a5, a6)
	struct ifnet *ifp;
	char *fmt;
	char *a1, *a2, *a3, *a4, *a5, *a6;
{
	static long last;		/* time of last error message */
	static char *lastmsg;		/* pointer to last message text */

	if (last == (time.tv_sec & ~1) && lastmsg == fmt)
		return;
	identify(ifp);
	printf(fmt, a1, a2, a3, a4, a5, a6);
	last = (time.tv_sec & ~1);
	lastmsg = fmt;
}

/*
 * Checks to see if the packet is a trailer packet.  If so, the
 * offset of the trailer (after the type and remaining length fields)
 * is written into offout and length is changed to reflect the length
 * of the trailer packet (minus the the type and remaining length fields).
 * If it is not a trailer packet, 0 is written into offout and length
 * is left unchanged.
 *
 * The return value is 0 if there is no error, 1 if there is an error.
 */
check_trailer(header, buffer, length, offout)
	struct ether_header *header;
	caddr_t buffer;
	int *length, *offout;
{
	register int off, resid;
	u_short type;

	/*
	 * Deal with trailer protocol: if type is trailer type
	 * get true type from first 16-bit word past data.
	 * Remember that type was trailer by setting off.
	 */
	*offout = 0;
	type = ntohs(header->ether_type);
	if (type >= ETHERTYPE_TRAIL &&
	    type < ETHERTYPE_TRAIL + ETHERTYPE_NTRAILER) {
		off = (type - ETHERTYPE_TRAIL) << 9;
		/*
		 * Originally had a check for off >= ETHERMTU
		 * for "sanity", but on FDDI packets can be larger.
		 * assume the above check on NTRAILER is good enough.
		 */
		header->ether_type = *(u_short *)((char *)(buffer + off));
		resid = ntohs(*(u_short *)((char *)(buffer+off+2)));
		/*
		 * Can  off + resid  ever be  < length?  If not,
		 * the test should be for  !=  instead of  >, and
		 * *length could be left alone.
		 */
		if (off + resid > *length)
			return (1);		/* sanity */

		/*
		 * Off is now the offset to the start of the
		 * trailer portion, which includes 2 shorts that were
		 * added.  The 2 shorts will be removed by copy_to_mbufs.
		 * The 2 extra shorts contain the real type field and the
		 * length of the trailer.
		 */
		*length = off + resid;
		*offout = off;
	}
	return (0);
}

/*
 * Add the multicast address specified in ifr to the list associated
 * with ap.  Return 0 if successful, errno value otherwise.
 *
 * Note that each multicast address has a reference count.
 */
int
addmultiaddr(ap, ifr)
	register struct arpcom	*ap;
	struct ifreq		*ifr;
{
	register struct mcaddr	*mca;
	register struct mcaddr	*mclimit = &ap->ac_mcaddr[ap->ac_nmcaddr];

	/*
	 * Allocate the array if it doesn't yet exist.
	 */
	if (ap->ac_nmcaddr == 0 && ap->ac_mcaddr == NULL) {
		ap->ac_mcaddr = (struct mcaddr *)
			new_kmem_alloc((u_int)(MCADDRMAX * sizeof *mca),
					KMEM_NOSLEEP);
		if (ap->ac_mcaddr == NULL)
			return (ENOMEM);
	}

	/*
	 * If the address is already in the table
	 * then just increment the reference count.
	 */
	for (mca = ap->ac_mcaddr; mca < mclimit; mca++)
		if (!ether_cmp((caddr_t)ifr->ifr_addr.sa_data,
					&mca->mc_enaddr)) {
			if (mca->mc_count >= MCCOUNTMAX)
				return (EADDRNOTAVAIL);
			mca->mc_count++;
			return (0);
		}

	/*
	 * Not already in the table - add a new entry.
	 */

	if (ap->ac_nmcaddr >= MCADDRMAX)
		return (EADDRNOTAVAIL);

	mca = &ap->ac_mcaddr[ap->ac_nmcaddr++];
	ether_copy(ifr->ifr_addr.sa_data, &mca->mc_enaddr);
	mca->mc_count = 1;

	return (0);
}

/*
 * Remove the multicast address specified in ifr from the list associated
 * with ap.  Return 0 if successful, errno value otherwise.  The case
 * where the address wasn't in the list is considered erroneous.
 */
int
delmultiaddr(ap, ifr)
	register struct arpcom	*ap;
	struct ifreq		*ifr;
{
	register caddr_t	target;
	register struct mcaddr	*mca;
	register struct mcaddr	*mclimit = &ap->ac_mcaddr[ap->ac_nmcaddr];

	if (ap->ac_nmcaddr == 0)
		return (EADDRNOTAVAIL);

	target = (caddr_t)ifr->ifr_addr.sa_data;
	for (mca = ap->ac_mcaddr; mca < mclimit; mca++)
		if (!ether_cmp((caddr_t)&mca->mc_enaddr, target))
			/*
			 * Found it.  If the reference count is > 1 then
			 * just decrement reference count and we're done,
			 * otherwise remove the mcaddr entry by sliding the
			 * remainder of the multicast address array down one
			 * slot.
			 */
			if (mca->mc_count > 1) {
				mca->mc_count--;
				break;
			} else {
				register u_int	numbeyond = mclimit - mca - 1;

				if (numbeyond != 0)
					ovbcopy((caddr_t)(mca + 1),
						(caddr_t)mca,
						numbeyond * sizeof *mca);
				ap->ac_nmcaddr--;
				break;
			}

	return (mca == mclimit ? EADDRNOTAVAIL : 0);
}

/*
 * Common routine for passing packets from interfaces to protocol input
 * queues.  Also handles shunting packets off to the Network Interface
 * Tap (NIT) code.
 *
 * XXX:	Interfaces with multicasting turned on may pass us packets that
 *	they shouldn't really have received at all.  (The LANCE driver
 *	is an example of such an interface -- it matches against a hash
 *	filter, but can get spurious matches and doesn't check further.)
 *	Should we check here and reject spurious multicast packets?  It's
 *	tempting to do so, since we have to do this kind of checking anyway
 *	for NIT.
 */
do_protocol(header, m, ap, wirelen)
	register struct ether_header *header;
	register struct mbuf *m;
	struct arpcom *ap;
	int wirelen;
{
	register struct ifqueue *inq;
	int s;
	struct nit_if	nif;
	int	promisc;

	/*
	 * Set promisc nonzero if we received this packet only because
	 * the interface happened to be in promiscuous mode.  Start by
	 * checking against our Ethernet address and the broadcast address,
	 * then check against all currently active multicast addresses.
	 */
	promisc =
	    ether_cmp(&header->ether_dhost, &ap->ac_enaddr) &&
	    ether_cmp(&header->ether_dhost, &etherbroadcastaddr);

	/*
	 * If the promisc bit is set and the dest addr is a multicast
	 * then compare the dest addr with all registered multicast addrs.
	 */
	if (promisc && header->ether_dhost.ether_addr_octet[0] & 1) {
		register struct	mcaddr	*mca;
		register struct mcaddr	*mclimit =
						&ap->ac_mcaddr[ap->ac_nmcaddr];

		for (mca = ap->ac_mcaddr; mca < mclimit; mca++)
			if (!ether_cmp(&header->ether_dhost, &mca->mc_enaddr)) {
				promisc = 0;
				break;
			}
	}

	/*
	 * Fill in header information that the various NIT
	 * implementations require. Protect against races by raising
	 * to splimp here (snit_intr and ENQUEUE(inq) require it).
	 */
	s = splimp();
#if	NSNIT > 0
	nif.nif_header = (caddr_t)header;
	nif.nif_hdrlen = sizeof *header;
	nif.nif_bodylen = wirelen;
	nif.nif_promisc = promisc;
	snit_intr(&ap->ac_if, m, &nif);
#endif	NSNIT > 0
	/*
	 * If the promisc bit is set, the packet wasn't destined
	 * for us; after we send it to NIT, get rid of it.
	 */
	if (promisc) {
		m_freem(m);
		(void) splx(s);
		return;
	}

	/*
	 * If there is no room for the IFP in the first mbuf, then
	 * tack another one on the front; then stuff the ifnet pointer
	 * into it.
	 */
	if (m->m_off > MMAXOFF ||
	    MMINOFF + sizeof (struct ifnet *) > m->m_off) {
		register struct mbuf *m1;

		MGET(m1, M_DONTWAIT, MT_IFADDR);
		if (m1 == 0) {
			m_freem(m);
			(void) splx(s);
			return;
		}
		m1->m_next = m;
		m1->m_len = sizeof (struct ifnet *);
		m = m1;
	} else {
		m->m_off -= sizeof (struct ifnet *);
		m->m_len += sizeof (struct ifnet *);
	}
	* mtod(m, struct ifnet **) = (struct ifnet *)ap;

	/*
	 * Shunt the packet off to the appropriate destination.
	 */
	switch (ntohs(header->ether_type)) {
#ifdef	INET
	case ETHERTYPE_IP:
		schednetisr(NETISR_IP);
		inq = &ipintrq;
		break;

	case ETHERTYPE_ARP:
		arpinput(ap, m);
		(void) splx(s);
		return;

	case ETHERTYPE_REVARP:
		revarpinput(ap, m);
		(void) splx(s);
		return;
#endif	INET

	default:
	    {
		/*
		 * If it was not one of our "native" Ethernet types,
		 * check the extended family table to see if anyone else
		 * is potentially interested, and give them a chance at it.
		 */
		register struct ether_family *efp;

		for (efp = ether_families; efp != NULL; efp = efp->ef_next) {
			if (efp->ef_ethertype == ntohs(header->ether_type))
				break;
		/*
		 * Check for IEEE802.3 packet (as opposed to Ethernet packet).
		 * The two types of packet headers differ only in the last two
		 * bytes.  Ethernet interprets them as a type field, 802.3 as
		 * a length field.  The maximum legal length for an 802.3
		 * packet is MAX_8023_DLEN, which happens to be less than all
		 * supported type values.
		 *
		 * If it is a potential 802.3 packet, check the extended
		 * family table to see if anyone has registered for the
		 * 802.3 special "ethertype".
		 */
			if (ntohs(header->ether_type) <= MAX_8023_DLEN &&
			    efp->ef_ethertype == EF_8023_TYPE)
				break;
		}
		if (efp != NULL && efp->ef_infunc != NULL) {
			inq =  (*efp->ef_infunc)(ap, m, header);
			if (inq == NULL) {
				(void) splx(s);
				return;
			}
			if (efp->ef_netisr != NULL)
				schednetisr(efp->ef_netisr);
			break;
		}
		/*
		 * If we fall through to here, the packet is not
		 * recognized; toss it.
		 */
		m_freem(m);
		(void) splx(s);
		return;
	    }
	}

	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		m_freem(m);
		(void) splx(s);
		return;
	}
	IF_ENQUEUE(inq, m);
	(void) splx(s);
}

/*
 * Enough bytes so that the network drivers won't need
 * to allocate another mbuf near the front of the chain
 * if this mbuf chain ends up being routed.
 */
#define	ETHER_PRE_PAD	24

/*
 * Routine to copy from a buffer belonging to an interface into mbufs.
 *
 * The buffer consists of a header and a body.  In the normal case,
 * the buffer starts with the header, which is followed immediately
 * by the body.  However, the buffer may contain a trailer packet.
 * In this case, the body appears first and is followed by an expanded
 * header that starts with two additional shorts (containing a type and
 * a header length) and finishes with a header identical in format to
 * that of the normal case.
 *
 * These cases are distinguished by off0, which is the offset to the
 * start of the header part of the buffer.  If nonzero, then we have a
 * trailer packet and must start copying from the beginning of the
 * header as adjusted to skip over its extra leading fields.  Totlen is
 * the overall length of the buffer, including the contribution (if any)
 * of the extra fields in the trailing header.
 */
struct mbuf *
copy_to_mbufs(buffer, totlen, off0)
	caddr_t buffer;
	int totlen, off0;
{
	register caddr_t	cp = buffer;
	caddr_t			cplim = cp + totlen;
	register int		len;
	register struct mbuf	*m;
	struct mbuf		*top = (struct mbuf *) 0,
				**mp = &top;
	int firstime = 1;

	if (off0) {
		/*
		 * Trailer: adjust starting postions and lengths
		 * to start of header.
		 */
		cp += off0 + 2 * sizeof (short);
		totlen -= 2 * sizeof (short);
	}

	while (totlen > 0) {
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == (struct mbuf *) 0) {
			m_freem(top);
			return ((struct mbuf *) 0);
		}

		/*
		 * If the copy has proceeded past the end of buffer,
		 * move back to the start.  This occurs when the copy
		 * of the header portion of a trailer packet completes.
		 */
		if (cp >= cplim) {
			cp = buffer;
			cplim = cp + off0;
		}

		/*
		 * Policy:
		 * Always start the chain with a regular mbuf
		 * leaving ETHER_PRE_PAD space in front, then copy
		 * the remaining data into [ cluster mbufs then ]
		 * regular mbufs until done.
		 */
		len = cplim - cp;
		if (firstime) {
			m->m_off += ETHER_PRE_PAD;
			m->m_len = MLEN - ETHER_PRE_PAD;
			firstime = 0;
		} else if (len < (MLEN * 2) || mclget(m) == 0)
			m->m_len = MLEN;

		m->m_len = MIN(m->m_len, len);
		len = m->m_len;

		bcopy(cp, mtod(m, caddr_t), (u_int) len);
		cp += len;
		totlen -= len;

		*mp = m;
		mp = &m->m_next;
	}
	return (top);
}

/*
 * Routine to copy from mbuf chain to a transmit buffer.
 * Returns the number of bytes copied
 */
copy_from_mbufs(buffer, m)
	register u_char *buffer;
	struct mbuf *m;
{
	register struct mbuf *mp;
	register int off;

	for (off = 0, mp = m; mp; mp = mp->m_next) {
		register u_int len = mp->m_len;
		u_char *mcp;

		if (len != 0) {
			mcp = mtod(mp, u_char *);
			bcopy((caddr_t)mcp, (caddr_t)buffer, len);
			off += len;
			buffer += len;
		}
	}
	m_freem(m);
	return (off);
}

#define	ETHERFRAME	60

/*
 * A close relative of m_pullup, differing from it in that:
 *    -	It is not an error for the aggregate length of the subject
 *	mbuf chain to be less than the length requested.
 *    -	Where possible (and convenient), it avoids allocating a
 *	fresh mbuf, instead using the first mbuf of the subject chain.
 */
struct mbuf *
ether_pullup(m0, len)
	struct mbuf *m0;
	register int len;
{
	register struct mbuf *m, *n;
	register int count;

	n = m0;
	if (len > MLEN)
		goto bad;
	/*
	 * See whether we can optimize by avoiding mbuf allocation.
	 * If already large enough, we need not do anything. Otherwise,
	 * check for enough room in first mbuf, or needing a copy.
	 */
	if (n->m_len >= len) {
		return (n);
	}

	if ((n->m_next == NULL) && (n->m_off + ETHERFRAME < MMAXOFF))
		return (n);

	if ((n->m_off + len < MMAXOFF) && (n->m_off + ETHERFRAME < MMAXOFF)) {
		m = n;
		n = n->m_next;
		if (! n) {
			return (m);
		}
		len -= m->m_len;
	} else {
		MGET(m, M_DONTWAIT, n->m_type);
		if (m == 0)
			goto bad;
		m->m_len = 0;
	}
	/*
	 * M names the mbuf we're filling.  N moves through
	 * the mbufs that are drained into m.
	 */
	do {
		count = MIN(MLEN - m->m_len, len);
		if (count > n->m_len)
			count = n->m_len;
		if (count > 0) {
			bcopy(mtod(n, caddr_t), mtod(m, caddr_t)+m->m_len,
			    (unsigned)count);
			len -= count;
			m->m_len += count;
			n->m_len -= count;
		}
		if (n->m_len > 0) {	/* bug 1010218 */
			n->m_off += count;
			break;
		}
		n = m_free(n);
	} while (n);
	m->m_next = n;
	return (m);
bad:
	m_freem(n);
	return ((struct mbuf *) 0);
}

/*
 * Return the count of attached ifnet's excluding loopback.
 */
ifcount()
{
	register	struct ifnet *p;
	int	n = 0;

	for (p = ifnet; p; p = p->if_next)
		if (strcmp(p->if_name, "lo") != 0)
			n++;
	return (n);
}

#ifdef	DEBUG
dumpbuffer(header, buffer)
	struct ether_header *header;
	caddr_t buffer;
{
	int i, j;
	short *buf = (short *)buffer;
	int length = buf[1];  /* Specific to IP packets */

	length /= 2;  /* convert to words */
	if (length > 48)
		length = 48;

	if (header->ether_dhost.ether_addr_octet[0] != 0xff) {
		printf("\nSource: ");
		for (i = 0; i < 6; i++)
			printf("%x ",
			    header->ether_shost.ether_addr_octet[i]&0xff);
		printf("\nDestin: ");
		for (i = 0; i < 6; i++)
			printf("%x ",
			    header->ether_dhost.ether_addr_octet[i]&0xff);
		printf("\nBuffer");
		j = 0;
		for (i = 0; i < length; i++) {
			if (j == 0)
				printf("\n");
			if (++j == 16)
				j = 0;
			printf("%x ", buf[i]&0xffff);
		}
		printf("\n");
	} else
		printf(" Broadcast ");
}

chkmbuf()
{
}
#endif	DEBUG
