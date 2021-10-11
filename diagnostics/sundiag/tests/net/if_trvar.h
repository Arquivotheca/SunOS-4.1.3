/*      @(#)if_trvar.h	1.1 7/30/92 SMI */
/*	@(#)if_trvar.h	1.13 4/2/91 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef	_IF_TRVAR_
#define	_IF_TRVAR_

#define      HOSTBUFSIZ      2736
/*
 * Definitions for data structures that the
 * TMS 830 Token Ring driver uses internally.
 *
 * This file exists primarily to allow network monitoring
 * programs easy access to the definition of tr_softc.
 */


/*
 * Transmit and receive buffer layout.
 *	The chip sees the lb_buffer field; the
 *	preceding fields are for the driver's benefit. The e_to_lb
 *	and b_to_lb macros defined below convert the address of the
 *	start of a lb_buffer field to the address of the
 *	start of the containing tr_buf structure.
 */
struct tr_buf {
	/* First 2 fields used only by driver: */
	struct tr_buf	*lb_next;		/* Link to next buffer */
	struct tr_softc	*lb_tr;			/* Link back to sw status */
	u_char lb_buffer[HOSTBUFSIZ];		/* Packet data */
};

/*
 * Given lb_buffer address, produce the address of its
 * containing tr_buf structure.
 *
 * N.B.: This definition assume that the compiler inserts no padding
 * between the fields of the tr_buf structure.  (If it did, we'd be
 * in trouble on other grounds as well.)
 */
#define e_to_lb(e) \
	(struct tr_buf *)((caddr_t)(e) - sizeof (struct tr_softc *) \
			- sizeof (struct tr_buf *))
#define b_to_lb(b) \
	(struct le_buf *)((caddr_t)(b) - sizeof (struct tr_header) \
			- sizeof (struct tr_softc *) \
			- sizeof (struct tr_buf *) - 2)


#ifdef TR_SOURCE_ROUTE
/*
 * Source Routing structures and defines
 */

#define	SRT_FREE	0x0
#define	SRT_PENDING	0x1
#define	SRT_SR		0x3		/* in use */
#define	SRT_NON_SR	0x5		/* in use */


/*
 * Analogous to arpcom, this structure resides in Tr_softc
 * and provides basic access to the sr table, interface, and
 * a cache of the last MAC address seen on the interface 
 */
struct srcom {
/*	struct ifnet *sc_if;		/* network visible interface */
	struct ether_addr sc_lastmac;	/* cache of last MAC address */
	struct srtab *sc_srp;		/* pointer to last table entry */
};


/* 
 * This is the actual routing table structure used for source routing.
 */
struct srtab {
	struct ether_addr st_mac;	/* destination MAC address */
	struct tr_ri st_ri;		/* routing information field */
	int st_timer;			/* entry accessed since last 15 min */
	u_char st_flag;			/* flag for used/!used and SR/!SR */
};

/* 
 * adds all 6 bytes of the MAC address to do the hash 
 */
#define SRTAB_HASH(a) \
	( (*((ushort *) a) + *((ushort *) a + 1) +  \
				*((ushort *) a + 2)) % Srtab_nb) 

/*
 * returns pointer into route table if route found
 * or NULL if route not available
 */
#define SRTAB_LOOK(srp,addr) { \
	register int i; \
	srp = &Srtab[SRTAB_HASH(addr) * Srtab_bsiz]; \
	for (i = 0 ; i < Srtab_bsiz ; i++,srp++) \
		if ( !ether_cmp(&srp->st_mac, addr) ) \
			break; \
	if (i >= Srtab_bsiz) \
		srp = NULL; \
}


/* source route timer values */
/* XXX these are compatible with ARP but what about OSI */
#define SRT_AGE        (60*1)  /* aging timer, 1 min. */
#define SRT_KILLC      20      /* kill completed entry in 20 mins. */
#define SRT_KILLI      3       /* kill incomplete entry in 3 minutes */

/* external source route definitions needed by all source files */
extern int Srtab_bsiz;		/* bin size of the source route hash table */
extern int Srtab_nb;		/* # of bins in the source route hash table */
extern struct srtab *Srtab;	/* source route table */
extern int Srtab_size;		/* total table size */ 

#endif TR_SOURCE_ROUTE



/*
 * Token Ring software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * tr_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 * "tr" indicates Token Ring Software status.
 *
 * The buffer-related declarations allow for more than one packet to
 * be transmitted at a time, although the driver (and other declarations)
 * currently allows for only one at a time.
 */
struct	tr_softc {
	struct	arpcom tr_ac;		/* common token ring structures */
#define	tr_if		tr_ac.ac_if	/* network-visible interface */
#define	tr_enaddr	tr_ac.ac_enaddr	/* hardware mac address */
#define	tr_mcaddr	tr_ac.ac_mcaddr	/* multicast address vector */
#define	tr_nmcaddr	tr_ac.ac_nmcaddr /* count of multicast addrs */

#ifdef TR_SOURCE_ROUTE
	struct	srcom	tr_sc;		/* source route common structs */
#endif TR_SOURCE_ROUTE
	struct	tr_init_pb *tr_initp;	/* Initialization block */
	struct 	tr_open_pb *tr_openp;	/* Open parameter block */
	struct 	tr_errlog *tr_logp;	/* Error log dump block */
	struct	tr_scb *tr_scb;		/* System Command Parameter Block */
	struct	tr_ssb *tr_ssb;		/* System Status Parameter Block */

