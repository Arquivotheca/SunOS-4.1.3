/* @(#)if_ieee802.h 1.1 92/07/30 Copyright 1987 Sun Micro */


#ifndef _net_if_ieee802_h
#define _net_if_ieee802_h

/*
 * Structure of a IEEE 802.3 header 
 */
struct ieee8023_header {
	u_char	ieee8023_dhost[6];
	u_char	ieee8023_shost[6];
	u_short	ieee8023_length;
};

#define	MAX_8023_DLEN		1500

/*
 * sepcial Ether Family type used for registering for 802.3 packets.
 */

#define EF_8023_TYPE		1500

/*
 * Address as seen by IEEE 802
 */

/* IEEE 802.[2|4] media access address */
union mac_addr_802 {
	u_char	l_fmt[6];			/* 48-bit address */
};

/* IEEE 802.2 link address */
struct link_addr_802 {
	union mac_addr_802	l_mac_addr;	/* media access address */
	u_char			l_sel;		/* link layer selector */
};
#define	SIZE_LINK_ADDR_802	7		/* DON'T use "sizeof" */

#define	S802_PAD_LEN		(16 - sizeof (struct link_addr_802) - \
					sizeof (u_short))
struct sockaddr_802 {
	u_short			s802_family;
	struct link_addr_802	s802_addr;
#define	s802_macaddr		s802_addr.l_mac_addr
#define	s802_lsel		s802_addr.l_sel
	u_char			pad[S802_PAD_LEN];
};

/*
 * Well known LSAPs
 */
#define	LSAP_ISONET	0xfe	/* ISO subnet point of attachment */
#define LSAP_SNAP	0xaa	/* "ethernet" subnet attachment point */

#endif /*!_net_if_ieee802_h*/
