/*  @(#)tmpdir.h 1.1 92/07/30 SMI */

/*
 * directory entry for temporary file system
 * modelled after ufs/fsdir.h
 */

#undef	MAXNAMLEN
#define	MAXNAMLEN	255

struct tdirent {
	u_short		td_reclen;		/* length of this record */
	u_short		td_namelen;		/* string length in td_name */
	struct tmpnode	*td_tmpnode;		/* tnode for this file */
	struct tdirent	*td_next;		/* next directory entry */
	char		td_name[MAXNAMLEN+1];	/* no longer than this */
};

#define	TDIRSIZ(tdp) \
	((sizeof (struct tdirent) - (MAXNAMLEN+1)) + \
	(((tdp)->td_namelen+1 + 3) &~ 3))

#define	TEMPTYDIRSIZE	32			/* fudge cookie */
