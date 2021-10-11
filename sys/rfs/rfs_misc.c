/*	@(#)rfs_misc.c 1.1 92/07/30 SMI 	*/

/*
 * 	Miscellaneous routines needed by RFS
 */

#include <sys/varargs.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/stream.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/pathname.h>
#include <sys/debug.h>
#include <sys/dirent.h>
#include <sys/uio.h>
#include <sys/kmem_alloc.h>
#include <rfs/rfs_misc.h>
#include <rfs/nserve.h>
#include <rfs/cirmgr.h>
#include <rfs/rdebug.h>
#include <rfs/sema.h>
#include <rfs/message.h>
#include <rfs/comm.h>
#include <rfs/rfs_node.h>
#include <rfs/rfs_mnt.h>
#include <rfs/idtab.h>
#include <rfs/rfs_serve.h>
#include <rfs/adv.h>
#include <rfs/recover.h>


extern struct vnodeops rfs_vnodeops;



/*
 * Client-side routines
 */


/*
 * rfsnode lookup stuff.
 * These routines maintain a table of rfsnodes hashed by  rd and
 * vfsp so that the rfsnode can be found if it already exists.
 * NOTE: RTABLESIZE must be a power of 2 for rftablehash to work!
 */

#define	RFTABLESIZE	16
#define	rftablehash(vfp, rdind)	((((u_int) vfp >> 8) + rdind) & (RFTABLESIZE-1))

static struct rfsnode *rftable[RFTABLESIZE];
char rfs_cache_flag;

/*
 * Put a rfsnode in the table
 */
rf_save(rfp, vfsp)
	struct rfsnode *rfp;
	struct vfs *vfsp;
{
	register int hashind = rftablehash(vfsp, rfp->rfs_sdp->sd_sindex);

	rfp->rfs_next = rftable[hashind];
	rftable[hashind] = rfp;
}

/*
 * Remove a rfsnode from the table
 */
rf_unsave(rfp, vfsp)
	struct rfsnode *rfp;
	struct vfs *vfsp;
{
	struct rfsnode *rt;
	struct rfsnode *rtprev = NULL;
	register int hashind = rftablehash(vfsp, rfp->rfs_sdp->sd_sindex);

	rt = rftable[hashind];
	while (rt != NULL) {
		if (rt == rfp) {
			if (rtprev == NULL) {
				rftable[hashind] = rt->rfs_next;
			} else {
				rtprev->rfs_next = rt->rfs_next;
			}
			return;
		}
		rtprev = rt;
		rt = rt->rfs_next;
	}
}

/*
 * Lookup a rfsnode by vfs pointer and remote receive descriptor index.
 * If there is a node in the cache which also has a matching name
 * return it. Otherwise return one without a matching name. The node is
 * returned in *rfpp.  The value returned by the routine is either
 * NO_MATCH, NAME_MATCH (for complete match) or OTHER_MATCH (for vfsp and
 * rdind match but different name).
 */
#define	NO_MATCH  0
#define	NAME_MATCH 1
#define	OTHER_MATCH 2

int
rf_find(vfsp, rdind, name, rfpp)
	struct vfs *vfsp;
	index_t rdind;
	char *name;
	struct rfsnode **rfpp;
{
	register struct rfsnode *rt;

	*rfpp = (struct rfsnode *) NULL;
	rt = rftable[rftablehash(vfsp, rdind)];

	while (rt != NULL) {
		if (rdind == rt->rfs_sdp->sd_sindex &&
		    vfsp == rfstov(rt)->v_vfsp) {
			*rfpp = rt;
			if (!strcmp(rt->rfs_name, name)) {
				rfstov(rt)->v_count++;
				return (NAME_MATCH);
			}
		}
		rt = rt->rfs_next;
	}
	return (*rfpp ? OTHER_MATCH : NO_MATCH);
}


/*
 * make an rfsnode corresponding to the vfs, parent send descriptor,
 * file component name, send descriptor and associated info for file.
 *	1) Look first for a node with matching remote reference in cache
 *	2) If found and name matches as well return the node.
 *	3) Otherwise, create a new node.
 *	4) If found matching node with different name in the cache,
 *	   copy state into new node.
 *	5) Cache the new node.
 */
int
get_rfsnode(vfsp, psdp, name, sdp, nip, vpp)
	struct vfs *vfsp;
	sndd_t psdp;
	char *name;
	sndd_t sdp;
	struct nodeinfo *nip;
	struct vnode **vpp;
{
	struct rfsnode *crfp, *rfp;
	register struct vnode *vp, *cvp;
	int nodesize;
	int match;
	struct rfs_stat rsb;
	int error = 0;
	extern char *strcpy();

	DUPRINT5(DB_RFSNODE, "get_rfsnode: vfsp %x, psdp %x, name %s, sdp %x\n",
		vfsp, psdp, name, sdp);

	ASSERT(name && psdp && sdp && nip);

	rfs_cache_lock();
	match = rf_find(vfsp, sdp->sd_sindex, name, &crfp);

	/* Found cached node with matching name: return it */
	if (match == NAME_MATCH) {
		*vpp = rfstov(crfp);
		crfp->rfs_sdp->sd_stat &= ~SDCACHE_MASK;
		crfp->rfs_sdp->sd_stat |= sdp->sd_stat & SDCACHE_MASK;
		crfp->rfs_sdp->sd_fhandle = sdp->sd_fhandle;
		if (error = del_sndd(sdp))
			rfstov(crfp)->v_count--;
		goto out;
	}
	/* Otherwise: make new node */
	nodesize = sizeof (*rfp) + (strlen(name) + 1);
	rfp = (struct rfsnode *)new_kmem_alloc((u_int)nodesize, KMEM_SLEEP);
	bzero((caddr_t)rfp, (u_int) nodesize);
	rfp->rfs_alloc = nodesize;
	rfp->rfs_psdp = psdp;
	rfp->rfs_psdp->sd_refcnt++;    /* New vnode refers to psd */
	rfp->rfs_sdp = sdp;
	bcopy((caddr_t)nip, (caddr_t)&rfp->rfs_ninfo,
			(u_int) sizeof (struct nodeinfo));
	(void) strcpy(rfp->rfs_name, name);
	vp = rfstov(rfp);
	vp->v_count = 1;
	vp->v_op = &rfs_vnodeops;
	vp->v_data = (caddr_t)rfp;
	vp->v_vfsp = vfsp;
	vp->v_type = RFSTOVT(nip->rni_ftype);
	vp->v_flag |= VNOMAP;	/* Hack, for now, to let RFS run 413 files */

