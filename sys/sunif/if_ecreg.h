/*	@(#)if_ecreg.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * 3Com Ethernet controller registers.
 */

#ifndef _sunif_if_ecreg_h
#define _sunif_if_ecreg_h

struct ecdevice {
	u_short	ec_csr;		/* control and status */
	u_short	ec_back;	/* backoff value */
	u_char	ec_pad1[0x400-2*2];
	struct	ether_addr ec_arom;	/* address ROM */
	u_char	ec_pad2[0x200-6];
	struct	ether_addr ec_aram;	/* address RAM */
	u_char	ec_pad3[0x200-6];
	u_char	ec_tbuf[2048];	/* transmit buffer */
	u_char	ec_abuf[2048];	/* receive buffer A */
	u_char	ec_bbuf[2048];	/* receive buffer B */
};

/*
 * Control and status bits
 */
#define	EC_BBSW		0x8000		/* buffer B belongs to ether */
#define	EC_ABSW		0x4000		/* buffer A belongs to ether */
#define	EC_TBSW		0x2000		/* transmit buffer belongs to ether */
#define	EC_JAM		0x1000		/* Ethernet jammed (collision) */
#define	EC_AMSW		0x0800		/* address RAM belongs to ether */
#define	EC_RBBA		0x0400		/* buffer B older than A */
#define	EC_RESET	0x0100		/* reset controller */
#define	EC_BINT		0x0080		/* buffer B interrupt enable */
#define	EC_AINT		0x0040		/* buffer A interrupt enable */
#define	EC_TINT		0x0020		/* transmitter interrupt enable */
#define	EC_JINT		0x0010		/* jam interrupt enable */
#define	EC_INTPA	0x00ff		/* mask for interrupt and PA fields */
#define	EC_PAMASK	0x000f		/* PA field */

#define	EC_PA		0x0007		/* receive mine+broadcast-errors */
#define EC_PROMISC	0x0001		/* receive all-errors */

/*
 * Receive status bits
 */
#define	EC_FCSERR	0x8000		/* FCS error */
#define	EC_BROADCAST	0x4000		/* packet was broadcast packet */
#define	EC_RGERR	0x2000		/* range error */
#define	EC_ADDRMATCH	0x1000		/* address match */
#define	EC_FRERR	0x0800		/* framing error */
#define	EC_DOFF		0x07ff		/* first free byte */

#define	ECRDOFF		2		/* packet offset in read buffer */
#define	ECMAXTDOFF	(2048-60)	/* max packet offset (min size) */

#endif /*!_sunif_if_ecreg_h*/
