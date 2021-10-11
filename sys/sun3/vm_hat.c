/*	@(#)vm_hat.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988-90 by Sun Microsystems, Inc.
 */

/*
 * VM - Hardware Address Translation management.
 *
 * This file implements the machine specific hardware translation
 * needed by the VM system.  The machine independent interface is
 * described in <vm/hat.h> while the machine dependent interface
 * and data structures are described in <machine/vm_hat.h>.  For
 * Sun computers, we actually share this same source for more than
 * one architecture (Sun-2, Sun-3, and Sun-4), since the static
 * segment and page map mmu's are very similar.  For this reason,
 * this file is located in the sun directory and we use ifdef's
 * for the few cases where they differ enough to be noticed here.
 * The actual loading of the hardware registers is done at the mmu
 * layer which is different for each Sun architecture type.  In
 * reality we probably need a more general cross architecture
 * sharing structure so that we can more easily share certain code
 * (like this file) across SOME Sun architectures without giving
 * the idea that the file MUST be shared across all Sun architectures.
 *
 * The hat layer manages the address translation hardware as a cache
 * driven by calls from the higher levels in the VM system.  Nearly
 * all the details of how the hardware is managed sound be invisible
 * above this layer except for miscellaneous machine specific functions
 * (e.g. mapin/mapout) that work in conjunction with this code.  Other
 * than a small number of machine specific places, the hat data
 * structures seen by the higher levels in the VM system are opaque
 * and are only operated on by the hat routines.  Each address space
 * contains a struct hat and a page contains an opaque pointer which
 * is used by the hat code to hold a list of active translations to
 * that page.
 */

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/debug.h>
#include <sys/user.h>			/* for u_ru.ru_minflt */
#include <sys/trace.h>
#include <sys/systm.h>

#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/mmu.h>
#ifdef IOC
#include <machine/iocache.h>
#endif

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/page.h>
#include <vm/mp.h>
#include <vm/rm.h>
#include <vm/seg_u.h>
#include <vm/seg_vn.h>
#include <vm/vpage.h>

/*
 * Private vm_hat data structures
 */
#ifdef KMON_DEBUG
static	kmon_t ctx_lock;		/* locks ctxs[] */
#ifdef MMU_3LEVEL
static  kmon_t sme_lock;		/* locks both smgrps[] and sments[] */
#endif MMU_3LEVEL
static	kmon_t pme_lock;		/* locks both pmgrps[] and pments[] */
#endif /*KMON_DEBUG*/

static	struct ctx *ctxhand;
#ifdef MMU_3LEVEL
static	struct smgrp *smgrphand;
static	struct smgrp *smgrpmin;
static	struct smgrp *smgrpfree;
#endif MMU_3LEVEL
static	struct pmgrp *pmgrphand;
static	struct pmgrp *pmgrpmin;
static	struct pmgrp *pmgrpfree;

static	struct pmgrp *hat_getpmg(/* addr, as */);
static	void hat_pteunload(/* pmg, pme, addr, flags */);
static	void hat_ptesync(/* pp, pmg, pme, addr, flags */);


static	void hat_pmgfree(/* pmg */);
static	void hat_pmglink(/* pmg, as, addr */);
static	struct pmgrp *hat_pmgalloc(/* seg, addr */);
static	void hat_wrongpmg(/* pmg, addr, as */);
#ifdef MMU_3LEVEL
static	struct smgrp *hat_getsmg(/* addr */);
static	void hat_smgfree(/* smg */);
static	void hat_smglink(/* smg, as, addr */);
static	void hat_smgalloc(/* as, addr, pmg */);
#endif MMU_3LEVEL

/*
 * Semi-private vm_hat data structures.  Other machine specific
 * routines need to access these however.
 */
u_short	ctx_time;			/* used by <machine/vax.s> */

#if !defined(sun4) && !defined(sun4c)
struct ctx ctxs[MNCTXS];		/* used by <machine/mmu.c> */
struct pmgrp pmgrps[MNPMGRPS];		/* used by <machine/mmu.c> */
struct pment pments[MNPMENTS];		/* used by <machine/machdep.c> */

#define	hat_xfree(as)	hat_xfree_other(as)
#define	init_smgs()

#else

struct	ctx *ctxs, *ctxsNCTXS;		/* used by <machine/mmu.c> */
struct	pmgrp *pmgrps;			/* used by <machine/mmu.c> */
struct	pment *pments, *pmentsNPMENTS;	/* used by <machine/machdep.c> */

#endif !sun4 && !sun4c

#ifdef MMU_3LEVEL
struct	smgrp *smgrps;			/* used by <machine/mmu.c> */
struct	sment *sments, *smentsNSMENTS;
#define	hat_xfree(as)	hat_xfree_sun4(as)
#else
#define	init_smgs()
#define	hat_xfree(as)	hat_xfree_other(as)
#define	hat_smginit()
#define	hat_smglink(smg, as, addr)
#define	hat_smgalloc(as, addr, pmg)
#define	hat_smgfree(smg)
#define	hat_smgreserve(seg, addr)
#define	vac_stolen_ctxflush()	vac_ctxflush()
#endif /* MMU_3LEVEL */


/*
 * hat layer statistics.
 *
 * XXX - this is just a start at collecting some interesting
 * statistics.  undoubtedly there are many more statistics
 * that can and should be collected.
 */
struct hatcnt {
	int	hc_ctxalloc;	/* number of context allocations */
	int	hc_ctxstolen;	/* number of contexts stolen from other as's */
	int	hc_pmghave;	/* number of pmg_allocs that already have pmg */
	int	hc_pmgalloc;	/* number of pmg allocations */
	int	hc_pmgstolennoctx; /* pmgs stolen from as's with no ctx */
	int	hc_pmgstolenctx; /* pmgs stolen from other as's with a ctx */
	int	hc_smghave;	/* number of smg_allocs that already have smg */
	int	hc_smgalloc;	/* number of smg allocations */
	int	hc_smgstolennoctx; /* smgs stolen from procs with no ctx */
	int	hc_smgstolenctx; /* smgs stolen from other procs with a ctx */
	int	hc_ctxpmgs;	/* number of pmgs found on as at ctxalloc */
} hatcnt;

struct cache_stats {
	int cs_ionc;		/* non-cached IO translations */
	int cs_ioc;		/* cached IO translations */
	int cs_knc;		/* non-cached kernel translations */
	int cs_kc;		/* cached kernel translations */
	int cs_unc;		/* non-cached user translations */
	int cs_uc;		/* cached user translations */
	int cs_other;		/* other non-type 0 pages */
	int cs_skip;		/* # of skipped vac changes (IO & upages) */
	int cs_kchange;		/* # of kernel cached to non-cached changes */
	int cs_uchange;		/* # of user cached to non-cached changes */
	int cs_unloadfix;	/* # of unload's that made pages cachable */
	int cs_unloadnofix;	/* # of  " that didn't made pages cachable */
} cache_stats;
extern char DVMA[];		/* addresses in kas above DVMA are for "IO" */

/*
 * Machine specific public data structures.
 */
struct as kas;
struct ctx *kctx;
struct pmgrp *pmgrp_invalid;
#ifdef MMU_3LEVEL
struct smgrp *smgrp_invalid;
#endif /* MMU_3LEVEL */

/*
 * XXX - should be made more efficent by using pment
 * indexing instead of virtual address checking.
 */
#define	VAC_MASK(a) (((u_int)(a) & MMU_PAGEMASK) & (shm_alignment - 1))
#define	VAC_ALIGNED(a1, a2) ((VAC_MASK(a1) ^ VAC_MASK(a2)) == 0)

/*
 * These macros allow us to walk the translation list in a sane manner.
 * The list walking must be protected, and if the translation we land
 * on is interrupt replaceable, all work on that translation must also
 * be protected. This complexity is hidden in the macros. NOTE that these
 * macros assume that variables "pme", "pmg", "pp", and "s" are declared as
 * locals in the calling function. The sequence for use is as follows:
 *
 *	PP_LIST_OPEN
 *	{
 * 		do operations;
 *		PP_LIST_NEXT(next item)
 *	}
 *	PP_LIST_CLOSE
 *
 * NOTE that you CANNOT put a semicolon after PP_LIST_OPEN or it will not
 * function.
 */
#define	PP_LIST_OPEN	s = splvm(); \
			pmg = (struct pmgrp *)NULL; \
			PP_LIST_NEXT((struct pment *)pp->p_mapping) \
			while (pme)

#define	PP_LIST_NEXT(x)	if (s < 0) \
			    s = splvm(); \
			if (pmg) \
			    pmg->pmg_keepcnt--; \
			pme = x; \
			if (pme) { \
			    pmg = &pmgrps[(pme-pments)>>NPMENTPERPMGRPSHIFT]; \
			    pmg->pmg_keepcnt++; \
			    if (!pme->pme_intrep) { \
				(void) splx(s); \
				s = -1; \
			    } \
			}

#define	PP_LIST_CLOSE	(void) splx(s);

/*
 *
 * The next set of routines implements the machine
 * independent hat interface described in <vm/hat.h>
 *
 */

/*
 * Initialize the hardware address translation structures.
 * Called by startup.
 */
