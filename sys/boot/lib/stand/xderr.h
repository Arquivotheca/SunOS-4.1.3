/*      @(#)xderr.h 1.1 92/07/30 SMI   */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Declarations of the error structures for the Xylogics 7053 driver.
 * They are separated to allow user programs access to the information.
 */

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
};

/*
 * The classes of errors.
 */
#define XD_ERCOR	0x00		/* corrected error */
#define XD_ERTRY	0x01		/* retryable error */
#define XD_ERBSY	0x02		/* drive busy error */
#define XD_ERFLT	0x03		/* drive faulted error */
#define XD_ERHNG	0x04		/* controller hung error */
#define XD_ERFTL	0x05		/* fatal error */

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

#define XDE_LINT	0x50		/* lost interrupt */
#define XDE_FERR	0x51		/* fatal error */

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

#define XDE_FIST	0xe1		/* IRAM self test failure */
#define XDE_FMT3	0xe3		/* MTest 3 Fail (dskcel ram) */
#define XDE_FMT4	0xe4		/* MTest 4 Fail (header shift reg) */
#define XDE_FMT5	0xe5		/* MTest 5 Fail (vmedma reg) */
#define XDE_FMT6	0xe6		/* MTest 6 Fail (regcel chip) */
#define XDE_FMT7	0xe7		/* MTest 7 Fail (buffer parity) */
#define XDE_FMT8	0xe8		/* MTest 8 Fail (disk fifo) */
#define XDE_FCKS	0xf0		/* iopb checksum miscompare */
#define XDE_FIDF	0xf1		/* iopb dma fatal */
#define XDE_FALN	0xf2		/* iopb address alignment error */
#define XDE_FFWR	0xf3		/* firmware error */
#define XDE_FIMT	0xf5		/* illegal maintenance mode test */
#define XDE_FACF	0xf6		/* acfail asserted */

