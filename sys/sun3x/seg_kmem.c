#ifndef lint
static	char sccsid[] = "@(#)seg_kmem.c 1.1 92/07/30";
#endif
/*	synced to sun/seg_kmem.c 1.47 */
/*	synced to sun/seg_kmem.c 1.51 (4.1) */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * VM - kernel segment routines
 */

#include <sys/param.h>
#include <sys/vm.h>
#include <sys/user.h>
#include <sys/mman.h>

#include <machine/cpu.h>
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
	register struct pte *spte, *pte;
	register addr_t eaddr;
	register u_int pprot;

	/* DEBUG */
	if (addr < seg->s_base || (addr + len) > (seg->s_base + seg->s_size))
		panic("segkmem_setprot -- out of segment");
	/* DEBUG */

	pte = NULL;
	if (prot != 0)
		pprot = hat_vtop_prot(addr, prot);
	if (seg->s_data) {
		spte = (struct pte *)seg->s_data;
		spte += mmu_btop(addr - seg->s_base);
	} else
		spte = NULL;
	for (eaddr = addr + len; addr < eaddr; addr += PAGESIZE) {
		if (pte == NULL || ((u_int)addr & (L3PTSIZE - 1))
		    < PAGESIZE) {
			pte = hat_ptefind(seg->s_as, addr);
			if (pte == NULL)
				return;
		} else
			pte++;
		if (prot == 0)
			pte->pte_vld = PTE_INVALID;
		else
			pte->pte_readonly = pprot;
		if (spte != NULL)
			*spte++ = *pte;
	}
}

