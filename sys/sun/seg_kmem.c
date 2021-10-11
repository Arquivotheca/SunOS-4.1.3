/*	@(#)seg_kmem.c  1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * VM - kernel segment routines
 */

#include <sys/param.h>
#include <sys/vm.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <sys/trace.h>

#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/seg_kmem.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/anon.h>
#include <vm/rm.h>
#include <vm/page.h>

/*
 * Machine specific public segments.
 */
struct seg kseg;
struct seg kdvmaseg;
struct seg bufheapseg;

/*
 * Private seg op routines.
 */
static	int segkmem_fault(/* seg, addr, len, type, rw */);
static	int segkmem_faulta(/* seg, addr */);
static	int segkmem_unload(/* seg, addr, ref, mod */);
static	int segkmem_setprot(/* seg, addr, len, prot */);
static	int segkmem_checkprot(/* seg, addr, len, prot */);
static	int segkmem_badop();


struct	seg_ops segkmem_ops = {
	segkmem_badop,		/* dup */
	segkmem_badop,		/* split */
	segkmem_badop,		/* free */
	segkmem_fault,
	segkmem_faulta,
	segkmem_unload,
	segkmem_setprot,
	segkmem_checkprot,
	segkmem_badop,		/* kluster */
	(u_int (*)()) NULL,	/* swapout */
	segkmem_badop,		/* sync */
	segkmem_badop,		/* incore */
	segkmem_badop,		/* lockop */
};

/*
 * The segkmem driver will (optional) use an array of pte's to back
 * up the mappings for compatibility reasons.  This driver treates
 * argsp as a pointer to the pte array to be used for the segment.
 */
int
segkmem_create(seg, argsp)
	struct seg *seg;
	caddr_t argsp;
{

	seg->s_ops = &segkmem_ops;
	seg->s_data = argsp;		/* actually a struct pte array */
	return (0);
}

static int
segkmem_fault(seg, addr, len, type, rw)
	struct seg *seg;
	addr_t addr;
	u_int len;
	enum fault_type type;
	enum seg_rw rw;
{
	register addr_t adr;

	/*
	 * For now the only `fault' that is supported by this driver
	 * is that of F_SOFTLOCK for the S_OTHER case.  This type of
	 * fault is used to denote "lock down already loaded
	 * translations".  This is used during UNIX startup.
	 */
	if (type != F_SOFTLOCK || rw != S_OTHER)
		return (-1);

	for (adr = addr; adr < addr + len; adr += PAGESIZE)
		hat_pmgreserve(seg, adr);
	return (0);
}

/*ARGSUSED*/
static int
segkmem_faulta(seg, addr)
	struct seg *seg;
	addr_t addr;
{

	return (-1);
}

/*ARGSUSED*/
static
segkmem_unload(seg, addr, ref, mod)
	struct seg *seg;
	addr_t addr;
	u_int ref, mod;
{

	/* ends up being called for mapout, ignore */
}

static int
segkmem_setprot(seg, addr, len, prot)
	struct seg *seg;
	addr_t addr;
	u_int len, prot;
{
	register struct pte *ppte;
	register addr_t eaddr;
	register u_int pprot;
#ifdef sun2
	struct ctx *uctx;

	if ((uctx = mmu_getctx()) != kctx)
		mmu_setctx(kctx);
#endif sun2

	pprot = hat_vtop_prot(prot);
	if (seg->s_data) {
		ppte = (struct pte *)seg->s_data;
		ppte += mmu_btop(addr - seg->s_base);
	} else
		ppte = NULL;
	for (eaddr = addr + len; addr < eaddr; addr += PAGESIZE) {
		if (prot == 0) {
			struct pmgrp *pmg = mmu_getpmg(addr);

			/*
			 * This code makes assumptions about kernel
			 * locking down pmegs that it really deals
			 * with on a page level when using this routine.
			 */
			if (pmg->pmg_lock == 0 && pmg->pmg_keepcnt == 0) {
				/* invalidate entire PMGRP */
				mmu_pmginval(addr);
			} else if (pmg != pmgrp_invalid) {
				/* invalidate PME */
				mmu_setpte(addr, mmu_pteinvalid);
			}
			/* invalidate software pte (if any) */
			if (ppte)
				*ppte++ = mmu_pteinvalid;
		} else {
			struct pte tpte;

			mmu_getpte(addr, &tpte);
			tpte.pg_prot = pprot;
#ifdef VAC
			tpte.pg_nc = 0;		/* XXX should use hat layer */
#endif VAC
			mmu_setpte(addr, tpte);
			if (ppte)
				*ppte++ = tpte;	/* structure assignment */
		}
	}
#ifdef sun2
	if (uctx != kctx)
		mmu_setctx(uctx);
#endif sun2
}

