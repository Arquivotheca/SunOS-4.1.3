/*      @(#)seg_vn.c 1.1 92/07/30 SMI      */

#ident	"$SunId: @(#)seg_vn.c 1.4 91/04/10 SMI [RMTC] $"


/*
 * Copyright (c) 1988, 1989 by Sun Microsystems, Inc.
 */

/*
 * VM - shared or copy-on-write from a vnode/anonymous memory.
 */

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/mman.h>
#include <sys/debug.h>
#include <sys/ucred.h>
#include <sys/trace.h>
#include <sys/vmsystm.h>
#include <sys/user.h>
#include <sys/kernel.h>

#include <vm/hat.h>
#include <vm/mp.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_vn.h>
#include <vm/pvn.h>
#include <vm/anon.h>
#include <vm/page.h>
#include <vm/swap.h>
#include <vm/vpage.h>

#include <machine/seg_kmem.h>

/*
 * Private seg op routines.
 */
static	int segvn_dup(/* seg, newsegp */);
static	int segvn_unmap(/* seg, addr, len */);
static	int segvn_free(/* seg */);
static	faultcode_t segvn_fault(/* seg, addr, len, type, rw */);
static	faultcode_t segvn_faulta(/* seg, addr */);
static	int segvn_hatsync(/* seg, addr, ref, mod, flags */);
static	int segvn_setprot(/* seg, addr, len, prot */);
static	int segvn_checkprot(/* seg, addr, len, prot */);
static	int segvn_kluster(/* seg, addr, delta */);
static	u_int segvn_swapout(/* seg */);
static	int segvn_sync(/* seg, addr, len, flags */);
static	int segvn_incore(/* seg, addr, len, vec */);
static	int segvn_lockop(/* seg, addr, len, op */);
static	int segvn_advise(/* seg, addr, len, function */);

struct	seg_ops segvn_ops = {
	segvn_dup,
	segvn_unmap,
	segvn_free,
	segvn_fault,
	segvn_faulta,
	segvn_hatsync,
	segvn_setprot,
	segvn_checkprot,
	segvn_kluster,
	segvn_swapout,
	segvn_sync,
	segvn_incore,
	segvn_lockop,
	segvn_advise,
};

/*
 * Common zfod structures, provided as a shorthand for others to use.
 */
static
struct	segvn_crargs zfod_segvn_crargs = {
	(struct vnode *)NULL,
	0,
	(struct ucred *)NULL,
	MAP_PRIVATE,
	PROT_ALL,
	PROT_ALL,
	(struct anon_map *)NULL,
};

static
struct	segvn_crargs kzfod_segvn_crargs = {
	(struct vnode *)NULL,
	0,
	(struct ucred *)NULL,
	MAP_PRIVATE,
	PROT_ALL & ~PROT_USER,
	PROT_ALL & ~PROT_USER,
	(struct anon_map *)NULL,
};

caddr_t	zfod_argsp = (caddr_t)&zfod_segvn_crargs;	/* user zfod argsp */
caddr_t	kzfod_argsp = (caddr_t)&kzfod_segvn_crargs;	/* kernel zfod argsp */

/*
 * Variables for maintaining the free lists
 * of segvn_data and anon_map structures.
 */
static struct segvn_data *segvn_freelist;
static int segvn_freeincr = 8;
static struct anon_map *anonmap_freelist;
static int anonmap_freeincr = 4;

#define	vpgtob(n)	((n) * sizeof (struct vpage))	/* For brevity */

static	u_int anon_slop = 64*1024;	/* allow segs to expand in place */

static	int concat(/* seg1, seg2, a */);
static	int extend_prev(/* seg1, seg2, a */);
static	int extend_next(/* seg1, seg2, a */);
static	void anonmap_alloc(/* seg, swresv */);
static	void segvn_vpage(/* seg */);

/*
 * Routines needed externally
 */
struct anon_map *anonmap_fast_alloc();
void anonmap_fast_free(/* amp */);