void
hat_init()
{
	register struct ctx *ctx;
	register struct pmgrp *pmg;
	register struct pment *pme;
	register int i;

	i = 0;
	for (ctx = ctxs; ctx < &ctxs[NCTXS]; ctx++)
		ctx->c_num = i++;
	ctxhand = ctxs;

#ifdef MMU_3LEVEL
	if (mmu_3level) {
		register struct smgrp *smg;
		register struct sment *sme;

		i = 0;
		sme = sments;
		for (smg = smgrps; smg < &smgrps[NSMGRPS]; smg++) {
			smg->smg_num = i++;
			smg->smg_sme = sme;
			sme += NSMENTPERSMGRP;
		}
		smgrps[SMGRP_INVALID].smg_lock = 1;	/* lock invalid smgrp */
		smgrps[SMGRP_INVALID].smg_keepcnt++;
		smgrp_invalid = &smgrps[SMGRP_INVALID];
		smgrphand = smgrps;
	}
#endif MMU_3LEVEL

	i = 0;
	pme = pments;
	for (pmg = pmgrps; pmg < &pmgrps[NPMGRPS]; pmg++) {
		pmg->pmg_num = i++;
		pmg->pmg_pme = pme;
		pme += NPMENTPERPMGRP;
	}
	pmgrps[PMGRP_INVALID].pmg_lock = 1;	/* lock invalid pmgrp */
	pmgrps[PMGRP_INVALID].pmg_keepcnt++;
	pmgrp_invalid = &pmgrps[PMGRP_INVALID];
	pmgrphand = pmgrps;

	/*
	 * For now, we just grab a context and keep it locked, as well
	 * as locking all of the kernel which was loaded into memory.
	 */
	kctx = &ctxs[KCONTEXT];
	kctx->c_lock = 1;
	kctx->c_as = &kas;
	kas.a_hat.hat_ctx = kctx;
}

/*
 * Free all the hat resources held by an address space.
 * Called from as_free when an address space is being
 * destroyed and when it is to be "swapped out".
 *
 * XXX - should we do anything about locked translations here?
 */
void
hat_free(as)
	register struct as *as;
{
	register struct ctx *ctx;

	kmon_enter(&ctx_lock);

again:
	ctx = as->a_hat.hat_ctx;
	if (ctx == NULL)
		goto out;
	if (ctx->c_lock) {
		kcv_wait(&ctx_lock, (char *)ctx);
		goto again;
	}
	mmu_setctx(ctx);

	/*
	 * free the rest of the hardware mapping resources now
	 */
	hat_xfree(as);

	as->a_hat.hat_ctx = NULL;
	ctx->c_as = NULL;
	kcv_broadcast(&ctx_lock, (char *)ctx);
out:

	kmon_exit(&ctx_lock);

	/*
	 * If we didn't have a context, free up the rest of the hardware
	 * mapping resources now
	 */
	if (ctx == NULL)
		hat_xfree(as);

	mmu_setctx(kctx);		/* lost ctx, run in kernel context */
}

/*
 * Set up addr to map to page pp with protection prot.
 */
void
hat_memload(seg, addr, pp, prot, lock)
	struct seg *seg;
	addr_t addr;
	struct page *pp;
	u_int prot;
	int lock;
{
	struct pte pte;

	hat_mempte(pp, prot, &pte);
	hat_pteload(seg, addr, pp, pte, lock ? PTELD_LOCK : 0);
}

/*
 * Cons up a struct pte using the device's pf bits and protection
 * prot to load into the hardware for address addr; treat as minflt.
 */
void
hat_devload(seg, addr, pf, prot, lock)
	struct seg *seg;
	addr_t addr;
	int pf;
	u_int prot;
	int lock;
{
	struct page *pp, *page_numtouserpp();
	union {
		struct pte u_pte;
		int u_pf;
	} apte;
	register int s = -1;

	/*
	 * If the request is to load a page which is really something
	 * for which we have a struct page, then we must be sure we
	 * maintain cache consistency.  However, we don't want to
	 * maintain such consistency for processes just running through
	 * physical memory, so we only pass along the page struct if it's
	 * not in a transition state.  This is why we use page_numtouserpp
	 * instead of page_numtopp here.  This should fix some readers
	 * of /dev/mem, but also allow those using the window system lock
	 * structures to work right too.
	 */
	if ((pf & PGT_MASK) == PGT_OBMEM) {
		s = splvm();
		pp = page_numtouserpp((u_int)(pf & PG_PFNUM));
	} else
		pp = NULL;
	apte.u_pf = pf & PG_PFNUM;
	apte.u_pte.pg_v = 1;
	apte.u_pte.pg_prot = hat_vtop_prot(prot);
	hat_pteload(seg, addr, pp, apte.u_pte, lock ? PTELD_LOCK : 0);
	u.u_ru.ru_minflt++;
	if (s != -1) {
		(void) splx(s);
	}
}

/*
 * Release one hardware address translation lock on the given address.
 * For the Sun MMU, this means decrementing the counter on the pmgrp.
 */
void
hat_unlock(seg, addr)
	struct seg *seg;
	addr_t addr;
{
	register struct pmgrp *pmg;
	register int s;

	hat_setup(seg->s_as);
	pmg = hat_getpmg(addr, seg->s_as);
	if (pmg == pmgrp_invalid || pmg->pmg_keepcnt < 2)
		panic("hat_unlock");
#ifdef VAC
	if (vac && pmg->pmg_keepcnt == 2) {
		register struct pment *pme = pmg->pmg_pme;
		register int cnt;
		struct page *pp;

		/*
		 * Now check to see if we now can cache any non-cached pages.
		 * For now, we use the simple minded algorithm and just
		 * unload any locked of the locked translations if the
		 * corresponding page is currently marked as non-cachable.
		 * This situation doesn't happen all the much, so the
		 * efficency doesn't have to be all that great.
		 */
		for (cnt = 0; cnt < NPMENTPERPMGRP; cnt++, pme++) {
			if (pme->pme_valid && (pp = pme->pme_page) != NULL &&
			    pp->p_nc) {
				hat_pteunload(pmg, pme, (addr_t)NULL,
				    HAT_RMSYNC);
				if (pp->p_nc) {
					/*
					 * We lost - unloading the mmu
					 * translation wasn't enough to
					 * make the page cacheable again.
					 */
					cache_stats.cs_unloadnofix++;
				} else {
					/*
					 * We won - unloading the mmu
					 * translation made the page
					 * cacheable again.
					 */
					cache_stats.cs_unloadfix++;
				}
			}
		}
	}
#endif VAC
	s = splvm();
	pmg->pmg_keepcnt -= 2;	/* once for hat_getpmg, once for unlock */
	(void) splx(s);
}

/*
 * Change the protections in the virtual address range
 * given to the specified virtual protection.  If
 * vprot == ~PROT_WRITE, then all the write permission
 * is taken away for the current translations, else if
 * vprot == ~PROT_USER, then all the user permissions
 * are takem away for the current translations, otherwise
 * vprot gives the new virtual protections to load up.
 *
 * addr and len must be MMU_PAGESIZE aligned.
 */
void
hat_chgprot(seg, addr, len, vprot)
	struct seg *seg;
	addr_t addr;
	u_int len;
	u_int vprot;			/* virtual page protections */
{
	register addr_t a, ea;
	register struct pmgrp *pmg = NULL;
	register u_int pprot;		/* physical page protections */
	register int newprot;
	struct pte pte;

	if (vprot != ~PROT_WRITE && vprot != ~PROT_USER)
		pprot = hat_vtop_prot(vprot);
	hat_setup(seg->s_as);
	for (a = addr, ea = addr + len; a < ea; a += MMU_PAGESIZE) {
		if (pmg == NULL || ((u_int)a & (PMGRPSIZE - 1)) < MMU_PAGESIZE){
			if (pmg) {
				register int s;
				s = splvm();
				pmg->pmg_keepcnt--;
				(void) splx(s);
			}
			pmg = hat_getpmg(a, seg->s_as);
			if (pmg == pmgrp_invalid) {
				/*
				 * Bump up `a' to avoid checking all
				 * the pte's in the invalid pmgrp.
				 */
				a = (addr_t)((u_int)a & ~(PMGRPSIZE - 1)) +
				    PMGRPSIZE - MMU_PAGESIZE;
				continue;
			}
		}
		mmu_getpte(a, &pte);
		if (!pte_valid(&pte))
			continue;
		if (vprot == ~PROT_WRITE) {
			switch (pte.pg_prot) {
			case KW:
				pprot = KR;
				newprot = 1;
				break;
			case UW:
				pprot = UR;
				newprot = 1;
				break;
			default:
				newprot = 0;
				break;
			}
		} else if (vprot == ~PROT_USER) {
#ifdef sun2
			/* XXX - need a better way to do this */
			if (pte.pg_prot & 07) {
				pprot = pte.pg_prot & ~07;
				newprot = 1;
			} else {
				newprot = 0;
			}
#else sun2
			switch (pte.pg_prot) {
			case UW:
				pprot = KW;
				newprot = 1;
				break;
			case UR:
				pprot = KR;
				newprot = 1;
				break;
			default:
				newprot = 0;
				break;
			}
#endif sun2
		} else if (pte.pg_prot != pprot) {
			newprot = 1;
		} else {
			newprot = 0;
		}
		if (newprot) {
			pte.pg_prot = pprot;
#ifdef VAC
			if (vac && !pte.pg_nc && pte.pg_r)
				vac_pageflush(a);
#endif VAC
			mmu_setpte(a, pte);
		}
	}
	if (pmg != NULL) {
		register int s;
		s = splvm();
		pmg->pmg_keepcnt--;
		(void) splx(s);
	}
}