	/*
	 * Cache has matching node with other name: copy state from that node
	 * Later (?) just hook up to shared state pointer
	 */
	if (match == OTHER_MATCH) {
		cvp = rfstov(crfp);
		ASSERT(vp->v_type == cvp->v_type);
		vp->v_flag = cvp->v_flag;
		/*
		 * XXX Copying the following 3 fields doesn't really
		 * make them work, but what the hell
		 */
		vp->v_shlockc = cvp->v_shlockc;
		vp->v_exlockc = cvp->v_exlockc;
		vp->v_vfsmountedhere = cvp->v_vfsmountedhere;
		vp->v_stream = cvp->v_stream;
		vp->v_rdev = cvp->v_rdev;
	/* Newly referenced device file: do stat to get device # for vnode */
	} else if (vp->v_type==VBLK || vp->v_type==VCHR) {
		if (error = du_fstat(sdp, u.u_cred, &rsb)) {
			(void) del_sndd(rfp->rfs_psdp);
			(void) del_sndd(rfp->rfs_sdp);
			kmem_free((caddr_t)rfp, (u_int) rfp->rfs_alloc);
			goto out;
		}
		vp->v_rdev = rsb.st_rdev;
	}

	rf_save(rfp, vfsp);		/* Cache the new node */
	((struct rfsmnt *)(vfsp->vfs_data))->rm_refcnt++;
	*vpp = rfstov(rfp);
out:
	DUPRINT5(DB_RFSNODE, "get_rfsnode: error %d, match %d, vp %x, name %s\n",
		error, match, *vpp, error ? "" : vtorfs(*vpp)->rfs_name);
	rfs_cache_unlock();
	if (error)
		*vpp = (struct vnode *) NULL;
	return (error);
}


/* Transform an RFS node from one without a remote reference into one with
 * a remote reference. This is essentially a no-op since we are
 * assuming a true lookup op which always returns a remote reference
 * to the file. Thus you don't need the remote reference you acquire
 * later as a side effect of some RFS system calls like DUOPEN.
 * Thus, what this routine does is to simply sanity check the new
 * remote reference ("gift") against the one already in the vnode to
 * make sure they really refer to the same remote file. It then throws
 * away the new reference.
 */
/*ARGSUSED*/
int
transform_rfsnode(vpp, gift, nip)
	struct vnode **vpp;
	sndd_t gift;
	struct nodeinfo *nip;
{
	register sndd_t sdp = vtorfs(*vpp)->rfs_sdp;
	int error = 0;

	if (!sdp || sdp->sd_sindex != gift->sd_sindex)
		ASSERT(sdp && (sdp->sd_sindex == gift->sd_sindex));
	/* Transfer new info returned to old remote reference */
	sdp->sd_fhandle = gift->sd_fhandle;
	error = del_sndd(gift); /* XXX what if it fails with error? */
	return (error);
}

/*
 * Save and restore a uio structure -- so system calls involving data
 * movement can be retried from the start if they fail in the middle due
 * to server overload.
 */
uio_save(uiop, suiop, siov)
register struct uio *uiop, *suiop;
register struct iovec siov[];
{
	bcopy((caddr_t) uiop, (caddr_t) suiop, sizeof (struct uio));
	bcopy((caddr_t) uiop->uio_iov, (caddr_t) siov,
		(u_int) uiop->uio_iovcnt * sizeof (struct iovec));
}

uio_restore(uiop, suiop, siov)
register struct uio *uiop, *suiop;
register struct iovec siov[];
{
	bcopy((caddr_t) suiop, (caddr_t) uiop, sizeof (struct uio));
	bcopy((caddr_t) suiop->uio_iov, (caddr_t) siov,
		(u_int) suiop->uio_iovcnt * sizeof (struct iovec));
}

/* rfs_cache_check(vp, sd, vcode) */
/* Check and invalidate cache. If mandatory lock set or not caching
 * invalidate and turn cache off.
 * Else if vcode > vp->rfs_node->rfs_vcode invalidate cache, but don't
 * turn cache off.
 */

/*
 * Server side routines.
 */

/*
 * Server-side version of the lookupname code. This is essentially identical
 * to the routines in vfs_lookup.c. The big difference is that if you hit
 * a ".." you can end up backing out of the server RFS filesystem and you
 * then have to return the remaining pathname to the client to continue the
 * evaluation.
 */

/*
 * lookup the user file name,
 * Handle allocation and freeing of pathname buffer, return error.
 * On return, fnamep points to the remaining chunk of unevaluated pathname.
 */
rs_lookupname(fnamep, seg, followlink, sp, dirvpp, compvpp)
	char *fnamep;			/* user pathname */
	int seg;			/* addr space that name is in */
	enum symfollow followlink;	/* follow sym links */
	struct serv_proc *sp;		/* server proc doing lookup */
	struct vnode **dirvpp;		/* ret for ptr to parent dir vnode */
	struct vnode **compvpp;		/* ret for ptr to component vnode */
{
	struct pathname lookpn;
	register int error;
	register char *cp;

