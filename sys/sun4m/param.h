/*	@(#)param.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 *
 * This file is intended to contain the basic
 * specific details of a given architecture.
 */

#ifndef _sun4m_param_h
#define	_sun4m_param_h

/*
 * Machine dependent constants for Sparc running SunOS 4.1 with
 * the Sparc Reference MMU.
 */

/*
 * The following symbols are required for the Sun4c architecture.
 *	OPENPROMS	# Open Boot Prom interface
 *	NEWDUMP		# uses sparse crash dumping scheme
 *	SAIO_COMPAT	# OBP uses "bootparam" interface
 */
#define	OPENPROMS
#define	NEWDUMP
#define	SAIO_COMPAT

#if !defined(MULTIPROCESSOR) && !defined(UNIPROCESSOR)
/*
 * If neither MULTIPROCESSOR nor UNIPROCESSOR is specified,
 * default to MULTIPROCESSOR. This should do the right thing
 * for users including this file as <sys/param.h>. Care
 * should be taken that the layout of system information does
 * not change between UNI and MULTIprocessor kernels.
 */
#define	MULTIPROCESSOR
#endif

#ifdef MULTIPROCESSOR
#define	NCPU 4
#else
#define	NCPU 1
#endif

/*
 * Define the VAC symbol if we could run on a machine
 * which has a Virtual Address Cache
 *
 * Eventually, this should be turned on if we are
 * supporting any Mbus modules that have a VAC.
 *
 * IOMMU and IOC are attributes of the system,
 * not the module; anyway, logic similar to the
 * above should be used.
 */
#define	IOMMU			/* support sun-4m IOMMU */
#define	VAC			/* support virtual addressed caches */
#ifdef SUN4M_690
#define	IOC			/* support sun-4m IOCache - VME-only */
#define	VME			/* support vme devices */
#endif

/*
 * Define the FPU symbol if we could run on a machine with an external
 * FPU (i.e. not integrated with the normal machine state like the vax).
 */
#define	FPU

/*
 * MMU_PAGES* describes the physical page size used by the mapping hardware.
 * PAGES* describes the logical page size used by the system.
 */

#define	MMU_PAGESIZE	0x1000		/* 4096 bytes */
#define	MMU_PAGESHIFT	12		/* log2(MMU_PAGESIZE) */
#define	MMU_PAGEOFFSET	(MMU_PAGESIZE-1) /* Mask of address bits in page */
#define	MMU_PAGEMASK	(~(MMU_PAGEOFFSET))

#define	MMU_SEGSIZE	0x40000		/* 256 K bytes */
#define	MMU_SEGSHIFT	18
#define	MMU_SEGOFFSET	(MMU_SEGSIZE - 1)
#define	MMU_SEGMASK	(~(MMUPAGEOFFSET))

#define	PAGESIZE	0x1000		/* All of the above, for logical */
#define	PAGESHIFT	12
#define	PAGEOFFSET	(PAGESIZE - 1)
#define	PAGEMASK	(~PAGEOFFSET)

/*
 * DATA_ALIGN is used to define the alignment of the Unix data segment.
 * We leave this 8K so we are Sun4 binary compatible.
 */
#define	DATA_ALIGN	0x2000

/*
 * Some random macros for units conversion.
 */

/*
 * MMU pages to bytes, and back (with and without rounding)
 */
#define	mmu_ptob(x)	((x) << MMU_PAGESHIFT)
#define	mmu_btop(x)	(((unsigned)(x)) >> MMU_PAGESHIFT)
#define	mmu_btopr(x)	((((unsigned)(x) + MMU_PAGEOFFSET) >> MMU_PAGESHIFT))

/*
 * pages to bytes, and back (with and without rounding)
 */
#define	ptob(x)		((x) << PAGESHIFT)
#define	btop(x)		(((unsigned)(x)) >> PAGESHIFT)
#define	btopr(x)	((((unsigned)(x) + PAGEOFFSET) >> PAGESHIFT))

/*
 * 2 versions of pages to disk blocks
 */
#define	mmu_ptod(x)	((x) << (MMU_PAGESHIFT - DEV_BSHIFT))
#define	ptod(x)		((x) << (PAGESHIFT - DEV_BSHIFT))

/*
 * Delay units are in microseconds.
 */
#define	DELAY(n)	(void) usec_delay((unsigned) n)
#define	CDELAY(c, n)	\
{ \
	register int N = n; \
	while (--N > 0) { \
		if (c) \
			break; \
		(void) usec_delay(1); \
	} \
}

