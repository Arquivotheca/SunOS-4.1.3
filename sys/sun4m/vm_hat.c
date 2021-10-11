#ifndef lint
static  char sccsid[] = "@(#)vm_hat.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*
 * VM - Hardware Address Translation management.
 *
 * This file implements the machine specific hardware translation
 * needed by the VM system.  The machine independent interface is
 * described in <vm/hat.h> while the machine dependent interface
 * and data structures are described in <machine/vm_hat.h>.
 *
 * The hat layer manages the address translation hardware as a cache
 * driven by calls from the higher levels in the VM system.  Nearly
 * all the details of how the hardware is managed shound not be visable
 * about this layer except for miscellanous machine specific functions
 * (e.g. mapin/mapout) that work in conjunction with this code.  Other
 * than a small number of machine specific places, the hat data
 * structures seen by the higher levels in the VM system are opaque
 * and are only operated on by the hat routines.  Each address space
 * contains a struct hat and a page contains an opaque pointer which
 * is used by the hat code to hold a list of active translations to
 * that page.
 */

/* Hardware address translation routines for the SPARC(tm) Reference
 * MMU. Originally constructed from pegasus code.
 */

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/debug.h>
#include <sys/types.h>
#include <sys/user.h>			/* for u_ru.ru_minflt */
#include <sys/trace.h>
#include <sys/systm.h>
#include <sys/mbuf.h>

#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/mmu.h>
#include <machine/devaddr.h>
#include <machine/param.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/page.h>
#include <vm/mp.h>
#include <vm/rm.h>
#include <vm/seg_u.h>
#include <vm/seg_vn.h>
#include <vm/vpage.h>

#ifdef	MULTIPROCESSOR
#include <machine/async.h>	/* so percpu.h isn't confused */
#include "percpu.h"
#endif	MULTIPROCESSOR
extern struct seg *segkmap;

extern u_int nswctxs;
extern u_int nl1pts;

#ifdef	MULTIPROCESSOR
extern struct ptbl *percpu_ptbl;	/* machdep.c */
extern union ptpe percpu_ptpe[];	/* machdep.c */
#endif	MULTIPROCESSOR

#ifndef MULTIPROCESSOR
#define xc_attention() 
#define xc_dismissed()
#else	MULTIPROCESSOR
extern	xc_attention();
extern	xc_dismissed();
#endif	MULTIPROCESSOR


extern void mmu_flushpagectx();
extern void mmu_flushseg();
extern void mmu_flushrgn();
extern void mmu_flushctx();
extern void mmu_flushall();

#ifndef	VAC
#define	vac_pagectxflush(va,cno)
#define	vac_segflush(va)
#define	vac_rgnflush(va)
#define	vac_ctxflush(cno)
#define	vac_flushall()
#else	VAC
extern void vac_pagectxflush();
extern void vac_pageflush();
extern void vac_segflush();
extern void vac_rgnflush();
extern void vac_ctxflush();
extern void vac_flushall();
#endif	VAC

unsigned	pte_read();
unsigned	pte_write();
unsigned	pte_decache();
unsigned	pte_recache();
unsigned	pte_invalidate();
unsigned	pte_setprot();
unsigned	pte_offon();
unsigned	pte_rmsync();

/*
 * Private vm_hat data structures.
 */
static struct ctx *ctxhand;		/* hand for allocating contexts */
static struct ptbl *ptblhand;		/* hand for page table alloc */
static struct ptbl *ptblmax;		/* last page table open for alloc */
static u_short  ptbl_freelist;		/* page table free list */


#ifdef KMON_DEBUG
static kmon_t   ptbl_lock;		/* locks ptbls[] */
static kmon_t   ctx_lock;		/* locks ctxs[] */
#endif					/* KMON_DEBUG */

static void hat_pteunload(/* ptbl, pte, mode */);
static void hat_ptesync(/* pp, pte, flags */);
static struct pte *hat_ptealloc(/* as, addr */);
static struct ptbl *hat_ptblalloc(/* */);
static void hat_ptblempty(/* ptbl */);
static void hat_ptbllink(/* ptbl, as, addr, lvl */);
static void hat_ptblunlink(/* ptbl, lvl, vaddr */);

extern void	stphys();
extern u_int	ldphys();
extern u_int	swphys();

/*
 * forward declarations for stuff at the bottom.
 */
int		va2pp();
unsigned	va2pa();

/*
 * Semi-private data
 */
struct ctx *ctxs, *ectxs;		/* used by <machine/mmu.c> */
struct ptbl *ptbls, *eptbls;		/* the page table array */
struct pte *ptes, *eptes;		/* the pte array */
struct spte *sptes, *esptes;		/* the pte array */
union ptpe *contexts, *econtexts;	/* The hardware context table */
struct l1pt  *l1pts, *el1pts;		/* Level 1 table pool, non-cached */

#ifdef	IOMMU
extern int iom;
extern union iommu_pte *ioptes, *eioptes; /* virtual addr of ioptes */
#endif	IOMMU

u_int pts_addr;				/* physical address of page tables */

#ifdef HAT_DEBUG
u_int load_debug=0;
u_int prot_debug=0;
u_int find_debug=0;
u_int alloc_debug=0;
u_int unload_debug=0;
u_int setup_debug=0;
u_int ptbl_debug=0;
#endif

extern addr_t	econtig;	/* XXX - start of segu */
extern addr_t	eecontig;	/* XXX - end of segu */

extern	int	nmod;		/* number of modules */

extern char DVMA[];             /* addresses in kas above DVMA are for "IO" */

/*
 * Global data
 */
struct as kas;				/* kernel's address space */
struct ctx *kctx;			/* kernel's context */

#define VAC_MASK(a) (((u_int)(a) & MMU_PAGEMASK) & (shm_alignment - 1))
#define VAC_ALIGNED(a1, a2) ((VAC_MASK(a1) ^ VAC_MASK(a2)) == 0)

/*
 * There are a number of page tables whose ptbl_lockcnt keeps getting
 * higher and higher, eventually overflowing the counter. Should some
 * task, say the sundiag "pmem" test, then map a page from that page table
 * into its address space at an improperly aliased virtual address, then
 * the mapping in the table whose lockcnt overflowed would be unmapped.
 * Then, the next time the newly invalidated mapping is used, a fault
 * occurs. This happens with the page table containing the mappings for a
 * number of mbuf clusters, resulting in BAD TRAP faults when the kernel
 * gets a DATA FAULT at addresses in the kernel heap area. Ptbls based at
 * 0xFF040000 and FF640000 have been observed to have this problem in the
 * current GENERIC kernel; other configs may move the "FF040000" number
 * around a bit.
 * 
 * One line of reasoning is, if there are 65536 outstanding locks to a page
 * table, it is probably supposed to be permanently locked, so we just
 * freeze the count at 65535. This avoids the problem by not allowing
 * hat_pteload to demap the page.
 *
 * The variable "ptbl_lock_perm_show" controls whether we inform
 * the console when a ptbl is permanently locked. This is turned off
 * by default.
 * 
 * XXX - better would be to find out where the unmatched lock counts are
 * coming from and fix them so the counter never overflows.
 * 
 * XXX - The precise value of this constant needs to be derived from the
 * definition of the "ptbl_lockcnt" field in the "ptbl" structure. As long
 * as it is a nonzero value that can compare true to some value of
 * ptbl_lockcnt, the LOCK_PERM related code should work, the intent is for
 * it to be the maximum value that ptbl_lockcnt can take on before
 * incrementing to zero.
 */

int	ptbl_lock_perm_show = 0;
#define	PTBL_LOCK_PERM	(0xFFFF)		/* max u_short value */
#define	PTBL_LOCK_MAX	(PTBL_LOCK_PERM - 1)


/*
 * Initialize the hardware address translation structures.
 * Called by startup() after the vm structures have been allocated
 * and mapped in.
 */
void
hat_init()
{
	register struct ptbl *ptbl;
	register struct ctx *ctx;
	int i;
	int	sx = splvm();

	/*
	 * All the arrays were zero filled in startup(), so there is no
	 * need to initialize them here (luckily 0 is an invalid pte).
	 */
	i = 0;
	for (ctx = ctxs; ctx < ectxs; ctx++)
		ctx->c_num = i++;

	/*
	 * The first NUM_LOCKED_CTXS (0, .. NUM_LOCKED_CTXS-1)
	 * contexts are always locked, so we start allocating
	 * contexts at the context whose number is NUM_LOCKED_CTXS.
	 */
	ctxhand = &ctxs[NUM_LOCKED_CTXS];

	/*
	 * Initialize the page table clock hand.
	 */
	ptblhand = ptbls;
	/*
	 * Link all the ptbls into the free list. Note that the fact that
	 * they end up on the free list in reverse order is counted on later
	 * when the kernel ptbls are allocated.
	 */
	ptbl_freelist = PTBL_NULL;
	for (ptbl = ptbls; ptbl < eptbls; ptbl++) {
		ptbl->ptbl_next = ptbl_freelist;
		ptbl_freelist = ptbltonum(ptbl);
	}
	/*
	 * initialize CPU 0's context table.
	 */
	i = nctxs;
	while (i-->0)
		contexts[i].ptpe_int = MMU_STD_INVALIDPTP;

	/*
	 * Create a free pool of level 1 tables.
	 */
	for (i=0; i < nl1pts; i++)
		hat_l1free(&l1pts[i]);

	/*
	 * We must initialize kas here so that hat_ptblreserve() can add
	 * the allocated tables to the list for kas. This list will
	 * later get destroyed when we call hat_setup(&kas), but we
	 * have to let the code build it anyway. Losing the list doesn't
	 * hurt us since all the tables are locked forever anyway.
	 */
	kctx = &ctxs[KCONTEXT];
	kctx->c_as = &kas;
	kas.a_hat.hat_ctx = kctx;
	kas.a_hat.hat_ptvalid = 0;
	kas.a_hat.hat_oncpu = 0;
	kas.a_hat.hat_l2pts = kas.a_hat.hat_l3pts = PTBL_NULL;

	(void)splx(sx);
}

/*
 * Free all the translation resources for the specified address space.
 * Called from as_free when an address space is being destroyed.
 */
