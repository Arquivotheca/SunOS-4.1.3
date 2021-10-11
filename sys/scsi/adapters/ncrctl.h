/* @(#)ncrctl.h 1.1 92/07/30 SMI */
/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef	_scsi_adapters_ncrctl_h
#define	_scsi_adapters_ncrctl_h

/*
 * NCR 5380 Host adapter Control Registers
 */

/*
 * The most general layout that could possibly describe
 * these varied implementations is a base address that
 * contains the NCR 5380 registers, followed by some DMA
 * registers and then followed by some control registers.
 * The 3/E actually prefaces this with 64kb of local memory,
 * but ignore that for the moment. The issue is incredibly
 * contorted due to the fact that there are 4 different
 * DMA engines (in two general classes), and that access
 * and alignment requirements vary due to requirements of
 * different supported cpu platform architectures. The
 * best way that we've come up with to represent the
 * different register sets is two break them into the
 * the three areas of NCR 5380 registers (common across
 * all host adapters), DMA registers, and Control Registers.
 * The addressing of the NCR 5380 registers is the same
 * across all machines. The differences lie in the definition
 * and placement of the DMA and Control Registers. The map
 * described below best specifies the way this can be done.
 * Note that this map just specifies placement of registers-
 * not whether any given register is used in all platforms
 * in which the access described applies.
 *
 * ('missing' denotes a span in the address space)
 * ('XXXXX' denotes an explicit pad)
 *
 * 			DMA OFFSETS
 *
 *
 *	4/110		3/50, 3/60		sun4 VME	3/E
 *			sun3, sun3x VME
 *
 * u_int dma_addr	u_int dma_addr		u_short addr_h	u_short XXXXX
 *						u_short addr_l	u_short addr
 *
 * u_int dma_count	u_int dma_count		u_short count_h	u_short XXXXX
 *						u_short count_l	u_short count
 *
 * u_int bcr		u_short udc_rdata	u_short XXXXX	u_short XXXXX
 * (bcr not used)	u_short udc_raddr	u_short XXXXX	u_short XXXXX
 *   (missing)		u_short fifo_data	u_short XXXXX	u_short XXXXX
 *   (missing)		u_short bcr		u_short bcr	u_short XXXXX
 *
 * 			CSR OFFSETS
 *
 * u_int csr		(missing)		(missing)	u_short XXXXX
 *			u_short csr		u_short csr	u_short csr
 * u_int bpr		u_short bprh		u_short bprh	u_short XXXXX
 *			u_short bprl		u_short bprl	u_char  XXXXX
 *								u_char  ivec2
 * u_int XXXXX		u_short iv_am		u_short iv_am	(missing)
 *			u_short bcrh		u_short bcrh	(missing)
 *
 *
 * Given this map, it is possible to define DMA and Control structures
 * that will, with a bit of work, be valid across all architectures.
 *
 * (Acknowledgements: Sam Ho, Kevin Sheehan, Steve Bilan)
 *
 */

/*
 * Define a structure the divvies up a longword into it's components.
 * Note the assumptions below and elsewhere about byte && word order
 * within a long word.
 */

typedef union {
	u_char	ca[4];
	u_short wa[2];
	u_long	la;
} vaccess;
#define	msw	wa[0]
#define	lsw	wa[1]
#define	lsb	ca[3]

struct ncrdma {
	/*
	 * DMA address register (SCSI-3, 3/E and 4/110) (r/w)
	 */
	vaccess	addr;	/*
			 * csr.la	- (4/110, SCSI-3 sun3/3x)
			 * csr.msw	- (SCSI-3 sun4)
			 * csr.lsw	- (SCSI-3 sun4, 3/E)
			 * csr.ca[0-3]	- not used
			 *
			 */

	/*
	 * DMA count register (SCSI-3, 3/E and 4/110 only) (r/w)
	 */

	vaccess	count;	/*
			 * csr.la	- (4/110, SCSI-3 sun3/3x)
			 * csr.msw	- (SCSI-3 sun4)
			 * csr.lsw	- (SCSI-3 sun4, 3/E)
			 * csr.ca[0-3]	- not used
			 *
			 */

	/*
	 * AMD UDC 9516 controller data register (3/50, 3/60) (r/w)
	 */

	u_short	udc_rdata;

	/*
	 * AMD UDC 9516 controller address register (3/50, 3/60) (w)
	 */

