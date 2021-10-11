/*	@(#)mmu.h 1.1 92/07/30 SMI	*/

/*
 * Copyright 1988-1989 Sun Microsystems, Inc.
 */

#ifndef _sun4c_mmu_h
#define	_sun4c_mmu_h

/*
 * Sun-4c memory management unit.
 * All sun-4c implementations use 32 bits of address.
 * A particular implementation may implement a smaller MMU.
 * If so, the missing addresses are in the "middle" of the
 * 32 bit address space. All accesses in this range behave
 * as if there was an invalid page map entry correspronding
 * to the address.
 */

/*
 * Hardware context and segment information
 * Mnemonic decoding:
 *	PMENT - Page Map ENTry
 *	PMGRP - Group of PMENTs (aka "segment")
 */
/* fixed SUN4C constants */
#define	NPMENTPERPMGRP		64
#define	NPMENTPERPMGRPSHIFT	6	/* log2(NPMENTPERPMGRP) */
#define	NPMGRPPERCTX		4096
#define	PMGRPSIZE	(NPMENTPERPMGRP * PAGESIZE)
#define	PMGRPOFFSET	(PMGRPSIZE - 1)
#define	PMGRPSHIFT	(PAGESHIFT + NPMENTPERPMGRPSHIFT)
#define	PMGRPMASK	(~PMGRPOFFSET)

/* variable (per implementation) */

/* SUN4C_60 -- not in boot PROM */
#define	NCTXS_60		8
#define	NPMGRPS_60		128
#define	VAC_SIZE_60		65536
#define	VAC_LINESIZE_60		16

/*
 * max mips at Mhz rates
 */
#define	CPU_MAXMIPS_20MHZ	20
#define	CPU_MAXMIPS_25MHZ	25

/*
 * Useful defines for hat constants
 */
#define	NCTXS		nctxs
#define	NPMGRPS		npmgrps

/*
 * Variables set at boot time to reflect cpu type.
 */
#ifndef LOCORE
extern u_int nctxs;		/* number of implemented contexts */
extern u_int npmgrps;		/* number of pmgrps in page map */
extern addr_t hole_start;	/* addr of start of MMU "hole" */
extern addr_t hole_end;		/* addr of end of MMU "hole" */
extern u_int shm_alignment;	/* VAC address consistency modulus */

#define	PMGRP_INVALID (NPMGRPS - 1)

/*
 * Macro to determine whether an address is within the range of the MMU.
 */
#define	good_addr(a) \
	((addr_t)(a) < hole_start || (addr_t)(a) >= hole_end)
#endif !LOCORE

/*
 * Address space identifiers.
 */
#define	ASI_CTL		0x2	/* control space */
#define	ASI_SM		0x3	/* segment map */
#define	ASI_PM		0x4	/* page map */
#define	ASI_FCS_HW	0x5	/* flush cache segment (HW assisted) */
#define	ASI_FCP_HW	0x6	/* flush cache page (HW assisted) */
#define	ASI_FCC_HW	0x7	/* flush cache context (HW assisted) */
#define	ASI_UP		0x8	/* user program */
#define	ASI_SP		0x9	/* supervisor program */
#define	ASI_UD		0xA	/* user data */
#define	ASI_SD		0xB	/* supervisor data */
#define	ASI_FCS		0xC	/* flush cache segment */
#define	ASI_FCP		0xD	/* flush cache page */
#define	ASI_FCC		0xE	/* flush cache context */
#define	ASI_FCU_HW	0xF	/* flush cache unconditional (HW assisted) */

/*
 * ASI_CTL addresses
 */

#define	CONTEXT_REG		0x30000000
#define	SYSTEM_ENABLE		0x40000000
#define	SYNC_ERROR_REG		0x60000000
#define	SYNC_VA_REG		0x60000004
#define	ASYNC_ERROR_REG		0x60000008
#define	ASYNC_VA_REG		0x6000000C
#define	ASYNC_DATA1_REG		0x60000010	/* not avail. on 4/60 */
#define	ASYNC_DATA2_REG		0x60000014	/* not avail. on 4/60 */
#define	CACHE_TAGS		0x80000000
#define	CACHE_DATA		0x90000000
#define	UART_BYPASS		0xF0000000

/*
 * Various I/O space related constants
 */
#define	SBUS_BASE	0xf8000000	/* address of Sbus slots in OBIO */
#define	SBUS_SIZE	0x02000000	/* size of each slot */
#define	SBUS_SLOTS	4		/* number of slots */

/*
 * The usable DVMA space size.	 XXXXXXXXXXX - this is not real!!!
 */
#define	DVMASIZE	((1024*1024)-PMGRPSIZE)
#define	DVMABASE	(0-(1024*1024))

/*
 * Context for kernel. On a Sun-4c the kernel is in every address space,
 * but KCONTEXT is magic in that there is never any user context there.
 */
#define	KCONTEXT	0

/*
 * SEGTEMP & SEGTEMP2 are virtual segments reserved for temporary operations.
 * We use the segments immediately before the start of debugger area.
 */
#include <debug/debug.h>
#define	SEGTEMP		((addr_t)(DEBUGSTART - (2 * PMGRPSIZE)))
#define	SEGTEMP2	((addr_t)(DEBUGSTART - PMGRPSIZE))

#if defined(KERNEL) && !defined(LOCORE)
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
 * Cache specific routines and variables.
 */
extern int vac_size;		/* cache size, bytes */
extern int vac_linesize;	/* line size, bytes */
extern int vac_hwflush;		/* VAC has hardware flush */

void	vac_control();
void	vac_flushall();
void	vac_ctxflush();
void	vac_segflush(/* base */);
void	vac_pageflush(/* base */);
void	vac_flush(/* base, len */);

int	valid_va_range(/* basep, lenp, minlen, dir */);

#endif defined(KERNEL) && !defined(LOCORE)

#endif /* !_sun4c_mmu_h */
