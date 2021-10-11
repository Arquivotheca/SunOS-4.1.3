/*      @(#)eccreg.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _sun4_eccreg_h
#define _sun4_eccreg_h

#define	OBIO_ECCREG0_ADDR 0xFF1E0000	/* addr in obio space - sunrise */
#define	OBIO_ECCREG1_ADDR 0xFF000000	/* addr in obio space - sunray */

#define MAX_ECC		6		/* max number of ECC memory cards */

#define ECCREG_ADDR	(MDEVBASE + 0x4000)	/* virtual address mapped to */

#ifdef LOCORE
#define	ECCREG ECCREG_ADDR
#else
#define	ECCREG ((union eccreg *)ECCREG_ADDR)
union eccreg {
	struct sirius {
		u_int	ena;
		u_int	syn;
		u_int	dreg1;
		u_int	dreg2;
		u_char	pad[64 - (4 * sizeof (u_int))];
	} ecc_s;
	struct moonshine {
		u_int	ena;
		u_int	err;
		u_int	syn;
		u_char	pad0[32 - (3 * sizeof (u_int))];
		u_int	dreg1;
		u_int	dreg2;
		u_char	pad1[32 - (2 * sizeof (u_int))];
	} ecc_m;
};
#endif /* LOCORE */

/*
 * since access to memory board registers is limited
 * to a particular sized operation, we define a bunch of masks
 */

/* sirius ecc memory enable register */
/* <31..29>, r/o - board type identifier */
#define ENA_TYPE_MASK		0xE0000000
#define TYPE0			0x0
/* <28, 27>, r/w - operation mode */
#define ENA_MODE_MASK		0x18000000
#define ENA_MODE_NORMAL		0
#define ENA_MODE_DIAG_GENERATE	0x08000000
#define ENA_MODE_DIAG_DETECT	0x10000000
#define ENA_MODE_DIAG_INIT	0x18000000
/* <26..25>, r/o - board size */
#define ENA_BDSIZE_MASK		0x06000000
#define ENA_BDSIZE_4MB		0
#define ENA_BDSIZE_8MB		0x02000000
#define ENA_BDSIZE_16MB		0x04000000
#define ENA_BDSIZE_32MB		0x06000000
/* <24>, r/w - enable refresh scrub cycle */
#define ENA_SCRUB_MASK		0x01000000
/* <23>, r/w - enable mem bus ecc reporting */
#define ENA_BUSENA_MASK		0x00800000
/* <22>, r/w - overall board enable */
#define ENA_BOARDENA_MASK	0x00400000
/* <21..16>, r/w - base address <A27..A22> */
#define ENA_ADDR_MASK		0x003F0000
/* <21..15>, r/w - base address <A29..A23> */
#define ENA_ADDR_MASKL		0x003F8000
/* left logical shift to become <A27..A22> */
#define ENA_ADDR_SHIFT		(22-16)
/* left logical shift to become <A29..A24> */
#define ENA_ADDR_SHIFTL		(23-16)
/* <14> r/w P2.CANCEL enable */
#define ENA_CANCEL		0x00004000
/* <13> r/w 1 GB Enable */
#define ENA_1GB			0x00002000
/* <12..0>, non-exist */

/* syndrome */
/* <31..24>, r/o - syndrome for first error */
#define SY_SYND_MASK		0xFF000000
/* right shift to printf in %b format */
#define SY_SYND_SHIFT		24
/* <23>, reserved, read as 0 */
/* <22..1>, r/o - real addr bits <A24..A3> */
#define SY_ADDR_MASK		0x007FFFFE
/* left logical shift to become <24..A3> */
#define SY_ADDR_SHIFT		(3-1)
/* <0>, r/o - correctable error */
#define SY_CE_MASK		0x00000001

/* eccdiag_reg1 */
/* <31..24>, w/o - don' care */
/* <23>, w/o - check bit 32 (data D<71>) */
#define DR_CB32_MASK		0x00800000
/* <22..7>, w/o - don't care */
/* <6>, w/o - check bit 16 (data D<70>) */
#define DR_CB16_MASK		0x00000040
/* <5..0>, w/o - don't care */

/* eccdiag_reg2 */
/* <31..6>, w/o - don't care */
/* <5>, w/o - check bit 8  (data D<69>) */
#define DR_CB8_MASK		0x00000020
/* <4>, w/o - check bit 4  (data D<68>) */
#define DR_CB4_MASK		0x00000010
/* <3>, w/o - check bit 2  (data D<67>) */
#define DR_CB2_MASK		0x00000008
/* <2>, w/o - check bit 1  (data D<66>) */
#define DR_CB1_MASK		0x00000004
/* <1>, w/o - check bit 1  (data D<65>) */
#define DR_CB0_MASK		0x00000002
/* <0>, w/o - check bit X  (data D<64>) */
#define DR_CBX_MASK		0x00000001

