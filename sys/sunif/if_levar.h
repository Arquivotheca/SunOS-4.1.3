/*	@(#)if_levar.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef _sunif_if_levar_h
#define	_sunif_if_levar_h

/*
 * Definitions for data structures that the
 * AMD 7990 LANCE driver uses internally.
 *
 * This file exists primarily to allow network monitoring
 * programs easy access to the definition of le_softc.
 */

/*
 * Transmit and receive buffer layout.
 *	The chip sees only the fields from lb_ehdr onwards; the
 *	preceding fields are for the driver's benefit.
 *
 * Alignment requirements:
 * The lb_buffer field must be word aligned for IP and
 * also 32-byte aligned for some platforms for optimal
 * programmed i/o bcopy() performance.  Thus a pad field
 * is included and the structure is made a multiple of
 * 32bytes.  No, this isn't portable.
 */
struct le_buf {
	/* Fields used only by driver: */
	struct le_buf	*lb_next;		/* Link to next buffer */
	struct le_softc	*lb_es;			/* Link back to sw status */
	int		lb_loaned;		/* loaned up or not? */
	u_char		lb_pad[6];
	/* Fields seen by LANCE chip: */
	struct ether_header	lb_ehdr;	/* Packet's ether header */
	u_char		lb_buffer[MAXBUF];	/* Packet's data */
};

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * es_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 * "es" indicates Ethernet Software status.
 */
struct	le_softc {
	struct	arpcom es_ac;		/* common ethernet structures */
#define	es_if		es_ac.ac_if	/* network-visible interface */
#define	es_enaddr	es_ac.ac_enaddr	/* hardware ethernet address */
#define	es_mcaddr	es_ac.ac_mcaddr	/* multicast address vector */
#define	es_nmcaddr	es_ac.ac_nmcaddr
					/* count of multicast addrs */

	u_int	es_ipl;		/* IPL for this device */

	struct	le_init_block *es_ib;	/* Initialization block */

	/* LANCE message descriptor info */
	struct	le_md *es_rdrp;		/* Receive Descriptor Ring Ptr */
	struct	le_md *es_rdrend;	/* Receive Descriptor Ring End */
	int	es_nrmdp2;		/* log(2) Num. Rcv. Msg. Descs. */
	int	es_nrmds;		/* Num. Rcv. Msg. Descs. */
	struct	le_md *es_tdrp;		/* Transmit Descriptor Ring Ptr */
	struct	le_md *es_tdrend;	/* Receive Descriptor Ring End */
	int	es_ntmdp2;		/* log(2) Num. Tran. Msg. Descs. */
	int	es_ntmds;		/* Num. Xmit. Msg. Descs. */
	struct	le_md *es_his_rmd;	/* Next descriptor in ring */
	struct	le_md *es_cur_tmd;	/* Tmd for start of current xmit req */
	struct	le_md *es_nxt_tmd;	/* Tmd for start of next xmit request */

	/* Buffer info */
	struct	le_buf *es_rbufs;	/* Receive Buffers */
	int	es_nrbufs;		/* Number of Receive Buffers */
	struct	le_buf *es_rbuf_free;	/* Head of free list */
	caddr_t	*es_tbuf;		/* xmit buffer area pointers */
	struct	mbuf	**es_tmbuf;	/* xmit mbuf chain holder */
	struct	le_buf	**es_rbufc;	/* points to le_buf for recv packets */

	/*
	 * Device local memory info.
	 * Allocate Transmit and Receive Descriptors plus multiple
	 * fixed-size receive buffers plus a single transmit area
	 * managed as a wrapping fifo in the device local memory.
	 */
	caddr_t	es_membase;	/* base address of device ram */
	caddr_t	es_iobase;	/* device memory base address for device use */
	int	es_memsize;	/* # bytes device ram */
	caddr_t	es_tbase;	/* base of transmit area in device ram */
	caddr_t	es_tlimit;	/* limit of transmit area in device ram */
	caddr_t	es_twr;		/* next available byte in transmit area */
	caddr_t	es_trd;		/* oldest used byte in tranmit area */

	/* Device-specific routines */
	int	(*es_leinit)();	/* initialize */
	int	(*es_leintr)();	/* interrupt */
	caddr_t	es_learg;	/* device-specific routines arg */

