/*	@(#)tfs_vfsops.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/vfs.h>
#include <sys/vfs_stat.h>
#include <sys/vnode.h>
#include <sys/pathname.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <tfs/tfs.h>
#include <sys/mount.h>
#include <tfs/tnode.h>
#include <nfs/nfs_clnt.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/if_ether.h>

#ifdef TFSDEBUG
extern int	tfsdebug;
#endif

extern char	*strcpy();
extern struct tnode *tfind();
#define	MAPSIZE  256/NBBY
static char tfs_minmap[MAPSIZE]; /* Map for minor device allocation */

/*
 * tfs vfs operations.
 */
static int tfs_mount();
static int tfs_unmount();
static int tfs_root();
static int tfs_statfs();
static int tfs_sync();
extern int tfs_badop();
extern int tfs_noop();

struct vfsops tfs_vfsops = {
	tfs_mount,
	tfs_unmount,
	tfs_root,
	tfs_statfs,
	tfs_sync,
	tfs_noop,		/* vget */
	tfs_noop,		/* mountroot */
	tfs_noop,		/* swapvp */
};

/*
 * tfs mount vfsop
 * Set up mount info record and attach it to vfs struct.
 */
/*ARGSUSED*/
static int
tfs_mount(vfsp, path, data)
	struct vfs *vfsp;
	char *path;
	caddr_t data;
{
	int error;
	struct vnode *rtvp = NULL;	/* the server's root */
	struct nfs_args args;		/* tfs mount arguments */
	fhandle_t fh;			/* root fhandle */
	struct sockaddr_in saddr;	/* server's address */
	char host[HOSTNAMESZ];		/* server's name */
	struct mntinfo *mi;
	struct vattr va;
	struct ifaddr *ifa;

	/*
	 * get arguments
	 */
	error = copyin(data, (caddr_t)&args, sizeof (args));
	if (error) {
		return (error);
	}

	/*
	 * Get server address
	 */
	error = copyin((caddr_t)args.addr, (caddr_t)&saddr,
	    sizeof (saddr));
	if (error) {
		return (error);
	}

	/*
	 * Get the root fhandle
	 */
	error = copyin((caddr_t)args.fh, (caddr_t)&fh, sizeof (fh));
	if (error) {
		return (error);
	}

	/*
	 * Get server's hostname
	 */
	if (args.flags & NFSMNT_HOSTNAME) {
		error = copyin((caddr_t)args.hostname, (caddr_t)host,
		    HOSTNAMESZ);
		if (error) {
			return (error);
		}
	} else {
		(void) strcpy(host, "tfs");
	}

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_mount(%x)  host '%s'\n",
			vfsp, host);
#endif
	/*
	 * create a mount record and link it to the vfs struct
	 */
	mi = (struct mntinfo *)new_kmem_alloc((u_int)sizeof (*mi), KMEM_SLEEP);
	bzero((caddr_t)mi, sizeof (*mi));
	mi->mi_hard = ((args.flags & NFSMNT_SOFT) == 0);
	mi->mi_int = ((args.flags & NFSMNT_INT) == NFSMNT_INT);
	mi->mi_tsize = min(NFS_MAXDATA, (u_int)nfstsize());
	mi->mi_curread = mi->mi_tsize;
	mi->mi_addr = saddr;
	bcopy(host, mi->mi_hostname, HOSTNAMESZ);
	if (args.flags & NFSMNT_RETRANS) {
		mi->mi_retrans = args.retrans;
		if (args.retrans < 0) {
			error = EINVAL;
			goto bad;
		}
	} else {
		mi->mi_retrans = 1;
	}
	if (args.flags & NFSMNT_TIMEO) {
		/*
		 * With dynamic retransmission, the mi_timeo is used only
		 * as a hint for the first one.  Set the deviation and
		 * rtxcur to the supplied timeout value.  (The timeo is
		 * in units of 100msec, deviation is in units of hz
		 * shifted left by two, and rtxcur is in units of hz.)
		 */
		mi->mi_timeo = args.timeo;
		mi->mi_timers[NFS_CALLTYPES].rt_deviate = (args.timeo*hz*2)/5;
		mi->mi_timers[NFS_CALLTYPES].rt_rtxcur = args.timeo*hz/10;
		if (args.timeo <= 0) {
			error = EINVAL;
			goto bad;
		}
	} else {
		mi->mi_timeo = 300;
	}
	vfsp->vfs_data = (caddr_t)mi;

	/*
	 * Make root vnode
	 */
	error = tfs_rootvp(&fh, vfsp, &rtvp, u.u_cred);
	if (error) {
		goto bad;
	}
	if (rtvp->v_flag & VROOT) {
		error = EBUSY;
		goto bad;
	}
	rtvp->v_flag |= VROOT;
	/*
	 * Get attributes of the root vnode to determine the
	 * filesystem block size.
	 */
	error = tfs_getattr(rtvp, &va, u.u_cred);
	if (error) {
		goto bad;
	}
	mi->mi_rootvp = rtvp;
	mi->mi_mntno = vfs_getnum(tfs_minmap, MAPSIZE);

	/*
	 * Use same heuristic that NFS uses to determine if the readdir
	 * buffer size should be adjusted dynamically.  (It should be
	 * adjusted dynamically if saddr is in a different subnet or net.)
	 */
	ifa = ifa_ifwithdstaddr((struct sockaddr *)&saddr);
	if (ifa == (struct ifaddr *)0)
		ifa = ifa_ifwithnet((struct sockaddr *)&saddr);
	mi->mi_dynamic = (ifa == (struct ifaddr *)0 ||
		ifa->ifa_ifp == (struct ifnet *)0 ||
		ifa->ifa_ifp->if_mtu < ETHERMTU);

	vfsp->vfs_fsid.val[0] = mi->mi_mntno;
	vfsp->vfs_fsid.val[1] = MOUNT_TFS;

	/*
	 * Set filesystem block size to at least CLBYTES and at most MAXBSIZE.
	 */
	vfsp->vfs_bsize = MAX(va.va_blocksize, CLBYTES);
	vfsp->vfs_bsize = MIN(vfsp->vfs_bsize, MAXBSIZE);

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 5, "tfs_mount: returning 0\n");
#endif
	return (0);