/*
 * Associate all the mappings in the range [addr..addr+len) with
 * segment seg. Since we don't cache segments in this hat implementation,
 * this routine is a noop.
 */
/*ARGSUSED*/
void
hat_newseg(seg, addr, len, nseg)
	struct seg *seg;
	addr_t addr;
	u_int len;
	struct seg *nseg;
{

	return;
}

/*
 * Unload all the mappings in the range [addr..addr+len).
 *
 * addr and len must be MMU_PAGESIZE aligned.
 */
void
hat_unload(seg, addr, len)
	struct seg *seg;
	addr_t addr;
	u_int len;
{
	register addr_t a;
	register struct pment *pme;
	register struct pmgrp *pmg = NULL;
	register int s;
	addr_t ea;

	if (seg->s_as->a_hat.hat_pmgrps == NULL) {
		/*
		 * If there are no allocated pmgrps for this
		 * address space, we don't want to do anything.
		 */
		return;
	}

	hat_setup(seg->s_as);
	for (a = addr, ea = addr + len; a < ea; a += MMU_PAGESIZE) {
		if (pmg == NULL || ((u_int)a & (PMGRPSIZE - 1)) < MMU_PAGESIZE){
			if (pmg != NULL) {
				s = splvm();
				pmg->pmg_keepcnt--;
				(void) splx(s);
			}
			pmg = hat_getpmg(a, seg->s_as);
			if (pmg == pmgrp_invalid) {
				/*
				 * Bump up `a' to avoid checking all
				 * the pme's in the invalid pmgrp.
				 */
				a = (addr_t)((u_int)a & ~(PMGRPSIZE - 1)) +
				    PMGRPSIZE - MMU_PAGESIZE;
				continue;
			}
			pme = &pmg->pmg_pme[mmu_btop(a - pmg->pmg_base)];
		} else {
			pme++;
		}
		/*
		 * Throw out the mapping.
		 */
		if (pme->pme_nosync) {
			if (pmg->pmg_keepcnt < 2)
				panic("hat_unload - pmg not kept");
			s = splvm();
			pmg->pmg_keepcnt--;
			(void) splx(s);
			hat_pteunload(pmg, pme, a, HAT_NOSYNC | HAT_VADDR);
		} else {
			hat_pteunload(pmg, pme, a, HAT_RMSYNC | HAT_VADDR);
		}
	}
	s = splvm();
	pmg->pmg_keepcnt--;
	(void) splx(s);
}

/*
 * Unload all the hardware translations that map page `pp'.
 */
void
hat_pageunload(pp)
	register struct page *pp;
{
	register struct pmgrp *pmg;
	register struct pment *pme;
	int s;

	PP_LIST_OPEN
	{
		hat_pteunload(pmg, pme, (addr_t)NULL, HAT_RMSYNC);
		PP_LIST_NEXT((struct pment *)pp->p_mapping)
	}
	PP_LIST_CLOSE
}

/*
 * Get all the hardware dependent attributes for a page struct
 */
void
hat_pagesync(pp)
	struct page *pp;
{
	register struct pment *pme;
	register struct pmgrp *pmg;
	int s;

	PP_LIST_OPEN
	{
		/*
		 * Get page dependent info from hardware for
		 * each translation, but don't unload them.
		 */
		hat_ptesync(pp, pmg, pme, (addr_t)NULL, HAT_RMSYNC);
		PP_LIST_NEXT(PMENXT_PTR(pme->pme_next))
	}
	PP_LIST_CLOSE
}

/*
 * Returns the page frame number for a given kernel virtual address.
 */
u_int
hat_getkpfnum(addr)
	addr_t	addr;
{
	struct	pte pte;

	mmu_getkpte(addr, &pte);
	return (MAKE_PFNUM(&pte));
}



/*
 * End of machine independent interface routines.
 *
 * The next few routines implement some machine dependent functions
 * need for the Sun MMU.  Note that each hat implementation can define
 * whatever additional interfaces that make sense for that machine
 * These routines are defined in <machine/vm_hat.h>.
 *
 * Start machine specific interface routines.
 */


/*
 * This routine is called for kernel initialization
 * to cause a pmgrp to be reserved w/o any unloading,
 * links the pmgrp into the address space (if not already there),
 * and return with the pmgrp in question leaving its keepcnt incremented.
 */
void
hat_pmgreserve(seg, addr)
	struct seg *seg;
	addr_t addr;
{
	register struct as *as = seg->s_as;
	register struct pmgrp *pmg;

	pmg = hat_getpmg(addr, as);	/* keeps the pmg for us */

	if (as->a_hat.hat_ctx == NULL ||
	    (pmg->pmg_as != NULL && pmg->pmg_as != as))
		panic("hat_pmgreserve");

	if (pmg != pmgrp_invalid && pmg->pmg_as == NULL) {
		hat_pmglink(pmg, as, addr);
		hat_smgreserve(seg, addr);
	}
}

/*
 * Initialize all the unlocked pmgs to have invalid pme's
 * and add them to the free list.
 * This routine is called during startup after all the
 * kernel pmgs have been reserved.  This routine will
 * also set the pmgrpmin variable for use in hat_pmgalloc.
 */
void
hat_pmginit()
{
	register struct pmgrp *pmg;
	register addr_t addr;
	int s;

	s = splvm();
	kmon_enter(&pme_lock);

	for (pmg = pmgrps; pmg < &pmgrps[NPMGRPS]; pmg++) {
		if (pmg->pmg_lock || pmg->pmg_keepcnt != 0)
			continue;

		if (pmgrpmin == NULL)
			pmgrpmin = pmg;

		mmu_settpmg(SEGTEMP, pmg);

		for (addr = SEGTEMP; addr < SEGTEMP + PMGRPSIZE;
		    addr += PAGESIZE)
			mmu_setpte(addr, mmu_pteinvalid);

		pmg->pmg_next = pmgrpfree;
		pmgrpfree = pmg;
	}
	pmgrphand = pmgrpmin;
	mmu_pmginval(SEGTEMP);

	kmon_exit(&pme_lock);
	(void) splx(s);

	hat_smginit();
}

/*
 * Set addr in segment seg to use pte to (possibly) map to page pp.
 * This is the common routine used for hat_memload and hat_devload
 * in addition to the machine dependent mapin implementation.
 */
