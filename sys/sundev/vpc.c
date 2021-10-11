#ifndef lint
static	char sccsid[] = "@(#)vpc.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * DMA interface driver for Systech vpc-2200 Versatec/Centronics interface
 * uses Intel 8237 DMA controller chip
 * uses 16 bit I/O transfers and 68000 byte order
 * the driver tends to be rather ugly because there are two channels on one
 * device, and we need to deal with each channel as an independent device
 * to allow for separate DMA on each channel
 * the board does not allow DMA transfers to begin on an odd byte boundary
 * for 16 bit I/O so we copy the odd byte to an even address location and
 * break the DMA into 2 separate operations
 */

#include "vpc.h"
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/ioctl.h>
#include <sys/vcmd.h>
#include <sys/uio.h>

#include <machine/psl.h>
#include <machine/mmu.h>
#include <sundev/vpcreg.h>
#include <sundev/mbvar.h>

#define	VPCPRI	(PZERO-1)

/*
 * NNVPC is the real number of channels on the VPCs, there are NCHANNELS 
 * minor devices per board, one minor device for each channel. 
 */

#define	NCHANNELS	2
#define	NNVPC		(NVPC * NCHANNELS)

/* NNVPC is (NVPC=2) * 2 = 4  */

/*
 * channel numbers for the centronics and the versatec interface
 * VERSATEC IS THE FIRST CHANNELL
 */

#define	VERSATEC	0
#define	PRINTER		1

/*
 * software state structure, one for each channel
 */

struct vpc_softc {
	int	sc_state;
	int	sc_mbinfo;
	int	sc_addr;
	int	sc_length;
} vpc_softc[NNVPC];

/*
 * sc_state bits for internal state
 */
#define	VPC_BUSY	0100000
#define	VPC_OPEN	0200000
#define	VPC_ODD_ADDR	0400000		/* DMA starts on an odd address */

/*
 * sc_state bits - passed in VGETSTATE and VSETSTATE ioctls
 */
#define	VPC_MODE	0000700
#define	VPC_SPP		0000400
#define	VPC_PLOT	0000200
#define	VPC_PRINT	0000100
#define	VPC_CMNDS	0000076

/*
 * macros for determining board, channel and minor device
 */
#define	VPC_BOARD(dev)		(minor(dev) / 2)
#define	VPC_CHANNEL(dev)	(minor(dev) & 0x01)
#define	VPC_UNIT(dev)		(minor(dev))

/*
 * one buf for each channel for multi-threaded DMA
 */
struct buf	rvpcbuf[NNVPC];

/*
 * one scratch word (2 bytes) for each channel to support writing
 * buffers that begin at odd addresses
 */
char	*odd_addr_words;

int vpcprobe(), vpcintr();

struct mb_device	*vpcdinfo[NVPC];

struct mb_driver vpcdriver = {
	vpcprobe, 0, 0, 0, 0, vpcintr,
	sizeof (struct vpcdevice), "vpc", vpcdinfo, 0, 0, MDR_DMA,
};

vpcprobe(reg)
	caddr_t reg;
{
	register struct vpcdevice *vpcaddr = (struct vpcdevice *)reg;

	if (peekc((char *)&vpcaddr->control[VERSATEC].csr) == -1)
		return (0);
	if (peekc((char *)&vpcaddr->control[PRINTER].csr) == -1)
		return (0);
	if (pokec((char *)&vpcaddr->vpc_clear, VPC_ANY))
		return (0);
	if (pokec((char *)&vpcaddr->control[PRINTER].misc, VPC_ANY))
		return (0);
	vpcaddr->vpc_mode = VPC_MVPCMODE | 0x00;	/* channel 0 */
	vpcaddr->vpc_mode = VPC_MVPCMODE | 0x01;	/* channel 1 */

	/*
	 * bit to enabling interrupts for the board is hidden in
	 * the printer csr
	 */
	vpcaddr->control[PRINTER].csr = VPC_BC_EN_INT;

	/*
	 * allocate space that we can DMA from for writing single bytes
	 * from transfers beginning on odd boundaries, two bytes for each
	 * channel, allign the space an even boundary.  Also, allocate
	 * an even number of bytes to prevent possible alignment problems
	 * for other device drivers.
	 */
	odd_addr_words = (char *)rmalloc(iopbmap, (long)((NNVPC * 2) + 4));
	if ((int)odd_addr_words & 0x01) {
		odd_addr_words += 1;
	}
	return (sizeof(struct vpcdevice));
}