	u_int	es_flags;		/* State info: see below */

	/* Error counters */
	int	es_extrabyte;		/* Rev C,D LANCE extra byte problem */
	int	es_fram;		/* Receive Framing Errors (dribble) */
	int	es_crc;			/* Receive CRC Errors */
	int	es_oflo;		/* Receive overruns */
	int	es_uflo;		/* Transmit underruns */
	int	es_retries;		/* Transmit packets w/at least one */
					/* retry (collision) */
	int	es_missed;		/* Number of missed packets */
	int	es_memerr;		/* Memory errors */
	int	es_init;		/* Number of initializations */
	int	es_noheartbeat;		/* Number of nonexistent heartbeats */
	int	es_tBUFF;		/* BUFF bit in tmd occurrences */
	int	es_tlcol;		/* Transmit late collisions */
	int	es_trtry;		/* Transmit retry errors (failed */
					/* after 16 retries) */
	int	es_tnocar;		/* No carrier errors */
	int	es_dogreset;		/* Number of ledog() chip resets */

	/* Performance statistics counters */
	int	es_no_tmds;		/* Number of output packets dropped */
					/* due to unavailability of tmd's */
	int	es_no_tbufs;	/* Times transmit buffer unavailable */
	int	es_tsync;		/* Times we lost xmit sync with chip */
	int	es_defer;		/* Deferred transmission counter */
	int	es_incpy;		/* Input copy operations */
	int	es_outcpy;		/* Output copy operations */
};

/*
 * Bit definitions for es_flags field:
 */
#define	LE_PIO		0x01	/* generic "Programmed I/O" interface */
#define	LE_IOADDR	0x02	/* must translate for i/o address */

#define	LEPIO(es)		(es->es_flags & LE_PIO)

/*
 * Onboard RAM amounts (fixed constants).
 */
#define	LE_ESCMEMSIZE	(128 * 1024)	/* ESC/LANCE has 128K local memory */
#define	LE_HAWKMEMSIZE	(256 * 1024)	/* Interphase Hawk has 256K local mem */

/*
 * Ops structure to accomodate the differences between
 * different implementations of the LANCE in Suns.
 * This declaration will grow as time goes on...
 */
struct	leops {
	struct	leops	*lo_next;	/* next in linked list */
	struct	dev_info	*lo_dev;	/* node pointer (key) */
	u_int	lo_flags;		/* misc. flags */
	caddr_t	lo_membase;		/* cpu address for device memory */
	caddr_t	lo_iobase;		/* io address for device memory */
	int	lo_size;		/* # bytes device memory */
	int	(*lo_init)();		/* device-specific init */
	int	(*lo_intr)();		/* device-specific intr */
	caddr_t	lo_arg;			/* arg for device-specific routines */
};

/* leops flags */
#define	LO_PIO		0x01		/* Programmed i/o (slave-only) device */
#define	LO_IOADDR	0x02		/* must convert kernel->io address */

#ifdef	OPENPROMS

/*
 * This structure for OPENPROM machines which have no mb structures
 */

struct le_info {
	struct le_device	*le_addr;	/* mapped in chip register */
	struct dev_info		*le_dev;	/* backpointer to dev_info */
};

#define	LE_ADDR(unit)	leinfo[(unit)].le_addr
#define	LE_ALIVE(unit)	\
	(leinfo[unit].le_dev && leinfo[unit].le_dev->devi_driver)

#else	OPENPROMS

#define	LE_ADDR(unit)	(struct le_device *)leinfo[(unit)]->md_addr
#define	LE_ALIVE(unit)	(leinfo[unit] && leinfo[unit]->md_alive)

#endif	OPENPROMS
#ifdef KERNEL
/*
 * Variables imported from le_conf.c
 */
extern struct le_softc	*le_softc;
#ifdef	OPENPROMS
extern struct le_info *leinfo;
#else	OPENPROMS
extern struct mb_device *leinfo[];
#endif	OPENPROMS
extern int le_units;
extern int le_high_ntmdp2, le_high_nrmdp2, le_high_nrbufs;
extern int le_low_ntmdp2, le_low_nrmdp2, le_low_nrbufs;
#endif KERNEL

#endif /* !_sunif_if_levar_h */
