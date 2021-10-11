#ifndef lint
static	char sccsid[] = "@(#)vm_hat.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
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
 * segment and page map mmu's are very similar.	 For this reason,
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
 * all the details of how the hardware is managed shound not be visable
 * above this layer except for miscellaneous machine specific functions
 * (e.g. mapin/mapout) that work in conjunction with this code.	 Other
 * than a small number of machine specific places, the hat data
 * structures seen by the higher levels in the VM system are opaque
 * and are only operated on by the hat routines.  Each address space
 * contains a struct hat and a page contains an opaque pointer which
 * is used by the hat code to hold a list of active translations to
 * that page.
 *
 * XXX - At integration into 5.0, replace all #define'd symbols such as
 *	 NPMGRPSSW with the actual variable names (npmgrpssw in this case).
 */

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/debug.h>
#include <sys/user.h>			/* for u_ru.ru_minflt */
#include <sys/trace.h>
#include <sys/vmmeter.h>		/* for flush_cnt */

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
#include <vm/vpage.h>
#include <vm/rm.h>
#include <vm/seg_u.h>
#include <vm/seg_vn.h>
#include <vm/faultcode.h>

#if !(defined(sun4) || defined(sun4c))
ERROR - This HAT works only for sun4 and sun4c
#endif !(defined(sun4) || defined(sun4c))

/*
 * XXX - Review all ASSERT's in this vm_hat.c. Change inexpensive ones to
 *	 test and panic code. Remove #include <sys/debug>.
 */
#define	ASSERTPMGMAPPED(pmg, msg) /* assertpmgmapped(pmg, msg) */
#define	TEST_PANIC(cond, panicmsg) {if (cond) panic(panicmsg); }

/*
 * Machine specific public data structures.
 */
struct as kas;
struct ctx *kctx;
struct pmgrp *pmgrp_invalid;
#ifdef MMU_3LEVEL
struct smgrp *smgrp_invalid;
#endif MMU_3LEVEL

/*
 * Semi-private vm_hat data structures.
 * Other machine specific routines need to access these.
 */
extern int	npmgrpssw;		/* now defined in param.c */
int		npmghash;
u_short		ctx_time;

struct	ctx	*ctxs, *ctxsNCTXS;	/* used by <machine/mmu.c> */
struct	pmgrp	*pmgrps, *pmgrpsNPMGRPS; /* used by <machine/mmu.c> */
struct	pment	*pments, *pmentsNPMENTS; /* used by <machine/machdep.c> */
struct	hwpmg	*hwpmgs, *hwpmgsNHWPMGS; /* used by <machine/mmu.c> */
struct	pte	*ptes, *ptesNPTES;	/* used by <machine/machdep.c> */
struct	pmgrp	**pmghash, **pmghashNPMGHASH; /* used by <machine/machdep.c> */
#ifdef MMU_3LEVEL
struct	smgrp *smgrps, *smgrpsNSMGRPS;
struct	sment *sments, *smentsNSMENTS;
#endif MMU_3LEVEL

extern void vac_flushallctx();

/*
 * hat layer statistics.
 */

/*
 * Context, smeg, and pmeg statistics.
 */
struct vmhatstat {

	/* Context allocation statistics */
	u_int	vh_ctxfree;	  /* ctx allocations without a ctx steal */
	u_int	vh_ctxstealclean; /* ctx allocations requiring a ctx steal */
	u_int	vh_ctxstealflush; /* ctx allocations requiring a ctx steal */
	u_int	vh_ctxmappmgs;	  /* pmgs mapped at ctx allocation */

	/* SW pmg statistics */
	u_int	vh_pmgallocfree;  /* pmg allocation without a pmg steal */
	u_int	vh_pmgallocsteal; /* pmg allocations requiring a pmg steal */

	/* HW pmg map and load/unload statistics */
	u_int	vh_pmgmap;	  /* mappings of loaded pmg's */
	u_int	vh_pmgldfree;	/* alloc.s. of free HW pmg */
	u_int	vh_pmgldnoctx;	/* allocs. of HW pmg with no ctx */
	u_int	vh_pmgldcleanctx; /* allocs. of HW pmg with clean ctx */
	u_int	vh_pmgldflush;	/* allocs. of HW pmg needing VAC flush */
	u_int	vh_pmgldnomap;	/* allocs. of HW pmg taking unmapped pmg */

	/* hat_fault statitistics */
	u_int	vh_faultmap;	  /* hat_fault mapped HW pmg */
	u_int	vh_faultload;	  /* hat_fault loaded SW pmg in HW */
	u_int	vh_faultinhw;	  /* hat_fault failed to resolve the fault */
	u_int	vh_faultnopmg;	  /* hat_fault failed to resolve the fault */

	/* HW smg allocation statistics */
	u_int	vh_smgfree;
	u_int	vh_smgnoctx;
	u_int	vh_smgcleanctx;
	u_int	vh_smgflush;

	/* added later to the end not to break pmegstat */
	u_int	vh_pmgallochas;	/* has already a pmeg */

} vmhatstat;

/*
 * Page cacheability statistics.
 */
struct cache_stats {
	int cs_ionc;		/* non-cached IO translations */
	int cs_ioc;		/* cached IO translations */
	int cs_knc;		/* non-cached kernel translations */
	int cs_kc;		/* cached kernel translations */
	int cs_unc;		/* non-cached user translations */
	int cs_uc;		/* cached user translations */
	int cs_other;		/* other non-type 0 pages */
	int cs_iowantchg;	/* # of IO cached to NC changes skipped */
	int cs_kchange;		/* # of kernel cached to non-cached changes */
	int cs_uchange;		/* # of user cached to non-cached changes */
	int cs_unloadfix;	/* # of unload's that made pages cachable */
	int cs_unloadnofix;	/* # of	 " that didn't made pages cachable */
	int cs_skip;		/* XXX should be after cs_other */
} cache_stats;

/*
 * hat_pmgfind() look aside buffer hit/miss statistics.
 */
struct pmgfindstat {
	u_int	pf_hit;
	u_int	pf_miss;
	u_int	pf_notfound;
} pmgfindstat;


/*
 * Private vm_hat data structures
 */

enum ptesflag { PTESFLAG_SKIP, PTESFLAG_UNLOAD };

static	struct ctx *ctxhand;

static	struct pmgrp *pmgrphand;
static	struct pmgrp *pmgrpfree;
static	struct pmgrp *pmgrpmin;

static	struct hwpmg *hwpmghand;
static	struct hwpmg *hwpmgfree;
static	struct hwpmg *hwpmgmin;

#ifdef MMU_3LEVEL
static	struct smgrp *smgrphand;
static	struct smgrp *smgrpfree;
static	struct smgrp *smgrpmin;
#endif MMU_3LEVEL

static void		hat_xfree(/* as */);
static void		hat_pteunload(/* pmg, pme, addr, flags */);
static void		hat_ptesync(/* pp, pmg, pme, addr, flags */);

static void		hat_pmgfree(/* pmg */);
static void		hat_pmglink(/* pmg, as, addr */);
static void		hat_pmgload(/* pmg */);
static void		hat_pmgunload(/* pmg, ptesflag */);
static struct pmgrp *	hat_pmgalloc(/* seg, addr */);
static struct pmgrp *	hat_pmgfind(/* addr, as */);
static void		hat_pmgloadptes(/* a, ppte */);
static void		hat_pmgunloadptes(/* a, ppte */);
static void		hat_pmgswapptes(/* a, ppte1, ppte2 */);
static void		hat_pmgmap(/* pmg */);
static void		hat_clrcleanbit();
static void		hat_unmap_aspmgs(/* as */);

#ifdef MMU_3LEVEL
static	struct smgrp	*hat_getsmg(/* addr */);
static	void		hat_smgfree(/* smg */);
static	void		hat_smglink(/* smg, as, addr */);
static	void		hat_smgalloc(/* as, addr, pmg */);
#define	hat_pmgtosmg(pmg) \
	(&smgrps[((pmg)->pmg_sme - sments) >> NSMENTPERSMGRPSHIFT])
#endif MMU_3LEVEL

static int hatunmaplimit = 30;		/* % limit used by hat_unmap_aspmgs() */

/* local inline functions */
#define	hat_pmgbase(a) ((addr_t)((u_int)a & PMGRPMASK))
#define	hat_pmgisloaded(pmg) (pmg->pmg_num != PMGNUM_SW)
#define	hat_addrtopte(pmg, a) \
	((pmg)->pmg_pte + (((u_int)(a) & PMGRPOFFSET) >> PAGESHIFT))
#define	hat_pmetopte(pmg, pme) ((pmg)->pmg_pte + ((pme) - (pmg)->pmg_pme))

/**************** misc. macros and declarations below ************************/

/*
 * XXX - Function defitions should be in a header file.
 */
extern	u_int map_getsgmap();

extern char DVMA[];		/* addresses in kas above DVMA are for "IO" */

#ifdef VAC
/*
 * XXX - should be made more efficent by using pment
 * indexing instead of virtual address checking.
 */
