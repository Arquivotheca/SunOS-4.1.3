#ifndef lint
static	char sccsid[] = "@(#)if_en.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "en.h"

/*
 * Sun 3Mb Ethernet interface driver.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/vmmac.h>
#include <net/netisr.h>
#include <sys/errno.h>

#include <sundev/mbvar.h>
#include <sunif/if_en.h>
#include <sunif/if_enreg.h>

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>

#include <netpup/pup.h>

#define	ENMTU	(1024+512)
#define	ENMRU	(1024+512+16)		/* 16 is enough to receive trailer */

int	enprobe(), enattach(), enintr();
struct	mb_device *eninfo[NEN];
struct	mb_driver endriver = {
	enprobe, 0, enattach, 0, 0, enintr,
	0, "en", eninfo, 0, 0, 0,
};
#define	ENUNIT(x)	minor(x)

int	eninit(),enoutput(),enreset();

/*
 * Stanford will be the only site running SUNs on 3mb
 * and since they have other TCPs that don't understand
 * trailers (BBN and MIT), disable trailer output.
 * The alternative would be to add a trailer negotiation
 * similar to the 10mb ARP.
 */
int	entrailers = 0;

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * es_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
struct	en_softc {
	struct	ifnet es_if;		/* network-visible interface */
	short	es_oactive;		/* is output active? */
} en_softc[NEN];

/*
 * Probe address to determine interface presence
 */
