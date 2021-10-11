/*       @(#)vm_hat.h 1.1 92/07/30 SMI    */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * VM - Hardware Address Translation management.
 *
 * This file describes the contents of the machine specific
 * hat data structures and the machine specific hat procedures.
 * The machine independent interface is described in <vm/hat.h>.
 */

#ifndef	_sun4m_vm_hat_h
#define	_sun4m_vm_hat_h

#ifndef LOCORE
#include <sys/types.h>
#include <machine/mmu.h>
#include <machine/pte.h>
#include <debug/debug.h>

/* 
 * Software context structure. There is a one to one map from these context
 * structures to entries into the context (root) table of the Sparc Reference
 * MMU.
 * To conserve memory, efforts have been made to eliminate a few
 * of the things that were once stored in this structure.
 * XXX - Since the context number can be calculated from the
 * position that this structure is in the 'ctxs' array, c_num
 * could also be eliminated too.  This may be more trouble than it
 * is worth though. For a machine with 4096 contexts this would
 * save 4 pages.
 */

struct ctx {
	u_int c_num;		/* hardware info (context number) */
	struct as *c_as;	/* back pointer to the address space */
};

/*
 * Page table structure. There is one of these for each level 2 and level 3
 * page table in the system.
 */
struct ptbl {
	u_short         ptbl_lockcnt;	/* lock count for table */
	u_short         ptbl_validcnt;	/* valid count for table */
	u_short         ptbl_keep:1;	/* kept permanently for kernel */
	u_short	        ptbl_ncflag:1;	/* some pte decached by hat_pteload */
	struct as      *ptbl_as;	/* address space using table */
	addr_t          ptbl_base;	/* base address within address space */
	u_short         ptbl_next;	/* next table for address space */
	u_short         ptbl_prev;	/* previous table for address space */
};

/* 
 * Software version of hardware pte. Used to keep track of a
 * variety of things not storable in the hardware pte
 * Since one of these corresponds to every pte, efforts were
 * made to use as little memory as possible.  See comment where
 * MAX_SPTES is defined for more related information.
 */

struct spte {
	u_int spte_next : 22;		/* other sptes mapping same page */
	u_int spte_intrep : 1;		/* interrupt replaceable flag */
	u_int spte_nosync : 1;		/* ghost unload flag */
	u_int spte_valid : 1;			/* pme loaded flag */
	u_int spte_iocache : 1;		/* Mapping is in the iocache? */
	u_int spte_rsvd : 6;		/* reserved */
};


struct l3pt {
	struct pte pte[MMU_NPTE_THREE];
};

struct l2pt {
	union ptpe ptpe[MMU_NPTE_TWO];
};

struct l1pt {
	union ptpe ptpe[MMU_NPTE_ONE];
};

/*
 * "struct ctx_table"'s size depends on how
 * many contexts you are supporting, and is
 * not known at compile time.
 */
struct ctx_table {
	struct ptp ct[1];
};

/*
 * The hat structure is the machine dependent hardware address translation
 * structure kept in the address space structure to show the translation.
 *
 * If hat_valid is NULL, there are no translations for the address space.
 */

struct hat {
	unsigned	hat_ptvalid:1;		/* flag for page table valid */
	unsigned	__spare:23;		/* == space for rent == */
	unsigned char	hat_oncpu;		/* active MID, or 0 */
	struct l1pt    *hat_l1pt;		/* va of level-1 table */
	unsigned	hat_l2pts;		/* ix of 1st level-2 table */
	unsigned	hat_l3pts;		/* ix of 1st level-3 table */
	struct ctx     *hat_ctx;		/* va of sw context */
};

#endif !LOCORE

/*
 * Starting with context 0, the first NUM_LOCKED_CTXS contexts
 * are locked so that hat_getctx can't steal any of these
 * contexts.  At the time this software was being developed, the
 * only context that needs to be locked is context 0, the kernel
 * context, and so this constant was originally defined to be 1.
 */
#define	NUM_LOCKED_CTXS	1

/*
 * This mask allows assembly code to extract the l1pfn from an int.
 */

#define HAT_L1PFNMASK	MMU_STD_PAGEMASK
#define HAT_L1PFNSHIFT	MMU_STD_PAGESHIFT

/*
 * Flags to pass to hat_pteunload().
 */
#define HAT_NOSYNC      0x00
#define HAT_RMSYNC      0x01
#define HAT_NCSYNC      0x02
#define HAT_INVSYNC     0x04

/*
 * Flags to pass to hat_pteload() and segkmem_mapin().
 */