static int
segkmem_checkprot(seg, addr, len, prot)
	register struct seg *seg;
	register addr_t addr;
	u_int len;
	register u_int prot;
{
	register struct pte *ppte;
	register u_int pprot;
	struct pte tpte;
	addr_t eaddr;

	if (seg->s_data) {
		ppte = (struct pte *)seg->s_data;
		ppte += mmu_btop(addr - seg->s_base);
	} else
		ppte = NULL;

	for (eaddr = addr + len; addr < eaddr; addr += PAGESIZE) {
		if (ppte)
			tpte = *ppte++;		/* structure assignment */
		else
			mmu_getkpte(addr, &tpte);
		if (!pte_valid(&tpte)) {
			pprot = 0;
		} else {
			switch (tpte.pg_prot) {
			case KR:
				pprot = PROT_READ | PROT_EXEC;
				break;
			case UR:
				pprot = PROT_READ | PROT_EXEC | PROT_USER;
				break;
			case KW:
				pprot = PROT_READ | PROT_EXEC | PROT_WRITE;
				break;
			case UW:
				pprot = PROT_READ | PROT_EXEC | PROT_WRITE |
				    PROT_USER;
				break;
			default:
				pprot = 0;
				break;
			}
		}
		if ((pprot & prot) != prot)
			return (-1);
	}
	return (0);
}

static
segkmem_badop()
{

	panic("segkmem_badop");
	/*NOTREACHED*/
}

/*
 * Special public segkmem routines
 */

/*
 * Allocate physical pages for the given kernel virtual address.
 * Performs most of the work of the old memall/vmaccess combination.
 */
int
segkmem_alloc(seg, addr, len, canwait, type)
	struct seg *seg;
	addr_t addr;
	u_int len;
	int canwait;
	u_int type;
{
	struct page *pp;
	struct pte *ppte;
	struct pte tpte;
	int val;
#ifdef sun2
	register struct ctx *uctx;

	if ((uctx = mmu_getctx()) != kctx)
		mmu_setctx(kctx);
#endif sun2

	pp = rm_allocpage(seg, addr, len, canwait);
	if (pp != (struct page *)NULL) {
		trace6(TR_SEG_ALLOCPAGE, seg, (u_int)addr & PAGEMASK,
			TRC_SEG_KSEG, 0, (u_int)addr & PAGEMASK, pp);
		if (seg->s_data) {
			ppte = (struct pte *)seg->s_data;
			ppte += mmu_btop(addr - seg->s_base);
		} else
			ppte = NULL;
		while (pp != (struct page *)NULL) {
			hat_mempte(pp, PROT_ALL & ~PROT_USER, &tpte);
			hat_pteload(seg, addr, pp, tpte, PTELD_LOCK);
			addr += PAGESIZE;
			pp->p_offset = type;
			page_sub(&pp, pp);
			if (ppte)
				*ppte++ = tpte;	/* structure assignment */
		}
		val = 1;
	} else
		val = 0;

#ifdef sun2
	if (uctx != kctx)
		mmu_setctx(uctx);
#endif sun2
	return (val);
}

