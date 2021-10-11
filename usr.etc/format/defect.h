 
/*      @(#)defect.h 1.1 92/07/30 SMI   */

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*
 * This file contains definitions related to the defect list.
 */
#ifndef	_DEFECT_
#define	_DEFECT_


extern	struct defect_list work_list;
extern	struct dkbad badmap;

/*
 * This is the structure of the header of a defect list.  It is always
 * the first sector on a track containing a defect list.
 */
struct defect_header {
	u_int	magicno;
	int	count;
	int	cksum;
	int	save[125];
};

/*
 * This is the structure of a defect.  Defects are stored on the disk
 * as an array of these structures following the defect header.
 */
struct defect_entry {
	short	cyl;
	short	head;
	short	sect;
	short	nbits;
	int	bfi;
};

/*
 * This is the internal representation of a defect list.  We store
 * the header statically, but dynamically allocate space for the
 * actual defects, since their number may vary.  The flags field is
 * used to keep track of whether the list has been modified.
 */
struct defect_list {
	struct	defect_header header;
	struct	defect_entry *list;
	int	flags;
};

/*
 * This defines the number of copies of the defect list kept on the disk.
 * They are stored 1/track, starting at track 0 of the second alternate cyl.
 */
#define	LISTCOUNT	2

/*
 * This defines the size (in sectors) of the defect array given the number
 * of defects in the array.  It must be rounded to a sector boundary since
 * that is the atomic disk size.  We make a zero length list use up a
 * sector because it is convenient to have malloc'd space in every
 * non-null list.
 */
#define	LISTSIZE(x)	((x) ? ((x) * sizeof(struct defect_entry) + \
			SECSIZE - 1) / SECSIZE : 1)

/*
 * These defines are the flags for the defect list.
 */
#define	LIST_DIRTY	0x01	/* List needs to be synced */
#define	LIST_RELOAD	0x02	/* Reload list after formatting (SCSI) */
#define	LIST_PGLIST	0x04	/* Both manufacturer's (P) and grown (G) */
				/* lists - embedded scsi only */

/*
 * Miscellaneous defines.
 */
#define	DEFECT_MAGIC	0x89898989	/* magic no for defect lists */
#define NO_CHECKSUM     0x1             /* magic no for no checksum in */
					/* defect list                 */
#define UNKNOWN		(-1)		/* value used in defect fields */
#define	DEF_PRINTHEADER	" num     cyl     hd     bfi     len     sec\n"

/*
 * This defines the number of copies of the bad block table kept on the
 * disk.  They are stored in the first 5 even sectors on the last track
 * of the disk.
 */
#define	BAD_LISTCNT	5

#endif	!_DEFECT_

