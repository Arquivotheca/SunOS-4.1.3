/*	@(#)if_iereg.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _sunif_if_iereg_h
#define _sunif_if_iereg_h

/*
 * Control block definitions for Intel 82586 (Ethernet) chip
 * All fields are byte-swapped because the damn chip wants bytes
 * in Intel byte order only, so we swap everything going to it.
 */

/* byte-swapped data types */
typedef u_short		ieoff_t;	/* CB offsets from iscp cbbase */
#define IENULLOFF	0xffff		/* null offset value */

typedef short		ieint_t;	/* 16 bit integers */
typedef long		ieaddr_t;	/* data (24-bit) addresses */

/*
 * System Configuration Pointer
 * Must be at 0xFFFFF6 in chip's address space
 */
#ifdef	sparc
#define	IESCPADDR	0xFFFFF4	/* compensate for alignment junk */
#else	sparc
#define	IESCPADDR	0xFFFFF6
#endif	sparc

struct iescp {
#ifdef	sparc
	u_short		ies_junk0;	/* pad for sparc aligment */
#endif	sparc
	char		ies_sysbus;	/* bus width: 0 => 16, 1 => 8 */
	char		ies_junk[5];	/* unused */
	ieaddr_t	ies_iscp;	/* address of iscp */
};

/*
 * Intermediate System Configuration Pointer
 * Specifies base of all other control blocks and the offset of the SCB
 */
struct ieiscp {
	char		ieis_busy;	/* 1 => initialization in progress */
	char		ieis_junk;	/* unused */
	ieoff_t		ieis_scb;	/* offset of SCB */
	ieaddr_t	ieis_cbbase;	/* base of all control blocks */
};


/*
 * The remaining chip data structures all start with a 16-bit status
 * word.  The meaning of the individual bits within a status word
 * depends on the kind of descriptor or control block the word is
 * embedded in.  The driver accesses some status words only through
 * their individual bits; others are accessed as words as well.
 * These latter are defined as unions.
 */

/*
 * System Control Block - the focus of communication
 */
struct iescb {
    union {
	u_short	Ie_status;		/* chip status word */
	struct {
		u_char		: 1;	/* mbz */
		u_char	Ie_rus	: 3;	/* receive unit status */
		u_char		: 4;	/* mbz */
		u_char	Ie_cx	: 1;	/* command done (interrupt) */
		u_char	Ie_fr	: 1;	/* frame received (interrupt) */
		u_char	Ie_cnr	: 1;	/* command unit not ready */
		u_char	Ie_rnr	: 1;	/* receive unit not ready */
		u_char		: 1;	/* mbz */
		u_char	Ie_cus	: 3;	/* command unit status */
	} ie_sb;
    } ie_sw;
#define	ie_status	ie_sw.Ie_status
#define	ie_rus		ie_sw.ie_sb.Ie_rus
#define	ie_cx		ie_sw.ie_sb.Ie_cx
#define	ie_fr		ie_sw.ie_sb.Ie_fr
#define	ie_cnr		ie_sw.ie_sb.Ie_cnr
#define	ie_rnr		ie_sw.ie_sb.Ie_rnr
#define	ie_cus		ie_sw.ie_sb.Ie_cus

	u_short	ie_cmd;			/* command word */
	ieoff_t	ie_cbl;			/* command list */
	ieoff_t	ie_rfa;			/* receive frame area */
	ieint_t	ie_crcerrs;		/* count of CRC errors */
	ieint_t	ie_alnerrs;		/* count of alignment errors */
	ieint_t	ie_rscerrs;		/* count of discarded packets */
	ieint_t	ie_ovrnerrs;		/* count of overrun packets */
	/* end of fields recognized by the chip */
#ifdef	KERNEL
	u_short	ie_magic;		/* for checking overwrite bug */
#define	IEMAGIC	0x1956
#endif	KERNEL
};

/* ie_rus */
#define	IERUS_IDLE		0
#define	IERUS_SUSPENDED		1
#define	IERUS_NORESOURCE	2
#define	IERUS_READY		4

/* ie_cus */
#define	IECUS_IDLE		0
#define	IECUS_SUSPENDED		1
#define	IECUS_READY		2