	error = pn_get(fnamep, seg, &lookpn);
	if (error)
		return (error);
	error = rs_lookuppn(&lookpn, followlink, sp, dirvpp, compvpp);
	cp = fnamep;
	while (lookpn.pn_pathlen--)
		*cp++ = *lookpn.pn_path++;
	*cp = '\0';
	pn_free(&lookpn);
	return (error);
}

/*
 * Starting at current directory, translate pathname pnp to end or until
 * you cross back out of the server mount point by virtue of "..". If
 * the latter happens, return the remaining pathname and an error of EDOTDOT.
 * Otherwise, leave pathname of final component in pnp, return the vnode
 * for the final component in *compvpp, and return the vnode
 * for the parent of the final component in dirvpp.
 *
 * This is the central routine in pathname translation and handles
 * multiple components in pathnames, separating them at /'s.  It also
 * implements mounted file systems and processes symbolic links.
 */
rs_lookuppn(pnp, followlink, sp, dirvpp, compvpp)
	register struct pathname *pnp;		/* pathaname to lookup */
	enum symfollow followlink;		/* (don't) follow sym links */
	struct serv_proc *sp;			/* server proc doing lookup */
	struct vnode **dirvpp;			/* ptr for parent vnode */
	struct vnode **compvpp;			/* ptr for entry vnode */
{
	register struct vnode *vp;		/* current directory vp */
	register struct vnode *cvp;		/* current component vp */
	struct vnode *tvp;			/* non-reg temp ptr */
	register struct vfs *vfsp;		/* ptr to vfs for mount indir */
	char component[MAXNAMLEN+1];		/* buffer for component */
	register int error;
	register int nlink;
	extern  struct vfsops rfs_vfsops;
	int lookup_flags;

	lookup_flags = dirvpp ? LOOKUP_DIR : 0;

	nlink = 0;
	cvp = (struct vnode *)0;

	/*
	 * start at current directory.
	 */
	vp = u.u_cdir;
	VN_HOLD(vp);

begin:
	/*
	 * Each time we begin a new name interpretation (e.g.
	 * when first called and after each symbolic link is
	 * substituted), we allow the search to start at the
	 * root directory if the name starts with a '/', otherwise
	 * continuing from the current directory.
	 */
	component[0] = 0;
	if (pn_peekchar(pnp) == '/') {
		VN_RELE(vp);
		pn_skipslash(pnp);
		if (u.u_rdir)
			vp = u.u_rdir;
		else
			vp = rootdir;
		VN_HOLD(vp);
	}

next:
	/*
	 * Make sure we have a directory.
	 */
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto bad;
	}
	/*
	 * Process the next component of the pathname.
	 */
	error = pn_stripcomponent(pnp, component);
	if (error)
		goto bad;

	/*
	 * Check for degenerate name (e.g. / or "")
	 * which is a way of talking about a directory,
	 * e.g. "/." or ".".
	 */
	if (component[0] == 0) {
		/*
		 * If the caller was interested in the parent then
		 * return an error since we don't have the real parent
		 */
		if (dirvpp != (struct vnode **)0) {
			VN_RELE(vp);
			return (EEXIST);
		}
		(void) pn_set(pnp, ".");
		if (compvpp != (struct vnode **)0) {
			*compvpp = vp;
		} else {
			VN_RELE(vp);
		}
		return (0);
	}

	/*
	 * Handle "..": three special cases.
	 * 1. If at chroot() directory then ignore it so can't get out.
	 * 2. If this is the root of an RFS resource the client has mounted,
	 *    then return to client to evaluate rest of pathname.
	 * 3. If this vnode is the root of a mounted
	 *    file system, then replace it with the
	 *    vnode which was mounted on so we take the
	 *    .. in the other file system.
	 */
	if (strcmp(component, "..") == 0) {
checkforroot:
		if (vp == u.u_rdir) {
			cvp = vp;
			VN_HOLD(cvp);
			goto skip;
		}
		if (VN_CMP(vp, srmount[sp->s_mntindx].sr_rootvnode)) {
			struct pathname dotdotpath;

			pn_alloc(&dotdotpath);
			(void) pn_set(&dotdotpath, "..");
			(void) pn_combine(pnp, &dotdotpath); /* ".." for client */
			pn_free(&dotdotpath);
			error = EDOTDOT;
			goto bad;
		}
		if (vp->v_flag & VROOT) {
			cvp = vp;
			vp = vp->v_vfsp->vfs_vnodecovered;
			VN_HOLD(vp);
			VN_RELE(cvp);
			cvp = (struct vnode *)0;
			goto checkforroot;
		}
	}

	/*
	 * Perform a lookup in the current directory.
	 */
	error = VOP_LOOKUP(vp, component, &tvp, u.u_cred, pnp, lookup_flags);
	cvp = tvp;
	if (error) {
		cvp = (struct vnode *)0;
		/*
		 * On error, if more pathname or if caller was not interested
		 * in the parent directory then hard error.
		 */
		if (pn_pathleft(pnp) || dirvpp == (struct vnode **)0)
			goto bad;
		(void) pn_set(pnp, component);
		*dirvpp = vp;
		if (compvpp != (struct vnode **)0) {
			*compvpp = (struct vnode *)0;
		}
		return (0);
	}
	/*
	 * If we hit a symbolic link and FOLLOW_LINK is set then place the
	 * contents of the link at the front of the remaining pathname.
	 */
	if (cvp->v_type == VLNK && ((followlink == FOLLOW_LINK))) {
		struct pathname linkpath;

		nlink++;
		if (nlink > 20) {
			error = ELOOP;
			goto bad;
		}
		error = getsymlink(cvp, &linkpath);
		if (error)
			goto bad;
		if (pn_pathleft(&linkpath) == 0)
			(void) pn_set(&linkpath, ".");
		error = pn_combine(pnp, &linkpath);	/* linkpath before pn */
		pn_free(&linkpath);
		if (error)
			goto bad;
		VN_RELE(cvp);
		cvp = (struct vnode *)0;
		goto begin;
	}

	/*
	 * If this vnode is mounted on, then we
	 * transparently indirect to the vnode which
	 * is the root of the mounted file system.
	 * Before we do this we must check that an unmount is not
	 * in progress on this vnode. This maintains the fs status
	 * quo while a possibly lengthy unmount is going on.
	 * If the mounted filesystem is an RFS type, return EMULTIHOP,
	 * since RFS does not allow crossing multiple machines.
	 */
