/*	@(#)tnode.h 1.1 92/07/30 SMI	*/

#ifndef __TNODE_HEADER__
#define	__TNODE_HEADER__

/*
 * Private information per vnode for TFS vnodes.
 * t_realvp is held until the TFS vnode is released (or until the real
 * vnode changes.)
 */
struct tnode {
	struct tnode	*t_next;	/* link for hash chain */
	struct vnode	t_vnode;
	fhandle_t	t_fh;		/* tfsd's file handle */
	u_long		t_nodeid;	/* nodeid of file */
	u_int		t_writeable : 1; /* is the file writeable? */
	u_int		t_eof : 1;	/* eof reached on directory? */
	u_int		t_locked : 1;	/* is locked? */
	u_int		t_wanted : 1;	/* someone wants a wakeup */
	long		t_size;		/* size of directory */
	struct tnode_unl *t_unlinkp;	/* data for unlinked file */
	struct timeval	t_ctime;	/* ctime of dir or readonly file */
	struct nfssattr	t_sattrs;	/* attrs to be changed */
	struct vnode	*t_realvp;	/* vnode of real file */
};

#define	TTOV(tp)	(&((tp)->t_vnode))
#define	VTOT(vp)	((struct tnode *)((vp)->v_data))
#define	REALVP(vp)	(VTOT(vp)->t_realvp)

/*
 * Data to handle unlinking open files.  (If the real vnode of a TFS vnode
 * is an NFS vnode, and the tfsd is not on the local machine, then a
 * TFS_REMOVE call to the tfsd will cause the file to be removed on the
 * remote machine, and our local NFS vnode will become stale.  So
 * tfs_remove has to behave like nfs_remove and rename the file to a tmp
 * name.)
 */
struct tnode_unl {
	struct ucred	*tu_cred;	/* unlinked credentials */
	char		*tu_name;	/* unlinked file name */
	struct vnode	*tu_dvp;	/* parent dir of unlinked file */
};

/*
 * Lock and unlock tnodes.  We need to put locks around the routines which
 * translate TFS vnodes to real vnodes, because a process doing a TFS
 * translation can sleep in either the rfscall to the tfsd or in the
 * lookupname of the real vnode.
 */
#define	TLOCK(tp) { \
	while ((tp)->t_locked) { \
		(tp)->t_wanted = TRUE; \
		tnode_sleeps++; \
		(void) sleep((caddr_t)(tp), PINOD); \
	} \
	(tp)->t_locked = TRUE; \
}

#define	TUNLOCK(tp) { \
	(tp)->t_locked = FALSE; \
	if ((tp)->t_wanted) { \
		(tp)->t_wanted = FALSE; \
		wakeup((caddr_t)(tp)); \
	} \
}

#endif !__TNODE_HEADER__
