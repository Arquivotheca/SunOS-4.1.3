/*      @(#)pte.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/* Copyright (c) 1988, Sun Microsystems, Inc. All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary 
 * trade secret, and it is available only under strict license 
 * provisions. This copyright notice is placed here only to protect
 * Sun in the event the source is deemed a published work. Disassembly,
 * decompilation, or other means of reducing the object code to human
 * readable form is prohibited by the license agreement under which
 * this code is provided to the user or company in possession of this
 * copy
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in subparagraph
 * (c) (1) (ii) of the Rights in Technical Data and Computeer Software
 * clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 * NASA FAR Supplement
 */
/* This file (translation_intern.h) contains all the structure 
 * and macro definitions for the Sparc Reference MMU as of 17 May, 1988
 */

#ifndef _sun4m_pte_h
#define _sun4m_pte_h

#ifndef LOCORE
#include <sys/types.h>

/* 
 * generate a pte in the specified space, for the specified page
 * frame, with the specified permissions, possibly cacheable.
 * note: upper bits of page frame may modify the space value,
 * this is a feature not a bug.
 */

extern int vac;
#define	MRPTEOF(s,p,a,c,m,r)	(((s)<<28)|((p)<<8)|((c)<<7)|((m)<<6)|((r)<<5)|((a)<<2)|MMU_ET_PTE)

#define	PTEOF(s,p,a,c)		MRPTEOF(s,p,a,c,0,0)

struct pte {
	u_int PhysicalPageNumber:24;
	u_char Cacheable:1;
	u_char Modified:1;
	u_char Referenced:1;
	u_char AccessPermissions:3;
	u_char EntryType:2;
};

struct proot {
	u_int rootMap:4;
	u_int rootBoundary:20;
	u_char Cacheable:1;
	u_char Modified:1;
	u_char Referenced:1;
	u_char AccessPermissions:3;
	u_char EntryType:2;
};

struct pfirst {
	u_int firstTopMap:4;
	u_int firstMap:8;
	u_int firstBoundary:12;
	u_char Cacheable:1;
	u_char Modified:1;
	u_char Referenced:1;
	u_char AccessPermissions:3;
	u_char EntryType:2;
};

struct psecond {
	u_int secondTopMap:12;
	u_int secondMap:6;
	u_int secondBoundary:6;
	u_char Cacheable:1;
	u_char Modified:1;
	u_char Referenced:1;
	u_char AccessPermissions:3;
	u_char EntryType:2;
};

union ptes {
	struct pte pte;
	u_int pte_int;
};
#endif !(LOCORE)

#define PTE_ETYPE(a)	((a) & 0x3)
#define PTE_ETYPEMASK	(0x3)
#define PTE_PERMS(b)	(((b) & 0x7) << 2)
#define PTE_PERMMASK	((0x7 << 2))
#define PTE_PERMSHIFT	(2)
#define PTE_REF(c)		(((c) & 0x1) << 5)
#define	PTE_REF_MASK	(0x20)
#define PTE_MOD(c)		(((c) & 0x1) << 6)
#define	PTE_MOD_MASK	(0x40)
#define	PTE_RM_MASK	(PTE_REF_MASK|PTE_MOD_MASK)
#define PTE_CACHEABLE(d)	(((d) & 0x1) << 7)
#define	PTE_CE_MASK	(0x80)
#define PTE_PFN(p)		(((p) & 0xFFFFFF) << 8)
#define	PTE_PFN_MASK	(0xFFFFFF00)
#define PTE_DSI_PPN(d,p)		((((d) & 0x3) << 22) | (p) & 0x3FFFFF)
#define	PTE_DSI_MASK	(0xC0000000)

#define PTE_DSI_MAINMEM	0
#define PTE_DSI_REGS	1
#define PTE_DSI_VME16	2
#define PTE_DSI_VME32	3


#define pte_valid(pte) (((pte)->EntryType) == MMU_ET_PTE)

#define pte_konly(pte) (((pte)->AccessPermissions == MMU_STD_SRWX) || \
					((pte)->AccessPermissions == MMU_STD_SRWX))

#define pte_ronly(pte) (((pte)->AccessPermissions == MMU_STD_SRUR) || \
						((pte)->AccessPermissions == MMU_STD_SXUX) || \
						((pte)->AccessPermissions == MMU_STD_SRXURX) || \
						((pte)->AccessPermissions == MMU_STD_SRX))

#define MAKE_PFNUM(a)	\
	(((struct pte *)(a))->PhysicalPageNumber)

/* Definitions for EntryType */

#define MMU_ET_INVALID		0
#define MMU_ET_PTP			1
#define MMU_ET_PTE			2

#define PG_V				MMU_ET_PTE

/* Definitions for AccessPermissions */

#define MMU_STD_SRUR		0
#define MMU_STD_SRWURW		1
#define MMU_STD_SRXURX		2
#define MMU_STD_SRWXURWX	3
#define MMU_STD_SXUX		4
#define MMU_STD_SRWUR		5
#define MMU_STD_SRX			6
#define MMU_STD_SRWX		7

