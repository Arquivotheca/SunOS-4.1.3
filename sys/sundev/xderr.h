/*	@(#)xderr.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Declarations of the error structures for the Xylogics 7053 driver.
 * They are separated to allow user programs access to the information.
 */

#ifndef _sundev_xderr_h
#define	_sundev_xderr_h

/*
 * List of actions to be taken for each class of error.
 * NOTE: the levels are symbolicly defined below, so
 * this ordering is required.  NOTE: the controller spec suggests not
 * attempting to recover from fatal errors, but we're paranoid and
 * give it a try anyway.
 */
struct xderract {
	u_char	retry;		/* number of retries */
	u_char	restore;	/* number of drive resets */
	u_char	reset;		/* number of controller resets */
} xderracts[] = {
	0,	0,	0,	/* level 0 = XD_ERCOR */
	2,	0,	0,	/* level 1 = XD_ERTRY */
	5,	0,	0,	/* level 2 = XD_ERBSY */
	0,	2,	0,	/* level 3 = XD_ERFLT */
	0,	0,	1,	/* level 4 = XD_ERHNG */
	1,	1,	0,	/* level 5 = XD_ERFTL */
	0,	0,	0,	/* level 6 = XD_ERPAS */

};

/*
 * The classes of errors.
 */
#define	XD_ERCOR	0x00		/* corrected error */
#define	XD_ERTRY	0x01		/* retryable error */
#define	XD_ERBSY	0x02		/* drive busy error */
#define	XD_ERFLT	0x03		/* drive faulted error */
#define	XD_ERHNG	0x04		/* controller hung error */
#define	XD_ERFTL	0x05		/* fatal error */
#define	XD_ERPAS	0x06		/* pass it on */

/*
 * Error codes
 */
#define	XDE_OK		0x00		/* successful completion */
#define	XDE_SFMF	0x01		/* set format fields 5 and 5A only */

#define	XDE_CADR	0x10		/* illegal cylinder address */
#define	XDE_HADR	0x11		/* illegal head address */
#define	XDE_SADR	0x12		/* illegal sector address */
#define	XDE_0CNT	0x13		/* count zero */
#define	XDE_ILLC	0x14		/* unimplemented command */
#define	XDE_IFL1	0x15		/* illegal field length 1 */
#define	XDE_IFL2	0x16		/* illegal field length 2 */
#define	XDE_IFL3	0x17		/* illegal field length 3 */
#define	XDE_IFL4	0x18		/* illegal field length 4 */
#define	XDE_IFL5	0x19		/* illegal field length 5 */
#define	XDE_IFL6	0x1a		/* illegal field length 6 */
#define	XDE_IFL7	0x1b		/* illegal field length 7 */
#define	XDE_ISGL	0x1c		/* illegal scatter/gather length */
#define	XDE_NSEC	0x1d		/* not enough sectors/track */
#define	XDE_ALGN	0x1e		/* next iopb alignment error */
#define	XDE_SGAA	0x1f		/* scatter/gather addr alignment */
#define	XDE_SGAE	0x20		/* scatter/gather with auto ecc */
#define	XDE_HOLE	0x21		/* illegal black hole address */

#define	XDE_FECC	0x30		/* soft ecc corrected */
#define	XDE_ECCI	0x31		/* ecc ignored */
#define	XDE_SRTY	0x32		/* auto seek retry recovered */
#define	XDE_SFTR	0x33		/* soft retry recovered */

#define	XDE_HECC	0x40		/* hard data ecc */
#define	XDE_HDNF	0x41		/* header not found */
#define	XDE_NRDY	0x42		/* drive not ready */
#define	XDE_OPTO	0x43		/* operation timeout */
#define	XDE_DMAT	0x44		/* vmedma timeout */
#define	XDE_DSEQ	0x45		/* disk sequencer error */
#define	XDE_BPER	0x46		/* fifo parity error */
#define	XDE_DPBY	0x47		/* dual port busy timeout */
#define	XDE_HDEC	0x48		/* header ecc error */
#define	XDE_RVER	0x49		/* read verify */
#define	XDE_FDMA	0x4a		/* fatal vmedma error */
#define	XDE_VMEB	0x4b		/* vmebus error */

#define	XDE_LINT	0x50		/* lost interrupt */
#define	XDE_FERR	0x51		/* fatal error */