/* ie_cmd */
#define	IECMD_RESET		0x8000	/* reset chip */
#define	IECMD_RU_START		(1<<12)	/* start receiver unit */
#define	IECMD_RU_RESUME		(2<<12)	/* resume receiver unit */
#define	IECMD_RU_SUSPEND	(3<<12)	/* suspend receiver unit */
#define	IECMD_RU_ABORT		(4<<12)	/* abort receiver unit */
#define	IECMD_ACK_CX		0x80	/* ack command executed */
#define	IECMD_ACK_FR		0x40	/* ack frame received */
#define	IECMD_ACK_CNR		0x20	/* ack CU not ready */
#define	IECMD_ACK_RNR		0x10	/* ack RU not ready */
#define	IECMD_CU_START		1	/* start command unit */
#define	IECMD_CU_RESUME		2	/* resume command unit */
#define	IECMD_CU_SUSPEND	3	/* suspend command unit */
#define	IECMD_CU_ABORT		4	/* abort command unit */


/*
 * Command Unit data structures
 */

/*
 * Generic command block
 *	Status word bits that are defined here have the
 *	same meaning for all command types.  Bits not
 *	defined here don't have a common meaning.
 */
struct iecb {
    union {
	u_short	Iec_status;	/* command block status word */
	struct {
	    u_char		: 8;	/* part of status */
	    u_char Iec_done	: 1;	/* command done */
	    u_char Iec_busy	: 1;	/* command busy */
	    u_char Iec_ok	: 1;	/* command successful */
	    u_char Iec_aborted 	: 1;	/* command aborted */
	    u_char		: 4;	/* more status */
	} ie_sb;
    } ie_sw;
#define	iec_status	ie_sw.Iec_status
#define	iec_done	ie_sw.ie_sb.Iec_done
#define	iec_busy	ie_sw.ie_sb.Iec_busy
#define	iec_ok		ie_sw.ie_sb.Iec_ok
#define	iec_aborted	ie_sw.ie_sb.Iec_aborted

	u_char			: 5;	/* unused */
	u_char	iec_cmd		: 3;	/* command # */
	u_char	iec_el		: 1;	/* end of list */
	u_char	iec_susp	: 1;	/* suspend when done */
	u_char	iec_intr	: 1;	/* interrupt when done */
	u_char			: 5;	/* unused */
	ieoff_t	iec_next;		/* next CB */
};

/*
 * CB commands (ie_cmd)
 */
#define	IE_NOP		0
#define	IE_IADDR	1	/* individual address setup */
#define	IE_CONFIG	2	/* configure */
#define	IE_MADDR	3	/* multicast address setup */
#define	IE_TRANSMIT	4	/* transmit */
#define	IE_TDR		5	/* TDR test */
#define	IE_DUMP		6	/* dump registers */
#define	IE_DIAGNOSE	7	/* internal diagnostics */

/*
 * TDR command block
 */
struct ietdr {
	struct iecb ietdr_cb;		/* common command block */
	u_char	ietdr_timlo	: 8;	/* time */
	u_char	ietdr_ok	: 1;	/* link OK */
	u_char	ietdr_xcvr	: 1;	/* transceiver bad */
	u_char	ietdr_open	: 1;	/* cable open */
	u_char	ietdr_shrt	: 1;	/* cable shorted */
	u_char			: 1;
	u_char	ietdr_timhi	: 3;	/* time */
};

/*
 * Individual address setup command block
 */
struct ieiaddr {
	struct iecb ieia_cb;	/* common command block */
	char	ieia_addr[6];	/* the actual address */
};


/*
 * Multicast address setup command block
 */
struct iemcaddr {
	struct iecb iemc_cb;		/* common command block */
	ieint_t	iemc_count;		/* count of MC addresses */
	char	iemc_addr[MCADDRMAX*6];	/* pool of addresses */
};

/*
 * Configure command
 */
