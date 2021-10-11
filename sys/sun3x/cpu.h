/*	@(#)cpu.h 1.1 92/07/30 SMI	*/
/* syncd to sun3/cpu.h 1.19 */

/*
* Copyright (c) 1988 by Sun Microsystems, Inc.
*/

#ifndef _sun3x_cpu_h
#define _sun3x_cpu_h

#include <debug/debug.h>

/*
 * This file is intended to contain the specific details
 * of various implementations of a given architecture.
 */

#define	CPU_ARCH	0xf0		/* mask for architecture bits */
#define	SUN3X_ARCH	0x40		/* arch value for Sun 3X */

#define	CPU_MACH	0x0f		/* mask for machine implementation */
#define	MACH_470	0x01
#define MACH_80         0x02

#define	CPU_SUN3X_470	(SUN3X_ARCH + MACH_470)
#define CPU_SUN3X_80    (SUN3X_ARCH + MACH_80)

#if defined(KERNEL) && !defined(LOCORE)
extern int cpu;			/* machine type we are running on */
extern int dvmasize;		/* usable dvma size in clicks */
extern int bcenable;		/* block copy hardware is enabled */
extern int iocenable;		/* I/O cache is enabled (4.1 compatibility) */
#endif defined(KERNEL) && !defined(LOCORE)

/*
 * Various I/O space related constants
 */
#define	VME16D16_BASE	0x7C000000
#define	VME16D16_SIZE	(1<<16)
#define	VME16D16_MASK	(VME16D16_SIZE-1)

#define	VME16D32_BASE	0x7D000000
#define	VME16D32_SIZE	(1<<16)
#define	VME16D32_MASK	(VME16D32_SIZE-1)

#define	VME24D16_BASE	0x7E000000
#define	VME24D16_SIZE	(1<<24)
#define	VME24D16_MASK	(VME24D16_SIZE-1)

#define	VME24D32_BASE	0x7F000000
#define	VME24D32_SIZE	(1<<24)
#define	VME24D32_MASK	(VME24D32_SIZE-1)

#define	VME32D32_BASE	0x80000000
#define	VME32D32_SIZE	(1<<31)
#define	VME32D32_MASK	(VME32D32_SIZE-1)

/*
 * Some physical constants.
 */
#define	ENDOFMEM		0x40000000	/* end of memory */
#define	ENDOFTYPE0		0x58000000	/* end of type 0 space */

/*
 * The Basic DVMA space size for all Sun-3x implementations
 * is given by DVMASIZE.  The actual usable dvma size
 * (in clicks) is given by the dvmasize variable declared
 * above.
 */
#define	DVMASIZE	0x100000

#endif /*!_sun3x_cpu_h*/

