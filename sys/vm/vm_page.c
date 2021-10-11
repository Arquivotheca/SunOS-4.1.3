/*	@(#)vm_page.c	1.1 92/07/30	SMI	*/

/*
 * Copyright (c) 1988, 1989 by Sun Microsystems, Inc.
 */

/*
 * VM - physical page management.
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/proc.h>		/* XXX - needed for outofmem() */
#include <sys/vm.h>
#include <sys/trace.h>
#include <sys/debug.h>

#include <vm/hat.h>
#include <vm/anon.h>
#include <vm/page.h>
#include <vm/seg.h>
#include <vm/swap.h>
#include <vm/pvn.h>
#include <vm/mp.h>

#ifdef SUN4_110
/* XXX this needs to be replaced by a more general mechanism */
#include <machine/cpu.h>
#include <machine/trap.h>

extern int fault_type;
#endif

#ifdef PAGE_DEBUG
int do_checks = 0;
int do_check_vp = 1;
int do_check_free = 1;
int do_check_list = 1;
int do_check_pp = 1;

#define	CHECK(vp)	if (do_checks && do_check_vp) page_vp_check(vp)
#define	CHECKFREE()	if (do_checks && do_check_free) page_free_check()
#define	CHECKLIST(pp)	if (do_checks && do_check_list) page_list_check(pp)
#define	CHECKPP(pp)	if (do_checks && do_check_pp) page_pp_check(pp)

#else PAGE_DEBUG

#define	CHECK(vp)
#define	CHECKFREE()
#define	CHECKLIST(pp)
#define	CHECKPP(pp)

#endif PAGE_DEBUG

/*
 * Set to non-zero to avoid reclaiming pages which are
 * busy being paged back until the IO and completed.
 */
int nopagereclaim = 0;

/*
 * The logical page free list is maintained as two physical lists.
 * The free list contains those pages that should be reused first.
 * The cache list contains those pages that should remain unused as
 * long as possible so that they might be reclaimed.
 *
 * The implementation of the free list is machine dependent.
 * page_freelist_add(), page_freelist_sub(), get_page_freelist()
 * are the interface to the machine dependent implementation.
 */
extern	struct	page **get_page_freelist();
static	struct	page *page_cachelist;	/* cache list of free pages */

static	void page_hashin(/* pp, vp, offset, lock */);

int	page_freelist_size;		/* size of free list */
int	page_cachelist_size;		/* size of cache list */

#ifdef KMON_DEBUG
static	kmon_t	page_mplock;		/* lock for manipulating page links */
static	kmon_t	page_freelock;		/* lock for manipulating free list */
#endif /* KMON_DEBUG */

struct	page *pages;			/* array of all page structures */
struct	page *epages;			/* end of all pages */
u_int total_pages = 0;			/* total number of pages */
struct	memseg *memsegs;		/* list of memory segments */
static struct memseg *last_memseg;	/* cache of last segment accessed */

u_int	max_page_get;			/* max page_get request size in pages */
u_int	freemem_wait;			/* someone waiting for freemem */

u_int	pages_pp_locked = 0;		/* physical pages actually locked */
u_int	pages_pp_claimed = 0;		/* physical pages reserved */
u_int	pages_pp_maximum = 0;		/* lock + claim <= max */
static	u_int pages_pp_factor = 10;	/* divisor for unlocked percentage */
#ifdef KMON_DEBUG
static	kmon_t page_locklock;		/* mutex on locking variables */
#endif /* KMON_DEBUG */
#define	PAGE_LOCK_MAXIMUM \
	((1 << (sizeof (((struct page *)0)->p_lckcnt) * NBBY)) - 1)

struct pagecnt {
	int	pc_free_cache;		/* free's into cache list */
	int	pc_free_dontneed;	/* free's with dontneed */
	int	pc_free_pageout;	/* free's from pageout */
	int	pc_free_free;		/* free's into free list */
	int	pc_get_cache;		/* get's from cache list */
	int	pc_get_free;		/* get's from free list */
	int	pc_reclaim;		/* reclaim's */
	int	pc_abortfree;		/* abort's of free pages */
	int	pc_find_hit;		/* find's that find page */
	int	pc_find_miss;		/* find's that don't find page */
#define	PC_HASH_CNT	(2*PAGE_HASHAVELEN)
	int	pc_find_hashlen[PC_HASH_CNT+1];
} pagecnt;

/*
 * Initialize the physical page structures.
 * Since we cannot call the dynamic memory allocator yet,
 * we have startup() allocate memory for the page
 * structs, hash tables and memseg struct for us.
 * num is the number of page structures and base is the
 * physical page number to be associated with the first page.
 *
 * NOTE:  we insist on being called with increasing (but not necessarily
 * contiguous) physical addresses and with contiguous page structures.
 */
