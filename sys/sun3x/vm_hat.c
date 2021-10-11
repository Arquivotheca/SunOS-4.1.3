#ifndef lint
static	char sccsid[] = "@(#)vm_hat.c 1.1 92/07/30 SMI";
#endif
/* syncd to sun/vm_hat.c 1.88 */

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
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

extern struct seg *segkmap;
extern int softenablereg;

/*
 * Private vm_hat data structures.
 */
static struct ptbl *ptblhand;		/* hand for page table alloc */
static struct ptbl *ptblmax;		/* last page table open for alloc */
static u_short ptbl_freelist;		 /* page table free list */

static void hat_pteunload(/* ptbl, pte, mode */);
static void hat_ptesync(/* pp, pte, flags */);
static struct pte *hat_ptealloc(/* as, addr */);
static void hat_mapl1pt(/* hat */);
static struct ptbl *hat_ptblalloc(/* */);
static void hat_ptblempty(/* ptbl */);
static void hat_ptbllink(/* ptbl, as, addr, lvl */);
static void hat_ptblunlink(/* ptbl, lvl */);

/*
 * Semi-private data
 */
struct ptbl *ptbls, *eptbls;		/* the page table array */
struct pte *ptes, *eptes;		/* the pte array */
struct pte **pte_nexts;			/* the next ptr array */
u_int pts_addr;				/* physical address of page tables */

/*
 * Global data
 */
struct as kas;				/* kernel's address space */

#ifdef	SUN3X_470
int nullmap;		/* count of active null page ptrs (to hat_pteload) */
char *iocs;
#endif	SUN3X_470

/*
 * Initialize the hardware address translation structures.
 * Called by startup() after the vm structures have been allocated
 * and mapped in.
 */
void
hat_init()
{
	register struct ptbl *ptbl;

	/*
	 * All the arrays were zero filled in startup(), so there is no
	 * need to initialize them here (luckily 0 is an invalid pte).
	 */
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
	 * We must initialize kas here so that hat_ptblreserve() can add
	 * the allocated tables to the list for kas. This list will
	 * later get destroyed when we call hat_setup(&kas), but we
	 * have to let the code build it anyway. Losing the list doesn't
	 * hurt us since all the tables are locked forever anyway.
	 */
	kas.a_hat.hat_l2pts = kas.a_hat.hat_l3pts = PTBL_NULL;
}

/*
 * Free all the translation resources for the specified address space.
 * Called from as_free when an address space is being destroyed.
 */
