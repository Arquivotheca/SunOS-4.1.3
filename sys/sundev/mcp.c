#ifndef lint
static char     sccsid[] = "@(#)mcp.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Sun MCP Multiprotocol Communication Processor
 * Sun ALM-2 Asynchronous Line Multiplexer
 * 
 * Common code for all MCP/ALM-2 modules, including boot-time autoconf,
 * interrupt, and on-board DMA routines.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/termios.h>
#include <sys/buf.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/tty.h>
#include <sys/clist.h>

#include <sundev/mbvar.h>
#include <sundev/zsreg.h>
#include <sundev/mcpreg.h>
#include <sundev/mcpcom.h>
#include <sundev/mcpcmd.h>

#define DMADONE(x)	((x)->d_chip->d_ioaddr->csr)

int		mcpdebug = 0;
#define printD		if(mcpdebug)(void)printf

extern struct mcpcom mcpcom[];
extern struct mb_driver mcpdriver;
extern struct mcpops *mcp_proto[];
extern struct dma_chip dma_chip[];
extern struct mb_device *mcpinfo[];
extern int	nmcpline;
extern int	mcpsoftCAR[];

extern int	mcpr_attach( /* struct mb_device *md; */);

int		mcpprobe( /* caddr_t reg, int unit */ );
int		mcpattach( /* struct mb_device *md */ );
static void	cioattach(/* struct ciochip *cp, struct mcp_device *mcpaddr */);
int		mcpintr( /* int unit */ );
static void	mcp_dmaattach( /* struct mcp_device *mcp, int unit */ );
static void	mcp_dmainit(/* struct dma_chip *dc, struct dma_device *base, char flag */ );
struct dma_chan *mcp_dmagetchan( /* int port, dir, unit, scc */ );
void		mcp_dmastart(/* struct dma_chan *dma, char *addr, short len */);
u_short		mcp_getwc( /* struct dma_chan *dma */ );
u_short		mcp_dmahalt( /* struct dma_chan *dma */ );

int
mcpprobe(reg, unit)
	caddr_t         reg;
	int             unit;
{
	register struct mcp_device *mcpaddr = (struct mcp_device *) reg;
	register struct mcpcom *zs = &mcpcom[unit * 16];
	register int    i;

	printD(" in mcpprobe, reg %x, unit %x \n", reg, unit);
	if (peek((short *) &mcpaddr->reset_bid) == -1) {
		printD("probe failed \n");
		return (0);
	}
	/*
	 * Reset the board. Note that after reset, the board interrupts are
	 * diabled. Pick them up in mcpattach().
	 */
	if (poke((short *) &mcpaddr->reset_bid, 0) == -1)
		return (0);

	printD("devctl = %x\n", mcpaddr->devctl[1].ctr);

	for (i = 0; i < 16; i++, zs++) {
		zs->zs_unit = i;
		zs->mc_unit = unit;
	}
	mcpcom[nmcpline + unit].mc_unit = unit;	/* zs_unit doesn't matter to
						 * printer */
	return (sizeof(struct mcp_device));
}

