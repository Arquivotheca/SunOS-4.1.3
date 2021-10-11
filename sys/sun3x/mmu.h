/*       @(#)mmu.h 1.1 92/07/30 SMI    */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun-3x memory management.
 * This file gives more mmu specific constants.
 */


#ifndef _sun3x_mmu_h
#define _sun3x_mmu_h

#include <machine/devaddr.h>
#include <machine/iocache.h>	/* for 4.1 driver source compatibility */

/*
 * Process address space is 32 bits
 */
#define	ADDRESS_MASK	0xffffffff	/* Mask of valid bits of an address */

/*
 * Page table information.
 */
#define	MMU_L3_SHIFT	MMU_PAGESHIFT		/* level 3 table shift */
#define	MMU_L3_MASK	0x0007e000		/* address bits in table */
#define	MMU_L2_SHIFT	(MMU_L3_SHIFT + 6)	/* level 2 table shift */
#define	MMU_L2_MASK	0x01f80000		/* address bits in table */
#define	MMU_L1_SHIFT	(MMU_L2_SHIFT + 6)	/* level 1 table shift */
#define	MMU_L1_MASK	0xfe000000		/* address bits in table */

#define NL3PTEPERPT	64				/* entries in table */
#define NL3PTESHIFT	6				/* log 2 of above */
#define	L3PTSIZE	(NL3PTEPERPT * MMU_PAGESIZE)	/* bytes mapped */
#define	L3PTOFFSET	(L3PTSIZE - 1)
#define NL2PTEPERPT	64				/* entries in table */
#define	L2PTSIZE	(NL2PTEPERPT * L3PTSIZE)	/* bytes mapped */
#define	L2PTOFFSET	(L2PTSIZE - 1)
#define NL1PTEPERPT	128				/* entries in table */

/*
 * Values for invalid page table entries. We use defines instead of
 * structures so that the startup code can use these even when it's
 * running at the wrong addresses.
 */
#define	MMU_INVALIDPTE		0
#define	MMU_INVALIDSPTP		0
#define	MMU_INVALIDLPTP1	0x7FFFFC00
#define	MMU_INVALIDLPTP2	0

/*
 * 68030 code register values.
 */
#define	FC_UD	1		/* user data */
#define	FC_UP	2		/* user program */
/*
 * we must use fc 4 rather than 3 as in sun3 for bcopy because the 68030 mmu
 * treats fc 3 accesses as in user mode
 */
#define	FC_MAP	4		/* Sun-3x bcopy hardware */
#define	FC_SD	5		/* supervisor data */
#define	FC_SP	6		/* supervisor program */
#define	FC_CPU	7		/* cpu space */

/*
 * Various devices.
 */
#define	ID_PROM		(V_IDPROM_ADDR)
#define ID_EEPROM       (V_EEPROM_ADDR+0x7d8)
#define	IDPROMSIZE	0x20
#define	IOMAP_SIZE	0x2000

/*
 * 68030 Cache Control Register
 */
#define ICACHE_BURST		0x0010
#define	ICACHE_ENABLE		0x0001 + ICACHE_BURST
#define	ICACHE_FREEZE		0x0002
#define	ICACHE_CLRENTRY		0x0004
#define	ICACHE_CLEAR		0x0008

#ifndef NO_DCACHE
#define DCACHE_BURST		0x1000
#define	DCACHE_ENABLE		0x2100 + DCACHE_BURST
#define	DCACHE_FREEZE		0x0200
#define	DCACHE_CLRENTRY		0x0400
#define	DCACHE_CLEAR		0x0800
#else
/* no data cache */
#define	DCACHE_ENABLE		0
#define	DCACHE_FREEZE		0
#define	DCACHE_CLRENTRY		0
#define	DCACHE_CLEAR		0
#endif

/*
 * 68030 MMU Status Register
 */
#define	MMU_BERR		0x8000		/* bus error */
#define	MMU_LIMIT		0x4000		/* limit violation */
#define	MMU_SUPER		0x2000		/* supervisor violation */
#define	MMU_WPROT		0x0800		/* write protected */
#define	MMU_INVLD		0x0400		/* invalid entry */
#define	MMU_MOD			0x0200		/* modified bit */
#define	MMU_TRANS		0x0040		/* transparent bit */
#define	MMU_LEVEL		0x0007		/* table level */

/*
 * Error types for mmu faults
 */
#define	MMUE_INVALID		0x01		/* invalid error */
#define	MMUE_SUPER		0x02		/* access violation */
#define	MMUE_PROTERR		0x03		/* write protect error */

/*
 * Block Copy Read and Block Copy Write. Because we have a 32 bit virtual
 * address, the masks don't actually do anything.
 */
#define	BC_LINESIZE		16
#define	BC_LINE_SHIFT		2
#define	BC_LINE_RESIDU		3
#define	BC_BLOCK_CPCMD		0x00000000
#define	BC_BLOCK_OFF		0xffffffff

#if defined(KERNEL) && !defined(LOCORE)
/*
 * Cache specific routines. These include routines for the data cache
 * and the translation cache.
 */
void	vac_flush();
void	atc_flush_all();
void	atc_flush_entry(/* addr */);
#endif defined(KERNEL) && !defined(LOCORE)

#endif /*!_sun3x_mmu_h*/

