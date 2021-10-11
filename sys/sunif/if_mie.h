/*	@(#)if_mie.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#ifndef _sunif_if_mie_h
#define _sunif_if_mie_h

#define	IEVVSIZ		1024		/* pages in page map --
					   vv as in virtual virtual */
#define IEPMEMSIZ	(256*1024)	/* physical memory */
#define IEPAGSIZ	1024
#define IEPAGSHIFT	10

/*
 * Register definitions for the Sun Multibus version of the
 * Intel EDLC based Ethernet interface.
 * FYI:	Board ignores high order nibble of chip generated addresses.
 *	Reset chip: mie_reset = 1; delay 10us; *(char *)&mie_reset = 0;
 */
struct mie_device {
	struct miepg {
		u_short	mp_swab	: 1;	/* 68000 byte order */
		u_short		: 1;	/* unused */
		u_short	mp_p2mem: 1;	/* p2 or onboard memory */
		u_short		: 1;	/* unused */
		u_short	mp_pfnum: 12;	/* page number */
	} mie_pgmap[IEVVSIZ];
	short	mie_prom[32];		/* 32 bytes - low bytes of words */
	struct {
		u_char	mies_reset	: 1;	/* board reset */
		u_char	mies_noloop	: 1;	/* loopback disable */
		u_char	mies_ca		: 1;	/* channel attention */
		u_char	mies_ie		: 1;	/* interrupt enable */
		u_char	mies_pie	: 1;	/* parity error enable */
		u_char			: 1;	/* unused */
		u_char	mies_pe		: 1;	/* parity error */
		u_char	mies_intr	: 1;	/* interrupt request */
		u_char			: 2;	/* unused */
		u_char	mies_p2mem	: 1;	/* P2 bus enabled */
		u_char	mies_bigram	: 1;	/* true if 256K rams */
		u_char	mies_mbmhi	: 4;	/* hi bits of mem port */
	} mie_status;
#define	mie_reset	mie_status.mies_reset
#define	mie_noloop	mie_status.mies_noloop
#define	mie_ca		mie_status.mies_ca
#define	mie_ie		mie_status.mies_ie
#define	mie_pie		mie_status.mies_pie
#define	mie_pe		mie_status.mies_pe
#define	mie_intr	mie_status.mies_intr
#define	mie_p2mem	mie_status.mies_pg2mem
#define	mie_bigram	mie_status.mies_bigram
#define	mie_mbmhi	mie_status.mies_mbmhi
	u_short			: 16;	/* unused */
	u_char			: 7;	/* unused */
	u_char	mie_peack	: 1;	/* dummy bit for pe ack */
	u_char	mie_pesrc	: 1;	/* source of parity error */
	u_char	mie_pebyte	: 1;	/* which byte caused parity error */
	u_char			: 2;	/* unused */
	u_int	mie_erraddr	: 20;	/* error address */
};

#endif /*!_sunif_if_mie_h*/