void
hat_free(as)
	register struct as *as;
{
	int	sx = splvm();
	register struct ptbl *ptbl;
	register struct l1pt *l1pt;
	register struct ctx *ctx;

	/*
	 * Switch to the kernel address space page tables.
	 */
	mmu_setctx(0);		/* Switch to kernel context */

	if (!as->a_hat.hat_ptvalid) {
		(void)splx(sx);
		return;
	}
		
	/*
	 * Release all the page tables owned by the address space.
	 * note that freeing a last 3rd level pt pointer in 
	 * a 2nd level pt causes the 2nd level pt 
	 * to be freed, and that freeing the last 2nd level pt in a 1st
	 * level pt causes the 1st level pt to be freed
	 */
	while (ptbl = numtoptbl(as->a_hat.hat_l3pts)) {
		hat_ptblempty(ptbl);
		(void)splx(splx(sx)); /* poll interrupts */
	}

	/*
	 * Free the context if we have one.
	 */
	if (ctx = as->a_hat.hat_ctx) {
		as->a_hat.hat_ctx = NULL;
		ctx->c_as = NULL; /* This frees the context */
	}

	/*
	 * Free the context if we have one.
	 */
	if (l1pt = as->a_hat.hat_l1pt) {
		as->a_hat.hat_l1pt = NULL;
		hat_l1free(l1pt);
	}

	/*
	 * Invalidate the hat structure.
	 */
	as->a_hat.hat_oncpu = 0;
	as->a_hat.hat_ptvalid = 0;

	(void)splx(sx);
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
	struct pte ptpe;
	int	sx = splvm();

	hat_mempte(pp, prot, &ptpe, addr);
	hat_pteload(seg, addr, pp, &ptpe, lock ? PTELD_LOCK : 0);

	(void)splx(sx);
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
	union	ptpe ptpe;
	int	sx = splvm();

	ptpe.ptpe_int = MMU_STD_INVALIDPTP;
	ptpe.pte.PhysicalPageNumber = pf;
	ptpe.pte.AccessPermissions = hat_vtop_prot(addr, prot);
	ptpe.pte.EntryType = MMU_ET_PTE;

	if (bustype(pf) == BT_OBMEM)
		pp = page_numtouserpp((u_int)pf);
	else
		pp = NULL;
	hat_pteload(seg, addr, pp, (struct pte *)&ptpe, lock ? PTELD_LOCK : 0);
	u.u_ru.ru_minflt++;
	(void)splx(sx);
}

/*
 * Release one hardware address translation lock on the given address.
 * This means decrementing the keep count on the page table.
 */
void
hat_unlock(seg, addr)
	struct seg *seg;
	addr_t addr;
{
	register union ptpe *ptpe;
	int	sx = splvm();

	if (seg->s_as->a_hat.hat_ptvalid == 0) {
		(void)splx(sx);
		return;
	}

	ptpe = hat_ptefind(seg->s_as, addr);
	/*
	 * If the address was mapped, we need to unlock
	 * the page table it resides in.
	 */
	if (pte_valid(&ptpe->pte))
		hat_unlock_ptbl(ptetoptbl(&ptpe->pte));
	(void)splx(sx);
}

/*
 * Unlock a ptbl. Decrements the keepcnt, and cleans up after
 * things that might have been left in a strange state.
#ifdef VAC
 * Check to see if we now can cache any non-cached pages.  For now, we
 * use the simple minded algorithm and just unload any of the locked
 * translations if the corresponding page is currently marked as
 * non-cachable.  This situation doesn't happen all the much, so the
 * efficency doesn't have to be all that great.
#endif 
 */
hat_unlock_ptbl(ptbl)
	register struct ptbl *ptbl;
{
	hat_unlock_ptbl_pte(ptbl, (struct pte *)0);
}

hat_unlock_ptbl_pte(ptbl, pte)
	register struct ptbl *ptbl;
	register struct pte *pte;
{
	register unsigned short lct;
	register int unloaded = 0;
	int	sx = splvm();

	/*
	 * Decrement the keep count.
	 *
	 * XXX - THIS IS A BAND-AID THAT MUST BE FIXED.
	 *
	 * Somewhere during initialization, we are decrementing the
	 * lock count for several kernel ptpes, when these lock counts
	 * are already zero. If we leave it at zero here, we end up
	 * crashing about three hours. Plainly, the protocol has
	 * been screwed if we do this, so we lock the table for
	 * the rest of time.
	 *
	 * Also, we have the other half of the code that fixes
	 * the other break in lockcnt: since we do not increment
	 * the count above PTBL_LOCK_MAX, the balance between
	 * increments and decrements is destroyed, so if the
	 * counter is at PTBL_LOCK_PERM, we leave it alone.
	 */
	lct = ptbl->ptbl_lockcnt;
	if (lct == 0) {
		ptbl->ptbl_lockcnt = lct = PTBL_LOCK_PERM;
		if (ptbl_lock_perm_show)
			printf("ptbl at %x frozen by unlock\n",
				 ptbl->ptbl_base);
	} else if (lct != PTBL_LOCK_PERM) {
		lct -= 1;
		ptbl->ptbl_lockcnt = lct;
	}
#ifdef VAC
	if (cache && (lct == 0) && ptbl->ptbl_ncflag) {
		register struct pte *apte;
		register struct spte *aspte;
		register int cnt;
		struct page *pp;

		ptbl->ptbl_ncflag = 0;

		/* get the first pte for this ptbl */
		apte = ptbltopte(ptbl);
		aspte = ptetospte(apte);
		for (cnt = 0; cnt < NL3PTEPERPT; cnt++, apte++, aspte++) {
			if (aspte->spte_valid && 
			    (pp = ptetopp(apte)) != NULL && 
			    pp->p_nc) {
				if (pte && (pte == apte)) {
					hat_pteunload(ptbl, pte, HAT_NOSYNC);
					unloaded = 1;
				} else {
					hat_pteunload(ptbl, apte, HAT_RMSYNC);
				}
			}
		}
	}
#endif
	if (pte && !unloaded)
		hat_pteunload(ptbl, pte, HAT_NOSYNC);
	(void)splx(sx);
}

/*
 * Change the protections in the virtual address range
 * given to the specified virtual protection. If vprot is ~PROT_WRITE,
 * then remove write permission, leaving the other
 * permissions unchanged. If vprot is ~PROT_USER, we are supposed to
 * to remove user permissions. However, this MMU doesn't let us do this
 * except at 32Mb granularity, so we panic.
 */
