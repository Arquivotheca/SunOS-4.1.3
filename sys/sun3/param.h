/*      @(#)param.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 *
 * This file is intended to contain the basic
 * specific details of a given architecture.
 */

/*
 * Machine dependent constants for Sun-3.
 */

#ifndef _sun3_param_h
#define _sun3_param_h

/*
 * MMU_PAGES* describes the physical page size used by the mapping hardware.
 * PAGES* describes the logical page size used by the system.
 */

#define	MMU_PAGESIZE	0x2000		/* 8192 bytes */
#define	MMU_PAGESHIFT	13		/* log2(MMU_PAGESIZE) */
#define	MMU_PAGEOFFSET	(MMU_PAGESIZE-1)/* Mask of address bits in page */
#define	MMU_PAGEMASK	(~MMU_PAGEOFFSET)

#define	PAGESIZE	0x2000		/* All of the above, for logical */
#define	PAGESHIFT	13
#define	PAGEOFFSET	(PAGESIZE - 1)
#define	PAGEMASK	(~PAGEOFFSET)


/*
 * DATA_ALIGN is used to define the alignment of the Unix data segment.
 * Unfortunately it was defined using the hardware segment size.
 */
#define	DATA_ALIGN	0x20000

/*
 * Msgbuf size.
 * Must be less than PAGESIZE - (sizeof(struct scb) + sizeof(struct msgbuf) + 1)
 */
#define MSG_BSIZE	((6 * 1024) - sizeof (struct msgbuf_hd))

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
 * Macros to decode processor status word.
 */
#define	USERMODE(ps)	(((ps) & SR_SMODE) == 0)
#define	BASEPRI(ps)	(((ps) & SR_INTPRI) == 0)

/*
 * Delay units are in microseconds.
 */
#define	DELAY(n)	\
{ \
	extern int cpudelay; \
	register int N = (((n)<<4) >> cpudelay); \
 \
	while (--N > 0) ; \
}

#define	CDELAY(c, n)	\
{ \
	extern int cpudelay; \
	register int N = (((n)<<3) >> cpudelay); \
 \
	while (--N > 0) \
		if (c) \
			break; \
}

/*
 * Define the VAC symbol if we could run on a machine
 * which has a Virtual Address Cache (e.g. SUN3_260).
 * Define the BCOPY symbol if we could run on a machine
 * which has bcopy hardware assist (e.g. SUN3_260).
 */
#ifdef SUN3_260
#define VAC
#define	BCOPY
#else
#undef VAC
#undef BCOPY
#endif SUN3_260

/*
 * The Virtual Address Cache in Sun-3 requires aliasing addresses be
 * matched in modulo 128K (0x20000) to guarantee data consistency.
 */
#define shm_alignment	((cpu == CPU_SUN3_260) ? 0x20000 : PAGESIZE)

#define	UPAGES		1		/* pages of u-area, w/o red zone */

#define	KERNSTACK	0x1800		/* size of kernel stack in u-area */

/*
 * KERNELBASE is the virtual address which
 * the kernel mapping starts in all contexts.
 */
#define	KERNELBASE	0x0E000000

/*
 * SYSBASE is the virtual address which
 * the kernel mapping starts in all contexts.
 * Added for compatibility with the Sun-4 case.
 */
#define	SYSBASE		0x0F000000

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

#endif /*!_sun3_param_h*/
