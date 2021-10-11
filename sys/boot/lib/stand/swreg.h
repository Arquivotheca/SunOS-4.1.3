/* @(#)swreg.h       1.1 92/07/30 Copyr 1988 Sun Micro */
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Register definitions for the Sun3/50 onboard and Sun3 32-bit VME version 
 * of the SCSI control logic interface. Both of these interfaces use the
 * NCR 5380 SBC (SCSI Bus Controller). The main difference between these
 * two interfaces is the dma interface. The Sun3/50 onboard version uses
 * the AMD 9516 UDC (Universal DMA Controller).
 * Since the NCR 5380 SBC chip is used, both interfaces support the SCSI
 * disconnect/reconnect capability thus implying support for the scsi
 * arbitration phase.
 */

/*
 * NCR 5380 SBC (SCSI Bus Controller) Registers.
 */

/* read of sbc registers yields the following: */
struct sbc_read_reg {
	u_char		cdr;	/* current data register */
	u_char		icr;	/* initiator command register */
	u_char		mr;	/* mode register */
	u_char		tcr;	/* target command register */
	u_char		cbsr;	/* current bus status register */
	u_char		bsr;	/* bus and status register */
	u_char		idr;	/* input data register */
	u_char		clr;	/* read to clear parity error, */
				/* interrupt request, and busy */
				/* failure bits in the bsr */
};

/* write of sbc registers yields the following: */
struct sbc_write_reg {
	u_char		odr;	/* output data register */
	u_char		icr;	/* initiator command register */
	u_char		mr;	/* mode register */
	u_char		tcr;	/* target command register */
	u_char		ser;	/* select/reselect enable register */
	u_char		send;	/* start dma for target/initiator send xfer */
	u_char		trcv;	/* start dma for target receive transfer */
	u_char		ircv;	/* start dma for initiator receive transfer */
};

/* bits in the sbc initiator command register */
#define	SBC_ICR_RST	0x80	/* (r/w) assert reset */
#define SBC_ICR_AIP	0x40	/* (r)   arbitration in progress */
#define SBC_ICR_TEST	0x40	/* (w)   test mode, disables output */
#define SBC_ICR_LA	0x20	/* (r)   lost arbitration */
#define SBC_ICR_DE	0x20	/* (w)   differential enable */
#define SBC_ICR_ACK	0x10	/* (r/w) assert acknowledge */
#define SBC_ICR_BUSY	0x08	/* (r/w) assert busy */
#define SBC_ICR_SEL	0x04	/* (r/w) assert select */
#define SBC_ICR_ATN	0x02	/* (r/w) assert attention */
#define SBC_ICR_DATA	0x01	/* (r/w) assert data bus */

/* bits in the sbc mode register (same on read or write) */
#define SBC_MR_BDMA	0x80	/* block mode dma */
#define SBC_MR_TRG	0x40	/* target mode */
#define SBC_MR_EPC	0x20	/* enable parity check */
#define SBC_MR_EPI	0x10	/* enable parity interrupt */
#define SBC_MR_EEI	0x08	/* enable eop interrupt */
#define SBC_MR_MBSY	0x04	/* monitor busy */
#define SBC_MR_DMA	0x02	/* dma mode */
#define SBC_MR_ARB	0x01	/* arbitration mode */

/* bits in the sbc target command register */
#define SBC_TCR_LAST	0x80	/* Last byte sent */
#define SBC_TCR_REQ	0x08	/* assert request */
#define SBC_TCR_MSG	0x04	/* assert message */
#define SBC_TCR_CD	0x02	/* assert command/data */
#define SBC_TCR_IO	0x01	/* assert input/output */

/* settings of tcr to reflect different information transfer phases */
#define TCR_COMMAND	(SBC_TCR_CD)
#define TCR_MSG_OUT	(SBC_TCR_MSG | SBC_TCR_CD)
#define TCR_DATA_OUT	0
#define TCR_STATUS	(SBC_TCR_CD | SBC_TCR_IO)
#define TCR_MSG_IN	(SBC_TCR_MSG | SBC_TCR_CD | SBC_TCR_IO)
#define TCR_DATA_IN	(SBC_TCR_IO)
#define TCR_UNSPECIFIED	(SBC_TCR_MSG)

/* bits in the sbc current bus status register */
#define SBC_CBSR_RST	0x80	/* reset */
#define SBC_CBSR_BSY	0x40	/* busy */
#define SBC_CBSR_REQ	0x20	/* request */
#define SBC_CBSR_MSG	0x10	/* message */
#define SBC_CBSR_CD	0x08	/* command/data */
#define SBC_CBSR_IO	0x04	/* input/output */
#define SBC_CBSR_SEL	0x02	/* select */
#define SBC_CBSR_DBP	0x01	/* data bus parity */
#define SBC_CBSR_RESEL	(SBC_CBSR_SEL | SBC_CBSR_IO)

