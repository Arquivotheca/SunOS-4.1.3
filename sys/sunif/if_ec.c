#ifndef lint
static  char sccsid[] = "@(#)if_ec.c 1.1 92/07/30 SMI";
#endif

#define dprintf printf

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "ec.h"

/*
 * 3Com Ethernet Controller interface
 */

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/socket.h>
#include <sys/errno.h>

#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

#include <sundev/mbvar.h>
#include <sunif/if_ecreg.h>

#define	ECSET(bits) addr->ec_csr = (addr->ec_csr & EC_INTPA) | (bits)
#define	ECCLR(bits) addr->ec_csr = addr->ec_csr & (EC_INTPA & ~(bits))
#define	ECUNIT(x)	minor(x)
#define	ECRAND(randx) (((randx) * 1103515245 + 12345) & 0x7fffffff)

int	ecprobe(), ecattach(), ecintr();
struct	mb_device *ecinfo[NEC];
struct	mb_driver ecdriver = {
	ecprobe, 0, ecattach, 0, 0, ecintr,
	sizeof (struct ecdevice), "ec", ecinfo, 0, 0, MDR_DMA,
};

int	ecinit(),ecioctl(),ecoutput(),ecreset();

struct mbuf *copy_to_mbufs();

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * es_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 * "es" indicates Ethernet Softwarestatus.
 */
struct	ec_softc {
	struct	arpcom es_ac;		/* common ethernet structures */
	long	es_rand;		/* random number for backoff */
	char	es_oactive;		/* is output active? */
	char	es_obroken;		/* output broken (e.g. coax shorted) */
	char	es_back;		/* number of backoffs done */
	char	es_prom;		/* promiscuous active */
	char	es_promloc;		/* promiscuous (local only) */
	short	es_promlen;		/* promiscuous length */
} ec_softc[NEC];

#define	es_if		es_ac.ac_if	/* network-visible interface */
#define	es_enaddr	es_ac.ac_enaddr	/* hardware ethernet address */

/*
 * Probe for device.
 */
