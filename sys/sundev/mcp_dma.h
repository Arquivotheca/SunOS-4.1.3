/*	@(#)mcp_dma.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _sundev_mcp_dma_h
#define _sundev_mcp_dma_h

#define TX_DIR  1
#define RX_DIR  0
#define SCC_DMA 0
#define PR_DMA  1

#define CHIP(x) ((x) >> 2)
#define CHAN(x) ((x) & 0x03)

struct dmachan *dmagetchan();

/*
 * Register and bit definitions for Intel 8237
 */
#define	DMA_OFF_CMD	8	/* w - command port */
#define	DMA_OFF_STAT	8	/* r - status port */
#define	DMA_OFF_REQ	9	/* w - single request port */
#define	DMA_OFF_MASK	10	/* w - single mask port */
#define	DMA_OFF_MODE	11	/* w - mode port */
#define	DMA_OFF_CLRFF	12	/* w - clear first/last flip-flop */
#define	DMA_OFF_RESET	13	/* w - master clear */
#define	DMA_OFF_ADDR(dma)	(dma->d_myport)
#define	DMA_OFF_COUNT(dma)	(dma->d_myport+1)

#define	DMA_CMD_DISABLE	0x04	/* controller disable */
#define	DMA_CMD_COMP	0x08	/* compressed timing */
#define	DMA_CMD_ROTPRI	0x10	/* rotating priority */
#define	DMA_CMD_EXTWRT	0x20	/* extended write */
#define	DMA_CMD_DREQLOW 0x40	/* DREQ active low */
#define	DMA_CMD_DACKHI	0x80	/* DACK active high */

#define	DMA_MODE_WRITE	0x04	/* direction is write */
#define	DMA_MODE_READ	0x08	/* direction is read */
#define	DMA_MODE_AUTO	0x10	/* autoinit enable */
#define	DMA_MODE_DECR	0x20	/* decrement addresses */

#define DMA_MODE_DEMAND  0x00    /* demand transfer mode select */
#define DMA_MODE_SINGLE  0x40    /* single transfer mode select */
#define DMA_MODE_BLOCK   0x80    /* block transfer mode select */
#define DMA_MODE_CASCADE 0xC0    /* cascade transfer mode select */k

#define	DMA_MASK_CLEAR	0	/* channel + clear mask bit */
#define	DMA_MASK_SET	4	/* channel + set mask bit */

#endif /*!_sundev_mcp_dma_h*/