int
mcpattach(md)
	register struct mb_device *md;
{
	register struct mcpcom *zs = &mcpcom[md->md_unit * 16];
	register struct mcp_device *mcpaddr = (struct mcp_device *) md->md_addr;
	register struct mcpops *zso;
	struct ciochip *cp = &mcpaddr->cio;
	register int    j, port;
	register u_char vector;

	mcpaddr->ivec = md->md_intr->v_vec;
	*(md->md_intr->v_vptr) = (int) (md->md_unit * 16);

	cioattach(cp, mcpaddr);
	mcp_dmaattach(mcpaddr, md->md_unit);
	mcpsoftCAR[md->md_unit] = md->md_flags;

	/*
	 * initialize each port of MCP
	 * includes:
	 *  disable xoff, clear one character buffer, set up links between
	 *  data structures, and initialize dma channel and enable master
	 *  interrupt and interrupt vector for SCC
	 * vector bits assignment:
	 *  bit 0 -- select CIO or SCC (0 == SCC)
	 *  bit 4 to 7 -- select SCC chip (0 to 8)
	 */

	for (port = 0, vector = 0; port < 16; port++, zs++) {
		zs->mcp_addr = mcpaddr;
		zs->zs_addr = &mcpaddr->sccs[port];
		zs->zs_flags = 0;
		zs->zs_rerror = 0;
		mcpaddr->xbuf[port].xoff = DISABLE_XOFF;
		if (port & 0x01) {
			SCC_WRITE(&zs->mcp_addr->sccs[port], 9,
			    ZSWR9_MASTER_IE | ZSWR9_VECTOR_INCL_STAT);
			SCC_WRITE(&mcpaddr->sccs[port], 2, vector);
			vector += 0x10;
		}
		zs->mcp_txdma = mcp_dmagetchan(zs->zs_unit, TX_DIR,
		    md->md_unit, SCC_DMA);
		zs->mcp_rxdma = mcp_dmagetchan(zs->zs_unit, RX_DIR,
		    md->md_unit, SCC_DMA);
		/* attach driver of individual protocol */
		for (j = 0; mcp_proto[j]; j++) {
			zso = mcp_proto[j];
			(*zso->mcpop_attach) (zs);
		}

	}

	/* Initialize printer port */
	(void) mcpr_attach(md);

	/* enable master interrupt for MCP */
	cp->portc_data = (cp->portc_data & 0xf) | MCP_IE;
}

static void
cioattach(cp, mcpaddr)
	register struct ciochip *cp;
	register struct mcp_device *mcpaddr;
{
	register u_char select;

	/* write a single zero to cio to clear the reset */
	cp->cio_ctr = 0;

	/* initialize cio port C. While here, detect RS232/449 interface */
	CIO_WRITE(cp, CIO_MICR, MASTER_INT_ENABLE);
	CIO_WRITE(cp, CIO_PC_DPPR, MCPRVMEINT | MCPRDIAG);
	CIO_WRITE(cp, CIO_PC_DDR, PORT0_RS232_SEL | PORT1_RS232_SEL);
	CIO_WRITE(cp, CIO_PC_SIOCR, ONES_CATCHER);
	if (((select = cp->portc_data) & PORT0_RS232_SEL) == 0) {
		(void) printf("***port 0 supports RS449 interface***\n");
		mcpaddr->devctl[0].ctr &= EN_RS449_TX;	
	}
	if ((select & PORT1_RS232_SEL) == 0) {
		(void) printf("***port 1 supports RS449 interface***\n");
		mcpaddr->devctl[1].ctr &= EN_RS449_TX;
	}
	cp->portc_data = (select & 0xf) & ~MCPRDIAG;	/* Ensure diag off */

	/* initialize port A of cio chip */
	CIO_WRITE(cp, CIO_PA_MODE, BIT_PORT_MODE);
	CIO_WRITE(cp, CIO_PA_DDR, ALL_INPUT);
	CIO_WRITE(cp, CIO_PA_DPPR, FIFO_NON_INVERT);
	CIO_WRITE(cp, CIO_PA_PP, FIFO_NOT_ONE);
	CIO_WRITE(cp, CIO_PA_PM, FIFO_EMPTY_INTR_ONLY);
	CIO_WRITE(cp, CIO_PA_CSR, PORT_INT_ENABLE);


	/* initialize port B of cio chip */
	CIO_WRITE(cp, CIO_PB_MODE, BIT_PORT_MODE);
	CIO_WRITE(cp, CIO_PB_DDR, ALL_INPUT);
	CIO_WRITE(cp, CIO_PB_SIOCR, EOP_ONE);
	CIO_WRITE(cp, CIO_PB_DPPR, EOP_INVERT | MCPRSLCT | MCPRPE);
	CIO_WRITE(cp, CIO_PB_PP, EOP_ONE);
	CIO_WRITE(cp, CIO_PB_PM, EOP_ONE);
	CIO_WRITE(cp, CIO_PB_CSR, PORT_INT_ENABLE);
	CIO_WRITE(cp, CIO_MCCR, MASTER_ENABLE);

	/*
	 * set vector for CIO
	 * vector bits assignment:
	 *  bit 0 -- select CIO or SCC   ( 1 == CIO)
	 *  bit 4 -- select port A or B  ( 0 == port A)
	 */
	CIO_WRITE(cp, CIO_PA_IVR, 0x01);
	CIO_WRITE(cp, CIO_PB_IVR, 0x11);

}