ecprobe(reg)
	caddr_t reg;
{
	register struct ecdevice *addr = (struct ecdevice *)reg;

	if (peek((short *)&addr->ec_csr) < 0 ||
	    peek((short *)&addr->ec_bbuf[2046]) < 0)
		return (0);
	return (1);
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
ecattach(md)
	struct mb_device *md;
{
	struct ec_softc *es = &ec_softc[md->md_unit];
	struct ecdevice *addr = (struct ecdevice *)md->md_addr;
	u_char *cp;

	addr->ec_csr = EC_RESET;	/* reset the board */
	DELAY(10);			/* wait for it to reset */

	/*
	 * Read the ethernet address off the board, one byte at a time.
	 */
	(void) localetheraddr(&addr->ec_arom, &es->es_enaddr);

	/*
	 * Initialize the random number generator used for collision backoff
	 * with the low-order 3 bytes of the Ethernet address
	 */
	cp = &es->es_enaddr.ether_addr_octet[3];
	es->es_rand = (cp[0]<<16) | (cp[1]<<8) | cp[2] | 1;

	/*
	 * Do hardware-independent attach stuff
	 */
	ether_attach(&es->es_if, md->md_unit,
		     "ec", ecinit, ecioctl, ecoutput, ecreset);
}

/*
 * Reset of interface after system reset.
 */
ecreset(unit)
	int unit;
{
	register struct mb_device *md;

	if (unit >= NEC || (md = ecinfo[unit]) == 0 || md->md_alive == 0)
		return;
	printf("reset ec%d", unit);
	ecinit(unit);
}

/*
 * Initialization of interface;
 * XXX should clear recorded pending operations.
 */
ecinit(unit)
	int unit;
{
	register struct ec_softc *es = &ec_softc[unit];
	register struct ecdevice *addr;
	int s;
	register struct ifnet *ifp = &es->es_if;

	addr = (struct ecdevice *)ecinfo[unit]->md_addr;
	s = splimp();
	addr->ec_csr = EC_RESET;	/* reset the board */
	DELAY(10);			/* wait for it to reset */
	if ((ifp->if_flags & IFF_UP)==0) {
		(void) splx(s);
		return;
	}

	addr->ec_aram = es->es_enaddr;	/* set the (possibly new) address */
	ECSET(EC_AMSW);			/* give away the address */
	ECCLR(EC_PAMASK);
	ECSET((ifp->if_flags&IFF_PROMISC) ? EC_PROMISC : EC_PA);
	/*
	 * Hang receive buffers and start any pending writes.
	 */
	ECCLR(EC_INTPA);
	ECSET(EC_ABSW|EC_AINT|EC_BBSW|EC_BINT|EC_PA);
	es->es_oactive = 0;
	ifp->if_flags |= IFF_RUNNING;
	if (ifp->if_snd.ifq_head)
		ecstart(unit);
	(void) splx(s);
}

/*
 * Start or restart output on interface.
 * If interface is already active, then this is a nop.
 * If interface is not already active, get another datagram
 * to send off of the interface queue, and map it to the interface
 * before starting the output.
 */
ecstart(dev)
	dev_t dev;
{
        int unit = ECUNIT(dev);
	struct ec_softc *es = &ec_softc[unit];
	struct ecdevice *addr;
	struct mbuf *m;
	struct ether_header *header;
	register struct mbuf *mp;
	register int off;

	addr = (struct ecdevice *)ecinfo[unit]->md_addr;
	if (es->es_oactive)
		return;

	IF_DEQUEUE(&es->es_if.if_snd, m);
	if (m == 0) {
		es->es_oactive = 0;
		return;
	}
	es->es_oactive = 1;
	es->es_back = 0;
	/* Set the ethernet source address because the hardware won't */
	header = mtod(m, struct ether_header *);
	header->ether_shost = es->es_enaddr;

	/*
	 * Find the starting position for the transmit data within the
	 * the transmit buffer.  The data must end at the end of the
	 * transmit buffer area (that's how the 3Com knows when to stop
	 * transmitting).
	 */
	for (off = 2048, mp = m; mp; mp = mp->m_next)
		off -= mp->m_len;

	if (off > ECMAXTDOFF)		/* enforce minimum packet size */
		off = ECMAXTDOFF;

	/* Tell the 3Com board where the data starts */
	*(u_short *)(addr->ec_tbuf) = off;

	/* Copy the data into the 3Com transmit buffer at the right place */
	(void) copy_from_mbufs((u_char *)(addr->ec_tbuf + off), m);

	ECSET(EC_TBSW|EC_TINT|EC_JINT);		/* Make it go */
}

/*
 * Ethernet interface interrupt.
 */
ecintr()
{
	register struct ec_softc *es;
	register struct ecdevice *addr;
	register struct mb_device *md;
	register int unit;
	int serviced = 0;

	es = &ec_softc[0];
	for (unit = 0; unit < NEC; unit++,es++) {
		if ((md = ecinfo[unit]) == 0 || md->md_alive == 0)
			continue;
		addr = (struct ecdevice *)md->md_addr;
		/*
		 * check for receive activity 
		 */
		switch (addr->ec_csr & (EC_ABSW|EC_BBSW)) {
		case EC_ABSW|EC_BBSW:
			/* no input packets */
			ECCLR(EC_AINT|EC_BINT);
			ECSET(EC_AINT|EC_BINT);
			break;

		case EC_ABSW:
			/* BBSW == 0, receive B packet */
			ECCLR(EC_BINT);
			ecread(es, (caddr_t)addr->ec_bbuf);
			ECSET(EC_BBSW|EC_BINT);
			serviced++;
			break;

		case EC_BBSW:
			/* ABSW == 0, receive A packet */
			ECCLR(EC_AINT);
			ecread(es, (caddr_t)addr->ec_abuf);
			ECSET(EC_ABSW|EC_AINT);
			serviced++;
			break;

		case 0:
			/* ABSW == 0, BBSW == 0 */ 
			ECCLR(EC_AINT|EC_BINT);
			if (addr->ec_csr & EC_RBBA) {
				/* RBBA, receive B, then A */
				ecread(es, (caddr_t)addr->ec_bbuf);
				ECSET(EC_BBSW|EC_BINT);
				ecread(es, (caddr_t)addr->ec_abuf);
				ECSET(EC_ABSW|EC_AINT);
			} else {
				/* RBBA == 0, receive A, then B */
				ecread(es, (caddr_t)addr->ec_abuf);
				ECSET(EC_ABSW|EC_AINT);
				ecread(es, (caddr_t)addr->ec_bbuf);
				ECSET(EC_BBSW|EC_BINT);
			}
			serviced++;
			break;

		default:
			panic("ecintr: impossible value");
			/*NOTREACHED*/
		}

		/*
		 * check for transmit activity
		 */
		if (es->es_oactive == 0) {
			ECCLR(EC_TINT|EC_JINT);
			continue;
		}
		if (addr->ec_csr & EC_JAM) {
			ECCLR(EC_TINT|EC_JINT);
			/*
			 * Collision on ethernet interface.  Do exponential
			 * backoff, and retransmit.  If have backed off all
			 * the way print warning diagnostic, and drop packet.
			 */
			es->es_if.if_collisions++;
			serviced++;
			ecdocoll(unit);
			continue;
		}
		if ((addr->ec_csr & EC_TBSW) == 0) {
			ECCLR(EC_TINT|EC_JINT);
			serviced++;
			es->es_if.if_opackets++;
			es->es_oactive = 0;
			es->es_obroken = 0;
			if (es->es_if.if_snd.ifq_head)
				ecstart(unit);
		}
	}
	return (serviced);
}

ecdocoll(unit)
	int unit;
{
	register struct ec_softc *es = &ec_softc[unit];
	register struct ecdevice *addr =
	    (struct ecdevice *)ecinfo[unit]->md_addr;
	register i,m;

	if (++es->es_back > 15) { 	/* if already backed off 15 times */
		es->es_if.if_oerrors++;
		if (es->es_obroken == 0)
			printf("ec%d: ethernet jammed\n", unit);
		es->es_obroken = 1;
#ifdef notdef
		/*
		 * We should reset interface here, to unlock
		 * transmit buffer.  After reseting, need
		 * to reenable some things.
		 */
		ECSET(EC_RESET);
#endif
		/*
		 * Reset and transmit next packet (if any).
		 */
		es->es_oactive = es->es_back = 0;
		if (es->es_if.if_snd.ifq_head)
			ecstart(unit);
		return;
	}
	/*
	 * Do backoff using local random number generator.  Generator
	 * was seeded originally with ethernet address.  Generator
	 * is the same as the libc.a "rand()" function; since the low
	 * order bits of this simple generator aren't very good, we
	 * throw them away.
	 */
	m = MIN(es->es_back, 10); /* "truncated binary exponential backoff" */
	i = -1;
	i = i << m;
	i = (((es->es_rand = ECRAND(es->es_rand))>>8) & ~i) + 1;
	addr->ec_back = -i;
	ECSET(EC_JAM|EC_JINT|EC_TINT);
}

/*
 * Move info from driver toward protocol interface
 */
ecread(es, ecbuf)
	struct ec_softc *es;
	register caddr_t ecbuf;
{
	int length;
	struct ether_header *header;
	caddr_t buffer;
	struct mbuf *m;
	int off;
	int ecoff;

	es->es_if.if_ipackets++;

	ecoff = *(short *)ecbuf & EC_DOFF;
	if (*(short *)ecbuf & (EC_FRERR|EC_RGERR|EC_FCSERR) ||
	    ecoff <= ECRDOFF || ecoff > 2046) {
		identify(&es->es_if);
		printf("garbled packet\n");
		es->es_if.if_ierrors++;
		return;
	}

	/*
	 * Get input data length, pointer to ethernet header,
	 * and address of data buffer
	 */
	/* 4 == FCS */
	length = ecoff - ECRDOFF - sizeof (struct ether_header) - 4;
	header = (struct ether_header *)(ecbuf + ECRDOFF);
	buffer = (caddr_t)(&header[1]);

	if ( check_trailer(header, buffer, &length, &off) ) {
		identify(&es->es_if);
		printf("trailer error\n");
		es->es_if.if_ierrors++;
		return;
	}

	/* Check for runt packet */
	if (length == 0) {
		identify(&es->es_if);
		printf("runt packet\n");
		es->es_if.if_ierrors++;
		return;
	}

#ifdef DEBUG
#ifdef DUMPBUFFER
{	int i,j;
    if (header->ether_dhost.ether_addr_octet[0] != 0xff)
    {
	dprintf("\nSource: ");
	for (i=0; i<6; i++)
             dprintf("%.02x ",header->ether_shost.ether_addr_octet[i]&0xff);
	dprintf("\nDestin: ");
	for (i=0; i<6; i++)
             dprintf("%.02x ",header->ether_dhost.ether_addr_octet[i]&0xff);
	dprintf("\nBuffer");
	j=0;
	for (i=0; i<length; i++) {
	    if ( j == 0 )
	        dprintf("\n");
	    if ( ++j == 16 )
	        j = 0;
	    dprintf("%.02x ", buffer[i]&0xff);
	}
	dprintf("\n");
    } else
	dprintf(" B ");
}
#endif DUMPBUFFER
#endif DEBUG
	/*
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; copy_to_mbufs will then force this header
	 * information to be at the front.
	 */

	if ( (m = copy_to_mbufs(buffer, length, off)) == (struct mbuf *) 0 )
		return;
	/*
	 * There used to be a separate variable "wirelen" which remembered
	 * the length of the packet before the trailer code changed the
	 * length.  The trailer code now no longer changes the length
	 * variable (but it does adjust the length reflected in the mbuf
	 * chain), so the separate "wirelen" is unnecessary.
	 */
	do_protocol(header, m, &es->es_ac, length);
}

ecoutput(ifp, m0, dst)
	struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
{
#ifdef DEBUG
dprintf(">");
#endif DEBUG
	return (
	  ether_output(ifp, m0, dst, ecstart)
	);
}

/*
 * Process an ioctl request.
 */
ecioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	int error = 0;
	register unit = ifp->if_unit;
	struct ec_softc *es = &ec_softc[unit];
	int s = splimp();

	switch (cmd) {

	/*
	 * Return link-level (Ethernet) address, filling it into
	 * the data part of the sockaddr embedded in the ifreq.
	 * The address family part of the sockaddr is problematic,
	 * and we leave it untouched.
	 */
	case SIOCGIFADDR:
		bcopy((caddr_t)(&((struct arpcom *)ifp)->ac_enaddr),
			(caddr_t)(((struct ifreq *)data)->ifr_addr.sa_data),
			sizeof (struct ether_addr));
		break;

	case SIOCSIFADDR:
		error = set_if_addr(ifp,
			&((struct ifaddr *)data)->ifa_addr, &es->es_enaddr);
		break;

	case SIOCSIFFLAGS:
		/*
		 * The following routine turns the board on/off depending
		 * on if the IFF_UP bit is set (already done at the 
		 * higher level.
		 */
		ecinit(unit);
		break;

	default:
		error = EINVAL;
	}

	(void) splx(s);
	return (error);
}
