/*	@(#)nit.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Constants and structures defined for nit
 */

#ifndef _net_nit_h
#define _net_nit_h

#define NITIFSIZ	10		/* size of ifname in sockaddr */
#define NITBUFSIZ	1024		/* buffers are rounded up to a
					 * multiple of this size (MCLBYTES) */

/*
 * Protocols
 */
#define	NITPROTO_RAW		1		/* raw protocol */
#define	NITPROTO_MAX		2

/*
 * Sockaddr_nit, as the "local address" associated with a raw socket,
 * needs to uniquely identify the associated process, since this is
 * what rawintr will use to distribute packets.  We do not distribute
 * on the "foreign address", as we need it to provide the outgoing
 * network address.
 */
struct sockaddr_nit {
	u_short	snit_family;
	caddr_t	snit_cookie;			/* link to filtering */
	char	snit_ifname[NITIFSIZ];		/* interface name (ie0) */
};

/* Header preceeding each packet returned to user */
struct nit_hdr {
	int	nh_state;		/* state of tap -- see below */
	struct	timeval	nh_timestamp;	/* time of arriving packet */
	int	nh_wirelen;		/* length (with header) off wire */
	union	{ 
		int	info;		/* generic information */
		int	datalen;	/* length of saved packet portion */
		int	dropped;	/* number of dropped matched packets */
		int	seqno;		/* sequence number */
	} nh_un;
};
#define nh_info		nh_un.info
#define nh_datalen	nh_un.datalen
#define nh_dropped	nh_un.dropped
#define nh_seqno	nh_un.seqno

/*
 * Ioctl parameter block
 * 	When setting parameter values, values that
 *	are otherwise impossible mean "don't change".
 *
 *	Nioc_bufalign and nioc_bufoffset control packet placement
 *	within buffers.  The (nit) header for each packet in a buffer
 *	starts nioc_bufoffset bytes past some multiple of nioc_bufalign
 *	bytes from the beginning.  The packet itself appears immediately
 *	beyond the header.
 */
struct nit_ioc {
	int	nioc_bufspace;		/* total buffer space to use */
	int	nioc_chunksize;		/* size of chunks to send */
	u_int	nioc_typetomatch;	/* magic type with which to match */
	int	nioc_snaplen;		/* length of packet portion to snap */
	int	nioc_bufalign;		/* packet header alignment multiple */
	int	nioc_bufoffset;		/* packet header alignment offset */
	struct	timeval nioc_timeout;	/* delay after packet before flush */
	int	nioc_flags;		/* see below */
};
#define NT_NOTYPES	((u_int)0)	/* match no packet types */
#define NT_ALLTYPES	((u_int)-1)	/* match all packet types */

#define NF_PROMISC	0x01		/* enter promiscuous mode */
#define NF_TIMEOUT	0x02		/* timeout value valid */
#define NF_BUSY		0x04		/* buffer is busy (has data) */

/*
 * States for the packet capture portion of nit,
 * some of which are passed to the user.
 */
#define NIT_QUIET	0	/* inactive */
#define NIT_CATCH	1	/* capturing packets */
#define NIT_NOMBUF	2	/* discarding -- out of mbufs */
#define NIT_NOCLUSTER	3	/* discarding -- out of mclusters */
#define NIT_NOSPACE	4	/* discarding -- would exceed buffer */
/* Pseudo-states returned in information packets */
#define NIT_SEQNO	5	/* sequence number of buffer -- helps
				   to detect other software drops */
#define NIT_MAXSTATE	6	/* state count */

#if defined(KERNEL) && defined(NIT)
struct ifnet *nit_ifwithaddr();

/* Interface provided information */
struct nit_ii {
	caddr_t	nii_header;		/* header to be prepended to mbuf */
	int	nii_hdrlen;		/* length of header to prepend */
	u_int	nii_type;		/* magic "type" field for matching */
	int	nii_datalen;		/* lenght (sans header) off wire */
	int	nii_promisc;		/* packet not destined for host */
};
#endif defined(KERNEL) && defined(NIT)

#endif /*!_net_nit_h*/