/*
 * Handle a level 4 interrupt
 * unit is a multiple of 16
 * i.e. unit=48 for mcp3
 */
int
mcpintr(unit)
	register int    unit;	/* which mcp board */
{
	register struct mcpcom *zs = &mcpcom[unit];
	register struct mcp_device *mcp = zs->mcp_addr;
	register struct ciochip *cio = &mcp->cio;
	register int    i;
	register u_char dvector;
	register u_char status;
	register u_char mask;
	register u_short data;
	int loops = -1;
	int bd = unit >> 4;
	int ln = unit & 15;

	do {
		++loops;
		/*
		 * If the vector corresponds to an action that requires
		 * us to empty the fifo first, and there is data in
		 * the FIFO, drain the data up to the next layer.
		 */
		dvector = mcp->devvector.ctr;
		if ((dvector == MCP_NOINTR) ||
		    (dvector == CIO_PAD4_FIFO_E) ||
		    ((dvector & 0x87) == SCC0_XSINT) ||	/* any XSINT */
		    ((dvector & 0x87) == SCC0_SRINT) ||	/* any SRINT */
		    (dvector == CIO_PAD5_FIFO_HF))
			while ((data = mcp->fifo[0]) != FIFO_EMPTY) {
				++loops;
				ln = data >> 8;
				if (ln > 15) {
					printf("mcp%d: unexpected data 0x%x from fifo (dvector=%x); probable hardware fault.\n",
					       bd, data, dvector);
					continue;
				}
				zs = &mcpcom[unit + ln];
				if (zs->mcp_ops) (*zs->mcp_ops->mcpop_rxchar) (zs, data & 0xff);
			}
		/*
		 * Process the interrupt vector.
		 */
		ln = (dvector >> 3) & 15; /* most common line encoding */
		switch (dvector) {

		case CIO_PBD0_TXEND:
		case CIO_PBD1_TXEND:
		case CIO_PBD2_TXEND:
		case CIO_PBD3_TXEND:
			zs = &mcpcom[unit + CIO_MCPBASE(dvector)];
			/* reset & clear the source of interrupt */
			CIO_DMARESET(cio, dvector);
			CIO_WRITE(cio, CIO_PB_CSR, CIO_CLRIP);
			status = DMADONE(zs->mcp_txdma);
			for (i = 0; i < 4; zs++, i++) {
				if (zs->zs_flags & MCP_WAIT_DMA) {
					mask = 1 << zs->mcp_txdma->d_chan;
					if (status & mask) {
						zs->mcp_txdma->d_chip->d_mask &= ~mask;
						zs->zs_flags &= ~MCP_WAIT_DMA;
						if (zs->mcp_ops) (*zs->mcp_ops->mcpop_txend) (zs);
					}
				}
			}
			break;

		case SCC0_XSINT:
		case SCC1_XSINT:
		case SCC2_XSINT:
		case SCC3_XSINT:
		case SCC4_XSINT:
		case SCC5_XSINT:
		case SCC6_XSINT:
		case SCC7_XSINT:
		case SCC8_XSINT:
		case SCC9_XSINT:
		case SCC10_XSINT:
		case SCC11_XSINT:
		case SCC12_XSINT:
		case SCC13_XSINT:
		case SCC14_XSINT:
		case SCC15_XSINT:
			zs = &mcpcom[unit + ln];
			if (zs->mcp_ops) (*zs->mcp_ops->mcpop_xsint) (zs);
			zs->zs_addr->zscc_control = ZSWR0_CLR_INTR;
			DELAY(2);
			break;

		case SCC0_SRINT:
		case SCC1_SRINT:
		case SCC2_SRINT:
		case SCC3_SRINT:
		case SCC4_SRINT:
		case SCC5_SRINT:
		case SCC6_SRINT:
		case SCC7_SRINT:
		case SCC8_SRINT:
		case SCC9_SRINT:
		case SCC10_SRINT:
		case SCC11_SRINT:
		case SCC12_SRINT:
		case SCC13_SRINT:
		case SCC14_SRINT:
		case SCC15_SRINT:
			zs = &mcpcom[unit + ln];
			if (zs->mcp_ops) (*zs->mcp_ops->mcpop_srint) (zs);
			zs->zs_addr->zscc_control = ZSWR0_CLR_INTR;
			DELAY(2);
			break;

		case SCC0_TXINT:
		case SCC1_TXINT:
		case SCC2_TXINT:
		case SCC3_TXINT:
		case SCC4_TXINT:
		case SCC5_TXINT:
		case SCC6_TXINT:
		case SCC7_TXINT:
		case SCC8_TXINT:
		case SCC9_TXINT:
		case SCC10_TXINT:
		case SCC11_TXINT:
		case SCC12_TXINT:
		case SCC13_TXINT:
		case SCC14_TXINT:
		case SCC15_TXINT:
			zs = &mcpcom[unit + ln];
			if (zs->mcp_ops) (*zs->mcp_ops->mcpop_txint) (zs);
			zs->zs_addr->zscc_control = ZSWR0_CLR_INTR;
			DELAY(2);
			break;

		case CIO_PBD4_RXEND:
			zs = &mcpcom[unit];
			/* reset & clear the source of interrupt */
			CIO_DMARESET(cio, dvector);
			CIO_WRITE(cio, CIO_PB_CSR, CIO_CLRIP);
			status = DMADONE(zs->mcp_rxdma);
			for (i = 0; i < 4; zs++, i++) {
				mask = 1 << zs->mcp_rxdma->d_chan;
				if (status & mask) {
					zs->mcp_rxdma->d_chip->d_mask &= ~mask;
					if (zs->mcp_ops) (*zs->mcp_ops->mcpop_rxend) (zs);
				}
			}
			break;

		case SCC0_RXINT:
		case SCC1_RXINT:
		case SCC2_RXINT:
		case SCC3_RXINT:
		case SCC4_RXINT:
		case SCC5_RXINT:
		case SCC6_RXINT:
		case SCC7_RXINT:
		case SCC8_RXINT:
		case SCC9_RXINT:
		case SCC10_RXINT:
		case SCC11_RXINT:
		case SCC12_RXINT:
		case SCC13_RXINT:
		case SCC14_RXINT:
		case SCC15_RXINT:
			zs = &mcpcom[unit + ln];
			if (zs->mcp_ops) (*zs->mcp_ops->mcpop_rxint) (zs);
			zs->zs_addr->zscc_control = ZSWR0_CLR_INTR;
			DELAY(2);
			break;

		case CIO_PBD5_PPTX:
			zs = &mcpcom[nmcpline + bd];
			/* reset & clear the source of interrupt */
			cio->portb_data = ~0x20;
			CIO_WRITE(cio, CIO_PB_CSR, CIO_CLRIP);
			status = DMADONE(zs->mcp_txdma);
			if (zs->zs_flags & MCP_WAIT_DMA) {
				mask = 1 << zs->mcp_txdma->d_chan;
				if (status & mask) {
					zs->mcp_txdma->d_chip->d_mask &= ~mask;
					zs->zs_flags &= ~MCP_WAIT_DMA;
					if (zs->mcp_ops) (*zs->mcp_ops->mcpop_txend) (zs);
				}
			}
			break;

		case CIO_PAD6_FIFO_F:
			while (mcp->fifo[0] != FIFO_EMPTY)
				;
			CIO_WRITE(cio, CIO_PA_CSR, CIO_CLRIP);
			break;

		case CIO_PAD0_DSRDM:
		case CIO_PAD1_DSRDM:
		case CIO_PAD2_DSRDM:
		case CIO_PAD3_DSRDM:

		case CIO_PAD5_FIFO_HF:
		case CIO_PAD4_FIFO_E:
			CIO_WRITE(cio, CIO_PA_CSR, CIO_CLRIP);
			break;

		case CIO_PBD6_PE:
			zs = &mcpcom[nmcpline + bd];
			/* reset & clear the source of interrupt */
			cio->portb_data = ~MCPRPE;
			CIO_WRITE(cio, CIO_PB_CSR, CIO_CLRIP);
			if (zs->mcp_ops) (*zs->mcp_ops->mcpop_pe) (zs);
			break;
		case CIO_PBD7_SLCT:
			zs = &mcpcom[nmcpline + bd];
			/* reset & clear the source of interrupt */
			cio->portb_data = ~MCPRSLCT;
			CIO_WRITE(cio, CIO_PB_CSR, CIO_CLRIP);
			if (zs->mcp_ops) (*zs->mcp_ops->mcpop_slct) (zs);
			break;

		}
  		/*
		 * If we had an interrupt vector, go back and check
		 * for more FIFO data or more interrupt vectors.
  		 */
	} while (dvector != MCP_NOINTR);
	return (loops);		/* zero if nothing done */
}