#define XDE_UNKN	0xff	    /* unknown error */

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
#ifndef BOOTBLOCK
	XDE_OK,   XD_ERCOR,     DK_NONMEDIA,    "spurious err",
	XDE_NRDY, XD_ERBSY,     DK_NONMEDIA,    "drive not ready",
	XDE_FECC, XD_ERCOR,     DK_ISMEDIA,     "soft ecc corrected",
	XDE_SRTY, XD_ERCOR,     DK_NONMEDIA,    "seek retry",
	XDE_SFTR, XD_ERCOR,     DK_NONMEDIA,    "soft retry",
	XDE_HECC, XD_ERTRY,     DK_ISMEDIA,     "hard data ecc",
	XDE_HDNF, XD_ERTRY,     DK_ISMEDIA,     "header not found",
	XDE_OPTO, XD_ERTRY,     DK_NONMEDIA,    "operation timeout",
	XDE_DMAT, XD_ERTRY,     DK_NONMEDIA,    "vmedma timeout",
	XDE_DSEQ, XD_ERFLT,     DK_NONMEDIA,    "disk sequencer err",
	XDE_BPER, XD_ERTRY,     DK_NONMEDIA,    "fifo parity err",
	XDE_HDEC, XD_ERTRY,     DK_ISMEDIA,     "header ecc err",
	XDE_RVER, XD_ERTRY,     DK_ISMEDIA,     "read verify err",
	XDE_FDMA, XD_ERTRY,     DK_NONMEDIA,    "fatal vmedma err",
	XDE_VMEB, XD_ERTRY,     DK_NONMEDIA,    "vmebus err",
	XDE_DPBY, XD_ERBSY,     DK_NONMEDIA,    "dual port busy",
	XDE_DFLT, XD_ERFLT,     DK_NONMEDIA,    "drive faulted",
	XDE_CHER, XD_ERFLT,     DK_ISMEDIA,     "cyl header err",
	XDE_HHER, XD_ERFLT,     DK_ISMEDIA,     "head header err",
	XDE_OFCL, XD_ERFLT,     DK_NONMEDIA,    "drive not on cyl",
	XDE_SEEK, XD_ERFLT,     DK_NONMEDIA,    "drive seek err",
	XDE_LINT, XD_ERHNG,     DK_NONMEDIA,    "lost interrupt",

	XDE_FERR, XD_ERFLT,	DK_NONMEDIA,	"fatal err",
	XDE_FIST, XD_ERFLT,	DK_NONMEDIA,	"fatal err: iram self test",
	XDE_FMT3, XD_ERFLT,	DK_NONMEDIA,	"fatal err: test 3",
	XDE_FMT4, XD_ERFLT,	DK_NONMEDIA,	"fatal err: test 4",
	XDE_FMT5, XD_ERFLT,	DK_NONMEDIA,	"fatal err: test 5",
	XDE_FMT6, XD_ERFLT,	DK_NONMEDIA,	"fatal err: test 6",
	XDE_FMT7, XD_ERFLT,	DK_NONMEDIA,	"fatal err: test 7",
	XDE_FMT8, XD_ERFLT,	DK_NONMEDIA,	"fatal err: test 8",
	XDE_FCKS, XD_ERFLT,	DK_NONMEDIA,	"fatal err: iopb checksum",
	XDE_FIDF, XD_ERFLT,	DK_NONMEDIA,	"fatal err: iopb dma",
	XDE_FALN, XD_ERFLT,	DK_NONMEDIA,	"fatal err: iopb addr align",
	XDE_FFWR, XD_ERFLT,	DK_NONMEDIA,	"fatal err: firmware err",
	XDE_FIMT, XD_ERFLT,	DK_NONMEDIA,	"fatal err: ill test",
	XDE_FACF, XD_ERFLT,	DK_NONMEDIA,	"fatal err: acfail asserted",

	XDE_PROT, XD_ERFTL,     DK_NONMEDIA,    "write protect err",
	XDE_ILLC, XD_ERFTL,     DK_NONMEDIA,    "unimp cmd",
	XDE_0CNT, XD_ERFTL,     DK_NONMEDIA,    "zero sec count",
	XDE_SSIZ, XD_ERFTL,     DK_NONMEDIA,    "ill sec size",
	XDE_FIRM, XD_ERFTL,     DK_NONMEDIA,    "firmware failure",
	XDE_CADR, XD_ERFTL,     DK_NONMEDIA,    "cyl addr err",
	XDE_HADR, XD_ERFTL,     DK_NONMEDIA,    "head addr err",
	XDE_SADR, XD_ERFTL,     DK_NONMEDIA,    "sec addr err",
	XDE_IFL1, XD_ERFTL,     DK_NONMEDIA,    "illfld len 1",
	XDE_IFL2, XD_ERFTL,     DK_NONMEDIA,    "illfld len 2",
	XDE_IFL3, XD_ERFTL,     DK_NONMEDIA,    "illfld len 3",
	XDE_IFL4, XD_ERFTL,     DK_NONMEDIA,    "illfld len 4",
	XDE_IFL5, XD_ERFTL,     DK_NONMEDIA,    "illfld len 5",
	XDE_IFL6, XD_ERFTL,     DK_NONMEDIA,    "illfld len 6",
	XDE_IFL7, XD_ERFTL,     DK_NONMEDIA,    "illfld len 7",
	XDE_ISGL, XD_ERFTL,     DK_NONMEDIA,    "illfld/gather len",
	XDE_NSEC, XD_ERFTL,     DK_NONMEDIA,    "not enough sec/track",
	XDE_ALGN, XD_ERFTL,     DK_NONMEDIA,    "iopb alignment err",
	XDE_SECC, XD_ERFTL,     DK_ISMEDIA,     "soft ecc err",
	XDE_IRAM, XD_ERFTL,     DK_NONMEDIA,    "iram checksum err",
	XDE_ECCI, XD_ERFTL,     DK_ISMEDIA,     "ecc err ignored",
	XDE_SFMF, XD_ERCOR,     DK_NONMEDIA,    "set format field 5, 5A",
	XDE_SGAA, XD_ERFTL,     DK_NONMEDIA,    "scatter/gather addr alignment",
	XDE_SGAE, XD_ERFTL,     DK_NONMEDIA,    "scatter/gather with auto ecc",
	XDE_HOLE, XD_ERFTL,     DK_NONMEDIA,    "ill black hole address",
	XDE_UNKN, XD_ERFTL,     DK_NONMEDIA,    "unknown err",
