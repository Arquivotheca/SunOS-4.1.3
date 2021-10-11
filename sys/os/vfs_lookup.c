/*	@(#)vfs_lookup.c 1.1 92/07/30 SMI 	*/


#include <sys/param.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/pathname.h>
#include <sys/dirent.h>
#include <sys/audit.h>
#include <sys/syslog.h>
#include <sys/trace.h>

#ifdef TRACE
#define	trace_lookup(DOIT, VP, SS, ES) { 				\
if (DOIT) {								\
	char sc, *tp = (char *) ((((u_long)(ES) + (sizeof (long) - 1))	\
			& ~(sizeof (long) - 1)) - 5 * sizeof (long));	\
	if (tp < (SS))							\
		tp = (SS);						\
	sc = *(ES);							\
	*(ES) = '\0';							\
	(void) trace6(TR_VN_LOOKUPNAME, VP, trs(tp, 0), trs(tp, 1),	\
		trs(tp, 2), trs(tp, 3), trs(tp, 4));			\
	*(ES) = sc;							\
}									\
}
#else TRACE
#define	trace_lookup(DOIT, VP, SS, ES)
#endif TRACE

void au_pathbuild();

/*
 * lookup the user file name,
 * Handle allocation and freeing of pathname buffer, return error.
 * Supports old interface to lookupname.
 */
lookupname(fnamep, seg, followlink, dirvpp, compvpp)
	char *fnamep;			/* user pathname */
	int seg;			/* addr space that name is in */
	enum symfollow followlink;	/* follow sym links */
	struct vnode **dirvpp;		/* ret for ptr to parent dir vnode */
	struct vnode **compvpp;		/* ret for ptr to component vnode */
{

	return (au_lookupname(fnamep, seg, followlink, dirvpp,
	    compvpp, (au_path_t *) 0));
}

/*
 * lookup the user file name,
 * Handle allocation and freeing of pathname buffer, return error.
 */
au_lookupname(fnamep, seg, followlink, dirvpp, compvpp, au_path)
	char *fnamep;			/* user pathname */
	int seg;			/* addr space that name is in */
	enum symfollow followlink;	/* follow sym links */
	struct vnode **dirvpp;		/* ret for ptr to parent dir vnode */
	struct vnode **compvpp;		/* ret for ptr to component vnode */
	au_path_t *au_path;		/* Final pathname */
{
	struct pathname lookpn;
	register int error;

	error = pn_get(fnamep, seg, &lookpn);
	if (error)
		return (error);
	error = au_lookuppn(&lookpn, followlink, dirvpp, compvpp, au_path);
	pn_free(&lookpn);
	return (error);
}

/*
 * Starting at current directory, translate pathname pnp to end.
 * Leave pathname of final component in pnp, return the vnode
 * for the final component in *compvpp, and return the vnode
 * for the parent of the final component in dirvpp.
 */
lookuppn(pnp, followlink, dirvpp, compvpp)
	register struct pathname *pnp;		/* pathaname to lookup */
	enum symfollow followlink;		/* (don't) follow sym links */
	struct vnode **dirvpp;			/* ptr for parent vnode */
	struct vnode **compvpp;			/* ptr for entry vnode */
{

	return (au_lookuppn(pnp, followlink, dirvpp, compvpp, (au_path_t *)0));
}
#if defined(DEBUG_PATH)
printpath(fmt, pnp)
char *fmt;
struct pathname *pnp;
{
	char tbuf[1024];
	register char *src, *dst;
	register int len;

	len = pn_pathleft(pnp);
	if (len >= sizeof tbuf)
		len = sizeof tbuf - 1;
	dst = tbuf;
	src = pn_getpath(pnp);
	while (len-- > 0)
		*dst++ = *src++;
	*dst = 0;
	printf(fmt, tbuf);
}
#endif

