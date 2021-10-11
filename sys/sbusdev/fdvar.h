/*	@(#)fdvar.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef	_sbusdev_fdvar_h
#define	_sbusdev_fdvar_h

/* macros for partition/unit from floppy device number */
#define	PARTITION(x)	(minor(x) & 0x7)
#define	UNIT(x)		((minor(x) & 0x18) >> 3)

/*
 * Structure definitions for the floppy driver.
 */

/******************************************************************************
 * floppy disk command and status block
 * needed to execute a command.  Contains controller & iopb ptrs
 * and some error recovery information.
 */
struct fdcsb {
	struct	fdunit *csb_un;		/* ptr to fdunit struct */
	u_char	csb_unit;		/* floppy unit number */
	u_char	csb_part;		/* floppy partition number */
	u_char	csb_cmds[10];		/* commands to send to chip */
	u_char	csb_ncmds;		/* how many command bytes */
	u_char	csb_rslt[10];		/* results from chip */
	u_char	csb_nrslts;		/* number of results */
	short	csb_opflags;		/* opflags, see below */
/*
 * defines for csb_opflags
 */
#define	CSB_OFIMMEDIATE	0x01		/* grab results immediately */
#define	CSB_OFSEEKOPS	0x02		/* seek/recal type cmd */
#define	CSB_OFXFEROPS	0x04		/* read/write type cmd */
#define	CSB_OFRAWIOCTL	0x10		/* raw ioctl - no recovery */
#define	CSB_OFNORESULTS	0x20		/* no results at all */
#define	CSB_OFTIMEIT	0x40		/* timeout (timer) */

	caddr_t	csb_addr;		/* data buffer address */
	u_int	csb_len;		/* length of data to transfer */
	u_int	csb_status;		/* status returned from hwintr */
#define	CSB_CMDTO 0x01
	caddr_t	csb_raddr;		/* result data buffer address */
	u_int	csb_rlen;		/* result length of data transfered */
	short	csb_maxretry;		/* maximum retries this opertion */
	short	csb_retrys;		/* how may retrys done so far */
	int	csb_cmdstat;		/* if 0 then success */
					/* if !0 then complete failure */
	struct buf *c_current;		/* current bp used */
};


/******************************************************************************
 * Data per unit (ie per drive data) - also has data about the diskette in
 *	the drive.
 */
struct fdunit {
	struct	fdk_char *un_chars;	/* ptr to drive characteristics */
	struct	packed_label *un_label;	/* ptr to label, w/ partition map */
	short	un_flags;		/* state information */
	/* Data per partition, ie. minor device. */
	u_char	un_openmask;		/* set to indicate open */
	u_char	un_exclmask;		/* set to indicate exclusive open */
};

/* unit flags (state info) */
#define	FDUNIT_DRVCHECKED	0x01	/* this is drive present */
#define	FDUNIT_DRVPRESENT	0x02	/* this is drive present */
/* (the presence of a diskette is another matter) */
#define	FDUNIT_CHAROK		0x04	/* characteristics are known */
#define	FDUNIT_LABELOK		0x08	/* label was read from disk */
#define	FDUNIT_UNLABELED	0x10	/* no label using default */
#define	FDUNIT_CHANGED		0x20	/* diskette was changed after open */


/* a place to keep some statistics on what's going on */
struct fdstat {
	/* first operations */
	int rd;		/* count reads */
	int wr;		/* count writes */
	int recal;	/* count recalibrates */
	int form;	/* count format_tracks */
	int other;	/* count other ops */

	/* then errors */
	int reset;	/* count resets */
	int to;		/* count timeouts */
	int run;		/* count overrun/underrun */
	int de;		/* count data errors */
	int bfmt;	/* count bad format errors */
};

/******************************************************************************
 * Data per controller - there is ONE of these for THE controller.
 */
struct fdctlr {
	/* KEEP ARRAY OF UNITS POINTERS 1ST! - initialized by struct assigns! */
	struct	fdunit *c_units[NFD];	/* units on controller */
	struct fdcsb *c_csb;		/* pointer to (current) fdcsb */
				/* if you use other than the one, reset it! */
	struct	diskhd c_head;		/* head of buf queue */
	int	c_flags;		/* state information */
	struct	fdstat fdstats;		/* gather statistics */
	int	c_intr;			/* interrupt vector level */
};

/* ctlr flags */
#define	FDCFLG_BUSY	0x0001	/* operation in progress */
#define	FDCFLG_WANT	0x0002	/* structure and its iopb wanted */
#define	FDCFLG_WAITING	0x0004	/* wake &fdctlr.c_csb when done */
#define	FDCFLG_BUFWAIT	0x0008	/* wake &fdkbuf when done vs. iodone() */
#define	FDCFLG_TIMEDOUT	0x0010	/* the current operation just timed out */


/******************************************************************************
 * define a structure to hold the packed default labels,
 * based on the real dk_label structure - but much shorter than 512 bytes
 */
struct packed_label {
	/* ptr TO string, vs string itself! */
	char    *dkl_asciip;		/* for ascii compatibility */
	unsigned short  dkl_rpm;	/* rotations per minute */
	unsigned short  dkl_pcyl;	/* # physical cylinders */
	unsigned short  dkl_apc;	/* alternates per cylinder */
	unsigned short  dkl_intrlv;	/* interleave factor */
	unsigned short  dkl_ncyl;	/* # of data cylinders */
	unsigned short  dkl_acyl;	/* # of alternate cylinders */
	unsigned short  dkl_nhead;	/* # of heads in this partition */
	unsigned short  dkl_nsect;	/* # of 512 byte sectors per track */
	struct dk_map	dkl_map[NDKMAP];	/* partition map, see dkio.h */
};

#endif	/* !_sbusdev_fdvar_h */
