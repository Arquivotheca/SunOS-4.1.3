/* @(#)mcpcom.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun MCP Multiprotocol Communication Processor
 * Sun ALM-2 Asynchronous Line Multiplexer
 */

#ifndef _sundev_mcpcom_h
#define _sundev_mcpcom_h

struct mcpops {
	int             (*mcpop_attach) ();	/* attach protocol */
	int             (*mcpop_txint) ();	/* xmit buffer empty */
	int             (*mcpop_xsint) ();	/* external/status */
	int             (*mcpop_rxint) ();	/* receive char available */
	int             (*mcpop_srint) ();	/* special receive condition */
	int             (*mcpop_txend) ();	/* transmit DMA done */
	int             (*mcpop_rxend) ();	/* receive DMA done */
	int             (*mcpop_rxchar) ();	/* fifo receive char
						 * available */
	int             (*mcpop_dsr) ();	/* DSR/DM */
	int             (*mcpop_pe) ();		/* PE: printer out of paper */
	int             (*mcpop_slct) ();	/* SLCT: printer is on-line */
};

/*
 * SunOS 4.0[.x] had a "struct last_char" that had this
 * layout; we want unbundled stuff from that era to work
 * properly. keeping the same shape makes us trivially
 * compatible across m68k/sparc/386 archetectures, and
 * changing the names notifies the world that we don't
 * really keep the last transmitted character there.
 */
struct __pad__ {
	char	__pad_1__;
	char	__pad_2__;
};

/*
 * Common data
 */

struct mcpcom {
	struct mcp_device *mcp_addr;	/* address of mcp register set */
	struct zscc_device *zs_addr;	/* address of half zs chip */
	short           zs_unit;	/* which port (0:15) */
	short           mc_unit;	/* which MCP board */
	caddr_t         zs_priv;	/* protocol private data */
	struct __pad__	___pad___;	/* compat. for unbundled MCP drivers */
	struct mcpops  *mcp_ops;	/* interrupt op vector */
	struct dma_chan *mcp_rxdma;	/* receive DMA channel */
	struct dma_chan *mcp_txdma;	/* transmit DMA channel */
	u_char          zs_wreg[16];	/* shadow of write registers */
	char            zs_flags;	/* random flags */
	short           zs_rerror;	/* receive err, must throw away char */
};


#define	MCP_RINGBITS	8		/* # of bits in ring ptrs */
#define	MCP_RINGSIZE	(1<<MCP_RINGBITS)	/* size of ring */
#define	MCP_RINGMASK	(MCP_RINGSIZE-1)
#define	MCP_RINGFRAC	2		/* fraction of ring to force flush */

#define	MCP_RING_INIT(mp)	((mp)->za_rput = (mp)->za_rget = 0)
#define	MCP_RING_CNT(mp)	(((mp)->za_rput - (mp)->za_rget) & MCP_RINGMASK)
#define	MCP_RING_FRAC(mp)	(MCP_RING_CNT(mp) >= (MCP_RINGSIZE/MCP_RINGFRAC))
#define	MCP_RING_POK(mp, n) 	(MCP_RING_CNT(mp) < (MCP_RINGSIZE-(n)))
#define	MCP_RING_PUT(mp, c) \
	((mp)->za_ring[(mp)->za_rput++ & MCP_RINGMASK] =  (u_char)(c))
#define	MCP_RING_UNPUT(mp)	((mp)->za_rput--)
#define	MCP_RING_GOK(mp, n) 	(MCP_RING_CNT(mp) >= (n))
#define	MCP_RING_GET(mp)	((mp)->za_ring[(mp)->za_rget++ & MCP_RINGMASK])
#define	MCP_RING_EAT(mp, n)	 ((mp)->za_rget += (n))



/*
 * Communication between H/W level 4 interrupts and driver
 */
