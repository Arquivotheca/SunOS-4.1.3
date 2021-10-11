/*  @(#)tmp_tnode.c 1.1 92/07/30 SMI */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/ucred.h>
#include <sys/user.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/kmem_alloc.h>
#include <debug/debug.h>
#include <vm/seg.h>
#include <vm/seg_map.h>
#include <vm/anon.h>
#include <vm/page.h>
#include <tmpfs/tmp.h>
#include <tmpfs/tmpnode.h>
#include <tmpfs/tmpdir.h>
#include <sys/debug.h>

static struct anon_map *tmpamap_freelist;
static int tmpamap_freeincr = 4;
extern func_t caller();
extern int tmp_anonmem;
extern int tmp_files;
extern int tmp_anonalloc;

struct tmpnode *
tmpnode_alloc(tm, type)
	struct tmount *tm;
	enum vtype type;
{
	struct tmpnode *t;
	extern struct vnodeops tmp_vnodeops;
	extern long tmpimapalloc();

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdebugalloc)
		printf("tmpnode_alloc: tm %x type %d\n", tm, type);
#endif TMPFSDEBUG
	t = (struct tmpnode *)tmp_memalloc(tm, sizeof (struct tmpnode));
	if (t == NULL)
		return (NULL);
	/*
	 * Muck with the doubly linked lists, if appropriate
	 */
	if (tm->tm_rootnode != (struct tmpnode *)NULL) {
		if ((t->tn_forw = tm->tm_rootnode->tn_forw) != NULL)
			t->tn_forw->tn_back = t;
		t->tn_back = tm->tm_rootnode;
		tm->tm_rootnode->tn_forw = t;
	}
	t->tn_attr.va_nodeid = tmpimapalloc(tm);
	t->tn_attr.va_type = type;
	t->tn_attr.va_blocksize = PAGESIZE;
	t->tn_attr.va_fsid = 0xFFFF &
	(long)makedev(vfs_fixedmajor(tm->tm_vfsp), 0xFF & tm->tm_mntno);
	t->tn_vnode.v_vfsp = tm->tm_vfsp;
	t->tn_vnode.v_type = type;
	t->tn_vnode.v_data = (char *)t;
	t->tn_vnode.v_op = &tmp_vnodeops;

	t->tn_amapp = (struct anon_map *)new_kmem_fast_alloc(
	    (caddr_t *)&tmpamap_freelist, sizeof (*tmpamap_freelist),
	    tmpamap_freeincr, KMEM_SLEEP);
	t->tn_amapp->refcnt = 1;	/* keep till last link OR mapping */
	t->tn_amapp->size = 0;
	t->tn_amapp->swresv = 0;
	t->tn_amapp->flags = 0;
	t->tn_amapp->anon = (struct anon **)NULL;

	switch (type) {
	case VDIR:
		tm->tm_directories++;
		tmp_files++;
		break;
	case VREG:
	case VBLK:
	case VCHR:
	case VLNK:
	case VSOCK:
	case VFIFO:
		tm->tm_files++;
		tmp_files++;
		break;
	default:
		panic("tmpnode_alloc()- unknown file type\n");
		break;
	}

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdebugalloc)
		printf("tmpnode_alloc: returning tp %x\n", t);
#endif TMPFSDEBUG

	tmpnode_get(t);
	return (t);
}

void
tmpnode_get(tp)
	struct tmpnode *tp;
{
#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmplockdebug)
		printf("tmpnode_get: tp %x count %d caller %x\n", tp,
		    tp->tn_count, caller());
#endif TMPFSDEBUG
	tp->tn_flags |= TREF;
#ifdef TMPFSDEBUG
	if (tmplockdebug && (tp->tn_count > 0) &&
	    (tp->tn_owner != uniqpid()))
		printf("tmpnode_get: gonna sleep on lock!\n");
#endif TMPFSDEBUG

	(void) tmpnode_lock(tp);
	VN_HOLD(TP_TO_VP(tp));
}