mloop:
	while (vfsp = cvp->v_vfsmountedhere) {
		if (vfsp->vfs_op == &rfs_vfsops) {
			error = EMULTIHOP;
			goto bad;
		}
		while (vfsp->vfs_flag & VFS_MLOCK) {
			vfsp->vfs_flag |= VFS_MWAIT;
			(void) sleep((caddr_t)vfsp, PVFS);
			goto mloop;
		}
		error = VFS_ROOT(cvp->v_vfsmountedhere, &tvp);
		if (error)
			goto bad;
		VN_RELE(cvp);
		cvp = tvp;
	}

skip:
	/*
	 * Skip to next component of the pathname.
	 * If no more components, return last directory (if wanted)  and
	 * last component (if wanted).
	 */
	if (pn_pathleft(pnp) == 0) {
		(void) pn_set(pnp, component);
		if (dirvpp != (struct vnode **)0) {
			/*
			 * check that we have the real parent and not
			 * an alias of the last component
			 */
			if (vp == cvp) {
				VN_RELE(vp);
				VN_RELE(cvp);
				return (EINVAL);
			}
			*dirvpp = vp;
		} else {
			VN_RELE(vp);
		}
		if (compvpp != (struct vnode **)0) {
			*compvpp = cvp;
		} else {
			VN_RELE(cvp);
		}
		return (0);
	}
	/*
	 * skip over slashes from end of last component
	 */
	pn_skipslash(pnp);

	/*
	 * Searched through another level of directory:
	 * release previous directory handle and save new (result
	 * of lookup) as current directory.
	 */
	VN_RELE(vp);
	vp = cvp;
	cvp = (struct vnode *)0;
	goto next;

bad:
	/*
	 * Error. Release vnodes and return.
	 */
	if (cvp)
		VN_RELE(cvp);
	VN_RELE(vp);
	return (error);
}

/*
 * rfs server version of vn_open (see sys/vfs_vnode.c). Basically
 * identical, but uses rfs server lookupname and vn_create routines. If the
 * pathname given, pnamep, backs out of a server mount point then this routine
 * returns EDOTDOT with the unevaluated remainder of the pathname in
 * pnamep.
 */
int
rs_vnopen(pnamep, seg, filemode, createmode, sp, vpp)
	char *pnamep;
	int seg;
	register int filemode;
	int createmode;
	struct serv_proc *sp;
	struct vnode **vpp;
{
	struct vnode *vp;		/* ptr to file vnode */
	register int mode;
	register int error;

	mode = 0;
	if (filemode & FREAD)
		mode |= VREAD;
	if (filemode & (FWRITE | FTRUNC))
		mode |= VWRITE;

	if (filemode & FCREAT) {
		struct vattr vattr;
		enum vcexcl excl;

		/*
		 * Wish to create a file.
		 */
		vattr_null(&vattr);
		vattr.va_type = VREG;
		vattr.va_mode = createmode;
		if (filemode & FTRUNC)
			vattr.va_size = 0;
		if (filemode & FEXCL)
			excl = EXCL;
		else
			excl = NONEXCL;
		filemode &= ~(FCREAT | FTRUNC | FEXCL);

		error = rs_vncreate(pnamep, seg, &vattr, excl, mode, sp, &vp);
		if (error)
			return (error);
	} else {
		/*
		 * Wish to open a file.
		 * Just look it up.
		 */
		error =
		    rs_lookupname(pnamep, seg, NO_FOLLOW, sp,
			(struct vnode **)0, &vp);
		if (error)
			return (error);
		/*
		 * cannnot write directories, active texts or
		 * read only filesystems
		 */
		if (filemode & (FWRITE | FTRUNC)) {
			if (vp->v_type == VDIR) {
				error = EISDIR;
				goto out;
			}
			if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
				error = EROFS;
				goto out;
			}
		}
		/*
		 * check permissions
		 */
		error = VOP_ACCESS(vp, mode, u.u_cred);
		if (error)
			goto out;
		/*
		 * Sockets in filesystem name space are not supported (yet?)
		 */
		if (vp->v_type == VSOCK) {
			error = EOPNOTSUPP;
			goto out;
		}
	}
	/*
	 * do opening protocol.
	 */

	error = VOP_OPEN(&vp, filemode, u.u_cred);
	/*
	 * truncate if required
	 */
	if ((error == 0) && (filemode & FTRUNC)) {
		struct vattr vattr;

		filemode &= ~FTRUNC;
		vattr_null(&vattr);
		vattr.va_size = 0;
		error = VOP_SETATTR(vp, &vattr, u.u_cred);
	}
out:
	if (error) {
		VN_RELE(vp);
	} else {
		*vpp = vp;
	}

	return (error);
}

/*
 * rfs server version of vn_create (see sys/vfs_vnode.c). Basically
 * identical, but uses rfs server lookupname routines. If the pathname
 * given, pnamep, backs out of a server mount point then this routine
 * returns EDOTDOT with the unevaluated remainder of the pathname in
 * pnamep.
 */
/*ARGSUSED*/
int
rs_vncreate(pnamep, seg, vap, excl, mode, sp, vpp)
	char *pnamep;
	int seg;
	struct vattr *vap;
	enum vcexcl excl;
	int mode;
	struct serv_proc *sp;
	struct vnode **vpp;
{
	struct vnode *dvp;	/* ptr to parent dir vnode */
	int error = 0;