bad:
	if (mi) {
		kmem_free((caddr_t)mi, sizeof (*mi));
	}
	if (rtvp) {
		VN_RELE(rtvp);
	}
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 5, "tfs_mount: returning %d\n", error);
#endif
	return (error);
}

/*
 * Undo tfs mount
 */
static int
tfs_unmount(vfsp)
	struct vfs *vfsp;
{
	struct mntinfo *mi;

	mi = vftomi(vfsp);
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_unmount(%x) mi %x\n", vfsp, mi);
#endif
	if (mi->mi_refct != 1 || mi->mi_rootvp->v_count != 1) {
#ifdef TFSDEBUG
		tfs_dprint(tfsdebug, 4, "tfs_unmount refct %d v_ct %d\n",
				mi->mi_refct, mi->mi_rootvp->v_count);
#endif
		return (EBUSY);
	}
	VN_RELE(mi->mi_rootvp);
	vfs_putnum(tfs_minmap, mi->mi_mntno);
	kmem_free((caddr_t)mi, (u_int)sizeof (*mi));
	return (0);
}

/*
 * find root of tfs
 */
static int
tfs_root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{
	*vpp = (struct vnode *)vftomi(vfsp)->mi_rootvp;
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_root(0x%x) = %x\n", vfsp, *vpp);
	VFS_RECORD(vfsp, VS_ROOT, VS_CALL);
#endif
	VN_HOLD(*vpp);
	return (0);
}

/*
 * Get file system statistics.
 */
static int
tfs_statfs(vfsp, sbp)
	struct vfs *vfsp;
	struct statfs *sbp;
{
	fhandle_t *fh;
	struct nfsstatfs fs;
	int error;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_statfs %x\n", vfsp);
	VFS_RECORD(vfsp, VS_STATFS, VS_CALL);
#endif
	fh = &VTOT(vftomi(vfsp)->mi_rootvp)->t_fh;
	error = tfscall(vftomi(vfsp), TFS_STATFS, xdr_fhandle, (caddr_t)fh,
			xdr_statfs, (caddr_t)&fs, u.u_cred);
	if (!error) {
		error = geterrno(fs.fs_status);
	}
	if (!error) {
		sbp->f_bsize = fs.fs_bsize;
		sbp->f_blocks = fs.fs_blocks;
		sbp->f_bfree = fs.fs_bfree;
		sbp->f_bavail = fs.fs_bavail;
		/*
		 * XXX This is wrong - should be a real fsid
		 */
		bcopy((caddr_t)&vfsp->vfs_fsid,
			(caddr_t)&sbp->f_fsid, sizeof (fsid_t));
	}
	return (error);
}

/*
 * The sync operation in this VFS does nothing, since the TFS itself
 * doesn't do any I/O.  (The underlying VFS's will take care of syncing
 * the real vnodes with pending I/O.)
 */
/*ARGSUSED*/
static int
tfs_sync(vfsp)
	struct vfs *vfsp;
{
	return (0);
}
