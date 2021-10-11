/*	@(#)tfs.h 1.1 92/07/30 SMI	*/

#ifndef __TFS_HEADER_
#define	__TFS_HEADER_

#include <nfs/nfs.h>

/*
 * Arguments/results for TFS procedures which are callable by the kernel.
 */

struct tfstransargs {
	fhandle_t	tr_fh;
	bool_t		tr_writeable;
};

/*
 * Results from directory operation.  Note that the tfsd passes the
 * pathname of the real file back to the kernel, so that the kernel can
 * lookup that pathname to determine the real vnode.  It would be nicer
 * if the tfsd could pass back a file handle for the file, so that we
 * could avoid a lookup, but that isn't possible, since we can't
 * generate an fhandle for an NFS file.
 */
struct tfsdiropres {
	enum nfsstat	dr_status;	/* result status */
	fhandle_t	dr_fh;		/* fhandle for the virtual file */
	u_long		dr_nodeid;	/* nodeid of the virtual file */
	char		*dr_path;	/* pathname of real file */
	u_int		dr_pathlen;	/* strlen(pathname) */
	bool_t		dr_writeable;	/* is the file writeable? */
	struct nfssattr	dr_sattrs;	/* attrs that need to be changed */
};

bool_t	xdr_tfstransargs();
bool_t	xdr_tfsdiropres();

#define	TFS_SETATTR	1
#define	TFS_LOOKUP	2
#define	TFS_CREATE	3
#define	TFS_REMOVE	4
#define	TFS_RENAME	5
#define	TFS_LINK	6
#define	TFS_SYMLINK	7
#define	TFS_MKDIR	8
#define	TFS_RMDIR	9
#define	TFS_READDIR	10
#define	TFS_STATFS	11
#define	TFS_TRANSLATE	12
#define	TFS_NPROC	13

#define	TFS_PROGRAM	((u_long)100037)
#define	TFS_VERSION	((u_long)2)

#endif !__TFS_HEADER_
