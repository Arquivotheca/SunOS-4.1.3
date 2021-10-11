/*	@(#)screg.h 1.1 92/07/30 SMI	*/

#define	HOST_ADDR	0x00	/* 0x80 is right but Sysgen violates spec */

#ifndef	sun4
/* 
 * SCSI Sun host adapter control registers for Sun-2 and Sun-3.
 */
#define	SET_DMA_ADDR(har,val)	(har)->dma_addr = val;
#define	GET_DMA_ADDR(har)	(har)->dma_addr;

struct	scsi_ha_reg {		/* host adapter (I/O space) registers */
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

#else	sun4
/* 
 * SCSI Sun host adapter control registers for Sun-4.
 */
#define	SET_DMA_ADDR(har,val)	(har)->dma_addrh = (val >> 16);\
				(har)->dma_addrl = (val & 0xffff);
#define	GET_DMA_ADDR(har)	(((har)->dma_addrh  << 16) | ((har)->dma_addrl))

struct	scsi_ha_reg {		/* host adapter (I/O space) registers */
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