int
segvn_create(seg, argsp)
	struct seg *seg;
	caddr_t argsp;
{
	register struct segvn_crargs *a = (struct segvn_crargs *)argsp;
	register struct segvn_data *svd;
	register u_int swresv = 0;
#ifdef LWP
	extern int runthreads;
#endif LWP

	/*
	 * Check arguments for invalid combinations.
	 */
	if ((a->type != MAP_PRIVATE && a->type != MAP_SHARED) ||
	    (a->amp != NULL && a->vp != NULL))
		panic("segvn_create args");

	/*
	 * If segment may need anonymous pages, reserve them now.
	 */
	if ((a->vp == NULL && a->amp == NULL) ||
	    (a->type == MAP_PRIVATE && (a->prot & PROT_WRITE))) {
		if (anon_resv(seg->s_size) == 0)
			return (ENOMEM);
		swresv = seg->s_size;
	}

	/*
	 * If more than one segment in the address space, and
	 * they're adjacent virtually, try to concatenate them.
	 * Don't concatenate if an explicit anon_map structure
	 * was supplied (e.g., SystemV shared memory).
	 *
	 * We also don't try concatenation if this is a segment
	 * for the kernel's address space.  This is a kludge
	 * because the kernel has several threads of control
	 * active at the same time and we can get in trouble
	 * if we reallocate the anon_map while another process
	 * is trying to fill the old anon_map in.
	 * XXX - need as/seg locking to fix the general problem of
	 * multiple threads in an address space instead of this kludge.
	 */
#ifdef LWP
	if ((seg->s_prev != seg) && (a->amp == NULL) &&
	    (seg->s_as != &kas) && !runthreads) {
#else
	if ((seg->s_prev != seg) && (a->amp == NULL) && (seg->s_as != &kas)) {
#endif LWP
		register struct seg *pseg, *nseg;

		/* first, try to concatenate the previous and new segments */
		pseg = seg->s_prev;
		if (pseg->s_base + pseg->s_size == seg->s_base &&
		    pseg->s_ops == &segvn_ops &&
		    extend_prev(pseg, seg, a, swresv) == 0) {
			/* success! now try to concatenate with following seg */
			nseg = pseg->s_next;
			if (nseg != pseg && nseg->s_ops == &segvn_ops &&
			    pseg->s_base + pseg->s_size == nseg->s_base)
				(void) concat(pseg, nseg);
			return (0);
		}
		/* failed, so try to concatenate with following seg */
		nseg = seg->s_next;
		if (seg->s_base + seg->s_size == nseg->s_base &&
		    nseg->s_ops == &segvn_ops &&
		    extend_next(seg, nseg, a, swresv) == 0)
			return (0);
	}

	svd = (struct segvn_data *)new_kmem_fast_alloc(
			(caddr_t *)&segvn_freelist, sizeof (*segvn_freelist),
			segvn_freeincr, KMEM_SLEEP);
	lock_init(&svd->lock);
	seg->s_data = (char *)svd;
	seg->s_ops = &segvn_ops;

	if (a->vp != NULL) {
		VN_HOLD(a->vp);
	}
	svd->vp = a->vp;
	svd->offset = a->offset & PAGEMASK;
	svd->prot = a->prot;
	svd->maxprot = a->maxprot;
	svd->pageprot = 0;
	svd->type = a->type;
	svd->vpage = NULL;
	svd->advice = -1;
	svd->pageadvice = 0;
	if (a->cred != NULL) {
		svd->cred = a->cred;
		crhold(svd->cred);
	} else {
		svd->cred = crgetcred();
	}

	if (svd->type == MAP_SHARED && a->vp == NULL && a->amp == NULL) {
		/*
		 * We have a shared mapping to an anon_map object
		 * which hasn't been allocated yet.  Allocate the
		 * struct now so that it will be properly shared
		 * by remembering the swap reservation there.
		 */
		svd->swresv = 0;
		anonmap_alloc(seg, swresv);
	} else if ((svd->amp = a->amp) != NULL) {
		u_int anon_num;

		/*
		 * Mapping to an existing anon_map structure.
		 */
		svd->swresv = swresv;

		/*
		 * For now we will insure that the segment size isn't larger
		 * than the size - offset gives us.  Later on we may wish to
		 * have the anon array dynamically allocated itself so that
		 * we don't always have to allocate all the anon pointer
		 * slots.  This of course involves adding extra code to check
		 * that we aren't trying to use an anon pointer slot beyond
		 * the end of the currently allocated anon array.
		 */
		if ((a->amp->size - a->offset) < seg->s_size)
			panic("segvn_create anon_map size");

		anon_num = btopr(a->offset);
		if (a->type == MAP_SHARED) {
			/*
			 * SHARED mapping to a given anon_map.
			 */
			a->amp->refcnt++;
			svd->anon_index = anon_num;
		} else {
			/*
			 * PRIVATE mapping to a given anon_map.
			 * Then make sure that all the needs anon
			 * structures are created (so that we will
			 * share the underlying pages if nothing
			 * is written by this mapping) and then
			 * duplicate the anon array as is done
			 * when a privately mapped segment is dup'ed.
			 */
			register struct anon **app;
			register addr_t addr;
			addr_t eaddr;

			anonmap_alloc(seg, 0);

			AMAP_LOCK(svd->amp);
			app = &a->amp->anon[anon_num];
			eaddr = seg->s_base + seg->s_size;

			for (addr = seg->s_base; addr < eaddr;
			    addr += PAGESIZE, app++) {
				struct page *pp;

				if (*app != NULL)
					continue;

				/*
				 * Allocate the anon struct now.
				 * Might as well load up translation
				 * to the page while we're at it...
				 */
				pp = anon_zero(seg, addr, app);
				if (*app == NULL)
					panic("segvn_create anon_zero");
				hat_memload(seg, addr, pp,
				    svd->prot & ~PROT_WRITE, 0);
				PAGE_RELE(pp);
			}

			anon_dup(&a->amp->anon[anon_num], svd->amp->anon,
			    seg->s_size);
			AMAP_UNLOCK(svd->amp);
		}
	} else {
		svd->swresv = swresv;
		svd->anon_index = 0;
	}

	return (0);
}

/*
 * Concatenate two existing vnode segments, if possible.
 * Return 0 on success.
 */
static int
concat(seg1, seg2)
	struct seg *seg1, *seg2;
{
	register struct segvn_data *svd1, *svd2;
	register u_int size, swresv;
	register struct anon_map *amp1, *amp2;
	register struct vpage *vpage1, *vpage2;

	svd1 = (struct segvn_data *)seg1->s_data;
	svd2 = (struct segvn_data *)seg2->s_data;

	/* both segments exist, try to merge them */
#define	incompat(x)	(svd1->x != svd2->x)
	if (incompat(vp) || incompat(maxprot) ||
	    (!svd1->pageprot && !svd2->pageprot && incompat(prot)) ||
	    (!svd1->pageadvice && !svd2->pageadvice && incompat(advice)) ||
	    incompat(type) || incompat(cred))
		return (-1);
#undef incompat
	/* XXX - need to check credentials more carefully */
	/* vp == NULL implies zfod, offset doesn't matter */
	if (svd1->vp != NULL &&
	    svd1->offset + seg1->s_size != svd2->offset)
		return (-1);
	amp1 = svd1->amp;
	amp2 = svd2->amp;
	/* XXX - for now, reject if any private pages.  could merge. */
	if (amp1 != NULL || amp2 != NULL)
		return (-1);
	/* if either seg has vpages, create new merged vpages */
	vpage1 = svd1->vpage;
	vpage2 = svd2->vpage;
	if (vpage1 != NULL || vpage2 != NULL) {
		register int npages1, npages2;
		register struct vpage *vp, *new_vpage;

		npages1 = seg_pages(seg1);
		npages2 = seg_pages(seg2);
		new_vpage = (struct vpage *)new_kmem_zalloc(
		    (u_int)(vpgtob(npages1 + npages2)), KMEM_SLEEP);
		if (vpage1 != NULL)
			bcopy((caddr_t)vpage1, (caddr_t)new_vpage,
			    (u_int)vpgtob(npages1));
		if (vpage2 != NULL)
			bcopy((caddr_t)vpage2, (caddr_t)(new_vpage + npages1),
			    (u_int)vpgtob(npages2));
		for (vp = new_vpage; vp < new_vpage + npages1; vp++) {
			if (svd2->pageprot && !svd1->pageprot)
				vp->vp_prot = svd1->prot;
			if (svd2->pageadvice && !svd1->pageadvice)
				vp->vp_advice = svd1->advice;
		}
		for (vp = new_vpage + npages1;
		    vp < new_vpage + npages1 + npages2; vp++) {
			if (svd1->pageprot && !svd2->pageprot)
				vp->vp_prot = svd2->prot;
			if (svd1->pageadvice && !svd2->pageadvice)
				vp->vp_advice = svd2->advice;
		}
		if (vpage1 != NULL)
			kmem_free((caddr_t)vpage1, (u_int)vpgtob(npages1));
		if (svd2->pageprot)
			svd1->pageprot = 1;
		if (svd2->pageadvice)
			svd1->pageadvice = 1;
		svd1->vpage = new_vpage;
	}

	/* all looks ok, merge second into first */
	size = seg2->s_size;
	swresv = svd2->swresv;
	svd2->swresv = 0;	/* so seg_free doesn't release swap space */
	seg_free(seg2);
	seg1->s_size += size;
	svd1->swresv += swresv;
	return (0);
}

/*
 * Extend the previous segment (seg1) to include the
 * new segment (seg2 + a), if possible.
 * Return 0 on success.
 */
static int
extend_prev(seg1, seg2, a, swresv)
	struct seg *seg1, *seg2;
	register struct segvn_crargs *a;
	u_int swresv;
{
	register struct segvn_data *svd1;
	register u_int size;
	register struct anon_map *amp1;
	struct vpage *new_vpage;

	/* second segment is new, try to extend first */
	svd1 = (struct segvn_data *)seg1->s_data;
	if (svd1->vp != a->vp || svd1->maxprot != a->maxprot ||
	    (!svd1->pageprot && (svd1->prot != a->prot)) ||
	    svd1->type != a->type)
		return (-1);
	/* vp == NULL implies zfod, offset doesn't matter */
	if (svd1->vp != NULL &&
	    svd1->offset + seg1->s_size != (a->offset & PAGEMASK))
		return (-1);
	amp1 = svd1->amp;
	if (amp1) {
		/*
		 * segment has private pages, can
		 * data structures be expanded?
		 */
		if (amp1->refcnt > 1 && amp1->size != seg1->s_size)
			return (-1);
		AMAP_LOCK(amp1);
		if (amp1->size - ctob(svd1->anon_index) <
		    seg1->s_size + seg2->s_size) {
			struct anon **aa;
			u_int asize;

			/*
			 * We need a bigger anon array.  Allocate a new
			 * one with anon_slop worth of slop at the
			 * end so it will be easier to expand in
			 * place the next time we need to do this.
			 */
			asize = seg1->s_size + seg2->s_size + anon_slop;
			aa = (struct anon **) new_kmem_zalloc(
				(u_int)btop(asize) * sizeof (struct anon *),
				KMEM_SLEEP);
			bcopy((caddr_t)(amp1->anon + svd1->anon_index),
			    (caddr_t)aa,
			    (u_int)(btop(seg1->s_size) * sizeof (struct anon *)));
			kmem_free((caddr_t)amp1->anon,
			    btop(amp1->size) * sizeof (struct anon *));
			amp1->anon = aa;
			amp1->size = asize;
			svd1->anon_index = 0;
		} else {
			/*
			 * Can just expand anon array in place.
			 * Clear out anon slots after the end
			 * of the currently used slots.
			 */
			bzero((caddr_t)(amp1->anon + svd1->anon_index +
			    seg_pages(seg1)),
			    seg_pages(seg2) * sizeof (struct anon *));
		}
		AMAP_UNLOCK(amp1);
	}
	if (svd1->vpage != NULL) {
		new_vpage = (struct vpage *)new_kmem_zalloc(
			(u_int)(vpgtob(seg_pages(seg1) + seg_pages(seg2))),
			KMEM_SLEEP);
		bcopy((caddr_t)svd1->vpage, (caddr_t)new_vpage,
		    (u_int)vpgtob(seg_pages(seg1)));
		kmem_free((caddr_t)svd1->vpage, (u_int)vpgtob(seg_pages(seg1)));
		svd1->vpage = new_vpage;
		if (svd1->pageprot) {
			register struct vpage *vp, *evp;

			vp = new_vpage + seg_pages(seg1);
			evp = vp + seg_pages(seg2);
			for (; vp < evp; vp++)
				vp->vp_prot = a->prot;
		}
	}
	size = seg2->s_size;
	seg_free(seg2);
	seg1->s_size += size;
	svd1->swresv += swresv;
	return (0);
}

/*
 * Extend the next segment (seg2) to include the
 * new segment (seg1 + a), if possible.
 * Return 0 on success.
 */
static int
extend_next(seg1, seg2, a, swresv)
	struct seg *seg1, *seg2;
	register struct segvn_crargs *a;
	u_int swresv;
{
	register struct segvn_data *svd2 = (struct segvn_data *)seg2->s_data;
	register u_int size;
	register struct anon_map *amp2;
	struct vpage *new_vpage;

	/* first segment is new, try to extend second */
	if (svd2->vp != a->vp || svd2->maxprot != a->maxprot ||
	    (!svd2->pageprot && (svd2->prot != a->prot)) ||
	    svd2->type != a->type)
		return (-1);
	/* vp == NULL implies zfod, offset doesn't matter */
	if (svd2->vp != NULL &&
	    (a->offset & PAGEMASK) + seg1->s_size != svd2->offset)
		return (-1);
	amp2 = svd2->amp;
	if (amp2) {
		/*
		 * Segment has private pages, can
		 * data structures be expanded?
		 */
		if (amp2->refcnt > 1)
			return (-1);
		AMAP_LOCK(amp2);
		if (ctob(svd2->anon_index) < seg1->s_size) {
			struct anon **aa;
			u_int asize;

			/*
			 * We need a bigger anon array.  Allocate a new
			 * one with anon_slop worth of slop at the
			 * beginning so it will be easier to expand in
			 * place the next time we need to do this.
			 */
			asize = seg1->s_size + seg2->s_size + anon_slop;
			aa = (struct anon **)new_kmem_zalloc(
				(u_int)btop(asize) * sizeof (struct anon *),
				KMEM_SLEEP);
			bcopy((caddr_t)(amp2->anon + svd2->anon_index),
			    (caddr_t)(aa + btop(anon_slop) + seg_pages(seg1)),
			    (u_int)(btop(seg2->s_size) * sizeof (struct anon *)));
			kmem_free((caddr_t)amp2->anon,
			    btop(amp2->size) * sizeof (struct anon *));
			amp2->anon = aa;
			amp2->size = asize;
			svd2->anon_index = btop(anon_slop);
		} else {
			/*
			 * Can just expand anon array in place.
			 * Clear out anon slots going backwards
			 * towards the beginning of the array.
			 */
			bzero((caddr_t)(amp2->anon + svd2->anon_index -
			    seg_pages(seg1)),
			    seg_pages(seg1) * sizeof (struct anon *));
			svd2->anon_index -= seg_pages(seg1);
		}
		AMAP_UNLOCK(amp2);
	}
	if (svd2->vpage != NULL) {
		new_vpage = (struct vpage *)new_kmem_zalloc(
		    (u_int)vpgtob(seg_pages(seg1) + seg_pages(seg2)),
		    KMEM_SLEEP);
		bcopy((caddr_t)svd2->vpage,
		    (caddr_t)(new_vpage + seg_pages(seg1)),
		    (u_int)vpgtob(seg_pages(seg2)));
		kmem_free((caddr_t)svd2->vpage, (u_int)vpgtob(seg_pages(seg2)));
		svd2->vpage = new_vpage;
		if (svd2->pageprot) {
			register struct vpage *vp, *evp;

			vp = new_vpage;
			evp = vp + seg_pages(seg1);
			for (; vp < evp; vp++)
				vp->vp_prot = a->prot;
		}
	}
	size = seg1->s_size;
	seg_free(seg1);
	seg2->s_size += size;
	seg2->s_base -= size;
	svd2->offset -= size;
	svd2->swresv += swresv;
	return (0);
}

/*
 * Allocate and initialize an anon_map structure for seg
 * associating the given swap reservation with the new anon_map.
 */
static void
anonmap_alloc(seg, swresv)
	register struct seg *seg;
	u_int swresv;
{
	register struct segvn_data *svd = (struct segvn_data *)seg->s_data;

	svd->amp = anonmap_fast_alloc();

	svd->amp->refcnt = 1;
	svd->amp->size = seg->s_size;
	svd->amp->anon = (struct anon **)new_kmem_zalloc(
		(u_int)(seg_pages(seg) * sizeof (struct anon *)), KMEM_SLEEP);
	svd->amp->swresv = swresv;
	svd->amp->flags = 0;

	svd->anon_index = 0;
}

/*
 * Allocate an anon_map structure;
 * also used by ipc_shm.c
 */
struct anon_map *
anonmap_fast_alloc()
{
	register struct anon_map *amp;

	amp = (struct anon_map *)new_kmem_fast_alloc(
	    (caddr_t *)&anonmap_freelist, sizeof (*anonmap_freelist),
	    anonmap_freeincr, KMEM_SLEEP);
	amp->flags = 0;		/* XXX for ipc_shm.c */

	return (amp);
}

/*
 * Free an anon_map structure;
 * also used by ipc_shm.c
 */
void
anonmap_fast_free(amp)
	register struct anon_map *amp;
{
	kmem_fast_free((caddr_t *)&anonmap_freelist,
	    (caddr_t)amp);
}

static int
segvn_dup(seg, newseg)
	struct seg *seg, *newseg;
{
	register struct segvn_data *svd = (struct segvn_data *)seg->s_data;
	register struct segvn_data *newsvd;
	register u_int npages = seg_pages(seg);

	/*
	 * If a segment has anon reserved,
	 * reserve more for the new segment.
	 */
	if (svd->swresv && anon_resv(svd->swresv) == 0)
		return (-1);
	newsvd = (struct segvn_data *)new_kmem_fast_alloc(
			(caddr_t *)&segvn_freelist, sizeof (*segvn_freelist),
			segvn_freeincr, KMEM_SLEEP);
	lock_init(&newsvd->lock);
	newseg->s_ops = &segvn_ops;
	newseg->s_data = (char *)newsvd;

	if ((newsvd->vp = svd->vp) != NULL) {
		VN_HOLD(svd->vp);
	}
	newsvd->offset = svd->offset;
	newsvd->prot = svd->prot;
	newsvd->maxprot = svd->maxprot;
	newsvd->pageprot = svd->pageprot;
	newsvd->type = svd->type;
	newsvd->cred = svd->cred;
	crhold(newsvd->cred);
	newsvd->advice = svd->advice;
	newsvd->pageadvice = svd->pageadvice;
	newsvd->swresv = svd->swresv;
	if ((newsvd->amp = svd->amp) == NULL) {
		/*
		 * No associated anon object.
		 */
		newsvd->anon_index = 0;
	} else {
		if (svd->type == MAP_SHARED) {
			svd->amp->refcnt++;
			newsvd->anon_index = svd->anon_index;
		} else {
			/*
			 * Allocate and initialize new anon_map structure.
			 */
			anonmap_alloc(newseg, 0);
			AMAP_LOCK(svd->amp);
			newsvd->anon_index = 0;

			hat_chgprot(seg, seg->s_base, seg->s_size, ~PROT_WRITE);

			anon_dup(&svd->amp->anon[svd->anon_index],
			    newsvd->amp->anon, seg->s_size);
			AMAP_UNLOCK(svd->amp);
		}
	}
	/*
	 * If necessary, create a vpage structure for the new segment.
	 * Do not copy any page lock indications.
	 */
	if (svd->vpage != NULL) {
		register u_int i;
		register struct vpage *ovp = svd->vpage;
		register struct vpage *nvp;

		nvp = newsvd->vpage = (struct vpage *)
			new_kmem_alloc(vpgtob(npages), KMEM_SLEEP);
		for (i = 0; i < npages; i++) {
			*nvp = *ovp++;
			(nvp++)->vp_pplock = 0;
		}
	} else
		newsvd->vpage = NULL;
	return (0);
}

static int
segvn_unmap(seg, addr, len)
	register struct seg *seg;
	register addr_t addr;
	u_int len;
{
	register struct segvn_data *svd = (struct segvn_data *)seg->s_data;
	register struct segvn_data *nsvd;
	register struct seg *nseg;
	register u_int npages, spages, tpages;
	struct anon **app;
	addr_t nbase;
	u_int nsize, hpages;

	/*
	 * Check for bad sizes
	 */
	if (addr < seg->s_base || addr + len > seg->s_base + seg->s_size ||
	    (len & PAGEOFFSET) || ((u_int)addr & PAGEOFFSET))
		panic("segvn_unmap");

	/*
	 * Remove any page locks set through this mapping.
	 */
	(void) segvn_lockop(seg, addr, len, MC_UNLOCK);

	/*
	 * Unload any hardware translations in the range to be taken out.
	 */
	hat_unload(seg, addr, len);

	/*
	 * Check for entire segment
	 */
	if (addr == seg->s_base && len == seg->s_size) {
		seg_free(seg);
		return (0);
	}

	/*
	 * Check for beginning of segment
	 */
	npages = btop(len);
	spages = seg_pages(seg);
	if (addr == seg->s_base) {
		if (svd->vpage != NULL) {
			svd->vpage = (struct vpage *)new_kmem_resize(
				(caddr_t)svd->vpage, vpgtob(npages),
				vpgtob(spages - npages), vpgtob(spages),
				KMEM_SLEEP);
		}
		if (svd->amp != NULL && (svd->amp->refcnt == 1 ||
		    svd->type == MAP_PRIVATE)) {
			/*
			 * Free up now unused parts of anon_map array.
			 */
			AMAP_LOCK(svd->amp);
			app = &svd->amp->anon[svd->anon_index];
			anon_free(app, len);
			svd->anon_index += npages;
			AMAP_UNLOCK(svd->amp);
		}
		if (svd->vp != NULL)
			svd->offset += len;

		if (svd->swresv) {
			anon_unresv(len);
			svd->swresv -= len;
		}

		seg->s_base += len;
		seg->s_size -= len;
		return (0);
	}

	/*
	 * Check for end of segment
	 */
	if (addr + len == seg->s_base + seg->s_size) {
		tpages = spages - npages;
		if (svd->vpage != NULL) {
			svd->vpage = (struct vpage *)
			new_kmem_resize((caddr_t)svd->vpage, (u_int)0,
				vpgtob(tpages), vpgtob(spages), KMEM_SLEEP);
		}
		if (svd->amp != NULL && (svd->amp->refcnt == 1 ||
		    svd->type == MAP_PRIVATE)) {
			/*
			 * Free up now unused parts of anon_map array
			 */
			AMAP_LOCK(svd->amp);
			app = &svd->amp->anon[svd->anon_index + tpages];
			anon_free(app, len);
			AMAP_UNLOCK(svd->amp);
		}

		if (svd->swresv) {
			anon_unresv(len);
			svd->swresv -= len;
		}

		seg->s_size -= len;
		return (0);
	}

	/*
	 * The section to go is in the middle of the segment,
	 * have to make it into two segments.  nseg is made for
	 * the high end while seg is cut down at the low end.
	 */
	nbase = addr + len;				/* new seg base */
	nsize = (seg->s_base + seg->s_size) - nbase;	/* new seg size */
	seg->s_size = addr - seg->s_base;		/* shrink old seg */
	nseg = seg_alloc(seg->s_as, nbase, nsize);
	if (nseg == NULL)
		panic("segvn_unmap seg_alloc");

	nseg->s_ops = seg->s_ops;
	nsvd = (struct segvn_data *)new_kmem_fast_alloc(
		(caddr_t *)&segvn_freelist, sizeof (*segvn_freelist),
		segvn_freeincr, KMEM_SLEEP);
	nseg->s_data = (char *)nsvd;
	lock_init(&nsvd->lock);
	nsvd->pageprot = svd->pageprot;
	nsvd->prot = svd->prot;
	nsvd->maxprot = svd->maxprot;
	nsvd->type = svd->type;
	nsvd->vp = svd->vp;
	nsvd->cred = svd->cred;
	nsvd->offset = svd->offset + nseg->s_base - seg->s_base;
	nsvd->swresv = 0;
	nsvd->advice = svd->advice;
	nsvd->pageadvice = svd->pageadvice;
	if (svd->vp != NULL)
		VN_HOLD(nsvd->vp);
	crhold(svd->cred);

	tpages = btop(nseg->s_base - seg->s_base);
	hpages = btop(addr - seg->s_base);

	if (svd->vpage == NULL)
		nsvd->vpage = NULL;
	else {
		nsvd->vpage = (struct vpage *)
			new_kmem_alloc(vpgtob(spages - tpages), KMEM_SLEEP);
		bcopy((caddr_t)&svd->vpage[tpages], (caddr_t)nsvd->vpage,
			(u_int)vpgtob(spages - tpages));
		svd->vpage = (struct vpage *)
		new_kmem_resize((caddr_t)svd->vpage, (u_int)0,
				vpgtob(hpages), vpgtob(spages), KMEM_SLEEP);
	}

	if (svd->amp == NULL) {
		nsvd->amp = NULL;
		nsvd->anon_index = 0;
	} else {
		/*
		 * Share the same anon_map structure.
		 */
		if (svd->amp->refcnt == 1 ||
		    svd->type == MAP_PRIVATE) {
			/*
			 * Free up now unused parts of anon_map array
			 */
			AMAP_LOCK(svd->amp);
			app = &svd->amp->anon[svd->anon_index + hpages];
			anon_free(app, len);
			AMAP_UNLOCK(svd->amp);
		}
		nsvd->amp = svd->amp;
		nsvd->anon_index = svd->anon_index + tpages;
		nsvd->amp->refcnt++;
	}
	if (svd->swresv) {
		if (seg->s_size + nseg->s_size + len != svd->swresv)
			panic("segvn_unmap: can split swap reservation");
		anon_unresv(len);
		svd->swresv = seg->s_size;
		nsvd->swresv = nseg->s_size;
	}

	/*
	 * Now we do something so that all the translations which used
	 * to be associated with seg but are now associated with nseg.
	 */
	hat_newseg(seg, nseg->s_base, nseg->s_size, nseg);

	return (0);			/* I'm glad that's all over with! */
}

static
segvn_free(seg)
	struct seg *seg;
{
	register struct segvn_data *svd = (struct segvn_data *)seg->s_data;
	register struct anon **app;
	u_int npages = seg_pages(seg);

	/*
	 * Be sure to unlock pages. XXX Why do things get free'ed instead
	 * of unmapped? XXX
	 */
	(void) segvn_lockop(seg, seg->s_base, seg->s_size, MC_UNLOCK);

	/*
	 * Deallocate the vpage and anon pointers if necessary and possible.
	 */
	if (svd->vpage != NULL)
		kmem_free((caddr_t)svd->vpage, npages * sizeof (struct vpage));
	if (svd->amp != NULL) {
		/*
		 * If there are no more references to this anon_map
		 * structure, then deallocate the structure after freeing
		 * up all the anon slot pointers that we can.
		 */
		if (--svd->amp->refcnt == 0) {
			AMAP_LOCK(svd->amp);
			if (svd->type == MAP_PRIVATE) {
				/*
				 * Private - we only need to anon_free
				 * the part that this segment refers to.
				 */
				anon_free(&svd->amp->anon[svd->anon_index],
				    seg->s_size);
			} else {
				/*
				 * Shared - anon_free the entire
				 * anon_map's worth of stuff and
				 * release any swap reservations.
				 */
				anon_free(svd->amp->anon, svd->amp->size);
				if (svd->amp->swresv)
					anon_unresv(svd->amp->swresv);
			}
			kmem_free((caddr_t)svd->amp->anon,
			    btop(svd->amp->size) * sizeof (struct anon *));
			AMAP_UNLOCK(svd->amp);
			anonmap_fast_free(svd->amp);
		} else if (svd->type == MAP_PRIVATE) {
			/*
			 * We had a private mapping which still has
			 * a held anon_map so just free up all the
			 * anon slot pointers that we were using.
			 */
			AMAP_LOCK(svd->amp);
			app = &svd->amp->anon[svd->anon_index];
			anon_free(app, seg->s_size);
			AMAP_UNLOCK(svd->amp);
		}
	}

	/*
	 * Release swap reservation.
	 */
	if (svd->swresv)
		anon_unresv(svd->swresv);

	/*
	 * Release claim on vnode, credentials, and finally free the
	 * private data.
	 */
	if (svd->vp != NULL)
		VN_RELE(svd->vp);
	crfree(svd->cred);
	kmem_fast_free((caddr_t *)&segvn_freelist, (caddr_t)svd);
}

/*
 * Do a F_SOFTUNLOCK call over the range requested.
 * The range must have already been F_SOFTLOCK'ed.
 */
static void
segvn_softunlock(seg, addr, len, rw)
	struct seg *seg;
	addr_t addr;
	u_int len;
	enum seg_rw rw;
{
	register struct segvn_data *svd = (struct segvn_data *)seg->s_data;
	register struct anon **app;
	register struct page *pp;
	register struct vpage *vpage;
	register addr_t adr;
	struct vnode *vp;
	u_int offset;

	if (svd->amp != NULL) {
		AMAP_LOCK(svd->amp);
		app = &svd->amp->anon[svd->anon_index + seg_page(seg, addr)];
	} else
		app = NULL;

	if (svd->vpage != NULL)
		vpage = &svd->vpage[seg_page(seg, addr)];
	else
		vpage = NULL;

	for (adr = addr; adr < addr + len; adr += PAGESIZE) {
		if (app != NULL && *app != NULL)
			swap_xlate(*app, &vp, &offset);
		else {
			vp = svd->vp;
			offset = svd->offset + (adr - seg->s_base);
		}

		/*
		 * For now, we just kludge here by finding the page
		 * ourselves since we would not find the page using
		 * page_find() if someone has page_abort()'ed it.
		 * XXX - need to redo things to avoid this mess.
		 */
		for (pp = page_hash[PAGE_HASHFUNC(vp, offset)]; pp != NULL;
		    pp = pp->p_hash)
			if (pp->p_vnode == vp && pp->p_offset == offset)
				break;
		if (pp == NULL || pp->p_pagein || pp->p_free)
			panic("segvn_softunlock");

		if (rw == S_WRITE) {
			pg_setmod(pp, 1);
			if (vpage != NULL)
				vpage->vp_mod = 1;
		}
		if (rw != S_OTHER) {
			trace4(TR_PG_SEGVN_FLT, pp, vp, offset, 1);
			pg_setref(pp, 1);
			if (vpage != NULL)
				vpage->vp_ref = 1;
		}
		hat_unlock(seg, adr);
		PAGE_RELE(pp);
		if (vpage != NULL)
			vpage++;
		if (app != NULL)
			app++;
	}
	if (svd->amp != NULL)
		AMAP_UNLOCK(svd->amp);
}

/*
 * Returns true if the app array has some non-anonymous memory
 * The offp and lenp paramters are in/out paramters.  On entry
 * these values represent the starting offset and length of the
 * mapping.  When true is returned, these values may be modified
 * to be the largest range which includes non-anonymous memory.
 */
static int
non_anon(app, offp, lenp)
	register struct anon **app;
	u_int *offp, *lenp;
{
	register int i, el;
	int low, high;

	low = -1;
	for (i = 0, el = *lenp; i < el; i += PAGESIZE) {
		if (*app++ == NULL) {
			if (low == -1)
				low = i;
			high = i;
		}
	}
	if (low != -1) {
		/*
		 * Found at least one non-anon page.
		 * Set up the off and len return values.
		 */
		if (low != 0)
			*offp += low;
		*lenp = high - low + PAGESIZE;
		return (1);
	}
	return (0);
}

#define	PAGE_HANDLED	((struct page *)-1)

/*
 * Release all the pages in the NULL terminated ppp list
 * which haven't already been converted to PAGE_HANDLED.
 */
static void
segvn_pagelist_rele(ppp)
	register struct page **ppp;
{

	for (; *ppp != NULL; ppp++) {
		if (*ppp != PAGE_HANDLED)
			PAGE_RELE(*ppp);
	}
}

int stealcow = 1;

/*
 * Handles all the dirty work of getting the right
 * anonymous pages and loading up the translations.
 * This routine is called only from segvn_fault()
 * when looping over the range of addresses requested.
 *
 * The basic algorithm here is:
 * 	If this is an anon_zero case
 *		Call anon_zero to allocate page
 *		Load up translation
 *		Return
 *	endif
 *	If this is an anon page
 *		Use anon_getpage to get the page
 *	else
 *		Find page in pl[] list passed in
 *	endif
 *	If not a cow
 *		Load up the translation to the page
 *		return
 *	endif
 *	Load and lock translation to the original page
 *	Call anon_private to handle cow
 *	Unload translation to the original page
 *	Load up (writable) translation to new page
 */
static int
segvn_faultpage(seg, addr, off, app, vpage, pl, vpprot, type, rw)
	struct seg *seg;		/* seg_vn of interest */
	addr_t addr;			/* address in as */
	u_int off;			/* offset in vp */
	struct anon **app;		/* pointer to anon for vp, off */
	struct vpage *vpage;		/* pointer to vpage for vp, off */
	struct page *pl[];		/* object source page pointer */
	u_int vpprot;			/* access allowed to object pages */
	enum fault_type type;		/* type of fault */
	enum seg_rw rw;			/* type of access at fault */
{
	register struct segvn_data *svd = (struct segvn_data *)seg->s_data;
	register struct page *pp, **ppp;
	u_int pageflags = 0;
	struct page *anon_pl[1 + 1];
	struct page *opp;		/* original object page */
	u_int prot;
	int err;
	int cow;

	/*
	 * Initialize protection value for this page.
	 * If we have per page protection values check it now.
	 */
	if (svd->pageprot) {
		u_int protchk;

		switch (rw) {
		case S_READ:
			protchk = PROT_READ;
			break;
		case S_WRITE:
			protchk = PROT_WRITE;
			break;
		case S_EXEC:
			protchk = PROT_EXEC;
			break;
		case S_OTHER:
		default:
			protchk = PROT_READ | PROT_WRITE | PROT_EXEC;
			break;
		}

		prot = vpage->vp_prot;
		if ((prot & protchk) == 0)
			return (FC_PROT);	/* illegal access type */
	} else {
		prot = svd->prot;
	}
#ifdef WRITABLE_IMPLIES_MODIFIED
	if (rw != S_WRITE)
		prot &= ~PROT_WRITE;
#endif
	if (svd->vp == NULL && *app == NULL) {
		/*
		 * Allocate a (normally) writable
		 * anonymous page of zeroes
		 */
		if ((pp = anon_zero(seg, addr, app)) == NULL)
			return (FC_MAKE_ERR(ENOMEM));	/* out of swap space */
		pg_setref(pp, 1);
		if (type == F_SOFTLOCK) {
			/*
			 * Load up the translation keeping it
			 * locked and don't PAGE_RELE the page.
			 */
			hat_memload(seg, addr, pp, prot, 1);
		} else {
			hat_memload(seg, addr, pp, prot, 0);
			PAGE_RELE(pp);
		}
		trace5(TR_SPG_FLT, u.u_ar0[PC], addr, svd->vp, off,
		    TRC_SPG_ZERO);
		trace6(TR_SPG_FLT_PROC, time.tv_sec, time.tv_usec,
		    trs(u.u_comm,0), trs(u.u_comm,1),
		    trs(u.u_comm,2), trs(u.u_comm,3));
		return (0);
	}

	/*
	 * Obtain the page structure via anon_getpage() if it is
	 * a private copy of an object (the result of a previous
	 * copy-on-write), or from the pl[] list passed in if it
	 * is from the original object (i.e., not a private copy).
	 */
	if (app != NULL && *app != NULL) {
		err = anon_getpage(app, &vpprot, anon_pl, PAGESIZE,
		    seg, addr, rw, svd->cred);
		if (err)
			return (FC_MAKE_ERR(err));
		if (svd->type == MAP_SHARED) {
			/*
			 * If this is a shared mapping to an
			 * anon_map, then ignore the write
			 * permissions returned by anon_getpage().
			 * They apply to the private mappings
			 * of this anon_map.
			 */
			vpprot |= PROT_WRITE;
		}
		opp = anon_pl[0];
	} else {
		/*
		 * Find original page.  We must be bringing it in
		 * from the list in pl[].
		 */
		for (ppp = pl; (opp = *ppp) != NULL; ppp++) {
			if (opp == PAGE_HANDLED)
				continue;
			ASSERT(opp->p_vnode == svd->vp); /* XXX */
			if (opp->p_offset == off)
				break;
		}
		if (opp == NULL)
			panic("segvn_faultpage not found");
		*ppp = PAGE_HANDLED;
	}

	ASSERT(opp != NULL);

	trace4(TR_PG_SEGVN_FLT, opp, opp->p_vnode, opp->p_offset, 0);
	pg_setref(opp, 1);

	/*
	 * The fault is treated as a copy-on-write fault if a
	 * write occurs on a private segment and the object
	 * page is write protected.  We assume that fatal
	 * protection checks have already been made.
	 */

	cow = (rw == S_WRITE && svd->type == MAP_PRIVATE &&
	    (vpprot & PROT_WRITE) == 0);

	/*
	 * If not a copy-on-write case load the translation
	 * and return.
	 */
	if (cow == 0) {
		if (type == F_SOFTLOCK) {
			/*
			 * Load up the translation keeping it
			 * locked and don't PAGE_RELE the page.
			 */
			hat_memload(seg, addr, opp, prot & vpprot, 1);
		} else {
			hat_memload(seg, addr, opp, prot & vpprot, 0);
			PAGE_RELE(opp);
		}

		trace5(TR_SPG_FLT, u.u_ar0[PC], addr, svd->vp, off,
		    svd->vp == opp->p_vnode ? TRC_SPG_FILE : TRC_SPG_ANON);
		trace6(TR_SPG_FLT_PROC, time.tv_sec, time.tv_usec,
		    trs(u.u_comm,0), trs(u.u_comm,1),
		    trs(u.u_comm,2), trs(u.u_comm,3));
		return (0);
	}

	ASSERT(app != NULL);

	/*
	 * Steal the original page if the following conditions are true:
	 *
	 * We are low on memory, the page is not private, keepcnt is 1,
	 * not modified, not `locked' or if we have it `locked' and
	 * if it doesn't have any translations.
	 */
	if (stealcow && freemem < minfree && *app == NULL &&
	    opp->p_keepcnt == 1 && opp->p_mod == 0 &&
	    (opp->p_lckcnt == 0 || opp->p_lckcnt == 1 &&
	    vpage != NULL && vpage->vp_pplock)) {
		/*
		 * Check if this page has other translations
		 * after unloading our translation.
		 */
		if (opp->p_mapping != NULL)
			hat_unload(seg, addr, PAGESIZE);
		if (opp->p_mapping == NULL)
			pageflags |= STEAL_PAGE;
	}

	/*
	 * Copy-on-write case: anon_private() will copy the contents
	 * of the original page into a new page.  The page fault which
	 * could occur during the copy is prevented by ensuring that
	 * a translation to the original page is loaded and locked.
	 */
	hat_memload(seg, addr, opp, prot & vpprot, 1);

	/*
	 * If the vpage pointer is valid, see if it indicates that
	 * we have ``locked'' the page we map.  If so, ensure that
	 * anon_private() will transfer the locking resource to
	 * the new page.
	 */
	if (vpage != NULL && vpage->vp_pplock)
		pageflags |= LOCK_PAGE;

	/*
	 * Allocate a page and perform the copy.
	 */
	pp = anon_private(app, seg, addr, opp, pageflags);

	if (pp == NULL)
		return (FC_MAKE_ERR(ENOMEM));	/* out of swap space */
	/*
	 * Ok, now just unload the old translation since it has
	 * been unlocked in anon_private().
	 */
	hat_unload(seg, addr, PAGESIZE);

	if (type == F_SOFTLOCK) {
		/*
		 * Load up the translation keeping it
		 * locked and don't PAGE_RELE the page.
		 */
		hat_memload(seg, addr, pp, prot, 1);
	} else {
		hat_memload(seg, addr, pp, prot, 0);
		PAGE_RELE(pp);
	}
	trace5(TR_SPG_FLT, u.u_ar0[PC], addr, svd->vp, off, TRC_SPG_COW);
	trace6(TR_SPG_FLT_PROC, time.tv_sec, time.tv_usec,
	    trs(u.u_comm,0), trs(u.u_comm,1), trs(u.u_comm,2), trs(u.u_comm,3));
	return (0);
}

int fltadvice = 1;	/* set to free behind pages for sequential access */

/*
 * This routine is called via a machine specific fault handling routine.
 * It is also called by software routines wishing to lock or unlock
 * a range of addresses.
 *
 * Here is the basic algorithm:
 *	If unlocking
 *		Call segvn_softunlock
 *		Return
 *	endif
 *	Checking and set up work
 *	If we will need some non-anonymous pages
 *		Call VOP_GETPAGE over the range of non-anonymous pages
 *	endif
 *	Loop over all addresses requested
 *		Call segvn_faultpage passing in page list
 *		    to load up translations and handle anonymous pages
 *	endloop
 *	Load up translation to any additional pages in page list not
 *	    already handled that fit into this segment
 */
static faultcode_t
segvn_fault(seg, addr, len, type, rw)
	struct seg *seg;
	addr_t addr;
	u_int len;
	enum fault_type type;
	enum seg_rw rw;
{
	struct segvn_data *svd = (struct segvn_data *)seg->s_data;
	register struct page **plp, **ppp, *pp;
	struct anon **app;
	u_int off;
	addr_t a;
	struct vpage *vpage;
	u_int vpprot, prot;
	int err;
	struct page *pl[PVN_GETPAGE_NUM + 1];
	u_int plsz, pl_alloc_sz;
	int page;

	/*
	 * First handle the easy stuff
	 */
	if (type == F_SOFTUNLOCK) {
		segvn_softunlock(seg, addr, len, rw);
		return (0);
	}

	/*
	 * If we have the same protections for the entire segment,
	 * insure that the access being attempted is legitimate.
	 */
	if (svd->pageprot == 0) {
		u_int protchk;

		switch (rw) {
		case S_READ:
			protchk = PROT_READ;
			break;
		case S_WRITE:
			protchk = PROT_WRITE;
			break;
		case S_EXEC:
			protchk = PROT_EXEC;
			break;
		case S_OTHER:
		default:
			protchk = PROT_READ | PROT_WRITE | PROT_EXEC;
			break;
		}

		if ((svd->prot & protchk) == 0)
			return (FC_PROT);	/* illegal access type */
	}

	/*
	 * Check to see if we need to allocate an anon_map structure.
	 */
	if (svd->amp == NULL && (svd->vp == NULL ||
	    (rw == S_WRITE && svd->type == MAP_PRIVATE))) {
		anonmap_alloc(seg, 0);
	}

	page = seg_page(seg, addr);
	/*
	 * Lock the anon_map if the segment has private pages.  This is
	 * necessary to ensure that updates of the anon array associated
	 * with the anon_map are atomic.
	 */
	if (svd->amp == NULL)
		app = NULL;
	else {
		AMAP_LOCK(svd->amp);
		app = &svd->amp->anon[svd->anon_index + page];
	}

	if (svd->vpage == NULL)
		vpage = NULL;
	else
		vpage = &svd->vpage[page];

	plp = pl;
	*plp = (struct page *)NULL;
	pl_alloc_sz = 0;
	off = svd->offset + (addr - seg->s_base);
	/*
	 * See if we need to call VOP_GETPAGE for
	 * *any* of the range being faulted on.
	 * We can skip all of this work if there
	 * was no original vnode.
	 */
	if (svd->vp != NULL) {
		u_int vp_off, vp_len;
		int dogetpage;

		if (len > ptob((sizeof (pl) / sizeof (pl[0])) - 1)) {
			/*
			 * Page list won't fit in local array,
			 * allocate one of the needed size.
			 */
			pl_alloc_sz = (btop(len) + 1) * sizeof (struct page *);
			plp = (struct page **)new_kmem_zalloc(
						pl_alloc_sz, KMEM_SLEEP);
			plsz = len;
		} else
			plsz = PVN_GETPAGE_SZ;

		vp_off = off;
		vp_len = len;

		if (app == NULL)
			dogetpage = 1;
		else if (len <= PAGESIZE)
			dogetpage = (*app == NULL);	/* inline non_anon() */
		else
			dogetpage = non_anon(app, &vp_off, &vp_len);

		if (dogetpage) {
			enum seg_rw arw;

			/*
			 * Need to get some non-anonymous pages.
			 * We need to make only one call to GETPAGE to do
			 * this to prevent certain deadlocking conditions
			 * when we are doing locking.  In this case
			 * non_anon() should have picked up the smallest
			 * range which includes all the non-anonymous
			 * pages in the requested range.  We have to
			 * be careful regarding which rw flag to pass in
			 * because on a private mapping, the underlying
			 * object is never allowed to be written.
			 */
			if (rw == S_WRITE && svd->type == MAP_PRIVATE) {
				arw = S_READ;
			} else {
				arw = rw;
			}
			trace3(TR_SEG_GETPAGE, seg, addr, TRC_SEG_FILE);
			err = VOP_GETPAGE(svd->vp, vp_off, vp_len, &vpprot,
			    plp, plsz, seg, addr + (vp_off - off), arw,
			    svd->cred);
			if (err) {
				if (svd->amp != NULL)
					AMAP_UNLOCK(svd->amp);
				segvn_pagelist_rele(plp);
				if (pl_alloc_sz)
					kmem_free((caddr_t)plp, pl_alloc_sz);
				return (FC_MAKE_ERR(err));
			}
			if (svd->type == MAP_PRIVATE)
				vpprot &= ~PROT_WRITE;
		}
	}

	/*
	 * If MADV_SEQUENTIAL has been set for the particular page we
	 * are faulting on, free behind all pages in the segment and put
	 * them on the free list.
	 */
	if ((page > 0) && fltadvice) {	/* not if first page in segment */
		struct vpage *vpp;
		register u_int pgoff;
		int fpage;
		u_int fpgoff;
		struct vnode *fvp;
		struct anon **fap = NULL;
		register int s;

		if (svd->advice == MADV_SEQUENTIAL ||
		    (svd->pageadvice &&
		    vpage->vp_advice == MADV_SEQUENTIAL)) {
			pgoff = off - PAGESIZE;
			fpage = page - 1;
			if (vpage)
				vpp = &svd->vpage[fpage];
			if (svd->amp)
				fap = &svd->amp->anon[svd->anon_index + fpage];

			while (pgoff > svd->offset) {
				if (svd->advice != MADV_SEQUENTIAL ||
				    (!svd->pageadvice && (vpage &&
				    vpp->vp_advice != MADV_SEQUENTIAL)))
					break;

				/*
				 * if this is an anon page, we must find the
				 * correct <vp, offset> for it
				 */

				if (svd->amp && *fap)
					swap_xlate(*fap, &fvp, &fpgoff);
				else {
					fpgoff = pgoff;
					fvp = svd->vp;
				}

				s = splvm();
				pp = page_exists(fvp, fpgoff);
				if (pp != NULL &&
				    (!(pp->p_free || pp->p_intrans))) {

					/*
					 * we should build a page list
					 * to kluster putpages XXX
					 */
					(void) splx(s);
					(void)VOP_PUTPAGE(fvp, fpgoff, PAGESIZE,
					    (B_DONTNEED|B_FREE|B_ASYNC|B_DELWRI),
					    svd->cred);
					--vpp, --fap;
					pgoff -= PAGESIZE;
				} else {
					(void) splx(s);
					break;
				}
			}
		}
	}

	/*
	 * N.B. at this time the plp array has all the needed non-anon
	 * pages in addition to (possibly) having some adjacent pages.
	 */

	/*
	 * Ok, now loop over the address range and handle faults
	 */
	for (a = addr; a < addr + len; a += PAGESIZE, off += PAGESIZE) {
		err = segvn_faultpage(seg, a, off, app, vpage, plp, vpprot,
		    type, rw);
		if (err) {
			if (svd->amp != NULL)
				AMAP_UNLOCK(svd->amp);
			if (type == F_SOFTLOCK && a > addr)
				segvn_softunlock(seg, addr, (u_int)(a - addr),
				    S_OTHER);
			segvn_pagelist_rele(plp);
			if (pl_alloc_sz)
				kmem_free((caddr_t)plp, pl_alloc_sz);
			return (err);
		}
		if (app)
			app++;
		if (vpage)
			vpage++;
	}

	/*
	 * Now handle any other pages in the list returned.
	 * If the page can be used, load up the translations now.
	 * Note that the for loop will only be entered if "plp"
	 * is pointing to a non-NULL page pointer which means that
	 * VOP_GETPAGE() was called and vpprot has been initialized.
	 */
	if (svd->pageprot == 0)
		prot = svd->prot & vpprot;
	for (ppp = plp; (pp = *ppp) != NULL; ppp++) {
		int diff;

		if (pp == PAGE_HANDLED)
			continue;

		diff = pp->p_offset - svd->offset;
		if (diff >= 0 && diff < seg->s_size) {
			ASSERT(svd->vp == pp->p_vnode);

			page = btop(diff);
			if (svd->pageprot)
				prot = svd->vpage[page].vp_prot & vpprot;

			if (svd->amp == NULL ||
			    svd->amp->anon[svd->anon_index + page] == NULL) {
				hat_memload(seg, seg->s_base + diff, pp,
				    prot, 0);
			}
		}
		PAGE_RELE(pp);
	}
	if (svd->amp != NULL)
		AMAP_UNLOCK(svd->amp);

	if (pl_alloc_sz)
		kmem_free((caddr_t)plp, pl_alloc_sz);
	return (0);
}

/*
 * This routine is used to start I/O on pages asynchronously.
 */
static faultcode_t
segvn_faulta(seg, addr)
	struct seg *seg;
	addr_t addr;
{
	register struct segvn_data *svd = (struct segvn_data *)seg->s_data;
	register struct anon **app;
	int err;

	if (svd->amp != NULL) {
		AMAP_LOCK(svd->amp);
		app = &svd->amp->anon[svd->anon_index + seg_page(seg, addr)];
		if (*app != NULL) {
			err = anon_getpage(app, (u_int *)NULL,
			    (struct page **)NULL, 0, seg, addr, S_READ,
			    svd->cred);
			AMAP_UNLOCK(svd->amp);
			if (err)
				return (FC_MAKE_ERR(err));
			return (0);
		}
		AMAP_UNLOCK(svd->amp);
	}

	if (svd->vp == NULL)
		return (0);			/* zfod page - do nothing now */

	trace3(TR_SEG_GETPAGE, seg, addr, TRC_SEG_FILE);
	err = VOP_GETPAGE(svd->vp, svd->offset + (addr - seg->s_base),
	    PAGESIZE, (u_int *)NULL, (struct page **)NULL, 0, seg, addr,
	    S_OTHER, svd->cred);
	if (err)
		return (FC_MAKE_ERR(err));
	return (0);
}

static
segvn_hatsync(seg, addr, ref, mod, flags)
	struct seg *seg;
	addr_t addr;
	u_int ref, mod;
	u_int flags;
{
	register struct segvn_data *svd = (struct segvn_data *)seg->s_data;
	register int page;
	register struct vpage *vpage;
	register struct anon *ap;

	/* For now, only ski allocates vpage structures, on demand */
	if (svd->vpage == NULL && seg->s_as->a_ski) {
		svd->vpage = (struct vpage *)
			new_kmem_zalloc(vpgtob(seg_pages(seg)), KMEM_SLEEP);
	}
	if (svd->vpage == NULL)
		return;

	page = seg_page(seg, addr);
	vpage = &svd->vpage[page];
	if (flags & AHAT_UNLOAD) {
		if (svd->amp && (ap = svd->amp->anon[svd->anon_index + page]))
			anon_unloadmap(ap, ref, mod);
		else
			pvn_unloadmap(svd->vp,
				svd->offset + (addr - seg->s_base), ref, mod);
	}
	vpage->vp_mod |= mod;
	vpage->vp_ref |= ref;

	/* Ski keeps separate vpage ref and mod info */
	if (seg->s_as->a_ski) {
		vpage->vp_ski_mod |= mod;
		vpage->vp_ski_ref |= ref;
	}
}

static int
segvn_setprot(seg, addr, len, prot)
	register struct seg *seg;
	register addr_t addr;
	register u_int len, prot;
{
	register struct segvn_data *svd = (struct segvn_data *)seg->s_data;
	register struct vpage *vp, *evp;

	if ((svd->maxprot & prot) != prot)
		return (-1);				/* violated maxprot */

	/*
	 * If it's a private mapping and we're making it writable
	 * and no swap space has been reserved, have to reserve
	 * it all now.  If it's private and we're removing write
	 * permission on the entire segment and we haven't modified
	 * any pages, we can release the swap space.
	 */
	if (svd->type == MAP_PRIVATE) {
		if (prot & PROT_WRITE) {
			if (svd->swresv == 0) {
				if (anon_resv(seg->s_size) == 0)
					return (-1);
				svd->swresv = seg->s_size;
			}
		} else {
			if (svd->swresv != 0 && svd->amp == NULL &&
			    addr == seg->s_base && len == seg->s_size &&
			    svd->pageprot == 0) {
				anon_unresv(svd->swresv);
				svd->swresv = 0;
			}
		}
	}

	if (addr == seg->s_base && len == seg->s_size && svd->pageprot == 0) {
		if (svd->prot == prot)
			return (0);			/* all done */
		svd->prot = prot;
	} else {
		/*
		 * A vpage structure exists or else the change does not
		 * involve the entire segment.  Establish a vpage structure
		 * if none is there.  Then, for each page in the range,
		 * adjust its individual permissions.  Note that write-
		 * enabling a MAP_PRIVATE page can affect the claims for
		 * locked down memory.  Overcommitting memory terminates
		 * the operation.
		 */
		segvn_vpage(seg);
		evp = &svd->vpage[seg_page(seg, addr + len)];
		for (vp = &svd->vpage[seg_page(seg, addr)]; vp < evp; vp++) {
			if (vp->vp_pplock && (svd->type == MAP_PRIVATE)) {
				if ((vp->vp_prot ^ prot) & PROT_WRITE)
					if (prot & PROT_WRITE) {
						if (!page_addclaim(1))
							break;
					} else
						page_subclaim(1);
			}
			vp->vp_prot = prot;
		}

		/*
		 * Did we terminate prematurely?  If so, simply unload
		 * the translations to the things we've updated so far.
		 */
		if (vp != evp) {
			len = (vp - &svd->vpage[seg_page(seg, addr)]) *
			    PAGESIZE;
			if (len != 0)
				hat_unload(seg, addr, len);
			return (-1);
		}
	}

	if (((prot & PROT_WRITE) != 0) || ((prot & ~PROT_USER) == PROT_NONE)) {
		/*
		 * Either private data with write access (in which case
		 * we need to throw out all former translations so that
		 * we get the right translations set up on fault and we
		 * don't allow write access to any copy-on-write pages
		 * that might be around) or we don't have permission
		 * to access the memory at all (in which case we have to
		 * unload any current translations that might exist).
		 */
		hat_unload(seg, addr, len);
	} else {
		/*
		 * A shared mapping or a private mapping in which write
		 * protection is going to be denied - just change all the
		 * protections over the range of addresses in question.
		 */
		hat_chgprot(seg, addr, len, prot);
	}

	return (0);
}

static int
segvn_checkprot(seg, addr, len, prot)
	register struct seg *seg;
	register addr_t addr;
	register u_int len, prot;
{
	struct segvn_data *svd = (struct segvn_data *)seg->s_data;
	register struct vpage *vp, *evp;

	/*
	 * If segment protection can be used, simply check against them.
	 */
	if (svd->pageprot == 0)
		return (((svd->prot & prot) != prot) ? -1 : 0);

	/*
	 * Have to check down to the vpage level.
	 */
	evp = &svd->vpage[seg_page(seg, addr + len)];
	for (vp = &svd->vpage[seg_page(seg, addr)]; vp < evp; vp++)
		if ((vp->vp_prot & prot) != prot)
			return (-1);

	return (0);
}

/*
 * Check to see if it makes sense to do kluster/read ahead to
 * addr + delta relative to the mapping at addr.  We assume here
 * that delta is a signed PAGESIZE'd multiple (which can be negative).
 *
 * For segvn, we currently "approve" of the action if we are
 * still in the segment and it maps from the same vp/off,
 * or if the advice stored in segvn_data or vpages allows it.
 * Currently, klustering is not allowed only if MADV_RANDOM is set.
 */
static int
segvn_kluster(seg, addr, delta)
	register struct seg *seg;
	register addr_t addr;
	int delta;
{
	register struct segvn_data *svd = (struct segvn_data *)seg->s_data;
	register struct anon *oap, *ap;
	register struct vpage *bvpp, *evpp, *tvpp;
	register int pd;
	register u_int page;
	struct vnode *vp1, *vp2;
	u_int off1, off2;

	if (addr + delta < seg->s_base ||
	    addr + delta >= (seg->s_base + seg->s_size))
		return (-1);		/* exceeded segment bounds */

	pd = delta >> PAGESHIFT;	/* assumes that sign bit is preserved */
	page = seg_page(seg, addr);

	/*
	 * Check to see if any of the pages in the range have advice
	 * set that prevents klustering
	 */
	switch (svd->advice) {	/* if advice set for entire segment */
	case MADV_RANDOM:
		return (-1);
	case MADV_SEQUENTIAL:
	case MADV_NORMAL:
	case MADV_WILLNEED:
	case MADV_DONTNEED:
	default:
		break;
	}

	if (svd->pageadvice && svd->vpage) {
		bvpp = &svd->vpage[page];
		evpp = &svd->vpage[page + pd];
		if (evpp < bvpp) {	/* searching backwards */
			tvpp = bvpp;
			bvpp = evpp;
			evpp = tvpp;
		}
		for (; bvpp < evpp; bvpp++) {
			switch (bvpp->vp_advice) {
			case MADV_RANDOM:
				return (-1);
			case MADV_SEQUENTIAL:
			case MADV_NORMAL:
			case MADV_WILLNEED:
			case MADV_DONTNEED:
			default:
				break;
			}
		}
	}

	if (svd->type == MAP_SHARED)
		return (0);		/* shared mapping - all ok */

	if (svd->amp == NULL)
		return (0);		/* off original vnode */

	page += svd->anon_index;
	oap = svd->amp->anon[page];
	ap = svd->amp->anon[page + pd];

	if ((oap == NULL && ap != NULL) || (oap != NULL && ap == NULL))
		return (-1);		/* one with and one without an anon */

	if (oap == NULL)		/* implies that ap == NULL */
		return (0);		/* off original vnode */

	/*
	 * Now we know we have two anon pointers - check to
	 * see if they happen to be properly allocated.
	 */
	swap_xlate(ap, &vp1, &off1);
	swap_xlate(oap, &vp2, &off2);
	if (!VOP_CMP(vp1, vp2) || off1 - off2 != delta)
		return (-1);
	return (0);
}

/*
 * Swap the pages of seg out to secondary storage, returning the
 * number of bytes of storage freed.
 *
 * The basic idea is first to unload all translations and then to call
 * VOP_PUTPAGE for all newly-unmapped pages, to push them out to the
 * swap device.  Pages to which other segments have mappings will remain
 * mapped and won't be swapped.  Our caller (as_swapout) has already
 * performed the unloading step.
 *
 * The value returned is intended to correlate well with the process's
 * memory requirements.  However, there are some caveats:
 * 1)	When given a shared segment as argument, this routine will
 *	only succeed in swapping out pages for the last sharer of the
 *	segment.  (Previous callers will only have decremented mapping
 *	reference counts.)
 * 2)	We assume that the hat layer maintains a large enough translation
 *	cache to capture process reference patterns.
 */
static u_int
segvn_swapout(seg)
	struct seg *seg;
{
	struct segvn_data *svd = (struct segvn_data *)seg->s_data;
	struct anon_map *amp = svd->amp;
	register u_int pgcnt = 0;
	u_int npages;
	register u_int page;

	/*
	 * Find pages unmapped by our caller and force them
	 * out to the virtual swap device.
	 */
	npages = seg->s_size >> PAGESHIFT;
	if (amp != NULL)
		AMAP_LOCK(amp);
	for (page = 0; page < npages; page++) {
		register struct page *pp;
		register struct anon **app;
		struct vnode *vp;
		u_int off;
		register int s;

		/*
		 * Obtain <vp, off> pair for the page, then look it up.
		 *
		 * Note that this code is willing to consider regular
		 * pages as well as anon pages.  Is this appropriate here?
		 */
		if (amp != NULL &&
		    *(app = &amp->anon[svd->anon_index + page]) != NULL) {
			swap_xlate(*app, &vp, &off);
		} else {
			off = svd->offset + ptob(page);
			vp = svd->vp;
		}
		s = splvm();
		if ((pp = page_exists(vp, off)) == NULL || pp->p_free) {
			(void) splx(s);
			continue;
		}
		(void) splx(s);

		/*
		 * Skip if page is logically unavailable for removal.
		 */
		if (pp->p_lckcnt != 0)
			continue;

		/*
		 * Examine the page to see whether it can be tossed out,
		 * keeping track of how many we've found.
		 */
		if (pp->p_keepcnt != 0) {
			/*
			 * If the page is marked as in transit going out
			 * and has no mappings, it's very likely that
			 * the page is in transit because of klustering.
			 * Assume this is so and take credit for it here.
			 */
			if (pp->p_intrans && !pp->p_pagein && !pp->p_mapping)
				pgcnt++;
			continue;
		}

		if (pp->p_mapping != NULL)
			continue;

		/*
		 * Since the keepcnt was 0 the page should not be
		 * in a gone state nor is not directly or indirectly
		 * involved in any IO at this time.
		 */

		/*
		 * No longer mapped -- we can toss it out.  How
		 * we do so depends on whether or not it's dirty.
		 *
		 * XXX:	Need we worry about locking between the
		 * time of the hat_pagesync call and the actions
		 * that depend on its result?
		 */
		hat_pagesync(pp);
		if (pp->p_mod && pp->p_vnode) {
			/*
			 * We must clean the page before it can be
			 * freed.  Setting B_FREE will cause pvn_done
			 * to free the page when the i/o completes.
			 * XXX:	This also causes it to be accounted
			 *	as a pageout instead of a swap: need
			 *	B_SWAPOUT bit to use instead of B_FREE.
			 */
			(void) VOP_PUTPAGE(vp, off, PAGESIZE, B_ASYNC | B_FREE,
			    svd->cred);
		} else {
			/*
			 * The page was clean.  Lock it and free it.
			 *
			 * XXX:	Can we ever encounter modified pages
			 *	with no associated vnode here?
			 */
			page_lock(pp);
			page_free(pp, 0);
		}

		/*
		 * Credit now even if i/o is in progress.
		 */
		pgcnt++;
	}
	if (amp != NULL)
		AMAP_UNLOCK(amp);

	return (ptob(pgcnt));
}

/*
 * Synchronize primary storage cache with real object in virtual memory.
 */
static int
segvn_sync(seg, addr, len, flags)
	struct seg *seg;
	register addr_t addr;
	u_int len;
	u_int flags;
{
	register struct segvn_data *svd = (struct segvn_data *)seg->s_data;
	register struct anon **app;
	register u_int offset;
	struct vpage *vpp = svd->vpage;
	addr_t eaddr;
	int bflags;
	int err;
	int new_len;
	int lock_error = 0;

	if (svd->vp == NULL)
		return (0);	/* all anonymous memory - nothing to do */

	offset = svd->offset + (addr - seg->s_base);
	bflags = B_FORCE | ((flags & MS_ASYNC) ? B_ASYNC : 0) |
	    ((flags & MS_INVALIDATE) ? B_INVAL : 0);

	/*
	 * See if any of these pages are locked --  if so, then we
	 * will have to truncate an invalidate request at the first
	 * locked one.
	 */
	if ((flags & MS_INVALIDATE) && vpp)
		for (new_len = 0; new_len < len; new_len += PAGESIZE)
			if ((vpp++)->vp_pplock) {

				/*
				 * A page is locked.  Reset the length
				 * of this operation.  If the result is
				 * zero, simply return now.
				 */
				lock_error = EPERM;
				if ((len = new_len) == 0)
					return (lock_error);
				break;
			}

	if (svd->amp == NULL) {
		/*
		 * No anonymous pages, just use one big request.
		 */
		err = VOP_PUTPAGE(svd->vp, offset, len, bflags, svd->cred);
	} else {
		err = 0;
		AMAP_LOCK(svd->amp);
		app = &svd->amp->anon[svd->anon_index + seg_page(seg, addr)];

		for (eaddr = addr + len; addr < eaddr;
		    addr += PAGESIZE, offset += PAGESIZE) {
			if (*app++ != NULL)
				continue;	/* don't sync anonymous pages */

			/*
			 * XXX - Should ultimately try to kluster
			 * calls to VOP_PUTPAGE for performance.
			 */
			err = VOP_PUTPAGE(svd->vp, offset, PAGESIZE, bflags,
			    svd->cred);
			if (err)
				break;
		}
		AMAP_UNLOCK(svd->amp);
	}
	return (err ? err : lock_error);
}

/*
 * Determine if we have data corresponding to pages in the
 * primary storage virtual memory cache (i.e., "in core").
 * N.B. Assumes things are "in core" if page structs exist.
 */
static int
segvn_incore(seg, addr, len, vec)
	struct seg *seg;
	addr_t addr;
	u_int len;
	char *vec;
{
	register struct segvn_data *svd = (struct segvn_data *)seg->s_data;
	register struct anon **app;
	struct vnode *vp;
	u_int offset;
	u_int p = seg_page(seg, addr);
	u_int ep = seg_page(seg, addr + len);
	u_int v;

	if (svd->amp != NULL)
		AMAP_LOCK(svd->amp);
	for (v = 0; p < ep; p++, addr += PAGESIZE, v += PAGESIZE) {
		if ((svd->amp != NULL) &&
		    (*(app = &svd->amp->anon[svd->anon_index + p]) != NULL)) {
			swap_xlate(*app, &vp, &offset);
			*vec++ = (page_exists(vp, offset) != NULL);
		} else if (vp = svd->vp) {
			offset = svd->offset + (addr - seg->s_base);
			*vec++ = (page_exists(vp, offset) != NULL);
		} else {
			*vec++ = 0;
		}
	}
	if (svd->amp != NULL)
		AMAP_UNLOCK(svd->amp);
	return (v);
}

/*
 * Lock down (or unlock) pages mapped by this segment.
 */
static int
segvn_lockop(seg, addr, len, op)
	struct seg *seg;
	addr_t addr;
	u_int len;
	int op;
{
	struct segvn_data *svd = (struct segvn_data *)seg->s_data;
	register struct vpage *vpp = svd->vpage;
	register struct vpage *evp;
	struct page *pp;
	struct anon **app;
	u_int offset;
	u_int off;
	int claim;
	int err;
	struct vnode *vp;

	/*
	 * If we're locking, then we must create a vpage structure if
	 * none exists.  If we're unlocking, then check to see if there
	 * is a vpage -- if not, then we could not have locked anything.
	 */
	if (vpp == NULL)
		if (op == MC_LOCK)
			segvn_vpage(seg);
		else
			return (0);

	/*
	 * Set up bounds for looping over the range of pages.  Guard
	 * against lazy creation of the anonymous data vector (i.e.,
	 * previously unreferenced mapping to swap space) by lazily
	 * testing for its existence.
	 */
	app = NULL;
	offset = svd->offset + (addr - seg->s_base);
	evp = &svd->vpage[seg_page(seg, addr + len)];

	/*
	 * Loop over all pages in the range.  Process if we're locking and
	 * page has not already been locked in this mapping; or if we're
	 * unlocking and the page has been locked.
	 */
	for (vpp = &svd->vpage[seg_page(seg, addr)]; vpp < evp; vpp++) {
		if (((op == MC_LOCK) && (!vpp->vp_pplock)) ||
		    ((op == MC_UNLOCK) && (vpp->vp_pplock))) {

			/*
			 * If we're locking, softfault the page in memory.
			 */
			if (op == MC_LOCK)
				if (segvn_fault(seg, addr, PAGESIZE,
				    F_SOFTLOCK, S_OTHER) != 0)
					return (EIO);

			/*
			 * Check for lazy creation of anonymous
			 * data vector.
			 */
			if (app == NULL)
				if (svd->amp != NULL)
					app = &svd->amp->anon[svd->anon_index +
					    seg_page(seg, addr)];

			/*
			 * Get name for page, accounting for
			 * existence of private copy.
			 */
			if (app != NULL && *app != NULL) {
				AMAP_LOCK(svd->amp);
				swap_xlate(*app, &vp, &off);
				AMAP_UNLOCK(svd->amp);
				claim = 0;
			} else {
				vp = svd->vp;
				off = offset;
				claim = ((vpp->vp_prot & PROT_WRITE) != 0) &&
				    (svd->type == MAP_PRIVATE);
			}

			/*
			 * Get page frame.  It's ok if the page is
			 * not available when we're unlocking, as this
			 * may simply mean that a page we locked got
			 * truncated out of existence after we locked it.
			 */
			if ((pp = page_lookup(vp, off)) == NULL)
				if (op == MC_LOCK)
					panic("segvn_lockop: no page");

			/*
			 * Perform page-level operation appropriate to
			 * operation.  If locking, undo the SOFTLOCK
			 * performed to bring the page into memory
			 * after setting the lock.  If unlocking,
			 * and no page was found, account for the claim
			 * separately.
			 */
			if (op == MC_LOCK) {
				err = page_pp_lock(pp, claim, 1);
				(void) segvn_fault(seg, addr, PAGESIZE,
				    F_SOFTUNLOCK, S_OTHER);
				if (!err)
					return (EAGAIN);
				vpp->vp_pplock = 1;
			} else {
				if (pp)
					page_pp_unlock(pp, claim);
				else
					page_subclaim(claim);
				vpp->vp_pplock = 0;
			}
		}
		addr += PAGESIZE;
		offset += PAGESIZE;
		if (app)
			app++;
	}
	return (0);
}

/*
 * Set advice from user for specified pages
 * There are 5 types of advice:
 *	MADV_NORMAL	- Normal (default) behavior (whatever that is)
 *	MADV_RANDOM	- Random page references
 *				do not allow readahead or 'klustering'
 *	MADV_SEQUENTIAL	- Sequential page references
 *				Pages previous to the one currently being
 *				accessed (determined by fault) are 'not needed'
 *				and are freed immediately
 *	MADV_WILLNEED	- Pages are likely to be used (fault ahead in mctl)
 *	MADV_DONTNEED	- Pages are not needed (synced out in mctl)
 */
static
segvn_advise(seg, addr, len, behav)
	struct seg *seg;
	addr_t addr;
	u_int len;
	int behav;
{
	struct segvn_data *svd = (struct segvn_data *)seg->s_data;
	int page;

	/*
	 * If advice is to be applied to entire segment,
	 * use advice field in seg_data structure
	 * otherwise use appropriate vpage entry.
	 */

	if ((addr == seg->s_base) && (len == seg->s_size)) {
		switch (behav) {
		case MADV_SEQUENTIAL:
	/* unloading mapping guarantees detection in segvn_fault */
			hat_unload(seg, addr, len);
		case MADV_NORMAL:
		case MADV_RANDOM:
			svd->advice = behav;
			svd->pageadvice = 0;
			break;
		case MADV_WILLNEED:
		case MADV_DONTNEED:
			break;	/* handled in mctl */
		default:
			return (EINVAL);
		}
		return (0);
	} else
		svd->advice = -1;  /* convenience to check if advice set */

	page = seg_page(seg, addr);

	if ((svd->vpage) == NULL)
		segvn_vpage(seg);

	switch (behav) {
		register struct vpage *bvpp, *evpp;

	case MADV_SEQUENTIAL:
		hat_unload(seg, addr, len);
	case MADV_NORMAL:
	case MADV_RANDOM:
		bvpp = &svd->vpage[page];
		evpp = &svd->vpage[page + (len >> PAGESHIFT)];
		for (; bvpp < evpp; bvpp++)
			bvpp->vp_advice = behav;
		svd->pageadvice = 1;
		break;
	case MADV_WILLNEED:
	case MADV_DONTNEED:
		break;
	default:
		return (EINVAL);
	}
	return (0);
}

/*
 * Create a vpage structure for this seg.
 */
static void
segvn_vpage(seg)
	struct seg *seg;
{
	register struct segvn_data *svd = (struct segvn_data *)seg->s_data;
	register struct vpage *vp, *evp;

	/*
	 * If no vpage structure exists, allocate one.  Copy the protections
	 * from the segment itself to the individual pages.
	 */
	if (svd->vpage == NULL) {
		svd->pageprot = 1;
		svd->vpage = (struct vpage *)
		    new_kmem_zalloc((u_int)vpgtob(seg_pages(seg)), KMEM_SLEEP);
		evp = &svd->vpage[seg_page(seg, seg->s_base + seg->s_size)];
		for (vp = svd->vpage; vp < evp; vp++)
			vp->vp_prot = svd->prot;
	}
}
