#ifndef lint
static        char sccsid[] = "@(#)vfs_vnode.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/pathname.h>
#include <sys/vfs.h>
#include "boot/vnode.h"

#undef u
extern	struct user u;

static	int	nfsdebug = 10;

/*
 * realease a vnode. Decrements reference count and
 * calls VOP_INACTIVE on last.
 */
void
vn_rele(vp)
        register struct vnode *vp;
{
#ifdef	NFSDEBUG1
	dprint(nfsdebug, 6, "vn_rele(vp 0x%x)\n", vp);
#endif	/* NFSDEBUG */
        /*
         * sanity check
         */
        if (vp->v_count == 0)	{
#ifdef	NFSDEBUG
	dprint(nfsdebug, 6, "vn_rele: zero count\n");
#endif	/* NFSDEBUG */
                panic("vn_rele");
	}
        if (--vp->v_count == 0) {
#ifdef	NFSDEBUG1
	dprint(nfsdebug, 6, "vn_rele: vp 0x%x v_op 0x%x  call inactive 0x%x \n",
		vp, vp->v_op,  vp->v_op->vn_inactive);
#endif	/* NFSDEBUG */
                (void)VOP_INACTIVE(vp, u.u_cred);
        }
}


/*
 * Open/create a vnode.
 * This may be callable by the kernel, the only known side effect being that
 * the current user uid and gid are used for permissions.
 */
int
vn_open(pnamep, seg, filemode, createmode, vpp)
        char *pnamep;
        register int filemode;
        int createmode;
        struct vnode **vpp;
{
        struct vnode *vp;               /* ptr to file vnode */
        register int mode;
        register int error;
#ifdef	NFSDEBUG
	dprint(nfsdebug, 6,
		"vn_open(pnamep '%s' seg 0x%x filemode 0x%x createmode 0x%x vpp 0x%x)\n",
		pnamep, seg, filemode, createmode, vpp);
#endif	/* NFSDEBUG */
 
        mode = 0;
        if (filemode & FREAD)
                mode |= VREAD;
        if (filemode & (FWRITE | FTRUNC))
                mode |= VWRITE;
 
        if (filemode & FCREAT) {
#ifdef	NFSDEBUG
		dprint(nfsdebug, 0,
			"vn_open: create not implemented\n");
#endif	/* NFSDEBUG */
		error = EACCES;
		return (error);
	} else {
		/*
                 * Wish to open a file.
                 * Just look it up.
                 */
                error =
                    lookupname(pnamep, seg, FOLLOW_LINK,
                        (struct vnode **)0, &vp);
                if (error)	{
			if (error == ENOENT)
#ifdef	NFSDEBUG
				dprint (nfsdebug, 10, "%s: No such file or directory\n", pnamep);
#endif	/* NFSDEBUG */
                        return (error);
		} else {
#ifdef	NFSDEBUG
	dprint(nfsdebug, 6,
		"vn_open: lookupname vp 0x%x\n", vp);
#endif	/* NFSDEBUG */
		}
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
#ifdef	NEVER
                        /*
                         * If there's shared text associated with
                         * the vnode, try to free it up once.
                         * If we fail, we can't allow writing.
                         */
                        if (vp->v_flag & VTEXT) {
                                xrele(vp);
                                if (vp->v_flag & VTEXT) {
                                        error = ETXTBSY;
                                        goto out;
                                }
                        }
#endif	/* NEVER */
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
#ifdef	NEVER
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
#endif	/* NEVER */
out:
        if (error) {
#ifdef	NFSDEBUG
	dprint(nfsdebug, 0,
		"vn_open: error 0x%x\n", error);
#endif	/* NFSDEBUG */
                VN_RELE(vp);
        } else {
                *vpp = vp;
        }
        return (error);
}

