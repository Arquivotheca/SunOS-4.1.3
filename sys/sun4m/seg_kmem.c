#ifndef lint
static  char sccsid[] = "@(#)seg_kmem.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
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
 * extern decls missing from include files
 */
extern unsigned	pte_invalidate();	/* vm_hat.c */
extern unsigned	pte_setprot();		/* vm_hat.c */

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

/*ARGSUSED*/
static int
segkmem_fault(seg, addr, len, type, rw)
	struct seg *seg;
	addr_t addr;
	u_int len;
	enum fault_type type;
	enum seg_rw rw;
{
	return (-1);
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
	register union ptpe *sptpe;
	register union ptpe *dptpe;
	register addr_t eaddr;
	register u_int pprot;

	/* DEBUG */
	if ((addr < seg->s_base) ||
	    ((addr + len) > (seg->s_base + seg->s_size)))
		panic("segkmem_setprot -- out of segment");
	/* End DEBUG */

	sptpe = NULL;
	if (prot != 0)
		pprot = hat_vtop_prot(addr, prot);
	if (seg->s_data)
		dptpe = (((union ptpe *)seg->s_data) +
			 mmu_btop(addr - seg->s_base));
	else
		dptpe = NULL;

	for (eaddr = addr + len; addr < eaddr; addr += PAGESIZE) {
		if (((sptpe == NULL) ||
		     ((u_int)addr & (L3PTSIZE -1)) < PAGESIZE) &&
		    ((sptpe = hat_ptefind(seg->s_as, addr)) == NULL))
			break;

		if (prot == 0)
			(void)pte_invalidate(sptpe);
		else
			(void)pte_setprot(sptpe, pprot);

		if (dptpe != NULL)
			*dptpe++ = *sptpe;
		sptpe ++;
	}
}

static int
segkmem_checkprot(seg, addr, len, prot)
	register struct seg *seg;
	register addr_t addr;
	u_int len;
	register u_int prot;
{
	register union ptpe *ptpe;
	register u_int pprot;
	addr_t eaddr;

	/* DEBUG */
	if (addr < seg->s_base || (addr + len) > (seg->s_base + seg->s_size))
		panic("segkmem_checkprot");
	/* End DEBUG */

	if (seg->s_data)
		ptpe = (((union ptpe *)seg->s_data) +
			mmu_btop(addr - seg->s_base));
	else
		ptpe = NULL;

	for (eaddr = addr + len; addr < eaddr; addr += PAGESIZE) {
		if ((seg->s_data == NULL) &&
		    ((ptpe == NULL) ||
		     ((u_int) addr & (L3PTSIZE -1)) < PAGESIZE))
			ptpe = hat_ptefind(seg->s_as, addr);

		if ((ptpe == NULL) || (!pte_valid(&ptpe->pte)))
			pprot = 0;
		else
			pprot = hat_ptov_prot(&ptpe->pte);

		if ((pprot & prot) != prot)
			return (-1);
		ptpe++;
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
	struct pte tpte;
	struct pte *pte;

	pp = rm_allocpage(seg, addr, len, canwait);
	if (pp == NULL)
		return 0;

	trace6(TR_SEG_ALLOCPAGE, seg, (u_int)addr & PAGEMASK,
	       TRC_SEG_KSEG, 0, (u_int)addr & PAGEMASK, pp);

	if (seg->s_data)
		pte = (((struct pte *)seg->s_data) +
		       mmu_btop(addr - seg->s_base));
	else
		pte = NULL;

	while (pp != NULL) {
		hat_mempte(pp, PROT_ALL & ~PROT_USER, &tpte, addr);
		hat_pteload(seg, addr, pp, &tpte, PTELD_LOCK);
		addr += PAGESIZE;
		pp->p_offset = type;
		page_sub(&pp, pp);
		if (pte != NULL)
			*pte++ = tpte;
	}
	return(1);
}

/*ARGSUSED*/
void
segkmem_free(seg, addr, len)
	register struct seg *seg;
	addr_t addr;
	u_int len;
{
	register union ptpe *ptpe;
	struct page *pp;

	if (seg->s_data)
		ptpe = (((union ptpe *)seg->s_data) +
			mmu_btop(addr - seg->s_base));
	else
		ptpe = NULL;

	while (len > 0) {
		if ((seg->s_data == NULL) &&
		    ((ptpe == NULL) ||
		     ((u_int)addr & (L3PTSIZE - 1)) < PAGESIZE)) {
			ptpe = hat_ptefind(seg->s_as, addr);
			if (ptpe == NULL)
				panic("segkmem_free: no ptpe");
		}

		pp = page_numtookpp(ptpe->pte.PhysicalPageNumber);
		if (pp == NULL)
			panic("segkmem_free: no page");
		hat_unlock(seg, addr);
		hat_unload(seg, addr, PAGESIZE);
		PAGE_RELE(pp);			/* this should free the page */

		ptpe ++;
		len -= PAGESIZE;
		addr += PAGESIZE;
	}
}

/*
 * mapin() and mapout() are for manipulating kernel addresses only.
 * They are provided for backward compaitibility. They have been
 * replaced by segkmem_mapin() and segkmem_mapout().
 */
/*ARGSUSED*/
mapin(pte, v, paddr, size, access)
	struct pte *pte;
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
	if (!pte_valid((struct pte *)&access))
		panic("mapin -- invalid pte");
	vprot = hat_ptov_prot((struct pte *)&access);
	/* XXX check if vprot is == 0? */
	segkmem_mapin(seg, vaddr, (u_int)mmu_ptob(size), vprot,
		      paddr, 0);
}