void
tmpnode_put(tp)
	struct tmpnode *tp;
{
#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdirdebug || tmplockdebug)
		printf("tmpnode_put: tp %x caller %x\n", tp, caller());
#endif TMPFSDEBUG
	(void) tmpnode_unlock(tp);
	VN_RELE(TP_TO_VP(tp));
}

tmpnode_inactive(tm, tp)
	struct tmount *tm;
	struct tmpnode *tp;
{
#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdirdebug || tmplockdebug)
		printf("tmpnode_inactive: tp %x count %d\n", tp, tp->tn_count);
#endif TMPFSDEBUG
	if (tp->tn_attr.va_nlink <= 0)
		tmpnode_free(tm, tp);
	else
		tp->tn_flags = 0;
}

/*
 * free tmpnode and all its associated anonymous memory (if any)
 */
void
tmpnode_free(tm, tp)
	struct tmount *tm;
	struct tmpnode *tp;
{

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdebugalloc)
		printf("tmpnode_free: tp %x nlink %d type %x\n", tp,
		    tp->tn_attr.va_nlink, tp->tn_attr.va_type);
#endif TMPFSDEBUG
	switch (tp->tn_attr.va_type) {
	case VDIR:
		/* free . & .. */
		ASSERT(tp->tn_dir == NULL);
		tp->tn_attr.va_nlink -= 2;
		(void) tmpnode_trunc(tm, tp, (u_long)0);
		tm->tm_directories--;
		tmp_files--;
		break;
	case VLNK:
		tmp_memfree(tm, (char *)tp->tn_symlink,
		    (u_int)tp->tn_attr.va_size);
		tm->tm_files--;
		tmp_files--;
		break;
	case VSOCK:
	case VFIFO:
	case VBLK:
	case VCHR:
	case VREG:
		/*
		 * See comment below on anon maps
		 */
		if (tp->tn_amapp->refcnt == 1)
			(void) tmpnode_trunc(tm, tp, (u_long)0);
		tm->tm_files--;
		tmp_files--;
		break;
	default:
		panic("tmpnode_free()- unknown file type\n");
		break;
	}
	/* adjust links in list */
	if ((tp->tn_back->tn_forw = tp->tn_forw) != NULL)
		tp->tn_forw->tn_back = tp->tn_back;

	(void) tmpimapfree(tm, tp->tn_attr.va_nodeid);
	/*
	 * Decrement anon map's reference count
	 * If the count goes to 0, then free the kmem_alloc'd anon map.
	 * If the count >= 1, then let the
	 * mapping process free it up (during a segvn_free)
	 */
	if (--tp->tn_amapp->refcnt == 0) {
		kmem_fast_free((caddr_t *)&tmpamap_freelist,
		    (caddr_t)tp->tn_amapp);
#ifdef TMPFSDEBUG
		if (tmpdebugalloc)
			printf("tmpnode_free: free amap for tp %x\n", tp);
#endif TMPFSDEBUG
	}
	tmp_memfree(tm, (char *)tp, sizeof (struct tmpnode));
}

tmpnode_lock(tp)
	struct tmpnode *tp;
{
#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmplockdebug)
		printf("tmpnode_lock: tp %x caller %x\n", tp, caller());
#endif TMPFSDEBUG
	while (tp->tn_flags & TLOCKED && tp->tn_owner != uniqpid()) {
		tp->tn_flags |= TWANTED;
#ifdef TMPFSDEBUG
		if (tmplockdebug)
			printf("tmpnode_lock: pid %x sleeping on tp %x\n",
			    uniqpid(), tp);
#endif TMPFSDEBUG
		(void) sleep((caddr_t)(tp), PINOD);
	}
	tp->tn_owner = uniqpid();
	tp->tn_count++;
	tp->tn_flags |= TLOCKED;
	/* can't swap process holding a tmpnode lock */
	masterprocp->p_swlocks++;
}

