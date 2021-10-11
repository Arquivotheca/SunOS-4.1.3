/*	@(#)ufs_vfsops.c 1.1 92/07/30 SMI; from UCB 4.1 83/05/27	*/

#include <sys/param.h>
#include "boot/systm.h"
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/pathname.h>
#include <sys/vfs.h>
#include "boot/vnode.h"
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <ufs/fs.h>
#include <ufs/mount.h>
#include "boot/inode.h"
#undef NFS
#include <sys/mount.h>
#include <sys/bootconf.h>
#include <machine/param.h>

#ifdef	 NFS_BOOT
#undef u
extern struct user u;
#endif	 /* NFS_BOOT */


/*
 * ufs vfs operations.
 */
int ufs_root();
int ufs_statfs();
int ufs_mountroot();
extern	int	ufs_badop();

struct vfsops ufs_vfsops = {
        ufs_badop,	/* ufs_mount, */
        ufs_badop,	/* ufs_unmount, */
        ufs_root,
        ufs_statfs,
        ufs_badop,	/* ufs_sync, */
        ufs_badop,	/* ufs_vget, */
        ufs_mountroot,
        ufs_badop,	/* ufs_swapvp, */
};

#ifdef	 DUMP_DEBUG
static	int	dump_debug = 10;
#endif	 /* DUMP_DEBUG */

static int ufsrootdone = 0;

/*
 * Mount table.
 */
struct mount    *mounttab = NULL;

/*
 * Called by vfs_mountroot when ufs is going to be mounted as root
 */
ufs_mountroot(vfsp, vpp, name)
	struct vfs *vfsp;
	struct vnode **vpp;
	char *name;
{
        register int error;
	dev_t getblockdev();

#ifdef	DUMP_DEBUG
	dprint(dump_debug, 6,
		"ufs_mountroot(vfsp 0x%x, vpp 0x%x, name '%s')\n",
		vfsp,
		vpp,
		name);
#endif	/* DUMP_DEBUG */
        if (ufsrootdone) {
#ifdef	 DUMP_DEBUG
		dprint(dump_debug, 10, "ufs_mountroot: already mounted\n");
#endif	 /* DUMP_DEBUG */
                return (EBUSY);
        }
	rootdev = getblockdev("root", name);
        if (rootdev == (dev_t)0) {
                return (ENODEV);
        }
        *vpp = bdevvp(rootdev);
	if (*vpp == (struct vnode *)-1) {
		return (ENODEV);
	}

	error = mountfs(vpp, "/", vfsp);
        if (error) {
#ifdef	DUMP_DEBUG
	dprint(dump_debug, 10, "ufs_mountroot: mountfs error 0x%x\n", error);
#endif	/* DUMP_DEBUG */
                VN_RELE(*vpp);
                *vpp = (struct vnode *)0;
                return (error);
        }
	ufsrootdone++;
#ifdef	 DUMP_DEBUG
	dprint(dump_debug, 6, "ufs_mountroot: done\n");
#endif	 /* DUMP_DEBUG */
	return(0);
}

int
mountfs(devvpp, path, vfsp)
        struct vnode **devvpp;
        char *path;
        struct vfs *vfsp;
{
        register struct fs *fsp;
        register struct mount *mp = 0;
        register struct buf *bp = 0;
        struct buf *tp = 0;
        int error;
        int blks;
        caddr_t space;
        int i;
        int size;
        u_int len;
        static int initdone = 0;

#ifdef	 DUMP_DEBUG
	dprint(dump_debug, 6, "mountfs(devvpp 0x%x path '%s' vfsp 0x%x)\n", 
		devvpp, path, vfsp);
#endif	 /* DUMP_DEBUG */