	u_short	udc_raddr;

	/*
	 * FIFO Data (incoming) (3/50, 3/60) (r)
	 */

	u_short	fifo_data;

	/*
	 * Byte Counter (3/50, 3/60; SCSI-3, low 16 bits) (r/w)
	 */

	u_short	bcr;
};

/*
 * This structure must be aligned on a 4-byte boundary on the 4/110.
 * On other platforms, the first address of the *2nd* halfword of this
 * structure will be at the address of that platform's csr.
 */

struct ncrctl {
	/*
	 * Control and Status register (all variants) r/w
	 */
	vaccess csr;	/*
			 * csr.la	- (4/110)
			 * csr.msw	- not used
			 * csr.lsw	- (SCSI-3, 3/E, 3/50, 3/60)
			 * csr.ca[0-3]	- not used
			 *
			 */

	/*
	 * Byte pack register- used on SCSI-3 and 4/110 implementations,
	 * except that the lsb is used by the 3/E for the interrupt
	 * vector register.
	 */

	vaccess bpr;	/*
			 * bpr.la	- (4/110)
			 * bpr.msw	- (SCSI-3)
			 * bpr.lsw	- (SCSI-3)
			 * bpr.ca[0-2]	- not used
			 * bpr.ca[3]	- (3/E) (vector, not byte pack)
			 *
			 */
#define	ivec2	bpr.lsb

	/*
	 * SCSI-3: vector/address modifier register
	 */

	u_short	iv_am;			/* (SCSI-3) */

	/*
	 * SCSI-3: high portion of byte counter.
	 */

	u_short	bcrh;			/* (SCSI-3) */

};

/*
 * Status Register Definitions.
 * Note:
 *	(r)	indicates bit is read only.
 *	(rw)	indicates bit is read or write.
 *	(v)	vme host adaptor interface only.
 *	(o)	sun3/50 onboard host adaptor interface only.
 *	(b)	both vme and sun3/50 host adaptor interfaces.
 *
 * XXX OLD INFO - UPDATE
 */

#define	NCR_CSR_DMA_ACTIVE	0x8000	/* (r, o) dma transfer active */
#define	NCR_CSR_DMA_CONFLICT	0x4000	/* (r, b) reg accessed while dmaing */
#define	NCR_CSR_DMA_BUS_ERR	0x2000	/* (r, b) bus error during dma */
#define	NCR_CSR_ID		0x1000	/* (r, b) 0 for 3/50, 1 for SCSI-3, */
					/* 0 if SCSI-3 unmodified */
#define	NCR_CSR_FIFO_FULL	0x0800	/* (r, b) fifo full */
#define	NCR_CSR_FIFO_EMPTY	0x0400	/* (r, b) fifo empty */
#define	NCR_CSR_NCR_IP		0x0200	/* (r, b) sbc interrupt pending */
#define	NCR_CSR_DMA_IP		0x0100	/* (r, b) dma interrupt pending */
#define	NCR_CSR_LOB		0x00c0	/* (r, v) number of leftover bytes */
#define	NCR_CSR_LOB_CNT(x)	(((x)&NCR_CSR_LOB) >> 6)
#define	NCR_CSR_BPCON		0x0020	/* (rw, v) byte packing control */
					/* dma is in 0=longwords, 1=words */
#define	NCR_CSR_DMA_EN		0x0010	/* (rw, v) dma enable */
#define	NCR_CSR_SEND		0x0008	/* (rw, b) dma dir, 1=to device */
#define	NCR_CSR_INTR_EN		0x0004	/* (rw, b) interrupts enable */
#define	NCR_CSR_FIFO_RES	0x0002	/* (rw, b) inits fifo, 0=reset */
#define	NCR_CSR_SCSI_RES	0x0001	/* (rw, b) reset sbc and udc, 0=reset */


#define	CSR_BITS	\
"\20\20DMA_ACT\17DMA_CFLCT\16DMA_BERR\12NCR_IP\11DMA_IP\5DMA_EN\4SND\3ENA"


#ifdef	sun4

#define	GET_CSR(ncr)	\
	((ncr)->n_type == IS_COBRA)? \
		(ncr)->n_ctl->csr.la : (ncr)->n_ctl->csr.lsw

#define	SET_CSR(ncr, val)	\
	((ncr)->n_type == IS_COBRA)? \
		(u_int)((ncr)->n_ctl->csr.la = (u_int)(val)) :  \
		(u_short)((ncr)->n_ctl->csr.lsw = (u_short)(val))

