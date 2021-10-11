/*  @(#)tmp_vfsops.c 1.1 92/07/30 SMI */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/kmem_alloc.h>
#include <vm/anon.h>
#include <tmpfs/tmpnode.h>
#include <tmpfs/tmp.h>
#include <tmpfs/tmpdir.h>

/*
 * tmpfs vfs operations.
 */
static int tmp_mount();
static int tmp_unmount();
static int tmp_root();
static int tmp_statfs();
static int tmp_sync();
static int tmp_vget();
static int tmp_mountroot();
static int tmp_swapvp();

struct vfsops tmp_vfsops = {
	tmp_mount,
	tmp_unmount,
	tmp_root,
	tmp_statfs,
	tmp_sync,
	tmp_vget,
	tmp_mountroot,
	tmp_swapvp
};

/*
 * patchable variables, otherwise tmpfs_maxkmem set on first tmp_mount.
 */
u_int tmpfs_maxprockmem = TMPMAXPROCKMEM;	/* percent of kernel memory */
						/* this tmpfs can use */
u_int tmpfs_maxkmem = 0;		/* patch at the risk of your life */

#define	TMPMAPSIZE	32/NBBY
static char tmpfs_minmap[TMPMAPSIZE];	/* map for minor dev num allocation */
static struct tmount *tmpfs_mountp = 0;	/* linked list of tmpfs mount structs */

/*ARGSUSED*/
static int
tmp_mount(vfsp, path, data)
	struct vfs *vfsp;
	char *path;
	caddr_t data;
{
	int error;
	register struct tmount *tmx;
	struct tmount *tm;
	struct tmpnode *tp;
	struct ucred rootcred;
	extern int physmem;
	extern long tmpimapalloc();
	u_int len;

	if (tmpfs_maxkmem == 0) {	/* first mount or (god forbid) patch */
		tmpfs_maxkmem = MAX(PAGESIZE,
		    (ptob(physmem) * tmpfs_maxprockmem)/100);
	}
	/* allocate and initialize tmount structure */
	tm = (struct tmount *)
		new_kmem_zalloc(sizeof (struct tmount), KMEM_SLEEP);
#ifdef TMPFSDEBUG
	if (tmpfsdebug)
		printf("tmp_mount: vfsp %x tm %x path %s\n", vfsp, tm, path);
#endif TMPFSDEBUG

	/* link the structure into the list */
	tmx = tmpfs_mountp;
	if (tmx == (struct tmount *)NULL)
		tmpfs_mountp = tm;
	else {
		for (; tmx->tm_next; tmx = tmx->tm_next)
			;
		tmx->tm_next = tm;
	}

	tm->tm_vfsp = vfsp;
	if ((tm->tm_mntno = vfs_getnum(tmpfs_minmap, TMPMAPSIZE)) == -1) {
		kmem_free((char *)tm, sizeof (struct tmount));
		return (EAGAIN);	/* why not 'mount table full'? */
	}
	/*
	 * Kludge Alert!
	 * inodes 0 and 1 have traditionally been unused.
	 * the next two calls have the effect of invalidating 0 and 1
	 * for tmpfs use
	 */
	(void)tmpimapalloc(tm);
	(void)tmpimapalloc(tm);

	vfsp->vfs_data = (caddr_t)tm;
	vfsp->vfs_fsid.val[0] = tm->tm_mntno;
	vfsp->vfs_fsid.val[1] = MOUNT_TMP;
	(void) copystr(path, tm->tm_mntpath, sizeof (tm->tm_mntpath) - 1, &len);
	bzero(tm->tm_mntpath + len, sizeof (tm->tm_mntpath) - len);

	/* allocate and initialize root tmpnode structure */
	tp = tmpnode_alloc(tm, VDIR);
	if (tp == NULL) {
		kmem_free((char *)tm, sizeof (struct tmount));
		return (ENOSPC);
	}
	tmpnode_unlock(tp);
	tm->tm_rootnode = tp;
	newnode(tp, S_IFDIR | 0777, 0, 0);	/* XXX Permissions? */
	tp->tn_vnode.v_flag |= VROOT;
	rootcred.cr_uid = 0;
	rootcred.cr_gid = 0;
	if ((error = tdirenter(tm, tp, ".", tp, &rootcred)) ||
	    (error = tdirenter(tm, tp, "..", tp, &rootcred))) {
		tmpnode_free(tm, tp);
		kmem_free((char *)tm, sizeof (struct tmount));
		return (error);		/* XXX Could we lose some memory? */
	}
	return (0);
}

static int
tmp_sync()
{
	return (0);
}

/*
 * global statistics
 */