void
hat_pteload(seg, addr, pp, pte, flags)
	struct seg *seg;
	addr_t addr;
	struct page *pp;
	struct pte pte;
	int flags;
{
	register struct pment *ppme;
	register struct pmgrp *ppmg;
	int s;

	if (pp != NULL && pp->p_free)
		panic("hat_pteload free page");
	ppmg = hat_pmgalloc(seg, addr);
	ppme = &ppmg->pmg_pme[mmu_btop(addr - ppmg->pmg_base)];

	/*
	 * We must be sure that setting the pte and adding to the list
	 * of mappings is atomic.
	 */
	s = splvm();
#ifdef VAC
	/*
	 * If there's no page structure associated with this mapping,
	 * or the vac is turned off, or the page is non-cacheable,
	 * then force the mapping to be non-cached.
	 */
	if (pp == NULL || !vac || pp->p_nc)
		pte.pg_nc = 1;
#endif VAC
	if (ppme->pme_valid) {
		/*
		 * Reloading a translation - be sure to preserve the
		 * existing ref and mod bits for this translation.
		 *
		 * XXX - should cache all the attributes of a loaded
		 * translation in the pme structure so that we can
		 * avoid reloading all together unless something
		 * is actually going to change.
		 */
		struct pte opte;

		mmu_getpte(addr, &opte);
		pte.pg_r = opte.pg_r;
		pte.pg_m = opte.pg_m;
	} else {
		ppme->pme_valid = 1;
	}

#ifdef IOC
	if (ioc) {
		struct pte *p = &pte;

		if (flags & PTELD_IOCACHE) {
			ioc_pteset(p);
		}
		ioc_mbufset(p, addr);
	}
#endif IOC

	mmu_setpte(addr, pte);		/* load the pte into hardware mmu */

	/*
	 * Check to see if this pme needs to be added
	 * to the list of pme's mapping this page.
	 */
	if (pp != ppme->pme_page) {
		if (ppme->pme_page != NULL)
			panic("hat_pteload");
		ASSERT(pp != NULL && ppme->pme_page == NULL);

		ppme->pme_page = pp;
		ppme->pme_next = PMENXT_INDEX(pp->p_mapping);
		pp->p_mapping = (caddr_t)ppme;
		(void) splx(s);

		seg->s_as->a_rss += 1;
#ifdef VAC
		/*
		 * If (vac) active, then check for conflicts.
		 * A conflict exists if the new and existent mappings
		 * do not match in their "shm_alignment" fields
		 * XXX and one of them is writable XXX.  If conflicts
		 * exist, the extant mappings are flushed UNLESS
		 * one of them is locked.  If one of them is locked,
		 * then the mappings are flushed and converted to
		 * non-cacheable mappings [must be deconverted in
		 * hat_pteunload].
		 * XXX	need to store protections in pme
		 * 	to employ writable optimization.
		 */
		if (vac && !pp->p_nc) {
			struct pmgrp *pmg;	/* temporary pmg */
			struct pment *pme;	/* temporary pme */
			struct pmgrp *pmgpc;	/* user's text pmg */
			struct pmgrp *pmgsp;	/* user's stack pmg */
			int ccf;		/* cache conflict found flag */
			int first = 1;

			/*
			 * mappings to the user's current stack and
			 * text locations must be locked in memory,
			 * or we run the risk of getting into an
			 * infinite paging loop if the program tries
			 * to read the physical pages containing either
			 * via a mapping that is not cache aliased.
			 */
			s = splvm();
			if ((u.u_ar0 != (int *)0) && (u.u_ar0[SP] != 0)) {
				pmgsp = mmu_getpmg ((addr_t)u.u_ar0[SP]);
				if (pmgsp != (struct pmgrp *)0)
					pmgsp->pmg_keepcnt ++;
			} else
				pmgsp = (struct pmgrp *)0;
			if ((u.u_ar0 != (int *)0) && (u.u_ar0[PC] != 0)) {
				pmgpc = mmu_getpmg ((addr_t)u.u_ar0[PC]);
				if (pmgpc != (struct pmgrp *)0)
					pmgpc->pmg_keepcnt ++;
			} else
				pmgpc = (struct pmgrp *)0;
			(void)splx(s);

			ccf = 0;
			PP_LIST_OPEN
			{
				if (pme == ppme) {
					PP_LIST_NEXT(PMENXT_PTR(pme->pme_next))
					continue;
				}
				if (first && VAC_ALIGNED(addr, pmg->pmg_base +
				    mmu_ptob(pme - pmg->pmg_pme))) {
					PP_LIST_NEXT((struct pment *)NULL)
					continue;
				}
				first = 0;
				/*
				 * Compare keep to 1 because
				 * list walker keeps it too.
				 */
				if (pmg->pmg_lock || pmg->pmg_keepcnt > 1) {
					ccf = 1;
					PP_LIST_NEXT(PMENXT_PTR(pme->pme_next))
					continue;
				}
				hat_pteunload(pmg, pme, (addr_t)NULL,
				    HAT_RMSYNC);
				PP_LIST_NEXT((struct pment *)pp->p_mapping)
			}
			PP_LIST_CLOSE

			/*
			 * Scan complete; we can release the locks
			 * on the text and stack pages now.
			 */
			if (pmgsp != (struct pmgrp *)0)
				pmgsp->pmg_keepcnt --;
			if (pmgpc != (struct pmgrp *)0)
				pmgpc->pmg_keepcnt --;

			if (ccf) {
				pte.pg_nc = 1;
				pp->p_nc = 1;
				/*
				 * This time we don't exclude our translation,
				 * so it will get remarked noncacheable.
				 */
				PP_LIST_OPEN
				{
					hat_ptesync(pp, pmg, pme, (addr_t)NULL,
					    HAT_NCSYNC);
					PP_LIST_NEXT(PMENXT_PTR(pme->pme_next))
				}
				PP_LIST_CLOSE
			}
		}
#endif VAC
	} else {
		(void) splx(s);
		if (pp != NULL &&
		    ((ppme->pme_intrep != ((flags & PTELD_INTREP) != 0)) ||
		    (ppme->pme_nosync != ((flags & PTELD_NOSYNC) != 0))))
			panic("pteload - remap flags");
#ifdef VAC
		/*
		 * Reloading a pte for an already mapped page.  If
		 * the page is "real", then if we've got a VAC, then
		 * flush the cache to ensure no protection mismatches
		 * between cache and MMU.
		 * XXX - need to store protections in pme to avoid this.
		 */
		if ((pp != NULL) && vac && pte.pg_r)
			vac_pageflush(addr);
#endif VAC
	}
	ppme->pme_intrep = (flags & PTELD_INTREP) != 0;
	ppme->pme_nosync = (flags & PTELD_NOSYNC) != 0;

	/* keep some statistics on the cache-ability of the translation */
	if (pte.pg_type == OBMEM) {
		if (seg->s_as == &kas) {
			if (addr >= DVMA) {
#ifdef VAC
				if (!pte.pg_nc)
					cache_stats.cs_ioc++;
				else
#endif VAC
					cache_stats.cs_ionc++;
			} else {
#ifdef VAC
				if (!pte.pg_nc)
					cache_stats.cs_kc++;
				else
#endif VAC
					cache_stats.cs_knc++;
			}
		} else {
#ifdef VAC
			if (!pte.pg_nc)
				cache_stats.cs_uc++;
			else
#endif VAC
				cache_stats.cs_unc++;
		}
	} else {
		cache_stats.cs_other++;
	}

	if ((flags & PTELD_LOCK) == 0) {
		s = splvm();
		ppmg->pmg_keepcnt--;	/* decr lock count from hat_pmgalloc */
		(void) splx(s);
	}
}

void
hat_mempte(pp, vprot, ppte)
	struct page *pp;
	u_int vprot;
	register struct pte *ppte;
{

	*ppte = mmu_pteinvalid;
	ppte->pg_prot = hat_vtop_prot(vprot);
	ppte->pg_v = 1;
	ppte->pg_type = OBMEM;
	ppte->pg_pfnum = page_pptonum(pp);
}

/*
 * Allocate a ctx for use by the specified address space.
 * If there are any pmgrps associated with the hat, load
 * them up after we get the ctx.
 */
void
hat_getctx(as)
	struct as *as;
{
	register struct ctx *ctx, *sctx = NULL;
	register struct pmgrp *pmg;
#ifdef MMU_3LEVEL
	register struct smgrp *smg;
#endif
	register u_short tt = 0;

	if (as->a_hat.hat_ctx)
		return;

	kmon_enter(&ctx_lock);
	hatcnt.hc_ctxalloc++;

	/* find a free ctx or an old one */
	for (;;) {
		ctx = ctxhand;
		do {
			ctx++;
			if (ctx == &ctxs[NCTXS])
				ctx = ctxs;
			if (ctx->c_lock != 0)		/* can't touch */
				continue;
			if (ctx->c_as == NULL) {	/* no as - use it */
				ctxhand = ctx;
				goto found;
			}
			if (sctx == NULL || ctx->c_time <= tt) {
				sctx = ctx;		/* new "best" ctx */
				tt = ctx->c_time;
			}
		} while (ctx != ctxhand);

		if (sctx != NULL) {
			ctxhand = ctx = sctx;
			goto found;
		}

		panic("hat_getctx - no ctx's");
	}
found:

	kmon_exit(&ctx_lock);

	mmu_setctx(ctx);

	if (ctx->c_as) {
		vac_ctxflush();
		/* invalidate any pmgrps already loaded for this ctx */
		for (pmg = ctx->c_as->a_hat.hat_pmgrps; pmg != NULL;
		    pmg = pmg->pmg_next)
			mmu_pmginval(pmg->pmg_base);
		ctx->c_as->a_hat.hat_ctx = NULL;
		hatcnt.hc_ctxstolen++;

#ifdef MMU_3LEVEL
		if (mmu_3level) {
		/* invalidate any smgrps already loaded for this ctx */
			for (smg = ctx->c_as->a_hat.hat_smgrps; smg != NULL;
			    smg = smg->smg_next)
				mmu_smginval(smg->smg_base);
			ctx->c_as->a_hat.hat_ctx = NULL;
		}
#endif
	}

	ctx->c_as = as;
	ctx->c_time = ctx_time++;
	as->a_hat.hat_ctx = ctx;

#ifdef MMU_3LEVEL
	if (mmu_3level) {
		/*
		 * Load up any smgrps already allocated to this hat
		 */
		for (smg = as->a_hat.hat_smgrps; smg != NULL;
		    smg = smg->smg_next)
			mmu_setsmg(smg->smg_base, smg);
	}
#endif

	/* load up any pmgrps already allocated to this hat */
	for (pmg = as->a_hat.hat_pmgrps; pmg != NULL; pmg = pmg->pmg_next) {
		mmu_setpmg(pmg->pmg_base, pmg);
		hatcnt.hc_ctxpmgs++;
	}
}

/*
 * Used to lock down hat resources for an address range. In this implementation,
 * this means locking down the necessary pmegs. This currently works only
 * for kernel addresses.
 *
 * addr and len must be MMU_PAGESIZE aligned.
 */
void
hat_reserve(seg, addr, len)
	struct seg *seg;
	addr_t addr;
	u_int len;
{
	register addr_t a;
	addr_t ea;

	if (seg->s_as != &kas)
		panic("hat_reserve");
	for (a = addr, ea = addr + len; a < ea; a += MMU_PAGESIZE)
		(void) hat_pmgalloc(seg, a);
}

