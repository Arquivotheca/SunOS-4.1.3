#ifndef lint
static  char sccsid[] = "@(#)mb_machdep.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988-1991 by Sun Microsystems, Inc.
 */

/*
 * Machine dependent mainbus support routines.
 *
 * N.B. The mainbus code no longer requires the mainbus structures (mb_ctlr
 * and so on) to ask for DVMA space or to queue up when DVMA is not available.
 * Drivers can now send in a generic map structure and a pointer to a function
 * which is queued and invoked later when space frees up again. This mechanism
 * allows devices to manage their own queues and especially to queue multiple
 * I/O's to the same device. The old mbsetup()/mbrelse() interface has been
 * retained and can be used in the same way as before (these routines can
 * be found in <sundev/mb.c>).
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vmmac.h>
#include <sys/vmmeter.h>
#include <sys/vmparam.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/mman.h>
#include <sys/debug.h>
#include <sys/kmem_alloc.h>

#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/reg.h>

#include <sundev/mbvar.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/page.h>
#include <vm/seg.h>
#include <machine/seg_kmem.h>

#ifdef IOMMU
#include <machine/iommu.h>
#endif IOMMU

#ifdef IOC
#include <machine/iocache.h>
#endif IOC

/*
 * Drivers queue function pointers here when they have to wait for
 * resources. Subsequent calls from the same driver that are forced to
 * wait need not queue up again, as the function pointed to will run the
 * driver's internal queues until done or space runs out again. The arg
 * parameter can be used by drivers as a cookie to be passed back by the
 * callback function. They should pass in NULL if not using the parameter.
 * Note that if drivers keep calling the allocation routine after
 * resources ran out that the arg parameter associated with subsequent
 * calls is ignored.
 *
 * The system priority level at which the allocation routine is called at
 * also indicates an implicit ordering as to whom is called back first.
 * When a resource is freed, the current system priority level is used to
 * screen out and not call back directly waiters who called the allocation
 * routine at a lower system priority level- they instead are called via
 * softcall() (if resources are left after calling back directly those who
 * can be called back directly). Note that if the original call had stored
 * up at priority 0, the intent of this indirect mechanism cannot be
 * achieved (and a warning message will be printed upon the storing of
 * such a callback).
 *
 * Because elements may be removed out of order (due to pri differences),
 * a simple cycle through a limited array isn't enough if we still wan't
 * preserve any semblance of FIFO servicing. A simple linked list will fit
 * the bill. In fact, an array could still be used, but it turns out that
 * a linked list uses fewer instructions to manage in this fashion (while
 * losing locality of reference and possible cache gains). On the other
 * hand, this is more like we'll want this to be, and it probably doesn't
 * make that big a difference.
 */

#define	MAXMBQLEN	10

static struct mbq_elem {
	struct mbq_elem	*mbq_next;
	func_t		mbq_funcp;
	caddr_t		mbq_arg;
	int		mbq_pri;
} mbq[MAXMBQLEN];


/*
 * MB related stats
 */
static struct mbs {
	int m_runout;   	/* times DVMA has run out */
	int m_queued;    	/* driver already had request queued */
	int m_bigreq;   	/* largest request received (bytes) */
	int m_hiqlen;    	/* longest length of wait queue */
	int m_softcall;		/* number of callbacks via softcall */
} mbs;

static int mbqlen;
static struct mbq_elem *mbqfree, *mbwaitq;
static void mbq_store();	/* store element */
static void mbq_retrieve();	/* retrieve element */
static int mbq_softcall();	/* entry into callback via softcall */

/*
 * run_mhq && mb_setup now in sundev/mb.c
 */

/*
 * Allocate and setup Mainbus map registers.  Flags says whether the
 * caller can't wait (e.g. if the caller is at interrupt level).  We
 * also allow DMA to memory already mapped at the Mainbus (e.g., for
 * Sun Ethernet board memory) if buscheck() says it's ok.
 *
 * We assume that the physical translations for the pages have already
 * been locked down before we were called.
 */

/*
 * New interface for mainbus resources. The main reason for this is to
 * provide smart controllers with the ability to queue multiple I/Os to
 * a single device, which was impossible with the old stuff. This is also
 * a move towards cleaning up the complexity of the queueing structures
 * both for DVMA resources and for autoconf.
 *
 * This routine accepts a function pointer which is queued if there is no
 * no space available and the caller cannot sleep.
 */