#define PTELD_LOCK	0x01
#define PTELD_NOSYNC	0x02
#define PTELD_IOCACHE	0x04
#define PTELD_INTREP	0x08

/*
 * Limit on number of page tables because we use indices in the hat struct.
 * This allows for up to approximately:
 * 64K (l3 tables) * 64 (ptes/table) * 4K (bytes/page) = 16G to be
 * mapped at any one time!!!  Sun-4M uses only one space (0) for
 * main memory, which means that the maximum possible memory size 
 * is 4G.  Actual systems will probably have much less.  While it
 * is true that memory is often mapped multiple times when
 * processes share memory, it would be unusual for there to be
 * a significant amount of sharing when 5G is mapped.  Even if
 * there were alot of sharing, 16G is still good enough to map
 * 5G of memory, memory 3.2 times!  Furthermore, note that since
 * the page tables are managed as a stealable resource in a pool,
 * the system will still work when demand exceeds this already
 * ridiculously generous limits.
 * Low-end desktop machines that have between 4 & 64MB of memory
 * require that the size of structures be kept as compact as possible.
 */
#define MAX_PTBLS       ((1 << 16) - 1)
#define PTBL_NULL       MAX_PTBLS

/*
 * Note that since each level-3 page table has 64 ptes, and
 * since there is a spte for every pte, then MAX_SPTE should
 * allow for 64 times as many sptes as level-3 page tables.
 * Note also that this allows the spte structure to be only
 * 32 bits long total.
 */
#define MAX_SPTES	((1 << 22) - 1)
#define SPTE_NULL	MAX_SPTES

#ifndef LOCORE
/*
 * These macros extract the indices into the various page tables for
 * a given virtual address.
 */
#define getl1in(a)   ((u_int) MMU_L1_INDEX((u_int)a))
#define getl2in(a)   ((u_int) MMU_L2_INDEX((u_int)a))
#define getl3in(a)   ((u_int) MMU_L3_INDEX((u_int)a))


/*
 * This macro takes a pte and returns a pointer to the page struct (if any)
 * that refers to the same page.
 */
#define ptetopp(a)    (pte_valid(a) ? \
	page_numtopp((a)->PhysicalPageNumber) : NULL)

/*
 * This macro takes an ptbl number and returns a pointer to the ptbl struct.
 */
#define numtoptbl(num)  ((num) == PTBL_NULL ? NULL : &ptbls[(num)])

/*
 * This macro takes a ptbl pointer and returns the ptbl number.
 */
#define ptbltonum(a) ((a) == NULL ? PTBL_NULL : (a) - ptbls)

/*
 * This macro takes a pte pointer and returns the pte number.
 */
#define ptetonum(a)   ((a) - ptes)

/* 
 * This macro take a spte pointer and returns the spte number
 */
#define sptetonum(b)	((b) == NULL ? SPTE_NULL : ((b) - sptes))


/* 
 * This macro takes a pte num and translates it into a spte pointer 
 */
#define numtospte(num) ((num) == SPTE_NULL ? (struct spte *)NULL : \
				&sptes[(num)])

/*
 * This macro takes a spte and translates it into a pte 
 */
#define sptetopte(b) ((b) == NULL ? NULL : &ptes[((b) - sptes)])

/*
 * This macro takes the table address field of a ptp structure and returns
 * a pointer to the page table it implies.
 */
#define tatopte(ta)  ((struct pte *)((u_int)ptes + (u_int)((u_int)((u_int)(ta) << \
            MMU_STD_PTPSHIFT) - (u_int)pts_addr)))

/*
 * This macro takes a PageTablePointer (as is found in a ptp struct)
 * which points a physical address and 
 * and converts it into a pointer to the first pte in the table implied
 * by that PageTablePointer as a virtual address
 */
#define ptptopte(ptp)  ((struct pte *)((u_int)KERNELBASE + (((u_int)(ptp) << \
            MMU_STD_PTPSHIFT))))

/*
 * This macro takes a pointer to a pte structure and returns an integer
 * that can be used as a table address for a page table starting at that
 * pte.
 */
#define ptetota(a)    (((u_int)pts_addr + (u_int)((u_int)(a) - (u_int)ptes)) >> \
            MMU_STD_PTPSHIFT)

/*
 * This macro takes a pointer to a pte sturcture and returns the offset of
 * the pte in its page table.
 */
#define ptetooff(a)   (((a) - ptes) & (NL3PTEPERPT - 1))
/*
 * This macro takes a pointer to a pte structure and returns the virtual
 * offset of the page in its page table.
 */