void
page_init(pp, num, base, memseg)
	register struct page *pp;
	u_int num, base;
	register struct memseg *memseg;
{
	register struct memseg *tseg;

	/*
	 * XXX:	Should probably trace additional information.
	 */
	trace2(TR_PG_INIT, pp, num);

	/*
	 * Store away info in globals.  These can be used to walk the
	 * page struct array linearly since all the page structures in
	 * the system are allocated contiguously, although they can
	 * represent discontiguous physical pages.
	 */
	if (pages == (struct page *)NULL)
		pages = pp;
	epages = &pp[num];

	/*
	 * Add this segment to the list of physical memory.
	 */
	if (memsegs == (struct memseg *)NULL)
		memsegs = memseg;
	else {
		for (tseg = memsegs; tseg->next != (struct memseg *)NULL;
		    tseg = tseg->next)
			;
		ASSERT(tseg->pages_base < base);
		ASSERT(tseg->epages == pp);
		tseg->next = memseg;
	}
	memseg->next = (struct memseg *)NULL;
	/*
	 * Initialize the segment cache.
	 */
	last_memseg = memseg;
	/*
	 * Fill in segment info.
	 */
	memseg->pages = pp;
	memseg->epages = &pp[num];
	memseg->pages_base = base;
	memseg->pages_end = base + num * (PAGESIZE/MMU_PAGESIZE);

	/*
	 * Arbitrarily limit the max page_get request
	 * to 1/2 of the page structs we have.
	 */
	total_pages += num;
	max_page_get = total_pages / 2;

	/*
	 * The physical space for the pages array
	 * representing ram pages have already been
	 * allocated.  Here we mark all the pages as
	 * locked.  Later calls to page_free() will
	 * make the pages available.
	 */
	for (; pp < epages; pp++)
		pp->p_lock = 1;

	/*
	 * Determine the number of pages that can be pplocked.  This
	 * is the number of page frames less the maximum that can be
	 * taken for buffers less another percentage.  The percentage should
	 * be a tunable parameter, and in SVR4 should be one of the "tune"
	 * structures.
	 */
	pages_pp_maximum = total_pages;
	pages_pp_maximum -= (btop(nbuf * MAXBSIZE) + 1) +
	    (pages_pp_maximum / pages_pp_factor);

	/*
	 * Verify that the hashing stuff has been initialized elsewhere.
	 */
	if (page_hash == NULL || page_hashsz == 0)
		panic("page_init");

#ifdef PAGE_DEBUG
	if (do_checks)
		page_print(pp);
#endif	/* PAGE_DEBUG */
}

/*
 * Use kcv_wait() to wait for the given page.  It is assumed that
 * the page_mplock is already locked upon entry and this lock
 * will continue to be held upon return.
 *
 * NOTE:  This routine must be called at splvm() and the caller must
 *	  re-verify the page identity.
 */
static void
page_cv_wait(pp)
	struct page *pp;
{
	register int s;

	pp->p_want = 1;
	kcv_wait(&page_mplock, (char *)pp);

	/*
	 * We may have been awakened from swdone,
	 * in which case we must clean up the i/o
	 * list before being able to use the page.
	 */
	kmon_exit(&page_mplock);
	if (bclnlist != NULL) {
		s = spl0();
		cleanup();
		(void) splx(s);
	}
	kmon_enter(&page_mplock);
}

/*
 * Reclaim the given page from the free list to vp list.
 */
void
page_reclaim(pp)
	register struct page *pp;
{
	register int s;
	register struct anon *ap;

	s = splvm();
	kmon_enter(&page_freelock);

	if (pp->p_free) {
#ifdef	TRACE
		register int age = pp->p_age;

		ap = NULL;
#endif	TRACE
		page_unfree(pp);
		pagecnt.pc_reclaim++;
		if (pp->p_vnode) {
			cnt.v_pgrec++;
			cnt.v_pgfrec++;

			if (ap = swap_anon(pp->p_vnode, pp->p_offset)) {
				if (ap->un.an_page == NULL && ap->an_refcnt > 0)
					ap->un.an_page = pp;
				cnt.v_xsfrec++;
			} else
				cnt.v_xifrec++;
			CHECK(pp->p_vnode);
		}

		trace6(TR_PG_RECLAIM, pp, pp->p_vnode, pp->p_offset,
			ap, age, freemem);
	}

	kmon_exit(&page_freelock);
	(void) splx(s);
}

/*
 * Search the hash list for a page with the specified <vp, off> and
 * then reclaim it if found on the free list.
 */
struct page *
page_find(vp, off)
	register struct vnode *vp;
	register u_int off;
{
	register struct page *pp;
	register int len = 0;
	register int s;

	s = splvm();
	kmon_enter(&page_mplock);
	for (pp = page_hash[PAGE_HASHFUNC(vp, off)]; pp; pp = pp->p_hash, len++)
		if (pp->p_vnode == vp && pp->p_offset == off && pp->p_gone == 0)
			break;
	if (pp)
		pagecnt.pc_find_hit++;
	else
		pagecnt.pc_find_miss++;
	if (len > PC_HASH_CNT)
		len = PC_HASH_CNT;
	pagecnt.pc_find_hashlen[len]++;
	if (pp != NULL && pp->p_free)
		page_reclaim(pp);
	kmon_exit(&page_mplock);
	(void) splx(s);
	return (pp);
}

/*
 * Quick page lookup to merely find if a named page exists
 * somewhere w/o having to worry about which list it is on.
 */
struct page *
page_exists(vp, off)
	register struct vnode *vp;
	register u_int off;
{
	register struct page *pp;
	register int len = 0;
	register int s;

	s = splvm();
	kmon_enter(&page_mplock);
	for (pp = page_hash[PAGE_HASHFUNC(vp, off)]; pp; pp = pp->p_hash, len++)
		if (pp->p_vnode == vp && pp->p_offset == off && pp->p_gone == 0)
			break;
	if (pp)
		pagecnt.pc_find_hit++;
	else
		pagecnt.pc_find_miss++;
	if (len > PC_HASH_CNT)
		len = PC_HASH_CNT;
	pagecnt.pc_find_hashlen[len]++;
	kmon_exit(&page_mplock);
	(void) splx(s);
	return (pp);
}

/*
 * Find a page representing the specified <vp, offset>.
 * If we find the page but it is intransit coming in,
 * we wait for the IO to complete and then reclaim the
 * page if it was found on the free list.
 */