#else BOOTBLOCK
	XDE_OK,   XD_ERCOR,     DK_NONMEDIA,    "err 0",
	XDE_NRDY, XD_ERBSY,     DK_NONMEDIA,    "err 1",
	XDE_FECC, XD_ERCOR,     DK_ISMEDIA,     "err 2",
	XDE_SRTY, XD_ERCOR,     DK_NONMEDIA,    "err 3",
	XDE_SFTR, XD_ERCOR,     DK_NONMEDIA,    "err 4",
	XDE_HECC, XD_ERTRY,     DK_ISMEDIA,     "err 5",
	XDE_HDNF, XD_ERTRY,     DK_ISMEDIA,     "err 6",
	XDE_OPTO, XD_ERTRY,     DK_NONMEDIA,    "err 7",
	XDE_DMAT, XD_ERTRY,     DK_NONMEDIA,    "err 8",
	XDE_DSEQ, XD_ERFLT,     DK_NONMEDIA,    "err 9",
	XDE_BPER, XD_ERTRY,     DK_NONMEDIA,    "err 10",
	XDE_HDEC, XD_ERTRY,     DK_ISMEDIA,     "err 11",
	XDE_RVER, XD_ERTRY,     DK_ISMEDIA,     "err 12",
	XDE_FDMA, XD_ERTRY,     DK_NONMEDIA,    "err 13",
	XDE_VMEB, XD_ERTRY,     DK_NONMEDIA,    "err 14",
	XDE_DPBY, XD_ERBSY,     DK_NONMEDIA,    "err 15",
	XDE_DFLT, XD_ERFLT,     DK_NONMEDIA,    "err 16",
	XDE_CHER, XD_ERFLT,     DK_ISMEDIA,     "err 17",
	XDE_HHER, XD_ERFLT,     DK_ISMEDIA,     "err 18",
	XDE_OFCL, XD_ERFLT,     DK_NONMEDIA,    "err 19",
	XDE_SEEK, XD_ERFLT,     DK_NONMEDIA,    "err 20",
	XDE_LINT, XD_ERHNG,     DK_NONMEDIA,    "err 21",

	XDE_FERR, XD_ERFLT,	DK_NONMEDIA,	"err 22",
	XDE_FIST, XD_ERFLT,	DK_NONMEDIA,	"err 23",
	XDE_FMT3, XD_ERFLT,	DK_NONMEDIA,	"err 24",
	XDE_FMT4, XD_ERFLT,	DK_NONMEDIA,	"err 25",
	XDE_FMT5, XD_ERFLT,	DK_NONMEDIA,	"err 26",
	XDE_FMT6, XD_ERFLT,	DK_NONMEDIA,	"err 27",
	XDE_FMT7, XD_ERFLT,	DK_NONMEDIA,	"err 28",
	XDE_FMT8, XD_ERFLT,	DK_NONMEDIA,	"err 29",
	XDE_FCKS, XD_ERFLT,	DK_NONMEDIA,	"err 30",
	XDE_FIDF, XD_ERFLT,	DK_NONMEDIA,	"err 31",
	XDE_FALN, XD_ERFLT,	DK_NONMEDIA,	"err 32",
	XDE_FFWR, XD_ERFLT,	DK_NONMEDIA,	"err 33",
	XDE_FIMT, XD_ERFLT,	DK_NONMEDIA,	"err 34",
	XDE_FACF, XD_ERFLT,	DK_NONMEDIA,	"err 35",

	XDE_PROT, XD_ERFTL,     DK_NONMEDIA,    "err 36",
	XDE_ILLC, XD_ERFTL,     DK_NONMEDIA,    "err 37",
	XDE_0CNT, XD_ERFTL,     DK_NONMEDIA,    "err 38",
	XDE_SSIZ, XD_ERFTL,     DK_NONMEDIA,    "err 39",
	XDE_FIRM, XD_ERFTL,     DK_NONMEDIA,    "err 40",
	XDE_CADR, XD_ERFTL,     DK_NONMEDIA,    "err 41",
	XDE_HADR, XD_ERFTL,     DK_NONMEDIA,    "err 42",
	XDE_SADR, XD_ERFTL,     DK_NONMEDIA,    "err 43",
	XDE_IFL1, XD_ERFTL,     DK_NONMEDIA,    "err 44",
	XDE_IFL2, XD_ERFTL,     DK_NONMEDIA,    "err 45",
	XDE_IFL3, XD_ERFTL,     DK_NONMEDIA,    "err 46",
	XDE_IFL4, XD_ERFTL,     DK_NONMEDIA,    "err 47",
	XDE_IFL5, XD_ERFTL,     DK_NONMEDIA,    "err 48",
	XDE_IFL6, XD_ERFTL,     DK_NONMEDIA,    "err 49",
	XDE_IFL7, XD_ERFTL,     DK_NONMEDIA,    "err 50",
	XDE_ISGL, XD_ERFTL,     DK_NONMEDIA,    "err 51",
	XDE_NSEC, XD_ERFTL,     DK_NONMEDIA,    "err 52",
	XDE_ALGN, XD_ERFTL,     DK_NONMEDIA,    "err 53",
	XDE_SECC, XD_ERFTL,     DK_ISMEDIA,     "err 54",
	XDE_IRAM, XD_ERFTL,     DK_NONMEDIA,    "err 55",
	XDE_ECCI, XD_ERFTL,     DK_ISMEDIA,     "err 56",
	XDE_SFMF, XD_ERCOR,     DK_NONMEDIA,    "err 57",
	XDE_SGAA, XD_ERFTL,     DK_NONMEDIA,    "err 58",
	XDE_SGAE, XD_ERFTL,     DK_NONMEDIA,    "err 59",
	XDE_HOLE, XD_ERFTL,     DK_NONMEDIA,    "err 60",
	XDE_UNKN, XD_ERFTL,     DK_NONMEDIA,    "err 61",
#endif BOOTBLOCK
};