int tmp_anonmem;	/* amount of anon space reserved for all tmpfs */
int tmp_kmemspace;	/* amount of kernel heap used by all tmpfs */
int tmp_files;		/* number of files or directories in all tmpfs */
int tmp_anonalloc;	/* approximate # of anon pages allocated to tmpfs */

static int
tmp_unmount(vfsp)
	struct vfs *vfsp;
{
	register struct tmount *tm = VFSP_TO_TM(vfsp);
	register struct tmount *tmx;
	void vfs_putnum();

#ifdef TMPFSDEBUG
	if (tmpfsdebug)
		printf("tmp_unmount: tm %x\n", tm);
#endif TMPFSDEBUG
	/*
	 * Don't close down the tmpfs if there are ANY opened files
	 */
	if (tmpnode_checkopen(tm))
		return (EBUSY);

	/*
	 * Remove from tmp mount list
	 */
	if ((tmx = tmpfs_mountp) == tm)
		tmpfs_mountp = tm->tm_next;
	else
		for (; tmx; tmx = tmx->tm_next)
			if (tmx->tm_next == tm)
				tmx->tm_next = tm->tm_next;

	vfs_putnum(tmpfs_minmap, (int)tm->tm_mntno);

	/*
	 * must free all kmemalloc'd and anonalloc'd memory associated with
	 * this filesystem
	 */
	(void)tmpnode_freeall(tm);	/* frees all files and directories */

	if ((tm->tm_anonmem != 0) || (tm->tm_files != 0) ||
	    (tm->tm_directories != 0) || (tm->tm_direntries != 0) ||
	    (tm->tm_kmemspace != 0)) {
#ifdef TMPFSDEBUG
		if (tm || tmpdebugerrs) {	/* always true for now */
			printf("tmpnode_freeall bad bookkeeping tm %x\n", tm);
			printf("files = %d\n", tm->tm_files);
			printf("directories = %d\n", tm->tm_directories);
			printf("direntries = %d\n", tm->tm_direntries);
			printf("anonmem = %d\n", tm->tm_anonmem);
			printf("kmemspace = %d\n", tm->tm_kmemspace);
		}
#endif TMPFSDEBUG
	}
	kmem_free((char *)tm, sizeof (struct tmount));
	return (0);
}

static int
tmp_root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{
	struct tmount *tm = VFSP_TO_TM(vfsp);
	struct tmpnode *tp = tm->tm_rootnode;

	tmpnode_get(tp);
	tmpnode_unlock(tp);
	*vpp = TP_TO_VP(tp);
#ifdef TMPFSDEBUG
	if (tmpfsdebug)
		printf("tmp_root: tm %x tp %x vpp\n", tm, tp, *vpp);
#endif TMPFSDEBUG
	return (0);
}

static int
tmp_statfs(vfsp, sbp)
	struct vfs *vfsp;
	struct statfs *sbp;
{
	struct tmount *tm = VFSP_TO_TM(vfsp);
	extern int tmpfs_hiwater;

#ifdef TMPFSDEBUG
	if (tmpfsdebug)
		printf("tmp_statfs: tm %x sbp %x\n", tm, sbp);
#endif TMPFSDEBUG
	sbp->f_bsize = PAGESIZE;
	sbp->f_blocks = anoninfo.ani_free - btop(tmpfs_hiwater) +
		btop(tmp_anonmem);
	sbp->f_bfree = sbp->f_blocks - btop(tm->tm_anonmem);
	sbp->f_bavail = sbp->f_bfree;
	/*
	 * The maximum number of files available is the number of tmpnodes we
	 * can allocate from the remaining kernel memory available to tmpfs.
	 */
	sbp->f_ffree = (tmpfs_maxkmem - tmp_kmemspace)/sizeof (struct tmpnode);
	sbp->f_files = sbp->f_ffree + tmp_files;
	if (sbp->f_bfree < 0) {
		sbp->f_blocks -= sbp->f_bfree;
		sbp->f_files -= sbp->f_bfree;
		sbp->f_bfree = sbp->f_bavail = sbp->f_ffree = 0;
	}
	sbp->f_fsid = vfsp->vfs_fsid;
	return (0);
}

/*ARGSUSED*/
static int
tmp_vget(vfsp, vpp, fidp)
	struct vfs *vfsp;
	struct vnode **vpp;
	struct fid *fidp;
{
#ifdef TMPFSDEBUG
	if (tmpfsdebug)
		printf("tmp_vget: vfsp %x fidp %x\n", vfsp, fidp);
#endif TMPFSDEBUG
	*vpp = NULL;
	return (0);
}

static int
tmp_mountroot()
{
	return (EINVAL);
}

static int
tmp_swapvp()
{
	return (EINVAL);
}