#define	VAC_MASK(a) (((u_int)(a) & MMU_PAGEMASK) & (shm_alignment - 1))
#define	VAC_ALIGNED(a1, a2) ((VAC_MASK(a1) ^ VAC_MASK(a2)) == 0)
#endif

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
 *		do operations;
 *		PP_LIST_NEXT(next item)
 *	}
 *	PP_LIST_CLOSE
 *
 * NOTE that you CANNOT put a semicolon after PP_LIST_OPEN or it will not
 * function.
 */
#define	PP_LIST_OPEN \
	s = splvm(); \
	pmg = (struct pmgrp *)NULL; \
	PP_LIST_NEXT((struct pment *)pp->p_mapping) \
	while (pme)

#define	PP_LIST_NEXT(x)	\
	(void) splx(s); \
	s = splvm(); \
	if (pmg) \
	    pmg->pmg_keepcnt--; \
	pme = x; \
	if (pme) { \
	    pmg = &pmgrps[(pme - pments) >> NPMENTPERPMGRPSHIFT]; \
	    pmg->pmg_keepcnt++; \
	}

#define	PP_LIST_CLOSE  (void) splx(s);

/*
 *
 * The next set of routines implements the machine
 * independent hat interface described in <vm/hat.h>
 *
 */

/*
 * Initialize the hardware address translation structures.
 * Called by startup.
 *
 * Initialize the SW page tables in the range [0..NPMGRPS) as loaded
 * and mapped. This is required for hat_pmgreserve to work. After startup
 * reserves kernel pmg's, it calls hat_pmginit. hat_pmginit will create
 * a list of free SW page tables and a list of free HW pmg's by skipping
 * pmg's reserved pmg's.
 */
