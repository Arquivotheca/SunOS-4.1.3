/*	@(#)lo_subr.c 1.1 92/07/30 SMI 	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/trace.h>

#include <lofs/lnode.h>
#include <lofs/loinfo.h>

extern struct vnodeops lo_vnodeops;
#ifdef	LODEBUG
extern int lodebug;
#endif	LODEBUG
static struct lnode *lfind();
static struct vfs *makelfsnode();
static struct lfsnode *lfsfind();
int lo_lfscount = 0;

struct lnode *lofreelist;

/*
 * return a looped back vnode for the given vnode.
 * If no lnode exists for this vnode create one and put it
 * in a table hashed by vnode.  If the lnode for
 * this fhandle is already in the table return it (ref count is
 * incremented by lfind.  The lnode will be flushed from the
 * table when lo_inactive calls freelonode.
 * NOTE: vp is assumed to be a held vnode.
 */
struct vnode *
makelonode(vp, li)
	struct vnode *vp;
	struct loinfo *li;
{
	register struct lnode *lp;

	if ((lp = lfind(vp, li)) == NULL) {
		if (lofreelist) {
			lp = lofreelist;
			lofreelist = lp->lo_next;
			bzero((caddr_t)lp, (u_int)(sizeof *lp));
		} else {
			lp = (struct lnode *)new_kmem_zalloc(
					sizeof (*lp), KMEM_SLEEP);
		}
		lp->lo_vp = vp;
		trace2(TR_MP_LNODE, ltov(lp), vp);
		ltov(lp)->v_count = 1;
		ltov(lp)->v_op = &lo_vnodeops;
		ltov(lp)->v_type = vp->v_type;
		ltov(lp)->v_rdev = vp->v_rdev;
		ltov(lp)->v_data = (caddr_t)lp;
		ltov(lp)->v_vfsp = makelfsnode(vp->v_vfsp, li);
		lsave(lp);
		li->li_refct++;
	} else {
		VN_RELE(vp);
	}
	return (ltov(lp));
}

/*
 * Get/Make vfs structure for given real vfs
 */
static struct vfs *
makelfsnode(vfsp, li)
	struct vfs *vfsp;
	struct loinfo *li;
{
	struct lfsnode *lfs;

	if (vfsp == li->li_realvfs)
		return (li->li_mountvfs);
	if ((lfs = lfsfind(vfsp, li)) == NULL) {
		lfs = (struct lfsnode *)
			new_kmem_zalloc(sizeof (*lfs), KMEM_SLEEP);
		lo_lfscount++;
		lfs->lfs_realvfs = vfsp;
		lfs->lfs_vfs.vfs_op = &lo_vfsops;
		lfs->lfs_vfs.vfs_data = (caddr_t)li;
		lfs->lfs_vfs.vfs_flag =
			(vfsp->vfs_flag | li->li_mflag) & INHERIT_VFS_FLAG;
		lfs->lfs_vfs.vfs_bsize = vfsp->vfs_bsize;
		lfs->lfs_next = li->li_lfs;
		li->li_lfs = lfs;
	}
	lfs->lfs_refct++;
	return (&lfs->lfs_vfs);
}

/*
 * Free lfs node since no longer in use
 */
static void
freelfsnode(lfs, li)
	struct lfsnode *lfs;
	struct loinfo *li;
{
	struct lfsnode *prev = NULL;
	struct lfsnode *this;

	for (this = li->li_lfs; this; this = this->lfs_next) {
		if (this == lfs) {
			if (prev == NULL)
				li->li_lfs = lfs->lfs_next;
			else
				prev->lfs_next = lfs->lfs_next;
			kmem_free((caddr_t)lfs, sizeof (struct lfsnode));
			lo_lfscount--;
			return;
		}
		prev = this;
	}
	panic("freelfsnode");
}

/*
 * Find lfs given real vfs and mount instance(li)
 */
static struct lfsnode *
lfsfind(vfsp, li)
	struct vfs *vfsp;
	struct loinfo *li;
{
	struct lfsnode *lfs;

	for (lfs = li->li_lfs; lfs; lfs = lfs->lfs_next)
		if (lfs->lfs_realvfs == vfsp)
			return (lfs);
	return (NULL);
}

