/*	@(#)xdvar.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _sundev_xdvar_h
#define	_sundev_xdvar_h

/*
 * Structure definitions for Xylogics 7053 disk driver.
 */
#include <sundev/xdcreg.h>
#include <sundev/xdreg.h>
#include <sundev/xycom.h>

/*
 * Structure needed to execute a command.  Contains controller & iopb ptrs
 * and some error recovery information.  The link is used to identify
 * chained iopbs.
 */
struct xdcmdblock {
	struct	xdctlr *c;		/* ptr to controller */
	struct	xdunit *un;		/* ptr to unit */
	struct	xdiopb *iopb;		/* ptr to IOPB */
	struct	xdcmdblock *next;	/* next iopb in queue */
	struct	buf *breq;		/* */
	caddr_t	mbaddr;			/* */
	u_char	retries;		/* retry count */
	u_char	restores;		/* restore count */
	u_char	resets;			/* reset count */
	u_char  slave;			/* current drive no. */
	u_short	cmd;			/* current command */
	int	flags;			/* state information */
	caddr_t	baddr;			/* physical buffer address */
	daddr_t	blkno;			/* current block */
	daddr_t	altblkno;		/* alternate block (forwarding) */
	u_short	nsect;			/* sector count active */
	short	device;			/* current minor device */
	u_char	failed;			/* command failure */
#ifdef	DKIOCWCHK
	u_char	forwarded:	1,	/* did this xfer have a mapped block */
		nverifies:	7;	/* number of verify actions */
	daddr_t	vblk;			/* (for verify of forwarded blocks) */
#endif
};

/*
 * Data per unit
 */
struct xdunit {
	struct	xdctlr *c;		/* controller */
	struct	dk_map *un_map;		/* logical partitions */
	struct	dk_geom *un_g;		/* disk geometry */
	struct	buf *un_rtab;		/* for physio */
	int	un_ltick;		/* last time active */
	struct	mb_device *un_md;	/* generic unit */
	struct	mb_ctlr *un_mc;		/* generic controller */
	struct	dkbad *un_bad;		/* bad sector info */
	int	un_errsect;		/* sector in error */
	u_char	un_errno;		/* error number */
	u_char	un_errsevere;		/* error severity */
	u_short	un_errcmd;		/* command in error */
	u_char	un_flags;		/* state information */
#ifdef	DKIOCWCHK
	u_char	un_wchkmap;		/* partition map for write check ops */
#endif	DKIOCWCHK
};

/*
 * Data per controller
 */
struct xdctlr {
	struct	xdunit *c_units[XDUNPERC];	/* units on controller */
	struct	xddevice *c_io;			/* ptr to I/O space data */
	struct	xdcmdblock *actvcmdq; 		/* current command queue */
	struct	xdiopb *rdyiopbq;		/* ready iopbs */
	struct	xdiopb *lrdy;			/* last ready iopb */
	int	c_wantint;			/* controller is busy */
	int	c_intpri;			/* interrupt priority */
	int	c_intvec;			/* interrupt vector */
	u_char	c_flags;			/* state information */
};

/*
 * after RIO is found to be set, this is the number of
 * seconds to wait for the interrupt handler to get invoked
 * before timing out and invoking xdintr via the
 * watch-dog handler.  This value was originally set to
 * 1, but this resulted in recovery messages being printed
 * out constantly under conditions of heavy I/O.
 */
#define	MAX_RIO_TICKS 10


struct xd_w_info {
	u_int curr_timeout;	/* remaining ticks in interval */
	u_int next_timeout;	/* ticks for next interval */
	u_int got_interrupt;    /* got an interrupt in interval */
	u_int got_rio;		/* number of seconds XD_RIO */
				/* has been set, but not serviced */
};

/*
 * The tuning parameter for each cpu architecture is held in this structure.
 * Currently the structure has to be ifdef'd for sun3 & sun4s
 * A cpu type of zero should be the last entry in the list.  This is the
 * default value and will be used if there weren't any matches
 *
 * Note: a delay value of 0 is really 2 msec. Otherwise the delay is in
 *	 256usecs increments.
 */

struct xd_ctlrpar2 {
	int   cpu;	/* the cpu type that will match this entry */
	short delay;	/* the delay sent to the controller */
};

#define	XD_MAXDELAYS	20  /* max number of entries in xd_ctlrpar2 struct */
#endif	/*!_sundev_xdvar_h*/