vpcopen(dev)
	dev_t dev;
{
	register struct vpc_softc *sc;
	register struct mb_device *md;
	register struct vpcdevice *vpcaddr;
	register int s;

	if (VPC_UNIT(dev) >= NNVPC ||
	   ((sc = &vpc_softc[VPC_UNIT(dev)])->sc_state & VPC_OPEN) ||
	   (md = vpcdinfo[VPC_BOARD(dev)]) == 0 || md->md_alive == 0) {
		return (ENXIO);
	}
	vpcaddr = (struct vpcdevice *)md->md_addr;
	s = splx(pritospl(md->md_intpri));
	if ((vpcaddr->control[VPC_CHANNEL(dev)].csr & VPC_PS_OK)!=VPC_PS_OK) {
		(void) splx(s);
		return (EIO);
	}
	vpcaddr->control[VPC_CHANNEL(dev)].interrupt = VPC_IC_EN_ATN;
	if (VPC_CHANNEL(dev) == PRINTER) {
		sc->sc_state = VPC_OPEN;
	} else {
		sc->sc_state = VPC_OPEN | VPC_PRINT | VPC_CLRCOM | VPC_RESET;
		while (sc->sc_state & VPC_CMNDS) {
			if (vpcwait(dev)) {
				vpcclose(dev);
				(void) splx(s);
				return (EIO);
			}
			vpccmd(dev);
		}
	}
	(void) splx(s);
	return (0);
}

vpcclose(dev)
	dev_t dev;
{
	register int s;
	register struct vpc_softc *sc = &vpc_softc[VPC_UNIT(dev)];
	register struct mb_device *md = vpcdinfo[VPC_BOARD(dev)];
	register struct vpcdevice *vpcaddr = (struct vpcdevice *)md->md_addr;

	s = splx(pritospl(md->md_intpri));
	sc->sc_state = 0;
	vpcaddr->control[VPC_CHANNEL(dev)].interrupt = VPC_IC_DIS_ATN;
	(void) splx(s);

}

vpcstrategy(bp)
	register struct buf *bp;
{
	register struct vpc_softc *sc = &vpc_softc[VPC_UNIT(bp->b_dev)];
	register struct mb_device *md = vpcdinfo[VPC_BOARD(bp->b_dev)];
	register int s;
	register int addr;
	register int length;
	register int channel_number;
	char *odd_addr_byte;
	int tmp_addr;

	s = splx(pritospl(md->md_intpri));
	sc->sc_mbinfo = mbsetup(md->md_hd, bp, 0);
	addr = MBI_ADDR(sc->sc_mbinfo);	
	length = bp->b_bcount;
	bp->b_resid = 0;
	channel_number = VPC_CHANNEL(bp->b_dev);
	sc->sc_state |= VPC_BUSY;
	if (addr & 0x01) {
		odd_addr_byte = (char *)((int)odd_addr_words +
		    (VPC_UNIT(bp->b_dev) * 2));
		*odd_addr_byte = DVMA[addr];
		tmp_addr = odd_addr_byte - DVMA;
		if (length > 1) {
			sc->sc_state |= VPC_ODD_ADDR;
			sc->sc_addr = addr + 1;
			sc->sc_length = length - 1;
		}
		vpc_io(md, channel_number, tmp_addr, 1);
	} else {
		vpc_io(md, channel_number, addr, length);
	}
	(void) splx(s);
}

vpc_io(md, channel_number, addr, length)
	register struct mb_device *md;
	register int channel_number;
	register int addr;
	register int length;
{
	register int s;
	register struct vpcdevice *vpcaddr = (struct vpcdevice *)md->md_addr;

	s = splx(pritospl(md->md_intpri));
	if (length & 0x01) {
		vpcaddr->control[channel_number].interrupt = VPC_IC_ODD_XFER;
		length++;
	}
	length >>= 1;			
	length--;			/* 8237 quirk */
	vpcaddr->vpc_clearff = VPC_ANY;
	vpcaddr->channel[channel_number].count = length & 0xff;
	vpcaddr->channel[channel_number].count = (length >> 8) & 0xff;
	addr >>= 1;
	vpcaddr->channel[channel_number].addr = addr & 0xff;
	vpcaddr->channel[channel_number].addr = (addr >> 8) & 0xff;
	vpcaddr->control[channel_number].hiaddr = (addr >> 15) & 0x0f;
	vpcaddr->control[channel_number].interrupt = VPC_IC_EN_XDONE;
	vpcaddr->vpc_smb = channel_number;
	(void) splx(s);
}

/*ARGSUSED*/
vpcwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (physio(vpcstrategy, &rvpcbuf[VPC_UNIT(dev)], dev, B_WRITE,
	    minphys, uio));
}

