/*       @(#)param.h 1.1 92/07/30 SMI    */
/* syncd to sun3/param.h 1.19 */

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * This file contains basic details of the architecture.
 */

#ifndef _sun3x_param_h
#define _sun3x_param_h

#define	MMU_PAGESIZE	0x2000			/* 8192 bytes */
#define	MMU_PAGESHIFT	13			/* log2(MMU_PAGESIZE) */
#define MMU_PAGEOFFSET 	(MMU_PAGESIZE - 1)	/* address bits in page */
#define	MMU_PAGEMASK	(~MMU_PAGEOFFSET)

#define	PAGESIZE	0x2000			/* all of above, for logical */
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
#define	MSG_BSIZE	((6 * 1024) - sizeof (struct msgbuf_hd))

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
 * Define the BCOPY symbol if we run on a machine that has bcopy hardware.
 * Also define the IOC symbol if appropriate.
 */
#ifdef SUN3X_470
#define	BCOPY
#define IOC
#else
#undef BCOPY
#undef IOC
#endif SUN3X_470

/*
 * Software enable register - bits enable cpu/io cache functions
 */

#define S_CPUCACHE	0x1	/* cpu cache */
#define S_BCOPY		0x2	/* bcopy hardware (turns on bcenable) */
#define S_IOC_VME	0x4	/* io cache vme accesses */
#define S_IOC_XMIT	0x8	/* io cache ethernet transmit buffers */
#define S_IOC_RECV	0x10	/* io cache ethernet receive buffers */
#define S_IOC_NET	(S_IOC_XMIT + S_IOC_RECV)	/* 0x18 */
#define S_IOCACHE	(S_IOC_NET + S_IOC_VME)		/* 0x1c */
#define S_ECC		0x20	/* turn on ecc */
#define S_DBGCACHE	0x40	/* cache debug mode */

/*
 * Define UADDR as a 32 bit address so that the compiler will
 * generate short absolute references to access the u-area.
 */
#define	UADDR		(0-2*NBPG)	/* u-area virt addr - 2nd page down */
#define	UPAGES		1		/* pages of u-area, w/o red zone */

#define	KERNSTACK	0x1400		/* size of kernel stack in u-area */

/*
 * Location of kernel in every context and some related constants.
 */
#define	KERNELBASE	0xF8000000	/* start of first kernel segment */
#define	KERNEL_LIMIT	0xFED80000	/* max valloc'd address */
#define	SYSBASE		0xFF000000	/* start of second kernel segment */
#define	NKL2PTS		((0 - KERNELBASE) / L2PTSIZE)
#define	NKL3PTS		((0 - KERNELBASE) / L3PTSIZE)
#define	NKTMPPTPGS	(mmu_btopr(NKL3PTS * sizeof (struct l3pt)))

#ifdef SUN3X_470
#define	PCACHETAGS_SIZE		0x10000		/* size of cache tags */
#endif SUN3X_470

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

#endif /*!_sun3x_param_h*/