tmpnode_unlock(tp)
	struct tmpnode *tp;
{
#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmplockdebug)
		printf("tmpnode_unlock: tp %x count %d caller %x\n",
		    tp, tp->tn_count, caller());
#endif TMPFSDEBUG
	if (--tp->tn_count < 0)
		panic("tmpnode_unlock()- bad count");
	masterprocp->p_swlocks--;
	if (tp->tn_count == 0) {
		tp->tn_flags &= ~TLOCKED;
		if (tp->tn_flags & TWANTED) {
			tp->tn_flags &= ~TWANTED;
			wakeup((caddr_t)tp);
		}
	}
}

/*
 * patchable variable used to limit the amount of anon space taken by
 * a tmpfs so that we can attempt to avoid deadlock situations
 * XXX this should be set at first mount to some reasonable value
 *	(e.g. MAX(1/4 total available swap space, 4MB) ??)
 */
int tmpfs_hiwater = TMPHIWATER;

/*ARGSUSED*/
tmpnode_findpage(tm, tp, offset)
	register struct tmount *tm;
	register struct tmpnode *tp;
	register u_int offset;
{
	register int pageno = btop(offset);
	register struct anon *ap;
	struct anon *anon_alloc();
	register int allocated = 0;

#ifdef TMPFSDEBUG
	if (tmpfsdebug)
		printf("tmpnode_findpage: tp %x offset %x\n", tp, offset);
#endif TMPFSDEBUG
	/*
	 * Has the size of the file grown off the limits of the anon array?
	 */
	if (offset >= tp->tn_amapp->size)
		if (tmpnode_growmap(tp, offset) == 0)
			return (-1);
	/*
	 * now check to see if there is an allocated anon page
	 * for this access.
	 */
	AMAP_LOCK(tp->tn_amapp);
	if (tp->tn_amapp->anon[pageno] == NULL) {
		allocated = 1;
		if (anoninfo.ani_free < btop(tmpfs_hiwater)) {
#ifdef TMPFSDEBUG
			if (tmpdebugerrs)
				printf("tmpnode_findpage: out of anon space\n");
#endif TMPFSDEBUG
			AMAP_UNLOCK(tp->tn_amapp);
			return (-1);
		}
		if ((ap = anon_alloc()) == NULL) {
#ifdef TMPFSDEBUG
			if (tmpdebugerrs)
				printf("tmpnode_findpage: anon_alloc failed\n");
#endif TMPFSDEBUG
			AMAP_UNLOCK(tp->tn_amapp);
			return (-1);
		}
		tmp_anonalloc++;
		tp->tn_amapp->anon[pageno] = ap;
	}
	AMAP_UNLOCK(tp->tn_amapp);
#ifdef TMPFSDEBUG
	if (tmpfsdebug)
		printf("tmpnode_findpage: allocated %d anon[%d] %x\n",
		    allocated, pageno, tp->tn_amapp->anon[pageno]);
#endif TMPFSDEBUG
	return (allocated);
}

struct timeval tmp_lastgrowtime;

/*
 * grow the anon pointer array to cover 'offset' bytes plus slack
 * The anon_map is always locked before allocating a new anon array
 * and copying the old anon array contents into it.  This is necessary
 * to prevent the `seg_vn' fault handling routine from using the old
 * anon array as a result of a fault on a memory mapped "tmpfs" file.
 */