#define MAKE_PROT(v)	PTE_PERMS(v)
#define PG_PROT			MAKE_PROT(0x7)
#define PG_KW			MAKE_PROT(MMU_STD_SRWX)
#define PG_KR			MAKE_PROT(MMU_STD_SRX)
#define PG_UW			MAKE_PROT(MMU_STD_SRWXURWX)
#define PG_URKR			MAKE_PROT(MMU_STD_SRUR)
#define PG_UR			MAKE_PROT(MMU_STD_SRUR)
#define PG_UPAGE		MAKE_PROT(MMU_SRWUR)

#define MMU_STD_PAGEMASK	0xFFFFFF000
#define MMU_STD_PAGESHIFT	12 /* only maps 35:12 */
#define MMU_STD_PTPSHIFT	6 /* Pointer is 35:6 */
#define MMU_STD_PAGESIZE	(1 << MMU_STD_PAGESHIFT)

#define MMU_STD_SEGSHIFT	18
#define MMU_STD_RGNSHIFT	24

#define MMU_STD_ROOTMASK	0xFFFFF /* after pageshift */
#define MMU_STD_ROOTSHIFT	20
#define MMU_STD_FIRSTMASK	0xFFF
#define MMU_STD_FIRSTSHIFT	12
#define MMU_STD_SECONDMASK	0x3F
#define MMU_STD_SECONDSHIFT	6
#define MMU_STD_THIRDMASK	0x0
#define MMU_STD_THIRDSHIFT	0

#define MMU_STD_THIRDTOP	0xFFF
#define MMU_STD_SECONDTOP	0x3FFFF
#define MMU_STD_FIRSTTOP	0xFFFFFF

#define	MMU_ROOT_XTR(a)		(((a) & 0xFFFFF000) >> MMU_STD_PAGESHIFT)
#define MMU_FIRST_XTR(a)	(((a)&0x00FFF000) >> MMU_STD_PAGESHIFT)
#define MMU_SECOND_XTR(a)	(((a)&0x0003F000) >> MMU_STD_PAGESHIFT)

#define	MMU_L1_INDEX(a)	(((a) & 0xFF000000) >> \
	(MMU_STD_PAGESHIFT + MMU_STD_FIRSTSHIFT))
#define MMU_L2_INDEX(a)	(((a)&0x00FC0000) >>  \
	(MMU_STD_PAGESHIFT + MMU_STD_SECONDSHIFT))
#define MMU_L3_INDEX(a)	(((a)&0x0003F000) >>  \
	(MMU_STD_PAGESHIFT + MMU_STD_THIRDSHIFT))

#ifndef LOCORE
extern struct pte mmu_pteinvalid;
#endif
#define MMU_STD_INVALIDPTP	(0)	/* 0 entry type */
#define MMU_STD_INVALIDPTE	(0)	/* 0 entry type */

/* For vac.s */
#define SEGMENT_SHIFT (MMU_STD_SECONDSHIFT+MMU_STD_PAGESHIFT)

#ifndef LOCORE
/*
 * generate a ptp referencing a table starting at the specified
 * physical address.
 */
#define	PTPOF(pa)	(((((unsigned)(pa))>>MMU_STD_PTPSHIFT)<<2)|MMU_ET_PTP)

struct ptp {
	u_int PageTablePointer:30;
	u_char EntryType:2; /* Checked for validity */
};

union ptpe {
	struct pte pte;
	struct ptp ptp;
	u_int ptpe_int;
};

#endif !(LOCORE)

#define MMU_NPTE_ONE	256		/* 256 PTEs in first level table */
#define MMU_NPTE_TWO	64	
#define MMU_NPTE_THREE	64	

/* For backwards compatibility */
#define MAKE_PGT(v)     ((v) << 20)
#define PGT_OBMEM       MAKE_PGT(0x0)       /* onboard memory */
#define PGT_OBIO        MAKE_PGT(0xF)       /* onboard I/O */
#define PGT_VME_D16     MAKE_PGT(0xC)       /* VMEbus 16-bit data */
#define PGT_VME_D32     MAKE_PGT(0xD)       /* VMEbus 32-bit data */


/*
 * Macros so we can look at page numbers and tell what is on
 * the other end. The function "bustype" is in autoconf.c where
 * it is allowed to know the physical memory layout of the machine,
 * which we SHOULD be getting from the devinfo tree.
 */
#define	BT_UNKNOWN	1	/* not a defined map area */
#define	BT_OBMEM	2	/* system memory */
#define	BT_OBIO		3	/* on-board devices */
#define	BT_VIDEO	4	/* onboard video */
#define	BT_SBUS		5	/* S-Bus */
#define	BT_VME		6	/* VME Bus */

#if !defined(LOCORE) && defined(KERNEL)
/* utilities from autoconf.c */
extern int	bustype();
/* utilities defined in locore.s */
extern	struct pte Heapptes[];
extern	char Heapbase[];
extern	char Heaplimit[];
extern	struct pte Bufptes[];
extern	char Bufbase[];
extern	char Buflimit[];
extern  struct pte Sysmap[];
extern  char Sysbase[];
extern  struct pte mmap[];
extern  struct pte CMAP1[];
extern  char CADDR1[];
extern  struct pte CMAP2[];
extern  char CADDR2[];
extern  char Syslimit[];


/*
 * These defines are just here to be an aid
 * for someone that was using these defines.
 */
#define Usrptmap        Sysmap
#define usrpt           Sysbase
#endif !defined(LOCORE) && defined(KERNEL)

#endif /*!_sun4m_pte_h*/
