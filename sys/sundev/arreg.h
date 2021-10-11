/*	@(#)arreg.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Header file for Archive tape driver.
 *
 * This file contains definitions of the control bits for the
 * Sun Archive interface board, the commands which the Archive
 * will accept, and the format of its status information.
 */

#ifndef _sundev_arreg_h
#define _sundev_arreg_h

/*
 * ardevice defines the hardware interface.
 *
 * The Sun Archive interface occupies 8 bytes of Multibus I/O space.
 * 4 of those bytes are unused (the high-order half of each word).
 * The remainder are registers.  The registers at offset 3 and 5 are
 * read/write and the writeable bits retain their settings in reads.
 * Register 1 is the data port and either writes to the 8 data lines,
 * or reads from them, depending on the tape controller DIRC signal.
 * The register at offset 7 is not really a register.  A write to it
 * will pretend that the last byte read/written in Burst mode was ack-ed
 * by the tape drive, thus regaining CPU access to the control register
 * (which is also interlocked in Burst mode, so you can't turn off Burst
 * mode until after the ack of the final byte).  Reads from offset 7 have
 * no effect.
 */
struct ardevice {
	u_char		:8;
	u_char ardata;			/* Data byte for I/O */
	u_char		:8;
	u_char arrdyie	:1;		/* Enable interrupt on arrdyedge */
	u_char arexcie	:1;		/* Enable interrupt on arexc */
	u_char arcatch	:1;		/* Notice leading edge of arrdy */
	u_char arburst	:1;		/* Use burst mode(auto xfer/ack) */
	u_char arxfer	:1;		/* XFER wire to ctlr if !arburst */
	u_char arreset	:1;		/* RESET wire to controller */
	u_char arreq	:1;		/* REQUEST wire to controller */
	u_char aronline	:1;		/* ONLINE wire to controller */
	u_char		:8;
	u_char armempar	:1;		/* Enables parity check on MB mem */
	u_char		:1;		/* unused */
	u_char arintr	:1;		/* Board is now requesting interrupt */
	u_char arrdyedge:1;		/* A leading edge of Ready was seen
					   since the last time arcatch was
					   set. */
	u_char arack	:1;		/* ACK wire from controller */
	u_char arexc	:1;		/* EXCEPTION wire from controller */
	u_char ardirc	:1;		/* DIRC wire from controller */
	u_char arrdy	:1;		/* READY wire from controller */
	u_char		:8;
	u_char arunwedge;		/* Write to here unwedges Burst */
};


/*
 * The following are commands that can be written to the data port while
 * appropriately toggling REQUEST and READY.
 */
#define ARCMD_LED	((u_char)0x10)	/* Light the LED on the unit */
#define ARCMD_REWIND	((u_char)0x21)	/* Rewind tape */
#define ARCMD_ERASE	((u_char)0x22)	/* Erase entire tape, BOT to EOT */
#define ARCMD_TENSION	((u_char)0x24)	/* Retension tape */
#define ARCMD_WRDATA	((u_char)0x40)	/* Write data */
#define ARCMD_WREOF	((u_char)0x60)	/* Write EOF */
#define ARCMD_RDDATA	((u_char)0x80)	/* Read data */
#define ARCMD_RDEOF	((u_char)0xA0)	/* Read EOF */
#define ARCMD_RDSTAT	((u_char)0xC0)	/* Read status */

/*
 * This struct defines the 6 status bytes returned by the Archive during
 * a Read Status arcommand (AR_rdstat).
 */
struct arstatus {
	unsigned AnyEx0		:1;	/* Logical-OR of the next 7 bits */
	unsigned NoCart		:1;	/* No fully inserted cartridge */
	unsigned NoDrive	:1;	/* Drive not connected to ctrlr */
	unsigned WriteProt	:1;	/* Cart is write protected */
	unsigned EndOfMedium	:1;	/* End of last track reached on wr */
	unsigned HardErr	:1;	/* Hard(unrecoverable) I/O error */
	unsigned GotWrongBlock	:1;	/* Ctrlr sent us wrong bad block */
	unsigned FileMark	:1;	/* We just read a File Mark */

	unsigned AnyEx1		:1;	/* Logical-OR if the next 7 bits */
	unsigned InvalidCmd	:1;	/* We sent a bad command to ctrlr */
	unsigned NoData		:1;	/* No data block, blank tape */
	unsigned GettingFlakey	:1;	/* >= 8 retries on some block */
	unsigned BOT		:1;	/* Cartridge is at BOT */
	unsigned 		:1;
	unsigned 		:1;
	unsigned GotReset	:1;	/* Ctrlr reset since last Status */
	unsigned short	SoftErrs;	/* # soft errors (R or W) since last
					   time we looked */
	unsigned short	TapeStops;	/* # times tape stopped 'cuz CPU
					   didn't keep up (since last time) */
};

/*
 * Drive status bits.
 */
#define ARCH_BITS\
"\20\17NoCart\16NoDrive\15WriteProt\14EndMedium\13HardErr\12WrongBlock\
\11FileMark\7InvCmd\6NoData\5Flaking\4BOT\0034\0022\1GotReset"

/* 
 * Printf %b string for bits in control/status registers (packed into a short,
 * as returned by IOCTL MTIOCGET)
 */

#define ARCH_CTRL_BITS \
"\20\20EnaReady\17EnaExcep\16CatchReady\15Burst\14Xfer\13Reset\12Request\
\11Online\6Interrupt\5EdgeReady\4Ack\3Exception\2Dirc\1Ready"

/* 
 * Control register bits.
 * Note that the long that you pass must be passed as
 *	*(long *)(2+(char *)araddr)
 */

#define ARCH_LONG_CTRL_BITS \
"\20\30EnaReady\27EnaExcep\26CatchReady\25Burst\24Xfer\23Reset\22Request\
\21Online\6Interrupt\5EdgeReady\4Ack\3Exception\2Dirc\1Ready"

/*
 * Block size of the Archive tape unit.  Don't depend on it.
 */
#define AR_BSIZE	512
#define AR_BSHIFT	9	/* log2(AR_BSIZE) */

#endif /*!_sundev_arreg_h*/