tmpnode_growmap(tp, offset)
	register struct tmpnode *tp;
	register u_int offset;
{
	register int i, oldsize, newsize;
	register struct anon **newapp;
	extern struct timeval time;

	/*
	 * calculate new length, rounding up in TMAP_ALLOC clicks
	 * to avoid reallocating the anon array each time the file grows
	 * XXX faster with shifts
	 */
	newsize = ((offset + TMAP_ALLOC)/TMAP_ALLOC)*TMAP_ALLOC;
	oldsize = tp->tn_amapp->size;
#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdebugalloc)
		printf("tmpnode_growmap: tp %x oldsize %x newsize %x\n",
		    tp, oldsize, newsize);
#endif TMPFSDEBUG

	newapp = (struct anon **)
	    new_kmem_zalloc(btop(newsize) * sizeof (struct anon *), KMEM_SLEEP);
	/*
	 * copy old array (if it exists)
	 * could have used anon_dup()/anon_free() combination
	 */
	AMAP_LOCK(tp->tn_amapp);
	if (tp->tn_amapp->anon != NULL) {
		for (i = 0; i < btop(oldsize);  i++) {
			newapp[i] = tp->tn_amapp->anon[i];
		}
		tmp_lastgrowtime = time;
		kmem_free((char *)tp->tn_amapp->anon,
		    btop(oldsize) * sizeof (struct anon *));
	}
	tp->tn_amapp->anon = newapp;
	tp->tn_amapp->size = newsize;
	AMAP_UNLOCK(tp->tn_amapp);
#ifdef TMPFSDEBUG
	if (tmpfsdebug)
		printf("tmpnode_growmap: new anonmap %x\n", tp->tn_amapp->anon);
#endif TMPFSDEBUG
	return (1);
}

/*
 * tmpnode_trunc()- set length of tmpnode
 */
/*ARGSUSED*/
tmpnode_trunc(tm, tp, newsize)
	struct tmount *tm;
	struct tmpnode *tp;
	u_long newsize;
{
	register u_int oldsize;
	register struct tdirent *tdp, *tdpx;
	register u_int delta;
	register int pagecreate = 0;
	extern void swap_xlate();

	oldsize = tp->tn_attr.va_size;
#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdebugalloc) {
		printf("tmpnode_trunc: tp %x oldsz %d newsz %d",
		    tp, newsize, oldsize);
		printf(" type %d caller %x\n", tp->tn_attr.va_type, caller());
	}