	/*
	 * Lookup directory.
	 * If new object is a file, call lower level to create it.
	 * Note that it is up to the lower level to enforce exclusive
	 * creation, if the file is already there.
	 * This allows the lower level to do whatever
	 * locking or protocol that is needed to prevent races.
	 * If the new object is directory call lower level to make
	 * the new directory, with "." and "..".
	 */
	dvp = (struct vnode *)0;
	*vpp = (struct vnode *)0;

	/*
	 * lookup will find the parent directory for the vnode.
	 * When it is done the pn hold the name of the entry
	 * in the directory.
	 * If this is a non-exclusive create we also find the node itself.
	 */

	if (error = rs_lookupname(pnamep, UIOSEG_KERNEL, NO_FOLLOW, sp, &dvp,
	    (excl == NONEXCL? vpp: (struct vnode **)0)))
		return (error);

	/* EEXIST is returned for  open("."), so return EISDIR instead */
	if (excl == NONEXCL && error == EEXIST)
		error = EISDIR;
	/*
	 * Make sure filesystem is writeable
	 */
	if (dvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		if (*vpp) {
			VN_RELE(*vpp);
		}
		error = EROFS;
	} else if (excl == NONEXCL && *vpp != (struct vnode *)0) {
		/*
		 * we throw the vnode away to let VOP_CREATE truncate the
		 * file in a non-racy manner.
		 */
		VN_RELE(*vpp);
	}
	if (error == 0) {
		/*
		 * call mkdir if directory or create if other
		 */
		if (vap->va_type == VDIR) {
			error = VOP_MKDIR(dvp, pnamep, vap, vpp, u.u_cred);
		} else {
			error = VOP_CREATE(
			    dvp, pnamep, vap, excl, mode, vpp, u.u_cred);
		}
	}
	VN_RELE(dvp);
	return (error);
}


/* Server-side remote unmount routine. Used by both server to perform a
 * DUSRUMOUNT op and by recovery. Release mount table, vnode and advertise
 * table resources associated with the mount.
 */
srumount(smp, vp)
struct  srmnt *smp;
struct vnode *vp;
{
	struct  advertise *ap, *getadv();

	DUPRINT3 (DB_MNT_ADV, "srumount: entry %x, vnode %x \n", smp, vp);
	/* still busy for client machine   or  recovery is going on */
	if (smp->sr_refcnt != 1 || smp->sr_flags & MINTER)
		return (EBUSY);

	/*
	 * Get advertise table entry for resource. Decrement count and free
	 * entry and vnode if no references remain and resource no longer
	 * advertised.
	 */
	ap = getadv(vp);
	ASSERT(ap != (struct advertise *) NULL);
	if ((--(ap->a_count) == 0) && (ap->a_flags & A_MINTER)) {
		ap->a_flags = A_FREE;
		VN_RELE(vp);
		ap->a_queue = (rcvd_t) NULL;
	}
	/* Now free the server-side mount table entry */
	smp->sr_flags = MFREE;
	smp->sr_rootvnode = (struct vnode *) NULL;
	return (0);
}

/*
 * Set this server processes's working directory to the value requested
 * by the client, so that service will be done with respect to the
 * correct directory. rdp is receive descriptor for the current client.
 * rrdir is the client-supplied index for the remote root directory, if
 * any.
 */
set_dir(rdp, rrdir)
rcvd_t rdp;
long rrdir;
{
	extern int nrcvd;

	u.u_cdir = rdp->rd_vnode;
	ASSERT(0 <= rrdir && rrdir < nrcvd);
	if (rrdir)
		u.u_rdir = rcvd[rrdir].rd_vnode;
	else
		u.u_rdir = (struct vnode *) NULL;
}

/*
 *	Find the stream (queue) to client with sysid.
 */

queue_t *
sysid_to_queue (sysid)
sysid_t sysid;
{
	register struct gdp *gdpp;

	DUPRINT2 (DB_MNT_ADV, "convert sysid %x to queue \n", sysid);
	for (gdpp = gdp; gdpp < &gdp[maxgdp]; gdpp++) {
		if (gdpp->sysid == sysid) {
			DUPRINT2 (DB_MNT_ADV, " queue is %x \n", gdpp->queue);
			return (gdpp->queue);
		}
	}

	return (NULL);
}

/*
 *	nameptr returns a pointer to the last element of
 *	a domain name.  E.g., if name == a.b.c, nameptr
 *	returns a ptr to c.  If name == a, it would return
 *	a ptr to a.
 */
char	*
nameptr(name)
register char	*name;
{
	register char	*ptr=name;

	ASSERT(name != NULL);

	while (*name)
		if (*name++ == SEPARATOR)
			ptr = name;

	return (ptr);
}

/*
 * Get the receive descriptor corresponding to a vnode by searching the
 * rcvd table for an entry with matching vnode. Returns a pointer to
 * the receive descriptor or NULL if not found. This routine replaces the
 * i_rcvd pointer in the inode. (This should be hashed.)
 */
rcvd_t
get_rcvd(vp)
register struct vnode *vp;
{

	register struct rcvd *rdp;

	for (rdp = rcvd; rdp < &rcvd[nrcvd]; rdp++) {
		if ((rdp->rd_stat & RDUSED) && (rdp->rd_vnode == vp))
			return (rdp);
	}
	return ((rcvd_t) NULL);
}


/*  Find an empty entry in the smount table, make sure that this
 *  machine doesn't already have this directory mounted. Returns 0
 *  and pointer to table entry in rsmp, or non-zero if error.
 */
int
alocsrm(vp, sysid, rsmp)
register struct vnode *vp;
register sysid_t sysid;
struct srmnt **rsmp;
{
	struct srmnt *smp, *sfree;