u_int
hat_vtop_prot(vprot)
	u_int vprot;
{

	switch (vprot) {
	case 0:
	case PROT_USER:
		/*
		 * Since 0 might be a valid protection,
		 * the caller should not set valid bit
		 * if vprot == 0 to be sure.
		 */
		return (0);
	case PROT_READ:
	case PROT_EXEC:
	case PROT_READ | PROT_EXEC:
		return (KR);
	case PROT_WRITE:
	case PROT_WRITE | PROT_EXEC:
	case PROT_READ | PROT_WRITE:
	case PROT_READ | PROT_WRITE | PROT_EXEC:
		return (KW);
	case PROT_EXEC | PROT_USER:
	case PROT_READ | PROT_USER:
	case PROT_READ | PROT_EXEC | PROT_USER:
		return (UR);
	case PROT_WRITE | PROT_USER:
	case PROT_WRITE | PROT_EXEC | PROT_USER:
	case PROT_READ | PROT_WRITE | PROT_USER:
	case PROT_READ | PROT_WRITE | PROT_EXEC | PROT_USER:
		return (UW);
	default:
		panic("hat_vtop_prot");
		/* NOTREACHED */
	}
}

#if defined(sun4c) && defined(VAC)
/*
 * Flush all possible cache lines mapping the given physical page. This
 * is used for software cache consistency with I/O, to clean the cache
 * of all data subject to I/O.
 */
void
hat_vacsync(pfnum)
	u_int pfnum;
{
	register struct page *pp = page_numtopp(pfnum);
	register struct pmgrp *pmg;
	register struct pment *pme;
	struct pte tpte;
	int s;
	addr_t va;
	struct ctx *ctxsav, *nctx, *curctx;


	/*
	 * If the cache is off, the page isn't memory, or the page is
	 * non-cacheable, then none of the page could be in the cache
	 * in the first place, with the exception that a page frame
	 * for kernel .data or .bss objects could be in the cache,
	 * but will have no page structure.
	 */
	if (!vac) {
		return;
	} else if (pp == (struct page *) NULL) {
		extern u_int kpfn_dataseg, kpfn_endbss;
		if (pfnum >= kpfn_dataseg || pfnum <= kpfn_endbss) {
			extern char etext[];

			/*
			 * In sun4c, the page frame number for the start
			 * of the kernel data segment and the page frame
			 * number for end are latched up in kvm_init().
			 * If a page frame number ends up here, then some-
			 * body is doing i/o to an object in kernel .data
			 * or .bss.
			 *
			 * This is a temporary solution, and it does have
			 * some holes in it. It assumes that the page frame
			 * numbers between kernel .data and end are contiguous.
			 *
			 * As a side note, we could go to the effort of
			 * of reading the kernel pte for the calculated
			 * address to check with the passed page frame
			 * number, but it isn't really worth the effort.
			 */

			va = (addr_t) (roundup((u_int) etext, DATA_ALIGN) +
			    ((pfnum - kpfn_dataseg) << MMU_PAGESHIFT));

			vac_pageflush (va);
		}
		return;
	} else if (pp->p_nc) {
		return;
	}

	curctx = ctxsav = mmu_getctx();
	/*
	 * Walk the list of translations for this page, flushing each
	 * one.
	 */
	PP_LIST_OPEN
	{
		/*
		 * If the translation has no context, it can't be
		 * in the cache.
		 */
		if ((nctx = pmg->pmg_as->a_hat.hat_ctx) != NULL) {
			/*
			 * Calculate the virtual address, switch to the
			 * correct context, and flush the page.
			 */
			va = pmg->pmg_base + mmu_ptob(pme - pmg->pmg_pme);
			if (nctx != curctx) {
				mmu_setctx(nctx);
				curctx = nctx;
			}
			mmu_getpte(va, &tpte);
			if (tpte.pg_r)
				vac_pageflush(va);
		}
		PP_LIST_NEXT(PMENXT_PTR(pme->pme_next))
	}
	PP_LIST_CLOSE
	/*
	 * Restore the original context.
	 */
	if (curctx != ctxsav)
		mmu_setctx(ctxsav);
}
#endif defined(sun4c) && defined(VAC)

#ifdef	sun4c

/*
 * It would be nice if other parity recovery schemes used this mechanism.
 */
/*
 * Kill any processes that use this page. (Used for parity recovery)
 * If we encounter the kernel's address space, give up (return -1).
 * Otherwise, we return 0.
 */
hat_kill_procs(pp, addr)
	struct	page	*pp;
	addr_t	addr;
{
	register struct pmgrp *pmg;
	register struct pment *pme;
	int	s;
	struct	as	*as;
	struct	proc	*p;
	int	result = 0;

	PP_LIST_OPEN
	{
		/*
		 * Find the address space that contains this pment.
		 */
		as = pmg->pmg_as;

		/*
		 * If the address space is the kernel space, then fail.
		 * The memory is corrupted, and the only thing to do with
		 * corrupted kernel memory is die.
		 */
		if (as == &kas) {
			printf("parity recovery: kernel address space\n");
			result = -1;
		}

		/*
		 * Find the proc that uses this address space and kill
		 * it.  Note that more than one process can share the
		 * same address space, if vfork() was used to create it.
		 * This means that we have to look through the entire
		 * process table and not stop at the first match.
		 */
		for (p = allproc; p; p = p->p_nxt) {
			if (p->p_as == as) {
				printf("pid %d killed: parity error\n",
				    p->p_pid);
				uprintf("pid %d killed: parity error\n",
				    p->p_pid);
				psignal(p, SIGBUS);
				p->p_uarea->u_code = FC_HWERR;
				p->p_uarea->u_addr = addr;
			}
		}
		PP_LIST_NEXT(PMENXT_PTR(pme->pme_next))
	}
	PP_LIST_CLOSE

	return (result);
}
#endif	sun4c

/*
 * End machine specific interface routines.
 *
 * The remainder of the routines are private to this module and are used
 * by the routines above to implement a service to the outside caller.
 *
 * Start private routines.
 */

/*
 * Unload a pme.  We call hat_ptesync() to unload the translation
 * then remove the pme from the list of pme's mapping the page.
 * Should always be called with the pmgrp for the pme being held.
 */
static void
hat_pteunload(ppmg, ppme, vaddr, flags)
	struct pmgrp *ppmg;
	register struct pment *ppme;
	addr_t vaddr;
	int flags;
{
	int s;
	register u_short *next;		/* ptr for list removal */
	struct page *pp = ppme->pme_page;
#ifdef VAC
	struct pment *pme;		/* temporary for listwalk */
	struct pmgrp *pmg;		/* temporary for listwalk */
	struct pment *qpme;		/* second temporary for comparison */
	struct pmgrp *qpmg;		/* second temporary for comparison */
	addr_t pa, qa;			/* matching address values */
	int ccf;			/* cache conflict found flag */
	int s2;
#endif VAC

	if (pp != NULL) {
		/*
		 * Remove it from the list of mappings for the page.
		 */
		s = splvm();
		if (ppme == (struct pment *)(pp->p_mapping)) {
			next = &ppme->pme_next;
			pp->p_mapping = (caddr_t)PMENXT_PTR(*next);
			*next = PMENXT_NULL;
		} else {
			if (pp->p_mapping == NULL)
				panic("hat_pteunload - no mappings");
			for (next = &(((struct pment *)
			    (pp->p_mapping))->pme_next);
			    *next != PMENXT_INDEX(ppme);
			    next = &(PMENXT_PTR(*next)->pme_next))
				if (*next == PMENXT_NULL)
					panic("hat_pteunload - no mapping");
			*next = ppme->pme_next;
			ppme->pme_next = PMENXT_NULL;
		}
		(void) splx(s);
		ppme->pme_page = NULL;
		ppmg->pmg_as->a_rss -= 1;
#ifdef VAC
		if (vac && pp->p_nc) {
			ccf = 0;
			PP_LIST_OPEN
			{
				s2 = splvm();
				if ((qpme = PMENXT_PTR(pme->pme_next))
				    != NULL) {
					pa = pmg->pmg_base +
					    mmu_ptob(pme - pmg->pmg_pme);
					qpmg = &pmgrps[(qpme - pments) /
					    NPMENTPERPMGRP];
					qa = qpmg->pmg_base +
					    mmu_ptob(qpme - qpmg->pmg_pme);
					if (!VAC_ALIGNED(pa, qa)) {
						ccf = 1;
						(void) splx(s2);
						PP_LIST_NEXT(NULL)
						continue;
					}
				}
				(void) splx(s2);
				PP_LIST_NEXT(PMENXT_PTR(pme->pme_next))
			}
			PP_LIST_CLOSE
			if (!ccf) {
				/*
				 * No more cache conflict.
				 * Use hat_ptesync to resync.
				 */
				pp->p_nc = 0;
				PP_LIST_OPEN
				{
					hat_ptesync(pp, pmg, pme, (addr_t)NULL,
					    HAT_NCSYNC);
					PP_LIST_NEXT(PMENXT_PTR(pme->pme_next))
				}
				PP_LIST_CLOSE
			}
		}
#endif VAC
	}
	/*
	 * Invalidate the translation.
	 */
	if (ppme->pme_valid) {
		flags |= HAT_INVSYNC;
		hat_ptesync(pp, ppmg, ppme, vaddr, flags);
		ppme->pme_nosync = ppme->pme_intrep = ppme->pme_valid = 0;
	}
}

/*
 * Synchronize the hardware and software of a pte.  Used for updating the
 * hardware nocache bit, the software R & M bits, and invalidating ptes.
 */
