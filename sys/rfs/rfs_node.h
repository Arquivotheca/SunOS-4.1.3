/*	@(#)rfs_node.h 1.1 92/07/30 SMI	*/

#ifndef _rfs_rfs_node_h
#define _rfs_rfs_node_h

/*
 * Remote inode info returned with RFS system calls that open a remote
 * inode and return a reference to it as part of the call
 */
struct nodeinfo {
	u_short		rni_uid;	/* Remote uid for node */
	u_short		rni_gid;	/* Remote gid for node */
	off_t		rni_size;	/* Size of file */
	short 		rni_ftype;	/* File type = IFDIR, IFREG, etc. */
	short		rni_nlink;	/* directory entries */
};

/*
 * Remote file information structure for RFS.
 * The rfsnode is the client-side "inode" for RFS remote files. There are
 * two file handles in the node. One is the send descriptor, rfs_sdp, which
 * "points" at the remote inode for the file. The other is the combination
 * of rfs_psdp, which "points" at the parent directory inode for the file and
 * rfs_name the component name of the file in the directory. There can be
 * more than one rfsnode per remote file, since a file can have multiple names.
 */

struct rfsnode {
	struct rfsnode	*rfs_next;	/* active rfsnode list */
	struct vnode	rfs_vnode;	/* vnode for remote file */
	u_short		rfs_flags;	/* flags, see below */
	struct nodeinfo	rfs_ninfo;	/* Remote inode info */
	struct sndd	*rfs_sdp;	/* associated send descriptor */
	struct sndd	*rfs_psdp;	/* send descriptor of parent dir. */
	int 		rfs_vcode;	/* file version # for client caching */
	int		rfs_alloc;	/* Size of node, for alloc, free */
	char 		rfs_name[1];	/* filename associated with node */
};

#define rfs_uid rfs_ninfo.rni_uid
#define rfs_gid rfs_ninfo.rni_gid
#define rfs_size rfs_ninfo.rni_size
#define rfs_ftype rfs_ninfo.rni_ftype
#define rfs_nlink rfs_ninfo.rni_nlink


/*
 * Flags
 */
#define	RFSLOCKED	0x01		/* rfsnode is in use */
#define	RFSWANT		0x02		/* someone wants a wakeup */
#define	RFSATTRVALID	0x04		/* Attributes in the rfsnode are valid */
#define	RFSEOF		0x08		/* EOF encountered on read */
#define RFSOPEN		0x20		/* the rfsnode is currently open */
#define RFSEXCL		0x40		/* Exclusive type node: not cached */
#define RFSSHARE	0x80		/* Shareable type node: cached */

/*
 * Convert between vnode and rfsnode
 */
#define	rfstov(rp)	((struct vnode *) (&(rp)->rfs_vnode))
#define	vtorfs(vp)	((struct rfsnode *)((vp)->v_data))

/*
 * Lock and unlock rfsnodes
 */
#define rfslock(rp) { \
        while ((rp)->rfs_flags & RFSLOCKED) { \
                (rp)->rfs_flags |= RFSWANT; \
                (void) sleep((caddr_t)(rp), PINOD); \
        } \
        (rp)->rfs_flags |= RFSLOCKED; \
}

#define rfsunlock(rp) { \
        (rp)->rfs_flags &= ~RFSLOCKED; \
        if ((rp)->rfs_flags&RFSWANT) { \
                (rp)->rfs_flags &= ~RFSWANT; \
                wakeup((caddr_t)(rp)); \
        } \
}


/*
 * Lock and unlock rfsnode cache --
 * Use these to prevent races which could occur if two people want to
 * manipulate the same node in the cache, and they sleep in the middle
 * of the operation (e.g., because of a remote system call)
 */
#define rfs_cache_lock() { \
	extern char rfs_cache_flag; \
        while (rfs_cache_flag & RFSLOCKED) { \
                rfs_cache_flag |= RFSWANT; \
                (void) sleep((caddr_t) &rfs_cache_flag, PINOD); \
        } \
        rfs_cache_flag |= RFSLOCKED; \
}

#define rfs_cache_unlock() { \
	extern char rfs_cache_flag; \
        rfs_cache_flag &= ~RFSLOCKED; \
        if (rfs_cache_flag & RFSWANT) { \
		rfs_cache_flag &= ~RFSWANT; \
                wakeup((caddr_t) &rfs_cache_flag); \
        } \
}

#endif /*!_rfs_rfs_node_h*/
