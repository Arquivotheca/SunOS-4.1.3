/*	@(#)if_enreg.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Sun 3Mb Ethernet registers.
 *
 * NOTE: There is lots of strangeness here having
 *	 to do with Multibus addressing.  The Ethernet
 *	 board takes 256 bytes of Multibus address space
 *	 but only decodes a few of the low-order address
 *	 bits.  To avoid conflicting with other Multibus
 *	 peripherals that only decode 8 address bits we
 *	 start using addresses at 0xa0 within our 256 byte
 *	 chunk.  THIS IS A KLUDGE!!!
 */

#ifndef _sunif_if_enreg_h
#define _sunif_if_enreg_h

struct endevice {
	char	en_pad1[0xa0];	/* padding */
	u_short	en_data;	/* receiver data port */
	u_short	en_xxx;		/* unused */
	u_short	en_status;	/* control and status */
	u_short	en_filter;	/* receiver address filter */
	char	en_pad2[0x100-0xa0-4*sizeof(u_short)];
};

/*
 * Control and status bits.
 */

/* status register, read */
#define	EN_INTR		0x1000	/* interrupt flag */
#define	EN_RINTR	0x2000	/* receiver interrupt */
#define	EN_TIMEOUT	0x4000	/* transmitter timeout */
#define	EN_TINTR	0x8000	/* transmitter interrupt */

/* status register, write */
#define	EN_NOT_ILVL	0x0700	/* interrupt level bits (inverted) */
#define	EN_NOT_RIE	0x0800	/* receiver interrupt enable (0 = enabled) */
#define	EN_NOT_TIE	0x1000	/* transmitter interrupt enable (0 = enabled) */
#define	EN_NOT_FILTER	0x2000	/* filter data bit */
#define	EN_LOOPBACK	0x4000	/* loopback */
#define	EN_INIT		0x8000	/* initialize device */

/* data register, read */
#define	EN_COUNT	0x0fff	/* word count */
#define	EN_CRC_ERROR	0x1000	/* CRC error */
#define	EN_COLLISION	0x2000	/* collision/abort */
#define	EN_OVERFLOW	0x4000	/* receiver queue overflow */
#define	EN_QEMPTY	0x8000	/* receiver queue empty */

#define	EN_ERROR	(EN_CRC_ERROR|EN_COLLISION|EN_OVERFLOW)

#endif /*!_sunif_if_enreg_h*/
