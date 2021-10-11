/*	@(#)tfs.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#ifndef _TFS_H
#define _TFS_H

#include <nse_impl/tfs.h>
#include <nse_impl/tfs_user.h>

/*
 * Readdir result structure.  Structure reproduced here so that the rd_status
 * field could be placed at the front of the structure (to be compatible with
 * the layout of the result structures of all other NFS procedures.)
 */
struct nfsreaddirres {
	enum nfsstat    rd_status;
	u_long          rd_bufsize;	/* size of client request (not xdr'ed) */
	union {
		struct nfsrdok  rd_rdok_u;
	}               rd_u;
};


/*
 * user-level nfs cookie includes offset field.
 */
struct udirect {
	u_long          d_fileno;	/* file number of entry */
	u_short         d_reclen;	/* length of this record */
	u_short         d_namlen;	/* length of string d_name */
	u_long          d_offset;	/* offset into the directory */
	char            d_name[MAXNAMLEN + 1];	/* name (up to MAXNAMLEN + 1) */
};


/*
 * The ENTRYSIZ macro gives the minimum record length which will hold a
 * directory entry with a name of length 'len'
 */
#define	roundtoint(x)	(((x) + sizeof(int)) & ~(sizeof(int) - 1))
#define ENTRYSIZ(len) \
	((sizeof (struct direct) - (MAXNAMLEN+1)) + roundtoint(len+1))
#define UENTRYSIZ(len) \
	((sizeof (struct udirect) - (MAXNAMLEN+1)) + roundtoint(len+1))


#define	HASH_TABLE_SIZE	1024			/* Size of the various
						 * internal hash tables */

/*
 * Operations for the update_directory() procedure, which updates all vnodes
 * with links to a directory pnode when a change occurs in that directory.
 */
#define DIR_ADD_FILE	1		/* File created in the directory */
#define DIR_REMOVE_FILE	2		/* File removed (& maybe whited out) */
#define DIR_CLEAR_FILE	3		/* File cleared (maybe un-whited out)*/
#define DIR_RENAME_FILE	4		/* File renamed */
#define DIR_UTIME_FILE 5		/* File mtime changed */
#define DIR_FLUSH	6		/* Directory flushed */
#define DIR_FORWARD	7		/* Directory contents forwarded */

#define NSE_TFS_VERSION_FORMAT "VERSION %d\n"

#define NSE_TFS_SWAP_FILE	".tfs_swap"

#endif _TFS_H