#endif TMPFSDEBUG
	if (newsize == oldsize)
		return (0);
	switch (tp->tn_attr.va_type) {
	case VREG:
		if (tp->tn_amapp->refcnt > 1)	/* lock first? */
			return (EACCES);
		if (newsize > oldsize) {
			delta = newsize - oldsize;
			/*
			 * grow the size of the anonmap here in case
			 * someone maps the file.  Count the space we're
			 * growing the file here because we can't detect
			 * the actual allocation through a page fault.
			 * rwtmp or segvn_fault will fill in the holes
			 * in anonmap as needed
			 * XXX unfortunately, when the holes are filled in
			 * in segvn_fault, ani_free is decremented changing
			 * the percentage of anonymous memory tmpfs uses
			 */
#ifdef TMPFSDEBUG
			if (tmpfsdebug)
				printf("ttrunc: growing %d bytes\n", delta);
#endif TMPFSDEBUG
			if (btopr(newsize) != btopr(oldsize))
				pagecreate = 1;
			if (tmp_resv(tm, tp, delta, pagecreate))
				return (ENOSPC);
			(void) tmpnode_growmap(tp, (u_int)newsize);
			break;
		}
		/*
		 * XXX - we need to fail here if someone has a mapping to
		 * this tmpnode.  This is because there is no way to alert
		 * the segment layer that the file has shrunk wreaking
		 * havoc if one were to decide to access the anonmap
		 * past its current size
		 */
		if (tp->tn_amapp->refcnt > 1)
			return (EBUSY);
		AMAP_LOCK(tp->tn_amapp);
		/*
		 * delete entire anonmap for tmpnode
		 */
		if (newsize == 0 && tp->tn_amapp->refcnt == 1) { /* XXX */
			u_int anonextra = tp->tn_amapp->swresv - oldsize;
#ifdef TMPFSDEBUG
			if (tmpfsdebug || tmpdebugalloc)
				printf("ttrunc: shrinking %d bytes to 0\n",
				    oldsize);
#endif TMPFSDEBUG
			anon_free(tp->tn_amapp->anon, tp->tn_amapp->swresv);
			tmp_anonalloc -= btopr(oldsize);	/* XXX holes? */
			kmem_free((char *)tp->tn_amapp->anon,
			    btop(tp->tn_amapp->size) * sizeof (struct anon *));
			ASSERT(tp->tn_amapp->swresv >= oldsize);
			tmp_unresv(tm, tp, oldsize);
			/*
			 * anonextra is nonzero if someone had a mapping to
			 * the file and accessed it causing page faults
			 * to fill holes
			 */
			if (anonextra) {
#ifdef TMPFSDEBUG
				if (tmpfsdebug || tmpdebugalloc)
					printf("ttrunc: anonextra %d\n",
					    anonextra);
#endif TMPFSDEBUG
				anon_unresv(anonextra);
			}
			tp->tn_amapp->size = 0;
			tp->tn_amapp->anon = NULL;
			AMAP_UNLOCK(tp->tn_amapp);
			break;
		}
		/*
		 * free anon pages if necessary
		 */
		if (btopr(newsize) != btopr(oldsize)) {
			delta = oldsize - newsize;
#ifdef TMPFSDEBUG
			if (tmpfsdebug || tmpdebugalloc) {
				printf("ttrunc: shrinking %d bytes", delta);
				printf(" anonfree anon[%d] size:%d\n",
				    btopr(newsize),
				    oldsize - roundup(newsize, PAGESIZE));
			}
#endif TMPFSDEBUG
			tmp_unresv(tm, tp, delta);
			anon_free(&tp->tn_amapp->anon[btopr(newsize)],
			    (u_int)(oldsize - roundup(newsize, PAGESIZE)));
			tmp_anonalloc -= btopr(delta);
		}
		/*
		 * zero remainder of page
		 */
		if (tp->tn_amapp->anon[btop(newsize)] != NULL) {
			struct anon *ap = tp->tn_amapp->anon[btop(newsize)];
			struct vnode *swapvp;
			u_int swapoff;
			register struct page *pp;
			u_int offset = newsize & PAGEOFFSET;

			swap_xlate(ap, &swapvp, &swapoff);
			pp = ap->un.an_page;
#ifdef TMPFSDEBUG
			if (tmpfsdebug)
			    printf("ttrunc: zero %d bytes in anon[%d] pp:%x\n",
				    PAGESIZE - offset, btop(newsize), pp);
#endif TMPFSDEBUG
			if (pp != NULL && pp->p_vnode == swapvp &&
			    pp->p_offset == swapoff && !pp->p_gone) {
				if (pp->p_free)
					page_reclaim(pp);
				pagezero(pp, offset, PAGESIZE - offset);
			} else {
				char *base;

				base = segmap_getmap(segkmap, swapvp,
				    swapoff & MAXBMASK);
				(void) kzero(base + (swapoff & MAXBOFFSET) +
				    offset, PAGESIZE - offset);
				(void) segmap_release(segkmap, base, 0);
			}
		}
		AMAP_UNLOCK(tp->tn_amapp);
		break;
	case VLNK:
		if (newsize != 0)
			return (EINVAL);
		tmp_memfree(tm, (char *)tp->tn_symlink,
		    (u_int)tp->tn_attr.va_size);
		tp->tn_symlink = NULL;
		break;
	case VDIR:
		if (newsize != 0)
			return (EINVAL);
#ifdef TMPFSDEBUG
		if (tmpdebugalloc || tmpdirdebug)
			printf("ttrunc: dir %x caller %x\n", tp, caller());
#endif TMPFSDEBUG
		for (tdp = tp->tn_dir; tdp; tdp = tdpx) {
#ifdef TMPFSDEBUG
			if (tmpdebugalloc || tmpdirdebug)
				printf("ttrunc: deleting %s\n", tdp->td_name);
#endif TMPFSDEBUG
			tdpx = tdp->td_next;
			tm->tm_direntries--;
			tmp_memfree(tm, (char *)tdp, (u_int)tdp->td_reclen);
		}
		tp->tn_dir = NULL;
		break;
	default:
		return (0);
	}
	tp->tn_attr.va_size = newsize;
	return (0);
}

