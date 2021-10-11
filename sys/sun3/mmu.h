/*      @(#)mmu.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef _sun3_mmu_h
#define _sun3_mmu_h

/* 
 * Sun-3 memory management.
 *
 * This file gives more mmu specific constants and defines
 * the interface to some pretty low level mmu routines.
 */

/*
 * Process Address space is 28 bits
 */
#define	ADDRESS_MASK	0x0fffffff	/* Mask of valid bits in an address */

/*
 * Hardware context and segment information
 * Mnemonic decoding:
 *	PMENT - Page Map ENTry
 *	PMGRP - Group of PMENTs (aka "segment")
 */
#define	MNCTXS			8	/* Number of mapping contexts */
#define	NPMGRPPERCTX		0x800	/* 2048 */
#define	MNPMGRPS		0x100	/* 256 */
#define	NPMENTPERPMGRP		0x10	/* 16 */
#define	NPMENTPERPMGRPSHIFT	4	/* log2(NPMENTPERPMGRP) */

/*
 * Useful defines for hat constants
 */
#define NCTXS           MNCTXS
#define NPMGRPS         MNPMGRPS
/*
 * More useful manifest constants
 */
#define	TOTALPMGRP		(NCTXS * NPMGRPPERCTX)
#define	PMGRPSIZE		(NPMENTPERPMGRP * PAGESIZE)
#define	MNPMENTS		(NPMGRPS * NPMENTPERPMGRP)
#define	PMGRP_INVALID		(NPMGRPS - 1)
#define	CTXSIZE			(PMGRPSIZE * NPMGRPPERCTX)
#define	CTXMASK			(NCTXS - 1)
#define	PMGRPOFFSET		(PMGRPSIZE - 1)
#define	PMGRPMASK		(~PMGRPOFFSET)
#define	PMGRPSHIFT		(PAGESHIFT + NPMENTPERPMGRPSHIFT)

/*
 * XXX - Old names for some backwards compatibility
 */
#define	NBSG		PMGRPSIZE
#define	SGOFSET		PMGRPOFFSET
#define	SGSHIFT		PMGRPSHIFT

/*
 * 680x0 code register values.
 */
#define	FC_UD	1		/* user data */
#define	FC_UP	2		/* user program */
#define	FC_MAP	3		/* Sun-3 memory maps */
#define	FC_SD	5		/* supervisor data */
#define	FC_SP	6		/* supervisor program */
#define	FC_CPU	7		/* cpu space */

/*
 * FC_MAP base offsets, see architecture manual for definitions
 */
#define	ID_PROM			0x00000000
#define	PAGE_MAP		0x10000000
#define	SEGMENT_MAP		0x20000000
#define	CONTEXT_REG		0x30000000
#define	SYSTEM_ENABLE		0x40000000
#define	USER_DVMA_ENABLE 	0x50000000
#define	BUS_ERROR_REG		0x60000000
#define	DIAGNOSTIC_REG		0x70000000
#define	CACHE_TAGS		0x80000000
#define	CACHE_DATA		0x90000000
#define	FLUSH_CACHE		0xa0000000
#define	BLOCK_COPY		0xb0000000
						/* Gap: 0xc, d, e */
#define	UART_BYPASS		0xf0000000

#define	IDPROMSIZE		0x20		/* size of id prom in bytes */

/*
 * We have to define the control mask to strip off the low 3 order
 * bits of the address maks in order to get long word aligned addresses.
 * This little tid bit was left out of the Sun-3 Architecture Manual.
 */
#define	CONTROL_MASK		0x0ffffffc

/*
 * 68020 Cache Control Register
 */
#define CACHE_ENABLE	0x1	/* enable the cache */
#define CACHE_FREEZE	0x2	/* freeze the cache */
#define CACHE_CLRENTRY	0x4	/* clear entry specified by cache addr reg */
#define CACHE_CLEAR	0x8	/* clear entire cache */

/*
 * Sun-3 Virtual Address Cache (VAC)
 * The VAC h/w has a counter cycles bits 4 through bit 8.  We cycle
 * bit 9 through bit 15 for 64KB VAC.  For 128KB VAC, we cycle bit 9
 * through bit 16.  (There is only one size of VAC for Sun-3 260.
 * If there is more than one VAC size, VAC_FLUSH_HIGHBIT should be
 * a variable whose value is set at boot time.)
 */