	sfree = (struct srmnt *) NULL;
	for (smp = srmount; smp < &srmount [nsrmount]; smp++)
	{
		if (sfree == (struct srmnt *) NULL && smp->sr_flags == MFREE)  {
			sfree = smp;
			continue;
		}
		if ((smp->sr_flags & MINUSE) && (smp->sr_rootvnode == vp) &&
			(smp->sr_sysid == sysid))
			return (EBUSY);
	}
	if (!(smp = sfree))  {
		DUPRINT1(DB_MNT_ADV, "srmount table overflow\n");
		return (ENOSPC);
	}
	*rsmp = smp;
	return (0);
}

/*
 * 	Make a gift to give to a client.
 */
int
make_gift(vp, qsize, sp, ret_gift)
register struct vnode *vp;
register char qsize;
struct serv_proc *sp;
rcvd_t *ret_gift;		/* The gift (receive descriptor) */
{
	extern rcvd_t cr_rcvd();
	register rcvd_t gift;
	extern struct rd_user *cr_rduser();

	/* if sd went bad, forget it */
	if (sp->s_sdp->sd_stat & SDLINKDOWN)
		return (ENOLINK);

	gift = get_rcvd(vp);
	if (gift == (rcvd_t) NULL)  {
		if ((gift = cr_rcvd (qsize, GENERAL)) == (rcvd_t) NULL)
			return (ENOMEM);
		gift->rd_vnode = vp;
	} else
		gift->rd_refcnt++;

	/* keep track of who we gave it to */
	if (cr_rduser(gift, sp) == (struct rd_user *) NULL) {
		del_rcvd (gift, (struct serv_proc *) NULL);
		return (ENOSPC);
	}
	*ret_gift = gift;
	return (0);
}

/*
 * Set a signal (SIGTERM) to the current server process, on behalf of the client
 * whose request is currently being serviced. This will interrupt the request
 * when it is serviced and produce an EINTR response to the client.
 */
void
setrsig(sp, bp)
struct serv_proc *sp;
mblk_t *bp;
{
	if (((struct message *)bp->b_rptr)->m_stat & SIGNAL)
		sp->s_proc->p_sig |= sigmask(SIGTERM);
}


/*
 * Check for and clear any client-generated remote signals (SIGTERM) that occurred
 * during servicing of the current request.
 */
void
chkrsig(sp, error)
struct serv_proc *sp;
int *error;
{
	register struct proc *p = sp->s_proc;

	if ((p->p_sig & sigmask(SIGUSR1)) || (p->p_cursig == SIGUSR1))
		if (!(p->p_sig & sigmask(SIGTERM)) && (p->p_cursig != SIGTERM))
			if (*error == EINTR)
				*error = ENOMEM;
	p->p_sig &= ~(sigmask(SIGTERM));
	if (p->p_cursig == SIGTERM)
		p->p_cursig = 0;
}

/*
 * Construct a server response message for a remote system call request
 * and fill in some common fields.
 *   sp -- server process structure for process servicing the request.
 *   in_bp -- pointer to request message.  May be NULL (but not if
 *	 error == EDOTDOT). This may be freed in this routine in which
 *	 case it will have a value of NULL on return.
 *   error -- Code for error, if any, which occurred during request
 *	processing.
 *   vp -- vnode on which request is being performed. May be 0.
 *   va -- attributes of vnode on which request is performed. May be 0.
 *   out_bp -- returns pointer to response buffer.
 *   outsize -- returns the size of the message.
 */
void
make_response(sp, in_bp, error, vp, vap, out_bp, outsize)
register struct serv_proc *sp;
mblk_t **in_bp;
int error;
struct vnode *vp;
register struct vattr *vap;
mblk_t **out_bp;
int *outsize;
{
	register struct response *msg_out;
	struct request *msg_in;
	extern mblk_t *salocbuf();
	extern char *strcpy();

	/* Allocate response buffer */
	*out_bp = salocbuf(sizeof (struct response), BPRI_MED);
	msg_in = (struct request *) (in_bp ? PTOMSG((*in_bp)->b_rptr) : NULL);

	/* Fill in some common fields */
	msg_out = (struct response *)PTOMSG((*out_bp)->b_rptr);
	msg_out->rp_type = RESP_MSG;
	msg_out->rp_bp = (long) (*out_bp);
	msg_out->rp_subyte = 1;		/* set to indicate no data */
	msg_out->rp_rval = 0;
	msg_out->rp_sig = SIGTORFS(GET_SIGS(sp->s_proc));
	msg_out->rp_sysid = sp->s_sysid;

	/* Return new vnode attributes to client, if any */
	if (!error) {
		if (vp)
			msg_out->rp_ftype = VTTORFS(vp->v_type);
		if (vap) {
			msg_out->rp_size = vap->va_size;
			msg_out->rp_nlink = vap->va_nlink;
			msg_out->rp_uid = vap->va_uid;
			msg_out->rp_gid = vap->va_gid;
		}
	}

	if (error != EDOTDOT) {
		msg_out->rp_opcode = sp->s_op;
		msg_out->rp_mntindx = sp->s_mntindx;
		msg_out->rp_errno = ERRTORFS(error);
		*outsize = sizeof (struct response) - DATASIZE;

	/* Backed out of server mnt point, return remaining path to client */
	} else {
		msg_out->rp_opcode = DUDOTDOT;
		msg_out->rp_mntindx = srmount[sp->s_mntindx].sr_mntindx;
		msg_out->rp_errno = 0;
		ASSERT(msg_in);
		(void) strcpy(msg_out->rp_data, msg_in->rq_data);
		*outsize = sizeof (struct response) - DATASIZE +
				strlen(msg_in->rq_data) + 1;
	}
}


/*
 * This routine is called to search a receive descriptor queue for pid
 * and sysid.  Note that the sysid passed must be that of the client,
 * not the server. The message which matches will be removed from the
 * queue.
 */