void
hat_init()
{
	register struct ctx	*ctx;
	register struct pmgrp	*pmg;
	register struct pment	*pme;
	register struct pte	*pte;
	register int i;


	i = 0;
	for (ctx = ctxs; ctx < ctxsNCTXS; ctx++)
		ctx->c_num = i++;
	ctxhand = ctxs;

#ifdef MMU_3LEVEL
	if (mmu_3level) {
		register struct smgrp *smg;
		register struct sment *sme;

		i = 0;
		sme = sments;
		for (smg = smgrps; smg < smgrpsNSMGRPS; smg++) {
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
	pte = ptes;

	/*
	 * First NPMGRS pmgs are loaded in HW. hat_pmgreserve() assumes this.
	 *
	 * XXX - Note that &pmgrps[NPMGRPS] cannot be changed to pmgrpsNPMGRPS
	 */
	for (pmg = pmgrps; pmg < &pmgrps[NPMGRPS]; i++, pmg++) {

		pmg->pmg_num = i;
		pmg->pmg_mapped = 1;
		hwpmgs[i].hwp_pmgrp = pmg;

		pmg->pmg_pme = pme;
		pmg->pmg_pte = pte;
		pme += NPMENTPERPMGRP;
		pte += NPMENTPERPMGRP;
	}

	/*
	 * The remaining SW pmgrp are not loaded in HW.
	 */
	for (; pmg < pmgrpsNPMGRPS; pmg++) {

		pmg->pmg_num = PMGNUM_SW;
		pmg->pmg_mapped = 0;

		pmg->pmg_pme = pme;
		pmg->pmg_pte = pte;
		pme += NPMENTPERPMGRP;
		pte += NPMENTPERPMGRP;
	}

	pmgrps[PMGRP_INVALID].pmg_lock = 1;	/* lock invalid pmgrp */
	pmgrps[PMGRP_INVALID].pmg_keepcnt++;
	pmgrp_invalid = &pmgrps[PMGRP_INVALID];

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

	if ((ctx = as->a_hat.hat_ctx) != NULL) {

		TEST_PANIC(ctx->c_lock, "hat_free - ctx is locked");

		/*
		 * Clean context now. This will prevent expensive segment
		 * and page flushing when freeing individual pmgs.
		 */
		mmu_setctx(ctx);
		if (!ctx->c_clean) {
			vac_ctxflush();
			ctx->c_clean = 1;
		}
		hat_xfree(as);

		as->a_hat.hat_ctx = NULL;
		ctx->c_as = NULL;
	} else {
		hat_xfree(as);
	}

	/*
	 * Switch to kernel context.
	 */
	mmu_setctx(kctx);
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
	int	s = -1;

	/*
	 * If the request is to load a page which is really something
	 * for which we have a struct page, then we must be sure we
	 * maintain cache consistency.	However, we don't want to
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
	if (s == -1)
		return;
	else
		(void) splx(s);
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
	register struct pmgrp	*pmg;
	int			s;

	pmg = hat_pmgfind(addr, seg->s_as);

	TEST_PANIC(pmg == NULL || pmg->pmg_keepcnt < 2, "hat_unlock");

#ifdef VAC
	if (vac && pmg->pmg_keepcnt == 2) {
		register struct pment *pme = pmg->pmg_pme;
		register int tcnt;
		struct page *pp;

		/*
		 * Now check to see if we now can cache any non-cached pages.
		 * For now, we use the simple minded algorithm and just
		 * unload any locked of the locked translations if the
		 * corresponding page is currently marked as non-cachable.
		 * This situation doesn't happen all the much, so the
		 * efficency doesn't have to be all that great.
		 */
		for (tcnt = 0; tcnt < NPMENTPERPMGRP; tcnt++, pme++) {
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
#ifdef MMU_3LEVEL
	/* Decr. once for unlock. */
	if (mmu_3level)
		hat_pmgtosmg(pmg)->smg_keepcnt--;
#endif MMU_3LEVEL
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
 */
void
hat_chgprot(seg, addr, len, vprot)
	struct seg	*seg;
	addr_t		addr;
	u_int		len;
	u_int		vprot;		/* virtual page protections */
{
	register addr_t		a, ea;
	register struct pmgrp	*pmg = NULL;
	register u_int		pprot;	/* physical page protections */
	register int		newprot;
	struct pte		pte;
	struct pte		*ppte;

	if (vprot != ~PROT_WRITE && vprot != ~PROT_USER)
		pprot = hat_vtop_prot(vprot);

	/*
	 * We must get a context for the AS because we will be
	 * synchronizing SW PTE by reads from MMU.
	 */
	hat_setup(seg->s_as);

	for (a = addr, ea = addr + len; a < ea; a += MMU_PAGESIZE) {
		if (pmg == NULL ||
		    ((u_int)a & (PMGRPSIZE - 1)) < MMU_PAGESIZE) {
			if (pmg) {
				register int s;
				s = splvm();
				pmg->pmg_keepcnt--;
				(void) splx(s);
			}
			pmg = hat_pmgfind(a, seg->s_as);
			if (pmg == NULL) {
				/*
				 * Bump up `a' to avoid checking all
				 * the pte's in the invalid pmgrp.
				 */
				a = (addr_t)((u_int)a & ~(PMGRPSIZE - 1)) +
				    PMGRPSIZE - MMU_PAGESIZE;
				continue;
			}
			/*
			 * Make sure that loaded pmg is also mapped.
			 */
			if (hat_pmgisloaded(pmg))
				hat_pmgmap(pmg);
		}

		ppte = hat_addrtopte(pmg, a);
		if (!pte_valid(ppte))
			continue;

		/*
		 * Synchronize PTE from MMU.
		 */
		if (hat_pmgisloaded(pmg)) {
			mmu_getpte(a, ppte);
		}

		pte = *ppte;

		if (vprot == ~PROT_WRITE) {
			switch (pte.pg_prot) {
			case KW: pprot = KR; newprot = 1; break;
			case UW: pprot = UR; newprot = 1; break;
			default: newprot = 0; break;
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
			case UW: pprot = KW; newprot = 1; break;
			case UR: pprot = KR; newprot = 1; break;
			default: newprot = 0; break;
			}
#endif sun2
		} else if (pte.pg_prot != pprot) {
			newprot = 1;
		} else {
			newprot = 0;
		}
		if (newprot) {
			pte.pg_prot = pprot;

			*ppte = pte;	/* Set SW PTE */

			/*
			 * Synchronize HW PTE if this pmg is loaded.
			 * We assume that hat_setup has been done.
			 */
			if (hat_pmgisloaded(pmg)) {
#ifdef VAC
				if (vac && !pte.pg_nc && pte.pg_r)
					vac_pageflush(a);
#endif VAC
				mmu_setpte(a, pte);
			}
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
	struct seg	*seg;
	addr_t		addr;
	u_int		len;
	struct seg	*nseg;
{
	return;
}

/*
 * Unload all the mappings in the range [addr..addr+len).
 */
void
hat_unload(seg, addr, len)
	struct seg	*seg;
	addr_t		addr;
	u_int		len;
{
	register addr_t		a;
	register struct pment	*pme;
	register struct pmgrp	*pmg = NULL;
	addr_t			ea;
	int			s;

	if (seg->s_as->a_hat.hat_pmgrps == NULL) {
		/*
		 * If there are no allocated pmgrps for this
		 * address space, we don't want to do anything.
		 */
		return;
	}

	/*
	 * hat_setup needed for HAT_VADDR optimization to work.
	 */
	hat_setup(seg->s_as);

	for (a = addr, ea = addr + len; a < ea; a += MMU_PAGESIZE) {
		if (pmg == NULL ||
		    ((u_int)a & (PMGRPSIZE - 1)) < MMU_PAGESIZE) {
			if (pmg != NULL) {
				s = splvm();
				pmg->pmg_keepcnt--;
				(void) splx(s);
			}
			pmg = hat_pmgfind(a, seg->s_as);
			if (pmg == NULL) {
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

			TEST_PANIC(pmg->pmg_keepcnt < 2,
			    "hat_unload - pmg not kept");

			/*
			 * Decrement keepcnt on pmg and smg to indicate
			 * 'unlock' on the address.
			 */
			s = splvm();
#ifdef MMU_3LEVEL
			if (mmu_3level) {
				hat_pmgtosmg(pmg)->smg_keepcnt--;
			}
#endif MMU_3LEVEL
			pmg->pmg_keepcnt--;
			(void) splx(s);

			hat_pteunload(pmg, pme, a, HAT_NOSYNC | HAT_VADDR);
		} else {
			hat_pteunload(pmg, pme, a, HAT_RMSYNC | HAT_VADDR);
		}
	}
	if (pmg) {
		s = splvm();
		pmg->pmg_keepcnt--;
		(void) splx(s);
	}
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
 * Resolve a page fault by loading a cached translation.
 *
 * hat_fault() is called from the fault handler in locore.s and pagefault.
 *
 */
hat_fault(addr)
	caddr_t		addr;
{
	struct pmgrp	*pmg;
	int		s;
	struct as	*as;

	/*
	 * We assume that addresses above KERNELBASE belong to kas.
	 */
	if (addr >= (addr_t)KERNELBASE) {
		as = &kas;
	} else {

		/*
		 * We punt if the address space of the running process
		 * hasn't been set up. pagefault will allocate the context
		 * and call hat_fault again if this was a real pagefault.
		 */
		if ((as = u.u_procp->p_as) != mmu_getctx()->c_as)
			return (FC_NOMAP);
	}

	if ((pmg = hat_pmgfind(addr, as)) != NULL) {

		if (pmg->pmg_mapped) {
			/*
			 * Pmg is mapped and loaded. The hat layer cannot
			 * resolve the fault.
			 */
			ASSERT(pmg->pmg_num != PMGNUM_SW);
			s = splvm();
			pmg->pmg_keepcnt--;
			vmhatstat.vh_faultinhw++;
			(void) splx(s);
			return (FC_NOMAP);
		} else if (hat_pmgisloaded(pmg)) {

			/*
			 * pmg is loaded but not mapped.
			 */
			s = splvm();

			/*
			 * The code below is inline hat_pmgmap.
			 */
#ifdef MMU_3LEVEL
			if (mmu_3level) {
				hat_smgalloc(pmg->pmg_as, pmg->pmg_base, pmg);
				hat_pmgtosmg(pmg)->smg_keepcnt--;
			}
#endif MMU_3LEVEL
			mmu_setpmg(pmg->pmg_base, pmg);
			pmg->pmg_mapped = 1;
			/* End of inline hat_pmgmap. */

			pmg->pmg_keepcnt--;
			vmhatstat.vh_faultmap++;
			(void) splx(s);
			return ((faultcode_t)0);
		} else {
			/*
			 * Pmg is not loaded.
			 */
			hat_pmgload(pmg);
			s = splvm();
			pmg->pmg_keepcnt--;
#ifdef MMU_3LEVEL
			if (mmu_3level)
				hat_pmgtosmg(pmg)->smg_keepcnt--;
#endif MMU_3LEVEL
			(void) splx(s);
			vmhatstat.vh_faultload++;
			return ((faultcode_t)0);
		}
	}

	/*
	 * HAT couldn't resolve the fault because there is no SW pmg.
	 */
	vmhatstat.vh_faultnopmg++;
	return (FC_NOMAP);
}


/*
 * End of machine independent interface routines.
 *
 * The next few routines implement some machine dependent functions
 * needed for the Sun MMU.  Note that each hat implementation can define
 * whatever additional interfaces that make sense for that machine.
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
	register struct as	*as = seg->s_as;
	register struct pmgrp	*pmg;
	u_int			pmgnum;
	int			s;

	pmgnum = map_getsgmap(addr);
	TEST_PANIC(pmgnum == PMGRP_INVALID, "hat_pmgreserve: invalid pmg");

	pmg = hwpmgs[pmgnum].hwp_pmgrp;
	ASSERT(pmg != NULL);
	ASSERT(pmg->pmg_num == pmgnum);
	s = splvm();
	pmg->pmg_keepcnt++;
	(void) splx(s);

	TEST_PANIC(as->a_hat.hat_ctx == NULL ||
	    (pmg->pmg_as != NULL && pmg->pmg_as != as),
	    "hat_pmgreserve");

	if (pmg != pmgrp_invalid && pmg->pmg_as == NULL) {
		hat_pmglink(pmg, as, addr);
	}

#ifdef MMU_3LEVEL
	if (mmu_3level)
		hat_smgreserve(seg, addr);
#endif MMU_3LEVEL
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
	register struct pmgrp	*pmg;
	register addr_t		addr;
	int			s;
	int			i;
	struct hwpmg		*hwpmg;

	s = splvm();

	/*
	 * Make HW free list. Skip locked and kept pmgrps.
	 * Here we assume that <0..NPMGRPS) were loaded in MMU by hat_init().
	 */
	for (pmg = pmgrps, hwpmg = hwpmgs;
	    hwpmg < hwpmgsNHWPMGS; pmg++, hwpmg++) {

		if (pmg->pmg_lock || pmg->pmg_keepcnt > 0) {
			pmg->pmg_mapped = 1;
			continue;
		}

		if (pmgrpmin == NULL) {
			pmgrpmin = pmgrphand = pmg;
			hwpmgmin = hwpmghand = hwpmg;
		}

		mmu_settpmg(SEGTEMP, pmg);

		for (addr = SEGTEMP; addr < SEGTEMP + PMGRPSIZE;
		    addr += PAGESIZE)
			mmu_setpte(addr, mmu_pteinvalid);

		pmg->pmg_num = PMGNUM_SW;

		hwpmg->hwp_next = hwpmgfree;
		hwpmg->hwp_pmgrp = NULL;
		hwpmgfree	= hwpmg;

	}
	mmu_pmginval(SEGTEMP);

	/*
	 * Make SW pmg free list.
	 */
	for (pmg = pmgrps; pmg < pmgrpsNPMGRPS; pmg++) {

		if (pmg->pmg_lock || pmg->pmg_keepcnt > 0)
			continue;

		for (i = 0; i < NPMENTPERPMGRP; i++) {
			pmg->pmg_pte[i] = mmu_pteinvalid;
		}
		pmg->pmg_num = PMGNUM_SW;
		pmg->pmg_mapped = 0;

		pmg->pmg_next = pmgrpfree;
		pmgrpfree = pmg;
	}

	(void) splx(s);
#ifdef MMU_3LEVEL
	if (mmu_3level)
		hat_smginit();

	/*
	 * Check keepcnt on smegs and pmegs to detect double mapped pmegs.
	 */
	hat_smgcheck_keepcntall();
#endif MMU_3LEVEL
}

/*
 * Set addr in segment seg to use pte to (possibly) map to page pp.
 * This is the common routine used for hat_memload and hat_devload
 * in addition to the machine dependent mapin implementation.
 */
void
hat_pteload(seg, addr, pp, pte, flags)
	struct seg	*seg;
	addr_t		addr;
	struct page	*pp;
	struct pte	pte;
	int		flags;
{
	register struct pment	*ppme;
	register struct pmgrp	*ppmg;
	int			s;
	struct pte		opte;

	TEST_PANIC(pp != NULL && pp->p_free, "hat_pteload free page");

	/*
	 * We need to setup context because we will (potentially) be reading
	 * HW PTE's.
	 */
	hat_setup(seg->s_as);
	ppmg = hat_pmgalloc(seg, addr);
	hat_pmgload(ppmg);

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
		 * exiting ref and mod bits for this translation.
		 *
		 * XXX - should cache all the attributes of a loaded
		 * translation in the pme structure so that we can
		 * avoid reloading all together unless something
		 * is actually going to change.
		 */

		/*
		 * Synch SW PTE by reading MMU.
		 */
		opte = *hat_addrtopte(ppmg, addr);

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

	*hat_addrtopte(ppmg, addr) = pte; /* Set SW PTE */

	mmu_setpte(addr, pte);	/* load the SW pte in HW */

	/*
	 * Check to see if this pme needs to be added
	 * to the list of pme's mapping this page.
	 */
	if (pp != ppme->pme_page) {

		TEST_PANIC(ppme->pme_page != NULL, "hat_pteload");
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
		 * XXX and one of them is writable XXX.	 If conflicts
		 * exist, the extant mappings are flushed UNLESS
		 * one of them is locked.  If one of them is locked,
		 * then the mappings are flushed and converted to
		 * non-cacheable mappings [must be deconverted in
		 * hat_pteunload].
		 * XXX	need to store protections in pme
		 *	to employ writable optimization.
		 */
		if (vac && !pp->p_nc) {
			struct pmgrp *pmg;	/* temporary pmg */
			struct pment *pme;	/* temporary pme */
			struct pmgrp
			    *pmgpc = (struct pmgrp *)0,	/* user's text pmg */
			    *pmgsp = (struct pmgrp *)0;	/* user's stack pmg */
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
			if (!servicing_interrupt()) {
			    if (u.u_ar0 != (int *)0) {
				if (u.u_ar0[SP] != 0) {
				    pmgsp = mmu_getpmg ((addr_t)u.u_ar0[SP]);
				    if (pmgsp != (struct pmgrp *)0)
					pmgsp->pmg_keepcnt ++;
				}
				if (u.u_ar0[PC] != 0) {
				    pmgpc = mmu_getpmg ((addr_t)u.u_ar0[PC]);
				    if (pmgpc != (struct pmgrp *)0)
					pmgpc->pmg_keepcnt ++;
				}
			    }
			}
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
			 * Release locked pmgrps.
			 */
			s = splvm();
			if (pmgsp != (struct pmgrp *)0)
				pmgsp->pmg_keepcnt --;
			if (pmgpc != (struct pmgrp *)0)
				pmgpc->pmg_keepcnt --;
			(void) splx(s);

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

		TEST_PANIC(pp != NULL &&
			((ppme->pme_intrep != ((flags & PTELD_INTREP) != 0)) ||
			(ppme->pme_nosync != ((flags & PTELD_NOSYNC) != 0))),
				"pteload - remap flags");
#ifdef VAC
		/*
		 * Reloading a pte for an already mapped page.	If
		 * the page is "real", then if we've got a VAC, then
		 * flush the cache to ensure no protection mismatches
		 * between cache and MMU.
		 * XXX - need to store protections in pme to avoid this.
		 *
		 * XXX - this could cause writeback errors if protection
		 *	 was changed from writeable to read-only.
		 */
		if ((pp != NULL) && vac && pte.pg_r &&
		    *(int *)&pte != *(int *)&opte)
			vac_pageflush(addr);
#endif VAC
	}
	ppme->pme_intrep = (flags & PTELD_INTREP) != 0;
	ppme->pme_nosync = (flags & PTELD_NOSYNC) != 0;

	/* keep some statistics on the cache-ability of the translation */
#if defined(VAC)
	if (pte.pg_type == OBMEM) {
		if (seg->s_as == &kas) {
			if (addr >= DVMA) {
				if (!pte.pg_nc)
					cache_stats.cs_ioc++;
				else
					cache_stats.cs_ionc++;
			} else {
				if (!pte.pg_nc)
					cache_stats.cs_kc++;
				else
					cache_stats.cs_knc++;
			}
		} else {
			if (!pte.pg_nc)
				cache_stats.cs_uc++;
			else
				cache_stats.cs_unc++;
		}
	} else {
		cache_stats.cs_other++;
	}
#else defined(VAC)
	if (pte.pg_type == OBMEM) {
		if (seg->s_as == &kas) {
			if (addr >= DVMA) {
				cache_stats.cs_ionc++;
			} else {
				cache_stats.cs_knc++;
			}
		} else {
			cache_stats.cs_unc++;
		}
	} else {
		cache_stats.cs_other++;
	}
#endif defined(VAC)

	if ((flags & PTELD_LOCK) == 0) {
		/*
		 * Decrement lock count. It was incr. by hat_pmgalloc().
		 */
		s = splvm();
		ppmg->pmg_keepcnt--;
#ifdef MMU_3LEVEL
		if (mmu_3level)
			hat_pmgtosmg(ppmg)->smg_keepcnt--;
#endif MMU_3LEVEL
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

void
hat_getctx(as)
	struct as *as;
{
	register struct ctx	*ctx, *ttctx;
	register struct pmgrp	*pmg;
	register u_short	tt = 0;
	int			s;
#ifdef MMU_3LEVEL
	register struct smgrp *smg;
#endif MMU_3LEVEL

	if (as->a_hat.hat_ctx) {
		as->a_hat.hat_ctx->c_clean = 0;
		return;
	}


	/* find a free ctx or an old one */
	ttctx = NULL;
	for (ctx = ctxhand + 1; ctx != ctxhand; ctx++) {
		if (ctx == ctxsNCTXS) /* wrap around the ctx table */
			ctx = ctxs;
		if (ctx->c_lock != 0)	/* can't touch */
			continue;
		/*
		 * Take ctx with no address space.
		 */
		if (ctx->c_as == NULL) {
			ttctx = ctx;
			break;
		}
		if (ttctx == NULL || ctx->c_time <= tt) {
			ttctx = ctx;		/* new "best" ctx */
			tt = ctx->c_time;
		}
	}

	TEST_PANIC(ttctx == NULL, "hat_getctx - no ctx's");

	ctxhand = ctx = ttctx;

	/*
	 * Update vmhatstat statistics.
	 */
	if (ctx->c_as) {
		if (ctx->c_clean)
			vmhatstat.vh_ctxstealclean++;
		else
			vmhatstat.vh_ctxstealflush++;
	} else {
		vmhatstat.vh_ctxfree++;
	}

	mmu_setctx(ctx);

	if (ctx->c_as) {
		vac_flushallctx();
		s = splvm();

#ifdef MMU_3LEVEL
		if (mmu_3level) {
			/* invalidate any smgrps already loaded for this ctx */
			for (smg = ctx->c_as->a_hat.hat_smgrps; smg != NULL;
			    smg = smg->smg_next)
				mmu_smginval(smg->smg_base);
		} else {
#endif MMU_3LEVEL
			/* invalidate any pmgrps already loaded for this ctx */
			for (pmg = ctx->c_as->a_hat.hat_pmgrps; pmg != NULL;
			    pmg = pmg->pmg_next) {

				if (pmg->pmg_mapped) {
					ASSERT(pmg->pmg_num != PMGNUM_SW);
					mmu_pmginval(pmg->pmg_base);
					pmg->pmg_mapped = 0;
				}
			}
#ifdef	MMU_3LEVEL
		}
#endif	MMU_3LEVEL
		ctx->c_as->a_hat.hat_ctx = NULL;
		(void)splx(s);
	}

	s = splvm();
	ctx->c_as = as;
	ctx->c_clean = 0;
	ctx->c_time = ctx_time++;
	as->a_hat.hat_ctx = ctx;

#ifdef MMU_3LEVEL
	if (mmu_3level) {
		/* load up any smgrps already allocated to this hat */
		for (smg = as->a_hat.hat_smgrps; smg != NULL;
		    smg = smg->smg_next)
			mmu_setsmg(smg->smg_base, smg);
	} else {
#endif MMU_3LEVEL
		/* load up any pmgrps already allocated to this hat */
		for (pmg = as->a_hat.hat_pmgrps; pmg != NULL;
		    pmg = pmg->pmg_next) {

			if (pmg->pmg_num != PMGNUM_SW) {
				ASSERT(!pmg->pmg_mapped);
				mmu_setpmg(pmg->pmg_base, pmg);
				pmg->pmg_mapped = 1;
				vmhatstat.vh_ctxmappmgs++;
			}
		}
#ifdef	MMU_3LEVEL
	}
#endif	MMU_3LEVEL

	(void)splx(s);
}


/*
 * Used to lock down hat resources for an address range. In this implementation,
 * this means locking down the necessary pmegs. This currently works only
 * for kernel addresses.
 */
void
hat_reserve(seg, addr, len)
	struct seg *seg;
	addr_t addr;
	u_int len;
{
	register	addr_t a;
	addr_t		ea;
	struct pmgrp	*pmg;

	TEST_PANIC(seg->s_as != &kas, "hat_reserve");

	for (a = addr, ea = addr + len; a < ea; a += MMU_PAGESIZE) {
		pmg = hat_pmgalloc(seg, a);
		TEST_PANIC(pmg == NULL, "hat_reserve: pmg == NULL");
		hat_pmgload(pmg);
	}
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
	register struct page	*pp = page_numtopp(pfnum);
	register struct pmgrp	*pmg;
	register struct pment	*pme;
	int			s;
	addr_t			va;
	struct ctx		*ctxsav, *nctx, *curctx;
	struct pte		tpte;

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
		if ((nctx = pmg->pmg_as->a_hat.hat_ctx) != NULL &&
		    !nctx->c_clean && pmg->pmg_mapped) {
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

#ifdef sun4c
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
		 * it.	Note that more than one process can share the
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
#endif /* sun4c */

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
	struct pmgrp		*ppmg;
	register struct pment	*ppme;
	addr_t			vaddr;
	int			flags;
{
	int			s;
	struct page		*pp;
	struct pment		*pmentp;
	u_int			ppmeind;
#ifdef VAC
	struct pment		*pme;	/* temporary for listwalk */
	struct pmgrp		*pmg;	/* temporary for listwalk */
	struct pment		*qpme;	/* second temporary for comparison */
	struct pmgrp		*qpmg;	/* second temporary for comparison */
	addr_t			pa, qa; /* matching address values */
	int			ccf;	/* cache conflict found flag */
	int			s2;
#endif VAC

	s = splvm();
	pp = ppme->pme_page;
	ppmeind	 = PMENXT_INDEX(ppme);
	if (pp != NULL) {
		/*
		 * Remove it from the list of mappings for the page.
		 */
		if (ppme == (struct pment *)(pp->p_mapping)) {
			pp->p_mapping = (caddr_t)PMENXT_PTR(ppme->pme_next);
			ppme->pme_next = PMENXT_NULL;
		} else {
			TEST_PANIC(pp->p_mapping == NULL,
			    "hat_pteunload - no mappings");

			for (pmentp = ((struct pment *) (pp->p_mapping));
			    pmentp->pme_next != ppmeind;
			    pmentp = PMENXT_PTR(pmentp->pme_next))
				TEST_PANIC(pmentp->pme_next == PMENXT_NULL,
				    "hat_pteunload - no mapping");

			pmentp->pme_next = ppme->pme_next;
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
		s = splvm();
	}
	/*
	 * Invalidate the translation.
	 */
	if (ppme->pme_valid) {
		flags |= HAT_INVSYNC;
		hat_ptesync(pp, ppmg, ppme, vaddr, flags);
		ppme->pme_nosync = ppme->pme_intrep = ppme->pme_valid = 0;
	}
	(void) splx(s);
}

/*
 * Synchronize the hardware and software of a pte.  Used for updating the
 * hardware nocache bit, the software R & M bits, and invalidating ptes.
 */
static void
hat_ptesync(pp, pmg, pme, vaddr, flags)
	struct page		*pp;
	register struct pmgrp	*pmg;
	register struct pment	*pme;
	addr_t			vaddr;
	int			flags;
{
	register struct ctx	*ctxsav, *nctx;
	register addr_t		mapaddr;
	int			s, pmg_off;
	struct pte		pte;
	struct pte		*ppte;	/* pointer to SW pte */
	struct as		*as;
	struct ctx		*ctx;
	int			usetemp;
	int			dommu;
	int			doflush;
	int			didsetpte = 0;
	int			didflush = 0;

	ppte = hat_pmetopte(pmg, pme);
	as = pmg->pmg_as;
	ctx = as->a_hat.hat_ctx;

	TEST_PANIC(pme->pme_valid == 0, "hat_ptesync - invalid pme");

	/*
	 * The HAT_VADDR flag means that the vaddr argument contains a valid
	 * page address.
	 *
	 * It's used as optimization when hat_ptesync is called
	 * from hat_unload(). We know that:
	 *	(1) the pmg context is setup
	 *	(2) we don't need to splvm() here since we will not
	 *	    be using SEGTEMP
	 *
	 */
	if (flags & HAT_VADDR) {

		if (hat_pmgisloaded(pmg)) {
			/*
			 * pmg is loaded in HW - make sure that it is mapped.
			 */
			hat_pmgmap(pmg);

			mapaddr = vaddr;
			dommu = 1;
			usetemp = 0;
			/*
			 * We must flush VAC if the context is not clean.
			 */
			doflush = !ctx->c_clean;

		} else {
			/*
			 * pmg is not loaded in MMU
			 */
			dommu = doflush = usetemp = 0;
			mapaddr = (addr_t)0;
		}
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

	if (!hat_pmgisloaded(pmg)) {
		/*
		 * SW page table is not loaded.
		 */
		dommu = doflush = usetemp = 0;

		/*
		 * XXX - Set mapaddr to 0 so that we get a kernel text fault
		 * and panic if we try to use it in the code below.
		 */
		mapaddr = (addr_t)0;
	} else if (ctx == NULL) {
		/*
		 * pmg is loaded but its as does not have a context.
		 * Set things up so that the pmgrp is mapped into a temporary
		 * segment.  No need to do any VAC flushing since this
		 * was done when we took the ctx away.	Set
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
		dommu = usetemp = 1;
		doflush = 0;
	} else {
		/*
		 * pmg is loaded and pmg's as has a ctx.
		 * Make sure we are in running in the as context.
		 * Use the virtual address as the mapping address.
		 */
		if ((nctx = ctx) != ctxsav)
			mmu_setctx(nctx);

		/*
		 * Make sure that pmg is mapped.
		 */
		hat_pmgmap(pmg);

		mapaddr = vaddr;
		dommu = 1;
		usetemp = 0;
		/*
		 * We must flush VAC if the context is not clean.
		 */
		doflush = !ctx->c_clean;
	}
skip:
	if (!vac)
		doflush = 0;		/* no VAC on this system */

	if (dommu) {
		/* Assert that the pmg is mapped */
		ASSERTPMGMAPPED(pmg, "ptesync");
	}

	/*
	 * At this point, the flags are set:
	 *
	 * doflush - if the page must be flushed
	 * dommu   - if pte must be get/set from HW MMU
	 * usetemp - if SEGTEDMP is used to access the HW MMU map
	 *
	 * Note that a loaded pmg is also mapped if we have a context.
	 */
	if (pp != NULL) {
		if (flags & HAT_RMSYNC) {

			if (dommu) {
				mmu_getpte(mapaddr, ppte);
			}
			pte = *ppte;

			/*
			 * Call back to inform address space, if turned on.
			 */
			if (as->a_hatcallback) {
			    as_hatsync(as, vaddr, (u_int) pte.pg_r,
				(u_int) pte.pg_m,
				(u_int)(flags & HAT_INVSYNC ? AHAT_UNLOAD : 0));
			}
			pg_setref(pp, pp->p_ref | pte.pg_r);
			pg_setmod(pp, pp->p_mod | pte.pg_m);
#ifdef VAC
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
			if (doflush && pte.pg_r) {
				vac_pageflush(mapaddr);
				didflush = 1;
			}
#endif VAC
			pte.pg_r = pte.pg_m = 0;
		}
#ifdef VAC
		else if (flags & HAT_NCSYNC) {

			if (dommu) {
				mmu_getpte(mapaddr, ppte);
			}
			pte = *ppte;
			/*
			 * N.B.	 The following test assumes that there
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

				goto skip_ncsync;
			}

			/*
			 * To avoid lots of problems, we don't try to convert
			 * anything from cached to non-cached (or vice-versa)
			 * when it is being loaded for DVMA use.
			 */
			if (mapaddr < DVMA) {
				if (doflush && !pte.pg_nc && pp->p_nc) {
					int pri, iskas;

					/*
					 * Need to convert from a cached
					 * translation to a non-cached
					 * translation.	 There are lots
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
					 * The stack being considered safe
					 * will need to be watched if/when
					 * we go away from using a fixed
					 * virtual address for the user area
					 * that is not managed by the hat layer.
					 */
					TEST_PANIC(!pmg->pmg_mapped,
					    "hat_ptesync - pmg not mapped");

					pte.pg_nc = 1;

					iskas = (as == &kas);

					pri = splhigh();

					if (pte.pg_r)
						vac_pageflush(mapaddr);

					/* Change both SW and HW pte */
					*ppte = pte;
					mmu_setpte(mapaddr, pte);
					didsetpte = 1;

					if (iskas) {
						/*
						 * Flush the virtual address
						 * again just in case some IO
						 * got in behind our back
						 * above.  Doing this for
						 * iskas only assumes there
						 * is no UDVMA to worry about.
						 */
						struct pte	tpte;

						mmu_getpte(mapaddr, &tpte);
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
			} else {
				cache_stats.cs_iowantchg++;
			}
		skip_ncsync:
			;
		}
#endif VAC
	}

	if (flags & HAT_INVSYNC) {
#ifdef VAC
		if (vac) {
			if (mapaddr >= DVMA && mapaddr < DVMA + dvmasize) {
				/*
				 * Always flush before invalidating a DVMA
				 * translation because the ref bit may lie.
				 * See bugid 1039410.
				 */
				vac_pageflush(mapaddr);
			} else if (doflush && !didflush) {
				struct pte	tpte;

				mmu_getpte(mapaddr, &tpte);
				if (tpte.pg_r)
					vac_pageflush(mapaddr);
			}
		}
#endif VAC
		pte = mmu_pteinvalid;
	}

	if (!didsetpte) {
		*ppte = pte;
		if (dommu) {
			mmu_setpte(mapaddr, pte);
		}
	}

	/*
	 * Optimized return when hat_ptesync was called from hat_unload.
	 */
	if (flags & HAT_VADDR)
		return;
	if (usetemp)
		mmu_settpmg(SEGTEMP, pmgrp_invalid);
	(void) splx(s);
	if (nctx != ctxsav)
		mmu_setctx(ctxsav);
}

/*
 * Allocate a SW page table for a given address.
 */
static struct pmgrp *
hat_pmgalloc(seg, addr)
	struct seg *seg;
	addr_t addr;
{
	register struct as *as = seg->s_as;
	register struct pmgrp *pmg;
	int s;

	s = splvm();
	if ((pmg = hat_pmgfind(addr, as)) != NULL) {
		vmhatstat.vh_pmgallochas++;
		(void) splx(s);
		return (pmg);
	}

	/*
	 * No pmgrp allocated to this address space contains the address;
	 * allocate a new pmg for this address space.  First, try
	 * the free list.
	 */

	/*
	 * Update vmhatstat statistics.
	 */
	if (pmgrpfree == NULL)
		vmhatstat.vh_pmgallocsteal++;
	else
		vmhatstat.vh_pmgallocfree++;

top:
	if ((pmg = pmgrpfree) == NULL) {
		int try;

		/*
		 * No SW pmg's free, have to take one from someone.
		 * Take from address spaces with no ctx first.
		 */
		pmg = pmgrphand;
		for (try = 1; /* empty */; try++) {
			do {
				pmg++;
				if (pmg == pmgrpsNPMGRPS) {
					/*
					 * Wrap around and skip kernels pmgs.
					 */
					pmg = pmgrpmin;
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
				panic("hat_pmgalloc out of hat");
			}
		}
	}

	pmgrpfree = pmg->pmg_next;	/* take it off the free list */
	pmg->pmg_lock = 1;
	pmg->pmg_keepcnt = 1;

	hat_pmglink(pmg, as, addr);
	pmg->pmg_lock = 0;

	(void) splx(s);

	return (pmg);
}

/*
 * Load SW pmg in HW
 */
static void
hat_pmgload(swpmg)
	struct pmgrp		*swpmg;
{
	register struct as	*as;
	register struct pmgrp	*pmg = (struct pmgrp *)NULL;
	register struct hwpmg	*hwpmg;
	int			s;
	int			pass;

	if (swpmg->pmg_mapped) {
		ASSERT(swpmg->pmg_num != PMGNUM_SW);
		goto locksmg;
	}

	as = swpmg->pmg_as;

	/*
	 * Loaded, but not mapped.
	 */
	if (swpmg->pmg_num != PMGNUM_SW) {
		ASSERT(as == &kas || mmu_getctx()->c_as == as);
		hat_pmgmap(swpmg);
		goto locksmg;
	}

	hat_setup(as);
	s = splvm();

	if ((hwpmg = hwpmgfree) != NULL) {
		vmhatstat.vh_pmgldfree++;
		goto found_free_hwpmg;
	}

	/*
	 * The strategy for stealing a HW pmeg:
	 * We make 3 passes over the array of HW pmegs (starting at hwpmghand)
	 * and will take the pmeg if:
	 *
	 * Pass 1. pmeg does not require VAC flush
	 *    (we clean all pmegs between Pass 1 and Pass 2).
	 * Pass 2. Take any unlocked pmeg. In pass 2, we will take
	 *    even a dirty pmeg because it may be an unused kernel pmeg.
	 */
	hwpmg = hwpmghand;
	for (pass = 1; pass < 3; pass++) {
		struct ctx	*ctx;

		do {
			hwpmg++;
			if (hwpmg == hwpmgsNHWPMGS) {
				/*
				 * Wrap around and skip kernel pmgs.
				 */
				hwpmg = hwpmgmin;
			}

			pmg = hwpmg->hwp_pmgrp;
			ASSERT(pmg != NULL);
			ASSERT(pmg->pmg_num != PMGNUM_SW);

			/*
			 * Skip locked and kept pmg's
			 */
			if (pmg->pmg_lock || pmg->pmg_keepcnt > 0) {
				continue;
			}

			if (pass == 1 &&
			    (ctx = pmg->pmg_as->a_hat.hat_ctx) != NULL &&
			    !ctx->c_clean && pmg->pmg_mapped)
				continue;

			if (pass == 1) {
				if (ctx == NULL)
					vmhatstat.vh_pmgldnoctx++;
				else if (ctx->c_clean)
					vmhatstat.vh_pmgldcleanctx++;
				else
					vmhatstat.vh_pmgldnomap++;
			}

			/*
			 * Keep the pmg until hat_pmgswapptes().
			 */
			pmg->pmg_keepcnt++;
			hat_pmgunload(pmg, PTESFLAG_SKIP);
			ASSERT(hwpmg = hwpmgfree);
			hwpmghand = hwpmg;
			goto found_free_hwpmg;

		} while (hwpmg != hwpmghand);

		if (pass == 1) {

			/*
			 * We haven't found a pmg that does not require
			 * VAC flush. Clear all pmg's now by flushing
			 * all contexts.
			 */
			vac_flushallctx();
			hat_unmap_aspmgs(as);
			vmhatstat.vh_pmgldflush++;
		}
	}
	panic("hat_pmgload: failed after two  passes");

found_free_hwpmg:
	hat_clrcleanbit();
	swpmg->pmg_num = (hwpmg - hwpmgs);
	hwpmg->hwp_pmgrp = swpmg;
	hwpmgfree = hwpmg->hwp_next;	/* take it off the free list */

	ASSERT(swpmg->pmg_as == &kas || mmu_getctx()->c_as == swpmg->pmg_as);

#ifdef MMU_3LEVEL
	if (mmu_3level)
		hat_smgalloc(as, swpmg->pmg_base, swpmg);
#endif MMU_3LEVEL

	/* We are already at splvm() */
	mmu_setpmg(swpmg->pmg_base, swpmg);
	swpmg->pmg_mapped = 1;

	/*
	 * Load all valid SW PTE's in MMU.
	 *
	 */
	if (pmg == (struct pmgrp *)NULL)
		hat_pmgloadptes(swpmg->pmg_base, swpmg->pmg_pte);
	else {
		hat_pmgswapptes(swpmg->pmg_base, swpmg->pmg_pte, pmg->pmg_pte);
		pmg->pmg_keepcnt--;
	}

	as->a_hat.hat_pmgldcnt++;    /* incr. number of HW pmegs for this as */

	(void) splx(s);
	return;

locksmg:
#ifdef MMU_3LEVEL
	if (mmu_3level) {
		/*
		 * Callers to hat_pmgload assume that hat_pmgload keeps smg.
		 */
		s = splvm();
		hat_pmgtosmg(swpmg)->smg_keepcnt++;
		(void)splx(s);
	}
#endif MMU_3LEVEL
	return;
}

/*
 * Map a pmgrp in segment map.
 */
static void
hat_pmgmap(pmg)
	struct pmgrp	*pmg;
{
	int		s;

	if (pmg->pmg_num != PMGNUM_SW && !pmg->pmg_mapped)  {
		s = splvm();
#ifdef MMU_3LEVEL
		if (mmu_3level) {
			hat_smgalloc(pmg->pmg_as, pmg->pmg_base, pmg);
			hat_pmgtosmg(pmg)->smg_keepcnt--;
		}
#endif MMU_3LEVEL
		mmu_setpmg(pmg->pmg_base, pmg);
		pmg->pmg_mapped = 1;
		(void) splx(s);
	}
}

/*
 * Free the specified SW pmgrp.	 This is done by calling hat_pteunload
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
	register struct pment	*pme = pmg->pmg_pme;
	register struct as	*as;
	register int		tcnt;
	int			s;

	ASSERT(pmg->pmg_keepcnt == 1);
	ASSERT(pmg->pmg_as != NULL);

	if (hat_pmgisloaded(pmg))
		hat_pmgunload(pmg, PTESFLAG_UNLOAD);

	s = splvm();

	if ((as = pmg->pmg_as) != NULL) {
		for (tcnt = 0; tcnt < NPMENTPERPMGRP; tcnt++, pme++) {
			if (pme->pme_valid)
				hat_pteunload(pmg, pme, (addr_t)NULL,
				    HAT_RMSYNC);
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

	pmg->pmg_keepcnt--;
	pmg->pmg_next = pmgrpfree;
	pmgrpfree = pmg;

	(void) splx(s);
}

static void
hat_pmgunload(pmg, ptesflag)
	struct pmgrp	*pmg;
	enum ptesflag	ptesflag;
{
	addr_t		a;
	struct pte	*ppte = pmg->pmg_pte;
	struct hwpmg	*hwpmg;
	struct ctx	*ctx, *ctxsav;
	int		s;

	ASSERT(pmg->pmg_num != PMGNUM_SW);

	s = splvm();			/* using SEGTEMP */

#ifdef notdef				/* 3 level mmu */
	ASSERT(pmg->pmg_as->a_hat.hat_ctx != NULL ||  !pmg->pmg_mapped);
#endif notdef

	/*
	 * Unmap pmg from segment map.
	 */
	if (pmg->pmg_mapped) {

#ifdef MMU_3LEVEL
		if (mmu_3level) {
			ASSERT(pmg->pmg_sme != NULL);
		}
#endif MMU_3LEVEL

		if ((ctx = pmg->pmg_as->a_hat.hat_ctx) != NULL) {
			ctxsav = mmu_getctx();
			if (ctxsav != ctx)
				mmu_setctx(ctx);

			if (!ctx->c_clean)
				vac_segflush(pmg->pmg_base);

			mmu_pmginval(pmg->pmg_base);

			if (ctxsav != ctx)
				mmu_setctx(ctxsav);
		} else {
#ifdef MMU_3LEVEL
			if (mmu_3level) {
				/*
				 * We have to use REGTEMP to map the smg.
				 * Note that REGTEMP may be used only in kctx.
				 */

				struct smgrp	*smg;

				ctxsav = mmu_getctx();
				if (ctxsav != kctx)
					mmu_setctx(kctx);

				smg = &smgrps[(pmg->pmg_sme - sments)
				    >> NSMENTPERSMGRPSHIFT];
				mmu_setsmg(REGTEMP, smg);
				mmu_pmginval(REGTEMP +
				    ((u_int)pmg->pmg_base & SMGRPOFFSET));
				mmu_smginval(REGTEMP);

				if (ctxsav != kctx)
					mmu_setctx(ctxsav);
			} else
				panic("hat_pmgunload");
#else  MMU_3LEVEL
			panic("hat_pmgunload");
#endif MMU_3LEVEL
		}
		pmg->pmg_mapped = 0;

#ifdef MMU_3LEVEL
		if (mmu_3level) {
			pmg->pmg_sme->sme_valid = 0;
			pmg->pmg_sme->sme_pmg = (struct pmgrp *)NULL;
			pmg->pmg_sme = (struct sment *)NULL;
		}
#endif MMU_3LEVEL
	}

	ASSERT(!pmg->pmg_mapped);

	if (ptesflag == PTESFLAG_UNLOAD) {
		/*
		 * Unload all valid SW PTE's in MMU.
		 *
		 */
		map_setsgmap(SEGTEMP, pmg->pmg_num);
		a = SEGTEMP;
		hat_pmgunloadptes(a, ppte);
		map_setsgmap(SEGTEMP, PMGRP_INVALID);
	}

	pmg->pmg_as->a_hat.hat_pmgldcnt--;

	/*
	 * Put hwpmg in HW pmg free list
	 */
	hwpmg = &hwpmgs[pmg->pmg_num];
	hwpmg->hwp_next = hwpmgfree;
	hwpmgfree = hwpmg;
	hwpmg->hwp_pmgrp = NULL;

	pmg->pmg_num = PMGNUM_SW;

	(void) splx(s);
}

/*
 * Add the specified pmgrp to the list of pmgrp's allocated to
 * the specified address space.	 We hang pmgrps off the address
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

#ifdef notdef
/*
 * Called when the wrong pmeg is read out from the MMU.
 * Most likely, this is a down rev Carrera CPU board that
 * is missing some pullup registers on the segment RAMs.
 * The ECO for the needed Carrera CPU board fix is 2550.
 */
static void
hat_wrongpmg(pmg, addr, as)
	struct pmgrp *pmg;
	addr_t addr;
	struct as *as;
{
#ifndef sun2
	register struct pmgrp *pmgp;

	addr = (addr_t)((u_int)addr & ~(PMGRPSIZE - 1));
	printf("hardware claims page map entry group number is 0x%x\n",
	    pmg->pmg_num);
	if (addr >= (addr_t)KERNELBASE) {
		/*
		 * Scan the kernel address space for the correct pmg.
		 */
		for (pmgp = kas.a_hat.hat_pmgrps; pmgp != NULL;
		    pmgp = pmgp->pmg_next) {
			if (pmgp->pmg_base == addr)
				break;
		}
	} else {
		for (pmgp = as->a_hat.hat_pmgrps; pmgp != NULL;
		    pmgp = pmgp->pmg_next) {
			if (pmgp->pmg_base == addr)
				break;
		}
	}

	if (pmgp != NULL) {
		printf("  software says page map entry group number is 0x%x\n",
		    pmgp->pmg_num);
	}
#endif
	printf("pmg = %x, pmg base = %x, addr = %x\n", pmg, pmg->pmg_base,
	    addr);
	panic("wrong pmg");
	/* NOTREACHED */
}
#endif notdef

static void
hat_xfree(as)
	register struct as *as;
{
	register int s;
	register struct pmgrp *pmg;

	/*
	 * Free pmgrp's.
	 */
	s = splvm();
	while (pmg = as->a_hat.hat_pmgrps) {
		pmg->pmg_keepcnt++;
		(void) splx(s);
		hat_pmgfree(pmg);
		s = splvm();
	}
	(void) splx(s);

#ifdef MMU_3LEVEL
	/*
	 * If three-level mmu, free smgrp's.
	 */
	if (mmu_3level) {
		register struct smgrp *smg;

		s = splvm();
		while (smg = as->a_hat.hat_smgrps) {
			smg->smg_keepcnt++;
			(void) splx(s);
			hat_smgfree(smg);
			s = splvm();
		}
		(void) splx(s);
	}
#endif MMU_3LEVEL

	ASSERT(as->a_hat.hat_pmgldcnt == 0);
}


/*
 * Find a SW page table.
 *
 * The returned page table is 'kept'.
 *
 * We optimize the search by using a look-aside buffer of mappings
 * from <as, addr> to a pointer to the pmg structure. For each hashed
 * <as, addr> value, we store a pointer to the most recently found pmgrp.
 * 1009 is a prime that was found to be optimal.
 */

#define	hash_asaddr(as, addr) \
	((((u_int)as >> 2) * 1009 + ((u_int)addr >> PMGRPSHIFT)) % (NPMGHASH-1))

static struct pmgrp *
hat_pmgfind(addr, as)
	addr_t		addr;
	struct as	*as;
{
	struct pmgrp	*pmg;
	addr_t		pmgaddr = hat_pmgbase(addr);
	int		s;
	int		hashind;

	hashind = hash_asaddr(as, pmgaddr);

	s = splvm();

	/*
	 * Check if pmg is in the lookaside buffer.
	 */
	if ((pmg = pmghash[hashind]) != NULL &&
	    pmg->pmg_as == as && pmg->pmg_base == pmgaddr) {
		pmg->pmg_keepcnt++;
		pmgfindstat.pf_hit++;
		goto out;
	}

	/*
	 * Exhaustive search of pmg's within the address space.
	 */
	for (pmg = as->a_hat.hat_pmgrps; pmg != NULL &&
	    pmg->pmg_base != pmgaddr; pmg = pmg->pmg_next)
		;

	if (pmg != NULL) {
		pmg->pmg_keepcnt++;
		pmgfindstat.pf_miss++;

		/*
		 * Write the <as, addr> -> pmg entry to look aside buffer
		 */
		pmghash[hashind] = pmg;
	} else {
		pmgfindstat.pf_notfound++;
	}

out:
	(void) splx(s);
	return (pmg);
}


/*
 * Clear running process's context clean bit.
 */
static void
hat_clrcleanbit()
{
	struct	proc	*p;
	struct	as	*a;
	struct	ctx	*c;

	if ((p = u.u_procp) != NULL && (a = p->p_as) != NULL &&
	    (c = a->a_hat.hat_ctx) != NULL) {
		c->c_clean = 0;
	}
}


#ifdef MMU_3LEVEL
/*
 * Code to handle allocation of smegs is cloned from the pmeg versions
 */

int getsmg_check = 1;

/*
 * This routine will return the smgrp structure for the given address
 * in the current ctx.	But unlike mmu_getsmg, this routine will protect
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
 */
static void
hat_smgfree(smg)
	register struct smgrp *smg;
{
	register struct sment *sme;
	register struct pmgrp *pmg;
	register struct as *as;
	register int tcnt;
	struct ctx *ctx, *ctxsav, *curctx;
	int s;

	ASSERT(smg->smg_keepcnt == 1);

	if ((as = smg->smg_as) != NULL) {

		/* XXX - we should simplify switching between ctx's */
		ctxsav = curctx = mmu_getctx();
		if ((ctx = smg->smg_as->a_hat.hat_ctx) != NULL &&
		    !ctx->c_clean) {
			mmu_setctx(curctx = ctx);
			vac_rgnflush(smg->smg_base);
		}

		if (curctx != kctx)
			mmu_setctx(curctx = kctx);

		/*
		 * XXX - we may optimize by not using kctx if smg has ctx.
		 */
		s = splvm();
		ASSERT(mmu_getsmg(REGTEMP) == smgrp_invalid);
		mmu_setsmg(REGTEMP, smg);
		sme = smg->smg_sme;
		for (tcnt = 0; tcnt < NSMENTPERSMGRP; tcnt++) {
			if (sme->sme_valid) {
				pmg = sme->sme_pmg;
				ASSERT(((u_int)pmg->pmg_base & SMGRPOFFSET) <
				    SMGRPSIZE);
				mmu_pmginval(REGTEMP +
				    ((u_int)pmg->pmg_base & SMGRPOFFSET));
				pmg->pmg_mapped = 0;
				sme->sme_valid = 0;
				pmg->pmg_sme = (struct sment *)NULL;
				sme->sme_pmg = (struct pmgrp *)NULL;
			}
			sme++;
		}
		mmu_smginval(REGTEMP);
		(void) splx(s);
		if ((ctx = smg->smg_as->a_hat.hat_ctx) != NULL) {
			if (ctx != curctx)
				mmu_setctx(curctx = ctx);
			mmu_smginval(smg->smg_base);
		}
		if (ctxsav != curctx)
			mmu_setctx(ctxsav);

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
 * the specified address space.	 We hang smgrps off the address
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
	register struct as	*as = seg->s_as;
	register struct smgrp	*smg;
	register struct pmgrp	*pmg;
	struct sment		*sme;

	if (!mmu_3level)
		return;

	smg = hat_getsmg(addr);		/* keeps the smg for us */

	/* DEBUGGING */
	if (smg == smgrp_invalid)
		printf("hat_smgreserve: addr 0x%x invalid smg\n", addr);

	if (as->a_hat.hat_ctx == NULL ||
	    (smg->smg_as != NULL && smg->smg_as != as))
		panic("hat_smgreserve");

	if (smg != smgrp_invalid && smg->smg_as == NULL)
		hat_smglink(smg, as, addr);

	smg->smg_lock = 1;	/* if its being reserved, also lock it */

	/*
	 * Set up sme structure.
	 */
	pmg = mmu_getpmg(addr);
	sme = &(smg->smg_sme[(mmu_btop(addr-smg->smg_base)/NPMENTPERPMGRP)]);

	ASSERT(!sme->sme_valid || sme == pmg->pmg_sme);

	if (!sme->sme_valid) {
		pmg = mmu_getpmg(addr);
		ASSERT(pmg->pmg_sme == NULL);
		sme->sme_pmg  = pmg;
		sme->sme_valid = 1;
		pmg->pmg_sme = sme;
	}
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

	for (smg = smgrps; smg < smgrpsNSMGRPS; smg++) {
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
	struct as		*as;
	addr_t			addr;
	struct pmgrp		*pmg;
{
	register struct smgrp	*smg;
	register struct sment	*sme;
	int			s;
	struct ctx		*ctx;

	if (!mmu_3level)
		return;

	s = splvm();
	if ((smg = mmu_getsmg(addr)) != smgrp_invalid) {
		smg->smg_keepcnt++;
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
	if (smgrpfree != NULL)
		vmhatstat.vh_smgfree++;

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
				if (smg == smgrpsNSMGRPS) {
					if (smgrpmin) {
						/* skip some kernel smgrps */
						smg = smgrpmin;
					} else {
						smg = smgrps;
					}
				}

				if (smg->smg_lock == 0 &&
				    smg->smg_keepcnt == 0) {
					/*
					 * On the first try, only take a smg
					 * from an address space with no ctx.
					 */
					if (try < 3 &&
					    (ctx = smg->smg_as->a_hat.hat_ctx)
						!= NULL &&
					    !ctx->c_clean)
						continue;

					/*
					 * Found a candidate, free
					 * it up and try again.
					 */
					if (try == 1) {
						if (ctx == NULL)
						    vmhatstat.vh_smgnoctx++;
						else
						    vmhatstat.vh_smgcleanctx++;
					}

					smg->smg_keepcnt++;
					hat_smgfree(smg);
					smgrphand = smg;
					goto top;
				}
			} while (smg != smgrphand);

			if (try == 1) {
				/*
				 * We were not able to find a segment that
				 * would not require flushing.
				 *
				 * Flush all user VAC lines and try again.
				 */
				vac_flushallctx();
				vmhatstat.vh_smgflush++;
			}

			/*
			 * Give up after 2 tries.
			 */
			if (try >= 3) {
				rm_outofhat();
			}
			try++;
		}
	}
	hat_clrcleanbit();
	smgrpfree = smg->smg_next;	/* take it off the free list */
	smg->smg_lock = 1;
	smg->smg_keepcnt = 1;

	hat_smglink(smg, as, addr);

	sme = &(smg->smg_sme[(mmu_btop(addr-smg->smg_base)/NPMENTPERPMGRP)]);
	sme->sme_pmg  = pmg;
	sme->sme_valid = 1;
	pmg->pmg_sme = sme;

	(void) splx(s);

	mmu_setsmg(smg->smg_base, smg);

	smg->smg_lock = 0;

	return;
}


hat_smgcheck_keepcnt(smg)
	struct smgrp	*smg;
{
	int	s;

	/*
	 * smgrp_invalid has smg_keepcnt == 1, but it has no pmgs.
	 */
	if (smg == smgrp_invalid)
		return;

	s = splvm();
	{
		register struct sment *sme;
		register struct pmgrp *pmg;
		int		tcnt;
		int		keepcnt = 0;

		sme = smg->smg_sme;
		for (tcnt = 0; tcnt < NSMENTPERSMGRP; tcnt++) {
			if (sme->sme_valid) {
				pmg = sme->sme_pmg;
				keepcnt += pmg->pmg_keepcnt;
			}
			sme++;
		}

		if (keepcnt != smg->smg_keepcnt) {
			printf(
	"check_smgkeepcnt: keepcnt %d smg_keepcnt %d base 0x%x smg# %d\n",
			    keepcnt, smg->smg_keepcnt,
			    smg->smg_base, smg->smg_num);
			panic("possibly double mapped pmgrp");
		}

	}
	(void) splx(s);
}

hat_smgcheck_keepcntall()
{
	register struct smgrp *smg;

	for (smg = smgrps; smg < smgrpsNSMGRPS; smg++)
		hat_smgcheck_keepcnt(smg);
}
#endif MMU_3LEVEL

/*
 * Unmap all HW pmegs (except for locked pmegs) held by an address space.
 */
static void
hat_unmap_aspmgs(as)
	struct as	*as;
{
	struct pmgrp	*pmg;

	/*
	 * Don't do anything if this is kernel address space.
	 */
	if (as == &kas)
		return;

	/*
	 * We don't unmap pmegs if the address space holds only a
	 * "small number" of HW pmegs. We expect that future pmeg allocations
	 * will steal HW pmegs from other address spaces with a clean
	 * or no context.
	 *
	 * If the address space holds less than 'hatunmaplimit' percent
	 * of the total number of HW pmegs we don't unmap. hatunmaplimit
	 * is set to 30% and may be patched.
	 */

	if (as->a_hat.hat_pmgldcnt * 100 < NPMGRPS * hatunmaplimit)
		return;

	ASSERT(mmu_getctx()->c_as == as);

	for (pmg = as->a_hat.hat_pmgrps; pmg != NULL; pmg = pmg->pmg_next) {
		if (pmg->pmg_mapped && pmg->pmg_keepcnt == 0) {
			ASSERT(pmg->pmg_num != PMGNUM_SW);
			ASSERT(!pmg->pmg_lock);
			mmu_pmginval(pmg->pmg_base);
			pmg->pmg_mapped = 0;
		}
	}
}

#ifdef notdef
/* XXX - temporary (and quite dirty) assertion function */
assertpmgmapped(pmg, msg)
	struct pmgrp	*pmg;
	char		*msg;
{
	addr_t		a = pmg->pmg_base;

	if (pmg == NULL) {
		printf("%s addr 0x%x\n", msg, pmg->pmg_base);
		ASSERT(pmg != NULL);
	}

	if (pmg->pmg_as->a_hat.hat_ctx == NULL) {
#ifdef notdef				/* 3 level mmu */
		if (pmg->pmg_mapped) {
			printf("%s addr 0x%x\n", msg, pmg->pmg_base);
			ASSERT(!pmg->pmg_mapped);
		}
#endif notdef
		a = SEGTEMP;
	} else {
		if (!pmg->pmg_mapped) {
			printf("%s addr 0x%x\n", msg, pmg->pmg_base);
			ASSERT(pmg->pmg_mapped);
		}
	}

	if (pmg->pmg_num == PMGNUM_SW) {
		printf("%s addr 0x%x\n", msg, pmg->pmg_base);
		ASSERT(pmg->pmg_num != PMGNUM_SW);
	}

	if (map_getsgmap(a) == PMGRP_INVALID) {
		printf("%s addr 0x%x\n", msg, pmg->pmg_base);
		/* Force traceback */
		printf("pmg_num %d, pmg_keepcnt %d, ctx 0x%x\n",
		    pmg->pmg_num, pmg->pmg_keepcnt,
		    pmg->pmg_as->a_hat.hat_ctx);
		ASSERT(map_getsgmap(a) != PMGRP_INVALID);
	}
}
#endif notdef