	/* transmit and receive list info */
	struct	tr_md *tr_rdptr;	/* Receive descriptor memory pointer */
	struct	tr_md *tr_rdtp;		/* Receive Descriptor tail pointer */
	struct	tr_md *tr_tdptr;	/* Transmit descriptor memory pointer */
	struct	tr_md *tr_tdhp;		/* Transmit Descriptor head pointer */
	struct	tr_md *tr_tdtp;		/* Transmit Descriptor tail pointer */

	/* Buffer info */
	struct	tr_buf *tr_rbhp;	/* Receive Buffers head pointer */
	struct	tr_buf *tr_rbhp_free;	/* Head of free list */
	struct	tr_buf *tr_rbhp_loaned;	/* Head of loan-out list */
	struct	tr_buf *tr_tbhp;	/* Transmit Buffer head pointer */
        struct  mbuf **tr_tmbufc;       /* mbuf chain heads for xmit packets */
	int	tr_nloanouts;		/* current number of bufs loaned out */


	u_int	tr_sysflags;		/* State info from the system */
	u_int	tr_flags;		/* State info: see below */

	/* Error counters */
	int	tr_linerr;		/* Line Errors */
	int 	tr_bsterr;		/* Burst Errors */
	int 	tr_fcierr;		/* ARI/FCI Errors */
	int 	tr_lostfrm;		/* Lost Frame Errors */
	int	tr_recvcong;		/* Receive Congestion Errors */
	int 	tr_frmcpy;		/* Frame Copy Errors */
	int	tr_token;		/* Token Errors */
	int	tr_dmabus;		/* Dma Bus Errors */
	int 	tr_dmapar;		/* Dma Parity Errors */
	int	tr_rngdwn;		/* Ring Down Errors */
#ifdef TR_SOURCE_ROUTE
	int	kill_srroute;		/* # of hash collisions in sr tbl*/
#endif TR_SOURCE_ROUTE

	/* Performance statistics counters */
	int	tr_started;		/* Times through trstart with > 0
					   packets ready to go out */
	int	tr_potential_rloans;	/* Number of opportunities to loan
					   out a receive buffer */
	int	tr_actual_rloans;	/* Cumulative number of receive buffers
					   loaned to protocol layers */
	int	tr_outcpy;		/* number of times non-chip addressable
					   buffer was used */

	/* Hooks for Sun4C autoconfiguration information */
	struct	tr_device *tr_addr;	/* Virtual address of our registers */
	struct	dev_info *tr_dev;	/* Pointer back to dev_vector info */
	char	*tr_eprom;		/* Virtual address of EPROM */
	int	*tr_s4dma;		/* Virtual address of S4 DMA chip */
	char	*tr_prod_id;		/* Pointer to product id string */

	/* hooks for sundiag - peter tan 4/1/91 */
	int sundiag_stsflag;
};

/*
 * Bit definitions for tr_flags field:
 */
#define	TR_NEEDTMD	0x1		/* out of xmit message descriptors */
#define	TR_DOWN		0x2		/* ring down */
#define	TR_CLOSED	0x4		/* controller closed */
#define	TR_OPEN		0x8		/* controller open */
#define	TR_CBUSY	0x10		/* controller busy with a command */
#define	TR_RINGUP	0x20		/* ring up */

#ifdef KERNEL
extern	char	Sysbase[];
extern int Tr_recv_lists;		/* number of receive lists (8@4500) */
extern int Tr_loanouts;			/* number of loaned out buffers */
extern int Tr_xmit_lists;		/* number of transmit lists (5@4500) */
extern int Tr_maxmitbufs;		/* max # of xmit buffers (4@4500) */
extern int Tr_minxmitbufs;		/* min # of xmit buffers (2@4500) */
extern int Tr_ctlrbufsiz;		/* controller internal buffer size */
extern int Tr_dmabstsiz;		/* dma burst size */
extern int Tr_mtutab[];
extern int Tr_mtu_idx;			/* index into mtu table */
extern int trdebug;
/* extern int sleepcount; */

#ifdef TR_SOURCE_ROUTE
extern int Srflag;
#endif TR_SOURCE_ROUTE

#endif KERNEL

/* special ioctl calls for TI chipset */
/* Do not assign 0x10, 0x11, 0x12 to defines because these values are */
/* reserved by freeze dump code written by Paul Simons                */
#define TR_FRZCONT 0x0
#define TR_FRZRST 0x1
#define TR_FRZFRE 0x2
#define TR_CHKSTS 0x3	/* sundiag ioctl hook -peter tan 3/29/91 */


/* bit masks and error flags for tr_softc's sundiag_stsflag-peter tan 4/1/91 */
/* ioctl call returns 22 = 0x016 upon ioctl call's error, therefore, 0x010 is */
/* omitted in order to avoid having confusions with that error code */

#define NET_SIG_LOSS		0x00000001
#define CAB_WIRE_FLT		0x00000002
#define BRD_BEACON_AUTO_RMV	0x00000004
#define NET_RMV_RECV		0x00000008
#define BRD_DIO_PAR		0x00000020
#define BRD_DMA_RD_ABORT	0x00000040
#define BRD_DMA_WR_ABORT	0x00000080
#define BRD_RNG_URUN		0x00000100
#define BRD_RNG_ORUN		0x00000200
#define BRD_UNREC_INTINTR	0x00000400
#define BRD_OTHER_REASONS	0x00000800
#define NET_LINE_ERR		0x00001000
#define NET_BST_ERR		0x00002000
#define NET_FCI_ERR		0x00004000
#define NET_LOST_FRM		0x00008000
#define NET_TKN_ERR		0x00010000

#endif	_IF_TRVAR_
