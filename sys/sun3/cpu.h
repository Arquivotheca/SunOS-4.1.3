/*      @(#)cpu.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 *
 * This file is intended to contain the specific details
 * of various implementations of a given architecture.
 */

#ifndef _sun3_cpu_h
#define _sun3_cpu_h

#define	CPU_ARCH	0xf0		/* mask for architecture bits */
#define	SUN3_ARCH	0x10		/* arch value for Sun 3 */

#define	CPU_MACH	0x0f		/* mask for machine implementation */
#define	MACH_160	0x01
#define	MACH_50		0x02
#define	MACH_260	0x03
#define	MACH_110	0x04
#define MACH_60		0x07
#define MACH_E		0x08

#define CPU_SUN3_160	(SUN3_ARCH + MACH_160)
#define CPU_SUN3_50	(SUN3_ARCH + MACH_50)
#define CPU_SUN3_260	(SUN3_ARCH + MACH_260)
#define CPU_SUN3_110	(SUN3_ARCH + MACH_110)
#define CPU_SUN3_60	(SUN3_ARCH + MACH_60)
#define CPU_SUN3_E	(SUN3_ARCH + MACH_E)

#if defined(KERNEL) && !defined(LOCORE)
int cpu;				/* machine type we are running on */
int dvmasize;				/* usable dvma size in clicks */
int fbobmemavail;			/* video copy memory is available */
#ifdef SUN3_260
int vac;				/* there is virtual address cache */
int bcenable;				/* bcopy hardware enabled */
#endif SUN3_260
#endif defined(KERNEL) && !defined(LOCORE)

/*
 * SEGTEMP & SEGTEMP2 are virtual segments reserved for temporary operations.
 * We use the segments immediately before the start of debugger area.
 */
#include <debug/debug.h>
#define	SEGTEMP		((addr_t)(DEBUGSTART - (2 * NBSG)))
#define	SEGTEMP2	((addr_t)(DEBUGSTART - NBSG))

/*
 * Various I/O space related constants
 */
#define	VME16_BASE	0xFFFF0000
#define	VME16_SIZE	(1<<16)
#define	VME16_MASK	(VME16_SIZE-1)

#define	VME24_BASE	0xFF000000
#define	VME24_SIZE	(1<<24)
#define	VME24_MASK	(VME24_SIZE-1)

/*
 * The Basic DVMA space size for all Sun-3 implementations
 * is given by DVMASIZE.  The actual usable dvma size
 * (in clicks) is given by the dvmasize variable declared
 * above (for compatibility with Sun-2).
 */
#define	DVMASIZE	0x100000

/*
 * FBSIZE is the amount of memory we will use for the frame buffer 
 * copy region if the copy mode of the frame buffer is to be used.
 */
#define FBSIZE   0x20000	/* size of frame buffer memory region */
#define OBFBADDR 0x100000	/* location of frame buffer in memory */

#endif /*!_sun3_cpu_h*/