struct page *
page_lookup(vp, off)
	struct vnode *vp;
	u_int off;
{
	register struct page *pp;
	register int s;

again:
	pp = page_find(vp, off);
	if (pp != NULL) {
		/*
		 * Try calling cleanup here to reap the
		 * async buffers queued up for processing.
		 */
		if (pp->p_intrans && pp->p_pagein && bclnlist) {
			cleanup();
			/*
			 * The page could have been freed by cleanup, so
			 * verify the identity after raising priority since
			 * it may be ripped away at interrupt level.  If
			 * we lost it, start all over again.
			 */
			s = splvm();
			if (pp->p_vnode != vp || pp->p_offset != off ||
			    pp->p_gone) {
				(void) splx(s);
				goto again;
			}
		} else
			s = splvm();

		kmon_enter(&page_mplock);
		while (pp->p_lock && pp->p_intrans && pp->p_vnode == vp &&
		    pp->p_offset == off && !pp->p_gone &&
		    (pp->p_pagein || nopagereclaim)) {
			cnt.v_intrans++;
			page_cv_wait(pp);
		}

		/*
		 * If we still have the right page and it is now
		 * on the free list, get it back via page_reclaim.
		 * Note that when a page is on the free list, it
		 * maybe ripped away at interrupt level.  After
		 * we reclaim the page, it cannot be taken away
		 * from us at interrupt level anymore.
		 */
		if (pp->p_vnode == vp && pp->p_offset == off && !pp->p_gone) {
			if (pp->p_free)
				page_reclaim(pp);
		} else {
			kmon_exit(&page_mplock);
			(void) splx(s);
			goto again;
		}
		kmon_exit(&page_mplock);
		(void) splx(s);
	}
	return (pp);
}

/*
 * Enter page ``pp'' in the hash chains and
 * vnode page list as referring to <vp, offset>.
 */
int
page_enter(pp, vp, offset)
	struct page *pp;
	struct vnode *vp;
	u_int offset;
{
	register int v;

	kmon_enter(&page_mplock);

	if (page_exists(vp, offset) != NULL) {
		/* already entered? */
		v = -1;
	} else {
		page_hashin(pp, vp, offset, 1);
		CHECK(vp);

		v = 0;
	}

	kmon_exit(&page_mplock);

	trace4(TR_PG_ENTER, pp, vp, offset, v);

	return (v);
}

/*
 * page_abort will cause a page to lose its
 * identity and to go (back) to the free list.
 */
void
page_abort(pp)
	register struct page *pp;
{

	ASSERT(pp->p_free == 0);

	/* Set the `gone' bit */
	if (pp->p_vnode != NULL)
		pp->p_gone = 1;

	if (pp->p_keepcnt != 0) {
		/*
		 * We cannot do anything with the page now.
		 * page_free will be called later when
		 * the keep count goes back to zero.
		 */
		trace4(TR_PG_ABORT, pp, pp->p_vnode, pp->p_offset, 1);
		return;
	}
	if (pp->p_intrans) {
		/*
		 * Since the page is already `gone', we can
		 * just let pvn_done() worry about freeing
		 * this page later when the IO finishes.
		 */
		trace4(TR_PG_ABORT, pp, pp->p_vnode, pp->p_offset, 2);
		return;
	}

	/*
	 * Page is set to go away -- kill any logical locking.
	 */
	if (pp->p_lckcnt != 0) {
		kmon_enter(&page_locklock);
		pages_pp_locked--;
		pp->p_lckcnt = 0;
		kmon_exit(&page_locklock);
	}

	if (pp->p_mapping) {
		/*
		 * Should be ok to just unload now
		 */
		hat_pageunload(pp);
	}
	pg_setref(pp, 0);
	pg_setmod(pp, 0);
	trace4(TR_PG_ABORT, pp, pp->p_vnode, pp->p_offset, 0);
	/*
	 * Let page_free() do the rest of the work
	 */
	page_free(pp, 0);
}

/*
 * Put page on the "free" list.  The free list is really two circular lists
 * with page_freelist and page_cachelist pointers into the middle of the lists.
 */
int nopageage = 1;
void
page_free(pp, dontneed)
	register struct page *pp;
	int dontneed;
{
	register struct vnode *vp;
	struct anon *ap;
	register int s;

	ASSERT(pp->p_free == 0);
	vp = pp->p_vnode;

	CHECK(vp);

	/*
	 * If we are a swap page, get rid of corresponding
	 * page hint pointer in the anon vector (since it is
	 * easy to do right now) so that we have to find this
	 * page via a page_lookup to force a reclaim.
	 */
	if (ap = swap_anon(pp->p_vnode, pp->p_offset)) {
		if (ap->an_refcnt > 0)
			ap->un.an_page = NULL;
	}

	if (pp->p_gone) {
		if (pp->p_intrans || pp->p_keepcnt != 0) {
			/*
			 * This page will be freed later from pvn_done
			 * (intrans) or the segment unlock routine.
			 * For now, the page will continue to exist,
			 * but with the "gone" bit on.
			 */
			trace6(TR_PG_FREE, pp, vp, pp->p_offset,
				dontneed, freemem, 0);
			return;
		}
		if (vp)
			page_hashout(pp);
		vp = NULL;
	}

	if (pp->p_keepcnt != 0 || pp->p_mapping != NULL ||
	    pp->p_lckcnt != 0)
		panic("page_free");

	s = splvm();
	kmon_enter(&page_freelock);

	/*
	 * Unlock the page before inserting it on the free list.
	 */
	page_unlock(pp);

	/*
	 * Now we add the page to the head of the free list.
	 * But if this page is associated with a paged vnode
	 * then we adjust the head forward so that the page is
	 * effectively at the end of the list.
	 */
	freemem++;
	pp->p_free = 1;
	pp->p_ref = pp->p_mod = 0;
	if (vp == NULL) {
		/* page has no identity, put it on the front of the free list */
		pp->p_age = 1;
		page_freelist_size++;
		page_freelist_add(pp);
		pagecnt.pc_free_free++;
		trace6(TR_PG_FREE, pp, vp, pp->p_offset, dontneed, freemem, 1);
	} else {
		page_cachelist_size++;
		page_add(&page_cachelist, pp);
		if (!dontneed || nopageage) {
			/* move it to the tail of the list */
			page_cachelist = page_cachelist->p_next;
			pagecnt.pc_free_cache++;
			trace6(TR_PG_FREE, pp, vp, pp->p_offset,
				dontneed, freemem, 2);
		} else {
			pagecnt.pc_free_dontneed++;
			trace6(TR_PG_FREE, pp, vp, pp->p_offset,
				dontneed, freemem, 3);
		}
	}

	kmon_exit(&page_freelock);

	CHECK(vp);
	CHECKFREE();

	if (freemem_wait) {
		freemem_wait = 0;
		wakeup((caddr_t)&freemem);
	}
	(void) splx(s);
}

