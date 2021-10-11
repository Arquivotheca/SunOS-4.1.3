/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dumprestore.h 1.1 92/07/30 SMI from UCB 5.3 1/28/87
 */

#ifndef _protocols_dumprestore_h
#define _protocols_dumprestore_h

/*
 * TP_BSIZE is the size of file blocks on the dump tapes.
 * Note that TP_BSIZE must be a multiple of DEV_BSIZE.
 *
 * NTREC is the number of TP_BSIZE blocks that are written
 * in each tape record. HIGHDENSITYTREC is the number of
 * TP_BSIZE blocks that are written in each tape record on
 * 6250 BPI or higher density tapes.  CARTRIDGETREC is the
 * number of TP_BSIZE blocks that are written in each tape
 * record on cartridge tapes.
 *
 * TP_NINDIR is the number of indirect pointers in a TS_INODE
 * or TS_ADDR record. Note that it must be a power of two.
 */
#define TP_BSIZE	1024
#define NTREC   	10
#define HIGHDENSITYTREC	32
#define CARTRIDGETREC	63
#define TP_NINDIR	(TP_BSIZE/2)
#define TP_NINOS	(TP_NINDIR / sizeof(long))
#define LBLSIZE		16
#define NAMELEN		64

#define OFS_MAGIC   	(int)60011
#define NFS_MAGIC   	(int)60012
#define CHECKSUM	(int)84446

union u_data {
	char	s_addrs[TP_NINDIR];  	    /* 1 => data; 0 => hole in inode */
	long	s_inos[TP_NINOS];	    /* starting inodes on tape */
};

union u_spcl {
	char dummy[TP_BSIZE];
	struct	s_spcl {
		long	c_type;		    /* record type (see below) */
		time_t	c_date;		    /* date of previous dump */
		time_t	c_ddate;	    /* date of this dump */
		long	c_volume;	    /* dump volume number */
		daddr_t	c_tapea;	    /* logical block of this record */
		ino_t	c_inumber;	    /* number of inode */
		long	c_magic;	    /* magic number (see above) */
		long	c_checksum;	    /* record checksum */
		struct	dinode	c_dinode;   /* ownership and mode of inode */
		long	c_count;	    /* number of valid c_addr entries */
		union	u_data c_data;	    /* see union above */
		char	c_label[LBLSIZE];   /* dump label */
		long	c_level;	    /* level of this dump */
		char	c_filesys[NAMELEN]; /* name of dumpped file system */
		char	c_dev[NAMELEN];	    /* name of dumpped device */
		char	c_host[NAMELEN];    /* name of dumpped host */
		long	c_flags;	    /* additional information */
	} s_spcl;
} u_spcl;
#define spcl u_spcl.s_spcl
#define c_addr c_data.s_addrs
#define c_inos c_data.s_inos
/*
 * special record types
 */
#define TS_TAPE 	1	/* dump tape header */
#define TS_INODE	2	/* beginning of file record */
#define TS_ADDR 	4	/* continuation of file record */
#define TS_BITS 	3	/* map of inodes on tape */
#define TS_CLRI 	6	/* map of inodes deleted since last dump */
#define	TS_EOM		7	/* end of medium marker (for floppy) */
#define TS_END  	5	/* end of volume marker */

/*
 * flag values
 */
#define DR_NEWHEADER	1	/* new format tape header */
#define DR_INODEINFO	2	/* header contains starting inode info */

#define	DUMPOUTFMT	"%-16s %c %s"		/* for printf */
						/* name, incno, ctime(date) */
#define	DUMPINFMT	"%16s %c %[^\n]\n"	/* inverse for scanf */

#endif /*!_protocols_dumprestore_h*/