#define	GET_BPR(ncr)	\
	((ncr)->n_type == IS_COBRA)? \
		(ncr)->n_ctl->bpr.la : \
		((ncr)->n_ctl->bpr.msw << 16) | (ncr)->n_ctl->bpr.lsw

#define	GET_DMA_ADDR(ncr)		\
	((ncr)->n_type == IS_3E)? ((u_long) (ncr)->n_dma->addr.lsw) : \
		(((ncr)->n_type == IS_COBRA) ? \
		(ncr)->n_dma->addr.la : \
		(((ncr)->n_dma->addr.msw << 16) | (ncr)->n_dma->addr.lsw))

#define	SET_DMA_ADDR(ncr, val)	\
	((ncr)->n_type == IS_3E)? (ncr)->n_dma->addr.lsw = (u_short)(val) : \
	    ((ncr)->n_type == IS_COBRA) ? \
		(u_int)((ncr)->n_dma->addr.la = (u_int)(val)) : \
		    (u_short)((ncr)->n_dma->addr.msw = (u_short)(val >> 16), \
			((ncr)->n_dma->addr.lsw = (u_short)(val)))

#define	GET_DMA_COUNT(ncr)	\
	((ncr)->n_type == IS_3E)? ((u_long) (ncr)->n_dma->count.lsw) : \
		(((ncr)->n_type == IS_COBRA) ? \
		(ncr)->n_dma->count.la : \
		(((ncr)->n_dma->count.msw << 16) | (ncr)->n_dma->count.lsw))

#define	SET_DMA_COUNT(ncr, val)	\
	((ncr)->n_type == IS_3E)? (ncr)->n_dma->count.lsw = (u_short)(val) : \
	    ((ncr)->n_type == IS_COBRA) ? \
		(u_int)((ncr)->n_dma->count.la = (u_int)(val)) : \
		    (u_short)((ncr)->n_dma->count.msw = (u_short)(val >> 16), \
			((ncr)->n_dma->count.lsw = (u_short)(val)))

#define	GET_BCR(ncr)	\
	((((u_long)(ncr)->n_ctl->bcrh) << 16) | ((u_long) (ncr)->n_dma->bcr))


#define	SET_BCR(ncr, val)	\
	(ncr)->n_ctl->bcrh = ((val) >> 16), \
	(ncr)->n_dma->bcr = (val) & 0xffff

#else	sun4

#define	GET_CSR(ncr)		(ncr)->n_ctl->csr.lsw

#define	SET_CSR(ncr, val)	(ncr)->n_ctl->csr.lsw = (val)

#define	GET_BPR(ncr)	((ncr)->n_ctl->bpr.msw << 16) | (ncr)->n_ctl->bpr.lsw

#define	GET_DMA_ADDR(ncr)	\
	(((ncr)->n_type == IS_3E)?\
		((u_long) (ncr)->n_dma->addr.lsw) : (ncr)->n_dma->addr.la)

#define	SET_DMA_ADDR(ncr, val)	\
	(((ncr)->n_type == IS_3E)? \
		((ncr)->n_dma->addr.lsw = (val)) : \
		((ncr)->n_dma->addr.la = (val)))

#define	GET_DMA_COUNT(ncr)	\
	(((ncr)->n_type == IS_3E)?\
		((u_long) (ncr)->n_dma->count.lsw) : (ncr)->n_dma->count.la)

#define	SET_DMA_COUNT(ncr, val)	\
	(((ncr)->n_type == IS_3E) ? \
		((ncr)->n_dma->count.lsw = (val)) :	\
		((ncr)->n_dma->count.la = (val)))

#define	GET_BCR(ncr)	(((ncr)->n_type == IS_SCSI3)? \
	(((u_long) (ncr)->n_ctl->bcrh << 16) | ((u_long) (ncr)->n_dma->bcr)) :\
	((u_long) (ncr)->n_dma->bcr))

#define	SET_BCR(ncr, val)	(((ncr)->n_type == IS_SCSI3) ? \
	((ncr)->n_ctl->bcrh = ((val) >> 16), \
	(ncr)->n_dma->bcr = ((val) & 0xffff)) : \
			((ncr)->n_dma->bcr = ((val) & 0xffff)))

#endif	sun4

