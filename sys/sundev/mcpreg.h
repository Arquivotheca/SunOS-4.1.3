/* @(#)mcpreg.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Sun MCP Multiprotocol Communication Processor
 * Sun ALM-2 Asynchronous Line Multiplexer
 * 
 * Chip, buffer, and register definitions for 8237 DMA contoller, Z8536 CIO,
 * Z8530 SCC, and MCP board (in that order).
 */

#ifndef _sundev_mcpreg_h
#define _sundev_mcpreg_h

/********************************************************************
 *	Intel 8237 DMA controller
 */

struct addr_wc {
	u_char          baddr;		/* base and current address register */
	u_char          wc;		/* base and current word count register */
};

struct dma_device {
	struct addr_wc  addr_wc[4];
	u_char          csr;		/* w - command port,  r - status port */
	u_char          request;	/* w - single request port */
	u_char          mask;		/* w - single mask port */
	u_char          mode;		/* w - mode port */
	u_char          clrff;		/* w - clear first/last flip-flop */
	u_char          reset;		/* w - master clear */
	u_char          clr_mask;
	u_char          w_all_mask;
};

/* dma channel information */

struct dma_chan {
	struct dma_chip *d_chip;	/* point back to the dma chip */
	struct addr_wc *d_myport;	/* I/O port of channel address and
					 * count */
	char            d_chan;		/* channel (0 - 3) */
	char            d_dir;		/* direction of DMA, 0 is read */
};

/* dma chip information */

struct dma_chip {
	struct dma_device *d_ioaddr;	/* dma chip address in MCP board */
	struct dma_chan d_chans[4];	/* one chip has 4 channel port  */
	u_char          d_mask;		/* mask of active channels  */
	u_char         *ram_base;	/* start of the ram buffer */
};

#define TX_DIR  1
#define RX_DIR  0
#define SCC_DMA 0
#define PR_DMA  1

#define CHIP(x) ((x) >> 2)
#define CHAN(x) ((x) & 0x03)

/*
 * Register and bit definitions for Intel 8237
 */

#define	DMA_OFF_CMD	8		/* w - command port */
#define	DMA_OFF_STAT	8		/* r - status port */
#define	DMA_OFF_REQ	9		/* w - single request port */
#define	DMA_OFF_MASK	10		/* w - single mask port */
#define	DMA_OFF_MODE	11		/* w - mode port */
#define	DMA_OFF_CLRFF	12		/* w - clear first/last flip-flop */
#define	DMA_OFF_RESET	13		/* w - master clear */
#define	DMA_OFF_ADDR(dma)	(dma->d_myport)
#define	DMA_OFF_COUNT(dma)	(dma->d_myport+1)

#define	DMA_CMD_DISABLE	0x04		/* controller disable */
#define	DMA_CMD_COMP	0x08		/* compressed timing */
#define	DMA_CMD_ROTPRI	0x10		/* rotating priority */
#define	DMA_CMD_EXTWRT	0x20		/* extended write */
#define	DMA_CMD_DREQLOW 0x40		/* DREQ active low */
#define	DMA_CMD_DACKHI	0x80		/* DACK active high */

#define	DMA_MODE_WRITE	0x04		/* direction is write */
#define	DMA_MODE_READ	0x08		/* direction is read */
#define	DMA_MODE_AUTO	0x10		/* autoinit enable */
#define	DMA_MODE_DECR	0x20		/* decrement addresses */

#define DMA_MODE_DEMAND  0x00		/* demand transfer mode select */
#define DMA_MODE_SINGLE  0x40		/* single transfer mode select */
#define DMA_MODE_BLOCK   0x80		/* block transfer mode select */
#define DMA_MODE_CASCADE 0xC0		/* cascade transfer mode select */

#define	DMA_MASK_CLEAR	0		/* channel + clear mask bit */
#define	DMA_MASK_SET	4		/* channel + set mask bit */

/********************************************************************
 *	Zilog Z8536 CIO Counter/Timer & Parallel I/O Unit
 */

struct ciochip {
	u_char          portc_data;	/* port C data register */
	u_char          res1;
	u_char          portb_data;	/* port b data register */
	u_char          res2;
	u_char          porta_data;	/* port a data register */
	u_char          res3;
	u_char          cio_ctr;	/* pointer register */
	u_char          res4;
};

/* CIO chip register address */

