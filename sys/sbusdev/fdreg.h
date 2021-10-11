/*	@(#)fdreg.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef	_sbusdev_fdreg_h
#define	_sbusdev_fdreg_h

#define I82077

/*
 * physical address of the floppy controller on campus
 */
#define	OBIO_FDC_ADDR	0xF7200000

#ifdef I82077
/*
 * types of controllers supported by this driver
 */
#define FD_82072	0
#define FD_82077	1
#endif I82077

/*
 * fdc registers
 */
#ifdef I82077
union fdcreg {
	struct {
		u_char	fdc_control;
		u_char	fdc_fifo;
	} fdc_82072_reg;

	struct fdc_82077_reg {
		u_char	fdc_filler1[2];
		u_char	fdc_dor;	/* Digital Output Register */
		u_char	fdc_filler2;
		u_char	fdc_control;	/* DSR on write, MSR on read */
#define			fdc_msr	fdc_control
#define			fdc_dsr	fdc_control
		u_char	fdc_fifo;
		u_char	fdc_filler3;
		u_char	fdc_dir;	/* Digital Input Register */
#define			fdc_ccr	fdc_dir
	} fdc_82077_reg;
};
#else
struct fdcreg {
	u_char	fdc_control;
	u_char	fdc_fifo;
};
#endif I82077

/* these defines are always the same for 82072 controller chip */
/* DSR - data rate select register */
#define	SWR 0x80	/* software reset */
#define	PD  0x40	/* power down */
#define	EPL 0x20	/* enable phase lock loop */
#define	PRECOMPMSK	0x1c	/* precomp mask */
#define	DRSELMSK	0x3	/* data rate select mask */

/* MSR - main status register */
#define	RQM 0x80	/* request for master - chip needs attention */
#define	DIO 0x40	/* data in/out - 1 = remove bytes from fifo */
#define	NDM 0x20	/* non-dma mode - 1 during execution phase */
#define	CB  0x10	/* controller busy - command in progress */

#ifdef I82077
/* DOR - Digital Output register - 82077 only */
#define	EJECT   	0x80	/* eject diskette - replaces bit in auxio reg */
#define	MOTEN   	0x10	/* motor enable bit */
#define	DMAGATE 	0x8	/* must be high to enable interrupts */
#define	RESET   	0x4	/* reset bit - must be 0 to come out of reset state */
#define	DRVSEL  	0x3	/* drive select */
#define ZEROBITS	0x22	/* the rest of the bits should be zero */

/* DIR - Digital Input register - 82077 only */
#define	DISKCHG   0x80	/* diskette change - replaces bit in auxio reg */
#endif I82077

/* DIR - Digital Input register - 82077 only */
#define	DSKCHG  0x80	/* diskette was changed - replaces bit in auxio */

#ifdef I82077
#define MOTON_DELAY	((3 * hz) / 4)		/* motor on delay 0.75 seconds */
#define MOTOFF_DELAY	(6 * hz)		/* motor off delay 6 seconds */
#endif I82077

struct fdcmdinfo {
	char *cmdname;		/* command name */
	u_char ncmdbytes;	/* number of bytes of command */
	u_char nrsltbytes;	/* number of bytes in result */
	u_char cmdtype;		/* characteristics */
} fdcmds[] = {
	"", 0, 0, 0,			/* - */
	"", 0, 0, 0,			/* - */
	"read_track", 9, 7, 1,		/* 2 */
	"specify", 3, 0, 3,		/* 3 */
	"sense_drv_status", 2, 1, 3,	/* 4 */
	"write", 9, 7, 1,		/* 5 */
	"read", 9, 7, 1,		/* 6 */
	"recalibrate", 2, 0, 2,		/* 7 */
	"sense_int_status", 1, 2, 3,	/* 8 */
	"write_del", 9, 7, 1,		/* 9 */
	"read_id", 2, 7, 2,		/* A */
	"motor_on/off", 1, 0, 4,	/* B */
	"read_del", 9, 7, 1,		/* C */
	"format_track", 10, 7, 1,	/* D */
	"dump_reg", 1, 10, 4,		/* E */
	"seek", 3, 0, 2,		/* F */
	"", 0, 0, 0,			/* - */
	"", 0, 0, 0,			/* - */
	"", 0, 0, 0,			/* - */
	"configure", 4, 0, 4,		/* 13 */
/* relative seek */
};

#ifdef NEVER
XXX add another field so we can look up by command value
>>>  but they are ordered now ??
XXX	maybe need a mask field also?
XXX add another field / rearrange this for the hwintr's opmode value
XXX -> YES make these a bit mask
#endif NEVER

/* command types */
#define	undefined 0
#define	data_tranfer 1
#define	control_with_int 2
#define	control_without_int 3
#define	new_control 4

#define	GPLN 0x1b	/* gap length for read/write command */
#define	GPLF 0x54	/* gap length for format command */
#define	FDATA 0xe5	/* fill data fields during format */

/* 82072 commands */
#define	MT		0x80
#define	MFM		0x40
#define	SK		0x20

#define	MOT		0x80

#define	DUMPREG		0x0e
#define	CONFIGURE	0x13

#define	SECSIZ512	0x02
#define	SSSDTL		0xff	/* special sector size */

#define	NCBRW		0x09	/* number cmd bytes for read/write cmds */
#define	NRBRW		0x07	/* number result bytes for read/write cmds */

/* 82072 results */
/* status reg0 */
#define	IC_SR0		0xc0	/* interrupt code */
#define	SE_SR0		0x20	/* seek end */
#define	EC_SR0		0x10	/* equipment check */
#define	NR_SR0		0x08	/* not ready */
#define	H_SR0		0x04	/* head address */
#define	DS_SR0		0x03	/* drive select */

/* status reg1 */
#define	EN_SR1		0x80	/* end of cylinder */
#define	DE_SR1		0x20	/* data error */
#define	OR_SR1		0x10	/* overrun/underrun */
#define	ND_SR1		0x04	/* no data */
#define	NW_SR1		0x02	/* not writable */
#define	MA_SR1		0x01	/* missing address mark */

/* status reg3 */
#define	WP_SR3		0x40	/* write protected */
#define	T0_SR3		0x10	/* track zero */

#endif	/* !_sbusdev_fdreg_h */