/*
 * Remove the page from the free list.
 * The caller is responsible for calling this
 * routine at splvm().
 */
void
page_unfree(pp)
	register struct page *pp;
{

	if (!pp->p_free)
		panic("page_unfree");

	trace2(TR_PG_UNFREE, pp, pp->p_age ? 0 : 1);

	if (pp->p_age)
	{
		page_freelist_sub(pp);
		page_freelist_size--;
	} else {
		page_sub(&page_cachelist, pp);
		page_cachelist_size--;
	}
	pp->p_free = pp->p_age = 0;
	freemem--;
}

/*
 * Allocate enough pages for bytes of data.
 * Return a doubly linked, circular list of pages.
 * Must spl around entire routine to prevent races from
 * pages being allocated at interrupt level.
 */
struct page *
page_get(bytes, canwait)
	u_int bytes;
	int canwait;
{
	register struct page *pp;
	struct page *plist;
	register int npages;
	int s;

	ASSERT(bytes > 0);
	npages = btopr(bytes);
	/*
	 * Try to see whether request is too large to *ever* be
	 * satisfied, in order to prevent deadlock.  We arbitrarily
	 * decide to limit maximum size requests to max_page_get.
	 */
	if (npages >= max_page_get) {
		trace4(TR_PAGE_GET, bytes, canwait, freemem, 1);
		return ((struct page *)NULL);
	}

	/*
	 * If possible, wait until there are enough
	 * free pages to satisfy our entire request.
	 *
	 * XXX:	Before waiting, we try to arrange to get more pages by
	 *	processing the i/o completion list and prodding the
	 *	pageout daemon.  However, there's nothing to guarantee
	 *	that these actions will provide enough pages to satisfy
	 *	the request.  In particular, the pageout daemon stops
	 *	running when freemem > lotsfree, so if npages > lotsfree
	 *	there's nothing going on that will bring freemem up to
	 *	a value large enough to satisfy the request.
	 */
	s = splvm();
	while (freemem < npages) {
		if (!canwait) {
			trace4(TR_PAGE_GET, bytes, canwait, freemem, 2);
			(void) splx(s);
			return ((struct page *)NULL);
		}
		/*
		 * Given that we can wait, call cleanup directly to give
		 * it a chance to add pages to the free list.  This strategy
		 * avoids the cost of context switching to the pageout
		 * daemon unless it's really necessary.
		 */
		if (bclnlist != NULL) {
			(void) splx(s);
			cleanup();
			s = splvm();
			continue;
		}
		/*
		 * There's nothing immediate waiting to become available.
		 * Turn the pageout daemon loose to find something.
		 */
		trace1(TR_PAGEOUT_CALL, 0);
		outofmem();
		freemem_wait++;
		trace4(TR_PAGE_GET_SLEEP, bytes, canwait, freemem, 0);
		(void) sleep((caddr_t)&freemem, PSWP+2);
		trace4(TR_PAGE_GET_SLEEP, bytes, canwait, freemem, 1);
	}

	freemem -= npages;
	trace4(TR_PAGE_GET, bytes, canwait, freemem, 0);
	/*
	 * If satisfying this request has left us with too little
	 * memory, start the wheels turning to get some back.  The
	 * first clause of the test prevents waking up the pageout
	 * daemon in situations where it would decide that there's
	 * nothing to do.  (However, it also keeps bclnlist from
	 * being processed when it otherwise would.)
	 *
	 * XXX: Check against lotsfree rather than desfree?
	 */
	if (nscan < desscan && freemem < desfree) {
		trace1(TR_PAGEOUT_CALL, 1);
		outofmem();
	}

	kmon_enter(&page_freelock);

	/*
	 * Pull the pages off the free list and build the return list.
	 */
	plist = NULL;
	while (npages--) {
		struct page **ppl;

		ppl = get_page_freelist();
		pp = *ppl;
		if (pp == NULL) {
			pp = *(ppl = &page_cachelist);
			if (pp == NULL)
				panic("out of memory");
			trace5(TR_PG_ALLOC, pp, pp->p_vnode, pp->p_offset,
				pp->p_age, 1);
			pagecnt.pc_get_cache++;
		} else {
			trace5(TR_PG_ALLOC, pp, pp->p_vnode, pp->p_offset,
				0, 0);
			pagecnt.pc_get_free++;
		}

/* XXX */
		if (pp->p_mapping != NULL) {
			printf("page_get: page %x has mappings!\n", pp);
			call_debug("page_get map");
			hat_pageunload(pp);
		}
/* XXX */
		trace3(TR_PG_ALLOC1, pp, plist, ppl == &page_freelist ? 0 : 1);
		if (pp->p_age) {
			page_freelist_sub(pp);
			page_freelist_size--;
		} else {
			page_sub(&page_cachelist, pp);
			page_cachelist_size--;
		}
		if (pp->p_vnode) {
			/* destroy old vnode association */
			CHECK(pp->p_vnode);
			page_hashout(pp);
		}

		pp->p_free = pp->p_mod = pp->p_nc = pp->p_age = 0;
		pp->p_lock = pp->p_intrans = pp->p_pagein = 0;
		pp->p_ref = 1;		/* protect against immediate pageout */
		pp->p_keepcnt = 1;	/* mark the page as `kept' */
		page_add(&plist, pp);
	}
#ifdef SUN4_110
	if (cpu == CPU_SUN4_110 && fault_type)
		fault_type = 0;
#endif SUN4_110

	kmon_exit(&page_freelock);

	CHECKFREE();
	(void) splx(s);

	return (pp);
}