static int
segkmem_checkprot(seg, addr, len, prot)
	register struct seg *seg;
	register addr_t addr;
	u_int len;
	register u_int prot;
{
	register struct pte *pte;
	register u_int vprot;
	addr_t eaddr;

	/* DEBUG */
	if (addr < seg->s_base || (addr + len) > (seg->s_base + seg->s_size))
		panic("segkmem_checkprot");
	/* DEBUG */

	if (seg->s_data) {
		pte = (struct pte *)seg->s_data;
		pte += mmu_btop(addr - seg->s_base) - 1;
	} else
		pte = NULL;
	for (eaddr = addr + len; addr < eaddr; addr += PAGESIZE) {
		if (seg->s_data == NULL && (pte == NULL ||
		    ((u_int)addr & (L3PTSIZE - 1)) < PAGESIZE))
			pte = hat_ptefind(seg->s_as, addr);
		else
			pte++;
		if (pte == NULL || !pte_valid(pte))
			vprot = 0;
		else {
			vprot = PROT_READ | PROT_EXEC;
			switch (pte->pte_readonly) {
			case RO:
				vprot = PROT_READ | PROT_EXEC;
				break;
			case RW:
				vprot = PROT_READ | PROT_EXEC | PROT_WRITE;
				break;
			default:
				vprot = 0;
				break;
			}
		}
		/* XXX - assumes PROT_USER is never checked for - XXX */
		if ((vprot & prot) != prot)
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
	register struct pte *pte;
	struct pte tpte;

	pp = rm_allocpage(seg, addr, len, canwait);
	if (pp != (struct page *)NULL) {
		if (seg->s_data) {
			pte = (struct pte *)seg->s_data;
			pte += mmu_btop(addr - seg->s_base);
		} else
			pte = NULL;
		while (pp != (struct page *)NULL) {
			hat_mempte(pp, PROT_ALL & ~PROT_USER, &tpte, addr);
			hat_pteload(seg, addr, pp, &tpte, PTELD_LOCK);
			addr += PAGESIZE;
			pp->p_offset = type;
			page_sub(&pp, pp);
			if (pte != NULL)
				*pte++ = tpte;	/* structure assignment */
		}
		return (1);
	} else
		return (0);
}

/*ARGSUSED*/
void
segkmem_free(seg, addr, len)
	register struct seg *seg;
	addr_t addr;
	u_int len;
{
	register struct pte *pte;
	struct page *pp;

	if (seg->s_data) {
		pte = (struct pte *)seg->s_data;
		pte += mmu_btop(addr - seg->s_base) - 1;
	} else
		pte = NULL;
	for (; (int)len > 0; len -= PAGESIZE, addr += PAGESIZE) {
		if (seg->s_data == NULL && (pte == NULL ||
		    ((u_int)addr & (L3PTSIZE - 1)) < PAGESIZE))
			pte = hat_ptefind(seg->s_as, addr);
		else
			pte++;
		pp = page_numtookpp(pte->pte_pfn);
		if (pp == NULL)
			panic("segkmem_free");
		hat_unlock(seg, addr);
		hat_unload(seg, addr, PAGESIZE);
		PAGE_RELE(pp);
	}
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
mapin(pte, vnum, pnum, size, access)
	struct pte *pte;
	u_int vnum;
	u_int pnum;
	int size, access;
{
	struct seg *seg;
	register addr_t vaddr;
	u_int vprot;

	vaddr = (addr_t)mmu_ptob(vnum);
	seg = as_segat(&kas, vaddr);
	if (seg == NULL)
		panic("mapin -- bad seg");
	if (!pte_valid((struct pte *)&access))
		panic("mapin -- invalid pte");
	if (((struct pte *)&access)->pte_readonly)
		vprot = PROT_READ;
	else
		vprot = PROT_READ | PROT_WRITE;
	segkmem_mapin(seg, vaddr, (u_int)mmu_ptob(size), vprot, pnum, 0);
}

/*
 * Release mapping for kernel. ppte must be a pointer to a pte within Sysmap[].
 */
mapout(pte, size)
	struct pte *pte;
	int size;
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
	u_int pcookie;
	int flags;
{
	register struct pte *pte, *hpte = (struct pte *)NULL;
	struct pte apte;
	struct page *pp;

	if (seg->s_data) {
		pte = (struct pte *)seg->s_data;
		pte += mmu_btop(addr - seg->s_base);
	} else
		pte = (struct pte *)NULL;

	*(u_int *)&apte = MMU_INVALIDPTE;
	apte.pte_pfn = pcookie;
	if (vprot == PROT_NONE)
		panic("segkmem_mapin -- invalid ptes");
	apte.pte_readonly = hat_vtop_prot(addr, vprot);
	apte.pte_vld = PTE_VALID;

	/*
	 * Always lock the mapin'd translations.
	 */
	flags |= PTELD_LOCK;

	for (; len != 0; addr += MMU_PAGESIZE, len -= MMU_PAGESIZE) {
		if (hpte == NULL || ((u_int)addr & (L3PTSIZE - 1))
		    < MMU_PAGESIZE) {
			hpte = hat_ptefind(&kas, addr);
			if (hpte == NULL)
				panic("mapin -- no pte");
		} else
			hpte++;
		if (pte_valid(hpte)) {
			hat_unlock(&kseg, addr);
			hat_unload(&kseg, addr, MMU_PAGESIZE);
		}
		if (bustype(apte.pte_pfn) == BT_OBMEM)
			pp = page_numtookpp(apte.pte_pfn);
		else
			pp = (struct page *)NULL;
		hat_pteload(&kseg, addr, pp, &apte, flags);

		if (pte != (struct pte *)NULL)
			*pte++ = apte;
		apte.pte_pfn++;
	}
}

/*
 * Release mapping for the kernel. The segment must be backed by software ptes.
 * The pages specified are only freed if they are valid. This allows callers to
 * clear out virtual space without knowing if it's mapped or not.
 * NOTE: addr and len must always be multiples of the mmu page size.
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
	pte = (struct pte *)seg->s_data;
	pte += mmu_btop(addr - seg->s_base);
	for (; len != 0; addr += MMU_PAGESIZE, len -= MMU_PAGESIZE, pte++) {
		if (!pte_valid(pte))
			continue;
		pte->pte_vld = PTE_INVALID;
		hat_unlock(&kseg, addr);
		hat_unload(&kseg, addr, MMU_PAGESIZE);
	}
}