/*
 * Release mapping for kernel.  This frees pmegs, which are a critical
 * resource.  ppte must be a pointer to a pte within Sysmap[].
 */
mapout(pte, size)
	struct pte *pte;
{
	addr_t vaddr;
	extern struct pte ESysmap[];

	if (pte < Sysmap || pte >= ESysmap)
		panic("mapout -- bad pte");
	vaddr = kseg.s_base + mmu_ptob(pte - (struct pte *)kseg.s_data);
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
	u_int pcookie;	/* Actually the physical page number */
	int flags;
{
	register union ptpe *sptpe;
	register union ptpe *dptpe;
	union ptpe apte;
	struct page *pp;

	if (seg->s_data)
		dptpe = (((union ptpe *)seg->s_data) +
			 mmu_btop(addr - seg->s_base));
	else
		dptpe = NULL;
	sptpe = NULL;

	apte.ptpe_int = MMU_STD_INVALIDPTE;
	apte.pte.PhysicalPageNumber = pcookie;
	if (vprot == PROT_NONE)
		panic("segkmem_mapin -- invalid ptes");
	apte.pte.AccessPermissions = hat_vtop_prot(addr, vprot);
	apte.pte.EntryType = MMU_ET_PTE;

	/*
	 * Always lock the mapin'd translations.
	 */
	flags |= PTELD_LOCK;

	for (; len != 0; addr += MMU_PAGESIZE, len -= MMU_PAGESIZE) {
		if ((sptpe == NULL) ||
		    (((u_int)addr & (L3PTSIZE - 1)) < MMU_PAGESIZE)) {
			sptpe = hat_ptefind(&kas, addr);
			if (sptpe == NULL)
				panic("segkmem_mapin: no pte");
		} else
			sptpe++;

		if (pte_valid(&sptpe->pte)) {
			hat_unlock(&kseg, addr);
			hat_unload(&kseg, addr, MMU_PAGESIZE);
		}

		pp = page_numtookpp(apte.pte.PhysicalPageNumber);
		hat_pteload(&kseg, addr, pp, &apte.pte, flags);
		if (dptpe != NULL)
			*dptpe++ = apte;
		apte.pte.PhysicalPageNumber++;
	}
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
	register struct pte *pte;
	if (seg->s_as != &kas)
		panic("segkmem_mapout -- bad as");
	if (seg->s_data == NULL)
		panic("segkmem_mapout -- no ptes");
	pte = (((struct pte *)seg->s_data) +
	       mmu_btop(addr - seg->s_base));

	for (; len != 0; addr += MMU_PAGESIZE, len -= MMU_PAGESIZE, pte++) {
		if (!pte_valid(pte))
			continue;
		pte->EntryType = MMU_ET_INVALID;
		hat_unlock(&kseg, addr);
		hat_unload(&kseg, addr, MMU_PAGESIZE);
	}
}
