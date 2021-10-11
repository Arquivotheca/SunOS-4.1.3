/*	@(#)nit_if.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _net_nit_if_h
#define _net_nit_if_h

/*
 * Definitions for the streams link-level network interface.
 */


/*
 * Ioctls.
 */

#define	NIOCBIND	_IOW(p, 3, struct ifreq)	/* bind to interface */

#define	NIOCSFLAGS	_IOW(p, 4, u_long)		/* set nit_if flags */
#define	NIOCGFLAGS	_IOWR(p, 5, u_long)		/* get nit_if flags */

#define	NIOCSSNAP	_IOW(p, 6, u_long)		/* set snapshot len */
#define	NIOCGSNAP	_IOWR(p, 7, u_long)		/* get snapshot len */

/*
 * User-visible flag bits, accessible with the NIOC[GS]FLAGS ioctls.
 *
 */
	/* interface state */
#define	NI_PROMISC	0x01	/* put underlying if into promiscuous mode */
	/* packet headers to be prepended to each packet */
#define	NI_TIMESTAMP	0x02	/* timestamp */
#define	NI_LEN		0x04	/* length of packet as received */
#define	NI_DROPS	0x08	/* cumulative number of dropped packets */

#define	NI_USERBITS	0x0f	/* mask for state bits users may see */


/*
 * Headers.
 *
 * Depending on the state of the flag bits set with the NIOCSFLAGS ioctl,
 * each packet will have one or more headers prepended.  These headers are
 * packed contiguous to the packet and themselves with no intervening
 * space.  Their lengths are chosen to avoid alignment problems.  Prepending
 * occurs in the order of the header definitions given below, with each
 * successive header appearing in front of everything that's already in place.
 */

/*
 * Header prepended to each packet when the NI_LEN flag bit is set.
 * It contains the packet's length as of the time it was received,
 * including the link-level header.  It does not account for any of the
 * headers that nit_if adds nor for trimming the packet to the snaplen.
 */
struct nit_iflen {
	u_long		nh_pktlen;	/* packet length as received */
};

/*
 * Header prepended to each packet when the NI_DROPS flag is set.
 * It records the cumulative number of packets dropped on this stream
 * because of flow control requirements since the stream was opened.
 */
struct nit_ifdrops {
	u_long		nh_drops;	/* cum # of packets lost to flow ctl */
};

/*
 * Header prepended to each packet when the NI_TIMESTAMP flag bit is set.
 */
struct nit_iftime {
	struct timeval	nh_timestamp;	/* packet arrival time */
};


#ifdef	KERNEL
/*
 * Bridge between hardware interface and nit subsystem.
 */
struct nit_if {
	caddr_t	nif_header;	/* pointer to packet header */
	u_int	nif_hdrlen;	/* length of packet header */
	u_int	nif_bodylen;	/* length of packet body */
	int	nif_promisc;	/* packet not addressed to host */
};
#endif	KERNEL

#endif /*!_net_nit_if_h*/