static void
mcp_dmaattach(mcp, unit)
	register struct mcp_device *mcp;
	register int    unit;	/* which mcp board */
{
	register int    i, j;
	register u_char *base = (u_char *) mcp;

	/* Chip[5].channels[1,2,3] are "Reserved". Initialize them anyway. */
	for (i = unit * NDMA_CHIPS, j = 0; j < NDMA_CHIPS; i++, j++) {
		mcp_dmainit(&dma_chip[i], &mcp->dmas[j],
		    (j == 4) ? RX_DIR : TX_DIR);
		dma_chip[i].ram_base = base;
	}
}

static void
mcp_dmainit(dc, base, flag)
	struct dma_chip *dc;
	struct dma_device *base;
	char            flag;
{
	struct dma_chan *dma;
	short           i, dir;
	struct addr_wc *port;

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
		port->baddr = 0;/* write the lower byte */
		port->baddr = 0;/* write the high byte */
		port->wc = 0xFF;/* write the lower byte */
		port->wc = 0xFF;/* write the high byte */
	}
}

struct dma_chan *
mcp_dmagetchan(port, dir, unit, scc)
	register int    port, dir, unit;
	int             scc;
{
	if (scc == PR_DMA) {
		if (dir == TX_DIR)
			return (&dma_chip[5 + (unit * NDMA_CHIPS)].d_chans[port]);
		return (0);
	}
	if (dir == TX_DIR)
	 return (&dma_chip[CHIP(port) + (unit*NDMA_CHIPS)].d_chans[CHAN(port)]);
	if (port < 4)
		return (&dma_chip[4 + (unit * NDMA_CHIPS)].d_chans[port]);
	return (0);
}