#define	SUN3E_DMABUF_ADDR(ncr)	(((u_long)ncr->n_sbc) - (1<<16))

/*
 * XXXX THESE MAY NOT WORK - THE COMPILER MAY OVER OPTIMIZE THINGS XXXX
 */

#define	DISABLE_DMA(ncr)	\
	SET_CSR(ncr, ((GET_CSR(ncr)) & ~NCR_CSR_DMA_EN))

#define	DISABLE_INTR(ncr)  	\
	SET_CSR(ncr, ((GET_CSR(ncr)) & ~NCR_CSR_INTR_EN))

#define	ENABLE_DMA(ncr)  	\
	SET_CSR(ncr, ((GET_CSR(ncr)) | NCR_CSR_DMA_EN))

#define	ENABLE_INTR(ncr)  	\
	SET_CSR(ncr, ((GET_CSR(ncr)) | NCR_CSR_INTR_EN))


/*
 * Misc defines
 */

/* arbitraty retry count */
#define	NCR_ARB_RETRIES		20
#define	NCR_SEL_RETRIES		5

/* scsi timer values, all in microseconds */
#define	NCR_ARBITRATION_DELAY	3	/*  1 us */
#define	NCR_BUS_CLEAR_DELAY	1	/*  1 us */
#define	NCR_BUS_SETTLE_DELAY	1	/*  1 us */
#define	NCR_UDC_WAIT		1	/*  1 us */
#define	NCR_ARB_WAIT		5	/* 10 us */
#define	NCR_RESET_DELAY		4000000	/*  1 us  (04.00 Sec.) */
#define	NCR_LONG_WAIT		3000000	/* 10 us  (30.00 Sec.) */
#define	NCR_WAIT_COUNT		400000	/* 10 us  (04.00 Sec.) */
#define	NCR_SHORT_WAIT		25000	/* 10 us  (00.25 Sec.) */
#define	NCR_PHASE_WAIT		30	/* 10 us */

/*
 * Shorthand Macros
 */

#define	REQ_ASSERTED(ncr)	\
	(ncr_sbcwait(&CBSR, NCR_CBSR_REQ, NCR_WAIT_COUNT, 1) == SUCCESS)

#define	REQ_DROPPED(ncr)	\
	(ncr_sbcwait(&CBSR, NCR_CBSR_REQ, NCR_WAIT_COUNT, 0) == SUCCESS)


#define	INTPENDING(ncr) 	((GET_CSR(ncr)) & \
	(NCR_CSR_NCR_IP | NCR_CSR_DMA_IP | \
	NCR_CSR_DMA_CONFLICT | NCR_CSR_DMA_BUS_ERR))

#define	RESELECTING(cbsr, cdr, id)	\
	(((cbsr & NCR_CBSR_RESEL) == NCR_CBSR_RESEL) && (cdr & id))

#define	hibyte(x)	(((x) & 0xff00)>>8)
#define	lobyte(x)	((x) & 0xff)

/* possible values for the address modifier, scsi3 vme version only */
#define	VME_SUPV_DATA_24	0x3d00

/*
 * XXX: OLD INFO: fix-
 * Some of these registers apply to only one interface and some
 * apply to both. The registers which apply to the Sun3/50 onboard
 * version only are udc_rdata and udc_raddr. The registers which
 * apply to the Sun3 vme version only are dma_addr, dma_count, bpr,
 * iv_am, and bcrh. Thus, the sbc registers, fifo_data, bcr, and csr
 * apply to both interfaces.
 * One other feature of the vme interface: a write to the dma count
 * register also causes a write to the fifo byte count register and
 * vis versa.
 *
 */



#define	NCR_HWSIZE	sizeof (struct ncrsbc) + sizeof (struct ncrdma) \
			+ sizeof (struct ncrctl)


#define	DMA_BASE_110    0xf00000

/* must massage dvma addresses for Sun3/50 hardware */
#define	DMA_BASE_3_50	(int)(DVMA - (char *)KERNELBASE)

#define	COBRA_CSR_OFF	sizeof (struct ncrsbc) + (3 * sizeof (u_int))
#define	SUN3E_CSR_OFF	sizeof (struct ncrsbc) + ((9 - 1) * sizeof (u_short))
#define	CSR_OFF		sizeof (struct ncrsbc) + ((8 - 1) * sizeof (u_short))

#endif	/* !_scsi_adapters_ncrctl_h */
