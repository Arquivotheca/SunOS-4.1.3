/*       @(#)pte.h 1.1 92/07/30 SMI    */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Sun-3x hardware page table info.
 */

#ifndef _sun3x_pte_h
#define _sun3x_pte_h

#ifndef LOCORE
/*
 * Short page pointer, used for level 3 tables.
 */
struct pte {
	unsigned int	pte_pfn		: 19;
	unsigned int			:  4;
	unsigned int	pte_loaded	:  1;
	unsigned int	pte_iocache	:  1;
	unsigned int	pte_nocache	:  1;
	unsigned int	pte_nosync	:  1;
	unsigned int	pte_modified	:  1;
	unsigned int	pte_referenced	:  1;
	unsigned int	pte_readonly	:  1;
	unsigned int	pte_vld		:  2;
};

/*
 * Short table pointer, used for level 2 tables.
 */
struct sptp {
	unsigned int	sptp_taddr	: 28;
	unsigned int			:  1;
	unsigned int	sptp_readonly	:  1;
	unsigned int	sptp_vld	:  2;
};

/*
 * Long table pointer, used for level 1 tables.
 */
struct lptp {
	unsigned int			:  1;
	unsigned int	lptp_fill	: 21;
	unsigned int			:  1;
	unsigned int	lptp_svisor	:  1;
	unsigned int			:  1;
	unsigned int	lptp_nocache	:  1;
	unsigned int			:  3;
	unsigned int	lptp_readonly	:  1;
	unsigned int	lptp_vld	:  2;
	unsigned int	lptp_taddr	: 28;
	unsigned int			:  4;
};
#endif !LOCORE

/*
 * For compatibility...
 */
#define	PG_V		PTE_VALID
#define	PG_R		0x00000008
#define	PG_M		0x00000010

/*
 * Since the supervisor bit is not in the pte, ignore it and deal only
 * with read/write permissions. Correctness of user/supervisor must
 * be checked by the code using these.
 */
#define	RW		0x0
#define	RO		0x1

#define	MAKE_PROT(v)	((v) << 2)
#define	PG_PROT		MAKE_PROT(0x1)
#define	PG_KW		MAKE_PROT(RW)
#define	PG_KR		MAKE_PROT(RO)
#define	PG_UW		MAKE_PROT(RW)
#define	PG_URKR		MAKE_PROT(RO)
#define	PG_UR		MAKE_PROT(RO)
#define	PG_UPAGE	PG_KW

/*
 * Valid values.
 */
#define	PTE_INVALID	0		/* invalid */
#define	PTE_VALID	1		/* valid page pointer */
#define	PTPTR_VALID	2		/* valid table pointer */
#define	PTRPTR_VALID	3		/* valid root pointer */

/*
 * This macro is used to extract the physical page number from a pte.
 * This is used for things like mapin and cdevsw[] mmap routines.
 */
#define	MAKE_PFNUM(pte)		((pte)->pte_pfn)

/*
 * Macro to test a pte for "valid".
 */
#define	pte_valid(pte)		((pte)->pte_vld != 0)

/*
 * These are pseudo bus types for locating devices.
 */
#define	BT_OBMEM	0x01		/* dvma & cpu access */
#define	BT_OBIO		0x02		/* cpu only access */
#define	BT_VME		0x03		/* vme master interface */

/*
 * This macro returns the bus type for a given physical page frame.
 */
#define	bustype(pfn)	(mmu_ptob(pfn) < ENDOFTYPE0 ? BT_OBMEM : \
			(mmu_ptob(pfn) < VME16D16_BASE ? BT_OBIO : \
			BT_VME))
/*
 * Shift for table addresses.
 */
#define	PGPT_TADDRSHIFT	4

/*
 * Alignment requirement for tables.
 */
#define	PT_ALIGNREQ	0x10
#define	PT_ALIGNMASK	(PT_ALIGNREQ - 1)

#if !defined(LOCORE) && defined(KERNEL)
/* utilities defined in locore.s */
extern	struct pte Sysmap[];
extern	char Sysbase[];
extern	struct pte mmap[];
extern	struct pte CMAP1[];
extern	char CADDR1[];
extern	struct pte CMAP2[];
extern	char CADDR2[];
extern	char Syslimit[];

/*
 * These defines are just here to be an aid
 * for someone that was using these defines.
 */
#define Usrptmap        Sysmap
#define usrpt           Sysbase

#endif !defined(LOCORE) && defined(KERNEL)

#endif /*!_sun3x_pte_h*/

