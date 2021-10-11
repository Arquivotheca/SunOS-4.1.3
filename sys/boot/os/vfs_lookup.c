#ifndef lint
static        char sccsid[] = "@(#)vfs_lookup.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <mon/sunromvec.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/vfs.h>
#include "boot/vnode.h"
#include <sys/dir.h>
#include <sys/pathname.h>

#ifdef	NFSDEBUG
static	int	nfsdebug = 10;
#endif	NFSDEBUG

#undef u
extern struct user u;

/*
 * lookup the user file name,
 * Handle allocation and freeing of pathname buffer, return error.
 */
lookupname(fnamep, seg, followlink, dirvpp, compvpp)
        char *fnamep;                   /* user pathname */
        int seg;                        /* addr space that name is in */
        enum symfollow followlink;      /* follow sym links */
        struct vnode **dirvpp;          /* ret for ptr to parent dir vnode */
        struct vnode **compvpp;         /* ret for ptr to component vnode */
{
        struct pathname lookpn;
        register int error;
#ifdef	NFSDEBUG
	dprint(nfsdebug, 6,
		"lookupname(fnamep '%s' seg 0x%x followlink 0x%x dirvpp 0x%x compvpp 0x%x)\n",
		fnamep, seg, followlink, dirvpp, compvpp);
#endif	NFSDEBUG

        error = pn_get(fnamep, seg, &lookpn);
        if (error)	{
		printf("Boot:  lookupname: pn_get failed: ");
		errno_print(error);
		printf("\n");
                return (error);
	}
        error = lookuppn(&lookpn, followlink, dirvpp, compvpp);
	if (error) {
		printf("Boot:  lookuppn failed: ");
		errno_print(error);
		printf("\n");
	}
        pn_free(&lookpn);
        return (error);
}

/*
 * Starting at current directory, translate pathname pnp to end.
 * Leave pathname of final component in pnp, return the vnode
 * for the final component in *compvpp, and return the vnode
 * for the parent of the final component in dirvpp.
 *
 * This is the central routine in pathname translation and handles
 * multiple components in pathnames, separating them at /'s.  It also
 * implements mounted file systems and processes symbolic links.
 */
lookuppn(pnp, followlink, dirvpp, compvpp)
        register struct pathname *pnp;          /* pathaname to lookup */
        enum symfollow followlink;              /* (don't) follow sym links */        struct vnode **dirvpp;                  /* ptr for parent vnode */
        struct vnode **compvpp;                 /* ptr for entry vnode */
{
        register struct vnode *vp;              /* current directory vp */
        register struct vnode *cvp;             /* current component vp */
	struct vnode *tvp;                      /* non-reg temp ptr */
        char component[MAXNAMLEN+1];            /* buffer for component */
        register int error;
        register int nlink;


        nlink = 0;
        cvp = (struct vnode *)0;

        /*
         * start at current directory.
         */
        vp = u.u_cdir;
        VN_HOLD(vp);

	/*
	 * This is a 'toy' version of lookuppn, which assumes a
	 * single-element filename starting in the current
	 * directory, which is always, in this version, the
	 * root directory.   We assume on entry that u.u_cdir
	 * does indeed point ot the root vnode.
	 */
#ifdef	NFSDEBUG
	dprint(nfsdebug, 6, "lookuppn: u_rdir 0x%x\n", u.u_rdir);
#endif	NFSDEBUG

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
#ifdef	NFSDEBUG
		dprint(nfsdebug, 6, "lookuppn: unnecessary /\n");
#endif	NFSDEBUG
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
#ifdef	NFSDEBUG
		dprint(nfsdebug, 10, "lookuppn: not directory 0x%x type 0x%x\n",
			vp, vp->v_type);
#endif	NFSDEBUG
                error = ENOTDIR;
                goto bad;
        }
	/*
         * Process the next component of the pathname.
         */
        error = pn_getcomponent(pnp, component);
        if (error)
                goto bad;
