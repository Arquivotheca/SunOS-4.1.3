
/*      @(#)global.h 1.1 92/07/30 SMI   */

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */
#ifndef	_GLOBAL_
#define	_GLOBAL_

/*
 * This file contains global definitions and declarations.  It is intended
 * to be included by everyone.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sun/dkio.h>
#include <sun/dklabel.h>
#include "hardware_structs.h"
#include "defect.h"
#include "io.h"

extern	char *strcpy();
extern	char *strcat();
extern	char *strncat();
extern	long strtol();
extern	char *sprintf();

extern	char *zalloc();
extern	char *rezalloc();

/*
 * These declarations are global state variables.
 */
struct	disk_info *disk_list;		/* list of found disks */
struct	ctlr_info *ctlr_list;		/* list of found ctlrs */
char	cur_menu;			/* current menu level */
char	last_menu;			/* last menu level */
char	option_msg;			/* extended message options */
char	option_s;			/* silent mode option */
char	*option_f;			/* input redirect option */
char	*option_l;			/* log file option */
FILE	*log_file;			/* log file pointer */
char	*option_d;			/* forced disk option */
char	*option_t;			/* forced disk type option */
char	*option_p;			/* forced partition table option */
char	*option_x;			/* data file redirection option */
FILE	*data_file;			/* data file pointer */

/*
 * These declarations are used for quick access to information about
 * the disk being worked on.
 */
int	cur_file;			/* file descriptor for current disk */
int	cur_flags;			/* flags for current disk */
struct	disk_info *cur_disk;		/* current disk */
struct	disk_type *cur_dtype;		/* current dtype */
struct	ctlr_info *cur_ctlr;		/* current ctlr */
struct	ctlr_type *cur_ctype;		/* current ctype */
struct	ctlr_ops *cur_ops;		/* current ctlr's ops vector */
struct	partition_info *cur_parts;	/* current disk's partitioning */
struct	defect_list cur_list;		/* current disk's defect list */
char	*cur_buf;			/* current disk's I/O buffer */
char	*pattern_buf;			/* current disk's pattern buffer */
u_short	pcyl;				/* # physical cyls on current disk */
u_short	ncyl;				/* # data cyls on current disk */
u_short	acyl;				/* # alt cyls on current disk */
u_short	nhead;				/* # heads on current disk */
u_short	nsect;				/* # data sects/track on current disk */
u_short	apc;				/* # alternates/cyl on current disk */
u_int	bpt;				/* # bytes/track on current disk */
u_short	psect;				/* physical sectors per track */
u_short phead;				/* physical heads */

/*
 * Flag to indicate which SCSI driver we are talking to:
 * sun4c, with the uscsi interface;  or the old sun4 driver,
 * using the "dk_cmd" interface.
 */
short	sun4_flag;

/*
 * These defines are used to manipulate the physical characteristics of
 * the current disk.
 */
#define	sectors(h)	((h) == nhead - 1 ? nsect - apc : nsect)
#define	spc()		(nhead * nsect - apc)
#define	chs2bn(c,h,s)	((daddr_t)((c) * spc() + (h) * nsect + (s)))
#define	bn2c(bn)	((short)((bn) / spc()))
#define	bn2h(bn)	((short)(((bn) % spc()) / nsect))
#define	bn2s(bn)	((short)(((bn) % spc()) % nsect))
#define	datasects()	(ncyl * spc())
#define	totalsects()	((ncyl + acyl) * spc())
#define	physsects()	(pcyl * spc())

/*
 * These values define flags for the current disk (cur_flags).
 */
#define	DISK_FORMATTED		0x01	/* disk is formatted */
#define	LABEL_DIRTY		0x02	/* label has been scribbled */

/*
 * These flags are for the controller type flags field.
 */
#define	CF_BLABEL	0x0001		/* backup labels in funny place */
#define	CF_DEFECTS	0x0002		/* disk has manuf. defect list */
#define	CF_APC		0x0004		/* ctlr uses alternates per cyl */
#define	CF_SMD_DEFS	0x0008		/* ctlr does smd defect handling */
#define	CF_BSK_WPR	0x0010		/* ctlr does buf_sk and wr_prcmp */
#define	CF_450_TYPES	0x0020		/* ctlr does drive types */
#define	CF_SCSI		0x0040		/* ctlr is for SCSI disks */
#define CF_IPI		0x0080		/* ctlr is for IPI disks */
#define CF_WLIST	0x0100		/* controller handles working list */
#define	CF_EMBEDDED	0x0200		/* embedded scsi controller */

/*
 * Macros to make life easier
 */
#define	EMBEDDED_SCSI	(cur_ctype->ctype_flags & CF_EMBEDDED)

/*
 * These flags are for the disk type flags field.
 */
#define	DT_NEED_SPEFS	0x01		/* specifics fields are uninitialized */

/*
 * These defines are used to access the ctlr specific
 * disk type fields (based on ctlr flags).
 */
#define	dtype_buf_sk	dtype_specifics[0]	/* buffer skew */
#define	dtype_wr_prcmp	dtype_specifics[1]	/* write precomp */
#define	dtype_bps	dtype_specifics[0]	/* bytes/sector */
#define	dtype_dr_type	dtype_specifics[1]	/* drive type */

/*
 * These flags are for the disk info flags field.
 */
#define	DSK_LABEL	0x01		/* disk is currently labelled */

/*
 * These flags are used to control disk command execution.
 */
#define	F_NORMAL	0x00		/* normal operation */
#define	F_SILENT	0x01		/* no error msgs at all */
#define	F_ALLERRS	0x02		/* return any error, not just fatal */

/*
 * These defines are the directional parameter for the rdwr controller op.
 */
#define	DIR_READ	0
#define	DIR_WRITE	1

/*
 * These defines are the mode parameter for the checksum routines.
 */
#define	CK_CHECKSUM		0		/* check checksum */
#define	CK_MAKESUM		1		/* generate checksum */

#endif	!_GLOBAL_

