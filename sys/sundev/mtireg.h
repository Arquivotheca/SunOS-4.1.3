/*	@(#)mtireg.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Registers and bits for Systech MTI-800/1600 terminal interface
 */

#ifndef _sundev_mtireg_h
#define _sundev_mtireg_h

/*
 * I/O registers, read-only (ro) and write-only (wo).
 */
struct	mtireg {
	u_char	mtiie;			/* Interrupt Enable bits (wo) */
#define		mtistat	mtiie		/* Status (ro) */
	u_char	mticmd;			/* Command FIFO (wo) */
#define		mtiresp	mticmd		/* Response FIFO (ro) */
	u_char	mtireset;		/* Reset board (wo) */
#define		mticlrtim mtireset	/* Clear timer (ro) */
	u_char	mtigo;			/* Set Execute (wo) */
#define		mticra	mtigo		/* Clear Response Available (ro) */
	u_char	mtires[4];		/* Reserved */
};

/*
 * Commands
 */
#define	MTI_ESCI	0x00		/* Enable Single Character Input */
#define	MTI_RSTAT	0x10		/* Read USART Status Register */
#define	MTI_RERR	0x20		/* Read Error Code */
#define	MTI_OUT		0x40		/* Single Character Output */
#define	MTI_WCMD	0x50		/* Write USART Command Register */
#define	MTI_DSCI	0x60		/* Disable Single Character Input */
#define	MTI_CONFIG	0x70		/* Configuration Command */
#define	MTI_BLKIN	0x80		/* Block Input */
#define	MTI_ABORTIN	0xA0		/* Abort Input */
#define	MTI_SUSPEND	0xB0		/* Suspend Output */
#define	MTI_BLKOUT	0xC0		/* Block Output */
#define	MTI_RESUME	0xD0		/* Resume Output */
#define	MTI_ABORTOUT	0xE0		/* Abort Output */

/*
 * Status bits
 */
#define	MTI_VD		0x80		/* Valid Data */
#define	MTI_TIMER	0x10		/* Timer Ticked */
#define	MTI_ERR		0x04		/* Command Error */
#define	MTI_RA		0x02		/* Response Available */
#define	MTI_READY	0x01		/* Ready to Accept Command */

/*
 * Configure Commands
 */
#define	MTIC_ASYNC	0x00		/* Configure Asynchronous */
#define	MTIC_SYNC	0x10		/* Configure Synchronous */
#define	MTIC_INPUT	0x40		/* Configure Input */
#define	MTIC_TMASK	0x50		/* Termination Mask */
#define	MTIC_OUTPUT	0x60		/* Configure Output */
#define MTIC_MODEM	0x70		/* Modem Status */
#define	MTIC_BUFSIZ	0x80		/* Buffer Sizes */
#define	MTIC_TIMER	0x90		/* Configure Timer */

/*
 * USART Registers, Signetics 2661
 */
#define	MTI_MR1_1STOP	0x40		/* 1 stop bit */
#define	MTI_MR1_1_5STOP	0x80		/* 1.5 stop bits */
#define	MTI_MR1_2STOP	0xC0		/* 2 stop bits */
#define	MTI_MR1_EPAR	0x20		/* even parity */
#define	MTI_MR1_PENABLE	0x10		/* party enable */
#define	MTI_MR1_BITS5	0x00		/* 5 bit characters */
#define	MTI_MR1_BITS6	0x04		/* 6 bit characters */
#define	MTI_MR1_BITS7	0x08		/* 7 bit characters */
#define	MTI_MR1_BITS8	0x0C		/* 8 bit characters */
#define	MTI_MR1_X1_CLK	0x01		/* async 1X clock */
#define	MTI_MR1_X16_CLK	0x02		/* async 16X clock */
#define	MTI_MR1_X64_CLK	0x03		/* async 64X clock */

#define	MTI_MR2_INIT	0x30		/* internal RX and TX clocks */

#define	MTI_CR_RTS	0x20		/* request to send */
#define	MTI_CR_RESET	0x10		/* reset error flags */
#define	MTI_CR_BREAK	0x08		/* force break */
#define	MTI_CR_RXEN	0x04		/* receiver enable */
#define	MTI_CR_DTR	0x02		/* data terminal ready */
#define	MTI_CR_TXEN	0x01		/* transmitter enable */

#define	MTI_SR_DSR	0x80		/* data set ready */
#define	MTI_SR_DCD	0x40		/* data carrier detect */
#define	MTI_SR_FE	0x20		/* framing error */
#define	MTI_SR_DO	0x10		/* data overrun */
#define	MTI_SR_PE	0x08		/* parity error */
#define	MTI_SR_DONE	0x02		/* receiver done */
#define	MTI_SR_READY	0x01		/* transmitter ready */

/* flags for modem control */
#define	MTI_ON	(MTI_CR_DTR|MTI_CR_RTS)
#define	MTI_OFF	MTI_CR_RTS

#endif /*!_sundev_mtireg_h*/