/*
 * Find real vfs given loopback vfs
 */
struct vfs *
lo_realvfs(vfsp)
	struct vfs *vfsp;
{
	struct loinfo *li = vtoli(vfsp);
	struct lfsnode *lfs;

	if (vfsp == li->li_mountvfs)
		return (li->li_realvfs);
	for (lfs = li->li_lfs; lfs; lfs = lfs->lfs_next)
		if (vfsp == &lfs->lfs_vfs)
			return (lfs->lfs_realvfs);
	panic("lo_realvfs");
	/* NOTREACHED */
}

/*
 * Lnode lookup stuff.
 * These routines maintain a table of lnodes hashed by vp so
 * that the lnode for a vp can be found if it already exists.
 * NOTE: LTABLESIZE must be a power of 2 for ltablehash to work!
 */

#define	LTABLESIZE	16
#define	ltablehash(vp)	((((int)(vp))>>4) & (LTABLESIZE-1))

struct lnode *ltable[LTABLESIZE];

/*
 * Put a lnode in the table
 */
lsave(lp)
	struct lnode *lp;
{

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lsave lp %x hash %d\n",
			lp, ltablehash(lp->lo_vp));
#endif
	lp->lo_next = ltable[ltablehash(lp->lo_vp)];
	ltable[ltablehash(lp->lo_vp)] = lp;
}

/*
 * Remove a lnode from the table
 */
freelonode(lp)
	struct lnode *lp;
{
	struct lnode *lt;
	struct lnode *ltprev = NULL;
	struct loinfo *li;
	struct lfsnode *lfs;
	struct vfs *vfsp;

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "freelonode lp %x hash %d\n",
			lp, ltablehash(lp->lo_vp));
#endif
	lt = ltable[ltablehash(lp->lo_vp)];
	while (lt != NULL) {
		if (lt == lp) {
#ifdef LODEBUG
			lo_dprint(lodebug, 4, "freeing %x, vfsp %x\n",
					ltov(lp), ltov(lp)->v_vfsp);
#endif
			vfsp = ltov(lp)->v_vfsp;
			li = vtoli(vfsp);
			li->li_refct--;
			if (vfsp != li->li_mountvfs) {
				/* check for ununsed lfs */
				lfs = lfsfind(lo_realvfs(vfsp), li);
				lfs->lfs_refct--;
				if (lfs->lfs_refct <= 0)
					freelfsnode(lfs, li);
			}
			if (ltprev == NULL) {
				ltable[ltablehash(lp->lo_vp)] = lt->lo_next;
			} else {
				ltprev->lo_next = lt->lo_next;
			}
			lt->lo_next = lofreelist;
			lofreelist = lt;
			return;
		}
		ltprev = lt;
		lt = lt->lo_next;
	}
}

/*
 * Lookup a lnode by vp
 */
static struct lnode *
lfind(vp, li)
	struct vnode *vp;
	struct loinfo *li;
{
	register struct lnode *lt;

	lt = ltable[ltablehash(vp)];
	while (lt != NULL) {
		if (lt->lo_vp == vp && vtoli(ltov(lt)->v_vfsp) == li) {
			ltov(lt)->v_count++;
			return (lt);
		}
		lt = lt->lo_next;
	}
	return (NULL);
}

/*
 * Utilities used by both client and server
 * Standard levels:
 * 0) no debugging
 * 1) hard failures
 * 2) soft failures
 * 3) current test software
 * 4) main procedure entry points
 * 5) main procedure exit points
 * 6) utility procedure entry points
 * 7) utility procedure exit points
 * 8) obscure procedure entry points
 * 9) obscure procedure exit points
 * 10) random stuff
 * 11) all <= 1
 * 12) all <= 2
 * 13) all <= 3
 * ...
 */

#ifdef LODEBUG
/*VARARGS2*/
lo_dprint(var, level, str, a1, a2, a3, a4, a5, a6, a7, a8, a9)
	int var;
	int level;
	char *str;
	int a1, a2, a3, a4, a5, a6, a7, a8, a9;
{

	if (var == level || (var > 10 && (var - 10) >= level))
		printf(str, a1, a2, a3, a4, a5, a6, a7, a8, a9);
}
#endif