struct ieconf {
	struct iecb ieconf_cb;		/* common command block */
	u_char			: 4;
	u_char	ieconf_bytes	: 4;	/* # of conf bytes */
	u_char			: 4;
	u_char	ieconf_fifolim	: 4;	/* fifo limit */
	u_char	ieconf_savbf	: 1;	/* save bad frames */
	u_char	ieconf_srdy	: 1;	/* srdy/ardy (?) */
	u_char			: 6;
	u_char	ieconf_extlp	: 1;	/* external loopback */
	u_char	ieconf_intlp	: 1;	/* external loopback */
	u_char	ieconf_pream	: 2;	/* preamble length code */
	u_char	ieconf_acloc	: 1;	/* addr&type fields separate */
	u_char	ieconf_alen	: 3;	/* address length */
	u_char	ieconf_bof	: 1;	/* backoff method */
	u_char	ieconf_exprio	: 3;	/* exponential prio */
	u_char			: 1;
	u_char	ieconf_linprio	: 3;	/* linear prio */
	u_char	ieconf_space	: 8;	/* interframe spacing */
	u_char	ieconf_slttml	: 8;	/* low bits of slot time */
	u_char	ieconf_retry	: 4;	/* # xmit retries */
	u_char			: 1;
	u_char	ieconf_slttmh	: 3;	/* high bits of slot time */
	u_char	ieconf_pad	: 1;	/* flag padding */
	u_char	ieconf_hdlc	: 1;	/* HDLC framing */
	u_char	ieconf_crc16	: 1;	/* CRC type */
	u_char	ieconf_nocrc	: 1;	/* disable CRC appending */
	u_char	ieconf_nocarr	: 1;	/* no carrier OK */
	u_char	ieconf_manch	: 1;	/* Manchester encoding */
	u_char	ieconf_nobrd	: 1;	/* broadcast disable */
	u_char	ieconf_promisc	: 1;	/* promiscuous mode */
	u_char	ieconf_cdsrc	: 1;	/* CD source */
	u_char	ieconf_cdfilt	: 3;	/* CD filter bits (?) */
	u_char	ieconf_crsrc	: 1;	/* carrier source */
	u_char	ieconf_crfilt	: 3;	/* carrier filter bits */
	u_char	ieconf_minfrm	: 8;	/* min frame length */
	u_char			: 8;
};

/*
 * Transmit frame descriptor ( Transmit command block )
 */
struct ietfd {
    union {
	u_short	Ietfd_status;		/* transmit command block status */
	struct {
	    u_char Ietfd_defer	: 1;	/* transmission deferred */
	    u_char Ietfd_heart	: 1;	/* Heartbeat */
	    u_char Ietfd_xcoll	: 1;	/* Too many collisions */
	    u_char		: 1;	/* unused */
	    u_char Ietfd_ncoll	: 4;	/* Number of collisions */
	    u_char Ietfd_done	: 1;	/* command done */
	    u_char Ietfd_busy	: 1;	/* command busy */
	    u_char Ietfd_ok	: 1;	/* command successful */
	    u_char Ietfd_aborted
				: 1;	/* command aborted */
	    u_char		: 1;	/* unused */
	    u_char Ietfd_nocarr	: 1;	/* No carrier sense */
	    u_char Ietfd_nocts	: 1;	/* Lost Clear to Send */
	    u_char Ietfd_underrun
				: 1;	/* DMA underrun */
	} Ietfd_sb;
    } ie_sw;
#define	ietfd_status	ie_sw.Ieftd_status
#define	ietfd_defer	ie_sw.Ietfd_sb.Ietfd_defer
#define	ietfd_heart	ie_sw.Ietfd_sb.Ietfd_heart
#define	ietfd_xcoll	ie_sw.Ietfd_sb.Ietfd_xcoll
#define	ietfd_ncoll	ie_sw.Ietfd_sb.Ietfd_ncoll
#define	ietfd_done	ie_sw.Ietfd_sb.Ietfd_done
#define	ietfd_busy	ie_sw.Ietfd_sb.Ietfd_busy
#define	ietfd_ok	ie_sw.Ietfd_sb.Ietfd_ok
#define	ietfd_aborted	ie_sw.Ietfd_sb.Ietfd_aborted
#define	ietfd_nocarr	ie_sw.Ietfd_sb.Ietfd_nocarr
#define	ietfd_nocts	ie_sw.Ietfd_sb.Ietfd_nocts
#define	ietfd_underrun	ie_sw.Ietfd_sb.Ietfd_underrun

	u_char			: 5;	/* unused */
	u_char	ietfd_cmd	: 3;	/* command # */
	u_char	ietfd_el	: 1;	/* end of list */
	u_char	ietfd_susp	: 1;	/* suspend when done */
	u_char	ietfd_intr	: 1;	/* interrupt when done */
	u_char			: 5;	/* unused */
	ieoff_t	ietfd_next;		/* next TFD */
	ieoff_t	ietfd_tbd;		/* pointer to buffer descriptor */
	u_char	ietfd_dhost[6];		/* destination address field */
	u_short	ietfd_type;		/* Ethernet packet type field */
	/* end of fields recognized by chip */
#ifdef	KERNEL
	struct ietfd	*ietfd_flink;	/* free list link */
	struct mbuf	*ietfd_mbuf;	/* associated mbuf chain */
	union ietbuf	*ietfd_tail;	/* buffer for slop at end */
	u_int		ietfd_tlen;	/* slop length */
#endif	KERNEL
};