/* scsi bus signals reflecting different information transfer phases */
#define CBSR_PHASE_BITS	(SBC_CBSR_CD | SBC_CBSR_MSG | SBC_CBSR_IO)
#define PHASE_COMMAND	(SBC_CBSR_CD)
#define PHASE_MSG_OUT	(SBC_CBSR_MSG | SBC_CBSR_CD)
#define PHASE_DATA_OUT	0
#define PHASE_STATUS	(SBC_CBSR_CD | SBC_CBSR_IO)
#define PHASE_MSG_IN	(SBC_CBSR_MSG | SBC_CBSR_CD | SBC_CBSR_IO)
#define PHASE_DATA_IN	(SBC_CBSR_IO)

#define PHASE_SPURIOUS		0x80	/* for driver use only */
#define PHASE_ARBITRATION	0x81	/* for driver use only */
#define PHASE_IDENTIFY		0x82	/* for driver use only */
#define PHASE_SAVE_PTR		0x83	/* for driver use only */
#define PHASE_RESTORE_PTR	0x84	/* for driver use only */
#define PHASE_DISCONNECT	0x85	/* for driver use only */
#define PHASE_RECONNECT		0x86	/* for driver use only */
#define PHASE_CMD_CPLT		0x87	/* for driver use only */

/* bits in the sbc bus and status register */
#define SBC_BSR_EDMA	0x80	/* end of dma */
#define SBC_BSR_RDMA	0x40	/* dma request */
#define SBC_BSR_PERR	0x20	/* parity error */
#define SBC_BSR_INTR	0x10	/* interrupt request */
#define SBC_BSR_PMTCH	0x08	/* phase match */
#define SBC_BSR_BERR	0x04	/* busy error */
#define SBC_BSR_ATN	0x02	/* attention */
#define SBC_BSR_ACK	0x01	/* acknowledge */

/*
 * AMD 9516 UDC (Universal DMA Controller) Registers.
 * Sun3/50 only.
 */

/* addresses of the udc registers accessed directly by driver */
#define UDC_ADR_MODE		0x38	/* master mode register */
#define UDC_ADR_COMMAND		0x2e	/* command register (write only) */
#define UDC_ADR_STATUS		0x2e	/* status register (read only) */
#define UDC_ADR_CAR_HIGH	0x26	/* chain addr reg, high word */
#define UDC_ADR_CAR_LOW		0x22	/* chain addr reg, low word */
#define UDC_ADR_CARA_HIGH	0x1a	/* cur addr reg A, high word */
#define UDC_ADR_CARA_LOW	0x0a	/* cur addr reg A, low word */
#define UDC_ADR_CARB_HIGH	0x12	/* cur addr reg B, high word */
#define UDC_ADR_CARB_LOW	0x02	/* cur addr reg B, low word */
#define UDC_ADR_CMR_HIGH	0x56	/* channel mode reg, high word */
#define UDC_ADR_CMR_LOW		0x52	/* channel mode reg, low word */
#define UDC_ADR_COUNT		0x32	/* number of words to transfer */

/* 
 * For a dma transfer, the appropriate udc registers are loaded from a 
 * table in memory pointed to by the chain address register.
 */
struct udc_table {
	u_short			rsel;	/* tells udc which regs to load */
	u_short			haddr;	/* high word of main mem dma address */
	u_short			laddr;	/* low word of main mem dma address */
	u_short			count;	/* num words to transfer */
	u_short			hcmr;	/* high word of channel mode reg */
	u_short			lcmr;	/* low word of channel mode reg */
};

/* indicates which udc registers are to be set based on info in above table */
#define UDC_RSEL_RECV		0x0182
#define UDC_RSEL_SEND		0x0282

/* setting of chain mode reg: selects how the dma op is to be executed */
#define UDC_CMR_HIGH		0x0040	/* high word of channel mode reg */
#define UDC_CMR_LSEND		0x00c2	/* low word of cmr when send */
#define UDC_CMR_LRECV		0x00d2	/* low word of cmr when receiving */

/* setting for the master mode register */
#define UDC_MODE		0xd	/* enables udc chip */

/* setting for the low byte in the high word of an address */
#define UDC_ADDR_INFO		0x40	/* inc addr after each word is dma'd */

/* udc commands */
#define UDC_CMD_STRT_CHN	0xa0	/* start chaining */
#define UDC_CMD_CIE		0x32	/* channel 1 interrupt enable */
#define UDC_CMD_RESET		0x00	/* reset udc, same as hdw reset */

/* bits in the udc status register */
#define UDC_SR_CIE		0x8000	/* channel interrupt enable */
#define UDC_SR_IP		0x2000	/* interrupt pending */
#define UDC_SR_CA		0x1000	/* channel abort */
#define UDC_SR_NAC		0x0800	/* no auto reload or chaining*/
#define UDC_SR_WFB		0x0400	/* waiting for bus */
#define UDC_SR_SIP		0x0200	/* second interrupt pending */
#define UDC_SR_HM		0x0040	/* hardware mask */
#define UDC_SR_HRQ		0x0020	/* hardware request */
#define UDC_SR_MCH		0x0010	/* match on upper comparator byte */
#define UDC_SR_MCL		0x0008	/* match on lower comparator byte */
#define UDC_SR_MC		0x0004	/* match condition ended dma */
#define UDC_SR_EOP		0x0002	/* eop condition ended dma */
#define UDC_SR_TC		0x0001	/* termination of count ended dma */