/*
 * check for any actively used files
 * called from tmp_unmount
 */
tmpnode_checkopen(tm)
	register struct tmount *tm;
{
	register struct tmpnode *tnp;

	/*
	 * The rootnode is always referenced with a v_count >= 1
	 * If v_count > 1, we're busy.
	 */
	if (tm->tm_rootnode->tn_vnode.v_count > 1)
		return (1);
	for (tnp = tm->tm_rootnode->tn_forw; tnp; tnp = tnp->tn_forw)
		if (tnp->tn_flags & TREF) {
#ifdef TMPFSDEBUG
			if (tmpdebugerrs)
				printf("tmpnode_checkopen: %x is referenced\n",
				    tnp);
#endif TMPFSDEBUG
			return (1);
		}
	return (0);
}

/*
 * free up all resources associated with the file system
 * called from tmp_unmount
 */
tmpnode_freeall(tm)
	register struct tmount *tm;
{
	register struct tmpnode *tnp, *tnpx;
	register struct tmpimap *tmapp0, *tmapp1;

#ifdef TMPFSDEBUG
	if (tmpfsdebug)
		printf("tmpnode_freeall: tm %x\n", tm);
#endif TMPFSDEBUG
	for (tnp = tm->tm_rootnode; tnp; tnp = tnpx) {
		switch (tnp->tn_attr.va_type) {
		case VDIR:
			tm->tm_directories--;
			tmp_files--;
			(void) tmpnode_trunc(tm, tnp, (u_long)0);
			break;
		case VLNK:
		case VREG:
		case VBLK:
		case VCHR:
		case VFIFO:
		case VSOCK:
			tm->tm_files--;
			tmp_files--;
			(void) tmpnode_trunc(tm, tnp, (u_long)0);
			break;
		default:
			break;		/* XXX shouldn't this be a panic? */
		}
		tnpx = tnp->tn_forw;
		tmp_memfree(tm, (char *)tnp, sizeof (struct tmpnode));
	}
	/*
	 * Free the inode maps
	 */
	tmapp0 = tm->tm_inomap.timap_next;
	while (tmapp0 != NULL) {
		tmapp1 = tmapp0->timap_next;
		tmp_memfree(tm, (char *)tmapp0, sizeof (struct tmpimap));
		tmapp0 = tmapp1;
	}
}

int
tmp_resv(tm, tp, delta, pagecreate)
	register struct tmount *tm;
	register struct tmpnode *tp;
	register u_int delta;
	register int pagecreate;
{

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdebugalloc)
		printf("tmp_resv: tm %x tp %x delta %d\n", tm, tp, delta);
#endif TMPFSDEBUG
	/*
	 * don't do anon_resv unless we actually need to reserve a page
	 * this is because of a rounding error that occurs because
	 * anon_resv does a btopr(delta)
	 */
	if (pagecreate && (anon_resv(delta) == 0))
		return (1);
	tm->tm_anonmem += delta;
	tp->tn_amapp->swresv += delta;
	tmp_anonmem += delta;
#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdebugalloc)
		printf("tmp_resv: ani_resv %d\n", anoninfo.ani_resv);
#endif TMPFSDEBUG
	return (0);
}

tmp_unresv(tm, tp, delta)
	register struct tmount *tm;
	register struct tmpnode *tp;
	register u_int delta;
{

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdebugalloc)
		printf("tmp_unresv: tm %x tp %x delta %d\n", tm, tp, delta);
#endif TMPFSDEBUG
	anon_unresv(delta);
	tm->tm_anonmem -= delta;
	tp->tn_amapp->swresv -= delta;
	tmp_anonmem -= delta;
#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdebugalloc)
		printf("tmp_unresv: ani_resv %d\n", anoninfo.ani_resv);
#endif TMPFSDEBUG
}