/*
 * XXX - need to fix up all this page rot!
 */

/*
 * Release a keep count on the page and handle aborting the page if the
 * page is no longer held by anyone and the page has lost its identity.
 */
void
page_rele(pp)
	struct page *pp;
{

	ASSERT(pp->p_free == 0);
	kmon_enter(&page_mplock);

	if (pp->p_keepcnt == 0)			/* sanity check */
		panic("page_rele");
	if (--pp->p_keepcnt == 0) {
		while (pp->p_want) {
			kcv_broadcast(&page_mplock, (char *)pp);
			pp->p_want = 0;
		}
	}

	kmon_exit(&page_mplock);

	if (pp->p_keepcnt == 0 && (pp->p_gone || pp->p_vnode == NULL))
		page_abort(pp);			/* yuck */
}

/*
 * Lock a page.
 */
void
page_lock(pp)
	struct page *pp;
{

	ASSERT(pp->p_free == 0);
	ASSERT(pp->p_lock == 0);
	kmon_enter(&page_mplock);
	pp->p_lock = 1;
	kmon_exit(&page_mplock);
}

/*
 * Unlock a page.
 */
void
page_unlock(pp)
	struct page *pp;
{

	ASSERT(pp->p_free == 0);
	kmon_enter(&page_mplock);
	/*
	 * XXX - This should never happen.
	 */
	if (pp->p_intrans)
		call_debug("page_unlock: unlocking intrans page");

	pp->p_lock = 0;
	kmon_exit(&page_mplock);
}

/*
 * Add page ``pp'' to the hash/vp chains for <vp, offset>.
 */
static void
page_hashin(pp, vp, offset, lock)
	register struct page *pp;
	register struct vnode *vp;
	u_int offset, lock;
{
	register struct page **hpp;
	register int s;

	/*
	 * Raise priority to splvm() since the hash list
	 * can be manipulated at interrupt level.
	 */
	s = splvm();
	trace4(TR_PG_HASHIN, pp, vp, offset, lock);

	pp->p_vnode = vp;
	pp->p_offset = offset;
	pp->p_lock = lock;

	hpp = &page_hash[PAGE_HASHFUNC(vp, offset)];
	pp->p_hash = *hpp;
	*hpp = pp;

	if (vp == (struct vnode *)NULL) {
		(void) splx(s);
		return;			/* no real vnode */
	}

	/*
	 * Add the page to the front of the linked list of pages
	 * using p_vpnext/p_vpprev pointers for the list.
	 */
	if (vp->v_pages == NULL) {
		pp->p_vpnext = pp->p_vpprev = pp;
	} else {
		pp->p_vpnext = vp->v_pages;
		pp->p_vpprev = vp->v_pages->p_vpprev;
		vp->v_pages->p_vpprev = pp;
		pp->p_vpprev->p_vpnext = pp;
	}
	vp->v_pages = pp;
	CHECKPP(pp);
	(void) splx(s);
}

/*
 * Remove page ``pp'' from the hash and vp chains and remove vp association.
 */
void
page_hashout(pp)
	register struct page *pp;
{
	register struct page **hpp, *hp;
	register struct vnode *vp;
	register int s;

	/*
	 * Raise priority to splvm() since the hash list
	 * can be manipulated at interrupt level.
	 */
	s = splvm();
	CHECKPP(pp);
	vp = pp->p_vnode;
	trace3(TR_PG_HASHOUT, pp, vp, pp->p_gone);
	hpp = &page_hash[PAGE_HASHFUNC(vp, pp->p_offset)];
	for (;;) {
		hp = *hpp;
		if (hp == pp)
			break;
		if (hp == NULL)
			panic("page_hashout");
		hpp = &hp->p_hash;
	}
	*hpp = pp->p_hash;

	pp->p_hash = NULL;
	pp->p_vnode = NULL;
	pp->p_offset = 0;
	pp->p_gone = 0;

	/*
	 * Remove this page from the linked list of pages
	 * using p_vpnext/p_vpprev pointers for the list.
	 */
	CHECKPP(pp);
	if (vp->v_pages == pp)
		vp->v_pages = pp->p_vpnext;		/* go to next page */

	if (vp->v_pages == pp)
		vp->v_pages = NULL;			/* page list is gone */
	else {
		pp->p_vpprev->p_vpnext = pp->p_vpnext;
		pp->p_vpnext->p_vpprev = pp->p_vpprev;
	}
	pp->p_vpprev = pp->p_vpnext = pp;	/* make pp a list of one */
	(void) splx(s);
}

/*
 * Add the page to the front of the linked list of pages
 * using p_next/p_prev pointers for the list.
 * The caller is responsible for protecting the list pointers.
 */
void
page_add(ppp, pp)
	register struct page **ppp, *pp;
{
	register struct page *next;

	next = *ppp;
	if (next == NULL) {
		pp->p_next = pp->p_prev = pp;
	} else {
		pp->p_next = next;
		pp->p_prev = next->p_prev;
		next->p_prev = pp;
		pp->p_prev->p_next = pp;
	}
	*ppp = pp;
	CHECKPP(pp);
}

/*
 * Remove this page from the linked list of pages
 * using p_next/p_prev pointers for the list.
 * The caller is responsible for protecting the list pointers.
 */
void
page_sub(ppp, pp)
	register struct page **ppp, *pp;
{
	register struct page *head;
	register struct page *next;
	register struct page *prev;

	CHECKPP(pp);
	if (*ppp == NULL || pp == NULL)
		panic("page_sub");

/* locate other interesting pages */
	next = pp->p_next;
	prev = pp->p_prev;
	head = *ppp;

/* unlink our page from the list */
	next->p_prev = prev;
	prev->p_next = next;

/* make our page a list of one */
	pp->p_prev = pp->p_next = pp;

/* if we were first on the list, update the list pointer. */
	if (head == pp)
		if (next == pp)
			*ppp = NULL;
		else
			*ppp = next;
}

