#ifndef lint
static	char sccsid[] = "@(#)mcp_dma.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include <sys/types.h>

#include <sundev/zsreg.h>
#include <sundev/mcpreg.h>
#include <sundev/mcp_dma.h>
#include  "mcp.h"

#define NDMA_CHIPS      6
#define NDMA            (NMCP*NDMA_CHIPS)

int mcpdma_debug = 0;
#define printA if(mcpdma_debug)printf

struct dma_chip dma_chip[NDMA];


mcp_dmaattach(mcp, unit)
	register struct mcp_device *mcp;
	register int unit;   /* which mcp board */
{
	register int i, j;
	register u_char *base = (u_char *)mcp;

	/* Chip[5].channels[1,2,3] are "Reserved". Initialize them anyway. */
	for (i = unit * NDMA_CHIPS, j = 0; j < NDMA_CHIPS; i++, j++) {
		mcp_dmainit(&dma_chip[i], &mcp->dmas[j], (j == 4) ? RX_DIR : TX_DIR);
		dma_chip[i].ram_base = base;
	}
}

mcp_dmainit(dc, base, flag)
	struct dma_chip *dc;
	struct dma_device *base;
	char flag;
{
	struct dma_chan *dma;
	short i, dir;
	struct addr_wc  *port;

	dc->d_ioaddr = base;
	dc->d_mask = 0;
	base->reset = 1;
	base->csr = DMA_CMD_DREQLOW;
	base->clrff = 1;
	for (i = 0; i < 4; i++) {
		dma = &dc->d_chans[i];
		dma->d_chip = dc;
		dma->d_chan = i;
		dma->d_myport = &base->addr_wc[i];
		dma->d_dir = flag;	/* 1 - xmit ,  0 - recv */
		if (dma->d_dir == TX_DIR)
			dir = DMA_MODE_READ | DMA_MODE_SINGLE;	/* Yes, xmit is read */
		else
			dir = DMA_MODE_WRITE | DMA_MODE_SINGLE;	/* Recv is write */
		base->mode = i + dir;
		port = &base->addr_wc[i];
		port->baddr = 0;            /* write the lower byte */
		port->baddr = 0;            /* write the high byte */
		port->wc = 0xFF;            /* write the lower byte */
		port->wc = 0xFF;            /* write the high byte */
	}
}

struct dma_chan *
mcp_dmagetchan(port, dir, unit, scc)
register int port, dir, unit;
int scc;
{
	if (scc == PR_DMA) {
		if (dir == TX_DIR)
			return (&dma_chip[5 + (unit * NDMA_CHIPS)].d_chans[port]);
		return (0);
	}
	if (dir == TX_DIR)
		return (&dma_chip[CHIP(port) + (unit * NDMA_CHIPS)].d_chans[CHAN(port)]);
	if (port < 4)
		return (&dma_chip[4 + (unit * NDMA_CHIPS)].d_chans[port]);
	return (0);
}

mcp_dmastart(dma, addr, len)
	struct dma_chan *dma;
	char *addr;
	short len;
{
	long linaddr;
	register struct addr_wc *port;

	linaddr = (long)addr - (long)dma->d_chip->ram_base;
	dma->d_chip->d_ioaddr->mask = DMA_MASK_SET + dma->d_chan;
	dma->d_chip->d_ioaddr->clrff =  1;
	port = dma->d_myport;
	port->baddr = (short)linaddr & 0x0FF;  /* write the lower byte */
	port->baddr = ((short)linaddr >>8) & 0x0FF;  /* write the high byte*/
	len--;
	port->wc = len & 0xFF ;
	port->wc = (len >> 8) & 0xFF;
	dma->d_chip->d_mask |= 1 << dma->d_chan;
	dma->d_chip->d_ioaddr->mask = DMA_MASK_CLEAR + dma->d_chan;
    printA("dma started, count = %x\n", mcp_getwc(dma));
}

/*#ifdef notdef*/
mcp_getwc(dma)
	register struct dma_chan *dma;
{
	register struct addr_wc *port;
	u_char lo, hi;

	port = dma->d_myport;
	lo = port->wc;
	hi = port->wc;
	return ((hi << 8) + lo);
}
/*#endif */

mcp_dmahalt(dma)
	struct dma_chan *dma;
{
	struct addr_wc *port;
	short len;

	dma->d_chip->d_ioaddr->mask = DMA_MASK_SET + dma->d_chan;
	port = dma->d_myport;
	len = port->wc;
	len |= port->wc << 8;
	len += 1;
	if (len > 0)
		dma->d_chip->d_mask &= ~(1 << dma->d_chan);
	return (len);
}

mcp_dmadump(dma)
	struct dma_chan *dma;
{
	short mask = 0x11 << dma->d_chan;
	struct dma_chip *dc = dma->d_chip;
	struct addr_wc *port;
	short addr;

	printf("dma softbusy=%x hardbusy=%x",
	dc->d_mask & mask,
	dc->d_ioaddr->csr & mask);
	port = dma->d_myport;
	addr = port->baddr;
	addr |= port->baddr << 8;
	printf(" addr=%x", addr);
	printf(" count=%d\n", mcp_dmahalt(dma));

}

mcp_dmawait(dma)
	struct dma_chan *dma;
{
	short mask = 1 << dma->d_chan;

	while ((dma->d_chip->d_ioaddr->csr & mask) == 0)
		;
}

mcp_dmadone(dma)
   	register struct dma_chan *dma;
{
	register struct dma_chip *dc = dma->d_chip;
	register u_char  mask = 1 << dma->d_chan;
	register u_char status;

    status = dc->d_ioaddr->csr; 
	return(status);

	/*if (dc->d_ioaddr->csr & mask){
		dc->d_mask  &= ~mask;
		return (1);
	}
	else
		return(0);
	*/
}