void
hat_chgprot(seg, addr, len, vprot)
	struct seg *seg;
	addr_t addr;
	u_int len;
	u_int vprot;			/* virtual page protections */
{
	register addr_t a, ea;
	register union ptpe *ptpe = NULL;
	register u_int pprot, newprot;
	int	sx = splvm();

	/*
	 * Make sure there is a context set up for the address space.
	 */
#ifdef HAT_DEBUG
	if(prot_debug)
		printf("hat_chgprot: seg %x addr %x len %x vprot %x\n",
			seg, addr, len, vprot);
#endif
	hat_setup(seg->s_as);
	/*
	 * Convert the virtual protections to physical ones. We can do
	 * this once with the first address because the kernel won't be
	 * in the same segment with the user, so it will always be
	 * one or the other for the entire length. If vprot is ~PROT_WRITE,
	 * turn off write permission.
	 */
	if (vprot != ~PROT_USER && vprot != ~PROT_WRITE)
		pprot = hat_vtop_prot(addr, vprot);

	ea = addr + len;

	for (a = addr; a < ea; a += MMU_PAGESIZE) {
		if ((ptpe == NULL) ||
		    ((u_int)a & (L3PTSIZE - 1)) < MMU_PAGESIZE) {
			ptpe = hat_ptefind(seg->s_as, a);
			/*
			 * If there is no page table, move the address up
			 * to the start of the next page table to avoid
			 * searching a bunch of invalid ptes.
			 */
			if (ptpe == NULL) {
				a = (addr_t)((u_int)a & ~(L3PTSIZE - 1)) +
					L3PTSIZE - MMU_PAGESIZE;
				continue;
			}
		} else
			ptpe ++;

		if (!pte_valid(&ptpe->pte))
			continue;
		if (vprot == ~PROT_WRITE) {
			switch(ptpe->pte.AccessPermissions) {
			case MMU_STD_SRWURW:
			case MMU_STD_SRWXURWX:
			case MMU_STD_SRWUR:
				pprot = MMU_STD_SRUR;
				newprot = 1;
				break;
			case MMU_STD_SRWX:
				pprot = MMU_STD_SRX;
				newprot = 1;
				break;
			case MMU_STD_SRUR:
			case MMU_STD_SRXURX:
			case MMU_STD_SXUX:
			case MMU_STD_SRX:
				newprot = 0;
				break;
			}
		}
		else if (vprot == ~PROT_USER) {
			switch(ptpe->pte.AccessPermissions) {
			case MMU_STD_SRWURW:
			case MMU_STD_SRWXURWX:
			case MMU_STD_SRWUR:
				pprot = MMU_STD_SRWX;
				newprot = 1;
				break;
			case MMU_STD_SRUR:
			case MMU_STD_SXUX:
			case MMU_STD_SRXURX:
				pprot = MMU_STD_SRX;
				newprot = 1;
				break;
			case MMU_STD_SRWX:
			case MMU_STD_SRX:
				newprot = 0;
				break;
			}
		}
		else if (ptpe->pte.AccessPermissions != pprot)
			newprot = 1;
		else
			newprot = 0;

		if (newprot) {
			(void)pte_setprot(ptpe, pprot);
#ifdef HAT_IOMAP
			/*
			 * Synchronize the I/O Mapper.
			 */
			IOMAP_SYNC(a, ptpe);
#endif
		}

	}

	(void)splx(sx);
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
 * addr and len must be MMU_PAGESIZE aligned.
 */
void
hat_unload(seg, addr, len)
	struct	seg *seg;
	addr_t	addr;
	u_int	len;
{
	register addr_t a;
	register union ptpe *ptpe = NULL;
	register struct spte *spte;
	register struct ptbl *ptbl;
	addr_t	ea;
	int	sx = splvm();

#ifdef HAT_DEBUG
	if(unload_debug)
		printf("hat_unload: seg %x addr %x len %x\n",
			seg, addr, len);
#endif

	if (seg->s_as->a_hat.hat_ptvalid == 0) {
		(void)splx(sx);
		return;
	}

	for (a = addr, ea = addr + len; a < ea; a += MMU_PAGESIZE) {
		if ((ptpe == NULL) ||
		    ((u_int)a & (L3PTSIZE - 1)) < MMU_PAGESIZE) {
			ptpe = hat_ptefind(seg->s_as, a);
			/*
			 * If there is no page table, increase addr to the
			 * start of the next page table to avoid searching
			 * a bunch of invalid ptes.
			 */
			if (ptpe == NULL) {
				a = (addr_t)((u_int)a & ~(L3PTSIZE - 1)) +
				    L3PTSIZE - MMU_PAGESIZE;
				continue;
			}
		} else
			ptpe++;

		/*
		 * Need to check to see if the pte is valid!
		 * Otherwise, this may result in a bad ptbl
		 * computed from the macro below, and this
		 * could then result in hat_pteunload panicing!!!
		 */
		if (!pte_valid(&ptpe->pte))
			continue;
		spte = ptetospte(&ptpe->pte);
		if(spte == NULL)
			panic("hat_unload: no spte for given pte");
		/*
		 * If ghostunloading, decrement keep count and unload
		 * without syncing. Otherwise, unload normally.
		 * NOTE: we must decrement the keep count before unloading
		 * so the page table will get freed when empty.
		 */
		ptbl = ptetoptbl(&ptpe->pte);
		if (spte->spte_nosync) {
			/*
			 * Decrement the lock and unload this
			 * translation. Had to mush the
			 * pteunload sortof into the midle
			 * of hat_unlock_ptbl, or the ptbl
			 * might not get free'd.
			 */
			hat_unlock_ptbl_pte(ptbl, &ptpe->pte);
		} else {
			hat_pteunload(ptbl, &ptpe->pte, HAT_RMSYNC);
		}
	}

	(void)splx(sx);
}

/*
 * Unload all the hardware translations that map page `pp'.
 */

void
hat_pageunload(pp)
struct page *pp;
{
	register struct spte *spte;
	int	sx = splvm();

	while (spte = (struct spte *)pp->p_mapping)
		hat_pteunload((struct ptbl *)sptetoptbl((spte)),
			      sptetopte(spte), HAT_RMSYNC);

	(void)splx(sx);
}

/*
 * Get all the hardware dependent attributes for a page struct
 */
void
hat_pagesync(pp)
	struct page *pp;
{
	register struct spte *spte;
	int	sx = splvm();

	/*
	 * Call ptesync() for each mapping of this page.
	 */

	for (spte = (struct spte *)pp->p_mapping; spte != NULL;
		spte = numtospte(spte->spte_next))
		hat_ptesync(pp, sptetopte(spte), HAT_RMSYNC);

	(void)splx(sx);
}

/*
 * Returns a page frame number for a given kernel virtual address.
 */
u_int
hat_getkpfnum(addr)
	addr_t	addr;
{
	union ptpe	ptpe;
/*
 * %%% this will fail badly if we start using
 * short translations, the probe will return
 * the page frame of the beginning of the
 * region mapped by the pte.
 */
	ptpe.ptpe_int = mmu_probe(addr);
	return ((ptpe.pte.EntryType == MMU_ET_PTE) ?
		ptpe.pte.PhysicalPageNumber : NULL);
}

/*
 * End of machine independent interface routines.
 *
 * The next few routines implement some machine dependent functions
 * needed for the SRMMU. Note that each hat implementation can define
 * whatever additional interfaces make sense for that machine. These
 * routines are defined in <machine/vm_hat.h>.
 *
 * Start machine specific interface routines.
 */

#ifdef	MULTIPROCESSOR
/*
 * Initialize the context table and context zero's
 * level 1 translation table for all processors
 * to provide the proper kernel context mapping
 * for everyone, with private cpu data mapped
 * in at VA_PERCPUME.
 */
void
hat_percpu_setupall()
{
	int cpu_id;

	for (cpu_id = 1; cpu_id < nmod; ++cpu_id)
		hat_setup_ctxtable(cpu_id);
}

hat_setup_ctxtable(cpu_id)
	int	cpu_id;
{
	union ptpe *ctx_table;
	union ptpe *l1_tbl;
	union ptpe *top_ptp;
	struct ptbl *ptbl;
	struct pte *pte, *percpu_pte;
	int i;
	int	sx = splvm();

/* allocate a level-2 page table for this CPU's per-CPU area. */
	ptbl = hat_ptblreserve((addr_t) VA_PERCPUME, 2);
	pte = ptbltopte(ptbl);

/* locate original ptes for mappings */
	percpu_pte = ptbltopte(percpu_ptbl) +
		cpu_id * (NL2PTEPERPT/16);

	i = NL2PTEPERPT;
/* invalidate ptes above what we can use */
	while (i-->NL2PTEPERPT/16)
		((unsigned *)pte)[i] = MMU_STD_INVALIDPTE;

/* copy in the valid mappings */
	while (i-->0)
		pte[i] = percpu_pte[i];

/* locate our level-1 table */
	l1_tbl = (union ptpe *)((u_int)kl1pt + cpu_id * PAGESIZE);

/* duplicate master level-1 table */
	bcopy((caddr_t)kl1pt, (caddr_t)l1_tbl, sizeof(struct l1pt));

/* establish low-memory one-to-one mappings */
	l1_tbl->ptpe_int = PTEOF(0, 0, MMU_STD_SRWX, 0);

/* find where to link the level-2 table into the level-1 table */
	top_ptp = &(l1_tbl[getl1in(VA_PERCPUME)]);

/* link new level-2 table into our level-1 table */
        top_ptp->ptpe_int = PTPOF((unsigned)pte - KERNELBASE);

/* save the level-1 ptp for hat_map_percpu */
	percpu_ptpe[cpu_id].ptp = top_ptp->ptp;

/* locate our context table */
	ctx_table = contexts + cpu_id * nctxs;

/* duplicate master context table */
	bcopy((caddr_t)contexts, (caddr_t)ctx_table, nctxs * sizeof (struct ptp));

/* link our level-1 table into our context table */
	ctx_table[0].ptpe_int = PTPOF(va2pa((addr_t)l1_tbl));

	(void)splx(sx);
}
/*
 * this routine is called from resume and yield_child before uunix
 * gets changed, so that a user process can be scheduled to 
 * run on some CPU.  this routine maps in the level-2 page table
 * for this CPU's per-CPU area.  note that this routine is called
 * from routines which don't have the kernel master lock, and
 * therefore this routine must be coded to run concurrently with
 * kernel code on another CPU.
 */
void
hat_map_percpu(up)
	struct user *up;
{
	int	sx = splvm();
	struct proc *pp;
	struct as *as;
	struct l1pt *l1pt;
	struct ptp *ptpp;

	if ((up != NULL) &&
	    (up != &idleuarea) &&
	    (pp = up->u_procp) &&
	    (pp != &idleproc) &&
	    (as = pp->p_as) &&
	    (as != &kas) &&
	    (as->a_hat.hat_ptvalid) &&
	    (l1pt = as->a_hat.hat_l1pt)) {
		ptpp = &(l1pt->ptpe[getl1in(VA_PERCPUME)].ptp);
		stphys(va2pa((addr_t)ptpp), percpu_ptpe[cpuid].ptpe_int);
		/*
		 * Since no processor ever references through the percpu_me
		 * area using the wrong mapping, no flushes are needed here.
		 */
	}

	(void)splx(sx);
}
#endif	MULTIPROCESSOR

/*
 * This routine is called for kernel initialization to cause a page table
 * to be allocated for the given address, locked, and linked into the
 * kernel address space.
 */
struct ptbl *
hat_ptblreserve(addr, lvl)
	addr_t	addr;
	int	lvl;
{
	register struct ptbl *ptbl;
	int	sx = splvm();

	ptbl = hat_ptblalloc();
#ifdef HAT_DEBUG
	if(ptbl_debug)
		printf("ptbl %x num %x addr %x lvl %x\n", ptbl, ptbltonum(ptbl),
			addr, lvl);
#endif
	hat_ptbllink(ptbl, &kas, addr, lvl);
	ptbl->ptbl_keep = 1;
	/*
	 * Reduce end of page table search to the ptbl before this one.
	 * We can do this because all the kernel ptbl's are allocated
	 * in reverse order at boot time.
	 */
	ptblmax = ptbl - 1;

	(void)splx(sx);
	return (ptbl);
}

/*
 * This routine is called for kernel initialization to copy ptes from
 * the temporary page tables to the real page tables. It locks the
 * new translations.
 */
void
hat_ptecopy(pte, npte)
	struct pte *pte, *npte;
{
	register struct ptbl *ptbl;
	register int i;
	int	sx = splvm();

	ptbl = ptetoptbl(npte);
	for (i = 0; i < NL3PTEPERPT; i++) {
		if (pte[i].EntryType != MMU_ET_INVALID)
			ptbl->ptbl_validcnt++;
		npte[i] = pte[i];
	}
	(void)splx(sx);
}

struct l1pt    *l1free_top = (struct l1pt *)0;
struct l1pt   **l1free_end = &l1free_top;

/*
 * hat_l1free(l1pt)
 * place this l1pt on the l1pt freelist,
 * which is organized as a FIFO.
 */
hat_l1free(l1pt)
	struct l1pt *l1pt;
{
	int s = splvm();
	*(struct l1pt **)l1pt = (struct l1pt *)0;
	*l1free_end = l1pt;
	l1free_end = (struct l1pt **)l1pt;
	(void)splx(s);
}

/*
 * hat_l1alloc()
 * Allocate a level 1 table from the free pool.
 * Try to use the entire table by going round the table.
 */
struct l1pt *
hat_l1alloc()
{
	int s = splvm();
	struct l1pt *l1pt;
	if (!(l1pt = l1free_top) ||
	    !(l1free_top = *(struct l1pt **)l1pt))
		panic("hat_l1alloc");
	(void)splx(s);
	return l1pt;
}


/*
 * Called by UNIX during initialization to finish setting
 * up the kernel address space.
 */
void
hat_setup_kas()
{
	int	sx = splvm();
	register struct as *as = &kas;
	register struct ctx *ctx = kctx;
	register struct l1pt *l1pt = hat_l1alloc();
	register unsigned	basepa = va2pa((addr_t)l1pt);
	u_int l1_index;

	as->a_hat.hat_l3pts = PTBL_NULL;
	as->a_hat.hat_l2pts = PTBL_NULL;
	as->a_hat.hat_l1pt = l1pt;
	as->a_hat.hat_oncpu = 0;
	as->a_hat.hat_ptvalid = 1;

#ifndef	MULTIPROCESSOR
	/* Invalidate the 1-1 mapping, va=0 to pa=0 */
	kl1pt->ptpe[0].ptpe_int = MMU_STD_INVALIDPTP;
	mmu_flushrgn(0);
	vac_rgnflush(0);
#endif	MULTIPROCESSOR

	/*
	 * Initialize the level 1 page table by copying
	 * over the level 1 page table made during startup.
	 */
	for (l1_index = MMU_NPTE_ONE - NKL2PTS;
	     l1_index < MMU_NPTE_ONE;
	     l1_index ++)
		stphys(basepa +
		       l1_index * sizeof (struct ptp),
		       kl1pt->ptpe[l1_index].ptpe_int);

#ifdef	MULTIPROCESSOR
	/*
	 * Initialize the per-cpu mappings
	 */
	hat_percpu_setupall();	/* construct percpu mappings */
#endif	MULTIPROCESSOR

	/*
	 * update all context tables.
	 */

	hat_update_ctables(ctx->c_num, PTPOF(basepa));

	/*
	 * Now set the hardware context 
	 * table pointer
	 */

	mmu_setctp((va2pa((addr_t)contexts) >>
		    MMU_STD_PTPSHIFT) <<
		   RMMU_CTP_SHIFT);
	mmu_flushall();
	vac_flushall();

	/* Now set the root pointer */
	mmu_setctx(ctx->c_num);
	(void)splx(sx);
}

/*
 * Called by UNIX during pagefault to insure the level 1 page table has
 * been initialized.
 */
void
hat_setup (as)
	register struct as *as;
{
	int                 sx = splvm();
	register unsigned   basepa;
	register struct ctx *ctx;
	register struct l1pt *l1pt;
	u_int               ix;
	int		    update_ctables = 0;

	if (as == &kas) {
		/*
		 * kas already set up (above).
		 * never need to do anything else.
		 */
		(void) splx (sx);
		return;
	}

	if (!as->a_hat.hat_ptvalid) {
		/*
		 * hat struct not valid.
		 * make an empty one.
		 */
		as->a_hat.hat_ctx = (struct ctx *)0;
		as->a_hat.hat_l3pts = PTBL_NULL;
		as->a_hat.hat_l2pts = PTBL_NULL;
		as->a_hat.hat_l1pt = (struct l1pt *)0;
		as->a_hat.hat_oncpu = 0;
		as->a_hat.hat_ptvalid = 1;
	}

	if (!(l1pt = as->a_hat.hat_l1pt)) {
		/*
		 * allocate and fill a level-1 page table.
		 */
		l1pt = hat_l1alloc ();
		basepa = va2pa ((addr_t) l1pt);

		ix = getl1in(KERNELBASE);
		while (ix-->0)
			stphys (basepa +
				ix * sizeof (struct ptp),
				0xFFFFFFFC);

		for (ix = MMU_NPTE_ONE - NKL2PTS;
		     ix < MMU_NPTE_ONE;
		     ix++)
			stphys (basepa +
				ix * sizeof (struct ptp),
				kl1pt->ptpe[ix].ptpe_int);

#ifdef	MULTIPROCESSOR
		/*
		 * fix up percpu region.
		 */
		stphys(basepa +
		       getl1in(VA_PERCPUME) * sizeof (struct ptp),
		       percpu_ptpe[cpuid].ptpe_int);
#endif	MULTIPROCESSOR

		as->a_hat.hat_l1pt = l1pt;
		update_ctables = 1;
	} else
		basepa = va2pa ((addr_t) l1pt);

	if (!(ctx = as->a_hat.hat_ctx)) {
		hat_getctx (as);
		ctx = as->a_hat.hat_ctx;
		update_ctables = 1;
	}

	if (update_ctables)
		hat_update_ctables (ctx->c_num, PTPOF(basepa));

/*
 * If this is the address space for the process
 * that is currently running on this processor,
 * then get into the context.
 * NOTE: If this is some other address space, then
 * getting into the context number could be fatal
 * as it may not have the right percpu area
 * mapped, and in fact may be active on some
 * other processor.
 */
	if (as == u.u_procp->p_as)
		mmu_setctx (ctx->c_num);
	else
		mmu_setctx (0);

	(void) splx (sx);
}

#ifdef	MULTIPROCESSOR
static
hat_vacate_context(cno)
	int cno;
{
	if (mmu_getctx() == cno) {
		flush_windows();
		mmu_setctx(0);
	}
}
#endif	MULTIPROCESSOR

/*
 * this routine keeps all the context tables consistent for user contexts.
 * it updates the specified entry of each context table with the specified
 * ptp. note that this is done efficiently, since all the context tables
 * are mapped in to a particular range of virtual addresses in the kas.
 */
hat_update_ctables (entry, new)
	u_int entry;
	u_int		    new;
{
	int	sx = splvm();
	u_int		    old;
#ifdef	MULTIPROCESSOR
	register int        ix;
#endif	MULTIPROCESSOR

	if (entry == 0) {
		/*
		 * kernel. change, then flush.
		 */
#ifndef	MULTIPROCESSOR
		contexts[entry].ptpe_int = new;
#else	MULTIPROCESSOR
		contexts[entry + nctxs * cpuid].ptpe_int = new;
#endif	MULTIPROCESSOR
		mmu_flushctx(entry);
		vac_ctxflush(entry);
		(void) splx (sx);
		return;
	}

	old = contexts[entry].ptpe_int;
	if (old == new) {
		/*
		 * no change, just return.
		 */
		(void) splx (sx);
		return;
	}

	if ((old & PTE_ETYPEMASK) != MMU_ET_PTP) {
		/*
		 * old not valid.
		 * change, then return.
		 */

#ifndef	MULTIPROCESSOR
		contexts[entry].ptpe_int = new;
#else	MULTIPROCESSOR
		for (ix=0; ix<nmod; ++ix) {
			contexts[entry].ptpe_int = new;
			entry += nctxs;
		}
#endif	MULTIPROCESSOR
	(void)splx(sx);
		return;
}

	/*
	 * old valid.
	 * flush, then change.
	 */
#ifndef	MULTIPROCESSOR
	mmu_flushctx(entry);
	vac_ctxflush(entry);
	contexts[entry].ptpe_int = new;
#else	MULTIPROCESSOR
	xc_attention();
	(void) xc_sync((int)entry,0,0,0,0,
		hat_vacate_context);
	mmu_flushctx(entry);
	vac_ctxflush(entry);
	for (ix = 0; ix < nmod; ++ix) {
		contexts[entry].ptpe_int = new;
		entry += nctxs;
	}
	xc_dismissed();
#endif	MULTIPROCESSOR
	(void) splx (sx);
	return;
}

int
nullpage_cacheable(addr, pte)
	addr_t	addr;
	struct pte *pte;
{
#ifndef	VAC
	return 0;
#else	VAC
	extern addr_t	kern_nc_top_va;
	extern addr_t	kern_nc_end_va;

	if (vac) 
	        return ((cache) &&
       	         (addr >= (addr_t)KERNELBASE) &&
	         (addr < eecontig) &&
       	         ((addr < kern_nc_top_va) ||
       	          (addr >= kern_nc_end_va)));
	else  /* physical cache such as Viking */
		return ((cache) &&
			(bustype((int) pte->PhysicalPageNumber) == BT_OBMEM) &&
			((addr < kern_nc_top_va) ||
			 (addr >= kern_nc_end_va)));
		
#endif	VAC
}


/*
 * Loads the pte for the given address with the given pte. Also sets it up
 * as a mapping for page pp, if there is one. The keep count of the page
 * table is incremented if the translation is to be locked.
 */
void
hat_pteload (seg, addr, pp, pte, flags)
	struct seg         *seg;
	addr_t              addr;
	struct page        *pp;
	struct pte         *pte;
	int                 flags;
{
	int                 sx = splvm ();
	register union ptpe *ptpe;
	register struct ptbl *ptbl;
	register struct spte *spte;
	register struct spte *aspte;
	struct page        *opp;
	unsigned            abits;

	/*
	 * Make sure there is a context set up for the address space.
	 */
	hat_setup (seg->s_as);

	/*
	 * If it's a kernel address, the page table should already exist.
	 * Otherwise we may have to create one.
	 */
	if (addr >= (addr_t) KERNELBASE)
		ptpe = (union ptpe *) hat_ptefind (seg->s_as, addr);
	else
		ptpe = (union ptpe *) hat_ptealloc (seg->s_as, addr);

	if (ptpe == (union ptpe *) 0) {
		printf ("hat_ptefind: seg %x addr %x pp %x pte %x (%x) flags %x\n",
			seg, addr, pp, pte,
			(pte != (struct pte *) 0 ?
			 *(u_int *) pte : 0), flags);
		panic ("hat_ptefind: null ptpe");
	}
	opp = ptetopp (&ptpe->pte);
	ptbl = ptetoptbl (&ptpe->pte);
	spte = ptetospte (&ptpe->pte);

	/*
	 * If we need to lock, increment the keep count.
	 * 
	 * XXX - THIS IS A BAND-AID THAT MUST BE FIXED.
	 * 
	 * Somewhere, someone is incrementing the lock count on a bunch of
	 * page tables, and not doing the corresponding decrement. The
	 * field is only sixteen bits long, so it wraps fairly frequently,
	 * and if someone should happen to try to establish a badly
	 * aliased mapping to one of the pages (say the sundiag "pmem"
	 * test) while the lockcount is a multiple of 64k, pages will get
	 * unmapped instead of decached.
	 * 
	 * Since we do not have time to find out why these tables are getting
	 * locked and not unlocked (hint: 0xFF640000 and 0xFF040000 are
	 * the base addresses), we are applying this band-aid solution.
	 * 
	 * Essentially, a sticky value is established for the lock; if it
	 * gets up to that value, we lock the ptbl down forever, or until
	 * someone explicitly unmaps all the mappings in which case we can
	 * assume that nobody needs the table.
	 */
	if (flags & PTELD_LOCK) {
#ifdef	PTBL_LOCK_MAX
		if (ptbl->ptbl_lockcnt == PTBL_LOCK_MAX) {
			ptbl->ptbl_lockcnt = PTBL_LOCK_PERM;
			if (ptbl_lock_perm_show)
				printf ("ptbl at %x frozen by lock\n",
					ptbl->ptbl_base);
		} else if (ptbl->ptbl_lockcnt != PTBL_LOCK_PERM)
#endif	PTBL_LOCK_MAX
			ptbl->ptbl_lockcnt ++;

		/*
		 * If we were only here to lock down an existing user
		 * mapping, then we are done and can return.
		 */
		if ((addr < (addr_t) KERNELBASE) && pp && (opp == pp)) {
			(void) splx (sx);
			return;
		}
	}
	if (pp && (pp != opp)) {

		/*
		 * We are loading a new translation. Add the translation
		 * to the list for this page, and increment the rss.
		 */
		spte->spte_next = sptetonum ((struct spte *) pp->p_mapping);
		pp->p_mapping = (caddr_t) spte;
		seg->s_as->a_rss += 1;
	}
	if (pp && (pp == opp)) {

		/*
		 * Reloading an existing translation. Do not wipe out the
		 * existing REF and MOD bits in the pte.
		 */
		abits = ~PTE_RM_MASK;
	} else
		abits = ~0;

	/*
	 * Increment the valid count if old one wasn't loaded.
	 */
	if (spte->spte_valid == 0)
		ptbl->ptbl_validcnt++;

#ifdef	IOC

	/*
	 * If the I/O cache flag is passed to us, go for it. This occurs
	 * for vme I/O that went through bp_map().
	 */
	if (flags & PTELD_IOCACHE)
		spte->spte_iocache = 1;
#endif	IOC

	spte->spte_valid = 1;

	/*
	 * Note whether this translation will be ghostunloaded or not.
	 */
	spte->spte_nosync = (flags & PTELD_NOSYNC) != 0;

	/*
	 * Do we want this mapping to be cacheable?
	 */
#ifdef	VAC
	if (cache) {
		if (pp == (struct page *) 0)
			pte->Cacheable = nullpage_cacheable (addr, pte);
		else if (pp->p_nc)
			pte->Cacheable = 0;
		else if (vac && (pp != opp) &&
			 (aspte = numtospte (spte->spte_next)) &&
			 (!VAC_ALIGNED (addr, sptetovaddr (aspte)))) {
			register struct ptbl *aptbl;
			register struct pte *apte;
			register struct spte *next;
			int                 usp = 0, upc = 0, ula;
			int                 decache = 0;

			/*
			 * CACHE ALIGNMENT CONFLICT! Must either unload
			 * all other mappings, or convert page to
			 * noncacheable.
			 */
			if (u.u_ar0) {
				usp = u.u_ar0[SP] & MMU_PAGEMASK;
				upc = u.u_ar0[PC] & MMU_PAGEMASK;
			}
			do {
				apte = sptetopte (aspte);
				aptbl = ptetoptbl (apte);
				next = numtospte (aspte->spte_next);
				ula = (int) ptetovaddr (apte);
				if ((aptbl->ptbl_lockcnt) ||
				    (ula == upc) || (ula == usp))
					decache = 1;
				else
					hat_pteunload (aptbl, apte, HAT_RMSYNC);
			} while (aspte = next);

			if (decache) {
				/*
				 * must decache the page.
				 */
				xc_attention ();
				pp->p_nc = 1;
				pte->Cacheable = 0;
				aspte = numtospte (spte->spte_next);
				do {
					next = numtospte (aspte->spte_next);
					hat_ptesync (pp, sptetopte (aspte),
						     HAT_NCSYNC);
					aptbl->ptbl_ncflag = 1;
				} while (aspte = next);
				xc_dismissed ();
			} else
				pte->Cacheable = 1;
		} else
			pte->Cacheable = 1;
	} else
#endif	/* VAC */
		pte->Cacheable = 0;

	/*
	 * Load the pte.
	 */
	(void) pte_offon (ptpe, abits, *(unsigned *) pte);

#ifdef	HAT_IOMAP
	IOMAP_SYNC (addr, ptr);
#endif

	(void) splx (sx);
}

/*
 * Construct a pte for a page.
 */
void
hat_mempte(pp, vprot, pte, addr)
	register struct page *pp;
	u_int	vprot;
	register struct pte *pte;
	addr_t	addr;
{
	*(int *)pte = PTEOF(0, page_pptonum(pp), hat_vtop_prot(addr, vprot), 1);
}

/*
 * Returns a pointer to the pte struct for the given virtual address.
 * If the necessary page tables do not exist, return NULL.
 */
union ptpe *
hat_ptefind(as, addr)
	struct	as *as;
	register addr_t addr;
{
	register union ptpe *lptp;
	register union ptpe *sptp;
	union ptpe *rv;
	union ptpe a_ptpe;
	int s;
	int	sx = splvm();

	/*
	 * Map in the level 1 page table (if necessary) and get the entry.
	 * Kernel addresses can be pulled from any level 1 table.
	 */
	s = splvm();
#ifdef HAT_DEBUG
	if(find_debug)
		printf("hat_ptefind as %x addr %x\n", as, addr);
#endif
	lptp = &a_ptpe;
	
	lptp->ptpe_int =
	       
		ldphys(va2pa((addr_t)as->a_hat.hat_l1pt) +
		       getl1in(addr) * sizeof (struct ptp));

#ifdef HAT_DEBUG
	if(find_debug > 2) {
		printf("hat_ptefind lptp %x\n",  lptp);
		printf(" *lptp %x\n",  *((u_int *)lptp));
	}
#endif
	if (lptp->ptp.EntryType == MMU_ET_INVALID) {
		/*
		 * If the entry is invalid, give up.
		 */
		(void) splx(s);
		(void)splx(sx);
		return (NULL);
	}
#ifdef HAT_DEBUG
	if(find_debug > 2) {
		printf("hat_ptefind lptp->ptp.PageTablePointer %x\n",  
			lptp->ptp.PageTablePointer);
		printf("hat_ptefind sptp %x\n",  
			ptptopte(lptp->ptp.PageTablePointer));
	}
#endif
	/*
	 * Get the level 2 entry. Once we have it, we no longer need the
	 * level 1 table so we can spl() down.
	 */
	sptp = &(((struct l2pt *)ptptopte(lptp->ptp.PageTablePointer))->ptpe[getl2in(addr)]);

	(void) splx(s);
	if (sptp->ptp.EntryType == MMU_ET_INVALID) {
		/*
		 * If the entry is invalid, give up.
		 */
		(void)splx(sx);
		return (NULL);
	}
	/*
	 * Return the address of the level 3 entry.
	 */
	rv = ((union ptpe *)&(((struct l3pt *)
			       ptptopte(sptp->ptp.PageTablePointer))->
			      pte[getl3in(addr)]));
#ifdef	VAC
	/* page tables can be cacheable if viking with E$ turned on
	 * so, don't panic if pte is cacheable in that case
	 */
	if (cache != CACHE_PAC_E) {	
		s = mmu_probe((addr_t) rv);
		if ((s & (0x83)) == (0x82)) {
			printf("hat_ptefind: pte at %x mapped with pte %x\n",
		       		rv, s);
			panic("found cacheable pte");
		}
	}
#endif	VAC

#ifdef HAT_DEBUG
	if(find_debug)
		printf("hat_ptefind: returning %x (%x)\n", rv, rv);
#endif
	(void)splx(sx);
	return rv;
}

/*
 * Allocate a ctx for use by the specified address space:
 * When there is at least one free context, this algorithm will
 * always allocate a free context.  When there are no free contexts
 * this algorithm will approximately allocate one of the oldest
 * contexts, by stealing this context from a process that has had
 * it for a long time.  The algorithm does not need to keep track
 * of how old a context is.  This is an improvement over the 
 * previous implementation which kept track of some rough time
 * figure, because keeping track of the time wasted extra storage
 * in each ctx structure (there are alot of these - one for each
 * context).  Since there are so many contexts (ie usually there
 * will be a free one anyway), and since this algorithm will still
 * tend to allocate old contexts when there are no free ones
 * left, this memory saving algorithm is superior.
 * Another algorithm considered was one where the ctx structures
 * are kept on a free list.  Freed ctx structures would be put on the
 * head, and the next ctx structure would be obtained by searching
 * for a free one starting at the head, and putting it on the tail,
 * once it is allocated.  This approach will also tend to allocate
 * old contexts when there are no more free ones, but it requires
 * forward and backward pointers, and thus it would consume more 
 * memory.
 *
 * Since someone failed to get the hand scanning algorithm
 * completely right, maybe some comments are in order.
 *
 * #define PEDANTIC_MODE
 *
 * The array of contexts is treated as a large ring, so when
 * stepping around the ring it is necessary to wrap back from
 * the last element to the first element. This has to be done
 * EVERYWHERE that math is performed on pointers into the ring;
 * someone neglected to do this where ctxhand is set from ctx,
 * resulting in our running off the end of the array.
 *
 * Additionally, collective wisdom of the community seems to
 * imply that is is a Bad Thing to even allow pointers to go
 * outside their assigned ranges at all, so it is preferable
 * to check the original value against the last element,
 * instead of checking the new value against a presumed illegal
 * value.
 *
 * Additionally, the ring is variable sized, so we really need
 * to use an inequality (ctx >= lastctx) to get a little simple
 * sanity checking and robustness. This also means that if we
 * whack on the "nctxs" variable with adb, the NEXT operation
 * will send us back into the new context number range.
 *
 * Since the only real pointer operations on "struct ctx *" are
 * dereferencing and the "next" operation, I chose to encapsulate
 * the NEXT operation in the macro NEXT_CTX.
 *
 * #undef PEDANTIC_MODE
 */

void
hat_getctx (as)
	struct as          *as;
{
	register struct ctx *firstctx;
	register struct ctx *lastctx;
	register struct ctx *ctx;
	register struct as *oas;

#define	NEXT_CTX(ctx) (((ctx)>=lastctx) ? firstctx : ((ctx)+1))

	firstctx = &ctxs[NUM_LOCKED_CTXS];
	lastctx = &ctxs[nswctxs - 1];

	/*
	 * Starting with ctxhand, search for the next free context.
	 */
	ctx = ctxhand;
	while (oas = ctx->c_as) {
		ctx = NEXT_CTX(ctx);
		if (ctx != ctxhand)
			continue;

		/*
		 * Wrapped all the way around. Keep scanning
		 * until an inactive one is found.
		 */
		while (oas = ctx->c_as) {
			if (ctx->c_as->a_hat.hat_oncpu) {
				ctx = NEXT_CTX(ctx);
				continue;
			}

			/*
			 * Inactive context found.
			 * Steal it.
			 */
			oas->a_hat.hat_ctx = NULL;
			hat_update_ctables(ctx->c_num,
					   MMU_STD_INVALIDPTP);
			break;
		}
		break;
	}

	/*
	 * Update the starting point for the next search. Note that if we
	 * have just allocated a context which is one of the oldest
	 * contexts, then the new value of ctxhand is also likely to point
	 * to an old context. NOTE: gotta wrap ctxhand, too, or we walk on
	 * down memory past the right place.
	 */
	ctxhand = NEXT_CTX (ctx);

	ctx->c_as = as;
	as->a_hat.hat_ctx = ctx;
#undef NEXT_CTX
}

/*
 * This routine converts virtual page protections to physical ones.
 * The value returned is only ro/rw; the supervisor mode is simply
 * checked for sanity (since the actual bit is in the level 1 page table).
 */
u_int
hat_vtop_prot(addr, vprot)
addr_t	addr;
register u_int	vprot;
{
	if ((vprot & PROT_ALL) != vprot)
		panic("hat_vtop_prot -- bad prot");

	if(vprot & PROT_USER) {	/* user permission */
		if (addr >= (addr_t)KERNELBASE) {
			printf("addr %x vprot %x\n", addr, vprot);
			panic("user addr in kernel space");
		}
	} else {
		 if (addr < (addr_t)KERNELBASE) {
			printf("addr %x vprot %x\n", addr, vprot);
			panic("kernel addr in user space");
		 }
	}
	switch(vprot) {
	case 0:
    case PROT_USER:
		/* XXXX No way to tell the difference between 0 and MMU_STD_SRUR */
        /*
         * Since 0 might be a valid protection,
         * the caller should not set valid bit
         * if vprot == 0 to be sure.
         */
        return (0);
    case PROT_READ:
    case PROT_EXEC:
    case PROT_READ | PROT_EXEC:
        return (MMU_STD_SRX);
    case PROT_WRITE:
    case PROT_WRITE | PROT_EXEC:
    case PROT_READ | PROT_WRITE:
    case PROT_READ | PROT_WRITE | PROT_EXEC:
        return (MMU_STD_SRWX);
    case PROT_EXEC | PROT_USER:
		return (MMU_STD_SXUX);
    case PROT_READ | PROT_USER:
		return (MMU_STD_SRUR); /* was SRWUR */
    case PROT_READ | PROT_EXEC | PROT_USER:
        return (MMU_STD_SRXURX);
    case PROT_WRITE | PROT_USER:
    case PROT_READ | PROT_WRITE | PROT_USER:
		return (MMU_STD_SRWURW);
    case PROT_WRITE | PROT_EXEC | PROT_USER:
    case PROT_READ | PROT_WRITE | PROT_EXEC | PROT_USER:
        return (MMU_STD_SRWXURWX);
	default:
        panic("hat_vtop_prot");
        /* NOTREACHED */
	}
}

u_int
hat_ptov_prot(pte)
struct pte *pte;
{
	u_int pprot;

	switch (pte->AccessPermissions) {
	case MMU_STD_SRUR:
		pprot = PROT_READ | PROT_USER;
		break;
	case MMU_STD_SRWURW:
		pprot = PROT_READ | PROT_WRITE | PROT_USER;
		break;
	case MMU_STD_SRXURX:
		pprot = PROT_READ | PROT_EXEC | PROT_USER;
		break;
	case MMU_STD_SRWXURWX:
		pprot = PROT_READ | PROT_WRITE | PROT_EXEC | PROT_USER;
		break;
	case MMU_STD_SXUX:
		pprot = PROT_EXEC | PROT_USER;
		break;
	case MMU_STD_SRWUR:	/* Hmmm, doesn't map nicely, demote */
		pprot = PROT_READ | PROT_USER;
		break;
	case MMU_STD_SRX:
		pprot = PROT_READ | PROT_EXEC;
		break;
	case MMU_STD_SRWX:
		pprot = PROT_READ | PROT_WRITE | PROT_EXEC;
		break;
	default:
		pprot = 0;
		break;
	}
	return(pprot);
}

#ifdef	VAC
/*
 * This routine syncs the virtual address cache for all mappings to
 * a particular physical page.  This routine originated in the
 * campus software, where it was used to keep I/O consistent with
 * what is in the vac, since hardware does not guarantee such
 * consistancy.  Since such consistancy will probably be achieved
 * via hardware on Galaxy, this routine is normally provided by
 * the Sun-4M hardware, we don't really need this, but we include it here
 * anyway just in case we want to do some debugging or feel paranoid.
 * XXX - It can always be removed later.
 */
void
hat_vacsync(pfnum)
	u_int pfnum;
{
	register struct page *pp = page_numtopp(pfnum);
	register struct spte *spte;
	register struct ptbl *ptbl;
	int s;
	struct ctx *nctx;

	/*
	 * If the cache is off, the page isn't memory, or the page is
	 * non-cacheable, then none of the page could be in the cache
	 * in the first place.
	 */
	if (!vac || (pp == (struct page *)NULL) || pp->p_nc)
		return;

	/* XXX May be able to get a smaller critical code section */
	s = splvm();

	for (spte = (struct spte *)pp->p_mapping; spte != NULL;
		spte = numtospte(spte->spte_next)) {
		ptbl = sptetoptbl(spte);

		/*
		 * If the translation has no context, it can't be in
		 * the cache. Otherwise, calculate the virtual
		 * address, switch to the correct context, and flush
		 * the page (the vac support code handles the details).
		 */
		if ((nctx = ptbl->ptbl_as->a_hat.hat_ctx) != NULL)
			vac_pagectxflush(sptetovaddr(spte), nctx->c_num);
	}	

	(void) splx(s);
}
#endif VAC

/*
 * Kill any processes that use this page. (Used for parity recovery)
 * If we encounter the kernel's address space, give up (return -1).
 * Otherwise, we return 0.
 * Kill them all; let GOD sort them out.
 */

/*
 * XXX - This code came from campus software for dealing with
 * parity errors.  It will still be useful for Sun-4M for handling
 * uncorrectable ECC errors, once such code is written.
 */
hat_kill_procs(pp, pagenum)
	struct  page   *pp;
	u_int		pagenum;
{
	register struct spte *spte;
	register struct ptbl *ptbl;
	int s;
	struct  as      *as;
	struct  proc    *p;
	int     result = 0;

	/* XXX - may be able to get a smaller critical code section */
	s = splvm();

	for (spte = (struct spte *)pp->p_mapping; spte != NULL;
		spte = numtospte(spte->spte_next)) {
		ptbl = ptetoptbl(sptetopte(spte));
		as = ptbl->ptbl_as;

		/*
		 * If the address space is the kernel space, then fail.
		 * The memory is corrupted, and the only thing to do with
		 * corrupted kernel memory is die.
		 */
		if (as == &kas) {
			printf("Bad page has kernel address space mapped\n");
			result = -1;
		}

		/*
		 * Find the proc that uses this address space and kill
		 * it.  Note that more than one process can share the
		 * same address space, if vfork() was used to create it
		 * This means that we have to look through the entire
		 * process table and not stop at the first match.
		 */
		for (p = allproc; p; p = p->p_nxt) {
			if (p->p_as == as) {
				printf("pid %d killed: memory error\n",
					p->p_pid);
				uprintf("pid %d killed: memory error\n",
					p->p_pid);
				psignal(p, SIGBUS);
				/* XXX should do this for them all */
				if (p == u.u_procp) {
					u.u_code = FC_HWERR;
					/* XXX - users wont grok this;
					 * but, we really do not know
					 * what the user's virtual address
					 * really is. Bag it.
					 */
					u.u_addr = (addr_t)pagenum;
				}
			}
		}
	}

	(void) splx(s);
	return (result);
}

/*
 * End machine specific interface routines.
 *
 * The remainder of the routines are private to this module and are
 * used by the routines above to implement a service to the outside
 * caller.
 *
 * Start private routines.
 */

/*
 * spte_alignok: return true if there are no VA conflicts
 * in this chain of SPTEs. Useful to see if a page can be
 * recached when a mapping has been removed. Brought out
 * as a subroutine to make the mainline code easiser
 * to read and maintain.
 */
static int
spte_alignok(spte)
struct spte *spte;
{
	addr_t base;
	if (spte == (struct spte *)0)
		return 1;
	base = sptetovaddr(spte);
	while (spte = numtospte(spte->spte_next))
		if (!VAC_ALIGNED(base, sptetovaddr(spte)))
			return 0;
	return 1;
}

/*
 * Unload the pte. If required, sync the referenced & modified bits. If
 * it's the last pte in the page table, and the table isn't locked, free
 * it up.
 */
static void
hat_pteunload (ptbl, pte, mode)
	register struct ptbl *ptbl;
	register struct pte *pte;
	int                 mode;
{
	int                 sx = splvm ();
	addr_t              vaddr;
	struct page        *pp;
	register union ptpe *mid_ptp;
	register struct spte *spte, *aspte, *next;
	union ptpe         *top_ptp;
	union ptpe          a_ptpe;
	unsigned            a_paddr;
	struct as          *as;

	as = ptbl->ptbl_as;

	spte = ptetospte (pte);		/* Get the software pte */

	if (spte->spte_valid == 0) {	/* If it is not loaded, return */
		(void) splx (sx);
		return;
	}
	spte->spte_valid = 0;

	vaddr = ptetovaddr (pte);

	if ((pp = ptetopp (pte)) &&
	    (next = (struct spte *)pp->p_mapping)) {
		/*
		 * Remove the pte from the list of mappings for the page.
		 */
		if (spte == next) {
			pp->p_mapping = (caddr_t) numtospte (spte->spte_next);
			as->a_rss -= 1;
			spte->spte_next = SPTE_NULL;
		} else {
			do {
				aspte = next;
				next = numtospte (aspte->spte_next);
				if (spte == next) {
					aspte->spte_next = spte->spte_next;
					as->a_rss -= 1;
					spte->spte_next = SPTE_NULL;
					break;
				}
			} while (next);
		}
	}

	if (pte->EntryType != MMU_ET_INVALID)
		hat_ptesync (pp, pte, HAT_INVSYNC | (mode & HAT_RMSYNC));

	spte->spte_nosync = 0;

	/*
	 * Synchronize the I/O Mapper.
	 */
#ifdef	HAT_IOMAP
	IOMAP_SYNC (vaddr, pte);
#endif

#ifdef	VAC
	/*
	 * See if we can recache the page.
	 */
	if (vac && pp && pp->p_nc &&
	    spte_alignok((struct spte *)pp->p_mapping)) {
		/*
		 * No misaligned mappings;
		 * Recache the page.
		 */
		pp->p_nc = 0;
		if (aspte = (struct spte *)pp->p_mapping) {
			xc_attention ();
			do {
				hat_ptesync (pp, sptetopte (aspte),
					     HAT_NCSYNC);
			} while (aspte = numtospte (aspte->spte_next));
			xc_dismissed ();
		}
	}
#endif

	/*
	 * Decrement the valid count;
	 * If the page table is no longer being used, free it up.
	 */
	ptbl->ptbl_validcnt--;

	if ((ptbl->ptbl_validcnt == 0) &&
	    (ptbl->ptbl_lockcnt == 0) &&
	    (ptbl->ptbl_keep == 0)) {

		/*
		 * Locate and read the entry in the Level-1 page table.
		 */
		top_ptp = &a_ptpe;
		top_ptp->ptpe_int =
			ldphys (a_paddr =
				va2pa ((addr_t) as->a_hat.hat_l1pt) +
				getl1in (ptbl->ptbl_base) *
				sizeof (struct ptp));

		/*
		 * Locate the entry in the Level-2 page table.
		 */
		mid_ptp = &(((struct l2pt *)
			     ptptopte (top_ptp->ptp.PageTablePointer))->
			    ptpe[getl2in (ptbl->ptbl_base)]);

		/*
		 * Unlink the level 3 page table and
		 * add it to the free list.
		 */
		hat_ptblunlink (ptbl, 3, (u_int) vaddr);
		ptbl->ptbl_next = ptbl_freelist;
		ptbl_freelist = ptbltonum (ptbl);

		/*
		 * Invalidate the entry in the Level-2 table.
		 */
		mid_ptp->ptp.EntryType = MMU_ET_INVALID;

		/*
		 * Decrement the valid count of the level 2 page table.
		 */
		ptbl = ptetoptbl (mid_ptp);
		ptbl->ptbl_validcnt--;

		/*
		 * If the level 2 table is also empty now, free it too.
		 */
		if ((ptbl->ptbl_validcnt == 0) &&
		    (ptbl->ptbl_keep == 0)) {

			/*
			 * Level 2 tables are always kept, so unkeep it.
			 */
			ptbl->ptbl_lockcnt = 0;

			/*
			 * Unlink the table and add it to the free list.
			 */
			hat_ptblunlink (ptbl, 2, (u_int) vaddr);
			ptbl->ptbl_next = ptbl_freelist;
			ptbl_freelist = ptbltonum (ptbl);

			/*
			 * Invalidate the entry in the Level-1 table.
			 */
			stphys (a_paddr, MMU_STD_INVALIDPTP);
		}
	}
	(void) splx (sx);
}

/*
 * Synchronization for Ref, Mod, and Cacheable bits between the
 * PTE and the page struct.
 *
 * HAT_INVSYNC: invalidate during update.
 * HAT_NCSYNC:	adjust the PTE's cacheable bit to match the page.
 * HAT_RMSYNC:	update page's Ref and Mod info from PTE, and
 *		clear the PTE's Ref and Mod bits.
 *
 * Does the right thing if there is no page struct, or if more than
 * one flag bit is set (all ops are atomic WRT. the TLB and VAC).
 */
static void
hat_ptesync (pp, pte, flags)
	register struct page *pp;
	register struct pte *pte;
	int                 flags;
{
	int                 sx = splvm ();
	u_int               abits = 0;
	u_int               obits = 0;
	union ptpe          rpte;
	addr_t              vaddr;

	vaddr = ptetovaddr (pte);

	if (flags & HAT_INVSYNC)
		abits = ~0;

	if (pp == (struct page *)0) {
		rpte.ptpe_int = pte_offon ((union ptpe *) pte, abits, obits);
		(void)splx(sx);
		return;
	}

	if (flags & HAT_RMSYNC)
		abits |= PTE_RM_MASK;

#ifdef	VAC
	if (flags & HAT_NCSYNC) {
		if (vaddr >= DVMA)
			;	/* leave DVMA mappings alone */
		else if ((segu != NULL) &&
			 (vaddr >= segu->s_base) &&
			 (vaddr < (segu->s_base + segu->s_size)))
			;	/* leave SEGU mappings alone */
		else if (!vac)
			abits |= PTE_CE_MASK;
		else if (pp->p_nc)
			abits |= PTE_CE_MASK;
		else
			obits |= PTE_CE_MASK;
	}
#endif	VAC

	rpte.ptpe_int = pte_offon ((union ptpe *) pte, abits, obits);

	if (flags & HAT_RMSYNC) {
		/*
		 * Call back to inform address space, if requested.
		 * The claim has been made that this is never used.
		 */
		struct ptbl        *ptbl = ptetoptbl (pte);
		struct as          *as = ptbl->ptbl_as;

		if (as->a_hatcallback)
			as_hatsync (as, vaddr,
				    rpte.pte.Referenced,
				    rpte.pte.Modified,
				    (u_int)((flags & HAT_INVSYNC) ?
					    AHAT_UNLOAD : 0));
		if (rpte.pte.Referenced)
			pp->p_ref = 1;
		if (rpte.pte.Modified)
			pp->p_mod = 1;
	}
	(void) splx (sx);
}

/*
 * Return a pointer to the pte struct for the given virtual address.
 * If necessary, page tables are allocated to create the pte.
 */
static struct pte *
hat_ptealloc(as, addr)
	struct	as *as;
	addr_t	addr;
{
	register union ptpe *top_ptp;
	register union ptpe *mid_ptp;
	register struct pte *low_ptp;
	register struct ptbl *ptbl = (struct ptbl *)0;
	union ptpe a_ptpe;
	int s;
	int	sx = splvm();

	/*
	 * Map in the level 1 page table (if necessary) and get the entry.
	 * Kernel addresses can be pulled from any level 1 table.
	 */
#ifdef HAT_DEBUG
	if(alloc_debug)
		printf("hat_ptealloc: as %x addr %x\n", as, addr);
#endif
	s = splvm();
	top_ptp = &a_ptpe;
	top_ptp->ptpe_int =
		ldphys(va2pa((addr_t)as->a_hat.hat_l1pt) +
		       getl1in(addr) * sizeof (struct ptp));

#ifdef HAT_DEBUG
	if(alloc_debug > 2) {
		printf("hat_ptealloc top_ptp %x\n",  top_ptp);
		printf(" *top_ptp %x\n",  *((u_int *)top_ptp));
	}
#endif
	if (top_ptp->ptp.EntryType == MMU_ET_INVALID) {
		/*
		 * Entry is invalid, so there's no level 2 page table.
		 * We must spl() down here and remap the level 1 table
		 * later, since the intervening routines may have to
		 * map in a different level 1 page table.
		 */
		(void) splx(s);
		/*
		 * Allocate a level 2 page table, lock it and link it into
		 * the address space.
		 */
		ptbl = hat_ptblalloc();
		hat_ptbllink(ptbl, as, addr, 2);
		ptbl->ptbl_lockcnt = 1;
		/*
		 * Now we remap the level 1 table and validate the entry.
		 * Note that we don't need to recalculate the entry pointer
		 * since we map it into the same address every time.
		 */
		s = splvm();
		top_ptp->ptp.PageTablePointer = ptetota(ptbltopte(ptbl));
		top_ptp->ptp.EntryType = MMU_ET_PTP;
		stphys(va2pa((addr_t)as->a_hat.hat_l1pt) +
		       getl1in(addr) * sizeof (struct ptp),
		       top_ptp->ptpe_int);
		/*
		 * Get the level 2 entry.
		 */
		mid_ptp = &((struct l2pt *)ptbltopte(ptbl))->ptpe[getl2in(addr)];
#ifdef	VAC
		/* page tables can be cacheable if viking with E$ turned on
	 	 * so, don't panic if pte is cacheable in that case
	 	 */
		if (cache != CACHE_PAC_E)
			if (mmu_probe((addr_t) mid_ptp) & 0x80)
				panic("allocated cacheable level two page table");
#endif	VAC
#ifdef HAT_DEBUG
		if(alloc_debug > 2) {
			printf("hat_ptealloc top_ptp->ptp.ptr %x ptbl %x alloc\n",
			       top_ptp->ptp.PageTablePointer, ptbl);
			printf("hat_ptealloc mid_ptp %x *(mid_ptp) %x\n",  
			       mid_ptp, *(u_int *)mid_ptp);
		}
#endif
	} else {
		/*
		 * Level 2 page table is valid. Simply get the appropriate
		 * entry.
		 */
		mid_ptp = &((struct l2pt *)tatopte(top_ptp->ptp.PageTablePointer))->
		    ptpe[getl2in(addr)];
#ifdef	VAC
		/* page tables can be cacheable if viking with E$ turned on
		 * so, don't panic if pte is cacheable in that case
		 */
		if (cache != CACHE_PAC_E)
			if (mmu_probe((addr_t) mid_ptp) & 0x80)
				panic("found cacheable level two page table");
#endif	VAC
#ifdef HAT_DEBUG
		if(alloc_debug > 2) {
			printf("hat_ptealloc top_ptp->ptp.ptr %x ptbl %x no alloc\n",  
			       top_ptp->ptp.PageTablePointer, ptbl);
			printf("hat_ptealloc mid_ptp %x *(mid_ptp) %x\n",  
			       mid_ptp, *(u_int *)mid_ptp);
		}
#endif
	}

	/*
	 * At this point, we have the level 2 entry. We no longer need
	 * the level 1 table. However, we don't want to spl down yet, or
	 * someone might try to free our level2 page table.
	 */
	if (mid_ptp->ptp.EntryType == MMU_ET_INVALID) {
		/*
		 * Level 2 entry is invalid, meaning there is no level
		 * 3 page table. Allocate one and link it into the
		 * address space. NOTE: we increment the valid count on
		 * the level 2 table first so it doesn't get freed out
		 * from under us by hat_ptblalloc().
		 */
		(ptetoptbl((struct pte *)mid_ptp))->ptbl_validcnt++;
		ptbl = hat_ptblalloc();
		hat_ptbllink(ptbl, as, addr, 3);
		/* ptbl->ptbl_lockcnt = 1; /* XXX Should this be done?? */
		/*
		 * Validate the level 2 entry (valid count already incremented).
		 */
		mid_ptp->ptp.PageTablePointer = ptetota(ptbltopte(ptbl));
		mid_ptp->ptp.EntryType = MMU_ET_PTP;
		low_ptp = &(((struct l3pt *)ptbltopte(ptbl))->pte[getl3in(addr)]);
#ifdef	VAC
		/* page tables can be cacheable if viking with E$ turned on
		 * so, don't panic if pte is cacheable in that case
		 */
		if (cache != CACHE_PAC_E)
			if (mmu_probe((addr_t) low_ptp) & 0x80)
				panic("allocated cacheable level three page table");
#endif	VAC
#ifdef HAT_DEBUG
		if(alloc_debug > 2) {
			printf(
			"hat_ptealloc mid_ptp->ptp.PageTablePointer %x ptbl %x alloc\n",
				mid_ptp->ptp.PageTablePointer, ptbl);
			printf("hat_ptealloc low_ptp %x *(low_ptp) %x\n",  
				low_ptp, *(u_int *)low_ptp);
		}
#endif
	} else {
		/*
		 * Level 2 table was valid, simply return the level 3 entry.
		 */
		low_ptp = &(((struct l3pt *)tatopte(mid_ptp->ptp.PageTablePointer))->
		    pte[getl3in(addr)]);
#ifdef	VAC
		/* page tables can be cacheable if viking with E$ turned on
		 * so, don't panic if pte is cacheable in that case
		 */
		if (cache != CACHE_PAC_E)
			if (mmu_probe((addr_t) low_ptp) & 0x80)
				panic("found cacheable level three page table");
#endif	VAC
#ifdef HAT_DEBUG
		if(alloc_debug > 2) {
			printf(
			"hat_ptealloc mid_ptp->ptp.PageTablePointer %x ptbl %x no alloc\n", 
				mid_ptp->ptp.PageTablePointer, ptbl);
			printf("hat_ptealloc low_ptp %x *(low_ptp) %x\n",  
				low_ptp, *(u_int *)low_ptp);
		}
#endif
	}
	(void) splx(s);
	(void)splx(sx);
	return (low_ptp);
}

/*
 * This routine allocates a ptbl structure and returns a pointer to it.
 * The algorithm is as follows:
 * 	If there is a ptbl on the free list, take it.
 *	Else, loop through the ptbl array, starting at ptblhand, looking
 *	for an unlocked one. Take the first unlocked one you find, and
 *	free it up. If there are no unlocked ones, call a routine to
 *	create more. Currently this routine dies.
 */
static struct ptbl *
hat_ptblalloc()
{
	register struct ptbl *ptbl;
	int	sx = splvm();

	/*
	 * If the free list isn't empty, we are done searching.
	 */
	kmon_enter(&ptbl_lock);
	if (ptbl_freelist != PTBL_NULL)
		goto found;
	/*
	 * Begin the search at the global hand.
	 */
	ptbl = ptblhand;
	for (;;) {
		/*
		 * Loop through all the page tables once.
		 */
		do {
			ptbl++;
			/*
			 * If we cross into the tables reserved for the kernel,
			 * go back to the beginning of the array.
			 * All the kernel tables are locked so there's
			 * no point in searching them.
			 */
			if (ptbl > ptblmax)
				ptbl = ptbls;
			/*
			 * If we find a page table that isn't locked, empty it.
			 * This causes it to be added to the free list.
			 */
			if ((ptbl->ptbl_lockcnt == 0) &&
				(ptbl->ptbl_keep == 0)) {
				ptblhand = ptbl;
				hat_ptblempty(ptbl);
				goto found;
			}
		} while (ptbl != ptblhand);
		/*
		 * If all the tables are locked, try to create more and
		 * rerun the search. This is currently not implemented.
		 */
		rm_outofhat();
	}
found:
	/*
	 * Remove the first table on the free list and use it.
	 */
	ptbl = numtoptbl(ptbl_freelist);
	ptbl_freelist = ptbl->ptbl_next;
	bzero((char *)ptbltopte(ptbl), sizeof (struct l3pt));
	kmon_exit(&ptbl_lock);
	ptbl->ptbl_keep = 0;
	ptbl->ptbl_validcnt = 0;
	(void)splx(sx);
	return (ptbl);
}

/*
 * Empty and free the specified page table. All the valid ptes are unloaded,
 * which in turn causes the page table to be freed.
 */
static void
hat_ptblempty(ptbl)
	register struct ptbl *ptbl;
{
	register struct pte *pte;
	register struct spte *spte;
	register int i;
	int	sx = splvm();

	/*
	 * Loop through all the ptes in the table.
	 */
	for (i = 0, pte = ptbltopte(ptbl), spte = ptbltospte(ptbl); 
		i < NL3PTEPERPT; i++, spte++, pte++) {
		/*
		 * If the valid count is 0, the table has already been
		 * freed. We are done.
		 * If the keep count is nonzero, go no further,
		 * lest we drop the ptbl on the floor.
		 */
		if ((ptbl->ptbl_validcnt == 0) ||
			(ptbl->ptbl_keep != 0))
			break;
		/*
		 * If the pte is valid, unload it. We know it's ok to
		 * sync the translation because all ghostunload
		 * translations are locked, so their page tables cannot
		 * be freed.
		 */
		if (spte->spte_valid)
			hat_pteunload(ptbl, pte, HAT_RMSYNC);
	}
	(void)splx(sx);
}

/*
 * Link the ptbl into the list of ptbl's allocated to this address space.
 * Separate lists are kept for level 2 and level 3 tables.
 */
static void
hat_ptbllink(ptbl, as, addr, lvl)
	register struct ptbl *ptbl;
	struct	as *as;
	addr_t	addr;
	int	lvl;
{
	register u_int *list;
	register u_int num;
	int	sx = splvm();

	num = ptbltonum(ptbl);
	ptbl->ptbl_as = as;
	if (lvl == 2)
		list = &as->a_hat.hat_l2pts;
	else
		list = &as->a_hat.hat_l3pts;
	ptbl->ptbl_next = *list;
	if (ptbl->ptbl_next != PTBL_NULL)
		(numtoptbl(ptbl->ptbl_next))->ptbl_prev = num;
	ptbl->ptbl_prev = PTBL_NULL;
	*list = num;
	if (lvl == 3)
		ptbl->ptbl_base = (addr_t)((u_int)addr & ~(L3PTSIZE - 1));
	else /* (lvl == 2) */
		ptbl->ptbl_base = (addr_t)((u_int)addr & ~(L2PTSIZE - 1));

	(void)splx(sx);
}

/*
 * Unlink the ptbl from the list of ptbl's allocated to this address space.
 * Separate lists are kept for level 2 and level 3 tables.
 */
static void
hat_ptblunlink(ptbl, lvl, vaddr)
	register struct ptbl *ptbl;
	int	lvl;
	u_int vaddr;
{
	register u_int *list;
	u_int addr, raddr;
	int	sx = splvm();

	addr = (u_int) ptbl->ptbl_base;
	raddr = ((vaddr) & ((lvl == 3) ? (~(L3PTSIZE - 1)) : (~(L2PTSIZE - 1))));
	if(addr != raddr) {
		printf("hat_ptblunlink: addr %x raddr %x (vaddr %x) mismatch\n",
			addr, raddr, vaddr);
		addr = raddr;
	}
	if (lvl == 2)
		list = &ptbl->ptbl_as->a_hat.hat_l2pts;
	else
		list = &ptbl->ptbl_as->a_hat.hat_l3pts;
	if (*list == ptbltonum(ptbl)) {
		*list = ptbl->ptbl_next;
		if (*list != PTBL_NULL)
			(numtoptbl(*list))->ptbl_prev = PTBL_NULL;
	} else {
		(numtoptbl(ptbl->ptbl_prev))->ptbl_next = ptbl->ptbl_next;
		if (ptbl->ptbl_next != PTBL_NULL)
			(numtoptbl(ptbl->ptbl_next))->ptbl_prev =
			    ptbl->ptbl_prev;
	}

	ptbl->ptbl_as = NULL;
	ptbl->ptbl_base = (addr_t)0xDEADCE11;
	ptbl->ptbl_next = ptbl->ptbl_prev = PTBL_NULL;
	(void)splx(sx);
}

/*
 * va2pp: translate a virtual address to a physical page.
 * Returns -1 if the address is not mapped.
 */
int
va2pp(va)
	addr_t	va;
{
	union ptpe p;
	extern unsigned mmu_probe();
	if ((va >= (addr_t)KERNELBASE) && (va < econtig))
		return mmu_btop(va - (addr_t)KERNELBASE);
	p.ptpe_int = mmu_probe(va);
	return pte_valid(&p.pte) ? p.pte.PhysicalPageNumber : -1;
}

/*
 * va2pa: translate a virtual address to a physical address.
 * assumes that it is mapped and that the destination is in
 * the low four gigabytes of the physical address range.
 * For Sun-4m this corresponds to things mapped to
 * real memory.
 * If the assumptions are violated, it panics.
 */
unsigned
va2pa(va)
	addr_t	va;
{
	int pp;
	if ((va >= (addr_t)KERNELBASE) && (va < econtig))
		return va - (addr_t)KERNELBASE;
	pp = va2pp(va);
	if (pp == -1)
		panic("va2pa: address not mapped");
	if (pp >> (32 - MMU_PAGESHIFT))
		panic("va2pa: address not mapped to space zero");
	return ((unsigned)va & MMU_PAGEOFFSET) | (pp << MMU_PAGESHIFT);
}

/*
 * pte_read: fetch the PTE at the specified
 * location (returned by hat_ptefind) and
 * return it as an unsigned int. Returns zero
 * for a null pointer.
 */
unsigned
pte_read(ptpe)
	union ptpe     *ptpe;
{
	return ptpe ? ptpe->ptpe_int : 0;
}

/*
 * pte_write: store a specified value
 * in a specified PTE, keeping all
 * proper caches up to snuff.
 */

unsigned
pte_write(ptpe, wpte)
	union ptpe     *ptpe;
	unsigned        wpte;
{
	return pte_offon(ptpe, ~0, wpte);
}

/*
 * pte_decache: turn off the cacheable bit in a specified pte,
 * keeping the VAC and TLB consistant.
 */
unsigned
pte_decache(ptpe)
	union ptpe     *ptpe;
{
	return pte_offon(ptpe, PTE_CE_MASK, 0x0);
}

/*
 * pte_recache: turn on the cacheable bit in a specified pte,
 * keeping the VAC and TLB consistant.
 */
unsigned
pte_recache(ptpe)
	union ptpe     *ptpe;
{
	return pte_offon(ptpe, 0, PTE_CE_MASK);
}

/*
 * pte_invalidate: invalidate a specified pte,
 * keeping the VAC and TLB consistant.
 */
unsigned
pte_invalidate(ptpe)
	union ptpe     *ptpe;
{
	return pte_write(ptpe, MMU_STD_INVALIDPTE);
}

/*
 * pte_setprot: change the protection modes for a specified pte,
 * keeping the VAC and TLB consistant.
 */
unsigned
pte_setprot(ptpe, pprot)
	union ptpe     *ptpe;
	unsigned        pprot;
{
	return pte_offon(ptpe, PTE_PERMMASK, PTE_PERMS(pprot));
}

/*
 * pte_rmsync: clear the ref and mod bits in the specified
 * PTE, returning their old values.
 */
unsigned
pte_rmsync(ptpe)
	union ptpe     *ptpe;
{
	return pte_offon(ptpe, PTE_RM_MASK, 0);
}

unsigned
proc_as_mid(pp)
	struct proc *pp;
{
	register struct as *as;
	if (!pp) return 0;
	as = pp->p_as;
	if (!as) return 0;
	return as->a_hat.hat_oncpu;
}

/*
 * clear a modified bit in a pte
 * used for viking HW bug workaround
 */
clr_modified_bit(vaddr)
	addr_t vaddr;
{
	int	sx = splvm();
	(void)pte_offon(hat_ptefind(u.u_procp->p_as, vaddr),
			PTE_MOD_MASK, 0);
	(void)splx(sx);
}