/*
 * Add this page to the list of pages, sorted by offset.
 * Assumes that the list given by *ppp is already sorted.
 * The caller is responsible for protecting the list pointers.
 */
void
page_sortadd(ppp, pp)
	register struct page **ppp, *pp;
{
	register struct page *p1;
	register u_int off;

	CHECKLIST(*ppp);
	CHECKPP(pp);
	if (*ppp == NULL) {
		pp->p_next = pp->p_prev = pp;
		*ppp = pp;
	} else {
		/*
		 * Figure out where to add the page to keep list sorted
		 */
		p1 = *ppp;
		if (pp->p_vnode != p1->p_vnode && p1->p_vnode != NULL &&
		    pp->p_vnode != NULL)
			call_debug("page_sortadd: bad vp");

		off = pp->p_offset;
		if (off < p1->p_prev->p_offset) {
			do {
				if (off == p1->p_offset)
					call_debug("page_sortadd: same offset");
				if (off < p1->p_offset)
					break;
				p1 = p1->p_next;
			} while (p1 != *ppp);
		}

		/* link in pp before p1 */
		pp->p_next = p1;
		pp->p_prev = p1->p_prev;
		p1->p_prev = pp;
		pp->p_prev->p_next = pp;

		if (off < (*ppp)->p_offset)
			*ppp = pp;		/* pp is at front */
	}
	CHECKLIST(*ppp);
}

/*
 * Wait for page if kept and then reclaim the page if it is free.
 * Caller needs to verify page contents after calling this routine.
 *
 * NOTE:  The caller is must ensure that the page is not on
 *	  the free list before calling this routine.
 */
void
page_wait(pp)
	register struct page *pp;
{
	register struct vnode *vp;
	register u_int offset;
	register int s;

	ASSERT(pp->p_free == 0);
	CHECKPP(pp);
	vp = pp->p_vnode;
	offset = pp->p_offset;

	/*
	 * Reap any pages in the to be cleaned list.
	 * This might cause the page that we might
	 * have to wait for to become available.
	 */
	if (bclnlist != NULL) {
		cleanup();
		/*
		 * The page could have been freed by cleanup, so
		 * verify the identity after raising priority since
		 * it may be ripped away at interrupt level.
		 */
		s = splvm();
		if (pp->p_vnode != vp || pp->p_offset != offset) {
			(void) splx(s);
			return;
		}
	} else
		s = splvm();

	kmon_enter(&page_mplock);
	while (pp->p_keepcnt != 0) {
		page_cv_wait(pp);
		/*
		 * Verify the identity of the page since it
		 * could have changed while we were sleeping.
		 */
		if (pp->p_vnode != vp || pp->p_offset != offset)
			break;
	}

	/*
	 * If the page is now on the free list and still has
	 * its original identity, get it back.  If the page
	 * has lost its old identity, the caller of page_wait
	 * is responsible for verifying the page contents.
	 */
	if (pp->p_vnode == vp && pp->p_offset == offset && pp->p_free)
		page_reclaim(pp);

	kmon_exit(&page_mplock);
	(void) splx(s);
	CHECKPP(pp);
}

/*
 * Lock a physical page into memory "long term".  Used to support "lock
 * in memory" functions.  Accepts the page to be locked, a claim variable
 * to indicate whether a claim for an extra page should be recorded (to
 * support, for instance, a potential copy-on-write), and a flag to
 * indicate whether the current reservation should be checked before
 * locking the page.
 */
static	void page_pp_dolock(/* pp */);
static	void page_pp_dounlock(/* pp */);

int
page_pp_lock(pp, claim, check_resv)
	struct page *pp;		/* page to be locked */
	int claim;			/* amount extra to be claimed */
	int check_resv;			/* check current reservation */
{
	int r = 0;			/* result -- assume failure */

	kmon_enter(&page_locklock);

	/*
	 * Allow the lock if check_resv is 0, or if the current
	 * reservation plus the pages reserved by this request
	 * do not exceed the maximum.
	 */
	if (check_resv == 0 || ((pages_pp_locked + pages_pp_claimed +
	    claim + (pp->p_lckcnt == 0 ? 1 : 0)) <= pages_pp_maximum)) {
		page_pp_dolock(pp);
		pages_pp_claimed += claim;
		r = 1;
	}
	kmon_exit(&page_locklock);
	return (r);
}

/*
 * Decommit a lock on a physical page frame.  Account for claims if
 * appropriate.
 */
void
page_pp_unlock(pp, claim)
	struct page *pp;		/* page to be unlocked */
	int claim;			/* amount to be unclaimed */
{

	kmon_enter(&page_locklock);
	page_pp_dounlock(pp);
	pages_pp_claimed -= claim;
	kmon_exit(&page_locklock);
}

/*
 * Locking worker function for incrementing a lock.  If a page has been
 * locked too many times, the lock sticks (just like your eyes would if you
 * crossed them too many times.)  Page struct must be locked and locking
 * mutex entered.
 */
static void
page_pp_dolock(pp)
	struct page *pp;
{

	if (pp->p_lckcnt == 0)
		pages_pp_locked++;
	if (pp->p_lckcnt < PAGE_LOCK_MAXIMUM)
		if (++pp->p_lckcnt == PAGE_LOCK_MAXIMUM)
			printf("Page frame 0x%x locked permanently\n",
				page_pptonum(pp));
}

/*
 * Locking worker function for decrementing a lock.  Page struct must
 * be locked and locking mutex monitor set.
 */
static void
page_pp_dounlock(pp)
	struct page *pp;
{

	if (pp->p_lckcnt < PAGE_LOCK_MAXIMUM)
		if (--pp->p_lckcnt == 0)
			pages_pp_locked--;
}