/*ARGSUSED*/
void
segkmem_free(seg, addr, len)
	register struct seg *seg;
	addr_t addr;
	u_int len;
{
	struct page *pp;
	struct pte *ppte;
	struct pte tpte;
#ifdef sun2
	register struct ctx *uctx;

	if ((uctx = mmu_getctx()) != kctx)
		mmu_setctx(kctx);
#endif sun2

	if (seg->s_data) {
		ppte = (struct pte *)seg->s_data;
		ppte += mmu_btop(addr - seg->s_base);
	} else
		ppte = NULL;

	for (; (int)len > 0; len -= PAGESIZE, addr += PAGESIZE) {
		if (ppte) {
			tpte = *ppte++;		/* structure assignment */
		} else {
			mmu_getpte(addr, &tpte);
		}
		pp = page_numtookpp(tpte.pg_pfnum);
		if (tpte.pg_type != OBMEM || pp == NULL)
			panic("segkmem_free");
		hat_unlock(seg, addr);
		hat_unload(seg, addr, PAGESIZE);
		PAGE_RELE(pp);			/* this should free the page */
	}

#ifdef sun2
	if (uctx != kctx)
		mmu_setctx(uctx);
#endif sun2
}

/*
 * mapin() and mapout() are for manipulating kernel addresses only.
 * They are provided for backward compaitibility. They have been
 * replaced by segkmem_mapin() and segkmem_mapout().
 */

/*
 * Map a physical address range into kernel virtual addresses.
 */
/*ARGSUSED*/
mapin(ppte, v, paddr, size, access)
	struct pte *ppte;
	u_int v;
	u_int paddr;
	int size, access;
{
	struct seg *seg;
	register addr_t vaddr;
	u_int vprot;

	vaddr = (addr_t)mmu_ptob(v);
	seg = as_segat(&kas, vaddr);
	if (seg == NULL)
		panic("mapin -- bad seg");
	if ((access & PG_V) == 0)
		panic("mapin -- invalid pte");
	switch (access & PG_PROT) {
	case PG_KR:
		vprot = PROT_READ;
		break;
	case PG_KW:
		vprot = PROT_READ | PROT_WRITE;
		break;
	case PG_UR:
		vprot = PROT_READ | PROT_USER;
		break;
	case PG_UW:
		vprot = PROT_READ | PROT_WRITE | PROT_USER;
		break;
	}
	segkmem_mapin(seg, vaddr, (u_int)mmu_ptob(size), vprot,
	    paddr & PG_PFNUM, 0);
}

/*
 * Release mapping for kernel.  This frees pmegs, which are a critical
 * resource.  ppte must be a pointer to a pte within Sysmap[].
 */
mapout(ppte, size)
	struct pte *ppte;
{
	addr_t vaddr;
	extern struct pte ESysmap[];

	if (ppte < Sysmap || ppte >= ESysmap)
		panic("mapout -- bad pte");
	vaddr = kseg.s_base + mmu_ptob(ppte - (struct pte *)kseg.s_data);
	segkmem_mapout(&kseg, vaddr, (u_int)mmu_ptob(size));
}

/*
 * segkmem_mapin() and segkmem_mapout() are for manipulating kernel
 * addresses only. Since some users of segkmem_mapin() forget to unmap,
 * this is done implicitly.
 * NOTE: addr and len must always be multiples of the mmu page size. Also,
 * this routine cannot be used to set invalid translations.
 */
