 
/*      @(#)vpcreg.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Registers for Systech VPC-2200 Versatec/Centronics interface.
 * Warning - read bits are not identical to written bits.
 */

#ifndef _sundev_vpcreg_h
#define _sundev_vpcreg_h

typedef struct xchannel {
        u_char  addr;           /* DMA word address */
        u_char  count;          /* DMA word count */
} Channel;

typedef struct xcontrol {
        u_char  hiaddr;          /* high address byte */
        u_char  interrupt;       /* interrupt status(r) / interrupt clear(w) */
        u_char  csr;          	 /* command and status */
        u_char  misc;            /* versatec programmed, printer reset IF */
} Control;

/*
 * 32 byte structure containing definitions for the control registers
 */
struct vpcdevice {
	/*
	 * fields for the 8237 DMA controller cheep
	 */
	Channel	channel[2];	/* 00-01: versatec count and address fields */
				/* 02-03: printer count and address fields */
	Channel ch_dummy[2];	/* 04-07: placeholder for channels 2 and 3 */
	u_char	vpc_dmacsr;	/* 08: 8237 command and status */
	u_char	vpc_dmareq;	/* 09: request */
	u_char	vpc_smb;	/* 0A: single mask bit */
	u_char	vpc_mode;	/* 0B: dma mode */
	u_char	vpc_clearff;	/* 0C: clear byte pointer flip-flop */
	u_char	vpc_clear;	/* 0D: DMA master clear */
	u_char	vpc_clrmask;	/* 0E: clear mask register */
	u_char	vpc_allmask;	/* 0F: all mask bits */
	/*
	 * fields for the controller / command registers
	 */
	Control	control[2];	/* 10-13: versatec control registers */
				/* 14-17: printer control registers */
	u_char	dummy_1;	/* 19: illegal */
	u_char	dummy_2;	/* 18: illegal */
	u_char	dummy_3;	/* 1B: illegal */
	u_char	dummy_4;	/* 1A: illegal */
	u_char	dummy_5;	/* 1D: illegal */
	u_char	dummy_6;	/* 1C: illegal */
	u_char	dummy_7;	/* 1F: illegal */
	u_char	dummy_8;	/* 1E: illegal */
};

#define VPC_ANY			0xff

/*
 * 8237 commands and status bits:
 */
#define	VPC_START		0x00	/* start command for chip */
#define VPC_STOP		0x04	/* stop command for chip */
#define	VPC_MVPCMODE		0x48	/* single mode / read transfer */

/* 
 * Interrupt Control bits:                      
 */
#define VPC_IC_CLEAR		0x80    /* Clear interrupt request */
#define VPC_IC_CL_ATN       	0x40    /* Clear "attention" bit */
#define VPC_IC_DIS_ATN      	0x20    /* Disable "attention" bit */
#define VPC_IC_EN_ATN       	0x10    /* Enable "attention" bit */
#define VPC_IC_CL_DONE      	0x08    /* Clear/Disable "done" bit*/
#define VPC_IC_EN_XDONE     	0x04    /* Enable "done" on xfer complete */
#define VPC_IC_EN_RDONE     	0x02    /* Enable "done" on READY (v_ only) */
#define VPC_IC_ODD_XFER     	0x01    /* Odd byte count (word mode only) */

/* 
 * Interrupt Status bits:
 */
#define VPC_IS_INT          	0x80    /* Interrupt request */
#define VPC_IS_ATN          	0x40    /* Attention bit */
#define VPC_IS_EOPL 		0x20    /* 8237 EOP signal, latched */
#define VPC_IS_ERR          	0x10    /* Memory error */
#define VPC_IS_DONE 		0x08    /* Done bit */
#define VPC_IS_EN_XDONE     	0x04    /* "Done" on xfer complete enabled */
#define VPC_IS_ENRDONE      	0x02    /* "Done" on READY enabled (v_ only) */
#define VPC_IS_ODD_XFER     	0x01    /* Odd byte count set */

/* 
 * Printer Control (Versatec) bits:    
 */
#define VPC_PC_V_PLOT       	0x80    /* Set plot mode */
#define VPC_PC_V_SPP        	0x40    /* Set simultaneous print/plot mode */
#define VPC_PC_V_EOT        	0x10    /* Send a remote end of transmission */
#define VPC_PC_V_FFEED      	0x08    /* Send a remote form feed */
#define VPC_PC_V_LTER       	0x04    /* Send a remote line terminate */
#define VPC_PC_V_CLEAR      	0x02    /* Send a remote clear */
#define VPC_PC_V_RESET      	0x01    /* Send a remote reset */

/* 
 * Printer Status bits: 
 */
#define VPC_PS_V_PLOT       	0x80    /* Versatec is in plot mode(v_ only) */
#define VPC_PS_V_SPP        	0x40    /* Versatec is in SPP mode (v_ only) */
#define VPC_PS_V_BFMT       	0x10    /* Versatec buffer empty (v_ only) */
#define VPC_PS_V_RDY        	0x08    /* Versatec is ready (v_ only) */
#define VPC_PS_PAPER        	0x04    /* Out of paper */
#define VPC_PS_ONLINE       	0x02    /* Device on line */
#define VPC_PS_OK           	0x01    /* Device OK; (has paper, etc.) */

/* 
 * Board control bits: 
 */
#define VPC_BC_EN_INT       	0x01    /* Enable board interrupts */
#define VPC_BC_DIS_INT      	0x00    /* Disable board interrupts */

#endif /*!_sundev_vpcreg_h*/
