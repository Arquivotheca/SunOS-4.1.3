/*      @(#)iommu.h	1.1 7/30/92 */

/*
 * Copyright (c) 1990-1991 by Sun Microsystems, Inc.
 */

#ifndef _sun4m_iommu.h
#define _sun4m_iommu.h

/* page offsets - fixed for architecture  */
#define IOMMU_CTL_REG           0x000      /* control regs */
#define IOMMU_BASE_REG          0x004      /* base addr regs */
#define IOMMU_FLUSH_ALL_REG     0x014      /* flush all regs */
#define IOMMU_ADDR_FLUSH_REG    0x018      /* addr flush regs */

#define V_IOMMU_CTL_REG		(V_IOMMU_ADDR+IOMMU_CTL_REG)
#define V_IOMMU_BASE_REG        (V_IOMMU_ADDR+IOMMU_BASE_REG)
#define V_IOMMU_FLUSH_ALL_REG	(V_IOMMU_ADDR+IOMMU_FLUSH_ALL_REG)
#define V_IOMMU_ADDR_FLUSH_REG  (V_IOMMU_ADDR+IOMMU_ADDR_FLUSH_REG)

/* constants for DVMA */
#define IOMMU_CTL_RANGE		(0)		/* 16MB for now */
#define IOMMU_DVMA_RANGE	((0x1000000) << IOMMU_CTL_RANGE)
#define IOMMU_DVMA_BASE		(0 - IOMMU_DVMA_RANGE)
#define	IOMMU_IOPBMEM_BASE	(0xFF800000)

/* this is for mapping mbutl */
#define	IOMMU_MBUTL_BASE	(0xFF600000)

/* this is for S-bus compability */
#define	IOMMU_SBUS_IOPBMEM_BASE	(0xFFF00000)

#define IOMMU_PAGE_SIZE		(4096)		/* 4k page */
#define IOMMU_N_PTES		(IOMMU_DVMA_RANGE/IOMMU_PAGE_SIZE)
#define IOMMU_PTE_TBL_SIZE	(IOMMU_N_PTES << 2)	/* 4B for each entry */

#define IOMMU_PTE_BASE_SHIFT	(14)	/* PA<35:14> is used in base reg */

#ifndef LOCORE
/* define IOPTEs */
union iommu_pte {
	struct {
		u_int pfn:24;		/* 24 bits PA<35:12> */
		u_int cache:1;		/* cacheable */
		u_int rsvd:4;		/* reserved */
		u_int write:1;		/* writeable */
		u_int valid:1;		/* valid */
		u_int waz:1;		/* write as zeros */
	} iopte;
	u_int iopte_uint;
};

/* iommu registers */
union iommu_ctl_reg {
	struct {
		u_int impl:4;	/* implementation # */
		u_int version:4;/* version # */
		u_int rsvd:19;	/* reserved */
		u_int range:3;	/* dvma range */
		u_int diag:1;	/* diagnostic enable */
		u_int enable:1;	/* iommu enable */
	} ctl_reg;
	u_int	ctl_uint;
};	/* control register */

union iommu_base_reg {
	struct {
		u_int base:22;	/* base of iopte tbl */
		u_int rsvd:10;	/* reserved */
	} base_reg;
	u_int	base_uint;
};	/* base register */

/* iommu address flush registers */
union iommu_flush_reg {
	struct {
		u_int rsvd1:1;		/* reserved 1 */
		u_int flush_addr:19;	/* flush addr */
		u_int rsvd2:12;		/* reserved 2 */
	} flush_reg;
	u_int	flush_uint;
};	/* flush register */

/* 
 * ptr to virtual addr that iopte starts/ends
 * NOTE: *ioptes MUST be aligned to its SIZE.
 */
extern union iommu_pte *ioptes, *eioptes;
extern int iom;

extern union iommu_pte* iom_ptefind( /* map_addr */ );
extern struct map* iom_choose_map( /* flags */ );
extern struct map* map_addr_to_map( /* map_addr */ );
#endif LOCORE

