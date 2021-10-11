/*
 * @(#)screg.h 1.1 92/07/30 SMI
 *
 * Copyright (C) 1989, Sun Microsystems Inc.
 *
 * Hardware and Software definitions for the SCSI-2 Host Adapter.
 *
 */

#ifndef	_scsi_adapters_scsitwo_h
#define	_scsi_adapters_scsitwo_h

	/*
	 * Since we are a non-arbitrating, non-disconnecting
	 * host adapter (initiator), these are for reference
	 * only:
	 */
#define	HOST_ADDR	0x80	/* bit pattern representation */
#define	HOST_ID		0x07	/* number of My ID */

#define SC_RESET_DELAY		4000000	/*  1 us  ( 4 Sec.) */
#define SC_LONG_WAIT		3000000	/* 10 us  (30 Sec.) */
#define SC_WAIT_COUNT		1000000	/* 10 us  (10 Sec.) */
#define SC_SHORT_WAIT		25000	/* 10 us  (.25 Sec.) */


#if	defined(sun2)  ||  defined(sun3) || defined(sun3x)
/* 
 * SCSI Sun host adapter control registers for Sun-2 and Sun-3.
 */
#define	SET_DMA_ADDR(har,val)	(har)->dma_addr = val;
#define	GET_DMA_ADDR(har)	(har)->dma_addr;

struct	screg {			/* host adapter (I/O space) registers */
	u_char	data;		/* data register */
	u_char	unused;
	u_char	cmd_stat;	/* command/status register */
	u_char	unused2;
	u_short	icr;		/* interface control register */
	u_short	unused3;
	u_long	dma_addr;	/* dma base address */
	u_short	dma_count;	/* dma count register */
	u_char	unused4;
	u_char	intvec;		/* interrupt vector for VMEbus versions */
};
#endif	defined(sun2)  ||  defined(sun3) || defined(sun3x)


#ifdef	sun4
/* 
 * SCSI Sun host adapter control registers for Sun-4.
 */
#define	SET_DMA_ADDR(har,val)	(har)->dma_addrh = (val >> 16);\
				(har)->dma_addrl = (val & 0xffff);
#define	GET_DMA_ADDR(har)	(((har)->dma_addrh  << 16) | ((har)->dma_addrl))

struct	screg {			/* host adapter (I/O space) registers */
	u_char	data;		/* data register */
	u_char	unused;
	u_char	cmd_stat;	/* command/status register */
	u_char	unused2;
	u_short	icr;		/* interface control register */
	u_short	unused3;
	u_short	dma_addrh;	/* dma base address */
	u_short	dma_addrl;
	u_short	dma_count;	/* dma count register */
	u_char	unused4;
	u_char	intvec;		/* interrupt vector for VMEbus versions */
};
#endif	sun4


/*
 * bits in the interface control register 
 */
#define	ICR_PARITY_ERROR	0x8000
#define	ICR_BUS_ERROR		0x4000
#define	ICR_ODD_LENGTH		0x2000
#define	ICR_INTERRUPT_REQUEST	0x1000
#define	ICR_REQUEST		0x0800
#define	ICR_MESSAGE		0x0400
#define	ICR_COMMAND_DATA	0x0200	/* command=1, data=0 */
#define	ICR_INPUT_OUTPUT	0x0100	/* input=1, output=0 */
#define	ICR_PARITY		0x0080
#define	ICR_BUSY		0x0040
/* Only the following bits may usefully be set by the CPU */
#define	ICR_SELECT		0x0020
#define	ICR_RESET		0x0010
#define	ICR_PARITY_ENABLE	0x0008
#define	ICR_WORD_MODE		0x0004
#define	ICR_DMA_ENABLE		0x0002
#define	ICR_INTERRUPT_ENABLE	0x0001

/*
 * Compound conditions of icr bits message, command/data and input/output.
 */
#define	ICR_COMMAND	(ICR_COMMAND_DATA)
#define	ICR_STATUS	(ICR_COMMAND_DATA | ICR_INPUT_OUTPUT)
#define	ICR_MESSAGE_IN	(ICR_MESSAGE | ICR_COMMAND_DATA | ICR_INPUT_OUTPUT)
#define	ICR_BITS	(ICR_MESSAGE | ICR_COMMAND_DATA | ICR_INPUT_OUTPUT)

/*
 * 'soft' structure, per host adapter
 */

struct scsitwo {
	/*
	 * This structure must come first, as we retrieve it's address
	 * and figure out which host adapter controller number we are
	 * talking to based upon it...
	 */
	struct scsi_transport	sc_tran;	/* transport vector */
	struct screg		*sc_reg;	/* hardware registers */
	struct scsi_cmd		*sc_que;	/* waiting list */
	u_char			sc_cmdid;	/* id of command */
	u_char			sc_busy;	/* h/a busy */
	u_char			sc_msgin;	/* last message in */
};

#endif	_scsi_adapters_scsitwo_h