static void
hat_ptesync(pp, pmg, pme, vaddr, flags)
	struct page *pp;
	register struct pmgrp *pmg;
	register struct pment *pme;
	addr_t vaddr;
	int flags;
{
	register struct ctx *ctxsav, *nctx;
	register addr_t mapaddr;
	int s, pmg_off;
	struct pte pte, tpte;
	int usetemp = 0;
	int didsetpte = 0;

	if (pme->pme_valid == 0)
		panic("hat_ptesync - invalid pme");
	if (flags & HAT_VADDR) {
		mapaddr = vaddr;
		goto skip;
	}
	pmg_off = mmu_ptob(pme - pmg->pmg_pme);
	vaddr = pmg->pmg_base + pmg_off;
	ctxsav = mmu_getctx();
	/*
	 * We must protect the use of the mapping address,
	 * since it is a shared resource.
	 */
	s = splvm();
	if (pmg->pmg_as->a_hat.hat_ctx == NULL) {
		/*
		 * No ctx - set things up so that the pmgrp
		 * is mapped into a temporary segment.  No
		 * need to do any cache flushing since this
		 * was done when we took the ctx away.  Set
		 * up the mapaddr within the temporary segment.
		 */
		/* XXX - this is disgusting! */
#ifdef sun2
		extern struct ctx *kctx;

		nctx = kctx;
		if (nctx != ctxsav)
			mmu_setctx(nctx);
#else sun2
		nctx = ctxsav;		/* no need to switch context */
#endif sun2
		mmu_settpmg(SEGTEMP, pmg);
		mapaddr = SEGTEMP + pmg_off;
		usetemp = 1;
	} else {
		/*
		 * We have a ctx, make sure we are in running
		 * in the right context.  Set up to use the
		 * virtual address as the mapping address.
		 */
		if ((nctx = pmg->pmg_as->a_hat.hat_ctx) !=
		    ctxsav)
			mmu_setctx(nctx);
		mapaddr = vaddr;
	}
skip:
	if (pp != NULL) {
		if (flags & HAT_RMSYNC) {
			mmu_getpte(mapaddr, &pte);

			/* Call back to inform address space, if turned on */
			if (pmg->pmg_as->a_hatcallback) {
				as_hatsync(pmg->pmg_as, vaddr,
					(u_int) pte.pg_r, (u_int) pte.pg_m,
					(u_int)(flags & HAT_INVSYNC ?
							AHAT_UNLOAD : 0));
			}
			pg_setref(pp, pp->p_ref | pte.pg_r);
			pg_setmod(pp, pp->p_mod | pte.pg_m);

			/*
			 * When you zero the modified bit in the MMU
			 * and leave it set in the cache you may not
			 * get it set in the mmu when the line is
			 * re-written.	Writeback caches perform the
			 * setting of the modified bit for a page in
			 * the MMU on the first write miss that happens
			 * to that page. Subsequent writes don't bother
			 * to set the modified bit because the first
			 * write did it.  Therefore if you are zeroing
			 * the modified bit you must flush the cache
			 * so that subsequent writes, see the modified
			 * bit unset in the cache and write it back to
			 * the MMU.
			 */
#ifdef VAC
			if (vac && pte.pg_r)
				vac_pageflush(mapaddr);
#endif VAC
			pte.pg_r = pte.pg_m = 0;
		}
#ifdef VAC
		else if (flags & HAT_NCSYNC) {
			/*
			 * N.B.  The following test assumes that there
			 * are no user addresses at the same virtual
			 * addresses as DVMA and segu in VAC machines.
			 */
			if (mapaddr >= DVMA || (segu != NULL &&
			    mapaddr >= segu->s_base &&
			    mapaddr < segu->s_base + segu->s_size)) {
				/*
				 * To avoid lots of problems, we don't
				 * try to convert anything from cached
				 * to non-cached (or vice-versa) when
				 * it is being loaded for DVMA use.
				 * Also, we refuse to mess with user
				 * areas since it is impossible to
				 * reliably flush when converting
				 * from cached to non-cached and we
				 * don't want to take any performance
				 * hits from using a non-cached stack.
				 */
				didsetpte = 1;
				cache_stats.cs_skip++;
			} else {
				mmu_getpte(mapaddr, &pte);
				if (vac && !pte.pg_nc && pp->p_nc &&
				    mapaddr == vaddr) {
					int pri, iskas;

					/*
					 * Need to convert from a cached
					 * translation to a non-cached
					 * translation.  There are lots
					 * of potential races here in the
					 * kernel's address space.  If
					 * some clean line ends up in the
					 * cache after it is flushed here
					 * and is then written to, the
					 * Sirius cache system will end
					 * up giving a memory timeout error.
					 *
					 * For now, we assume that between
					 * time that we flush the virtual
					 * address and reset the MMU that
					 * nothing will be getting into
					 * the cache from things like
					 * ethernet (this is questionable).
					 * We also assume that will never
					 * be converting anything from
					 * cached to non-cached in the
					 * kernel for the current stack,
					 * (i.e., the stack can be accessed
					 * safely w/o it being changed from
					 * cached to non-cached), the interrupt
					 * stack, or anything that might be
					 * touched at interrupts above splhigh
					 * (UARTS, level7 profiling).
					 */
					pte.pg_nc = 1;
					didsetpte = 1;
					iskas = pmg->pmg_as == &kas;

					pri = splhigh();

					if (pte.pg_r)
						vac_pageflush(mapaddr);
					mmu_setpte(mapaddr, pte);

#ifdef SUN4_470
#define	CFLUSH_TEST	(iskas && (cpu != CPU_SUN4_470))
#else
#define	CFLUSH_TEST	iskas
#endif
					if (CFLUSH_TEST) {
						mmu_getpte(mapaddr, &tpte);
						/*
						 * Flush the virtual address
						 * again just in case some IO
						 * got in behind our back
						 * above.  Doing this for
						 * iskas only assumes there
						 * is no UDVMA to worry about.
						 */
						if (tpte.pg_r)
							vac_pageflush(mapaddr);
						cache_stats.cs_kchange++;
					} else {
						cache_stats.cs_uchange++;
					}
					(void) splx(pri);
				} else {
					pte.pg_nc = pp->p_nc;
				}
			}
		}
#endif VAC
	}
	if (flags & HAT_INVSYNC) {
#ifdef VAC
		if (vac && mapaddr == vaddr) {
			mmu_getpte(mapaddr, &tpte);
			if (tpte.pg_r)
				vac_pageflush(mapaddr);
		}
#endif VAC
		pte = mmu_pteinvalid;
	}
	if (!didsetpte)
		mmu_setpte(mapaddr, pte);
	if (flags & HAT_VADDR)
		return;
	if (usetemp)
		mmu_settpmg(SEGTEMP, pmgrp_invalid);
	(void) splx(s);
	if (nctx != ctxsav)
		mmu_setctx(ctxsav);
}

int getpmg_check = 1;

/*
 * Allocate a pmgrp to map the specified address.
 * Returns w/ the keepcnt incremented for the particular pmgrp used.
 * First look for something in the free list and then steal one
 * that is currently being used.  Use simple round robin algorithm
 * to find a used one to steal, skipping over the first few that
 * we know are permanently allocated to the kernel, and starting
 * at pmgrpmin.  XXX - should do something closer to LRU.
 */
static struct pmgrp *
hat_pmgalloc(seg, addr)
	struct seg *seg;
	addr_t addr;
{
	register struct as *as = seg->s_as;
	register struct pmgrp *pmg;
	int s;

	/*
	 * Read from the mmu to see if we already have a pmgrp allocated.
	 * If so, keep it and return.  N.B. this is an inline hat_getpmg().
	 */
	hat_setup(as);
	s = splvm();
	if ((pmg = mmu_getpmg(addr)) != pmgrp_invalid) {
		pmg->pmg_keepcnt++;
		hatcnt.hc_pmghave++;
		if (getpmg_check && pmg->pmg_base !=
		    (caddr_t)((u_int)addr & ~(PMGRPSIZE - 1)))
			hat_wrongpmg(pmg, addr, as);
		(void) splx(s);
		return (pmg);
	}

	/*
	 * No pmgrp allocated to this address space contains the pme,
	 * allocate a new pmg for this address space.  First, try
	 * the free list.
	 */
	kmon_enter(&pme_lock);
top:
	if ((pmg = pmgrpfree) == NULL) {
		int try;

		/*
		 * No pmg's free, have to take one from someone.
		 * Take from address spaces with no ctx first.
		 * XXX - could do it with just one pass.
		 */
		pmg = pmgrphand;
		try = 1;
		for (;;) {
			do {
				pmg++;
				if (pmg == &pmgrps[NPMGRPS]) {
					if (pmgrpmin) {
						/* skip some kernel pmgrps */
						pmg = pmgrpmin;
					} else {
						pmg = pmgrps;
					}
				}
				if (pmg->pmg_lock == 0 &&
				    pmg->pmg_keepcnt == 0) {
					/*
					 * On the first try, only take a pmg
					 * from an address space with no ctx.
					 */
					if (try == 1 &&
					    pmg->pmg_as->a_hat.hat_ctx != NULL)
						continue;
					/*
					 * Found a candidate, free
					 * it up and try again.
					 */
					if (try == 1)
						hatcnt.hc_pmgstolennoctx++;
					else
						hatcnt.hc_pmgstolenctx++;
					pmg->pmg_keepcnt++;
					hat_pmgfree(pmg);
					pmgrphand = pmg;
					goto top;
				}
			} while (pmg != pmgrphand);
			/*
			 * Give up after 2 tries.
			 */
			if (try >= 2) {
				kmon_exit(&pme_lock);
				rm_outofhat();
				kmon_enter(&pme_lock);
			}
			try++;
		}
	}
	hatcnt.hc_pmgalloc++;
	pmgrpfree = pmg->pmg_next;	/* take it off the free list */
	pmg->pmg_lock = 1;
	pmg->pmg_keepcnt = 1;

	hat_pmglink(pmg, as, addr);

	hat_smgalloc(as, addr, pmg);

	kmon_exit(&pme_lock);
	(void) splx(s);

	mmu_setpmg(pmg->pmg_base, pmg);

	pmg->pmg_lock = 0;

	return (pmg);
}

