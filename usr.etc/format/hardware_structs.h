
/*      @(#)hardware_structs.h 1.1 92/07/30 SMI   */

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */
#ifndef	_HARDWARE_STRUCTS_
#define	_HARDWARE_STRUCTS_

/*
 * This file contains definitions of data structures pertaining to disks
 * and controllers.
 */

/*
 * This structure describes a specific disk.  These structures are in a
 * linked list because they are malloc'd as disks are found during the
 * initial search.
 */
struct disk_info {
	short	disk_unit;			/* unit number of disk */
	short	disk_slave;			/* slave number of disk */
	struct	disk_type *disk_type;		/* ptr to physical info */
	struct	partition_info *disk_parts;	/* ptr to partition info */
	struct	ctlr_info *disk_ctlr;		/* ptr to disk's ctlr */
	struct	disk_info *disk_next;		/* ptr to next disk */
	int	disk_flags;			/* misc gotchas */
};

/*
 * This structure describes a type (model) of drive.  It is malloc'd
 * and filled in as the data file is read and when a type 'other' disk
 * is selected.  The link is used to make a list of all drive types
 * supported by a ctlr type.
 */
struct disk_type {
	char	*dtype_asciilabel;		/* drive identifier */
	int	dtype_flags;			/* flags for disk type */
	u_long	dtype_options;			/* flags for following options */
	u_int	dtype_fmt_time;			/* format time */
	u_int	dtype_bpt;			/* # bytes per track */
	u_short	dtype_ncyl;			/* # of data cylinders */
	u_short	dtype_acyl;			/* # of alternate cylinders */
	u_short	dtype_pcyl;			/* # of physical cylinders */
	u_short	dtype_nhead;			/* # of heads */
	u_short	dtype_nsect;			/* # of data sectors/track */
	u_short	dtype_phead;			/* # physical heads */
	u_short	dtype_psect;			/* # physical sect/track */
	u_short	dtype_read_retries;		/* # read retries */
	u_short	dtype_write_retries;		/* # write retries */
	u_short	dtype_rpm;			/* rotations per minute */
	u_short	dtype_cyl_skew;			/* cylinder skew */
	u_short	dtype_trk_skew;			/* track skew */
	u_short	dtype_trks_zone;		/* # tracks per zone */
	u_short	dtype_atrks;			/* # alt. tracks  */
	u_short	dtype_asect;			/* # alt. sectors */
	u_short	dtype_cache;			/* cache control */
	u_short	dtype_threshold;		/* cache prefetch threshold */
	u_short	dtype_prefetch_min;		/* cache min. prefetch */
	u_short	dtype_prefetch_max;		/* cache max. prefetch */
	u_int	dtype_specifics[8];		/* ctlr specific drive info */
	struct	chg_list	*dtype_chglist;	/* mode sense/select */
						/* change list - scsi */
	struct	partition_info *dtype_plist;	/* possible partitions */
	struct	disk_type *dtype_next;		/* ptr to next drive type */
};

/*
 * This structure describes a specific ctlr.  These structures are in
 * a linked list because they are malloc'd as ctlrs are found during
 * the initial search.
 */
struct ctlr_info {
	char	ctlr_cname[DK_DEVLEN];		/* name of ctlr */
	char	ctlr_dname[DK_DEVLEN];		/* name of disks */
	u_short	ctlr_flags;			/* flags for ctlr */
	short	ctlr_num;			/* number of ctlr */
	int	ctlr_addr;			/* address of ctlr */
	u_int	ctlr_space;			/* bus space it occupies */
	int	ctlr_prio;			/* interrupt priority */
	int	ctlr_vec;			/* interrupt vector */
	struct	ctlr_type *ctlr_ctype;		/* ptr to ctlr type info */
	struct	ctlr_info *ctlr_next;		/* ptr to next ctlr */
};

/*
 * This structure describes a type (model) of ctlr.  All supported ctlr
 * types are built into the program statically, they cannot be added by
 * the user.
 */
struct ctlr_type {
	u_short	ctype_ctype;			/* type of ctlr */
	char	*ctype_name;			/* name of ctlr type */
	struct	ctlr_ops *ctype_ops;		/* ptr to ops vector */
	int	ctype_flags;			/* flags for gotchas */
	struct	disk_type *ctype_dlist;		/* list of disk types */
};

/*
 * This structure is the operation vector for a controller type.  It
 * contains pointers to all the functions a controller type can support.
 */
struct ctlr_ops {
	int	(*rdwr)();		/* read/write - mandatory */
	int	(*ck_format)();		/* check format - mandatory */
	int	(*format)();		/* format - mandatory */
	int	(*ex_man)();		/* get manufacturer's list - optional */
	int	(*ex_cur)();		/* get current list - optional */
	int	(*repair)();		/* repair bad sector - optional */
	int     (*create)();            /* create original manufacturers */
					/* defect list. - optional */
	int	(*wr_cur)();		/* write current list - optional */
};

/*
 * This structure describes a specific partition layout.  It is malloc'd
 * when the data file is read and whenever the user creates his own
 * partition layout.  The link is used to make a list of possible
 * partition layouts for each drive type.
 */
struct partition_info {
	char	*pinfo_name;			/* name of layout */
	struct	dk_map pinfo_map[NDKMAP];	/* layout info */
	struct	partition_info *pinfo_next;	/* ptr to next layout */
};


/*
 * This structure describes a change to be made to a particular
 * SCSI mode sense page, before issuing a mode select on that
 * page.  This changes are specified in format.dat, and one
 * such structure is created for each specification, linked
 * into a list, in the order specified.
 */
struct chg_list {
	int		pageno;		/* mode sense page no. */
	int		byteno;		/* byte within page */
	int		mode;		/* see below */
	int		value;		/* desired value */
	struct chg_list	*next;		/* ptr to next */
};

/*
 * Change list modes
 */
#define	CHG_MODE_UNDEFINED	(-1)		/* undefined value */
#define	CHG_MODE_SET		0		/* set bits by or'ing */
#define	CHG_MODE_CLR		1		/* clr bits by and'ing */
#define	CHG_MODE_ABS		2		/* set absolute value */


#endif	!_HARDWARE_STRUCTS_