void
mcp_dmastart(dma, addr, len)
	struct dma_chan *dma;
	char           *addr;
	short           len;
{
	long            linaddr;
	register struct addr_wc *port;

	linaddr = (long) addr - (long) dma->d_chip->ram_base;
	dma->d_chip->d_ioaddr->mask = DMA_MASK_SET + dma->d_chan;
	dma->d_chip->d_ioaddr->clrff = 1;
	port = dma->d_myport;
	port->baddr = (short) linaddr & 0x0FF;	/* write the low byte */
	port->baddr = ((short) linaddr >> 8) & 0x0FF; /* write the high byte */
	len--;
	port->wc = len & 0xFF;
	port->wc = (len >> 8) & 0xFF;
	printD("dma starting, count = %x\n", mcp_getwc(dma));
	dma->d_chip->d_mask |= 1 << dma->d_chan;
	dma->d_chip->d_ioaddr->mask = DMA_MASK_CLEAR + dma->d_chan;
}

u_short
mcp_getwc(dma)
	register struct dma_chan *dma;
{
	register struct addr_wc *port;
	register u_char lo, hi;

	port = dma->d_myport;
	lo = port->wc;
	hi = port->wc;
	return ((hi << 8) + lo + 1);
}

u_short
mcp_dmahalt(dma)
	struct dma_chan *dma;
{
	u_short          len;

	dma->d_chip->d_ioaddr->mask = DMA_MASK_SET + dma->d_chan;
	if ((len = mcp_getwc(dma)) != 0)
		dma->d_chip->d_mask &= ~(1 << dma->d_chan);
	return (len);
}