/* an mask that takes out unused bits in dvma address before flush */
#define IOMMU_FLUSH_MSK		0x7FFFF000

/* some macros for iommu ... */
#define iommu_btop(x)	mmu_btop(x)	/* all have 4k pages */
#define iommu_ptob(x)	mmu_ptob(x)	/* all have 4k pages */

/* define page frame number for IOPBMEM and DVMA_BASE, in IOMMU's view */
#define IOMMU_IOPBMEM_DVFN	iommu_btop(IOMMU_IOPBMEM_BASE)
#define IOMMU_SBUS_IOPBMEM_DVFN	iommu_btop(IOMMU_SBUS_IOPBMEM_BASE)
#define IOMMU_MBUTL_DVFN	iommu_btop(IOMMU_MBUTL_BASE)
#define IOMMU_DVMA_DVFN		iommu_btop(IOMMU_DVMA_BASE)

/* 
 * our VME slave port is at low addr, also note maps are described in 
 * iommu "pages".
 */ 

#define	VME24MAP_BASE		(iommu_btop(mmu_ptob(IOPBMEM))) 
#define	VME32MAP_BASE		(iommu_btop(0x100000))
#define	ALTSBUSMAP_BASE		(iommu_btop(IOMMU_IOPBMEM_BASE))
#define	MBUTLMAP_BASE		(iommu_btop(IOMMU_MBUTL_BASE)) 

/* NOTE: SBUS now returns low address too */
#define	SBUSMAP_BASE		(iommu_btop(mmu_ptob(IOPBMEM))) 
/* NOTE: bigsbusmap starts with correct high address */
#define	BIGSBUSMAP_BASE		(iommu_btop(IOMMU_DVMA_BASE)) 

/* dvma map sizes */

/* A24 size: 1 meg - IOPBMEM - 2 pages for dump */
#define	VME24MAP_SIZE	(VME32MAP_BASE - VME24MAP_BASE - iommu_btop(mmu_ptob(2)))
#define	VME32MAP_SIZE	(iommu_btop(0x600000))		/* 6 meg */
#define	ALTSBUSMAP_SIZE	(iommu_btop(0x700000))		/* 7 meg */
#define	MBUTLMAP_SIZE	(iommu_btop(0x200000))		/* 2 meg */
#define	SBUSMAP_SIZE	(iommu_btop(0-IOMMU_SBUS_IOPBMEM_BASE)-(iommu_btop(mmu_ptob(IOPBMEM))))
#define	BIGSBUSMAP_SIZE	(IOMMU_MBUTL_DVFN - BIGSBUSMAP_BASE) /* 6 meg */

/* 
 * NOTE: These mapping are not visible from host SRMMU. These mappings are 
 * 	 used on IOMMU only.
 */
#define	VME24DVMA_BASE		(IOMMU_IOPBMEM_BASE + iommu_ptob(VME24MAP_BASE))
#define	VME32DVMA_BASE		(IOMMU_SBUS_IOPBMEM_BASE - iommu_ptob(VME32MAP_SIZE))/* 6 meg below SBUS */ 
#define	ALTSBUSDVMA_BASE	(IOMMU_IOPBMEM_BASE)
#define	MBUTLDVMA_BASE		(IOMMU_MBUTL_BASE)
#define	SBUSDVMA_BASE		(IOMMU_SBUS_IOPBMEM_BASE + (mmu_ptob(IOPBMEM))) 
#define	BIGSBUSDVMA_BASE	(IOMMU_DVMA_BASE)

/* some macros */
#define dvma_pfn_to_iopte(dvma_pfn)	(&ioptes[(dvma_pfn)])
#define iopte_to_dvma(piopte)		(iommu_ptob(piopte - ioptes) + IOMMU_DVMA_BASE)

/* flags for iom_pteload/iom_pagesync */
#define IOM_WRITE	0x1
#define IOM_CACHE	0x2

#endif _sun4m_iommu.h