int
mb_mapalloc(map, bp, flags, waitfp, arg)
	register struct map *map;
	register struct buf *bp;
	int flags;
	func_t waitfp;
	caddr_t arg;
{
	register int reg, s, o, npf;
	struct mbcookie mbcookie;
#if defined(sun3x) || defined(sun4m)
	struct pte pte;
#endif sun3x || sun4m
#ifdef sun2
	struct ctx *ctx;
#endif sun2

#if defined(sun4m) && defined(IOMMU)
	struct map *iom_choose_map();
	extern int iom;
#endif defined(sun4m) && defined(IOMMU)

	/*
	 * Size the xfer, rounding up a page if necessary.
	 */
	o = (int)bp->b_un.b_addr & PAGEOFFSET;
	npf = mmu_btopr(bp->b_bcount + o);

	reg = buscheck(bp);
	if (reg < 0)
		panic("mbsetup buscheck fail");
#ifndef	sun4c
#if defined(sun4m) && defined(IOMMU)
	if (reg > 0) {
		/*
		 * If DVMA target is not main memory. Make
		 * sure they can be DVMA'ed.
		 */
		switch (bustype(reg)) {
		case BT_SBUS:
			if (map == mbutlmap) {
				/*
				 * mbutlmap use only from m_clalloc
				 */
				return(0);
			}
		case BT_VIDEO:
			/* go ahead to setup IOMMU */
			break;

		case BT_VME:
			if (map == mb_hd.mh_map) {
				/*
				 * VME-> VME.
				 *
				 * This does not involve IOC/IOMMU,
				 * just return VME slave address.
				 */
				mbcookie.mbi_mapreg = reg;
				mbcookie.mbi_offset = o;
				return (*(int *)&mbcookie);
			} else {
				/* Sbus to VME, not supported!! */
				return(0);
			}

		case BT_UNKNOWN:
		case BT_OBIO:
			/* NO DVMA allowed to these spaces! */
			return(0);
		}
	}

#else defined(sun4m) && defined(IOMMU)

	/*
	 * If not OBMEM, for non-sun4c machines, return the
	 * cookie that buscheck makes up for VME addresses.
	 *
	 * For sun4c, we skip this and force a mapping across
	 * DVMA[] (I/O on sun4c always goes through MMU context
	 * 0- even SBus to SBus transactions).
	 */
	if (reg > 0) {
		/*
		 * In contiguous mainbus memory,
		 * i.e., not on on-board memory
		 * (mainbus is historical).
		 */
		mbcookie.mbi_mapreg = reg;
		mbcookie.mbi_offset = o;
		return (*(int *)&mbcookie);
	}
#endif defined(sun4m) && defined(IOMMU)
#endif	sun4c

#if defined(sun4m) && defined(IOMMU)
	/* see if we can use vme32map */
	if ((map == mb_hd.mh_map) && (flags & MDR_VME32) &&
	    		!(flags & MDR_DVMA_PEEK))
		map = vme32map;

	/* see if we can use bigsbusmap */
	if ((map == dvmamap) && (flags & MDR_BIGSBUS))
		map = bigsbusmap;

	/* see if we can use altsbusmap */
	if ((map == dvmamap) && (flags & MDR_ALTSBUS))
		map = altsbusmap;
#endif defined(sun4m) && defined(IOMMU)

	/*
	 * Else the request is for all type 0 memory which still needs
	 * to be mapped into DVMA space.  Allocate virtual space for the
	 * mapping including one extra for redzone at end of mapping.
	 */

	s = splvm();
	while ((reg = bp_alloc(map, bp, npf + 1)) == 0) {
		/*
		 * No DVMA available. We either queue the function ptr
		 * or sleep, depending on the caller's state of mind.
		 */
		if (flags & MB_CANTWAIT) {
			if (waitfp) {
				mbq_store(waitfp, arg, spltoipl(s));
				mbs.m_bigreq = MAX(ctob(npf), mbs.m_bigreq);
			}
			(void) splx(s);
			return (0);
		}
		mapwant(map)++;
		(void) sleep((caddr_t)map, PSWP);
	}
	(void) splx(s);

#ifdef sun2
	ctx = mmu_getctx();
	if (ctx != kctx)
		mmu_setctx(kctx);
#endif sun2

#if defined(sun4m) && defined(IOMMU)
	if (!iom)
		bp_map(bp, &DVMA[mmu_ptob(reg)]);
	/* 
	 * if it's vme24map, pass DVMA_PEEK flag along
	 * so that bp_iom_map will load host SRMMU also.
	 */ 
	else if (map == vme24map)
		bp_iom_map(bp, reg, flags | DVMA_PEEK, map);
	else
		bp_iom_map(bp, reg, flags, map);
#else
	/*
	 * Map the bp into kernel virtual address in DVMA space.
	 * We over allocate one slot and mark this mapping
	 * invalid.  This protects against run away transfers
	 * and is also used as termination condition in mb_mapfree.
	 * We set the redzone to invalid every time because we're
	 * paranoid, but we use mmu_setpte instead of hat_pteload
	 * because we want it to be fast.
	 */
	bp_map(bp, &DVMA[mmu_ptob(reg)]);
#endif defined(sun4m) && defined(IOMMU)

	/* invalid red-zone's mapping */
#if defined(sun3x) 
	*(u_int *)&pte = MMU_INVALIDPTE;
	hat_pteload(&kseg, &DVMA[mmu_ptob(reg + npf)],
		(struct page *)NULL, &pte, PTELD_LOCK | PTELD_NOSYNC);
#elif defined(sun4m)
#ifdef IOMMU
	/* if iommu is on, only vme24map needs red zone on host */ 
	if (iom==0 || map == vme24map) {
		*(u_int *)&pte = MMU_STD_INVALIDPTE;
		hat_pteload(&kseg, &DVMA[mmu_ptob(reg + npf)],
		(struct page *)NULL, &pte, PTELD_LOCK | PTELD_NOSYNC);

	}
#else
	*(u_int *)&pte = MMU_STD_INVALIDPTE;
	hat_pteload(&kseg, &DVMA[mmu_ptob(reg + npf)],
		(struct page *)NULL, &pte, PTELD_LOCK | PTELD_NOSYNC);
#endif IOMMU
#else
	mmu_setpte(&DVMA[mmu_ptob(reg + npf)], mmu_pteinvalid);
#endif sun3x


#ifdef sun2
	if (ctx != kctx)
		mmu_setctx(ctx);
#endif sun2

	mbcookie.mbi_mapreg = reg;
	mbcookie.mbi_offset = o;
	return (*(int *)&mbcookie);
}

