/*	@(#)vpreg.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Registers for Ikon 10071-5 Multibus/Versatec interface
 * Only low byte of each word is used. (16 words total)
 * Warning - read bits are not identical to written bits.
 */

#ifndef _sundev_vpreg_h
#define _sundev_vpreg_h

struct vpdevice {
	u_short	vp_status;	/* 00: mode(w) and status(r) */
	u_short	vp_cmd;		/* 02: special command bits (w) */
	u_short	vp_pioout;	/* 04: PIO output data (w) */
	u_short	vp_hiaddr;	/* 06: hi word of Multibus DMA address (w) */
	u_short	vp_icad0;	/* 08: ad0 of 8259 interrupt controller */
	u_short	vp_icad1;	/* 0A: ad1 of 8259 interrupt controller */
	/* The rest of the fields are for the 8237 DMA controller */
	u_short	vp_addr;	/* 0C: DMA word address */
	u_short	vp_wc;		/* 0E: DMA word count */
	u_short	vp_dmacsr;	/* 10: command and status */
	u_short	vp_dmareq;	/* 12: request */
	u_short	vp_smb;		/* 14: single mask bit */
	u_short	vp_mode;	/* 16: dma mode */
	u_short	vp_clrff;	/* 18: clear first/last flip-flop */
	u_short	vp_clear;	/* 1A: DMA master clear */
	u_short	vp_clrmask;	/* 1C: clear mask register */
	u_short	vp_allmask;	/* 1E: all mask bits */
};
/* vp_status bits (read) */
#define	VP_IS8237	0x80	/* 1 if 8237 (sanity checker) */
#define	VP_REDY		0x40	/* printer ready */
#define	VP_DRDY		0x20	/* printer and interface ready */
#define	VP_IRDY		0x10	/* interface ready */
#define	VP_PRINT	0x08	/* print mode */
#define	VP_NOSPP	0x04	/* not in SPP mode */
#define	VP_ONLINE	0x02	/* printer online */
#define	VP_NOPAPER	0x01	/* printer out of paper */
/* vp_status bits (written) */
#define	VP_PLOT		0x02	/* enter plot mode */
#define	VP_SPP		0x01	/* enter SPP mode */

/* vp_cmd bits */
#define	VP_RESET	0x10	/* reset the plotter and interface */
#define	VP_CLEAR	0x08	/* clear the plotter */
#define	VP_FF		0x04	/* form feed to plotter */
#define	VP_EOT		0x02	/* EOT to plotter */
#define	VP_TERM		0x01	/* line terminate to plotter */

#define	VP_DMAMODE	0x47	/* magic for vp_mode */

#define	VP_ICPOLL	0x0C
#define	VP_ICEOI	0x20

#endif /*!_sundev_vpreg_h*/