/*ARGSUSED*/
vpcintr()
{
	register u_char	status;
	register struct vpc_softc *sc;
	register int device;
	register int board;
	register int channel;
	register int found;
	struct vpcdevice *vpcaddr;
	struct mb_device *md;

	found = 0;
	for (board = 0; board < NVPC; board++) {
		if ((md = vpcdinfo[board]) == NULL)
			continue;
		vpcaddr = (struct vpcdevice *) md->md_addr;
		for (channel = 0; channel < NCHANNELS; channel++) {
			status = vpcaddr->control[channel].interrupt;
			device = (board * NCHANNELS) + channel;
			sc = &vpc_softc[device];

			/*
			 * we should probably check for the specific reason
			 * for the attention interrupt, but because every
			 * printer seems to handle paper out / printer on-line
			 * conditions differently all we do is clear it
			 */
			if (status & VPC_IS_ATN) {
				found = 1;
				vpcaddr->control[channel].interrupt =
				    VPC_IC_CL_ATN;
				wakeup((caddr_t)sc);
			}
			if (status & VPC_IS_DONE) {
				found = 1;
				vpcaddr->control[channel].interrupt =
				    VPC_IC_CL_DONE;
				if (status & VPC_IS_ERR) {
					printf("vpc%d: bus address error channel %d.\n",
					    board, channel);
					/*
					 * reset the printer logic (but not
					 * the 8237 DMA chip)
					 */
					vpcaddr->control[PRINTER].misc =
					    VPC_ANY;
					vpcaddr->control[PRINTER].csr =
					    VPC_BC_EN_INT;
				}
				if (sc->sc_state & VPC_ODD_ADDR) {
					sc->sc_state &= ~VPC_ODD_ADDR;
					vpc_io(md, channel,
					    sc->sc_addr, sc->sc_length);
					return (found);
				}
				if (sc->sc_state & VPC_BUSY) {
					sc->sc_state &= ~VPC_BUSY;
					mbrelse(md->md_hd, &sc->sc_mbinfo);
					iodone(&rvpcbuf[device]);
				}
				wakeup((caddr_t)sc);
			}
		}
	}
	return (found);
}

vpcwait(dev)
	dev_t dev;
{
	register struct vpc_softc *sc = &vpc_softc[VPC_UNIT(dev)];
	register struct vpcdevice *vpcaddr =
	    (struct vpcdevice *)vpcdinfo[VPC_BOARD(dev)]->md_addr;

	for (;;) {
		if ((sc->sc_state & VPC_BUSY) == 0 &&
		    vpcaddr->control[VERSATEC].csr & VPC_PS_OK) {
			break;
		}
		(void) sleep((caddr_t)sc, VPCPRI);
	}
	return (0);	/* NO ERRORS YET */
}

struct pair {
	char	soft;		/* software bit */
	char	hard;		/* hardware bit */
} vpcbits[] = {
	VPC_RESET,	VPC_PC_V_RESET,
	VPC_CLRCOM,	VPC_PC_V_CLEAR,
	VPC_EOTCOM,	VPC_PC_V_EOT,
	VPC_FFCOM,	VPC_PC_V_FFEED,
	VPC_TERMCOM,	VPC_PC_V_LTER,
	0,		0,
};

vpccmd(dev)
	dev_t dev;
{
	register struct vpc_softc *sc = &vpc_softc[VPC_UNIT(dev)];
	register struct vpcdevice *vpcaddr =
	    (struct vpcdevice *)vpcdinfo[VPC_BOARD(dev)]->md_addr;
	register struct pair *bit;

	for (bit = vpcbits; bit->soft != 0; bit++) {
		if (sc->sc_state & bit->soft) {
			vpcaddr->control[VERSATEC].csr = bit->hard;
			sc->sc_state &= ~bit->soft;
			return;
		}
	}
}

/*ARGSUSED*/
vpcioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register int m;
	register struct vpc_softc *sc = &vpc_softc[VPC_UNIT(dev)];
	register struct mb_device *md = vpcdinfo[VPC_BOARD(dev)];
	register struct vpcdevice *vpcaddr = (struct vpcdevice *)md->md_addr;
	register int s;

	if (VPC_CHANNEL(dev) != VERSATEC)
		return (ENOTTY);
	switch (cmd) {

	case VGETSTATE:
		*(int *)data = sc->sc_state;
		break;

	case VSETSTATE:
		m = *(int *)data;
		sc->sc_state =
		    (sc->sc_state & ~VPC_MODE) | (m & (VPC_MODE | VPC_CMNDS));
		break;

	default:
		return (ENOTTY);
	}
	s = splx(pritospl(md->md_intpri));
	(void) vpcwait(dev);
	if (sc->sc_state & VPC_SPP) {
		vpcaddr->control[VERSATEC].csr = VPC_PC_V_SPP;
	} else if (sc->sc_state & VPC_PLOT) {
		vpcaddr->control[VERSATEC].csr = VPC_PC_V_PLOT;
	} else {
		vpcaddr->control[VERSATEC].csr = 0;
	}
	while (sc->sc_state & VPC_CMNDS) {
		(void) vpcwait(dev);
		vpcaddr->control[VERSATEC].interrupt = VPC_IC_EN_RDONE;
		vpccmd(dev);
		(void) vpcwait(dev);
	}
	(void) splx(s);
	return (0);
}