/*
 * Starting at current directory, translate pathname pnp to end.
 * Leave pathname of final component in pnp, return the vnode
 * for the final component in *compvpp, and return the vnode
 * for the parent of the final component in dirvpp.
 * If au_path is set, then return the resolved full pathname for the
 * pathname specified.
 *
 * This is the central routine in pathname translation and handles
 * multiple components in pathnames, separating them at /'s.  It also
 * implements mounted file systems and processes symbolic links.
 */
au_lookuppn(pnp, followlink, dirvpp, compvpp, au_path)
	register struct pathname *pnp;		/* pathaname to lookup */
	enum symfollow followlink;		/* (don't) follow sym links */
	struct vnode **dirvpp;			/* ptr for parent vnode */
	struct vnode **compvpp;			/* ptr for entry vnode */
	au_path_t *au_path;			/* Final pathname */
{
	register struct vnode *vp;		/* current directory vp */
	register struct vnode *cvp;		/* current component vp */
	struct vnode *tvp;			/* non-reg temp ptr */
	register struct vfs *vfsp;		/* ptr to vfs for mount indir */
	char component[MAXNAMLEN+1];		/* buffer for component */
	register int error;
	register int nlink;
	int lookup_flags;

#if defined(DEBUG_PATH)
	printpath("au_lookuppn: path <%s>\n\r", pnp);
#endif

	stripslash(pnp);		/* For Posix strip trailing '/'s */

	nlink = 0;
	cvp = (struct vnode *)0;
	lookup_flags = dirvpp ? LOOKUP_DIR : 0;

	/*
	 * start at current directory.
	 */
	vp = u.u_cdir;
	VN_HOLD(vp);
#ifdef SYSAUDIT
	if (au_path != (au_path_t *)0) {
		/* Initialize au_path buffer with CWD path */
		au_path->ap_size = 0;
		au_pathbuild(au_path, u.u_cwd->cw_dir);
	}
#endif SYSAUDIT

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
#ifdef SYSAUDIT
		/* Reset path to "", when pathname starts with '/' */
		if (au_path != (au_path_t *)0) {
			au_path->ap_ptr = au_path->ap_buf;
			*(au_path->ap_ptr) = '\0';
		}
#endif SYSAUDIT
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
		trace_lookup(compvpp, vp, pnp->pn_buf, pnp->pn_path);
		/*
		 * If the caller was interested in the parent then
		 * return an error since we don't have the real parent.
		 * Return EEXIST in this case, to disambiguate EINVAL.
		 */
		if (dirvpp != (struct vnode **)0) {
			VN_RELE(vp);
#if defined(DEBUG_PATH)
			printf("au_lookuppn: no parent [EEXIST]\n\r");
#endif
			return (EEXIST);
		}
		(void) pn_set(pnp, ".");
		if (compvpp != (struct vnode **)0) {
			*compvpp = vp;
		} else {
			VN_RELE(vp);
		}
#if defined(DEBUG_PATH)
		printf("au_lookuppn: done\n\r");
#endif
		return (0);
	}

	/*
	 * Handle "..": two special cases.
	 * 1. If at root directory (e.g. after chroot)
	 *    then ignore it so can't get out.
	 * 2. If this vnode is the root of a mounted
	 *    file system, then replace it with the
	 *    vnode which was mounted on so we take the
	 *    .. in the other file system.
	 */
	if (strcmp(component, "..") == 0) {
checkforroot:
		if (VN_CMP(vp, u.u_rdir) || VN_CMP(vp, rootdir)) {
			cvp = vp;
			VN_HOLD(cvp);
			goto skip;
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
		 * If the path is unreadable, fail now with the right error.
		 */
		if (pn_pathleft(pnp) || dirvpp == (struct vnode **)0 ||
		    (error == EACCES))
			goto bad;

		trace_lookup(dirvpp, vp, pnp->pn_buf,
			pnp->pn_path - strlen(component));

		(void) pn_set(pnp, component);
		*dirvpp = vp;
		if (compvpp != (struct vnode **)0) {
			*compvpp = (struct vnode *)0;
		}
#if defined(DEBUG_PATH)
		printf("au_lookuppn: done\n\r");
#endif
		return (0);
	}

	/*
	 * If this vnode is mounted on, then we
	 * transparently indirect to the vnode which
	 * is the root of the mounted file system.
	 * Before we do this we must check that an unmount is not
	 * in progress on this vnode. This maintains the fs status
	 * quo while a possibly lengthy unmount is going on.
	 */
mloop:
	while (vfsp = cvp->v_vfsmountedhere) {
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

	/*
	 * If we hit a symbolic link and there is more path to be
	 * translated or this operation does not wish to apply
	 * to a link, then place the contents of the link at the
	 * front of the remaining pathname.
	 */
	if (cvp->v_type == VLNK &&
	    ((followlink == FOLLOW_LINK) || pn_pathleft(pnp))) {
		struct pathname linkpath;

		nlink++;
		if (nlink > MAXSYMLINKS) {
			error = ELOOP;
			goto bad;
		}
		error = getsymlink(cvp, &linkpath);
		if (error)
			goto bad;
#if defined(DEBUG_PATH)
		printf("au_lookuppn: symbolic link\n\r");
#endif
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

#ifdef SYSAUDIT
	/* Add this component to the full pathname */
	if (au_path != (au_path_t *)0) {
		au_pathbuild(au_path, component);
	}
#endif SYSAUDIT

skip:
	/*
	 * Skip to next component of the pathname.
	 * If no more components, return last directory (if wanted)  and
	 * last component (if wanted).
	 */
	if (pn_pathleft(pnp) == 0) {
		trace_lookup(dirvpp, vp, pnp->pn_buf,
			pnp->pn_path - strlen(component));
		trace_lookup(compvpp, cvp, pnp->pn_buf, pnp->pn_path);

		(void) pn_set(pnp, component);
		if (dirvpp != (struct vnode **)0) {
			/*
			 * Check that we have the real parent and not
			 * an alias of the last component.
			 * Return EEXIST in this case, to disambiguate EINVAL.
			 */
			if (VN_CMP(vp, cvp)) {
				VN_RELE(vp);
				VN_RELE(cvp);
#if defined(DEBUG_PATH)
				printf("au_lookuppn: last == parent [EEXIST]\n\r");
#endif
				return (EEXIST);
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
#if defined(DEBUG_PATH)
		printf("au_lookuppn: done\n\r");
#endif
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
#if defined(DEBUG_PATH)
	printf("au_lookuppn: error %d\n\r", error);
#endif
	return (error);
}

/*
 * Gets symbolic link into pathname.
 */
int
getsymlink(vp, pnp)
	struct vnode *vp;
	struct pathname *pnp;
{
	struct iovec aiov;
	struct uio auio;
	register int error;

	pn_alloc(pnp);
	aiov.iov_base = pnp->pn_buf;
	aiov.iov_len = MAXPATHLEN;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_resid = MAXPATHLEN;
	error = VOP_READLINK(vp, &auio, u.u_cred);
	if (error)
		pn_free(pnp);
	pnp->pn_pathlen = MAXPATHLEN - auio.uio_resid;
	return (error);
}

/*
 * stripslash	- strip trailing '/'s from the pathname
 *		  Posix allows trailing slashes on names.
 */

stripslash(pnp)
	struct pathname *pnp;
{
	char *end;

	if (pnp->pn_pathlen < 1)
		return;

	end = &pnp->pn_path[pnp->pn_pathlen - 1];

	/*
 	 * Remove the trailing slashes starting at the end, but stop
	 * before removing the first character for the case of the
	 * the name "////", referring to the root with trailing '/'s
	 */

	while( *end == '/' && end != pnp->pn_path) {
		*end = 0;
		end--;
		pnp->pn_pathlen--;
	}
}

