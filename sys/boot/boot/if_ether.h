/*      @(#)if_ether.h 1.1 92/07/30 SMI; from UCB 6.2 83/09/19       */

/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef	_IF_ETHER_
#define	_IF_ETHER_

/*
 * Ethernet address - 6 octets
 */
struct ether_addr {
	u_char	ether_addr_octet[6];
};

/*
 * Structure of a 10Mb/s Ethernet header.
 */
struct	ether_header {
	struct	ether_addr ether_dhost;
	struct	ether_addr ether_shost;
	u_short	ether_type;
};

#define	ETHERTYPE_PUP		0x0200		/* PUP protocol */
#define	ETHERTYPE_IP		0x0800		/* IP protocol */
#define	ETHERTYPE_ARP		0x0806		/* Addr. resolution protocol */
#define	ETHERTYPE_REVARP	0x8035		/* Reverse ARP */

/*
 * The ETHERTYPE_NTRAILER packet types starting at ETHERTYPE_TRAIL have
 * (type-ETHERTYPE_TRAIL)*512 bytes of data followed
 * by a PUP type (as given above) and then the (variable-length) header.
 */
#define	ETHERTYPE_TRAIL		0x1000		/* Trailer packet */
#define	ETHERTYPE_NTRAILER	16

#ifndef	notyet
/*
 * 4.2 compatibility definitions.
 *	REMOVE THESE WHEN COMPATIBILITY WITH RELEASE 3.0
 *	IS NO LONGER REQUIRED.
 */
#define	ETHERPUP_PUPTYPE	0x0200		/* PUP protocol */
#define	ETHERPUP_IPTYPE		0x0800		/* IP protocol */
#define	ETHERPUP_ARPTYPE	0x0806		/* Addr. resolution protocol */
#define	ETHERPUP_REVARPTYPE	0x8035		/* Reverse ARP */

#define	ETHERPUP_TRAIL		0x1000		/* Trailer PUP */
#define	ETHERPUP_NTRAILER	16
#endif	notyet

#define	ETHERMTU	1500
#define	ETHERMIN	(60-14)

/*
 * Ethernet Address Resolution Protocol.
 *
 * See RFC 826 for protocol description.  Structure below is adapted
 * to resolving internet addresses.  Field names used correspond to 
 * RFC 826.
 */
struct	ether_arp {
	u_short	arp_hrd;	/* format of hardware address */
#define	ARPHRD_ETHER 	1	/* ethernet hardware address */
	u_short	arp_pro;	/* format of proto. address (ETHERTYPE_IPTYPE) */
	u_char	arp_hln;	/* length of hardware address (6) */
	u_char	arp_pln;	/* length of protocol address (4) */
	u_short	arp_op;
#define	ARPOP_REQUEST	1	/* request to resolve address */
#define	ARPOP_REPLY	2	/* response to previous request */
#define	REVARP_REQUEST	3	/* Reverse ARP request */
#define	REVARP_REPLY	4	/* Reverse ARP reply */
	u_char	arp_xsha[6];	/* sender hardware address */
	u_char	arp_xspa[4];	/* sender protocol address */
	u_char	arp_xtha[6];	/* target hardware address */
	u_char	arp_xtpa[4];	/* target protocol address */
};
#define	arp_sha(ea)	(*(struct ether_addr *)(ea)->arp_xsha)
#define	arp_spa(ea)	(*(struct in_addr *)(ea)->arp_xspa)
#define	arp_tha(ea)	(*(struct ether_addr *)(ea)->arp_xtha)
#define	arp_tpa(ea)	(*(struct in_addr *)(ea)->arp_xtpa)

/*
 * Structure shared between the ethernet driver modules and
 * the address resolution code.  For example, each ec_softc or il_softc
 * begins with this structure.
 */
#define MCADDRMAX	64		/* multicast addr table length */
struct	arpcom {
	struct	ifnet ac_if;		/* network-visible interface */
	struct	ether_addr ac_enaddr;	/* ethernet hardware address */
	struct	arpcom *ac_ac;		/* link to next ether driver */
};

/*
 * Internet to ethernet address resolution table.
 */
struct	arptab {
	struct	in_addr at_iaddr;	/* internet address */
	struct	ether_addr at_enaddr;	/* ethernet address */
	struct	mbuf *at_hold;		/* last packet until resolved/timeout */
	u_char	at_timer;		/* minutes since last reference */
	u_char	at_flags;		/* flags */
};

#ifdef	KERNEL
struct	ether_addr etherbroadcastaddr;
struct	arptab *arptnew();
#endif	KERNEL

#endif	_IF_ETHER_