#define	rsmsg	((struct request *)PTOMSG(current->b_rptr))
mblk_t *
chkrdq(que, pid, sysid)
rcvd_t que;
long pid, sysid;
{
	mblk_t *current, *deque();
	register int i;

	for (i = 0; i < que->rd_qcnt; i++) {
		current = deque(&que->rd_rcvdq);
		if ((rsmsg->rq_pid == pid) && (rsmsg->rq_sysid == sysid)) {
			DUPRINT3(DB_SIGNAL, "chkrdq: que %x pid %x\n", que, pid);
			que->rd_qcnt--;
			if (rcvdemp(que))
				rm_msgs_list(que);
			return (current);
		}
		enque(&que->rd_rcvdq, current);
	}
	return (NULL);	/* signal msg not on queue */
}

/*
 * Server locking routines.
 */

static caddr_t rsl_mem;

/*
 * Attach a new lock record to this file for the designated client, unless
 * there already is one. mntindx is the index of the server mount table for
 * this client and mount point.
 */
void
rsl_alloc(rd, epid, mntindx, cred)
rcvd_t rd;
short epid;
long mntindx;
struct ucred *cred;
{
	register struct rs_lock *lkp;
	register struct rs_lock *nlkp = (struct rs_lock *) NULL;
	register struct vnode *vp = rd->rd_vnode;
	register struct rd_user *rduptr;

	for (rduptr=rd->rd_user_list; rduptr; rduptr = rduptr->ru_next) {
		if (rduptr->ru_srmntindx == mntindx)
			break;
	}
	ASSERT(rduptr);
	DUPRINT5(DB_SERVE,
	    "rsl_alloc: rduser %x, vp %x, epid %d, mntindx %d\n",
	    rduptr, vp, epid, mntindx);

	rsl_lock(rduptr);

	/* If there's one already, just return */
	for (lkp = rduptr->ru_lk; lkp; lkp = lkp->lk_next) {
		if (lkp->lk_epid == epid)
			goto out;
	}
	nlkp = (struct rs_lock *)new_kmem_fast_alloc(
			&rsl_mem, sizeof (struct rs_lock), 8, KMEM_SLEEP);
	nlkp->lk_epid = epid;
	nlkp->lk_uid = cred->cr_uid;
	nlkp->lk_gid = cred->cr_gid;
	nlkp->lk_next = rduptr->ru_lk;
	rduptr->ru_lk = nlkp;
out:
	rsl_unlock(rduptr);

	DUPRINT3(DB_SERVE, "rsl_alloc: got %s lock record lk %x\n",
		nlkp ? "new" : "existing", nlkp ? nlkp : lkp);
}

/*
 * Release all locks on the designated file held by the designated client
 * process on the designated machine. If no client process is specified
 * (epid == 0), remove all locks on the file associated with the designated
 * machine. mntindx is the mount index for the server mount table corresponding
 * to this client mount.
 */
void
rsl_lockrelease(rd, epid, mntindx)
rcvd_t rd;
short epid;
long mntindx;
{
	register struct rs_lock *lkp, *lkpp, *slkp;
	struct flock fl;
	struct ucred cr;
	register struct vnode *vp = rd->rd_vnode;
	register struct rd_user *rduptr;

	bzero((caddr_t) &cr, sizeof (struct ucred));
	crhold(&cr);

	/* Set to unlock whole file */
	fl.l_type = F_UNLCK;
	fl.l_whence = 0;
	fl.l_start = 0;
	fl.l_len = 0;

	for (rduptr = rd->rd_user_list; rduptr; rduptr = rduptr->ru_next) {
		if (rduptr->ru_srmntindx == mntindx)
			break;
	}
	ASSERT(rduptr);

	DUPRINT5(DB_SERVE,
	    "rsl_lockrelease: rduser %x, vp %x, epid %d, mntindx %x\n",
	    rduptr, vp, epid, mntindx);

	rsl_lock(rduptr);

	for (lkpp = NULL, lkp=rduptr->ru_lk; lkp; ) {
		if ((lkp->lk_epid == epid) || (epid == 0)) {
			slkp = lkp;
			if (lkp == rduptr->ru_lk)
				rduptr->ru_lk = lkp = lkp->lk_next;
			else
				lkpp->lk_next = lkp = lkp->lk_next;
			cr.cr_uid = slkp->lk_uid;
			cr.cr_gid = slkp->lk_gid;
			kmem_fast_free(&rsl_mem, (caddr_t) slkp);
			DUPRINT3(DB_SERVE,
			    "rsl_lockrelease: rel lk %x, epid %d\n",
			    slkp, epid);
			(void)VOP_LOCKCTL(vp, &fl, F_SETLK, &cr,
			    RSL_PID(mntindx, epid));
			if (epid)
				break;
		} else {
			lkpp = lkp;
			lkp = lkp->lk_next;
		}
	}

	rsl_unlock(rduptr);
}

/*
 * Check if a file is mandatorily lockable based on permission bits.
 */
mnd_lck(vp, cred)
struct vnode *vp;
struct ucred *cred;
{
	struct vattr va;

	if (VOP_GETATTR(vp, &va, cred) != 0)
		return (0);
	return (MND_LCK(va.va_mode));
}


/*
 * Other miscellaneous routines.
 */

/*
 * 	Read in pathname from user space
 *	upath (from, to, maxbufsize);
 *	Returns -2 if the pathname is too long, -1 if a bad user
 *	address is supplied, otherwise returns the pathname length
 *	(not including the trailing \0; i.e., null pathnames return
 *	length 0.
 *	WARNING: Unlike the ATT version, this code does not check to
 *	see if you are a server and if so do the copy in-kernel, SO
 *	CHECK BEFORE USING IT IN NEWLY PORTED MODULES.
 */
