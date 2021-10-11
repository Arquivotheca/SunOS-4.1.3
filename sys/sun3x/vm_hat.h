/*       @(#)vm_hat.h 1.1 92/07/30 SMI    */
/* syncd to sun/vm_hat.h 1.24 */

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
#ifndef _sun3x_vm_hat_h
#define _sun3x_vm_hat_h

#include <sys/types.h>
#include <machine/mmu.h>
#include <machine/pte.h>

/*
 * Page table structure. There is one of these for each level 2 and level 3
 * page table in the system.
 */
struct ptbl {
	short	ptbl_keepcnt;		/* keep count for table */
	short	ptbl_validcnt;		/* valid count for table */
	struct	as *ptbl_as;		/* address space using table */
	addr_t	ptbl_base;		/* base address within address space */
	u_short	ptbl_next;		/* next table for address space */
	u_short	ptbl_prev;		/* previous table for address space */
};

/*
 * These are the various page tables. They are simply arrays of entries
 * that are defined in <sun3x/pte.h>.
 */
struct l3pt {
	struct	pte pte[NL3PTEPERPT];	/* short page pointers */
};

struct l2pt {
	struct	sptp sptp[NL2PTEPERPT];	/* short table pointers */
};

struct l1pt {
	struct	lptp lptp[NL1PTEPERPT];	/* long table pointers */
};

/*
 * The hat structure is the machine dependent hardware address translation
 * structure kept in the address space structure to show the translation.
 *
 * If hat_valid is NULL, there are no translations for the address space.
 */
struct hat {
	u_int	hat_valid	:  8;	/* level 1 address is valid */
	u_int	hat_l1pfn	: 24;	/* physical page # of level 1 table */
	u_short	hat_l2pts;		/* level 2 tables for this hat */
	u_short	hat_l3pts;		/* level 3 tables for this hat */
	struct user *hat_sv_uunix;	/* saved uunix, virtual addr of l1pfn */
};
/*
 * This mask allows assembly code to extract the l1pfn from an int.
 */
#define	HAT_L1PFNMASK		0x00FFFFFF

/*
 * Flags to pass to hat_pteunload().
 */
#define	HAT_NOSYNC		0x00
#define	HAT_RMSYNC		0x01
#define HAT_INVSYNC		0x02

/*
 * Flags to pass to hat_pteload() and segkmem_mapin().
 */
#define	PTELD_LOCK		0x01
#define	PTELD_NOSYNC		0x02
#define	PTELD_IOCACHE		0x04

/*
 * Flags for elements of the iocs array
 */
#define	IOCS_IO_CACHABLE	0x80

/*
 * Limit on number of page tables because we use indices in the hat struct.
 */
#define	MAX_PTBLS		((1 << 16) - 1)
#define PTBL_NULL		MAX_PTBLS

/*
 * Macros to define constant virtual locations.
 */
#define	L1PT			(&((struct user *)V_L1PT_ADDR)->u_pcb.pcb_l1pt)
#define	L1PT_PTE		(eptes - mmu_btop(0 - V_L1PT_ADDR) + \
				    mmu_btop(MONEND - DEBUGSTART))
#define	UAREA_PTE		(eptes - mmu_btop(0 - UADDR))

/*
 * These macros extract the indices into the various page tables for
 * a given virtual address.
 */
#define	getl1in(addr)	(((u_int)(addr) & MMU_L1_MASK) >> MMU_L1_SHIFT)
#define	getl2in(addr)	(((u_int)(addr) & MMU_L2_MASK) >> MMU_L2_SHIFT)
#define	getl3in(addr)	(((u_int)(addr) & MMU_L3_MASK) >> MMU_L3_SHIFT)
/*
 * This macro takes a pte and returns a pointer to the page struct (if any)
 * that refers to the same page.
 */
#define	ptetopp(pte)	(pte_valid(pte) ? page_numtopp((pte)->pte_pfn) : NULL)
/*
 * This macro takes an ptbl number and returns a pointer to the ptbl struct.
 */
#define	numtoptbl(num)	((num) == PTBL_NULL ? NULL : &ptbls[(num)])
/*
 * This macro takes a ptbl pointer and returns the ptbl number.
 */
#define	ptbltonum(ptbl)	((ptbl) == NULL ? PTBL_NULL : (ptbl) - ptbls)
/*
 * This macro takes a pte pointer and returns the pte number.
 */
#define	ptetonum(pte)	((pte) - ptes)
/*
 * This macro takes the table address field of a ptp structure and returns
 * a pointer to the page table it implies.
 */
#define	tatopte(taddr)	((struct pte *)((u_int)ptes + (((taddr) << \
			PGPT_TADDRSHIFT) - pts_addr)))
/*
 * This macro takes a pointer to a pte structure and returns an integer
 * that can be used as a table address for a page table starting at that
 * pte.
 */
#define ptetota(pte)	((pts_addr + ((u_int)pte - (u_int)ptes)) >> \
			PGPT_TADDRSHIFT)
/*
 * This macro takes a pointer to a pte sturcture and returns the offset of
 * the pte in its page table.
 */
#define	ptetooff(pte)	(((pte) - ptes) & (NL3PTEPERPT - 1))
/*
 * This macro takes a pointer to a pte structure and returns the virtual
 * offset of the page in its page table.
 */
#define	ptetovoff(pte)	(mmu_ptob(ptetooff(pte)))
/*
 * This macro takes a ptbl pointer and returns a pointer to the first pte 
 * in the ptbl.
 */
#define	ptbltopte(pt)	(&ptes[((struct ptbl *)(pt) - ptbls) << NL3PTESHIFT])
/*
 * This macro takes a pte and returns a pointer to the ptbl struct
 * for the page table containing the pte.
 */
#define ptetoptbl(pt)	(&ptbls[((struct pte *)(pt) - ptes) >> NL3PTESHIFT])

/*
 * This macro synchronizes the I/O Mapper to a kernel pte mapping if
 * appropriate.
 */
#define IOMAP_SYNC(addr, ptr)	{ if (addr >= (addr_t)SYSBASE) \
				((struct pte *)(V_IOMAP_ADDR))[ \
				mmu_btop((u_int)addr - SYSBASE)] = *ptr; }

#ifdef SUN3X_470
/* iocache counters - for hardware workaround */
extern char *iocs;

/* convert page pointer to ioc pointer */
#define pptoiocp(pp) (char *)(&iocs[pp - pages])
#endif

#ifdef	KERNEL
/*
 * For the 68030 MMU hat code, we don't need to do any initialization at
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
 * Since the sun3x implemenatation always has page tables locked for the
 * entire kernel address space, hat_reserve() isn't needed. If hat_reserve()
 * is ever redefined to work on non-kernel addresses, it will have to be
 * implemented.
 */
#define	hat_reserve(seg, addr, len)
void hat_setup(/* as */);
void hat_pteload(/* seg, addr, pp, pte, flags */);
void hat_mempte(/* pp, vprot, pte, addr */);
struct pte *hat_ptefind(/* as, addr */);
u_int hat_vtop_prot(/* addr, vprot */);

/*
 * This data is used in MACHINE-SPECIFIC places.
 */
extern	struct ptbl *ptbls, *eptbls;
extern	struct pte *ptes, *eptes;
extern	struct pte **pte_nexts;
extern	u_int pts_addr;
extern	struct l1pt *kl1pt;
extern	struct l2pt *kl2pts;
extern	minpts;
#endif	KERNEL

#endif /*!_sun3x_vm_hat_h*/