/* Main Control Registers */
#define  CIO_MICR     0x00		/* master interrupt control  */
#define  CIO_MCCR     0x01		/* master configuration control  */
#define  CIO_PA_IVR   0x02		/* port A interrupt vector  */
#define  CIO_PB_IVR   0x03		/* port B interrupt vector  */
#define  CIO_CT_IVR   0x04		/* counter/timer interrupt vector  */
#define  CIO_PC_DPPR  0x05		/* port C data path polarity */
#define  CIO_PC_DDR   0x06		/* port C data direction */
#define  CIO_PC_SIOCR 0x07		/* port C special I/O control */

/* Most Often Accessed Registers */
#define  CIO_PA_CSR   0x8		/* port A command and status */
#define  CIO_PB_CSR   0x9		/* port B command and status */
#define  CIO_CT1_CSR  0x0A		/* counter/timer 1 command and status */
#define  CIO_CT2_CSR  0x0B		/* counter/timer 2 command and status */
#define  CIO_CT3_CSR  0x0C		/* counter/timer 3 command and status */
#define  CIO_PA_DATA  0x0D		/* port A data */
#define  CIO_PB_DATA  0x0E		/* port B data */
#define  CIO_PC_DATA  0x0F		/* port C data */

/* Port A Specification Registers */
#define  CIO_PA_MODE  0x20		/* port A mode specification */
#define  CIO_PA_HANDSHAKE  0x21		/* port A handshake specification */
#define  CIO_PA_DPPR  0x22		/* port A data path polarity */
#define  CIO_PA_DDR   0x23		/* port A data direction */
#define  CIO_PA_SIOCR 0x24		/* port A special I/O control */
#define  CIO_PA_PP    0x25		/* port A pattern polarity */
#define  CIO_PA_PT    0x26		/* port A pattern transition */
#define  CIO_PA_PM    0x27		/* port A pattern mask */

/* Port B Specification Registers */
#define  CIO_PB_MODE  0x28		/* port B mode specification */
#define  CIO_PB_HANDSHAKE  0x29		/* port B handshake specification */
#define  CIO_PB_DPPR  0x2A		/* port B data path polarity */
#define  CIO_PB_DDR   0x2B		/* port B data direction */
#define  CIO_PB_SIOCR 0x2C		/* port B special I/O control */
#define  CIO_PB_PP    0x2D		/* port B pattern polarity */
#define  CIO_PB_PT    0x2E		/* port B pattern transition */
#define  CIO_PB_PM    0x2F		/* port B pattern mask */

/* CIO chip register value */
#define ALL_INPUT          0xFF		/* Port Data Direction Register */
#define ALL_ONE            0xFF		/* Port Pattern Mask Register(PPM) and */
#define BIT_PORT_MODE      0x06		/* Port Mode Specification Register */
					/* value also indicates "or_priority */
					/* encoded vector" mode */
#define CIO_CLRIP	   0x20		/* port command and status reg */
#define CIO_FIFO_EMPTY     0x80		/* Port A Data Register */
#define CIO_FIFO_EPTY	   0x10		/* port A data register */
#define CIO_FIFO_FULL	   0x40		/* port A data register */
#define CIO_IP		   0x90		/* port command and status reg */
#define CLR_IP             0xDF		/* clear IP in port c/s reg */
#define CLR_RESET          0x0		/* Master Interrupt Control Register */
#define EOP_INVERT         0x3F		/* Port B Data Path Polarity Register */
#define EOP_ONE            0x3F		/* Port B Pattern Registers */
#define EOP_ONES_CATCHER   0x1F		/* Port A Special IO Control Register */
#define FIFO_EMPTY_INTR_ONLY 0x10	/* FIFO empty status interrupt in
					 * Pattern Mask reg */
#define FIFO_NON_INVERT    0x0F		/* Port A Data Path Polarity Register */
#define FIFO_NOT_ONE       0xEF		/* FIFO not empty in Pattern Polarity Reg */
#define INPUT_BIT          0x1		/* Port Data Direction Register */
#define INVERTING          0x1		/* Port Data Path Polarity Register */
#define MASTER_ENABLE      0x94		/* Master Configuration Control Register */
#define MASTER_INT_ENABLE  0x9E		/* Master Interrupt Control Register */
#define MCP_IE             0x08		/* set VMEINTEN to enable interrupts */
#define NON_INVERTING      0x0		/* Port Data Path Polarity Register */
#define NORMAL_IO          0x0		/* Port Special I/O Control Register */
#define ONES_CATCHER       0x1		/* Port Special I/O Control Register */
#define OUTPUT_BIT         0x0		/* Port Data Direction Register */
#define PORT0_RS232_SEL    0x1		/* Port C Data Register masks for */
#define PORT1_RS232_SEL    0x2		/* RS232/449 selection...0 == 449 */
#define PORT_INT_ENABLE    0xC0		/* Port Command and Status Register */