int upath(from, to, maxbufsize)
register char *from, *to; int maxbufsize;
{
	register int l;
	register int c;

	for (l = 0; l < maxbufsize; l++) {
		c = fubyte(from++);
		if (c == -1)
			return (-1);
		if (c & 0x80)
			return (-1);
		*to++ = c;
		if (c == 0)
			return (l);
	}
	return (-2);
}

/*
 * Allocate memory for RFS data structures.
 */
rfs_memalloc()
{
	extern short *rfdev;
	register int i;

	rfheap = new_kmem_zalloc((u_int) rfsize, KMEM_SLEEP);
	advertise = (struct advertise *)new_kmem_zalloc(
		(u_int) nadvertise * sizeof (struct advertise), KMEM_SLEEP);
	rcvd = (struct rcvd *)new_kmem_zalloc(
		(u_int) nrcvd * sizeof (struct rcvd), KMEM_SLEEP);
	sndd = (struct sndd *)new_kmem_zalloc(
		(u_int) nsndd * sizeof (struct sndd), KMEM_SLEEP);
	gdp = (struct gdp *)new_kmem_zalloc(
		(u_int) maxgdp * sizeof (struct gdp), KMEM_SLEEP);
	rd_user = (struct rd_user *)new_kmem_zalloc(
		(u_int) nrduser * sizeof (struct rd_user), KMEM_SLEEP);
	rfsmount = (struct rfsmnt *)new_kmem_zalloc(
		(u_int) nrfsmount * sizeof (struct rfsmnt), KMEM_SLEEP);
	srmount = (struct srmnt *)new_kmem_zalloc(
		(u_int) nsrmount * sizeof (struct srmnt), KMEM_SLEEP);
	serv_proc = (struct serv_proc *)new_kmem_zalloc(
		(u_int) maxserve * sizeof (struct serv_proc), KMEM_SLEEP);
	rfdev = (short *)new_kmem_alloc(
		(u_int) RFDEVSIZE * sizeof (short), KMEM_SLEEP);

	for (i=0; i < RFDEVSIZE; i++)
		rfdev[i] = RFDEVEMPTY;
}

/*
 * Free memory for RFS data structures.
 */
rfs_memfree()
{
	extern short *rfdev;

	kmem_free((caddr_t) rfheap, (u_int) rfsize);
	kmem_free((caddr_t) advertise,
		(u_int) nadvertise * sizeof (struct advertise));
	kmem_free((caddr_t) rcvd, (u_int) nrcvd * sizeof (struct rcvd));
	kmem_free((caddr_t) sndd, (u_int) nsndd * sizeof (struct sndd));
	kmem_free((caddr_t) gdp, (u_int) maxgdp * sizeof (struct gdp));
	kmem_free((caddr_t) rd_user, (u_int) nrduser * sizeof (struct rd_user));
	kmem_free((caddr_t)rfsmount,
		(u_int) nrfsmount * sizeof (struct rfsmnt));
	kmem_free((caddr_t) srmount, (u_int) nsrmount * sizeof (struct srmnt));
	kmem_free((caddr_t) serv_proc,
		(u_int) maxserve * sizeof (struct serv_proc));
	kmem_free((caddr_t) rfdev, (u_int) RFDEVSIZE * sizeof (short));
}

/*
 * Make up a pseudo-device number for the filesystem of a file, to be
 * returned by the server as an st_dev value in response to a DUSTAT call.
 * The RFS client will use only the lower byte of the value returned so you
 * can't just return the lower byte of the server's real device number, as
 * this gives a high probability of collision. You will get collisions
 * anyways, if you go over 255 mounts on the server.
 * 	The algorithm just hashes into a table based on the lower byte of
 * the real device number. Collisions are handled by advancing to the
 * first free slot in the table. The pseudo-device number returned is
 * the hash table index. You don't have to worry about freeing table
 * entries, since the Sun kernel policies for device number allocation
 * are conservative, i.e., device numbers will tend to get re-used,
 * rather than be newly generated, across mounts. Note that when the
 * table starts getting full, performance will suffer.
 */
#define	HASHNO(ID)	((ID) & 0x0ff)
#define	NEXTIND(H)	((H) == 0x0ff ? 0 : (H) + 1)
/*
 * Return the pseudo device number given the real one.
 */
int
rfsdev_pseudo(fsid)
short fsid;
{
	int hashno, i;
	extern short *rfdev;

	hashno = HASHNO(fsid);
	i = hashno;
	do {
		if (rfdev[i] == RFDEVEMPTY)
			rfdev[i] = fsid;
		if (rfdev[i] == fsid)
			break;
		i = NEXTIND(i);
	} while (i != hashno);
	return (i);
}

/*
 * Return the real device number given the pseudo one.
 * Remote ustat() needs this.
 */
short
rfsdev_real(pseudo)
int pseudo;
{
	extern short *rfdev;

	return (rfdev[pseudo & 0x0ff]);
}

/*VARARGS1*/
rfs_printf(duval, numargs, va_alist)
	int duval;
	int numargs;
	va_dcl
{
	va_list x1;
	char *fmt;
	int a2 = 0, a3 = 0, a4 = 0, a5 = 0, a6 = 0;
	int save;

	if (!(dudebug & duval))
		return;

	va_start(x1);
	fmt = va_arg(x1, char *);
	if (numargs < 2)
		goto gotargs;
	a2 = va_arg(x1, int);
	if (numargs < 3)
		goto gotargs;
	a3 = va_arg(x1, int);
	if (numargs < 4)
		goto gotargs;
	a4 = va_arg(x1, int);
	if (numargs < 5)
		goto gotargs;
	a5 = va_arg(x1, int);
	if (numargs < 6)
		goto gotargs;
	a6 = va_arg(x1, int);
gotargs:
	va_end(x1);
	if (dudebug & DB_NOPRINT) {
		save = noprintf;
		noprintf = 1;
	}
	printf(fmt, a2, a3, a4, a5, a6);
	if (dudebug & DB_NOPRINT)
		noprintf = save;
}