/*
 * Size of u-area and kernel stack in u-area.
 */

#define	UPAGES		4	/* pages of u-area, NOT including red zone */
#define	KERNSTACK	0x3000	/* size of kernel stack in u-area */
#define	INTSTACKSIZE	0x3000	/* size of interrupt stack */

/*
 * KERNSIZE the amount of vitual address space the kernel
 * uses in all contexts.
 */
#define	KERNELSIZE	(256*1024*1024)

/*
 * KERNELBASE is the virtual address which
 * the kernel text/data mapping starts in all contexts.
 */
/* XXX - #define	KERNELBASE	(0-KERNELSIZE) */
#define	KERNELBASE	0xF0000000 /* start of first kernel segment */

/*
 * SYSBASE is the virtual address which
 * the kernel allocated memory mapping starts in all contexts.
 */
/* note that 0xFF000000 == (0-(16*1024*1024)) */
#define	SYSBASE		0xFF000000 /* start of second kernel segment */
#define	NKL2PTS		((0 - KERNELBASE) / L2PTSIZE)
#define	NKL3PTS		((0 - KERNELBASE) / L3PTSIZE)
#define	NKTMPPTPPGS	(mmu_btopr(NKL3PTS * sizeof (struct l3pt)))

#ifdef	NOPROM
#define	TOPOFTMPPTES	((union ptpe *)0xF8500000)
#define	TMPPTES		(TOPOFTMPPTES - NL3PTEPERPT*NKL3PTS - 2048)
#else NOPROM
	/* TMPPTES is now the size of mem for us to map for tmpptes */
#define	TMPPTES	((u_int)((sizeof (union ptpe *))*(NL3PTEPERPT*NKL3PTS + 2048)))
	/* This puts the tmpptes just below the heap */
#define	VA_TMPPTES	(HEAPBASE - TMPPTES)
	/* Space from mapping phys memory */
#endif NOPROM

/*
 * OBMEM  is used for prom related stuff only
 * maybe it should be in a promif header file.
 * sun4c has it in pte.h.
 */
#define	OBMEM	((u_int)0x0)

/*
 * MBPOOLBYTES gives the amount of virtual space to reserve for mbuf
 * storage.
 */
#define	MBPOOLBYTES	(2*1024*1024)
#define	MBPOOLMMUPAGES	(MBPOOLBYTES / MMU_PAGESIZE)

#include <machine/devaddr.h>	/* for VA_PERCPUME */
/*
 * BUFBYTES: Maximal size of Disk Buffer Cache
 * BUFBASE - BUFLIMIT: Disk Buffer Cache
 */
#define	BUFBYTES	(4*1024*1024)
#define	BUFPAGES	(BUFBYTES / MMU_PAGESIZE)
#define	BUFLIMIT	VA_PERCPUME
#define	BUFBASE		(BUFLIMIT - BUFBYTES)

/*
 * HEAPBYTES: Maximal size of Kernel Heap
 * HEAPBASE - HEAPLIMIT: Kernel Heap
 */
#define	HEAPBYTES	(28*1024*1024)
#define	HEAPPAGES	(HEAPBYTES / MMU_PAGESIZE)
#define	HEAPLIMIT	BUFBASE
#define	HEAPBASE	(HEAPLIMIT - HEAPBYTES)

/*
 * Msgbuf size.
 */
#define	MSG_BSIZE	((8 * 1024) - sizeof (struct msgbuf_hd))

/*
 * XXX - Macros for compatibility
 */
/* Clicks (MMU PAGES) to disk blocks */
#define	ctod(x)		mmu_ptod(x)

/* Clicks (MMU PAGES) to bytes, and back (with rounding) */
#define	ctob(x)		mmu_ptob(x)
#define	btoc(x)		mmu_btopr(x)

/*
 * XXX - Old names for some backwards compatibility
 */
#define	NBPG		MMU_PAGESIZE
#define	PGOFSET		MMU_PAGEOFFSET
#define	PGSHIFT		MMU_PAGESHIFT

#define	CLSIZE		1
#define	CLSIZELOG2	0
#define	CLBYTES		PAGESIZE
#define	CLOFSET		PAGEOFFSET
#define	CLSHIFT		PAGESHIFT
#define	clrnd(i)	(i)

#endif	/* !_sun4m_param_h */