/* Macros to access the registers */

#define CIO_RX_DMARESET(x)   ((x)->portb_data = 0xEF)
#define CIO_DMARESET(x,y)	((x)->portb_data = ~(1 << (((y) & 0x0e) >> 1)))
#define CIO_MCPBASE(x)		(((x) & 0x0e) << 1)

#define CIO_READ(cp, reg, var) {\
	(cp)->cio_ctr = reg; \
	var = (cp)->cio_ctr; \
}

#define CIO_WRITE(cp, reg, val) {\
	(cp)->cio_ctr = reg; \
	(cp)->cio_ctr = val; \
}

/********************************************************************
 *	Zilog Z8530 SCC Serial Communications Controller
 *
 *	Most definitions are shared with the zs driver.
 */

#define SCC_WRITE(port, reg, val)  { \
	(port)->zscc_control = reg;\
	DELAY(2);\
	(port)->zscc_control = val;\
	DELAY(2);\
	zs->zs_wreg[reg] = val; \
}

#define SCC_READ(port, reg, var) { \
	(port)->zscc_control = reg; \
	DELAY(2);\
	var = (port)->zscc_control; \
	DELAY(2);\
}

#define SCCBIS(port, reg, val) {\
	(port)->zscc_control = reg; \
	DELAY(2);\
	zs->zs_wreg[reg] |= val; \
	(port)->zscc_control = zs->zs_wreg[reg]; \
	DELAY(2);\
}

#define SCCBIC(port,reg, val) { \
	(port)->zscc_control = reg; \
	DELAY(2);\
	zs->zs_wreg[reg] &= ~val; \
	(port)->zscc_control = zs->zs_wreg[reg]; \
	DELAY(2);\
}


/********************************************************************
 * 	MCP on-board buffers
 */

#define ASYNC_BSZ 256
#define PR_BSZ	  4096

struct rambuf {
	u_char          syncbuf[4][1024 * 12];		/* for sync driver */
	u_char          asyncbuf[16][ASYNC_BSZ];	/* for async driver */
};

/* logical data structure for each fifo word on MCP */

struct fifo_val {
	u_char          data;		/* received fifo character */
	u_char          port;		/* second half part of FIFO 16 bits word */
					/* bit8 to 13 indicate the port address */
					/* bit 15 indicate fifo empty status */
};

/* data structure for 8 bits r/w within 32 bits word */
struct longword {
	u_char          ctr;		/* valid character */
	u_char          res[3];		/* unused */
};

/* defines each xoff word of the xbuf on the MCP */
struct xbuf {
	u_char          xoff;		/* character to suspend the transmission */
	u_char          res[3];
};

/* registers and chips on MCP communication board */

#define NDMA_CHIPS	6

struct mcp_device {
	struct zscc_device sccs[64];	/* mcp offset 0x0 */
	struct dma_device dmas[16];	/* mcp offset 0x0100 */
	struct longword devctl[64];	/* mcp offset 0x0200 */
	struct xbuf     xbuf[64];	/* mcp offset 0x0300 */
	struct ciochip  cio;		/* mcp offset 0x0400 */
	unsigned char   res1[248];	/* fill in the gap */
	struct longword devvector;	/* mcp offset 0x0500 */
	unsigned char   res2[253];
	unsigned char   ivec;		/* mcp offset 0x0600 */
	unsigned char   res3[254];
	unsigned short  reset_bid;	/* board id (R), reset (W) */
					/* mcp offset 0x0700 */
	unsigned char   res4[0x8FE];
	u_short         fifo[2048];	/* mcp offset 0x1000 */
	struct rambuf   ram;		/* mcp offset 0x2000 */
	unsigned char   printer_buf[PR_BSZ];	/* offset 0xf000 */
};

#define FIFO_EMPTY	0xffff		/* value indicate fifo empty */
#define DISABLE_XOFF	0x80		/* turn off xoff capability */
#define	PCLK		(19660800/4)	/* basic clock rate for UARTs */
					/* same for both async & mcp_sdlc */
#define MCP_DTR_ON	0x2		/* turn on the DTR signal (RS232) */
					/* or "Terminal Ready" signal (RS449) */
#define EN_FIFO_RX	0x4		/* reception into FIFO enabled */
#define NOT_EN_FIFO_RX	0xfb		/* disable reception into FIFO */
#define MCP_NOINTR	0xff
#define EN_RS449_TX	0xEF		/* enable Rx449 drivers */

#endif /*!_sundev_mcpreg_h*/
