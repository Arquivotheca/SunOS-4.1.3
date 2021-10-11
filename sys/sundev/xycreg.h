/*	@(#)xycreg.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _sundev_xycreg_h
#define _sundev_xycreg_h

/*
 * Common Xylogics 450/472 declarations
 */

/*
 * I/O space registers - byte accesses only 
 */
struct xydevice {		/* I/O space registers (at EE40) */
	u_char	xy_iopbrel[2];	/* 1,0 - IOPB relocation */
	u_char	xy_iopboff[2];	/* 3,2 - IOPB offset */
	u_char	xy_resupd;	/* 5 - reset/update */
	u_char	xy_csr;		/* 4 - controller status register */
};

/*
 * Macros to massage our address into 8086 style.
 */
#define	XYOFF(a)	((int)(a) & 0xFFFF)
#define	XYREL(xy, a)	((xy->xy_csr & XY_ADDR24) ? \
			(((int)(a) & 0xFF0000) >> 16) : \
			(((int)(a) & 0xF0000) >> 4))
#define XYNEWREL(f, a)	(((f) & XY_C_24BIT) ? \
			(((int)(a) & 0xFF0000) >> 16) : \
			(((int)(a) & 0xF0000) >> 4))

/*
 * xy_csr bits
 */
#define	XY_GO		0x80	/* w - start operation */
#define XY_BUSY		0x80	/* r - operation in progress */
#define XY_ERROR	0x40	/* r - error occurred / w450 - clear error */
#define	XY_DBLERR	0x20	/* r - double error */
#define	XY_INTR		0x10	/* r - interrupting / w450 - clear interrupt */
#define	XY_ADDR24	0x08	/* r450 - addressing mode */
#define	XY_ATTN		0x04	/* w450 - attention request */
#define	XY_ACK		0x02	/* r450 - attention acknowledge */
#define	XY_DREADY	0x01	/* r - drive ready */

/*
 * Controller types
 */
#define	XYC_440		0
#define	XYC_450		1
#define	XYC_472		2

#endif /*!_sundev_xycreg_h*/
