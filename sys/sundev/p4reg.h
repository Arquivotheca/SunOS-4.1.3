/* @(#)p4reg.h	1.1 7/30/92 SMI */

/*
 * Copyright 1987 by Sun Microsystems, Inc.
 */

#ifndef	p4reg_DEFINED
#define	p4reg_DEFINED

/*
 * P4 frame buffer hardware definitions.
 */

/* P4 register bit definitions */
/*
 * P4_REG_DIAG is the bit 31 read back
 * P4_REG_READBACK_CLR is bit 2 reak back
 * This means writing to bit 32/2 and read them back at 7/2
 *
 * P4_REG_SYNC synchronize the ramdac, we should write 1 to it
 * followed by writing 0 immediately.  It obsoletes P4_REG_RESET.
 *
 * Bit 0 reads back as P4_REG_FIRSTHALF which is asserted during the
 * first half of the vertical retrace and zero otherwise.
 *
 * P4_REG_INTCLR clear the current interrupt so that we won't get
 * interrrupt immediately after we enable it.
 */
#define P4_REG_DIAG		0x80	/* diagnosis bit */
#define P4_REG_READBACKCLR	0x40
#define P4_REG_VIDEO		0x20	/* video enable */
#define P4_REG_SYNC		0x10	/* ramdac sync */
#define P4_REG_VTRACE		0x08	/* vtrace period */
#define	P4_REG_INT		0x04	/* interrupt detect */
#define	P4_REG_INTCLR		0x04	/* interrupt clear */
#define	P4_REG_INTEN		0x02	/* interrupt enable */
#define P4_REG_FIRSTHALF	0x01	/* first half of vtrace */
#define P4_REG_RESET		0x01	/* device reset */

/* frame buffer type mask (allows 16 types, 16 resolutions) */
#define	P4_ID_MASK		0xf0

/* base frame buffer type codes */
#define	P4_ID_BW		0	/* memory monochrome */

#define P4_ID_FASTCOLOR		0x60	/* accelerated 8-bit color */
#define P4_ID_COLOR8P1		0x40	/* 8-bit color + overlay */
#define P4_ID_COLOR24		0x45	/* 24-bit color + overlay */

/* resolution codes -- added to base types above */
#define	P4_ID_640X480		5
#define	P4_ID_1152X900		1
#define	P4_ID_1024X1024		2
#define	P4_ID_1280X1024		3
#define	P4_ID_1600X1280		0
#define	P4_ID_1440X1440		4

#define	P4_ID_RESCODES		7

/* offset from P4 register to monochrome memory plane */
#define	P4_BW_OFF		0x100000

/* offsets from P4 register to color f.b. pieces */
#define P4_COLOR_OFF_OVERLAY	0x00100000
#define P4_COLOR_OFF_ENABLE	0x00300000
#define P4_COLOR_OFF_COLOR	0x00500000
#define P4_COLOR_OFF_CMAP	0xfff00000
#define P4_COLOR_OFF_LUT	(-0x100000)

#endif	!p4reg_DEFINED