void
hat_free(as)
	register struct as *as;
{
	register struct ptbl *ptbl;
	int s;

	if (as->a_hat.hat_valid == NULL)
		goto out;
	/*
	 * Release all the page tables owned by the address space.
	 */
	while (ptbl = numtoptbl(as->a_hat.hat_l3pts))
		hat_ptblempty(ptbl);
	/*
	 * Invalidate the hat structure.
	 */
	as->a_hat.hat_valid = NULL;
out:
	/*
	 * Switch to the kernel address space page tables. We must switch
	 * both the root pointer (for hardware) and the mapped level 1
	 * table (for software).
	 */
	s = splvm();
	hat_mapl1pt(&kas.a_hat);
	mmu_setrootptr(&(((struct user *)
	    (mmu_ptob(kas.a_hat.hat_l1pfn)))->u_pcb.pcb_l1pt));
	(void) splx(s);
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
	struct	pte pte;

	hat_mempte(pp, prot, &pte, addr);
	hat_pteload(seg, addr, pp, &pte, lock ? PTELD_LOCK : 0);
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
	struct	pte pte;

	*(int *)&pte = MMU_INVALIDPTE;
	pte.pte_pfn = pf;
	pte.pte_readonly = hat_vtop_prot(addr, prot);
	pte.pte_vld = PTE_VALID;

	if (bustype(pf) == BT_OBMEM)
		pp = page_numtouserpp((u_int)pf);
	else
		pp = NULL;
	hat_pteload(seg, addr, pp, &pte, lock ? PTELD_LOCK : 0);
	u.u_ru.ru_minflt++;
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
	register struct pte *pte;
	register struct ptbl *ptbl;

	pte = hat_ptefind(seg->s_as, addr);
	
	/*
	 * Need to check to see if the pte is valid!
	 * Otherwise, we might get a bad ptbl generated
	 * below.
	 */
	if (!pte_valid(pte)) {
		return;
	}
	ptbl = ptetoptbl(pte);
	ptbl->ptbl_keepcnt--;
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
	register struct pte *pte = NULL;
	register u_int pprot;
	int	flushed = 0;

	/*
	 * Make sure there is a context set up for the address space.
	 */
	hat_setup(seg->s_as);
	/*
	 * Convert the virtual protections to physical ones. We can do
	 * this once with the first address because the kernel won't be
	 * in the same segment with the user, so it will always be
	 * one or the other for the entire length. If vprot is ~PROT_WRITE,
	 * turn off write permission.
	 */
	if (vprot == ~PROT_USER)
		panic("hat_chgprot -- unimplemented prot");
	else if (vprot == ~PROT_WRITE)
		pprot = RO;
	else
		pprot = hat_vtop_prot(addr, vprot);
	for (a = addr, ea = addr + len; a < ea; a += MMU_PAGESIZE) {
		if (pte == NULL || ((u_int)a & (L3PTSIZE - 1)) < MMU_PAGESIZE) {
			pte = hat_ptefind(seg->s_as, a);
			/*
			 * If there is no page table, move the address up
			 * to the start of the next page table to avoid
			 * searching a bunch of invalid ptes.
			 */
			if (pte == NULL) {
				a = (addr_t)((u_int)a & ~(L3PTSIZE - 1)) +
				    L3PTSIZE - MMU_PAGESIZE;
				continue;
			}
		} else
			pte++;
		if (!pte_valid(pte))
			continue;
		if (pte->pte_readonly != pprot) {
			pte->pte_readonly = pprot;
			/*
			 * Only need to flush once, so keep track.
			 */
			if (flushed == 0 && pte->pte_nocache == 0) {
				vac_flush();
				flushed = 1;
			}
			/*
			 * Flush entry from the ATC since permissions changed.
			 */
			atc_flush_entry(a);
			/*
			 * Synchronize the I/O Mapper.
			 */
			IOMAP_SYNC(a, pte);
		}
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
 */
void
hat_unload(seg, addr, len)
	struct	seg *seg;
	addr_t	addr;
	u_int	len;
{
	register addr_t a;
	register struct pte *pte = NULL;
	register struct ptbl *ptbl;
	addr_t	ea;

	for (a = addr, ea = addr + len; a < ea; a += MMU_PAGESIZE) {
		if (pte == NULL || ((u_int)a & (L3PTSIZE - 1)) < MMU_PAGESIZE) {
			pte = hat_ptefind(seg->s_as, a);
			/*
			 * If there is no page table, increase addr to the
			 * start of the next page table to avoid searching
			 * a bunch of invalid ptes.
			 */
			if (pte == NULL) {
				a = (addr_t)((u_int)a & ~(L3PTSIZE - 1)) +
				    L3PTSIZE - MMU_PAGESIZE;
				continue;
			}
		} else
			pte++;

		/*
		 * Need to check to see if the pte is valid!
		 * Otherwise, this may result in a bad ptbl
		 * computed from the macro below, and this
		 * could then result in hat_pteunload panicing!!!
		 */
		if (!pte_valid(pte)) {
			continue;
		}
		/*
		 * If ghostunloading, decrement keep count and unload
		 * without syncing. Otherwise, unload normally.
		 * NOTE: we must decrement the keep count before unloading
		 * so the page table will get freed when empty.
		 */
		ptbl = ptetoptbl(pte);
		if (pte->pte_nosync) {
			ptbl->ptbl_keepcnt--;
			hat_pteunload(ptbl, pte, HAT_NOSYNC);
		} else {
			hat_pteunload(ptbl, pte, HAT_RMSYNC);
		}
	}
}

/*
 * Unload all the hardware translations that map page `pp'.
 */
void
hat_pageunload(pp)
	register struct page *pp;
{

	while (pp->p_mapping)
		hat_pteunload(ptetoptbl(pp->p_mapping),
		    (struct pte *)pp->p_mapping, HAT_RMSYNC);
}

/*
 * Get all the hardware dependent attributes for a page struct
 */
void
hat_pagesync(pp)
	struct page *pp;
{
	register struct pte *pte;

	/*
	 * Flush the data cache in case the MMU has changed the bits
	 * of one of the ptes behind our back.
	 */
	vac_flush();
	/*
	 * Call ptesync() for each mapping of this page.
	 */
	for (pte = (struct pte *)pp->p_mapping; pte != NULL;
	    pte = pte_nexts[ptetonum(pte)])
		hat_ptesync(pp, pte, 0);
}

/*
 * Returns a page frame number for a given kernel virtual address.
 */
u_int
hat_getkpfnum(addr)
	addr_t	addr;
{
	register struct pte *pte;

	pte = hat_ptefind(&kas, addr);

	/*
	 * Need to make sure the pte is valid too!
	 */
	if (pte == (struct pte *)NULL || !pte_valid(pte))
		return (NULL);
	else
		return (MAKE_PFNUM(pte));
}

/*
 * End of machine independent interface routines.
 *
 * The next few routines implement some machine dependent functions
 * needed for the 030. Not that each hat implementation can define
 * whatever additional interfaces make sense for that machine. These
 * routines are defined in <machine/vm_hat.h>.
 *
 * Start machine specific interface routines.
 */

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

	ptbl = hat_ptblalloc();
	hat_ptbllink(ptbl, &kas, addr, lvl);
	ptbl->ptbl_keepcnt = 1;
	/*
	 * Reduce end of page table search to the ptbl before this one.
	 * We can do this because all the kernel ptbl's are allocated
	 * in reverse order at boot time.
	 */
	ptblmax = ptbl - 1;
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

	ptbl = ptetoptbl(npte);
	for (i = 0; i < NL3PTEPERPT; i++) {
		ptbl->ptbl_validcnt++;
		ptbl->ptbl_keepcnt++;
		npte[i] = pte[i];
	}
}

/*
 * Called by UNIX during pagefault to insure the level 1 page table
 * has been initialized.
 */
void
hat_setup(as)
	register struct as *as;
{
	u_int	pfn;
	int	s;

	if (as->a_hat.hat_valid != NULL) {
		if (as == &kas)
			return;
		pfn = as->a_hat.hat_l1pfn;
		if (mmu_btop(mmu_getrootptr()) == pfn)
			return;
		s = splvm();

	} else {
		/*
		 * Pull the physical page number of the level 1 page table out
		 * of the pte mapping the u_area.
		 */
		if (as != &kas)
			pfn = MAKE_PFNUM(hat_ptefind(&kas, (addr_t)uunix));
		else
			pfn = UAREA_PTE->pte_pfn;
		/*
		 * Initialize the hat structure.
		 */
		as->a_hat.hat_l1pfn = pfn;
		as->a_hat.hat_sv_uunix = uunix;
		as->a_hat.hat_valid = 1;
		as->a_hat.hat_l3pts = as->a_hat.hat_l2pts = PTBL_NULL;
		/*
		 * Map in the level 1 page table and initialize it to be a copy
		 * of the kernel address space level 1 page table.
		 */
		s = splvm();
		hat_mapl1pt(&as->a_hat);
		bcopy((caddr_t)kl1pt, (caddr_t)L1PT, sizeof (struct l1pt));
	}
	/*
	 * Switch to the user's page table.
	 */
	mmu_setrootptr(&(((struct user *)
		(mmu_ptob(pfn)))->u_pcb.pcb_l1pt));
	(void) splx(s);
}

/*
 * Loads the pte for the given address with the given pte. Also sets it
 * up as a mapping for page pp, if there is one. The keep count of the
 * page table is incremented if the translation is to be locked.
 */
void
hat_pteload(seg, addr, pp, pte, flags)
	struct	seg *seg;
	addr_t	addr;

     struct	page *pp;
	struct	pte *pte;
	int	flags;
{
	register struct pte *ptr;
	register struct ptbl *ptbl;
	struct page *opp;
	int s;
	struct page *xpp;
	char *iop;

	/*
	 * Make sure there is a context set up for the address space.
	 */
	hat_setup(seg->s_as);
	/*
	 * If it's a kernel address, the page table should already exist.
	 * Otherwise we may have to create one.
	 */
	if (addr >= (addr_t)KERNELBASE)
		ptr = hat_ptefind(seg->s_as, addr);
	else
		ptr = hat_ptealloc(seg->s_as, addr);

	ptbl = ptetoptbl(ptr);
        /*
         * Increment the valid count if old one wasn't loaded.
         */
        if (ptr->pte_loaded == 0) {
                ptbl->ptbl_validcnt++;
        }
        /*
         * If we need to lock, increment the keep count.
         */
        if (flags & PTELD_LOCK)
                ptbl->ptbl_keepcnt++;

	opp = ptetopp(ptr);
	if (pp != opp) {
		/*
		 * We are loading a new translation. Add the translation to
		 * the list for this page, and increment the rss.
		 */
		s = splvm();
		pte_nexts[ptetonum(ptr)] = (struct pte *)pp->p_mapping;
		pp->p_mapping = (caddr_t)ptr;
		(void) splx(s);
		seg->s_as->a_rss += 1;
		/*
		 * If the page is non-cacheable, the translation is too.
		 */
		pte->pte_nocache = pp->p_nc;
	} else if (pp != NULL) {
		/*
		 * We are reloading an existing translation. Since this
		 * means we are probably changing the protections, we
		 * must flush the cache.
		 */
		vac_flush();
		/*
		 * ***** FCS show-stopper fix 3/29/89 gls ******
		 * Get the ref and mod bits for the existing
		 * translation, since the page may not have been
		 * hat_pagesync'ed yet.  Note that this must be
		 * done after flushing the cache!
		 */
		pte->pte_modified |= ptr->pte_modified;
		pte->pte_referenced |= ptr->pte_referenced;
	}
#ifdef	SUN3X_470
/* XXX */
	else {
		xpp = ptetopp(pte);
		iop = pptoiocp(xpp);
		if (xpp) {
			(*iop)++; /* total nullmap ref to this page */
			nullmap++; /* total nullmap ref for system */
		}
	}
/* XXX */
#endif	SUN3X_470
	/*
	 * If the virtual address is above SYSBASE or the physical address
	 * is not memory, we can't cache the page in the 030. This is because
	 * I/O is not kept cache consistent and devices don't cache.
	 */
	if (addr >= (addr_t)SYSBASE || MAKE_PFNUM(pte) >= mmu_btop(ENDOFMEM))
		pte->pte_nocache = 1;
	/*
	 * Load the pte and flush the ATC entry.
	 */
	*ptr = *pte;
	ptr->pte_loaded = 1;

#ifdef IOC
	/*
	 * If there is an I/O cache, figure out if this page should be
	 * I/O cacheable.
	 */
#ifdef SUN3X_470
	if (pp) {
		iop = pptoiocp(pp);
		*iop &= ~IOCS_IO_CACHABLE;
	}
#endif SUN3X_470

	if (ioc && ioc_net) {
#ifdef SUN3X_470
	if (nullmap == 0) {
/* XXXXXXXXXXXXXXXXXXXXX hardware workaround:
 * because there is a central cache and an IO cache and the hardware
 * doesn't ensure that they are consistent, software must prohibit
 * use of the IO cache when there are null mappings (a byproduct
 * of mmaping /dev/mem which is usually not done except in sysdiag).
 * Also, mmaping /dev/mem must not be allowed while that page has
 * IO cached mappings. (see vm/vm_page.c)
 * Data can get read causing the central cache to fill, and IO can
 * occur at the same time, causing the IO cache to fill. The IO cache
 * can (and is) flushed by software after the IO is done, but the
 * central cache line with the old data will flush later, corrupting the
 * data read.
 * It doesn't seem easy to get correct counts on a per-page basis,
 * as the ethernet code likes to map a page in IO cached multiple times
 * without unmapping it, so if there are any null mappings for any page,
 * no IO caching is allowed.
 * To do this in a better way would require putting these null mappings
 * on the page lists, and worrying about unmapping them and what to do with 
 * locked mappings, etc.
 */
#endif SUN3X_470
		/*
		 * If the I/O cache flag is passed to us, go for it.
		 * This occurs for vme I/O that went through bp_map().
		 */
		if (flags & PTELD_IOCACHE)
			ioc_pteset(ptr);
		/*
		 * All pages in the segkmap segment are I/O cacheable. The
		 * only I/O that can occur to these pages is ethernet.
		 */
		if (seg == segkmap)
			ioc_pteset(ptr);
		/*
		 * All pages that are mbufs are I/O cacheable. The
		 * only I/O that can occur to these pages is ethernet.
		 */
		if (addr >= (addr_t)mbutl && addr <
		    ((addr_t)mbutl + MBPOOLBYTES))
			ioc_pteset(ptr);
#ifdef SUN3X_470
		if (ptr->pte_iocache) {
			*iop |= IOCS_IO_CACHABLE;
			}
	}
#endif SUN3X_470
	}
#endif IOC
	atc_flush_entry(addr);
	/*
	 * Synchronize the I/O Mapper.
	 */
	IOMAP_SYNC(addr, ptr);
	/*
	 * Note whether this translation will be ghostunloaded or not.
	 */
	ptr->pte_nosync = (flags & PTELD_NOSYNC) != 0;
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

	*(int *)pte = MMU_INVALIDPTE;
	pte->pte_readonly = hat_vtop_prot(addr, vprot);
	pte->pte_vld = PTE_VALID;
	pte->pte_pfn = page_pptonum(pp);
}

/*
 * Returns a pointer to the pte struct for the given virtual address.
 * If the necessary page tables do not exist, return NULL.
 */
struct pte *
hat_ptefind(as, addr)
	struct	as *as;
	register addr_t addr;
{
	register struct lptp *lptp;
	register struct sptp *sptp;
	int s;

	/*
	 * Map in the level 1 page table (if necessary) and get the entry.
	 * Kernel addresses can be pulled from any level 1 table.
	 */
	s = splvm();
	if (addr < (addr_t)KERNELBASE)
		hat_mapl1pt(&as->a_hat);
	lptp = &L1PT->lptp[getl1in(addr)];
	if (lptp->lptp_vld == PTE_INVALID) {
		/*
		 * If the entry is invalid, give up.
		 */
		(void) splx(s);
		return (NULL);
	}
	/*
	 * Get the level 2 entry. Once we have it, we no longer need the
	 * level 1 table so we can spl() down.
	 */
	sptp = &((struct l2pt *)tatopte(lptp->lptp_taddr))->sptp[getl2in(addr)];
	(void) splx(s);
	if (sptp->sptp_vld == PTE_INVALID)
		/*
		 * If the entry is invalid, give up.
		 */
		return (NULL);
	/*
	 * Return the address of the level 3 entry.
	 */
	return (&((struct l3pt *)tatopte(sptp->sptp_taddr))->
	    pte[getl3in(addr)]);
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

	if (vprot & PROT_USER)	/* user permission */
	{
		if (addr >= (addr_t)KERNELBASE)
			panic("user addr in kernel space");
	}
	else			/* else kernel only */
	{
		 if (addr < (addr_t)KERNELBASE)
			panic("kernel addr in user space");
	}

	if (vprot & PROT_WRITE)
		return(RW);
	else if (vprot)
		return(RO);
	else
		panic("hat_vtop_prot -- null permission");
		/* NOTREACHED */
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
 * Unload the pte. If required, sync the referenced & modified bits.
 * If it's the last pte in the page table, and the table isn't locked,
 * free it up.
 */
static void
hat_pteunload(ptbl, pte, mode)
	register struct ptbl *ptbl;
	register struct pte *pte;
	int	mode;
{
	addr_t vaddr;
	struct page *pp;
	register struct pte **ptep;
	register struct sptp *sptp;
	struct	lptp *lptp;
	int	s;
	struct as *as = ptbl->ptbl_as;
	char *iop;

	s = splvm();
	if (pte->pte_loaded == 0) {
		(void) splx(s);
		return;
	}
	pte->pte_loaded = 0;
	(void) splx(s);

	vaddr = ptbl->ptbl_base + ptetovoff(pte);
	/*
	 * Remove the pte from the list of mappings for the page.
	 */
	pp = ptetopp(pte);
	if (pp != NULL) {	/* search for mapping */
		s = splvm();
		for (ptep = &(struct pte *)(pp->p_mapping); *ptep != NULL;
		    ptep = &pte_nexts[ptetonum(*ptep)])
			if (*ptep == pte)
				break;
		if (*ptep != NULL) {	/* found it */
			*ptep = pte_nexts[ptetonum(pte)];
			pte_nexts[ptetonum(pte)] = NULL;
			(void) splx(s);
			as->a_rss -= 1;
		}
#ifdef SUN3X_470
		else {
			(void) splx(s);
			iop = pptoiocp(pp);

			/*
			 * This is only a nullmap if the count of
			 * nullmap references to page pp is non-zero.
			 * This count is obtained by anding off the
			 * IOCS_IO_CACHABLE bit.
			 */
			if (*iop & ~IOCS_IO_CACHABLE) {
				(*iop)--; /* total nullmap ref for this page */
				if (nullmap == 0) panic("nullmap");
					nullmap--; /* total nullmap ref for system */
			}
		}
		if (pte->pte_iocache) {

			iop = pptoiocp(pp);
			*iop &= ~IOCS_IO_CACHABLE;
		}
#endif
	}
	/*
	 * We must flush the data cache here not only to get the page out
	 * of the cache, but so we can safely look at the referenced and
	 * modified bits of the pte. The MMU may have changed them behind
	 * our backs.
	 */
	vac_flush();
	/*
	 * If desired, sync up the referenced and modified
	 * bits with the hardware.
	 */
	if (mode & HAT_RMSYNC)
		hat_ptesync(pp, pte, HAT_INVSYNC);
	/*
	 * Invalidate the pte.
	 */
	pte->pte_vld = PTE_INVALID;
	pte->pte_loaded = pte->pte_nosync = 0;
	/*
	 * Synchronize the I/O Mapper.
	 */
	IOMAP_SYNC(vaddr, pte);
	/*
	 * Decrement the valid count.
	 */
	/*
	 * XXX raise priority earlier, so that decr valid and
	 * release page table done in same critical code section.
	 */
	s = splvm();
	ptbl->ptbl_validcnt--;
	/*
	 * This flush is unnecessary if the pte is from a different
	 * address space than our current one, but the check for that is
	 * probably more time consuming that just flushing. At worst,
	 * we are flushing a user address that will be used soon and
	 * we'll end up doing an extra table-walk.
	 */
	atc_flush_entry(vaddr);
	/*
	 * If the page table is no longer being used, free it up.
	 */
	if (ptbl->ptbl_validcnt == 0 && ptbl->ptbl_keepcnt == 0) {
		/*
		 * Map in the level 1 page table and find the entry.
		 */
		hat_mapl1pt(&as->a_hat);
		lptp = &L1PT->lptp[getl1in(ptbl->ptbl_base)];
		/*
		 * Use it to find the level 2 entry. Even though we may need
		 * the level 1 entry again, we spl() down and let it get
		 * reclaimed. This is done to avoid staying at a high
		 * priority for a long time.
		 */
		sptp = &((struct l2pt *)tatopte(lptp->lptp_taddr))->
		    sptp[getl2in(ptbl->ptbl_base)];

		/*
		 * Unlink the level 3 page table and add it to the free list.
		 */
		hat_ptblunlink(ptbl, 3);
		ptbl->ptbl_next = ptbl_freelist;
		ptbl_freelist = ptbltonum(ptbl);
		/*
		 * Invalidate the level 2 entry.
		 */
		sptp->sptp_vld = PTE_INVALID;
		/*
		 * Decrement the valid count of the level 2 page table.
		 */
		ptbl = ptetoptbl(sptp);
		ptbl->ptbl_validcnt--;
		/*
		 * If the level 2 table is also empty now, free it too.
		 */
		if (ptbl->ptbl_validcnt == 0) {
			/*
			 * Level 2 tables are always kept, so unkeep it.
			 */
			ptbl->ptbl_keepcnt = 0;
			/*
			 * Unlink the table and add it to the free list.
			 */
			hat_ptblunlink(ptbl, 2);
			ptbl->ptbl_next = ptbl_freelist;
			ptbl_freelist = ptbltonum(ptbl);
			/*
			 * Remap the level 1 table and ivalidate the entry.
			 * Note that the entry pointer doesn't need to be
			 * recalculated since we map it in at the same
			 * address every time.
			 */
			lptp->lptp_vld = PTE_INVALID;
		}
	}
	(void) splx(s);
}

/*
 * Sync the referenced and modified bits of the page struct
 * with the pte. Clears the bits in the pte.
 */
static void
hat_ptesync(pp, pte, flags)
	register struct page *pp;
	register struct pte *pte;
	register int flags;
{
	register struct as *as;

	if ((as = ptetoptbl(pte)->ptbl_as) && as->a_hatcallback) {
		addr_t vaddr = ptetoptbl(pte)->ptbl_base + ptetovoff(pte);

		as_hatsync(as, vaddr, (u_int)pte->pte_referenced,
				(u_int)pte->pte_modified,
				flags & HAT_INVSYNC ? (u_int)AHAT_UNLOAD : (u_int)0);
	}

	if (pp != NULL) {
		pp->p_ref |= pte->pte_referenced;
		pp->p_mod |= pte->pte_modified;
	}
	pte->pte_referenced = pte->pte_modified = 0;
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
	register struct lptp *lptp;
	register struct sptp *sptp;
	register struct ptbl *ptbl;
	int s;

	/*
	 * Map in the level 1 page table (if necessary) and get the entry.
	 * Kernel addresses can be pulled from any level 1 table.
	 */
	s = splvm();
	if (addr < (addr_t)KERNELBASE)
		hat_mapl1pt(&as->a_hat);
	lptp = &L1PT->lptp[getl1in(addr)];
	if (lptp->lptp_vld == PTE_INVALID) {
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
		ptbl->ptbl_keepcnt = 1;
		/*
		 * Now we remap the level 1 table and validate the entry.
		 * Note that we don't need to recalculate the entry pointer
		 * since we map it into the same address every time.
		 */
		s = splvm();
		if (addr < (addr_t)KERNELBASE)
			hat_mapl1pt(&as->a_hat);
		lptp->lptp_taddr = ptetota(ptbltopte(ptbl));
		lptp->lptp_vld = PTPTR_VALID;
		/*
		 * Get the level 2 entry.
		 */
		sptp = &((struct l2pt *)ptbltopte(ptbl))->sptp[getl2in(addr)];
	} else {
		/*
		 * Level 2 page table is valid. Simply get the appropriate
		 * entry.
		 */
		sptp = &((struct l2pt *)tatopte(lptp->lptp_taddr))->
		    sptp[getl2in(addr)];
	}
	/*
	 * At this point, we have the level 2 entry. We no longer need
	 * the level 1 table, so we spl() down.
	 */
	/*
	 * XXX - actually, we don't want to spl down yet, or
	 * someone might try to free our level2 page table
	 */
	if (sptp->sptp_vld == PTE_INVALID) {
		/*
		 * Level 2 entry is invalid, meaning there is no level
		 * 3 page table. Allocate one and link it into the
		 * address space. NOTE: we increment the valid count on
		 * the level 2 table first so it doesn't get freed out
		 * from under us by hat_ptblalloc().
		 */
		(ptetoptbl(sptp))->ptbl_validcnt++;
		ptbl = hat_ptblalloc();
		hat_ptbllink(ptbl, as, addr, 3);
		/*
		 * Validate the level 2 entry (valid count already incremented).
		 */
		sptp->sptp_taddr = ptetota(ptbltopte(ptbl));
		sptp->sptp_vld = PTPTR_VALID;
		(void) splx(s);
		/*
		 * Return the level 3 entry.
		 */
		return (&((struct l3pt *)ptbltopte(ptbl))->pte[getl3in(addr)]);
	} else {
		(void) splx(s);

		/*
		 * Level 2 table was valid, simply return the level 3 entry.
		 */
		return (&((struct l3pt *)tatopte(sptp->sptp_taddr))->
		    pte[getl3in(addr)]);
	}
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

	/*
	 * If the free list isn't empty, we are done searching.
	 */
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
			if (ptbl->ptbl_keepcnt == 0) {
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
	register int i;

	/*
	 * Loop through all the ptes in the table.
	 */
	for (i = 0, pte = ptbltopte(ptbl); i < NL3PTEPERPT; i++, pte++) {
		/*
		 * If the valid count is 0, the table has already been
		 * freed. We are done.
		 */
		if (ptbl->ptbl_validcnt == 0)
			break;
		/*
		 * If the pte is valid, unload it. We know it's ok to
		 * sync the translation because all ghostunload
		 * translations are locked, so their page tables cannot
		 * be freed.
		 */
		if (pte->pte_loaded)
			hat_pteunload(ptbl, pte, HAT_RMSYNC);
	}
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
	register u_short *list;
	register u_int num;

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
}

/*
 * Unlink the ptbl from the list of ptbl's allocated to this address space.
 * Separate lists are kept for level 2 and level 3 tables.
 */
static void
hat_ptblunlink(ptbl, lvl)
	register struct ptbl *ptbl;
	int	lvl;
{
	register u_short *list;

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
	ptbl->ptbl_next = ptbl->ptbl_prev = PTBL_NULL;
}

/*
 * This routine maps in a level 1 page table residing at the given
 * physical page. It uses a pre-defined virtual location. We may
 * get called for a page table that's already mapped, so we check for that.
 *
 * WARNING: because this routine uses a predefined virtual page, it is
 * 	    NOT safe to assume the mapping will remain. Interrupt routines
 * 	    may change the mapping out from under. Therefore, all invocations
 *	    must be protected from interrupts.
 */
static void
hat_mapl1pt(hat)
	struct	hat *hat;
{

	/*
	 * Check to see if the correct page is already mapped in.
	 */
	if (L1PT_PTE->pte_pfn == hat->hat_l1pfn)
		return;
	/*
	 * Oh well, we have to map it in.
	 */
	L1PT_PTE->pte_pfn = hat->hat_l1pfn;
	/*
	 * Flush any data from the old mapping. Would it be better to
	 * just have this page non-cacheable???
	 */
	vac_flush();
	/*
	 * Flush the old mapping out of the ATC.
	 */
	atc_flush_entry((addr_t)L1PT);
}