/*
 * This routine will return the pmgrp structure for the given address
 * in the current ctx.  But unlike mmu_getpmg, this routine will protect
 * against the pmgrp being lost by spl'ing and will return a kept pmgrp
 * pointer.  The keepcnt should be decremented by the caller when it is
 * done looking at the pmgrp contents.
 */
static struct pmgrp *
hat_getpmg(addr, as)
	addr_t addr;
	struct as *as;
{
	int s;
	struct pmgrp *pmg;

	s = splvm();
	pmg = mmu_getpmg(addr);
	pmg->pmg_keepcnt++;
	if (getpmg_check && pmg != pmgrp_invalid && pmg->pmg_base != 0 &&
	    pmg->pmg_base != (caddr_t)((u_int)addr & ~(PMGRPSIZE - 1)))
		hat_wrongpmg(pmg, addr, as);
	(void) splx(s);
	return (pmg);
}

/*
 * Free the specified pmgrp.  This is done by calling hat_pteunload
 * on all the pme's to process all the referenced and modified bits
 * and to invalidate the pme.  If the hat containing this pmg currently
 * has a ctx, then invalidate that mapping.  Finally we unlink the
 * the pmgrp from the hat pmgrp list and put it on the free list.
 * pmg should be kept (once) when this routine is called.
 */
static void
hat_pmgfree(pmg)
	register struct pmgrp *pmg;
{
	register struct pment *pme = pmg->pmg_pme;
	register struct as *as;
	register int cnt;
	struct ctx *ctx, *ctxsav;
	int s;

	ASSERT(pmg->pmg_keepcnt == 1);

	if ((as = pmg->pmg_as) != NULL) {
		for (cnt = 0; cnt < NPMENTPERPMGRP; cnt++, pme++) {
			if (pme->pme_valid)
				hat_pteunload(pmg, pme, (addr_t)NULL,
				    HAT_RMSYNC);
		}
		if ((ctx = pmg->pmg_as->a_hat.hat_ctx) != NULL) {
			ctxsav = mmu_getctx();
			if (ctxsav != ctx)
				mmu_setctx(ctx);
			mmu_pmginval(pmg->pmg_base);
			if (ctxsav != ctx)
				mmu_setctx(ctxsav);
		}
		if (as->a_hat.hat_pmgrps == pmg) {
			as->a_hat.hat_pmgrps = pmg->pmg_next;
			if (pmg->pmg_next)
				pmg->pmg_next->pmg_prev = NULL;
		} else {
			pmg->pmg_prev->pmg_next = pmg->pmg_next;
			if (pmg->pmg_next)
				pmg->pmg_next->pmg_prev = pmg->pmg_prev;
		}
		pmg->pmg_as = NULL;
		pmg->pmg_next = pmg->pmg_prev = NULL;
	}

	s = splvm();
	pmg->pmg_keepcnt--;
#ifdef MMU_3LEVEL
	if (mmu_3level) {
		pmg->pmg_sme->sme_valid = 0;
		pmg->pmg_sme = (struct sment *)NULL;
	}
#endif
	pmg->pmg_next = pmgrpfree;
	pmgrpfree = pmg;
	(void) splx(s);
}

/*
 * Add the specified pmgrp to the list of pmgrp's allocated to
 * the specified address space.  We hang pmgrps off the address
 * space and not the ctx so that we can keep them around even if
 * we don't have a hardware context.
 */
static void
hat_pmglink(pmg, as, addr)
	register struct pmgrp *pmg;
	struct as *as;
	addr_t addr;
{
	int s;

	ASSERT(pmg->pmg_keepcnt > 0);

	s = splvm();
	pmg->pmg_as = as;
	pmg->pmg_next = as->a_hat.hat_pmgrps;
	if (pmg->pmg_next)
		pmg->pmg_next->pmg_prev = pmg;
	pmg->pmg_prev = NULL;
	as->a_hat.hat_pmgrps = pmg;
	pmg->pmg_base = (addr_t)((u_int)addr & ~(PMGRPSIZE - 1));
	(void) splx(s);
}

/*
 * Called when the wrong pmeg is read out from the MMU.
 * Most likely, this is a down rev Carrera CPU board that
 * is missing some pullup registers on the segment RAMs.
 * The ECO for the needed Carrera CPU board fix is 2555.
 */
static void
hat_wrongpmg(pmg, addr, as)
	struct pmgrp *pmg;
	addr_t addr;
	struct as *as;
{
#ifndef sun2
	register struct pmgrp *pmgp;
	static char *wrp = "PMG 0x%8x claims VA 0x%8x is PMEG 0x%8x (%s)\n";
#ifdef sparc
	u_int map_getsgmap();
#endif sparc

	addr = (addr_t)((u_int)addr & ~(PMGRPSIZE - 1));

	printf("WRONG PAGE MAP GROUP FOUND FOR VIRTUAL ADDRESS 0x%8x!\n", addr);
#ifdef sparc
	printf("Hardware refers this address to segment map 0x%8x\n",
		map_getsgmap(addr));
#endif sparc
	printf("PMG 0x%8x claims VA 0x%8x is PMEG 0x%8x (hardware)\n",
		pmg, pmg->pmg_base, pmg->pmg_num);

/* scan for kernel mappings */
	for (pmgp = kas.a_hat.hat_pmgrps; pmgp != NULL; pmgp = pmgp->pmg_next)
		if (pmgp->pmg_base == addr)
			printf(wrp, pmgp, pmgp->pmg_base,
				pmgp->pmg_num, "kernel");

/* scan for user mappings */
	for (pmgp = as->a_hat.hat_pmgrps; pmgp != NULL; pmgp = pmgp->pmg_next)
		if (pmgp->pmg_base == addr)
			printf(wrp, pmgp, pmgp->pmg_base,
				pmgp->pmg_num, "user");
#else sun2
	printf("WRONG PMG! addr=%x pmg=%x base=%x\n", addr, pmg, base);
#endif sun2
	panic("wrong pmg");
	/* NOTREACHED */
}

#ifdef MMU_3LEVEL

/*
 * Code to handle allocation of smegs is cloned from the pmeg versions
 */

int getsmg_check = 1;

/*
 * This routine will return the smgrp structure for the given address
 * in the current ctx.  But unlike mmu_getsmg, this routine will protect
 * against the smgrp being lost by spl'ing and will return a kept smgrp
 * pointer.  The keepcnt should be decremented by the caller when it is
 * done looking at the smgrp contents.
 */
static struct smgrp *
hat_getsmg(addr)
	addr_t addr;
{
	int s;
	struct smgrp *smg;

	s = splvm();
	smg = mmu_getsmg(addr);
	smg->smg_keepcnt++;
	if (getsmg_check && smg != smgrp_invalid && smg->smg_base != 0 &&
	    smg->smg_base != (caddr_t)((u_int)addr & ~(SMGRPSIZE - 1))) {
		printf("hat_getsmg: addr=%x, smg=%x, smg base=%x\n",
		    addr, smg, smg->smg_base);
		call_debug("hat_getsmg");
	}
	(void) splx(s);
	return (smg);
}

/*
 * Free the specified smgrp.  This is done by calling hat_pmgfree
 * on all the sme's to invalidate the smgrp.  If the hat containing
 * this smg currently has a ctx, then invalidate that mapping.
 * Finally we unlink the the smgrp from the hat smgrp list and
 * put it on the free list.
 * smg should be kept (once) when this routine is called.
 *
 * XXX - at some level, this should be optimized to used
 *	segment and/or region flush
 */
