/*	@(#)idtab.h 1.1 92/07/30 SMI; from S5R3.1 10.4.1.1 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *
 *    defines for uid/gid translation.
 *
 */

#ifndef _rfs_idtab_h
#define _rfs_idtab_h

#define MAXSNAME	20
#define OTHERID		MAXUID+1
#define NO_ACCESS	MAXUID+2
#define CFREE		0
#define CINUSE		1
#define CINTER		2
#define GLOBAL_CH	'.'	/* name of the "global" table	*/
#define UID_DEV		0	/* minor device number for uid device	*/
#define	GID_DEV		1	/* minor device number for gid device	*/
#define UID_MAP		UID_DEV
#define GID_MAP		GID_DEV

struct idtab	{
	unsigned short	i_rem;
	unsigned short	i_loc;
};
#define i_defval i_rem
#define i_tblsiz i_loc

struct idhead {
	unsigned short	i_default;
	unsigned short	i_size;
	unsigned short	i_cend;
	unsigned short	i_next;
	unsigned long	i_tries;
	unsigned long	i_hits;
};
#define HEADSIZE	(sizeof(struct idhead)/sizeof(struct idtab))
#ifdef KERNEL
extern ushort	glid();
extern char	*rfheap;
extern int	rfsize;

#define	gluid(a,b)	glid(UID_DEV,a,b)
#define glgid(a,b)	glid(GID_DEV,a,b)
#endif

#endif /*!_rfs_idtab_h*/
