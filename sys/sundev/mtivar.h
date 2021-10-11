/*	@(#)mtivar.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987, 1989 by Sun Microsystems, Inc.
 */

/*
 * Systech MTI-800/1600 Multiple Terminal Interface driver
 */

#ifndef _sundev_mtivar_h
#define	_sundev_mtivar_h

/*
 * Per-board data structures.
 */
struct	mti_softc {
	int	msc_have;	/* number of response chars accumulated */
	u_char	msc_rbuf[8];	/* buffer for responses */
	struct	clist msc_cmdq;	/* queue of commands if cmd fifo is busy */
};

#define	MTI_RINGBITS	8		/* # of bits in ring ptrs */
#define	MTI_RINGSIZE	(1<<MTI_RINGBITS)	/* size of ring */
#define	MTI_RINGMASK	(MTI_RINGSIZE-1)
#define	MTI_RINGFRAC	2		/* fraction of ring to force flush */

#define	MTI_RING_INIT(mtp)	((mtp)->mt_rput = (mtp)->mt_rget = 0)
#define	MTI_RING_CNT(mtp)	(((mtp)->mt_rput - (mtp)->mt_rget) & MTI_RINGMASK)
#define	MTI_RING_FRAC(mtp)	(MTI_RING_CNT(mtp) >= (MTI_RINGSIZE/MTI_RINGFRAC))
#define	MTI_RING_POK(mtp, n) (MTI_RING_CNT(mtp) < (MTI_RINGSIZE-(n)))
#define	MTI_RING_PUT(mtp, c) \
	((mtp)->mt_ring[(mtp)->mt_rput++ & MTI_RINGMASK] = (u_char)(c))
#define	MTI_RING_UNPUT(mtp)	((mtp)->mt_rput--)
#define	MTI_RING_GOK(mtp, n) (MTI_RING_CNT(mtp) >= (n))
#define	MTI_RING_GET(mtp)	((mtp)->mt_ring[(mtp)->mt_rget++ & MTI_RINGMASK])
#define	MTI_RING_EAT(mtp, n) ((mtp)->mt_rget += (n))

/*
 * Per-line data structures.
 */
struct mtiline {
	int	mt_flags;		/* random flags */
	int	mt_dev;			/* unit number */
	tty_common_t mt_ttycommon;	/* data common to all tty drivers */
	int	mt_wbufcid;		/* id of pending write-side bufcall */
	int	mt_dtrlow;		/* time dtr last went low */
	u_char	mt_wbits;		/* copy of writable modem ctl bits */
	u_char	mt_rbits;		/* copy of readable modem ctl bits */
	char	*mt_buf;		/* pointer to DMA buffer */
	int	mt_dmaoffs;		/* offset into DMA buffer to start */
	int	mt_dmacount;		/* number of bytes to transmit */
	u_char	mt_flowc;		/* startc or stopc to transmit */
	u_char	mt_rput;		/* producing ptr for input */
	u_char	mt_rget;		/* consuming ptr for input */
	u_char	mt_ring[MTI_RINGSIZE];	/* input ring */
};

#define	MTS_WOPEN	0x00000001	/* waiting for open to complete */
#define	MTS_ISOPEN	0x00000002	/* open is complete */
#define	MTS_OUT		0x00000004	/* line being used for dialout */
#define	MTS_CARR_ON	0x00000008	/* carrier on last time we looked */
#define	MTS_XCLUDE	0x00000010	/* device is open for exclusive use */
#define	MTS_STOPPED	0x00000020	/* output is stopped */
#define	MTS_DELAY	0x00000040	/* waiting for delay to finish */
#define	MTS_BREAK	0x00000080	/* waiting for break to finish */
#define	MTS_BUSY	0x00000100	/* waiting for transmission to finish */
#define	MTS_FCXMIT	0x00000200	/*
					 * waiting for flow control char
					 * transmission to finish
					 */
#define	MTS_DRAINING	0x00000400	/* waiting for output to drain */
#define	MTS_FLUSH	0x00000800	/* flushing output being transmitted */
#define	MTS_DRAINPEND	0x00001000	/* ring-drain pending */
#define	MTS_OVERRUN	0x00002000	/* hardware silo overflow */
#define	MTS_SUSPD	0x00004000	/* output is suspended */
#define	MTS_SOFTC_ATTN	0x00008000	/* softcar status has changed */

#endif /*!_sundev_mtivar_h*/
