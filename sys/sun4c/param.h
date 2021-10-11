/*	@(#)param.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 *
 * This file is intended to contain the basic
 * specific details of a given architecture.
 */

#ifndef _sun4c_param_h
#define	_sun4c_param_h

/*
 * Machine dependent constants for Sun4c.
 */

/*
 * The following symbols are required for the Sun4c architecture.
 *	OPENPROMS	# Open Boot Prom interface
 *	MEMSEG		# uses 4.1 style memory segments
 *	NEWDUMP		# uses sparse crash dumping scheme
 *	SAIO_COMPAT	# OBP uses "bootparam" interface
 */
#define	OPENPROMS
#define	MEMSEG
#define	NEWDUMP
#define	SAIO_COMPAT

#ifdef MULTIPROCESSOR
#define NCPU 4
#else
#define NCPU 1
#endif


/*
 * All the Sun-4c machines have Virtual Address Cache.
 * So we define VAC here.
 */
#define	VAC

/*
 * Define the FPU symbol if we could run on a machine with an external
 * FPU (i.e. not integrated with the normal machine state like the vax).
 */
#define	FPU

/*
 * Define the MMU_3LEVEL symbol if we could run on a machine with
 * a three level mmu.   We also assume these machines have region
 * and user cache flush operations.
 */

/* none at present */

/*
 * Define IOC if we could run on machines that have an I/O cache.
 */

/* none at present */

/*
 * Define BCOPY_BUF if we could run on machines that have a bcopy buffer.
 */

/* none at present */

/*
 * All the Sun-4c machines have a hole in the virtual address space.
 * So we define VA_HOLE here.
 */

#define	VA_HOLE

/*
 * MMU_PAGES* describes the physical page size used by the mapping hardware.
 * PAGES* describes the logical page size used by the system.
 */

#define	MMU_PAGESIZE	0x1000		/* 4096 bytes */
#define	MMU_PAGESHIFT	12		/* log2(MMU_PAGESIZE) */
#define	MMU_PAGEOFFSET	(MMU_PAGESIZE-1) /* Mask of address bits in page */
#define	MMU_PAGEMASK	(~MMU_PAGEOFFSET)

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
#define	DELAY(n)	usec_delay(n)
#define	CDELAY(c, n)	\
{ \
	register int N = n; \
	while (--N > 0) { \
		if (c) \
			break; \
		usec_delay(1); \
	} \
}

/*
 * Size of u-area and kernel stack in u-area.
 */

#define	UPAGES		4	/* pages of u-area, NOT including red zone */
#define	KERNSTACK	0x3000	/* size of kernel stack in u-area */

/*
 * KERNSIZE the amount of vitual address space the kernel
 * uses in all contexts.
 */
#define	KERNELSIZE	(128*1024*1024)

/*
 * KERNELBASE is the virtual address which
 * the kernel text/data mapping starts in all contexts.
 */
#define	KERNELBASE	(0-KERNELSIZE)

/*
 * SYSBASE is the virtual address which
 * the kernel allocated memory mapping starts in all contexts.
 */
#define	SYSBASE		(0-(16*1024*1024))

/*
 * MBPOOLBYTES gives the amount of virtual space to reserve for mbuf
 * storage.
 */
#define	MBPOOLBYTES	(2*1024*1024)
#define MBPOOLMMUPAGES	(MBPOOLBYTES / MMU_PAGESIZE)

/*
 * GX_BASE - GX_LIMIT: Maximal range that PROM
 * may need for frame buffers.
 * XXX This should not be hard coded (1060915)
 */

#define GX_LIMIT	SYSBASE
#define GX_BASE	(SYSBASE - PROMAUXMAPSIZE)

/*
 * BUFBYTES: Maximal size of Disk Buffer Cache
 * BUFBASE - BUFLIMIT: Disk Buffer Cache
 */
#define BUFBYTES	(2*1024*1024)
#define BUFPAGES	(BUFBYTES / MMU_PAGESIZE)
#define BUFLIMIT	GX_BASE
#define BUFBASE		(BUFLIMIT - BUFBYTES)

/*
 * HEAPBYTES: Maximal size of Kernel Heap
 * HEAPBASE - HEAPLIMIT: Kernel Heap
 */
#define HEAPBYTES	(16*1024*1024)
#define HEAPPAGES	(HEAPBYTES / MMU_PAGESIZE)
#define HEAPLIMIT	BUFBASE
#define HEAPBASE	(HEAPLIMIT - HEAPBYTES)

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

#endif	/* !_sun4c_param_h */