/*
 * Simple claim adjust functions -- used to support to support changes in
 * claims due to changes in access permissions.
 */
int
page_addclaim(claim)
	int claim;			/* claim count to add */
{
	int r = 1;			/* result */

	kmon_enter(&page_locklock);
	if ((pages_pp_locked + pages_pp_claimed + claim) <= pages_pp_maximum)
		pages_pp_claimed += claim;
	else
		r = 0;
	kmon_exit(&page_locklock);
	return (r);
}

void
page_subclaim(claim)
	int claim;			/* claim count to remove */
{

	kmon_enter(&page_locklock);
	pages_pp_claimed -= claim;
	kmon_exit(&page_locklock);
}

/*
 * Simple functions to transform a page pointer to a
 * physical page number and vice versa. They search the memory segments
 * to locate the desired page. Within a segment, pages increase linearly
 * with one page structure per PAGESIZE / MMU_PAGESIZE physical page frames.
 * The search begins with the segment that was accessed last, to take
 * advantage of any locality.
 */
u_int
page_pptonum(pp)
	register struct page *pp;
{
	register struct memseg *tseg;
	/* make sure it's on the stack to avoid race conditions */
	register struct memseg *l_memseg = last_memseg;

	for (tseg = l_memseg; tseg != (struct memseg *)NULL;
	    tseg = tseg->next) {
		if (pp >= tseg->pages && pp < tseg->epages)
			goto found;
	}
	for (tseg = memsegs; tseg != l_memseg; tseg = tseg->next) {
		if (pp >= tseg->pages && pp < tseg->epages)
			goto found;
	}
	panic("page_pptonum");
found:
	last_memseg = tseg;
	return ((u_int)((pp - tseg->pages) * (PAGESIZE/MMU_PAGESIZE)) +
	    tseg->pages_base);
}

struct page *
page_numtopp(pfnum)
	register u_int pfnum;
{
	register struct memseg *tseg;
	/* make sure it's on the stack to avoid race conditions */
	register struct memseg *l_memseg = last_memseg;

	for (tseg = l_memseg; tseg != (struct memseg *)NULL;
	    tseg = tseg->next) {
		if (pfnum >= tseg->pages_base && pfnum < tseg->pages_end)
			goto found;
	}
	for (tseg = memsegs; tseg != l_memseg; tseg = tseg->next) {
		if (pfnum >= tseg->pages_base && pfnum < tseg->pages_end)
			goto found;
	}
	return ((struct page *)NULL);
found:
	last_memseg = tseg;
	return (tseg->pages +
	    ((pfnum - tseg->pages_base) / (PAGESIZE/MMU_PAGESIZE)));
}

/*
 * This routine is like page_numtopp, but will only return page structs
 * for pages which are ok for loading into hardware using the page struct.
 */
struct page *
page_numtookpp(pfnum)
	register u_int pfnum;
{
	register struct memseg *tseg;
	register struct page *pp;
	/* make sure it's on the stack to avoid race conditions */
	register struct memseg *l_memseg = last_memseg;

	for (tseg = l_memseg; tseg != (struct memseg *)NULL;
	    tseg = tseg->next) {
		if (pfnum >= tseg->pages_base && pfnum < tseg->pages_end)
			goto found;
	}
	for (tseg = memsegs; tseg != l_memseg; tseg = tseg->next) {
		if (pfnum >= tseg->pages_base && pfnum < tseg->pages_end)
			goto found;
	}
	return ((struct page *)NULL);
found:
	last_memseg = tseg;
	pp = tseg->pages +
	    ((pfnum - tseg->pages_base) / (PAGESIZE/MMU_PAGESIZE));
	if (pp->p_free || pp->p_gone)
		return ((struct page *)NULL);
	return (pp);
}

/*
 * This routine is like page_numtopp, but will only return page structs
 * for pages which are ok for loading into hardware using the page struct.
 * If not for the things like the window system lock page where we
 * want to make sure that the kernel and the user are exactly cache
 * consistent, we could just always return a NULL pointer here since
 * anyone mapping physical memory isn't guaranteed all that much
 * on a virtual address cached machine anyways.  The important thing
 * here is not to return page structures for things that are possibly
 * currently loaded in DVMA space, while having the window system lock
 * page still work correctly.
 */
struct page *
page_numtouserpp(pfnum)
	register u_int pfnum;
{
	register struct memseg *tseg;
	register struct page *pp;
	/* make sure it's on the stack to avoid race conditions */
	register struct memseg *l_memseg = last_memseg;

	for (tseg = l_memseg; tseg != (struct memseg *)NULL;
	    tseg = tseg->next) {
		if (pfnum >= tseg->pages_base && pfnum < tseg->pages_end)
			goto found;
	}
	for (tseg = memsegs; tseg != l_memseg; tseg = tseg->next) {
		if (pfnum >= tseg->pages_base && pfnum < tseg->pages_end)
			goto found;
	}
	return ((struct page *)NULL);
found:
	last_memseg = tseg;
	pp = tseg->pages +
	    ((pfnum - tseg->pages_base) / (PAGESIZE/MMU_PAGESIZE));
	if (pp->p_free || pp->p_gone || pp->p_intrans || pp->p_lock ||
	    /* is this page possibly involved in indirect (raw) IO? */
	    (pp->p_keepcnt != 0 && pp->p_vnode != NULL))
	{
#ifdef SUN3X_470
/* pegasus hardware workaround - see sun3x/vm_hat.c */
		char *iop;

		iop = pptoiocp(pp);
		if (*iop) page_wait(pp);
#endif	SUN3X_470
		return ((struct page *)NULL);
	}
	return (pp);
}

/*
 * Debugging routine only!
 * XXX - places calling this either need to
 * remove the test altogether or call panic().
 */