/* 
 * To compute the U number of the bad chip on 8meg memory boards we use
 * bits A3 and A22 of the physical address and the bit number calculated
 * from the syndrome.  The bit number forms the last two digits of the
 * U number and the address bits are decoded as follows to get the first
 * two digits
 * Physical address lines	Data bit 	U number 
 *  	 A3	A22		number		prefix
 * 
 *	 0	0		=<35		U15xx
 *	 1	0		=<35		U17xx
 * 	 0	1		=<35		U19xx
 *	 1	1		=<35		U21xx
 * 
 *	 0	0		=>36		U14xx
 *	 1	0		=>36		U16xx
 *	 0	1		=>36		U18xx
 *	 1	1		=>36		U20xx
 */
#define MEG8  0x7fffff		/* mask for physical address bits on board */
#define MEG16 0xffffff		/* mask for physical address bits */
#define MEG32 0x1ffffff		/* mask for physical address bits */
#define LOWER 36		/* divider between upper and lower memory */
#define ECC_BITS 0x400008	/* mask for physical address bits */
#define U14XX 0x0
#define U15XX 0x0
#define U16XX 0x8
#define U17XX 0x8
#define U18XX 0x400000
#define U19XX 0x400000
#define U20XX 0x400008
#define U21XX 0x400008

/* 
 * To compute the U number of the bad chip on 32/16meg memory boards we use
 * bits A3 and A24 of the physical address and the bit number calculated
 * from the syndrome.  The bit number forms the last two digits of the
 * U number and the address bits are decoded as follows to get the first
 * two digits
 * Physical address lines       Data bit        U number
 *       A3     A24             number          prefix
 *
 *       0      0               0-35            U15xx
 *       1      0               0-35            U17xx
 *       0      1               0-35            U19xx
 *       1      1               0-35            U21xx
 *
 *       0      0               36-72           U14xx
 *       1      0               36-72           U16xx
 *       0      1               36-72           U18xx
 *       1      1               36-72           U20xx
 */
#define PEG_ECC_BITS 0x1000008	/* mask for physical address bits */
#define PEG_U18XX 0x1000000
#define PEG_U19XX 0x1000000
#define PEG_U20XX 0x1000008
#define PEG_U21XX 0x1000008

#define SYNDERR_BITS	"\20\10S32\7S16\6S8\5S4\4S2\3S1\2S0\1SX"

/*
 * Moonshine memory boards have a "similar but not quite the same"
 * set of bits and registers for control and error handling
 */

/* moonshine ecc memory enable register */
#define MMB_BASE_ADDR	0x0000003f	/* base address */
#define MMB_ENA_BOARD	0x00000040	/* enable memory board */
#define MMB_ENA_ECC	0x00000080	/* enable ecc */
#define MMB_ENA_SCRUB	0x00000100	/* enable scrub */
#define MMB_BDSIZ	0x00000600	/* board size (read only) */
					/* 0 - 32M */
					/* 1 - 128M */
					/* 2,3 - reserved */
#define MMB_BDSIZ_SHIFT	9		/* shift to right justify board size */
#define MMB_ENS0	0x00000800	/* ecc chip mode pin 0 */
#define	MMB_ENS1	0x00001000	/* ecc chip mode pin 1 */
#define MMB_BOARDID	0x0000e000	/* board id (read only) */

/* moonshine ecc error register (read only) */
#define MMB_CE_ERROR	0x0000000f	/* a correctable error has occured */
#define MMB_BANK	0x0000000f	/* bank number which had error */
#define MMB_PA_32	0x0fffff00	/* phys address bits for first ce */
#define MMB_PA_128	0x3fffff00	/* phys address bits for first ce */
					/* really pa bits <26:5> */
#define MMB_PA_SHIFT	3		/* shift to make pa bits into pa */

/* moonshine syndrone/check bit register (read only) */
#define MMB_SC0		0x000000ff	/* s/c bits for bank 0 */
#define MMB_SC1		0x0000ff00	/* s/c bits for bank 1 */
#define MMB_SC2		0x00ff0000	/* s/c bits for bank 2 */
#define MMB_SC3		0xff000000	/* s/c bits for bank 3 */

#endif /*!_sun4_eccreg_h*/