#define	XDE_DFLT	0x60		/* drive faulted */
#define	XDE_CHER	0x61		/* header error/cylinder */
#define	XDE_HHER	0x62		/* header error/head */
#define	XDE_OFCL	0x63		/* drive not on-cylinder */
#define	XDE_SEEK	0x64		/* seek error */

#define	XDE_SSIZ	0x70		/* illegal sector size */
#define	XDE_FIRM	0x71		/* firmware failure */

#define	XDE_SECC	0x80		/* soft ecc */
#define	XDE_IRAM	0x81		/* iram checksum failure */

#define	XDE_PROT	0x90		/* write protect error */

#define	XDE_FIST	0xe1		/* IRAM self test failure */
#define	XDE_FMT3	0xe3		/* MTest 3 Fail (dskcel ram) */
#define	XDE_FMT4	0xe4		/* MTest 4 Fail (header shift reg) */
#define	XDE_FMT5	0xe5		/* MTest 5 Fail (vmedma reg) */
#define	XDE_FMT6	0xe6		/* MTest 6 Fail (regcel chip) */
#define	XDE_FMT7	0xe7		/* MTest 7 Fail (buffer parity) */
#define	XDE_FMT8	0xe8		/* MTest 8 Fail (disk fifo) */
#define	XDE_FCKS	0xf0		/* iopb checksum miscompare */
#define	XDE_FIDF	0xf1		/* iopb dma fatal */
#define	XDE_FALN	0xf2		/* iopb address alignment error */
#define	XDE_FFWR	0xf3		/* firmware error */
#define	XDE_FIMT	0xf5		/* illegal maintenance mode test */
#define	XDE_FACF	0xf6		/* acfail asserted */

#define	XDE_UNKN	0xff	    /* unknown error */

/*
 * List of each recognizable error.
 * The list is sorted from most likely to least likely error, using
 * the best guess algorithm.
 * NOTE : the entries for success and unknown error MUST remain first and
 * last respectively on the list due to the algorithm for finding errors.
 */