void
segkmem_mapin(seg, addr, len, vprot, pcookie, flags)
	struct seg *seg;
	register addr_t addr;
	register u_int len;
	u_int vprot;
	u_int pcookie;
	int flags;
{
	register struct pte *ppte;
	struct page *pp;
	union {
		struct pte u_pte;
		int u_ipte;
	} apte, tpte;
#ifdef sun2
	register struct ctx *uctx;

	if ((uctx = mmu_getctx()) != kctx)
		mmu_setctx(kctx);
#endif sun2
	if (seg->s_data) {
		ppte = (struct pte *)seg->s_data;
		ppte += mmu_btop(addr - seg->s_base);
	} else
		ppte = (struct pte *)NULL;

	apte.u_ipte = pcookie;
	if (vprot == PROT_NONE)
		panic("segkmem_mapin -- invalid ptes");
	apte.u_pte.pg_prot = hat_vtop_prot(vprot);
	apte.u_pte.pg_v = 1;

	/*
	 * Always lock the mapin'd translations.
	 */
	flags |= PTELD_LOCK;

	for (; len != 0; addr += MMU_PAGESIZE, len -= MMU_PAGESIZE) {
		/*
		 * Determine the page frame we're mapping to allow
		 * the translation layer to manage cache consistency.
		 * If we're replacing a valid mapping, then ask the
		 * translation layer to unload the mapping and update
		 * data structures (necessary to maintain the information
		 * for cache consistency).  We use page_numtookpp here
		 * instead of page_numtopp so that we don't get a page
		 * struct for physical pages in transition.
		 */
		if (apte.u_pte.pg_type == OBMEM)
			pp = page_numtookpp(apte.u_pte.pg_pfnum);
		else
			pp = (struct page *)NULL;

		if ((flags & PTELD_INTREP) == 0) {
			/*
			 * Because some users of mapin don't mapout things
			 * when they are done, we check for a currently
			 * valid translation.  If we find one, then we
			 * unlock the old translation now.
			 */
			mmu_getpte(addr, &tpte.u_pte);
			if (pte_valid(&tpte.u_pte)) {
				hat_unlock(&kseg, addr);
				/*
				 * If the page is different than the one
				 * already loaded, unload it now.  We test
				 * to see if the page is different before
				 * doing this since we can blow up if this
				 * virtual page contains a page struct used
				 * by hat_unload and we are just reloading
				 * going from non-cached to cached.
				 */
				if ((tpte.u_ipte &
				    (PG_V | PG_PROT | PG_PFNUM)) !=
				    (apte.u_ipte & (PG_V | PG_PROT | PG_PFNUM)))
					hat_unload(&kseg, addr, MMU_PAGESIZE);
			}
		}

		hat_pteload(&kseg, addr, pp, apte.u_pte, flags);

		if (ppte != (struct pte *)NULL)
			*ppte++ = apte.u_pte;
		apte.u_pte.pg_pfnum++;
	}
#ifdef sun2
	if (uctx != kctx)
		mmu_setctx(uctx);
#endif sun2
}

/*
 * Release mapping for the kernel. This frees pmegs, which are a critical
 * resource. The segment must be backed by software ptes. The pages
 * specified are only freed if they are valid. This allows callers to
 * clear out virtual space without knowing if it's mapped or not.
 * NOTE: addr and len must always be multiples of the page size.
 */
void
segkmem_mapout(seg, addr, len)
	struct seg *seg;
	register addr_t addr;
	register u_int len;
{
	register struct pte *ppte;
#ifdef sun2
	register struct ctx *uctx;

	if ((uctx = mmu_getctx()) != kctx)
		mmu_setctx(kctx);
#endif sun2
	if (seg->s_as != &kas)
		panic("segkmem_mapout -- bad as");
	if (seg->s_data == NULL)
		panic("segkmem_mapout -- no ptes");
	ppte = (struct pte *)seg->s_data;
	ppte += mmu_btop(addr - seg->s_base);
	for (; len != 0; addr += MMU_PAGESIZE, len -= MMU_PAGESIZE, ppte++) {
		if (!pte_valid(ppte))
			continue;
		ppte->pg_v = 0;
		hat_unlock(&kseg, addr);
		hat_unload(&kseg, addr, MMU_PAGESIZE);
	}
#ifdef sun2
	if (uctx != kctx)
		mmu_setctx(uctx);
#endif sun2
}