#ifdef	NFSDEBUG
	dprint(nfsdebug, 6, "lookuppn: component '%s'\n", component);
#endif	NFSDEBUG

	/*
         * Check for degenerate name (e.g. / or "")
         * which is a way of talking about a directory,
         * e.g. "/." or ".".
         */
        if (component[0] == 0) {
#ifdef	NFSDEBUG
		dprint(nfsdebug, 6, "lookuppn: null name\n");
#endif	NFSDEBUG
		/*
                 * If the caller was interested in the parent then
                 * return an error since we don't have the real parent
                 */
		if (dirvpp != (struct vnode **)0) {
#ifdef	NFSDEBUG
			dprint(nfsdebug, 10,
				"lookuppn: no parent!\n");
#endif	NFSDEBUG
                        VN_RELE(vp);
                        return(EINVAL);
                }
                (void) pn_set(pnp, ".");
                if (compvpp != (struct vnode **)0) {
                        *compvpp = vp;
#ifdef	NFSDEBUG
			dprint(nfsdebug, 6,
				"lookuppn: compvpp 0x%x\n", *compvpp);
#endif	NFSDEBUG
                } else {
#ifdef	NFSDEBUG
			dprint(nfsdebug, 10,
				"lookuppn: vacuous\n");
#endif	NFSDEBUG
                        VN_RELE(vp);
                }
		return(0);
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
                if ((vp == u.u_rdir) || (vp == rootdir)) {
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
	error = VOP_LOOKUP(vp, component, &tvp, u.u_cred);
        cvp = tvp;
        if (error) {
		if (error != ENOENT)
#ifdef	NFSDEBUG
			dprint(nfsdebug, 10, "lookuppn: lookup error 0x%x\n", error);
#endif	NFSDEBUG
                cvp = (struct vnode *)0;
		goto bad;
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
                if (pn_pathleft(&linkpath) == 0)
                        (void) pn_set(&linkpath, ".");
                error = pn_combine(pnp, &linkpath);     /* linkpath before pn */                pn_free(&linkpath);
                if (error)
                        goto bad;
                VN_RELE(cvp);
                cvp = (struct vnode *)0;
                goto begin;
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
#ifdef	NFSDEBUG
				dprint(nfsdebug, 0, "lookuppn: EINVAL\n");
#endif	NFSDEBUG
                                return(EINVAL);
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
#ifdef	NFSDEBUG
		dprint(nfsdebug, 6, "lookuppn: normal return\n");
#endif	/* NFSDEBUG */
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
	if (error != ENOENT)
#ifdef	NFSDEBUG
		dprint(nfsdebug, 10, "lookuppn: error 0x%x\n", error);
#endif	NFSDEBUG
        if (cvp)
                VN_RELE(cvp);
        VN_RELE(vp);
        return (error);

}

/*
 * Gets symbolic link into pathname.
 */
static int
getsymlink(vp, pnp)
        struct vnode *vp;
        struct pathname *pnp;
{
        struct iovec aiov;
        struct uio auio;
        register int error;
#ifdef	 NFSDEBUG
	dprint(nfsdebug, 6, "getsymlink(vp, pnp)\n", vp, pnp);
#endif	 /* NFSDEBUG */

        pn_alloc(pnp);
        aiov.iov_base = pnp->pn_buf;
        aiov.iov_len = MAXPATHLEN;
        auio.uio_iov = &aiov;
        auio.uio_iovcnt = 1;
        auio.uio_offset = 0;
        auio.uio_seg = UIOSEG_KERNEL;
        auio.uio_resid = MAXPATHLEN;
        error = VOP_READLINK(vp, &auio, u.u_cred);
        if (error)
                pn_free(pnp);
        pnp->pn_pathlen = MAXPATHLEN - auio.uio_resid;
#ifdef	 NFSDEBUG
	dprint(nfsdebug, 6, "getsymlink: error 0x%x\n", error);
#endif	 /* NFSDEBUG */
        return (error);
}