/*
 * mb_relse now back in sundev/mb.c
 */

/*
 * Release resources on Mainbus, and then unblock resource waiters.
 */
void
mb_mapfree(map, amr)
	register struct map *map;
	int *amr;
{
#if !defined(sun4m) || !defined(IOMMU)
	register char *addr;
	register int reg;
#endif
	register int npf;
#if defined(sun3x) || (defined(sun4m) && !defined(IOMMU))
	struct pte *ppte;
#endif
#if defined(sun3x) || defined(sun4m)
#ifdef SUN3X_470
	int flush = 0;
#endif SUN3X_470
#else sun3x || sun4m
	struct pte pte;
#endif sun3x || sun4m
	int mr, s;
#ifdef sun2
	struct ctx *ctx;
#endif sun2
#ifdef	sun4m
	int flush = 0;
#ifdef IOMMU
	struct map *ret_map;
	union iommu_pte *p_iopte;
#endif
#ifdef IOC
	int ioc_mr;
        extern void flush_writebuffers();
#endif
#endif

	/*
	 * Carefully see if we should release the space.
	 */
	s = splvm();
	mr = *amr;
	if (mr == 0) {
		printf("mb_mapfree: MR == 0!!!\n");
		(void) splx(s);
		return;
	}
	*amr = 0;
	(void) splx(s);		/* we're supposed to be safe for awhile */

#if defined(sun3x) || (defined(sun4m) && !defined(IOMMU))
	if ((reg = MBI_MR(mr)) < dvmasize) {		/* DVMA memory */
		/*
		 * First we have to read the hardware maps to figure
		 * out how big the transfer really was.  This relies
		 * on having an invalid pte to terminate the mapping.
		 * We start looking at the 2nd page to avoid always
		 * looking at the first page.
		 */
		addr = &DVMA[mmu_ptob(reg + 1)];
		/*
		 * NOTE -- this assumes that the ptes for DVMA space
		 * are contiguous and ascending. This must remain true!
		 */
		ppte = (struct pte *)hat_ptefind(&kas, addr);
		if (ppte == NULL)
			panic("mbrelse no ptes");
		/*
		 * See if the first page of the mapping was I/O cacheable.
		 */
#ifdef SUN3X_470
		if (cpu == CPU_SUN3X_470) {
			if ((ppte - 1)->pte_iocache)
				flush = 1;
		}
#endif SUN3X_470

#if defined(sun4m) && defined(IOC)
		if ((ptetospte(ppte -1))->spte_iocache)
			flush = 1;
#endif	sun4m && IOC

		npf = 2;
		for (;;) {
			if (!pte_valid(ppte))
				break;
			npf++;
			ppte++;
		}
#ifdef SUN3X_470
		if (cpu == CPU_SUN3X_470) {
			/*
			 * If the mapping was I/O cacheable, loop
			 * through the cache flushing all the lines
			 * for this mapping.
			 */
			if (flush) {
				for (flush = 0; flush < npf; flush++)
					ioc_flush(reg + flush);
			}
		}
#endif SUN3X_470

#if defined(sun4m) && defined(IOC)
		if (flush) {
			for (flush = 0; flush < npf; flush++)
				fast_ioc_flush(reg + flush);
		}
		flush_writebuffers();
#endif	sun4m && IOC

		/*
		 * Now use hat_unload to unload the translations
		 * without getting the referenced and modified bits.
		 * Note that even though sun4m needs to have its
		 * vac tended to, the following should do the
		 * trick, since it will end up going through the
		 * hat code.
		 */
		hat_unload(&kseg, &DVMA[mmu_ptob(reg)],
			(u_int)(npf << MMU_PAGESHIFT));
		/*
		 * Put back the registers in the resource map.
		 * The map code must not be reentered, so we
		 * do this at high spl.
		 */
		s = splvm();
		rmfree(map, (long)npf, (u_long)reg);
		(void) splx(s);
	}

#elif defined(sun4m) && defined(IOMMU)
	mr= mmu_btop(mr);	/* we really work on "pages" */
	if ((ret_map= map_addr_to_map(mr, map)) != NULL) {	/* DVMA */
#ifdef IOC
		/* IOC works with VME devices only, no S-bus devices */
		if (ret_map == vme32map || ret_map == vme24map) {
			/* see if IOC is on. */
			if (ioc_read_tag(mr) & IOC_LINE_ENABLE) {
				flush= 1;
			}
			/* see if there is padding at beginning */
			ioc_mr= mr= ioc_adj_start(mr);
		}

#endif IOC

		if ((p_iopte= iom_ptefind(mr, ret_map)) == NULL)
			panic("mb_mapfree: no iopte");

		/* count how many pages are there */
		for (npf= 0; ; p_iopte++, npf++) {
			if (p_iopte-> iopte.valid == 0)
				break;

#ifdef IOC
			/* flush every other page, we know ioc is 8K */
			if (flush && ((npf & 1)==0)) {
				fast_ioc_flush(ioc_mr);
				ioc_setup(ioc_mr, IOC_LINE_INVALID);
				ioc_mr += IOC_PAGES_PER_LINE;
			}
#endif IOC

			/* sync mod/ref bits */
			/* NOTE: I do not think we really need
			 *	 this iom_pagesync() call.
			 *
			 * ****************************
			 * iom_pagesync(p_iopte);
			 * ****************************
			 */
			/* unload IOMMU */
			iom_pteunload(p_iopte);
		}
		flush_writebuffers();

		if (ret_map == vme24map) {
			/* 
			 * NOTE: mr and npf here INCLUDES paddings.
			 * 	 Although we never loaded padding
			 *	 pages into host SRMMU, but we ask
			 *	 hat_unload to unload them anyway,
			 *	 since it can take this kind of 
			 *	 abuse. Otherwise, we will have
			 *	 to figure out the exact pages
			 *	 to be unloaded (excluding paddings).
			 */
			hat_unload(&kseg, &DVMA[mmu_ptob(mr)],
			    (u_int)mmu_ptob(npf));
		}

		s = splvm();
		/* add one for red zone */
		rmfree(ret_map, (long) (npf+1), (u_long)mr);
		(void) splx(s);
	}

#else defined(sun4m)
	if ((reg = MBI_MR(mr)) < dvmasize) {		/* DVMA memory */

#ifdef sun2
		ctx = mmu_getctx();
		if (ctx != kctx)
			mmu_setctx(kctx);
#endif sun2

#ifdef sun4c
		/*
		 * First we have to read the hardware maps to figure
		 * out how big the transfer really was.  This relies
		 * on having an invalid pte to terminate the mapping.
		 * We start looking at the first page since we are
		 * syncing the cache as we go. We only sync the
		 * cache for dirty pages, since output doesn't
		 * make the cache stale. This assumes that the
		 * I/O is going through the MMU and thus setting
		 * the dirty bit when appropriate.
		 *
		 * NOTE: this method of syncing the cache only
		 * works for write-through caches, where the flush
		 * is always an invalidate. A write-back cache
		 * would not work this way.
		 *
		 * Ah, but as this is an allocation in DVMA, then
		 * we didn't use segkmem and the hat layer to load
		 * things up, as cache consistency is not germane
		 * to sun4c where I/O doesn't go through the cache
		 * anyway. We still have to flush any I/U cache
		 * references to this physical page, but we can
		 * just nuke the pte mapping as we go..
		 */

		addr = &DVMA[mmu_ptob(reg)];
		npf = 0;
		for (;;) {
			mmu_getpte((addr_t)addr, &pte);
			if (!pte_valid(&pte))
				break;
#ifdef	VAC
			if (vac && pte.pg_m)
				hat_vacsync(MAKE_PFNUM(&pte));
#endif	VAC
			mmu_setpte((addr_t)addr, mmu_pteinvalid);
			addr += MMU_PAGESIZE;
			npf++;
		}
#else sun4c
#ifdef VAC
#ifdef sun4m
		if (cache) {
#else
		if (vac) {
#endif sun4m
			/*
			 * First we have to read the hardware maps to figure
			 * out how big the transfer really was.  This relies
			 * on having an invalid pte to terminate the mapping.
			 * If there is an i/o cache we have to check the
			 * first page to see if it needs to be flushed, so
			 * to make this code cleaner we just always start at
			 * the first page even though for non ioc machines
			 * we could start at the second.
			 */
			addr = &DVMA[mmu_ptob(reg)];
			npf = 0;
			for (;;) {
				mmu_getpte((addr_t)addr, &pte);
				if (!pte_valid(&pte))
					break;
#ifdef IOC
				if (pte.pg_ioc)
					ioc_flush(reg + npf);
#endif IOC
				addr += MMU_PAGESIZE;
				npf++;
			}
			/*
			 * Now use hat_unload to unload the translations
			 * without getting the referenced and modified bits.
			 */
			hat_unload(&kseg, &DVMA[mmu_ptob(reg)],
			    (u_int)mmu_ptob(npf));
		} else
#endif VAC
		{
			/*
			 * No vac, can just use mmu operations to unload
			 * the DVMA mappings.
			 */
			addr = &DVMA[mmu_ptob(reg)];
			npf = 0;
			for (;;) {
				mmu_getpte((addr_t)addr, &pte);
				if (!pte_valid(&pte))
					break;
#ifdef IOC
				if (pte.pg_ioc)
					ioc_flush(reg + npf);
#endif IOC
				mmu_setpte((addr_t)addr, mmu_pteinvalid);
				addr += MMU_PAGESIZE;
				npf++;
			}
		}
#endif sun4c
#ifdef sun2
		if (ctx != kctx)
			mmu_setctx(ctx);
#endif sun2

		/*
		 * Put back the registers in the resource map.
		 * The map code must not be reentered, so we
		 * do this at high spl.
		 */
		s = splvm();
		rmfree(map, (long)(npf + 1), (u_long)reg);
		(void) splx(s);
	}
#endif sun3x

	/*
	 * Try to map waiting devices, using the queued function ptr.
	 * If a request can't be mapped, we just return, as mb_mapalloc
	 * has already requeued the request.
	 */
	s = splvm();
	if (mbwaitq)
		mbq_retrieve(spltoipl(s));
	(void) splx(s);
}

/*
 * Store a queue element, returning if the funcp is already queued.
 * Always called at splvm().
 */
static void
mbq_store(funcp, arg, pri)
	func_t funcp;
	caddr_t arg;
	int pri;
{
	register struct mbq_elem *lq = NULL;
	register struct mbq_elem *q =  mbwaitq;

	/*
	 * Search the queue to see whether funcp is still queued
	 */

	while (q != NULL) {
		if (q->mbq_funcp == funcp) {
			mbs.m_queued++;
			return;
		}
		lq = q;
		q = q->mbq_next;
	}

	if ((q = mbqfree) == NULL) {
		int i;

		/*
		 * If mbwaitq is NULL when mbqfree is NULL, then
		 * this is the first time thru, else we just got
		 * too many callers. Die.
		 */

		if (mbwaitq) {
			panic("mbq_store: too many callers");
			/*NOTREACHED*/
		}

		/*
		 * ...else initialize the free queue
		 */

		for (i = 0; i < MAXMBQLEN-1; i++)
			mbq[i].mbq_next = &mbq[i+1];
		q = mbqfree = mbq;
	}

	mbqfree = q->mbq_next;

	q->mbq_funcp = funcp;
	q->mbq_arg = arg;
	if ((q->mbq_pri = pri) == 0) {
		printf("Warning: MB callback stored at priority 0!\n");
	}

	if (lq == NULL) {
		q->mbq_next = mbwaitq;
		mbwaitq = q;
	} else {
		q->mbq_next = (struct mbq_elem *) NULL;
		lq->mbq_next = q;
	}

	mbs.m_runout++;
	mbqlen++;
	mbs.m_hiqlen = MAX(mbqlen, mbs.m_hiqlen);
}

/*
 * Retrieve queue elements from the wait queue.
 * Always called at splvm(). Honor protocol of
 * first calling directly only waiters who are
 * waiting at pri or greater, using softcall
 * to call everybody else (if resources are
 * still left).
 */

static void
mbq_retrieve(pri)
	int pri;
{
	register struct mbq_elem *q, *pq;
	int stat;

	if ((q = mbwaitq) == (struct mbq_elem *) NULL)
		return;

	pq = (struct mbq_elem *) NULL;
	stat = 0;

	do {
		if (q->mbq_pri >= pri) {
			register func_t func;
			register caddr_t arg;

			/*
			 * save a copy of the function/argument pair
			 */

			func = q->mbq_funcp;
			arg = q->mbq_arg;

			/*
			 * Snip the queue element out of the chain
			 * and put it back on the free list before
			 * doing the callback.
			 */

			if (pq) {
				pq->mbq_next = q->mbq_next;
				q->mbq_next = mbqfree;
				mbqfree = q;
				q = pq->mbq_next;
			} else {
				mbwaitq = q->mbq_next;
				q->mbq_next = mbqfree;
				mbqfree = q;
				q = mbwaitq;
			}

			mbqlen -= 1;

			/*
			 * and call the function specified
			 */

			stat = (*(func)) (arg);

			/*
			 * If we ran out again, all bets are off
			 */
			if (stat == DVMA_RUNOUT)
				break;
		} else {
			pq = q;
			q = q->mbq_next;
		}

	} while (q != (struct mbq_elem *) NULL);

	if (stat != DVMA_RUNOUT && mbwaitq != (struct mbq_elem *) NULL) {
		softcall(mbq_softcall, (caddr_t) 0);
	}
}

/*ARGSUSED*/
static int
mbq_softcall(arg)
caddr_t arg;
{
	register s = splvm();
	mbs.m_softcall++;
	mbq_retrieve(0);
	(void) splx(s);
}

/*
 * Put swab here, as mb_machdep.c is compiled common to
 * both sun4c and sun{2, 3, 4} builds.
 */
/*
 * Swap bytes in 16-bit [half-]words
 * for going between the 11 and the interdata
 */
void
swab(pf, pt, n)
	register caddr_t pf, pt;
	register int n;
{
	register char temp;

	n = (n+1)>>1;

	while (--n >= 0) {
		temp = *pf++;
		*pt++ = *pf++;
		*pt++ = temp;
	}
}

/*
 * Non buffer setup interface... set up a buffer and call mb_mapalloc.
 * If DVMA runs out. waitfp is queued and invoked later when space becomes
 * available.
 */
int
mb_nbmapalloc(map, addr, bcnt, flags, waitfp, arg)
	struct map *map;
	caddr_t addr;
	int bcnt, flags;
	func_t waitfp;
	caddr_t arg;
{
	struct buf mbbuf;

	bzero((caddr_t)&mbbuf, sizeof (mbbuf));
	mbbuf.b_un.b_addr = addr;
	mbbuf.b_flags = B_BUSY;
	mbbuf.b_bcount = bcnt;
	return (mb_mapalloc(map, &mbbuf, flags, waitfp, arg));
}