#define ptetovoff(a)  (mmu_ptob(ptetooff(a)))
/*
 * This macro takes a ptbl pointer and returns a pointer to the first pte
 * in the ptbl.
 */
#define ptbltopte(arg)   (&ptes[((struct ptbl *)(arg) - ptbls) << NL3PTESHIFT])
/*
 * This macro takes a ptbl pointer and returns a pointer to the first spte
 * in the ptbl.
 */

#define ptbltospte(arg)   (&sptes[((struct ptbl *)(arg) - ptbls) << NL3PTESHIFT])
/*
 * This macro takes a pte and returns a pointer to the ptbl struct
 * for the page table containing the pte.
 */
#define ptetoptbl(arg)   (&ptbls[((struct pte *)(arg) - ptes) >> NL3PTESHIFT])

#define ptetospte(arg)   (&sptes[((struct pte *)(arg) - ptes)])

/*
 * This macro takes a spte and returns a pointer to the ptbl struct
 * for the page table containing the spte.
 */
#define sptetoptbl(a) ((a) == (struct spte *)NULL ? (struct ptbl *)PTBL_NULL : \
	&ptbls[((struct spte *)(a) - sptes) >> NL3PTESHIFT])

/*
 * This macro takes a pte and returns the virtual address that
 * it maps.
 */
/*
#define	ptetovaddr(a)	(ptetoptbl(a)->ptbl_base + mmu_ptob(pte - ptbltopte(ptetoptbl(a))))
*/

/* This version of the macro requires less work */
#define	ptetovaddr(a)	(ptetoptbl(a)->ptbl_base + ptetovoff(a))

/*
 * This macro takes a spte and returns the virtual address that
 * it maps.
 */
#define	sptetovaddr(a)	(ptetovaddr(sptetopte((a))))
/*
#define	sptetovaddr(a)	(sptetoptbl(a)->ptbl_base + mmu_ptob(spte - ptbltospte(sptetoptbl(a))))
*/

/*
 * This macro synchronizes the I/O Mapper to a kernel pte mapping if
 * appropriate.
 */
#ifdef IOC
#define IOMAP_SYNC(addr, ptr)   { if (addr >= (addr_t)SYSBASE) \
                ((struct pte *)(V_IOMAP_ADDR))[ \
                mmu_btop((u_int)addr - SYSBASE)] = *ptr; }
#else
#define IOMAP_SYNC(addr, ptr)	
#endif

#endif LOCORE

#if defined(KERNEL) && !defined(LOCORE)
/*
 * For the Standard Reference MMU hat code, 
 * we don't need to do any initialization at
 * hat creation time since we know that the structure is guaranteed to be
 * all zeros.  we can define the machine independent interface routine
 * hat_alloc() to be a nop here (<vm/hat.h> is set up to deal with this).
 */
#define hat_alloc(as)

/*
 * These routines are all MACHINE-SPECIFIC interfaces to the hat routines.
 * These routines are called from machine specific places to do the
 * dirty work for things like boot initialization, mapin/mapout and
 * first level fault handling.
 */
struct ptbl *hat_ptblreserve(/* seg, addr, lvl */);
void hat_ptecopy(/* pte, npte */);
/*
 * Since the sun4m implemenatation always has page tables locked for the
 * entire kernel address space, hat_reserve() isn't needed. If hat_reserve()
 * is ever redefined to work on non-kernel addresses, it will have to be
 * implemented.
 */
#define hat_reserve(seg, addr, len)
void hat_setup(/* as */);
void hat_pteload(/* seg, addr, pp, pte, flags */);
void hat_mempte(/* pp, vprot, pte, addr */);
union ptpe *hat_ptefind(/* as, addr */);
u_int hat_vtop_prot(/* addr, vprot */);
u_int hat_ptov_prot(/* struct pte * */);
void hat_getctx(/* struct as *as */);


/*
 * This data is used in MACHINE-SPECIFIC places.
 */
extern  int minpts;
extern  struct ctx *kctx;
extern  struct ptbl *ptbls, *eptbls;
extern  struct pte *ptes, *eptes;
extern  struct spte *sptes, *esptes;
/* The real context table */
extern  union ptpe *contexts, *econtexts;
extern  u_int pts_addr;
extern  struct l1pt *kl1pt;
extern  struct l2pt *kl2pts;

#endif KERNEL && !LOCORE


#endif	/* _sun4m_vm_hat_h */