enprobe(reg)
	caddr_t reg;
{

	if (peek(reg) < 0)
		return (0);
	return (sizeof (struct endevice));
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
enattach(md)
	struct mb_device *md;
{
	register struct en_softc *es = &en_softc[md->md_unit];
	register struct sockaddr_in *sin;

	es->es_if.if_unit = md->md_unit;
	es->es_if.if_name = "en";
	es->es_if.if_mtu = ENMTU;
	es->es_if.if_net = md->md_flags;
	es->es_if.if_host[0] =
	 (((struct endevice *)eninfo[md->md_unit]->md_addr)->en_filter) & 0xff;
	sin = (struct sockaddr_in *)&es->es_if.if_addr;
	sin->sin_family = AF_INET;
	sin->sin_addr = if_makeaddr(es->es_if.if_net, es->es_if.if_host[0]);
	es->es_if.if_init = eninit;
	es->es_if.if_output = enoutput;
	es->es_if.if_reset = enreset;
	if_attach(&es->es_if);
}

/*
 * Reset of interface after Multibus reset.
 */
enreset(unit)
	int unit;
{
	register struct mb_device *md;

	if (unit >= NEN || (md = eninfo[unit]) == 0 || md->md_alive == 0)
		return;
	printf(" en%d", unit);
	eninit(unit);
}

/*
 * Initialization of interface; clear recorded pending
 * operations, and reinitialize UNIBUS usage.
 */
eninit(unit)
	int unit;
{
	struct en_softc *es = &en_softc[unit];
	register struct mb_device *md = eninfo[unit];
	register struct endevice *addr;
	register int i;
	int s;
	register struct ifnet *ifp = &es->es_if;
	register struct sockaddr_in *sin;

	sin = (struct sockaddr_in *)&ifp->if_addr;
	if (sin->sin_addr.s_addr == 0)	/* if address still unknown */
		return;
	ifp->if_net = in_netof(sin->sin_addr);
	ifp->if_host[0] = in_lnaof(sin->sin_addr);
	sin = (struct sockaddr_in *)&ifp->if_broadaddr;
	sin->sin_family = AF_INET;
	sin->sin_addr = if_makeaddr(ifp->if_net, INADDR_ANY);
	ifp->if_flags = IFF_BROADCAST;

	addr = (struct endevice *)md->md_addr;
	addr->en_status = EN_INIT|EN_LOOPBACK|EN_NOT_RIE|EN_NOT_TIE|EN_NOT_FILTER;
	for (i = 0; i < 256; i++)
		addr->en_filter = i;
	addr->en_status = EN_INIT|EN_LOOPBACK|EN_NOT_RIE|EN_NOT_TIE;
	addr->en_filter = es->es_if.if_host[0];
	addr->en_filter = 0;

	/*
	 * Enable interrupts and start output.
 	 */
	s = splimp();
	addr->en_status = ((~md->md_intpri)&7)<<8;	/* enable interrupts */
	es->es_if.if_flags |= IFF_UP;
	enstart(unit);				/* unit == dev */
	(void) splx(s);
	if_rtinit(&es->es_if, RTF_UP);
}

/*
 * Start output on interface.
 * Get another datagram to send off of the
 * interface queue, and send it.
 */
enstart(dev)
	dev_t dev;
{
        int unit = ENUNIT(dev);
	struct mb_device *md = eninfo[unit];
	register struct en_softc *es = &en_softc[unit];
	register struct endevice *addr;
	register u_short *wp;
	register struct mbuf *m, *m0;
	int len;


	/*
	 * Dequeue a request and send it.
	 */
	IF_DEQUEUE(&es->es_if.if_snd, m0);
	if (m0 == 0) {
		es->es_oactive = 0;
		return;
	}
	addr = (struct endevice *)md->md_addr;
	for (len = 0, m = m0; m; m = m->m_next)
		len += m->m_len;
	len = (len + 1) >> 1;		/* make it words */
	addr->en_data = len;
	/*
	 * THIS LOOP WILL NOT HANDLE MULTIPLE MBUFS IF
	 * ANY BUT THE LAST HAVE ODD LENGTHS.
	 */
	for (m = m0; m; m = m->m_next)
		for (len = (m->m_len+1)>>1, wp = mtod(m, u_short *); len; len--)
			addr->en_data = *wp++;
	m_freem(m0);
	es->es_oactive = 1;
}

struct	sockaddr_pup pupsrc = { AF_PUP };
struct	sockaddr_pup pupdst = { AF_PUP };
struct	sockproto pupproto = { PF_PUP };
/*
 * Ethernet interface interrupt routine.
 * If input error just drop packet.
 * Otherwise examine packet to determine type.
 * If can't determine length
 * from type, then have to drop packet.  Othewise decapsulate
 * packet based on type and pass to type specific higher-level
 * input routine.
 */
enintr()
{
	register struct en_softc *es;
	register struct endevice *addr;
    	struct mbuf *m, *p;
	u_short *wp;
	int unit, len, plen, type;
	struct ifqueue *inq;
	int status, off, serviced = 0, w;

	for (unit = 0; unit < NEN; unit++) {
	if (eninfo[unit]->md_alive == 0)
		continue;
	es = &en_softc[unit];
	addr = (struct endevice *)eninfo[unit]->md_addr;

	if (addr->en_status & EN_RINTR) {
	serviced++;
	w = addr->en_data;	/* throw away read-ahead */
	while (((status = addr->en_data) & EN_QEMPTY) == 0) {
		es->es_if.if_ipackets++;
		MGET(m, M_DONTWAIT, MT_DATA);
		if ((status & EN_ERROR) || m == 0) {
			if (status & (EN_CRC_ERROR|EN_OVERFLOW))
				es->es_if.if_ierrors++;
			if (status & EN_COLLISION)
				es->es_if.if_collisions++;
flush:
			for (len = (status & EN_COUNT); len; len--)
				w = addr->en_data;
			if (m)
				MFREE(m, p);
			break;	/* out of while !QEMPTY */
		}
		len = (status & EN_COUNT) - 3;	/* dstsrc, type, crc */
		len <<= 1;			/* make it bytes */
		if (len > ENMRU || len <= 1) {
			es->es_if.if_ierrors++;
			goto flush;
		}
		if (len > MLEN) {
			/*
			 * If there is more data than will fit in a
			 * single mbuf put it in an mbuf cluster.  We
			 * assume (know) ENMRU < CLBYTES so we'll only
			 * need one cluster.
			 */
			if (mclbytes < len)
				panic("en: mclbytes too small");
			if (mclget(m) == 0)
				goto flush;
		} else
			m->m_off = MMINOFF;
		m->m_len = len;
		w = addr->en_data;
		type = addr->en_data;
		/*
		 * Calculate input data length.
		 * Get pointer to ethernet header.
		 * Deal with trailer protocol: if type is PUP trailer
		 * get true type from first 16-bit word past data.
		 */
		if (entrailers && type >= ENPUP_TRAIL &&
		    type < ENPUP_TRAIL+ENPUP_NTRAILER) {
			off = len - (type-ENPUP_TRAIL)*512 - 2*sizeof(u_short);
			wp = (u_short *)(mtod(m, u_char *) + off);
			len = (type - ENPUP_TRAIL) * 512;
			if (len > ENMRU)
				goto flush;	/* sanity */
			len >>= 1;
			while (len--)
				*wp++ = addr->en_data;
			type = addr->en_data;
			w = addr->en_data - 2*sizeof(u_short);
			if (w != off)
				printf("trailer length mismatch, w %d off %d\n", w, off);
			wp = mtod(m, u_short *);
			if (off > ENMRU)
				goto flush;	/* sanity */
			off >>= 1;
			while (off--)
				*wp++ = addr->en_data;
		} else {
			wp = mtod(m, u_short *);
			len >>= 1;
			while (len--)
				*wp++ = addr->en_data;
		}
		w = addr->en_data;		/* throw away CRC */

		switch (type) {

#ifdef INET
		case ENPUP_IPTYPE:
			schednetisr(NETISR_IP);
			inq = &ipintrq;
			break;
#endif
#ifdef PUP
		case ENPUP_PUPTYPE: {
			struct pup_header *pup = mtod(m, struct pup_header *);

			pupproto.sp_protocol = pup->pup_type;
			pupdst.spup_addr = pup->pup_dport;
			pupsrc.spup_addr = pup->pup_sport;
			raw_input(m, &pupproto, (struct sockaddr *)&pupsrc,
			  (struct sockaddr *)&pupdst);
			goto next;
		}
#endif
		default:
			m_freem(m);
			goto next;
		}

		if (IF_QFULL(inq)) {
			IF_DROP(inq);
			m_freem(m);
		} else
			IF_ENQUEUE(inq, m);
	} /* end of "while !QEMPTY" */
	} /* end of "if RINTR" */

next:
	if (addr->en_status & EN_TINTR) {
		w = addr->en_filter;		/* clear interrupt */
		serviced++;
		if (es->es_oactive == 0)
			continue;
		es->es_oactive = 0;
		if (addr->en_status & EN_TIMEOUT) {
			es->es_if.if_oerrors++;
			continue;
		}
		es->es_if.if_opackets++;
		if (es->es_if.if_snd.ifq_head == 0)
			continue;
		enstart(unit);
	}
	} /* end "for unit" */
	return (serviced);
}

/*
 * Ethernet output routine.
 * Encapsulate a packet of type family for the local net.
 * Use trailer local net encapsulation if enough data in first
 * packet leaves a multiple of 512 bytes of data in remainder.
 */
enoutput(ifp, m0, dst)
	struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
{
	int type, dest, s, error;
	register struct mbuf *m = m0;
	register struct en_header *en;
	register int off;

	switch (dst->sa_family) {

#ifdef INET
	case AF_INET:
		dest = ((struct sockaddr_in *)dst)->sin_addr.s_addr;
		dest &= 0xff;
		off = ntohs((u_short)mtod(m, struct ip *)->ip_len) - m->m_len;
		if (entrailers && off > 0 && (off & 0x1ff) == 0 &&
		    m->m_off >= MMINOFF + 2 * sizeof (u_short)) {
			type = ENPUP_TRAIL + (off>>9);
			m->m_off -= 2 * sizeof (u_short);
			m->m_len += 2 * sizeof (u_short);
			*mtod(m, u_short *) = ENPUP_IPTYPE;
			*(mtod(m, u_short *) + 1) = m->m_len;
			goto gottrailertype;
		}
		type = ENPUP_IPTYPE;
		off = 0;
		goto gottype;
#endif
#ifdef PUP
	case AF_PUP:
		dest = ((struct sockaddr_pup *)dst)->spup_addr.pp_host;
		type = ENPUP_PUPTYPE;
		off = 0;
		goto gottype;
#endif

	default:
		printf("en%d: can't handle af%d\n", ifp->if_unit,
			dst->sa_family);
		error = EAFNOSUPPORT;
		goto bad;
	}

gottrailertype:
	/*
	 * Packet to be sent as trailer: move first packet
	 * (control information) to end of chain.
	 */
	while (m->m_next)
		m = m->m_next;
	m->m_next = m0;
	m = m0->m_next;
	m0->m_next = 0;
	m0 = m;

gottype:
	/*
	 * Add local net header.  If no space in first mbuf,
	 * allocate another.
	 */
	if (m->m_off > MMAXOFF ||
	    MMINOFF + sizeof (struct en_header) > m->m_off) {
		m = m_get(M_DONTWAIT, MT_DATA);
		if (m == 0) {
			error = ENOBUFS;
			goto bad;
		}
		m->m_next = m0;
		m->m_off = MMINOFF;
		m->m_len = sizeof (struct en_header);
	} else {
		m->m_off -= sizeof (struct en_header);
		m->m_len += sizeof (struct en_header);
	}
	en = mtod(m, struct en_header *);
	en->en_shost = ifp->if_host[0];
	en->en_dhost = dest;
	en->en_type = type;

	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */
	s = splimp();
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		error = ENOBUFS;
		goto qfull;
	}
	IF_ENQUEUE(&ifp->if_snd, m);
	if (en_softc[ifp->if_unit].es_oactive == 0)
		enstart(ifp->if_unit);
	(void) splx(s);
	return (0);
qfull:
	m0 = m;
	(void) splx(s);
bad:
	m_freem(m0);
	return (error);
}
