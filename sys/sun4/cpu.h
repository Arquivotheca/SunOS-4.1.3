/*	@(#)cpu.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef _sun4_cpu_h
#define _sun4_cpu_h

#ifndef LOCORE
#include <mon/sunromvec.h>
#endif !LOCORE

#define	CPU_ARCH	0xf0		/* mask for architecture bits */
#define	SUN4_ARCH	0x20		/* arch value for Sun 4 */

#define	CPU_MACH	0x0f		/* mask for machine implementation */
#define	MACH_260	0x01
#define	MACH_110	0x02
#define MACH_330	0x03
#define MACH_470	0x04

#define CPU_SUN4_260	(SUN4_ARCH + MACH_260)
#define CPU_SUN4_110	(SUN4_ARCH + MACH_110)
#define CPU_SUN4_330	(SUN4_ARCH + MACH_330)
#define CPU_SUN4_470	(SUN4_ARCH + MACH_470)

#if defined(KERNEL) && !defined(LOCORE)
extern int cpu;				/* machine type we are running on */
extern int dvmasize;			/* usable dvma size in pages */

#ifdef VAC
extern int vac;				/* there is a virtual address cache */
#else
#define vac 0
#endif VAC

#ifdef IOC
extern int ioc;				/* there is an I/O cache */
#else
#define ioc 0
#endif IOC

#ifdef BCOPY_BUF
extern int bcopy_buf;			/* there is a bcopy buffer */
#else
#define bcopy_buf 0
#endif BCOPY_BUF

#endif KERNEL && !LOCORE

#endif /*!_sun4_cpu_h*/