struct xderror {
	u_char	errno;		/* error number */
	u_char	errlevel;	/* error level (corrected, fatal, etc) */
	u_char	errtype;	/* error type (media vs nonmedia) */
	char	*errmsg;	/* error message */
} xderrors[] = {
XDE_OK,		XD_ERCOR,	DK_NONMEDIA,	"spurious error",
XDE_NRDY,	XD_ERBSY,	DK_NONMEDIA,	"drive not ready",
XDE_FECC,	XD_ERCOR,	DK_ISMEDIA,	 "soft ecc corrected",
XDE_SRTY,	XD_ERCOR,	DK_NONMEDIA,	"seek retry",
XDE_SFTR,	XD_ERCOR,	DK_NONMEDIA,	"soft retry",
XDE_HECC,	XD_ERTRY,	DK_ISMEDIA,	 "hard data ecc",
XDE_HDNF,	XD_ERTRY,	DK_ISMEDIA,	 "header not found",
XDE_OPTO,	XD_ERTRY,	DK_NONMEDIA,	"operation timeout",
XDE_DMAT,	XD_ERTRY,	DK_NONMEDIA,	"vmedma timeout",
XDE_DSEQ,	XD_ERFLT,	DK_NONMEDIA,	"disk sequencer error",
XDE_BPER,	XD_ERTRY,	DK_NONMEDIA,	"fifo parity error",
XDE_HDEC,	XD_ERTRY,	DK_ISMEDIA,	 "header ecc error",
XDE_FDMA,	XD_ERTRY,	DK_NONMEDIA,	"fatal vmedma error",
XDE_VMEB,	XD_ERTRY,	DK_NONMEDIA,	"vmebus error",
XDE_DPBY,	XD_ERBSY,	DK_NONMEDIA,	"dual port busy",
XDE_DFLT,	XD_ERFLT,	DK_NONMEDIA,	"drive faulted",
XDE_CHER,	XD_ERFLT,	DK_ISMEDIA,	 "cylinder header error",
XDE_HHER,	XD_ERFLT,	DK_ISMEDIA,	 "head header error",
XDE_OFCL,	XD_ERFLT,	DK_NONMEDIA,	"drive not on cylinder",
XDE_SEEK,	XD_ERFLT,	DK_NONMEDIA,	"drive seek error",
XDE_LINT,	XD_ERHNG,	DK_NONMEDIA,	"lost interrupt",

XDE_FERR,	XD_ERFLT,	DK_NONMEDIA,	"fatal error",
XDE_FIST,	XD_ERFLT,	DK_NONMEDIA,	"fatal error: iram self test",
XDE_FMT3,	XD_ERFLT,	DK_NONMEDIA,	"fatal error: test 3",
XDE_FMT4,	XD_ERFLT,	DK_NONMEDIA,	"fatal error: test 4",
XDE_FMT5,	XD_ERFLT,	DK_NONMEDIA,	"fatal error: test 5",
XDE_FMT6,	XD_ERFLT,	DK_NONMEDIA,	"fatal error: test 6",
XDE_FMT7,	XD_ERFLT,	DK_NONMEDIA,	"fatal error: test 7",
XDE_FMT8,	XD_ERFLT,	DK_NONMEDIA,	"fatal error: test 8",
XDE_FCKS,	XD_ERFLT,	DK_NONMEDIA,	"fatal error: iopb checksum",
XDE_FIDF,	XD_ERFLT,	DK_NONMEDIA,	"fatal error: iopb dma",
XDE_FALN,	XD_ERFLT,	DK_NONMEDIA,	"fatal error: iopb addr align",
XDE_FFWR,	XD_ERFLT,	DK_NONMEDIA,	"fatal error: firmware error",
XDE_FIMT,	XD_ERFLT,	DK_NONMEDIA,	"fatal error: illegal test",
XDE_FACF,	XD_ERFLT,	DK_NONMEDIA,	"fatal error: acfail asserted",
XDE_PROT,	XD_ERFTL,	DK_NONMEDIA,	"write protect error",
XDE_ILLC,	XD_ERFTL,	DK_NONMEDIA,	"unimplemented command",
XDE_0CNT,	XD_ERFTL,	DK_NONMEDIA,	"zero sector count",
XDE_SSIZ,	XD_ERFTL,	DK_NONMEDIA,	"illegal sector size",
XDE_FIRM,	XD_ERFTL,	DK_NONMEDIA,	"firmware failure",
XDE_CADR,	XD_ERFTL,	DK_NONMEDIA,	"cylinder addr error",
XDE_HADR,	XD_ERFTL,	DK_NONMEDIA,	"head addr error",
XDE_SADR,	XD_ERFTL,	DK_NONMEDIA,	"sector addr error",
XDE_IFL1,	XD_ERFTL,	DK_NONMEDIA,	"illegal field length 1",
XDE_IFL2,	XD_ERFTL,	DK_NONMEDIA,	"illegal field length 2",
XDE_IFL3,	XD_ERFTL,	DK_NONMEDIA,	"illegal field length 3",
XDE_IFL4,	XD_ERFTL,	DK_NONMEDIA,	"illegal field length 4",
XDE_IFL5,	XD_ERFTL,	DK_NONMEDIA,	"illegal field length 5",
XDE_IFL6,	XD_ERFTL,	DK_NONMEDIA,	"illegal field length 6",
XDE_IFL7,	XD_ERFTL,	DK_NONMEDIA,	"illegal field length 7",
XDE_ISGL,	XD_ERFTL,	DK_NONMEDIA,	"illegal scatter/gather length",
XDE_NSEC,	XD_ERFTL,	DK_NONMEDIA,	"not enough sectors/track",
XDE_ALGN,	XD_ERFTL,	DK_NONMEDIA,	"iopb alignment error",
XDE_SECC,	XD_ERFTL,	DK_ISMEDIA,	 "soft ecc error",
XDE_IRAM,	XD_ERFTL,	DK_NONMEDIA,	"iram checksum error",
XDE_ECCI,	XD_ERFTL,	DK_ISMEDIA,	 "ecc error ignored",
XDE_SFMF,	XD_ERCOR,	DK_NONMEDIA,	"set format field 5,	5A",
XDE_SGAA,	XD_ERFTL,	DK_NONMEDIA,	"scatter/gather addr alignment",
XDE_SGAE,	XD_ERFTL,	DK_NONMEDIA,	"scatter/gather with auto ecc",
XDE_HOLE,	XD_ERFTL,	DK_NONMEDIA,	"illegal black hole address",

#ifdef	DKIOCWCHK
/*
 * Strictly speaking, the READ VERIFY error is not a fatal error. However,
 * we're marking it as a fatal error so the driver can turn around and
 * rewrite the failed transfer (DKIOCWCHK) only.
 */

XDE_RVER,	XD_ERPAS,	DK_ISMEDIA,	"read verify error",

#endif	DKIOCWCHK

XDE_UNKN,	XD_ERFTL,	DK_NONMEDIA,	"unknown error",

};

#endif /*!_sundev_xderr_h*/