#include <sys/reboot.h>
#include <sys/systm.h>
#include <debug/debug.h>
call_debug(mess)
	char *mess;
{

#ifdef SAS
	printf("call_debug(): trying to call kadb from SAS\n");
	panic(mess);
#else
	if (boothowto & RB_DEBUG) {
		printf("%s\n", mess);
		CALL_DEBUG();
	} else {
		panic(mess);
	}
#endif
}

#ifdef PAGE_DEBUG
/*
 * Debugging routine only!
 */
static
page_vp_check(vp)
	register struct vnode *vp;
{
	register struct page *pp;
	int count = 0;
	int err = 0;

	if (vp == NULL)
		return;

	if ((pp = vp->v_pages) == NULL) {
		/* random check to see if no pages on this vp exist */
		if ((pp = page_find(vp, 0)) != NULL) {
			printf("page_vp_check: pp=%x on NULL vp list\n", vp);
			call_debug("page_vp_check");
		}
		return;
	}

	do {
		if (pp->p_vnode != vp) {
			printf("pp=%x pp->p_vnode=%x, vp=%x\n",
			    pp, pp->p_vnode, vp);
			err++;
		}
		if (pp->p_vpnext->p_vpprev != pp) {
			printf("pp=%x, p_vpnext=%x, p_vpnext->p_vpprev=%x\n",
			    pp, pp->p_vpnext, pp->p_vpnext->p_vpprev);
			err++;
		}
		if (++count > 10000) {
			printf("vp loop\n");
			err++;
			break;
		}
		pp = pp->p_vpnext;
	} while (err == 0 && pp != vp->v_pages);

	if (err)
		call_debug("page_vp_check");
}

/*
 * Debugging routine only!
 */
static
page_free_check()
{
	int err = 0;
	int count = 0;
	register struct page *pp;

	if (page_freelist != NULL) {
		pp = page_freelist;
		do {
			if (pp->p_free == 0 || pp->p_age == 0) {
				err++;
				printf("page_free_check: pp = %x\n", pp);
			}
			count++;
			pp = pp->p_next;
		} while (pp != page_freelist);
	}
	if (page_cachelist != NULL) {
		pp = page_cachelist;
		do {
			if (pp->p_free == 0 || pp->p_age != 0) {
				err++;
				printf("page_free_check: pp = %x\n", pp);
			}
			count++;
			pp = pp->p_next;
		} while (pp != page_cachelist);
	}

	if (err || count != freemem) {
		printf("page_free_check:  count = %x, freemem = %x\n",
		    count, freemem);
		call_debug("page_check_free");
	}
}

/*
 * Debugging routine only!
 */
page_print(pp)
	register struct page *pp;
{
	register struct vnode *vp;

	printf("page 0x%x: ", pp);
	if (pp->p_lock) printf("LOCK ");
	if (pp->p_want) printf("WANT ");
	if (pp->p_free) printf("FREE ");
	if (pp->p_intrans) printf("INTRANS ");
	if (pp->p_gone) printf("GONE ");
	if (pp->p_mod) printf("MOD ");
	if (pp->p_ref) printf("REF ");
	if (pp->p_pagein) printf("PAGEIN ");
	printf("\nnio %d keepcnt %d\n", pp->p_nio, pp->p_keepcnt);
	printf("vnode 0x%x, offset 0x%x\n", pp->p_vnode, pp->p_offset);
	if (swap_anon(pp->p_vnode, pp->p_offset))
		printf("  (ANON)\n");
	else if (vp = pp->p_vnode)
		printf("  v_flag 0x%x, v_count %d, v_type %d\n", vp->v_flag,
		    vp->v_count, vp->v_type);
	printf("next 0x%x, prev 0x%x, vpnext 0x%x vpprev 0x%x mapping 0x%x\n",
	    pp->p_next, pp->p_prev, pp->p_vpnext, pp->p_vpprev, pp->p_mapping);
}

/*
 * Debugging routine only!
 * Verify that the list is properly sorted by offset on same vp
 */
page_list_check(plist)
	struct page *plist;
{
	register struct page *pp = plist;

	if (pp == NULL)
		return;
	while (pp->p_next != plist) {
		if (pp->p_next->p_offset <= pp->p_offset ||
		    pp->p_vnode != pp->p_next->p_vnode) {
			printf("pp = %x <%x, %x> pp next = %x <%x, %x>\n",
			    pp, pp->p_vnode, pp->p_offset, pp->p_next,
			    pp->p_next->p_vnode, pp->p_next->p_offset);
			call_debug("page_list_check");
		}
		pp = pp->p_next;
	}
}

/*
 * Debugging routine only!
 * Verify that pp is actually on vp page list.
 */
page_pp_check(pp)
	register struct page *pp;
{
	register struct page *p1;
	register struct vnode *vp;

	if ((vp = pp->p_vnode) == (struct vnode *)NULL)
		return;

	if ((p1 = vp->v_pages) == (struct page *)NULL) {
		printf("pp = %x, vp = %x\n", pp, vp);
		call_debug("NULL vp page list");
		return;
	}

	do {
		if (p1 == pp)
			return;
	} while ((p1 = p1->p_vpnext) != vp->v_pages);

	printf("page %x not on vp %x page list\n", pp, vp);
	call_debug("vp page list");
}
#endif PAGE_DEBUG

#ifdef TRACE
initialtrace_page()
{
	register struct page *pp;

	trace2(TR_PG_INIT, pages, epages - pages);
	for (pp = pages; pp < epages; pp++)
		trace4(TR_PG_PINIT, pp, pp->p_vnode, pp->p_offset, pp->p_free);
}

trace_vn_reuse(vp)
struct vnode *vp;
{
	int pagecount;
	struct page *pp;

	for (pp = vp->v_pages, pagecount = 0;
		pp != NULL && pp->p_vpnext != vp->v_pages;
		pp = pp->p_vpnext, pagecount++)
			;
	trace2(TR_VN_REUSE, vp, pagecount);
}
#endif TRACE