#define VAC_FLUSH_BASE		0xa0000000
#define VAC_FLUSH_LOWBIT	9
#define VAC_FLUSH_HIGHBIT	15	/* 64KB VAC */
#define VAC_FLUSH_INCRMNT	512	/* 2 << VAC_LOW_BIT */
#define VAC_UNIT_ADDRBITS	0x0ffffe00
#define VAC_UNIT_MASK		0x000001ff

/*
 * Number of cycles to flush a context, segment, and page.
 * For a context flush or segment flush, we loop
 * 2 << (VAC_HIGH_BIT - VAC_LOW_BIT + 1) times, and for a page flush,
 * we loop 2 << (PHSHIFT - VAC_LOW_BIT) times.  
 */
#define VAC_CTXFLUSH_COUNT	128
#define VAC_SEGFLUSH_COUNT	128
#define VAC_PAGEFLUSH_COUNT	16

/* contants to do a flush */
/* VAC_FLUSHALL is used for debugging only for SUN3_260 */
#define VAC_FLUSHALL		0
#define VAC_CTXFLUSH		0x01
#define VAC_SEGFLUSH		0x03
#define VAC_PAGEFLUSH		0x02

/*
 * VAC Read/Write Cache Tags.
 * R/W tag is for one line in the cache, which has 16 bytes.
 * VAC_RWTAG_COUNT is 2 ^ (VAC_RWTAG_HIGHBIT - VAC_RWTAG_LOWBIT + 1) - 1
 */
#define VAC_RWTAG_BASE		0x80000000
#define VAC_RWTAG_LOWBIT	4
#define VAC_RWTAG_HIGHBIT	15	/* 64KB VAC */
#define VAC_RWTAG_INCRMNT	16	/* 2 ^ VAC_RWTAG_LOWBIT */
#define VAC_RWTAG_COUNT		4096

/*
 * Block Copy Read and Block Copy Write
 */
#define	BC_LINESIZE		16
#define	BC_LINE_SHIFT		2
#define	BC_LINE_RESIDU		3
#define BC_BLOCK_CPCMD		0xb0000000
#define BC_BLOCK_OFF		0x0fffffff

/*
 * Masks for base addresses when doing VAC flush operations
 */
#define	VAC_PAGEMASK		0x0fffe000
#define	VAC_SEGMASK		0x0ffe0000

/*
 * Context for kernel.  On a Sun-3 the kernel is in every address space,
 * but KCONTEXT is magic in that there is never any user context there.
 */
#define	KCONTEXT	0

#if defined(KERNEL) && !defined(LOCORE)

#include <sys/types.h>
/*
 * Low level mmu-specific functions 
 */
struct	ctx *mmu_getctx();
void	mmu_setctx(/* ctx */);
void	mmu_setpmg(/* base, pmg */);
void	mmu_settpmg(/* base, pmg */);
struct	pmgrp *mmu_getpmg(/* base */);
void	mmu_setpte(/* base, pte */);
void	mmu_getpte(/* base, ppte */);
void	mmu_getkpte(/* base, ppte */);
void	mmu_pmginval(/* pmg */);

/*
 * Cache specific routines - ifdef'ed out if there is no chance
 * of running on a machine with a virtual address cache.
 */
#ifdef VAC
void	vac_init();
void	vac_flushall();
void	vac_ctxflush();
void	vac_segflush(/* base */);
void	vac_pageflush(/* base */);
void	vac_flush(/* base, len */);
int	bp_alloc(/* map, bp, size */);
#else VAC
#define	vac_init()
#define	vac_flushall()
#define	vac_ctxflush()
#define	vac_segflush(base)
#define	vac_pageflush(base)
#define	vac_flush(base, len)
#define bp_alloc(map, bp, size)		(int)rmalloc((map), (long)(size))
#endif VAC

void	sun_setcontrol_long(/* space, addr */);
u_int	sun_getcontrol_long(/* space, addr, value */);
void	sun_setcontrol_byte(/* space, addr */);
u_int	sun_getcontrol_byte(/* space, addr, value */);
int	valid_va_range(/* basep, lenp, minlen, dir */);

#endif defined(KERNEL) && !defined(LOCORE)

#endif /*!_sun3_mmu_h*/