        if (!initdone) {
                ihinit();
                initdone = 1;
        }
        /*
         * Open block device mounted on.
         * When bio is fixed for vnodes this can all be vnode operations
         */
        error = VOP_OPEN(devvpp,
            (vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE, u.u_cred);
        if (error) {
                return (error);
        }


        /*
         * read in superblock
         */
        tp = bread(*devvpp, SBLOCK, SBSIZE);
        if (tp->b_flags & B_ERROR) {
#ifdef	 DUMP_DEBUG
	dprint(dump_debug, 6, "mountfs: bread error 0x%x\n", tp->b_flags);
#endif	 /* DUMP_DEBUG */
                goto out;
        }
	

        /*
         * check for dev already mounted on
         */
	for (mp = mounttab; mp != NULL; mp = mp->m_nxt) {
                if (mp->m_bufp != 0 && (*devvpp)->v_rdev == mp->m_dev) {
                        mp = 0;
                        goto out;
                }
        }
        /*
         * find empty mount table entry
         */
	for (mp = mounttab; mp != NULL; mp = mp->m_nxt) {
                if (mp->m_bufp == 0)	{
#ifdef	 DUMP_DEBUG
	dprint(dump_debug, 6, "mountfs: mp 0x%x\n", mp);
#endif	 /* DUMP_DEBUG */
                      goto found;
		}

        }
	/*
	 * get new mount table entry
	 */
	mp = (struct mount *)kmem_alloc((u_int)sizeof(struct mount));
	if (mp == 0) {
		error = EMFILE;		/* needs translation */
		goto out;
	}
	mp->m_nxt = mounttab;
	mounttab = mp;

found:

        vfsp->vfs_data = (caddr_t)mp;
        mp->m_vfsp = vfsp;
        mp->m_bufp = tp;        /* just to reserve this slot */
        mp->m_dev = NODEV;
        mp->m_devvp = *devvpp;
        fsp = tp->b_un.b_fs;
        if (fsp->fs_magic != FS_MAGIC || fsp->fs_bsize > MAXBSIZE) {
#ifdef	 DUMP_DEBUG
		dprint(dump_debug, 0, "mountfs: bad superblock 0x%x 0x%x\n", 
			fsp->fs_magic, fsp->fs_bsize);
#endif	 /* DUMP_DEBUG */
                goto out;
        }

        /*
         * Copy the super block into a buffer in its native size.
         */
        bp = geteblk((int)fsp->fs_sbsize);
        mp->m_bufp = bp;
        bcopy((caddr_t)tp->b_un.b_addr, (caddr_t)bp->b_un.b_addr,
           (u_int)fsp->fs_sbsize);
        brelse(tp);
        tp = 0;

        fsp = bp->b_un.b_fs;
        if (vfsp->vfs_flag & VFS_RDONLY) {
                fsp->fs_ronly = 1;
        } else {
                fsp->fs_fmod = 1;
                fsp->fs_ronly = 0;
        }
        vfsp->vfs_bsize = fsp->fs_bsize;

        /*
         * Read in cyl group info
         */
        blks = howmany(fsp->fs_cssize, fsp->fs_fsize);
        space = wmemall(vmemall, (int)fsp->fs_cssize);
        if (space == 0)
                goto out;
        for (i = 0; i < blks; i += fsp->fs_frag) {
                size = fsp->fs_bsize;
                if (i + fsp->fs_frag > blks)
                        size = (blks - i) * fsp->fs_fsize;
                tp = bread(mp->m_devvp, (daddr_t)fsbtodb(fsp, fsp->fs_csaddr+i),                    size);
                if (tp->b_flags&B_ERROR) {
#ifdef	 DUMP_DEBUG
			dprint(dump_debug, 6, "mountfs: bad read flags 0x%x\n", 
				tp->b_flags);
#endif	 /* DUMP_DEBUG */
                        wmemfree(space, (int)fsp->fs_cssize);
                        goto out;
                }
                bcopy((caddr_t)tp->b_un.b_addr, space, (u_int)size);
                fsp->fs_csp[i / fsp->fs_frag] = (struct csum *)space;
                space += size;
                brelse(tp);
                tp = 0;
        }
        mp->m_dev = mp->m_devvp->v_rdev;
        vfsp->vfs_fsid.val[0] = (long)mp->m_dev;
        vfsp->vfs_fsid.val[1] = MOUNT_UFS;
        (void) copystr(path, fsp->fs_fsmnt, sizeof(fsp->fs_fsmnt)-1, &len);
        bzero(fsp->fs_fsmnt + len, sizeof (fsp->fs_fsmnt) - len);
#ifdef	 DUMP_DEBUG
	dprint(dump_debug, 0, "mountfs: normal return %s\n", fsp->fs_fsmnt);
#endif	 /* DUMP_DEBUG */
        return (0);

out:     
        if (mp)
                mp->m_bufp = 0;
        if (bp)
                brelse(bp);
        if (tp)
                brelse(tp);
        (void) VOP_CLOSE(*devvpp,
            (vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE, u.u_cred);

        return (EBUSY);
}

/*
 * find root of ufs
 */
int
ufs_root(vfsp, vpp)
        struct vfs *vfsp;
        struct vnode **vpp;
{
        register struct mount *mp;
        register struct inode *ip;
#ifdef	 DUMP_DEBUG
	dprint(dump_debug, 6, "ufs_root(vfsp 0x%x vpp 0x%x)\n", 
		vfsp, vpp);
#endif	 /* DUMP_DEBUG */

        mp = (struct mount *)vfsp->vfs_data;
        ip = iget(mp->m_dev, mp->m_bufp->b_un.b_fs, (ino_t)ROOTINO);
        if (ip == (struct inode *)0) {
                return (u.u_error);
        }
        iunlock(ip);
        *vpp = ITOV(ip);
        return (0);
}

/*
 * Get file system statistics.
 */
int
ufs_statfs(vfsp, sbp)
register struct vfs *vfsp;
struct statfs *sbp;
{
        register struct fs *fsp;
#ifdef	 DUMP_DEBUG
	dprint(dump_debug, 6, "ufs_statfs(vfsp 0x%x sbp 0x%x)\n", 
		vfsp, sbp);
#endif	 /* DUMP_DEBUG */

        fsp = ((struct mount *)vfsp->vfs_data)->m_bufp->b_un.b_fs;
        if (fsp->fs_magic != FS_MAGIC)
                panic("ufs_statfs");
        sbp->f_bsize = fsp->fs_fsize;
        sbp->f_blocks = fsp->fs_dsize;
        sbp->f_bfree =
            fsp->fs_cstotal.cs_nbfree * fsp->fs_frag +
                fsp->fs_cstotal.cs_nffree;
        /*
         * avail = MAX(max_avail - used, 0)
         */
        sbp->f_bavail =
            (fsp->fs_dsize * (100 - fsp->fs_minfree) / 100) -
                 (fsp->fs_dsize - sbp->f_bfree);
        /*
         * inodes
         */
        sbp->f_files =  fsp->fs_ncg * fsp->fs_ipg;
        sbp->f_ffree = fsp->fs_cstotal.cs_nifree;
        bcopy((caddr_t)fsp->fs_id, (caddr_t)&sbp->f_fsid, sizeof (fsid_t));
        return (0);
}


#include <sys/vm.h>

/*
 * Allocate wired-down (non-paged) pages in kernel virtual memory.
 */
caddr_t
wmemall(pmemall, n)
        int (*pmemall)(), n;
{
        register int npg;

#ifdef	 DUMP_DEBUG
	dprint(dump_debug, 6, "wmemall(pmemall 0x%x n 0x%x)\n", 
		pmemall, n);
#endif	 /* DUMP_DEBUG */

	npg = clrnd(btoc(n));
#ifdef	 DUMP_DEBUG
	dprint(dump_debug, 6, "wmemall: npg 0x%x\n", npg);
#endif	 /* DUMP_DEBUG */

	return(kmem_alloc((u_int)ptob(npg)));

}

wmemfree(va, n)
        caddr_t va;
        int n;
{
	register int npg;
#ifdef	 DUMP_DEBUG
	dprint(dump_debug, 6, "wmemfree(va 0x%x n 0x%x)\n", 
		va, n);
		
#endif	 /* DUMP_DEBUG */

	npg = clrnd(btoc(n));
	kmem_free(va, (u_int)ptob(npg));
	return;
}

/*
 * Dummy
 */

vmemall()
{
        
}