/*
 * Transmit buffer descriptor
 */
struct ietbd {
	u_char	 ietbd_cntlo	: 8;	/* Low order 8 bits of count */
	u_char	 ietbd_eof	: 1;	/* last buffer for this packet */
	u_char			: 1;	/* unused */
	u_char	 ietbd_cnthi	: 6;	/* High order 6 bits of count */
	ieoff_t	 ietbd_next;		/* next TBD */
	ieaddr_t ietbd_buf;		/* pointer to buffer */
};


/*
 * Receive Unit data structures
 */

/*
 * Receive frame descriptor
 */
struct ierfd {
    union {
	u_short	Ierfd_status;		/* rfd status word */
	struct {
	    u_char Ierfd_short	: 1;	/* short frame */
	    u_char Ierfd_noeof	: 1;	/* no EOF (bitstuffing mode only) */
	    u_char		: 6;	/* unused */
	    u_char Ierfd_done	: 1;	/* command done */
	    u_char Ierfd_busy	: 1;	/* command busy */
	    u_char Ierfd_ok	: 1;	/* command successful */
	    u_char		: 1;	/* unused */
	    u_char Ierfd_crcerr	: 1;	/* crc error */
	    u_char Ierfd_align	: 1;	/* alignment error */
	    u_char Ierfd_nospace
				: 1;	/* out of buffer space */
	    u_char Ierfd_overrun
				: 1;	/* DMA overrun */
	} Ierfd_sb;
    } ie_sw;
#define	ierfd_status	ie_sw.Ierfd_status
#define	ierfd_short	ie_sw.Ierfd_sb.Ierfd_short
#define	ierfd_done	ie_sw.Ierfd_sb.Ierfd_done
#define	ierfd_busy	ie_sw.Ierfd_sb.Ierfd_busy
#define	ierfd_noeof	ie_sw.Ierfd_sb.Ierfd_noeof
#define	ierfd_ok	ie_sw.Ierfd_sb.Ierfd_ok
#define	ierfd_crcerr	ie_sw.Ierfd_sb.Ierfd_crcerr
#define	ierfd_align	ie_sw.Ierfd_sb.Ierfd_align
#define	ierfd_nospace	ie_sw.Ierfd_sb.Ierfd_nospace
#define	ierfd_overrun	ie_sw.Ierfd_sb.Ierfd_overrun

	u_char			: 8;	/* unused */
	u_char	ierfd_el	: 1;	/* end of list */
	u_char	ierfd_susp	: 1;	/* suspend when done */
	u_char			: 6;	/* unused */
	ieoff_t	ierfd_next;		/* next RFD */
	ieoff_t	ierfd_rbd;		/* pointer to buffer descriptor */
	u_char	ierfd_dhost[6];		/* destination address field */
	u_char	ierfd_shost[6];		/* source address field */
	u_short	ierfd_type;		/* Ethernet packet type field */
	/* end of fields recognized by chip */
};

/*
 * Receive buffer descriptor
 */
struct ierbd {
    union {
	u_short	Ierbd_status;		/* rbd status word */
	struct {
	    u_char Ierbd_cntlo	: 8;	/* Low order 8 bits of count */
	    u_char Ierbd_eof	: 1;	/* last buffer for this packet */
	    u_char Ierbd_used	: 1;	/* EDLC sets when buffer is used */
	    u_char Ierbd_cnthi	: 6;	/* High order 6 bits of count */
	} Ierbd_sb;
    } ie_sw;
#define	ierbd_status	ie_sw.Ierbd_status
#define	ierbd_cntlo	ie_sw.Ierbd_sb.Ierbd_cntlo
#define	ierbd_eof	ie_sw.Ierbd_sb.Ierbd_eof
#define	ierbd_used	ie_sw.Ierbd_sb.Ierbd_used
#define	ierbd_cnthi	ie_sw.Ierbd_sb.Ierbd_cnthi

	ieoff_t ierbd_next;		/* next RBD */
	ieaddr_t ierbd_buf;		/* pointer to buffer */
	u_char	ierbd_sizelo	: 8;	/* Low order 8 bits of buffer size */
	u_char	ierbd_el	: 1;	/* end-of-list if set */
	u_char			: 1;	/* unused */
	u_char	ierbd_sizehi	: 6;	/* High order 6 bits of buffer size */
	/* End of fields recognized by chip */
#ifdef	KERNEL
	struct ierbuf	*ierbd_rbuf;	/* ptr to data packet descriptor */
#endif	KERNEL
};

#endif /*!_sunif_if_iereg_h*/
