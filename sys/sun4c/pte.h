/*	@(#)pte.h 1.1 92/07/30 SMI	*/

#ifndef	_sun4c_pte_h
#define	_sun4c_pte_h

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Sun-4c hardware page table entry
 */

#ifndef LOCORE
struct pte {
	unsigned int	pg_v:1;		/* valid bit */
	unsigned int	pg_prot:2;	/* access protection */
	unsigned int	pg_nc:1;	/* no cache bit */
	unsigned int	pg_type:2;	/* page type */
	unsigned int	pg_r:1;		/* referenced */
	unsigned int	pg_m:1;		/* modified */
	unsigned int	:8;
	unsigned int	pg_pfnum:16;	/* page frame number */
};
#endif	!LOCORE

#define	PG_V		0x80000000	/* page is valid */
#define		PG_W	0x40000000	/* write enable bit */
#define		PG_S	0x20000000	/* system page */
#define	PG_NC		0x10000000	/* no cache bit */
#define	PG_TYPE		0x0C000000	/* page type mask */
#define	PG_R		0x02000000	/* page referenced bit */
#define	PG_M		0x01000000	/* page modified bit */
#define	PG_PFNUM	0x0C00FFFF	/* page # mask - XXX includes type */

#define	KW		0x3
#define	KR		0x1
#define	UW		0x2
#define	UR		0x0
#define	URKR		0x0

#define	MAKE_PROT(v)	((v) << 29)
#define	PG_PROT		MAKE_PROT(0x3)
#define	PG_KW		MAKE_PROT(KW)
#define	PG_KR		MAKE_PROT(KR)
#define	PG_UW		MAKE_PROT(UW)
#define	PG_URKR		MAKE_PROT(URKR)
#define	PG_UR		MAKE_PROT(UR)
#define	PG_UPAGE	PG_KW		/* sun4 u pages not user accessable */

#define	OBMEM		0x0
#define	OBIO		0x1

#define	MAKE_PGT(v)	((v) << 26)
#define	PGT_MASK	MAKE_PGT(3)
#define	PGT_OBMEM	MAKE_PGT(OBMEM)		/* onboard memory */
#define	PGT_OBIO	MAKE_PGT(OBIO)		/* onboard I/O */

/*
 * This macro can be used to convert a struct pte * in to the relevant
 * bits needed for things like mapin and cdevsw[] mmap routine which
 * still deal with integers.  Effectively for the Sun-3 this macro is
 * really implementing the following expression more effectively:
 *
 *	(MAKE_PGT((pte)->pg_type) | (pte)->pg_pfnum)
 */
#define	MAKE_PFNUM(pte)	((*(unsigned int *)(pte)) & PG_PFNUM)

/*
 * Macros to test a pte.
 */
#define	pte_valid(pte)	((pte)->pg_v != 0)
#define	pte_konly(pte)	((*(int *)(pte) & PG_S) != 0)
#define	pte_ronly(pte)	((*(int *)(pte) & PG_W) == 0)

#if !defined(LOCORE) && defined(KERNEL)
/* utilities defined in locore.s */
extern	struct pte Heapptes[];
extern	char Heapbase[];
extern	char Heaplimit[];
extern	struct pte Bufptes[];
extern	char Bufbase[];
extern	char Buflimit[];
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
#define	Usrptmap	Sysmap
#define	usrpt		Sysbase

extern	struct pte mmu_pteinvalid;
#endif !defined(LOCORE) && defined(KERNEL)

#endif	/* !_sun4c_pte_h */
