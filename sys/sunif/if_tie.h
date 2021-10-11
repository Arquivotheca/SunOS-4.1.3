/*    @(#)if_tie.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef _sunif_if_tie_h
#define _sunif_if_tie_h

/*
 * This is the number of bytes occupied by the 3E interface buffer.
 */
#define IE_TE_MEMSIZE	0x20000


/*
 * Register definitions for the Sun 3E Ethernet board.  This interface
 * uses the Intel 82586 chip, and is similar to the Sun Multibus
 * board.
 *
 * The prefix for the 3E specific structures is tie (the t stands for
 * three).
 *
 *
 * The primary differences between the two boards are:
 * - the 3E has no page maps
 * - the 3E has no parity checking
 * - the 3E has no id prom
 *
 * FYI:	Board ignores high order nibble of chip generated addresses.
 *	Reset chip: tie_reset = 1; delay 10us; *(char *)&tie_reset = 0;
 */

struct tie_device {
	struct {
		u_char	ties_reset	: 1;	/* board reset */
		u_char	ties_noloop	: 1;	/* loopback disable */
		u_char	ties_ca		: 1;	/* channel attention */
		u_char	ties_ie		: 1;	/* interrupt enable */
		u_char	ties_nu1	: 1;	/* was: parity error enable */
		u_char			: 1;	/* unused */
		u_char			: 1;	/* was: parity error */
		u_char	ties_intr	: 1;	/* interrupt request */
		u_char			: 2;	/* unused */
		u_char			: 1;	/* was: P2 bus enabled */
		u_char			: 1;	/* was: true if 256K rams */
		u_char			: 4;	/* was: hi bits of mem port */
	} tie_status;
	char	tie_skip[0xf];
	u_char	tie_ivec;			/* interrupt vector */
#define	tie_reset	tie_status.ties_reset
#define	tie_noloop	tie_status.ties_noloop
#define	tie_ca		tie_status.ties_ca
#define	tie_ie		tie_status.ties_ie
#define	tie_nu1		tie_status.ties_nu1
#define	tie_pie		tie_status.ties_pie
#define	tie_pe		tie_status.ties_pe
#define	tie_intr	tie_status.ties_intr
};

#endif /*!_sunif_if_tie_h*/