/*
 * Misc defines 
 */

/* arbitraty retry count */
#define SI_ARB_RETRIES		20
#define SI_SEL_RETRIES		5

/* scsi timer values, all in microseconds */
#define SI_ARBITRATION_DELAY	3	/*  1 us */
#define SI_BUS_CLEAR_DELAY	1	/*  1 us */
#define SI_BUS_SETTLE_DELAY	1	/*  1 us */
#define SI_UDC_WAIT		1	/*  1 us */
#define SI_ARB_WAIT		5	/* 10 us */

/* directions for dma transfers */
#define	SI_NO_DATA	0
#define SI_RECV_DATA	1
#define	SI_SEND_DATA	2

/* initiator's scsi device id */
#define	SI_HOST_ID		0x80

/* possible values for the address modifier, sun3 vme version only */
#define VME_SUPV_DATA_24	0x3d00

/* must massage dvma addresses for Sun3/50 hardware */
#define DVMA_OFFSET	(int)(DVMA - (char *)KERNELBASE)

/* 
 * Register layout for the SCSI control logic interface.
 * Some of these registers apply to only one interface and some
 * apply to both. The registers which apply to the Sun3/50 onboard 
 * version only are udc_rdata and udc_raddr. The registers which
 * apply to the Sun3 vme version only are dma_addr, dma_count, bpr,
 * iv_am, and bcrh. Thus, the sbc registers, fifo_data, bcr, and csr 
 * apply to both interfaces.
 * One other feature of the vme interface: a write to the dma count 
 * register also causes a write to the fifo byte count register and
 * vis versa.
 */

#define	SET_DMA_ADDR(swr,val)	(swr)->dma_addr = val;
#define	GET_DMA_ADDR(swr)	(swr)->dma_addr;
#define	SET_DMA_COUNT(swr,val)	(swr)->dma_count = val;
#define	GET_DMA_COUNT(swr)	(swr)->dma_count;


struct scsi_sw_reg {
	union {
		struct sbc_read_reg	read;	/* scsi bus ctlr, read reg */
		struct sbc_write_reg	write;	/* scsi bus ctlr, write reg */
	} sbc;
#define sbc_rreg sbc.read
#define SBC_RD swr -> sbc.read
#define sbc_wreg sbc.write
#define SBC_WR swr -> sbc.write
	u_int			dma_addr;	/* dma address register */
	u_int			dma_count;	/* dma count register */
	u_int			bcr;		/* not really there */
	u_int			csr;		/* control/status register */
	u_int			bpr;		/* byte pack register */
};

/*
 * Status Register.
 * Note:
 *	(r)	indicates bit is read only.
 *	(rw)	indicates bit is read or write.
 *	(v)	vme host adaptor interface only.
 *	(o)	sun3/50 onboard host adaptor interface only.
 *	(b)	both vme and sun3/50 host adaptor interfaces.
 */
#define SI_CSR_DMA_ACTIVE	0x8000	/* (r,o) dma transfer active */
#define SI_CSR_DMA_CONFLICT	0x4000	/* (r,b) reg accessed while dmaing */
#define SI_CSR_DMA_BUS_ERR	0x2000	/* (r,b) bus error during dma */
#define SI_CSR_ID		0x1000	/* (r,b) 0 for 3/50, 1 for SCSI-3, */
					/* 0 if SCSI-3 unmodified */
#define SI_CSR_FIFO_FULL	0x0800	/* (r,b) fifo full */
#define SI_CSR_FIFO_EMPTY	0x0400	/* (r,b) fifo empty */
#define SI_CSR_SBC_IP		0x0200	/* (r,b) sbc interrupt pending */
#define SI_CSR_DMA_IP		0x0100	/* (r,b) dma interrupt pending */
#define SI_CSR_LOB		0x00c0	/* (r,v) number of leftover bytes */
#define SI_CSR_LOB_THREE	0x00c0	/* (r,v) three leftover bytes */
#define SI_CSR_LOB_TWO		0x0080	/* (r,v) two leftover bytes */
#define SI_CSR_LOB_ONE		0x0040	/* (r,v) one leftover byte */
#define SI_CSR_BPCON		0x0020	/* (rw,v) byte packing control */
					/* dma is in 0=longwords, 1=words */
#define SI_CSR_DMA_EN		0x0010	/* (rw,v) dma enable */
#define SI_CSR_SEND		0x0008	/* (rw,b) dma dir, 1=to device */
#define SI_CSR_INTR_EN		0x0004	/* (rw,b) interrupts enable */
#define SI_CSR_FIFO_RES		0x0002	/* (rw,b) inits fifo, 0=reset */
#define SI_CSR_SCSI_RES		0x0001	/* (rw,b) reset sbc and udc, 0=reset */

