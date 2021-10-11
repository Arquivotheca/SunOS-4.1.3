/*	@(#)if_lereg.h	1.1	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * AMD 7990 LANCE Ethernet controller registers.
 */

#ifndef _sunif_if_lereg_h
#define _sunif_if_lereg_h

/*
 * The LANCE chip saves address pins by accessing
 * several registers with one address pin by first writing the
 * register address to an internal address register, then reading
 * or writing a data register.  To use this safely, care must be
 * taken that the driver isn't reentered between the writing
 * of the address register and the access to the data register,
 * lest the reentered code try to touch the registers and
 * screw up the sequence.
 *
 * There are 4 registers accessible with this scheme, CSR0, CSR1,
 * CSR2, and CSR3.  In normal operation, only CSR0 can be or
 * needs to be accessed, so the driver normally leaves a 0 in the
 * Register Address Port, allowing CSR0 to be accessed simply by
 * accessing the Register Data Port.  During the initialization
 * sequence, when the other CSRs need to be accessed, the appropriate
 * CSR address is written into the address port, and afterwards
 * the 0 is put back in the address port.
 */

/*
 * Buffer size is chosen to give room for:
 *  -	the Ethernet maximum transmission unit,		1536
 *  -	the CRC,					   4
 *  -	and an overrun consisting of the entire
 *	fifo contents,					  48
 *  and rounded up to a multiple of 32 for optimal alignments
 */
#define MAXBUF	1600

#define ETHER_MIN_TU	 60	/* minimum output packet length */
#define LANCE_MIN_TU	100	/* minimum length of first buffer of
				   multi-buffer output packet */

struct le_device {
	u_short	le_rdp;		/* Register Data Port */
	/*
	 * Strictly speaking, the following definition is
	 *
	 *	u_short		: 14,
	 *		rap	:  2;
	 *
	 * but compilers will often generate byte accesses on
	 * machines where the lance chip needs to be accessed
	 * as a halfword (e.g., 4/60,3/80), so we'll declare
	 * it as a plain halfword here.
	 *
	 */
	u_short	le_rap;		/* Register Address Port */
};
#define le_csr le_rdp

#define	LE_CSR0		0
#define	LE_CSR1		1
#define	LE_CSR2		2
#define	LE_CSR3		3

/*
 * Control and status bits for CSR0.
 * These behave somewhat strangely, but the net effect is that
 * bit masks may be written to the register which affect only
 * those functions for which there is a one bit in the mask.
 * The exception is the interrupt enable, which must be explicitly
 * set to the correct value in each mask that is used.
 *
 * RO - Read Only, writing has no effect
 * RC - Read, Clear.  Writing 1 clears, writing 0 has no effect
 * RW - Read, Write.
 * W1 - Write with 1 only.  Writing 1 sets, writing 0 has no effect.
 *	Reading gives unpredictable data but doesn't hurt anything.
 * RW1 - Read, Write with 1 only.  Writing 1 sets, writing 0 has no effect.
 */

#define	LE_ERR		0x8000		/* RO BABL | CERR | MISS | MERR */
#define	LE_BABL		0x4000		/* RC transmitted too many bits */
#define	LE_CERR		0x2000		/* RC No Heartbeat */
#define	LE_MISS		0x1000		/* RC Missed an incoming packet */
#define	LE_MERR		0x0800		/* RC Memory Error; no acknowledge */
#define	LE_RINT		0x0400		/* RC Received packet Interrupt */
#define	LE_TINT		0x0200		/* RC Transmitted packet Interrupt */
#define	LE_IDON		0x0100		/* RC Initialization Done */
#define	LE_INTR		0x0080		/* RO BABL|MISS|MERR|RINT|TINT|IDON */
#define	LE_INEA		0x0040		/* RW Interrupt Enable */
#define	LE_RXON		0x0020		/* RO Receiver On */
#define	LE_TXON		0x0010		/* RO Transmitter On */
#define	LE_TDMD		0x0008		/* W1 Transmit Demand (send it now) */
#define	LE_STOP		0x0004		/* RW1 Stop */
#define	LE_STRT		0x0002		/* RW1 Start */
#define	LE_INIT		0x0001		/* RW1 Initialize */