struct mcpaline {
	int             za_flags;	/* random flags */
	dev_t           za_dev;		/* major/minor for this device */
	struct mcpcom  *za_common;	/* address of zs common data struct */
	tty_common_t    za_ttycommon;	/* data common to all tty drivers */
	int		za_wbufcid;	/* id of pending write-side bufcall */
	time_t		za_dtrlow;	/* time dtr last went low */
	u_char          za_rr0;		/* for break detection */
	char            za_xactive;	/* txmit active    */
	u_char         *za_xoff;	/* xoff char in h/w XOFF buffer */
	u_char          za_flowc;	/* xoff char to transmit */
	u_char          za_lnext;	/* literal-next character from termio */
	u_char         *za_devctl;	/* device control reg for this port */
	u_char         *za_dmabuf;	/* dma ram buffer for this port */
	u_char         *za_optr;	/* output ptr */
	short           za_ocnt;	/* output count */
	char            za_done;	/* dma is done */
	u_char		za_rput;	/* producing ptr for input */
	u_char		za_rget;	/* consuming ptr for input */
	u_char		za_ring[MCP_RINGSIZE];	/* circular queue for input
						 * buf */
};

struct zs_dmabuf {
	u_char         *d_baddr;	/* base addr of the buf */
	short           d_wc;		/* current word count */
};

#define MCP_WAIT_DMA   0x1
#define TXENABLE       0x1

/* interrupt vector value */
#define CIO_PBD0_TXEND    0x11
#define CIO_PBD1_TXEND    0x13
#define CIO_PBD2_TXEND    0x15
#define CIO_PBD3_TXEND    0x17
#define CIO_PBD4_RXEND    0x19
#define CIO_PBD5_PPTX     0x1b
#define CIO_PBD6_PE       0x1d
#define CIO_PBD7_SLCT     0x1f

#define CIO_PAD0_DSRDM    0x1
#define CIO_PAD1_DSRDM    0x3
#define CIO_PAD2_DSRDM    0x5
#define CIO_PAD3_DSRDM    0x7
#define CIO_PAD4_FIFO_E   0x9
#define CIO_PAD5_FIFO_HF  0xb
#define CIO_PAD6_FIFO_F   0xd

#define SCC0_TXINT        0x0
#define SCC0_XSINT        0x2
#define SCC0_RXINT        0x4
#define SCC0_SRINT        0x6
#define SCC1_TXINT        0x8
#define SCC1_XSINT        0xa
#define SCC1_RXINT        0xc
#define SCC1_SRINT        0xe

#define SCC2_TXINT        0x10
#define SCC2_XSINT        0x12
#define SCC2_RXINT        0x14
#define SCC2_SRINT        0x16
#define SCC3_TXINT        0x18
#define SCC3_XSINT        0x1a
#define SCC3_RXINT        0x1c
#define SCC3_SRINT        0x1e

#define SCC4_TXINT        0x20
#define SCC4_XSINT        0x22
#define SCC4_RXINT        0x24
#define SCC4_SRINT        0x26
#define SCC5_TXINT        0x28
#define SCC5_XSINT        0x2a
#define SCC5_RXINT        0x2c
#define SCC5_SRINT        0x2e

#define SCC6_TXINT        0x30
#define SCC6_XSINT        0x32
#define SCC6_RXINT        0x34
#define SCC6_SRINT        0x36
#define SCC7_TXINT        0x38
#define SCC7_XSINT        0x3a
#define SCC7_RXINT        0x3c
#define SCC7_SRINT        0x3e

#define SCC8_TXINT        0x40
#define SCC8_XSINT        0x42
#define SCC8_RXINT        0x44
#define SCC8_SRINT        0x46
#define SCC9_TXINT        0x48
#define SCC9_XSINT        0x4a
#define SCC9_RXINT        0x4c
#define SCC9_SRINT        0x4e

#define SCC10_TXINT       0x50
#define SCC10_XSINT       0x52
#define SCC10_RXINT       0x54
#define SCC10_SRINT       0x56
#define SCC11_TXINT       0x58
#define SCC11_XSINT       0x5a
#define SCC11_RXINT       0x5c
#define SCC11_SRINT       0x5e

#define SCC12_TXINT       0x60
#define SCC12_XSINT       0x62
#define SCC12_RXINT       0x64
#define SCC12_SRINT       0x66
#define SCC13_TXINT       0x68
#define SCC13_XSINT       0x6a
#define SCC13_RXINT       0x6c
#define SCC13_SRINT       0x6e

#define SCC14_TXINT       0x70
#define SCC14_XSINT       0x72
#define SCC14_RXINT       0x74
#define SCC14_SRINT       0x76
#define SCC15_TXINT       0x78
#define SCC15_XSINT       0x7a
#define SCC15_RXINT       0x7c
#define SCC15_SRINT       0x7e

#endif /*!_sundev_mcpcom_h*/
