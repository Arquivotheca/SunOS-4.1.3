/* @(#)sereg.h 1.1 92/07/30 SMI	*/


/*
 * Register definitions for the Sun3 32-bit VME version 
 * of the SCSI control logic interface. Both of these interfaces use the
 * NCR 5380 SBC (SCSI Bus Controller). The main difference between these
 * two interfaces is the dma interface.
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

/* scsi bus signals reflecting different information transfer phases */
#define CBSR_PHASE_BITS	(SBC_CBSR_CD | SBC_CBSR_MSG | SBC_CBSR_IO)
#define PHASE_COMMAND	(SBC_CBSR_CD)
#define PHASE_MSG_OUT	(SBC_CBSR_MSG | SBC_CBSR_CD)
#define PHASE_DATA_OUT	0
#define PHASE_STATUS	(SBC_CBSR_CD | SBC_CBSR_IO)
#define PHASE_MSG_IN	(SBC_CBSR_MSG | SBC_CBSR_CD | SBC_CBSR_IO)
#define PHASE_DATA_IN	(SBC_CBSR_IO)

/* bits in the sbc bus and status register */
#define SBC_BSR_EDMA	0x80	/* end of dma */
#define SBC_BSR_RDMA	0x40	/* dma request */
#define SBC_BSR_PERR	0x20	/* parity error */
#define SBC_BSR_INTR	0x10	/* interrupt request */
#define SBC_BSR_PMTCH	0x08	/* phase match */
#define SBC_BSR_BERR	0x04	/* busy error */
#define SBC_BSR_ATN	0x02	/* attention */
#define SBC_BSR_ACK	0x01	/* acknowledge */


struct udc_table {
    u_short rsel;
    u_short haddr;
    u_short laddr;
    u_short count;
    u_short hcmr;
    u_short lcmr;
};

/*
 * Misc defines 
 */

/* arbitraty retry count */
#define SI_NUM_RETRIES		2

/* scsi timer values, all in microseconds */
#define SI_ARBITRATION_DELAY	3
#define SI_BUS_CLEAR_DELAY	1
#define SI_BUS_SETTLE_DELAY	1
#define	SI_WAIT_COUNT		25000

/* directions for dma transfers */
#define SI_NO_DATA		0
#define SI_RECV_DATA		1
#define SI_SEND_DATA		2

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

/* Shorthand, to make the code look a bit cleaner. */
#define sbc_rreg	sbc.read	/* SBC read regs. */
#define SBC_RD		ser->sbc.read	/* ser points to SCSI ha regs */

#define sbc_wreg	sbc.write	/* SBC write regs. */
#define SBC_WR		ser->sbc.write	/* sir points to SCSI ha regs */

#define	SET_DMA_ADDR(ser,val)	(ser)->dma_addr = val;
#define	GET_DMA_ADDR(ser)	(ser)->dma_addr;
#define	SET_DMA_COUNT(ser,val)	(ser)->dma_count = val;
#define	GET_DMA_COUNT(ser)	(ser)->dma_count;

struct scsi_si_reg {
	u_char			dma_buf[65536];	/* DMA buffer		*/
	union {
		struct sbc_read_reg	read;	/* scsi bus ctlr, read reg */
		struct sbc_write_reg	write;	/* scsi bus ctlr, write reg */
	} sbc;
	u_short			unused1;
	u_short			dma_addr;	/* DMA offset register	*/
	u_short			unused2;
	u_short			dma_cntr;	/* DMA count down register */
	u_short			unused3;
	u_short			unused4;
	u_short			unused5;
	u_short			unused6;
	u_short			unused7;
	u_short			csr;		/* control/status register */
	u_short			unused8;
	u_char			unused9;
	u_char			ivec2;		/* interrupt vector	*/
	u_short			unused10;
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
#define SI_CSR_SBC_IP		0x0200	/* (r,b) sbc interrupt pending */
#define SI_CSR_SEND		0x0008	/* (rw,b) dma dir, 1=to device */
#define SI_CSR_INTR_EN		0x0004	/* (rw,b) interrupts enable */
#define	SI_SR_VCC		0x0002	/* (r) power signal to the chip	*/
#define SI_CSR_SCSI_RES		0x0001	/* (rw,b) reset sbc and udc, 0=reset */
