/*	@(#)ardef.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Standalone Driver for Archive Intelligent Streaming Tape
 * ardef.h 
 */

/*
 * Interesting tape commands
 */
#define AR_CLOSE	0	/* Close tape: WTM-if-writing, rewind */
#define AR_REWIND	1	/* Rewind (overlapped) */
#define AR_STATUS	2	/* Drive Status */
#define AR_READ		3	/* Read to MB memory */
#define AR_WRITE	4	/* Write to MB memory */
#define AR_WEOF		5	/* Write file mark (EOF) */ 
#define AR_ERASE	6	/* Erase entire tape */
#define AR_SELECT	7	/* Select drive of interest */
#define AR_DESELECT	8	/* Select no interesting drive */
#define AR_TENSE	9	/* Retension tape */
#define AR_SKIPFILE	10	/* Skip one file forward */
#define	AR_CMDOK	11	/* See if ok to do cmd */

/* 
 * States into which the tape drive can get.
 */
enum ARstates {
	FINstate = 0x00, IDLEstate, CMDstate,	/* Finished, Idle, Command  */
	WFMinit,				/* Write File Mark */
	RFMinit,				/* Read to File Mark */
	REWinit,				/* Rewind tape */
	TENSEinit,				/* Retension tape */
	ERASEinit,				/* Erase tape */
	SELinit,				/* Select a drive */
	DESELinit,				/* Deselect all drives */
	RDSTinit,				/* Read status */
	CLOSEinit,				/* Deassert aronline */
	READinit, READcmd, READburst, READfin, READidle,	/* Read */
	WRinit, WRcmd,   WRburst,   WRfin,   WRidle,		/* Write */
	CMDOKinit,				/* OK to issue commands? */
};

/*
 * Software state per tape controller.
 */
struct	ar_softc {
	enum ARstates sc_state;	/* Current state of hard/software */
	enum ARstates sc_oldstate;  /* Previous state of sc_state */
	struct arstatus sc_status; /* Status at last "Read status" cmd */
	int	sc_size;	/* Size of buffer to read/write */
	char 	*sc_bufptr;	/* Pointer to buffer to read/write */
	char	sc_initted;	/* Is controller initialized yet? */
	char	sc_opened;	/* Is this drive open? */
	char	sc_lastiow;	/* last op was write */
	int	sc_count;	/* # times to repeat high-level op */
	u_char	sc_drive;	/* Drive # to select/deselect */
	u_char	sc_histate;	/* Higher level state than sc_state */
	struct ardevice *sc_addr;/* Address of I/O registers */
	u_char	sc_qidle;	/* =0 if buf in progress, =1 if not. */
	char	sc_eoflag;	/* raw eof flag */
	u_char	sc_cmdok;	/* 0 => can only issue read/RFM/write/WFM */
	char	sc_selecteddev;	/* currently selected drive */
/* When adding new fields to ar_softc, also initialize them in arinit(). */
};