static void
hat_smgfree(smg)
	register struct smgrp *smg;
{
	register struct sment *sme;
	register struct pmgrp *pmg;
	register struct as *as;
	register int cnt;
	struct ctx *ctx, *ctxsav;
	int s;

	/*
	 * This may be prohibitively expensize as it flushes each segment by
	 * flushing its pages.  Rarely are all the pages in a pmeg
	 * used and most processes rarely use greater than two or three pmgs
	 * If this is a real problem we can flush the context and free
	 * everything, XXX - instrument this
	 */
	sme = smg->smg_sme;
	if ((as = smg->smg_as) != NULL) {
		for (cnt = 0; cnt < NSMENTPERSMGRP; cnt++) {
			if (sme->sme_valid) {
				pmg = sme->sme_pmg;
				s = splvm();
				pmg->pmg_keepcnt++;
				hat_pmgfree(pmg);
				sme->sme_pmg = pmgrp_invalid;
				(void) splx(s);
			}
			sme++;
		}
		if ((ctx = smg->smg_as->a_hat.hat_ctx) != NULL) {
			ctxsav = mmu_getctx();
			if (ctxsav != ctx)
				mmu_setctx(ctx);
			mmu_smginval(smg->smg_base);
			if (ctxsav != ctx)
				mmu_setctx(ctxsav);
		}
		if (as->a_hat.hat_smgrps == smg) {
			as->a_hat.hat_smgrps = smg->smg_next;
			if (smg->smg_next)
				smg->smg_next->smg_prev = NULL;
		} else {
			smg->smg_prev->smg_next = smg->smg_next;
			if (smg->smg_next)
				smg->smg_next->smg_prev = smg->smg_prev;
		}
		smg->smg_as = NULL;
		smg->smg_next = smg->smg_prev = NULL;
	}

	s = splvm();
	smg->smg_keepcnt--;
	smg->smg_next = smgrpfree;
	smgrpfree = smg;
	(void) splx(s);
}

/*
 * Add the specified smgrp to the list of smgrp's allocated to
 * the specified address space.  We hang smgrps off the address
 * space and not the ctx so that we can keep them around even if
 * we don't have a hardware context.
 */
static void
hat_smglink(smg, as, addr)
	register struct smgrp *smg;
	struct as *as;
	addr_t addr;
{
	int s;

	s = splvm();
	smg->smg_as = as;
	smg->smg_next = as->a_hat.hat_smgrps;
	if (smg->smg_next)
		smg->smg_next->smg_prev = smg;
	smg->smg_prev = NULL;
	as->a_hat.hat_smgrps = smg;
	smg->smg_base = (addr_t)((u_int)addr & ~(SMGRPSIZE - 1));
	(void) splx(s);
}

void
hat_smgreserve(seg, addr)
	struct seg *seg;
	addr_t addr;
{
	register struct as *as = seg->s_as;
	register struct smgrp *smg;

	if (!mmu_3level)
		return;

	smg = hat_getsmg(addr);			/* keeps the smg for us */

	if (as->a_hat.hat_ctx == NULL ||
	    (smg->smg_as != NULL && smg->smg_as != as))
		panic("hat_smgreserve");

	if (smg != smgrp_invalid && smg->smg_as == NULL)
		hat_smglink(smg, as, addr);

	smg->smg_lock = 1; 	/* if its being reserved, also lock it */
}

/*
 * Initialize all the unlocked smgs to have invalid sme's
 * and add them to the free list.
 * This routine is called during startup after all the
 * kernel smgs have been reserved.  This routine will
 * also set the smgrpmin variable for use in hat_smgalloc.
 *
 * REGTEMP is only used here so we temporarily steal
 * the region before KERNELBASE and mark it invalid
 * when we are finished.
 */
void
hat_smginit()
{
	register struct smgrp *smg;
	register addr_t addr;
	int s;

	if (!mmu_3level)
		return;

	s = splvm();
	kmon_enter(&sme_lock);

	for (smg = smgrps; smg < &smgrps[NSMGRPS]; smg++) {
		if (smg->smg_lock || smg->smg_keepcnt != 0)
			continue;

		if (smgrpmin == NULL)
			smgrpmin = smg;

		mmu_settsmg((addr_t)REGTEMP, smg);

		for (addr = (addr_t)REGTEMP;
		    addr < (addr_t)(REGTEMP + SMGRPSIZE);
		    addr += PMGRPSIZE) {
			mmu_pmginval(addr);
		}

		smg->smg_next = smgrpfree;
		smgrpfree = smg;
	}
	smgrphand = smgrpmin;

	mmu_smginval((addr_t)REGTEMP);

	kmon_exit(&sme_lock);
	(void) splx(s);
}

/*
 * Allocate a smgrp to map the specified address.
 * Returns w/ the keepcnt incremented for the particular smgrp used.
 * First look for something in the free list and then steal one
 * that is currently being used.
 */
static void
hat_smgalloc(as, addr, pmg)
	struct as *as;
	addr_t addr;
	struct pmgrp *pmg;
{
	register struct smgrp *smg;
	register struct sment *sme;
	int s;

	if (!mmu_3level)
		return;

	s = splvm();
	if ((smg = mmu_getsmg(addr)) != smgrp_invalid) {
		smg->smg_keepcnt++;
		hatcnt.hc_smghave++;
		if (getsmg_check && smg->smg_base !=
		    (caddr_t)((u_int)addr & ~(SMGRPSIZE - 1))) {
			printf("hat_smgalloc: addr=%x, smg=%x, smg base=%x\n",
			    addr, smg, smg->smg_base);
			call_debug("hat_smgalloc");
		}
		sme = &(smg->smg_sme
			[(mmu_btop(addr-smg->smg_base)/NPMENTPERPMGRP)]);
		sme->sme_pmg  = pmg;
		sme->sme_valid = 1;
		pmg->pmg_sme = sme;
		(void) splx(s);
		return;
	}

	/*
	 * No smgrp allocated to this address space contains the pme,
	 * allocate a new smg for this address space.  First, try
	 * the free list.
	 */
	kmon_enter(&sme_lock);
top:
	if ((smg = smgrpfree) == NULL) {
		int try;

		/*
		 * No smg's free, have to take one from someone.
		 * Take from address spaces with no ctx first.
		 * XXX - could do it with just one pass.
		 */
		smg = smgrphand;
		try = 1;
		for (;;) {
			do {
				smg++;
				if (smg == &smgrps[NSMGRPS]) {
					if (smgrpmin) {
						/* skip some kernel smgrps */
						smg = smgrpmin;
					} else {
						smg = smgrps;
					}
				}
				if (smg->smg_lock == 0 && hat_pmgcheck(smg)) {
			/*	 && smg->smg_keepcnt == 0) { */
					/*
					 * On the first try, only take a smg
					 * from an address space with no ctx.
					 */
					if (try == 1 &&
					    smg->smg_as->a_hat.hat_ctx != NULL)
						continue;
					/*
					 * Found a candidate, free
					 * it up and try again.
					 */
					if (try == 1)
						hatcnt.hc_smgstolennoctx++;
					else
						hatcnt.hc_smgstolenctx++;
					/* we are at splvm */
					smg->smg_keepcnt++;
					hat_smgfree(smg);
					smgrphand = smg;
					goto top;
				}
			} while (smg != smgrphand);
			/*
			 * Give up after 2 tries.
			 */
			if (try >= 2) {
				kmon_exit(&sme_lock);
				rm_outofhat();
				kmon_enter(&sme_lock);
			}
			try++;
		}
	}
	hatcnt.hc_smgalloc++;
	smgrpfree = smg->smg_next;	/* take it off the free list */
	smg->smg_lock = 1;
	smg->smg_keepcnt = 1;

	hat_smglink(smg, as, addr);

	sme = &(smg->smg_sme[(mmu_btop(addr-smg->smg_base)/NPMENTPERPMGRP)]);
	sme->sme_pmg  = pmg;
	sme->sme_valid = 1;
	pmg->pmg_sme = sme;

	kmon_exit(&sme_lock);
	(void) splx(s);

	mmu_setsmg(smg->smg_base, smg);

	smg->smg_lock = 0;

	return;
}

hat_xfree_sun4(as)
	register struct as *as;
{
	register int s;

	/* if three-level mmu, free smgs, else free pmgs */
	if (mmu_3level) {
		register struct smgrp *smg;
		s = splvm();
		while (smg = as->a_hat.hat_smgrps) {
			smg->smg_keepcnt++;
			(void) splx(s);
			hat_smgfree(smg);	/* should unkeep the smg */
			s = splvm();
		}
	} else {
		register struct pmgrp *pmg;
		s = splvm();
		while (pmg = as->a_hat.hat_pmgrps) {
			pmg->pmg_keepcnt++;	/* XXX pmg->pmp_keepcnt = 1? */
			(void) splx(s);
			hat_pmgfree(pmg);	/* should unkeep the pmg */
			s = splvm();
		}
	}
	(void) splx(s);
}

hat_pmgcheck(smg)
	struct smgrp *smg;
{
	register struct sment *sme;
	register struct pmgrp *pmg;
	int cnt;

	sme = smg->smg_sme;
	for (cnt = 0; cnt < NSMENTPERSMGRP; cnt++) {
		if (sme->sme_valid) {
			pmg = sme->sme_pmg;
			if (pmg->pmg_keepcnt != 0)
				return (0);
		}
		sme++;
	}
	return (1);
}

#else MMU_3LEVEL

hat_xfree_other(as)
	register struct as *as;
{
	register int s;
	register struct pmgrp *pmg;

	s = splvm();
	while (pmg = as->a_hat.hat_pmgrps) {
		pmg->pmg_keepcnt++;	/* XXX pmg->pmp_keepcnt = 1? */
		(void) splx(s);

		hat_pmgfree(pmg);	/* should unkeep the pmg */

		s = splvm();
	}
	(void) splx(s);
}

#endif /* MMU_3LEVEL */
