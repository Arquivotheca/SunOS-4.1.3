/*	@(#)cpu.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
#ifndef	_sun4c_cpu_h
#define	_sun4c_cpu_h

#ifndef LOCORE
#include <mon/sunromvec.h>
#endif !LOCORE


#define	CPU_ARCH	0xf0		/* mask for architecture bits */
#define	SUN4C_ARCH	0x50		/* arch value for Sun 4c */

#define	CPU_MACH	0x0f		/* mask for machine implementation */
#define	MACH_60		0x01		/* SPARCstation 1 */
#define	MACH_40		0x02		/* SPARCstation IPC */
#define	MACH_65		0x03		/* SPARCstation 1+ */
#define	MACH_20		0x04		/* SPARCstation SLC */
#define	MACH_75		0x05		/* SPARCstation 2 */
#define	MACH_25		0x06		/* SPARCstation ELC */
#define	MACH_50		0x07		/* SPARCstation IPX */
#define	MACH_70		0x08
#define	MACH_80		0x09
#define	MACH_10		0x0a
#define	MACH_45		0x0b
#define	MACH_05		0x0c
#define	MACH_85		0x0d
#define	MACH_32		0x0e
#define	MACH_HIKE	0x0f

#define	CPU_SUN4C_60	(SUN4C_ARCH + MACH_60)
#define	CPU_SUN4C_40	(SUN4C_ARCH + MACH_40)
#define	CPU_SUN4C_65	(SUN4C_ARCH + MACH_65)
#define	CPU_SUN4C_20	(SUN4C_ARCH + MACH_20)
#define	CPU_SUN4C_75	(SUN4C_ARCH + MACH_75)
#define	CPU_SUN4C_25	(SUN4C_ARCH + MACH_25)
#define	CPU_SUN4C_50	(SUN4C_ARCH + MACH_50)
#define	CPU_SUN4C_70	(SUN4C_ARCH + MACH_70)
#define	CPU_SUN4C_80	(SUN4C_ARCH + MACH_80)
#define	CPU_SUN4C_10	(SUN4C_ARCH + MACH_10)
#define	CPU_SUN4C_45	(SUN4C_ARCH + MACH_45)
#define	CPU_SUN4C_05	(SUN4C_ARCH + MACH_05)
#define	CPU_SUN4C_85	(SUN4C_ARCH + MACH_85)
#define	CPU_SUN4C_32	(SUN4C_ARCH + MACH_32)
#define	CPU_SUN4C_HIKE	(SUN4C_ARCH + MACH_HIKE)

#if defined(KERNEL) && !defined(LOCORE)
extern int cpu;				/* machine type we are running on */
extern int dvmasize;			/* usable dvma size in clicks */

#ifdef	VAC
extern int vac;				/* there is a virtual address cache */
#else
#define	vac 0
#endif	VAC

#ifdef	IOC
extern int ioc;				/* there is an I/O cache */
#else
#define	ioc 0
#endif	IOC

#ifdef	BCOPY_BUF
extern int bcopy_buf			/* there is a bcopy buffer */
#else
#define	bcopy_buf 0
#endif	BCOPY_BUF

#endif KERNEL && !LOCORE

#endif /*!_sun4c_cpu_h*/