/*
 * CSR1  is   the low 16 bits of the address of the initialization block
 * CSR2  is   the high 8 bits of the address of the initialization block
 *	      the high 8 bits of the register must be 0
 * CSR3 mode bits:
 *
 */
#define	LE_BSWP		0x4	/* Byte Swap (on for 68000 byte order) */
#define	LE_ACON		0x2	/* ALE Control (on for active low ALE) */
#define	LE_BCON		0x1	/* Byte Control (see the manual) */

/* The address contained in this structure must be longword aligned */
struct le_drp {			/* Descriptor Ring Pointer */
	u_short	drp_laddr;	/* Low 16 bits of ring address */
	u_char	drp_len	: 3;	/* Binary exponent of no. of ring entries */
	u_char		: 5;	/* Reserved */
	u_char	drp_haddr;	/* High 16 bits of ring address */
};

/*
 * Initialization Block.  This structure is constructed in memory,
 * and it's address is written into the chip during initialization.
 * The chip then fetches it's initialization info from the structure.
 */
struct le_init_block {
	/* In the normal mode, these 16 bits are all 0 */
	u_short	ib_prom	: 1;	/* Promiscuous Mode */
	u_short		: 8;	/* Reserved */
	u_short	ib_intl	: 1;	/* Internal Loopback */
	u_short	ib_drty	: 1;	/* Disable Retry */
	u_short	ib_coll	: 1;	/* Force Collision */
	u_short	ib_dtcr	: 1;	/* Disable Transmit CRC */
	u_short	ib_loop	: 1;	/* Loopback */
	u_short	ib_dtx	: 1;	/* Disable Transmitter */
	u_short	ib_drx	: 1;	/* Disable Receiver */

	/*
	 * The bytes must be swapped within the word, so that, for example,
	 * the address 8:0:20:1:25:5a is written in the order
	 *             0 8 1 20 5a 25
	 */
	u_char	ib_padr[6];

	u_short	ib_ladrf[4];	/* Multicast logical address filter */

	struct	le_drp ib_rdrp;	/* Receive Descriptor Ring Pointer */
	struct	le_drp ib_tdrp;	/* Transmit Descriptor Ring Pointer */
};

struct	le_md {			/* Message Descriptor */
	u_short	lmd_ladr;	/* Low Order 16 Address Bits */
	u_char	lmd_flags;
	u_char	lmd_hadr: 8;	/* High Order 8 Address Bits */
	u_short	lmd_bcnt;	/* Buffer Byte Count (maximum length) */
	u_short	lmd_mcnt;	/* Message Byte Count (actual length) */
};
#define lmd_flags3 lmd_mcnt	/* for Transmit message descriptor */

/* Bits common to both rmds and tmds */
#define	LMD_OWN		0x80	/* Chip owns the descriptor */
#define	LMD_ERR		0x40	/* Error occurred */
#define	LMD_STP		0x02	/* Start of Packet */
#define	LMD_ENP		0x01	/* End of Packet */

/* Bits in rmd flags */
#define	RMD_FRAM	0x20	/* Framing error */
#define	RMD_OFLO	0x10	/* Internal Silo Overflowed. Valid if !ENP */
#define	RMD_CRC		0x08	/* CRC Error */
#define	RMD_BUFF	0x04	/* Didn't have a buffer for the packet */
/* bits in tmd flags */
#define	TMD_MORE	0x10	/* More than one retry was needed */
#define	TMD_ONE  	0x08	/* Exactly One Retry, valid only if !LCOL */
#define	TMD_DEF  	0x04	/* Deferred (net was initially busy) */

/* Bits for lmd_errflags */
#define	TMD_BUFF 0x8000		/* Buffer Error (imples underflow too) */
#define	TMD_UFLO 0x4000		/* Underflow Error */
#define	TMD_LCOL 0x1000		/* Late Collision */
#define	TMD_LCAR 0x0800		/* Loss of Carrier */
#define	TMD_RTRY 0x0400		/* More than 16 Retry's */
#define	TMD_TDR	 0x003f		/* Time Domain Reflectometry counter mask */

#endif /*!_sunif_if_lereg_h*/
