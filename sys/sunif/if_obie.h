/*    @(#)if_obie.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Register definitions for the Sun-2 On-board version of the
 * Intel EDLC based Ethernet interface.
 * FYI:	Reset: write zeros to register
 *	Must poll to check for obie_buserr
 */

#ifndef _sunif_if_obie_h
#define _sunif_if_obie_h

struct obie_device {
	u_char	obie_noreset	: 1;	/* R/W: Ethernet chips reset */
	u_char	obie_noloop	: 1;	/* R/W: loopback */
	u_char	obie_ca		: 1;	/* R/W: channel attention */
	u_char	obie_ie		: 1;	/* R/W: interrupt enable */
	u_char			: 1;	/* R/O: unused */
	u_char	obie_level2	: 1;	/* R/O: 0=Level 1 xcvr, 1=Level 2 */
	u_char	obie_buserr	: 1;	/* R/O: Ether DMA got bus error */
	u_char	obie_intr	: 1;	/* R/O: interrupt request */
};

#endif /*!_sunif_if_obie_h*/
