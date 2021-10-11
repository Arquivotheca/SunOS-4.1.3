
/*      @(#)xyerr.h 1.1 92/07/30 SMI   */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Declarations of the error structures for the Xylogics 450/451 driver.
 * They are separated to allow user programs access to the information.
 */

#ifndef _sundev_xyerr_h
#define _sundev_xyerr_h

/*
 * List of actions to be taken for each class of error.
 * NOTE: the levels are symbolicly defined below, so
 * this ordering is required.  NOTE: the controller spec suggests not
 * attempting to recover from fatal errors, but we're paranoid and
 * give it a try anyway.
 */
struct xyerract {
	u_char	retry;		/* number of retries */
	u_char	restore;	/* number of drive resets */
	u_char	reset;		/* number of controller resets */
} xyerracts[] = {
	0,	0,	0,	/* level 0 = XY_ERCOR */
	2,	0,	0,	/* level 1 = XY_ERTRY */
	5,	0,	0,	/* level 2 = XY_ERBSY */
	0,	2,	0,	/* level 3 = XY_ERFLT */
	0,	0,	1,	/* level 4 = XY_ERHNG */
	1,	1,	0,	/* level 5 = XY_ERFTL */
};

/*
 * The classes of errors.
 */
#define XY_ERCOR	0x00		/* corrected error */
#define XY_ERTRY	0x01		/* retryable error */
#define XY_ERBSY	0x02		/* drive busy error */
#define XY_ERFLT	0x03		/* drive faulted error */
#define XY_ERHNG	0x04		/* controller hung error */
#define XY_ERFTL	0x05		/* fatal error */

/*
 * Error codes
 */
#define	XYE_OK		0x00		/* command succeeded */
#define	XYE_IPND	0x01		/* interrupt pending */
#define	XYE_BCON	0x03		/* busy conflict */
#define XYE_OPTO	0x04		/* operation timeout */
#define XYE_HDNF	0x05		/* header not found */
#define	XYE_HECC	0x06		/* hard ecc error */
#define XYE_CADR	0x07		/* cylinder addr error */
#define	XYE_SLIP	0x09		/* sector slip error */
#define	XYE_SADR	0x0a		/* sector addr error */
#define	XYE_2SML	0x0d		/* last sector too small */
#define	XYE_MADR	0x0e		/* memory addr error */
#define	XYE_HDER	0x12		/* cylinder & head header error */
#define	XYE_SRTY	0x13		/* seek retry */
#define	XYE_PROT	0x14		/* write protect error */
#define	XYE_ILLC	0x15		/* unimplemented command */
#define	XYE_NRDY	0x16		/* drive not ready */
#define	XYE_0CNT	0x17		/* zero sector count */
#define	XYE_DFLT	0x18		/* drive fault */
#define	XYE_SSIZ	0x19		/* illegal sector size */
#define	XYE_TSTA	0x1a		/* self test error a */
#define	XYE_TSTB	0x1b		/* self test error b */
#define	XYE_TSTC	0x1c		/* self test error c */
#define	XYE_SECC	0x1e		/* soft ecc error */
#define	XYE_FECC	0x1f		/* fixed ecc error */
#define	XYE_HADR	0x20		/* head addr error */
#define	XYE_DSEQ	0x21		/* disk sequencer error */
#define	XYE_SEEK	0x25		/* drive seek error */
#define	XYE_LINT	0x26		/* lost interrupt */
#define	XYE_ERR		0x2a		/* hard error */
#define	XYE_DERR	0x2b		/* double hard error */
#define	XYE_UNKN	0xff		/* unknown error */

/*
 * List of each recognizable error.
 * The list is sorted from most likely to least likely error, using
 * the best guess algorithm.
 * NOTE : the entries for success and unknown error MUST remain first and
 * last respectively on the list due to the algorithm for finding errors.
 */
struct xyerror {
	u_char	errno;		/* error number */
	u_char	errlevel;	/* error level (corrected, fatal, etc) */
	u_char	errtype;	/* error type (media vs nonmedia) */
	char	*errmsg;	/* error message */
} xyerrors[] = {
	XYE_OK,   XY_ERTRY,	DK_NONMEDIA,	"spurious error",
	XYE_NRDY, XY_ERBSY,	DK_NONMEDIA,	"drive not ready",
	XYE_FECC, XY_ERCOR,	DK_ISMEDIA,	"fixed ecc error",
	XYE_SRTY, XY_ERCOR,	DK_NONMEDIA,	"seek retry",
	XYE_HDNF, XY_ERTRY,	DK_ISMEDIA,	"header not found",
	XYE_HECC, XY_ERTRY,	DK_ISMEDIA,	"hard ecc error",
	XYE_DFLT, XY_ERFLT,	DK_NONMEDIA,	"drive fault",
	XYE_OPTO, XY_ERTRY,	DK_NONMEDIA,	"operation timeout",
	XYE_DSEQ, XY_ERFLT,	DK_NONMEDIA,	"disk sequencer error",
	XYE_MADR, XY_ERFTL,	DK_NONMEDIA,	"memory addr error",
	XYE_HDER, XY_ERFLT,	DK_ISMEDIA,	"cylinder & head header error",
	XYE_LINT, XY_ERHNG,	DK_NONMEDIA,	"lost interrupt",
	XYE_ERR,  XY_ERFLT,	DK_NONMEDIA,	"hard error",
	XYE_DERR, XY_ERFLT,	DK_NONMEDIA,	"double hard error",
	XYE_PROT, XY_ERFTL,	DK_NONMEDIA,	"write protect error",
	XYE_SEEK, XY_ERFLT,	DK_NONMEDIA,	"drive seek error",
	XYE_SLIP, XY_ERFTL,	DK_NONMEDIA,	"sector slip error",
	XYE_2SML, XY_ERFTL,	DK_NONMEDIA,	"last sector too small",
	XYE_SSIZ, XY_ERFTL,	DK_NONMEDIA,	"illegal sector size",
	XYE_IPND, XY_ERFTL,	DK_NONMEDIA,	"interrupt pending",
	XYE_BCON, XY_ERFTL,	DK_NONMEDIA,	"busy conflict",
	XYE_CADR, XY_ERFTL,	DK_NONMEDIA,	"cylinder addr error",
	XYE_SADR, XY_ERFTL,	DK_NONMEDIA,	"sector addr error",
	XYE_ILLC, XY_ERFTL,	DK_NONMEDIA,	"unimplemented command",
	XYE_0CNT, XY_ERFTL,	DK_NONMEDIA,	"zero sector count",
	XYE_TSTA, XY_ERFTL,	DK_NONMEDIA,	"self test error a",
	XYE_TSTB, XY_ERFTL,	DK_NONMEDIA,	"self test error b",
	XYE_TSTC, XY_ERFTL,	DK_NONMEDIA,	"self test error c",
	XYE_SECC, XY_ERFTL,	DK_ISMEDIA,	"soft ecc error",
	XYE_HADR, XY_ERFTL,	DK_NONMEDIA,	"head addr error",
	XYE_UNKN, XY_ERFTL,	DK_NONMEDIA,	"unknown error",
};

#endif /*!_sundev_xyerr_h*/
