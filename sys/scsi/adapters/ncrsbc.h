/* @(#)ncrsbc.h 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef	_scsi_adapters_ncrsbc_h
#define	_scsi_adapters_ncrsbc_h

/*
 * NCR 5380 SBC (SCSI Bus Controller) Registers.
 */

struct ncrsbc {
	u_char		cdr;	/* R:  current data register */
#define	odr	cdr		/* W:  output data register */
	u_char		icr;	/* RW: initiator command register */
	u_char		mr;	/* RW: mode register */
	u_char		tcr;	/* RW: target command register */
	u_char		cbsr;	/* R:  current bus status register */
#define	ser	cbsr		/* W:  select/reselect enable register */
	u_char		bsr;	/* R:  bus and status register */
#define	send	bsr		/* W:  start dma for tgt/initiator send xfer */
	u_char		idr;	/* R:  input data register */
#define	trcv	idr		/* W:  start dma for target receive transfer */
	u_char		clr;	/* R:  read to clear parity error, */
				/* interrupt request, and busy */
				/* failure bits in the bsr */
#define	ircv	clr		/* W:  start dma for initiator rcv transfer */
};

/*
 * bits in the sbc initiator command register
 */

#define	NCR_ICR_RST	0x80	/* (r/w) assert reset */
#define	NCR_ICR_AIP	0x40	/* (r)   arbitration in progress */
#define	NCR_ICR_TEST	0x40	/* (w)   test mode, disables output */
#define	NCR_ICR_LA	0x20	/* (r)   lost arbitration */
#define	NCR_ICR_DE	0x20	/* (w)   differential enable */
#define	NCR_ICR_ACK	0x10	/* (r/w) assert acknowledge */
#define	NCR_ICR_BUSY	0x08	/* (r/w) assert busy */
#define	NCR_ICR_SEL	0x04	/* (r/w) assert select */
#define	NCR_ICR_ATN	0x02	/* (r/w) assert attention */
#define	NCR_ICR_DATA	0x01	/* (r/w) assert data bus */

#define	ICR_BITS	"\20\10RST\7AIP\6LA\5ACK\4BSY\3SEL\2ATN\1DATA"

/*
 * bits in the sbc mode register (same on read or write)
 */

#define	NCR_MR_BDMA	0x80	/* block mode dma */
#define	NCR_MR_TRG	0x40	/* target mode */
#define	NCR_MR_EPC	0x20	/* enable parity check */
#define	NCR_MR_EPI	0x10	/* enable parity interrupt */
#define	NCR_MR_EEI	0x08	/* enable eop interrupt */
#define	NCR_MR_MBSY	0x04	/* monitor busy */
#define	NCR_MR_DMA	0x02	/* dma mode */
#define	NCR_MR_ARB	0x01	/* arbitration mode */

#define	MR_BITS	"\20\2DMA\1ARB"

/*
 * bits in the sbc target command register
 */

#define	NCR_TCR_LAST	0x80	/* Last byte sent */	/* for 53C80 only */
#define	NCR_TCR_REQ	0x08	/* assert request */
#define	NCR_TCR_MSG	0x04	/* assert message */
#define	NCR_TCR_CD	0x02	/* assert command/data */
#define	NCR_TCR_IO	0x01	/* assert input/output */

#define	TCR_BITS	"\20\10LAST\4REQ\3MSG\2CD\1IO"

/*
 * settings of tcr to reflect different information transfer phases
 */

#define	TCR_COMMAND	(NCR_TCR_CD)
#define	TCR_MSG_OUT	(NCR_TCR_MSG | NCR_TCR_CD)
#define	TCR_DATA_OUT	0
#define	TCR_STATUS	(NCR_TCR_CD | NCR_TCR_IO)
#define	TCR_MSG_IN	(NCR_TCR_MSG | NCR_TCR_CD | NCR_TCR_IO)
#define	TCR_DATA_IN	(NCR_TCR_IO)

/*
 * This is actually an illegal phase- we use it to keep the ncr
 * chip from recognizing a phase on the bus
 */

#define	TCR_UNSPECIFIED	(NCR_TCR_MSG)

/*
 * bits in the sbc current bus status register
 */

#define	NCR_CBSR_RST	0x80	/* reset */
#define	NCR_CBSR_BSY	0x40	/* busy */
#define	NCR_CBSR_REQ	0x20	/* request */
#define	NCR_CBSR_MSG	0x10	/* message */
#define	NCR_CBSR_CD	0x08	/* command/data */
#define	NCR_CBSR_IO	0x04	/* input/output */
#define	NCR_CBSR_SEL	0x02	/* select */
#define	NCR_CBSR_DBP	0x01	/* data bus parity */
#define	NCR_CBSR_RESEL	(NCR_CBSR_SEL | NCR_CBSR_IO)

#define	CBSR_BITS	"\20\10RST\7BSY\6REQ\5MSG\4CD\3IO\2SEL\1DBP"

/*
 * scsi bus signals reflecting different information transfer phases
 */

#define	CBSR_PHASE_BITS	(NCR_CBSR_CD | NCR_CBSR_MSG | NCR_CBSR_IO)
#define	PHASE_COMMAND	(NCR_CBSR_CD)
#define	PHASE_MSG_OUT	(NCR_CBSR_MSG | NCR_CBSR_CD)
#define	PHASE_DATA_OUT	0
#define	PHASE_STATUS	(NCR_CBSR_CD | NCR_CBSR_IO)
#define	PHASE_MSG_IN	(NCR_CBSR_MSG | NCR_CBSR_CD | NCR_CBSR_IO)
#define	PHASE_DATA_IN	(NCR_CBSR_IO)

/*
 * bits in the sbc bus and status register
 */

#define	NCR_BSR_EDMA	0x80	/* end of dma */
#define	NCR_BSR_RDMA	0x40	/* dma request */
#define	NCR_BSR_PERR	0x20	/* parity error */
#define	NCR_BSR_INTR	0x10	/* interrupt request */
#define	NCR_BSR_PMTCH	0x08	/* phase match */
#define	NCR_BSR_BERR	0x04	/* busy error */
#define	NCR_BSR_ATN	0x02	/* attention */
#define	NCR_BSR_ACK	0x01	/* acknowledge */

#define	BSR_BITS	"\20\6PERR\5INTR\4PMTCH\3BERR\2ATN\1ACK"


#endif	/* !_scsi_adapters_ncrsbc_h */
