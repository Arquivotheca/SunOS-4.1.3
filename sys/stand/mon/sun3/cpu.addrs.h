/*
 * cpu.addrs.h
 *
 * @(#)cpu.addrs.h 1.1 92/07/30 Copyr 1986 Sun Micro
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Memory Addresses of Interest in the Sun-3 "Monitor" memory map
 *
 * THE VALUE FOR PROM_BASE must be changed in several places when it is
 * changed.  A complete list:
 *	../conf/Makefile.mon	RELOC= near top of file
 *	../h/sunromvec.h	#define romp near end of file
 *	../* /Makefile		(best to rerun sunconfig to generate them)
 * Since sunromvec is exported to the world, such a change invalidates all
 * object programs that use it (eg standalone diagnostics, demos, boot code,
 * etc) until they are recompiled.  As a transition aid, it is often useful
 * to specially map the old sunromvec location(s) so they also work.
 *
 * THE VALUE FOR MONBSS_BASE must be changed in the Makefiles when you
 * change it here, too.
 */

/*
 * The following define the base addresses in mappable memory where
 * the various devices and forms of memory are mapped in when the ROM
 * Monitor powers up.  Ongoing operation of the monitor requires that
 * these locations remain mapped as they were.
 */

/* On-board I/O devices */
#define	KEYBMOUSE_BASE	((struct zscc_device *)		0x0FE00000)
#define	SERIAL0_BASE	((struct zscc_device *)		0x0FE02000)
#define	EEPROM_BASE	((unsigned char *)		0x0FE04000)
#define	CLOCK_BASE	((struct intersil7170 *)	0x0FE06000)
#define	MEMORY_ERR_BASE	((struct memreg *)		0x0FE08000)
#define	INTERRUPT_BASE	((unsigned char *)		0x0FE0A000)
#define	ETHER_BASE	((struct obie_device *)		0x0FE0C000)
#ifndef SUN3F
#define	COLORMAP_BASE	((struct obcolormap *)		0x0FE0E000)
#endif SUN3F
#define	AMD_ETHER_BASE	((struct amd_ether *)		0x0FE10000)
#define	SCSI_BASE	((struct scsichip *)		0x0FE12000)
#define	DES_BASE	((struct deschip *)		0x0FE14000)
#define	ECC_CTRL_BASE	((struct ecc_ctrl *)		0x0FE16000)

/* Video memory (at least one plane of it...) */
#define	VIDEOMEM_BASE	((char *)			0x0FE20000)

/*****************************************************************************/
#ifdef PRISM
#define BW_ENABLE_MEM_BASE	((char *)		0x0FEA0000)
#define PRISM_CFB_BASE		((char *)		0x0FD00000)
#define PRISM_CFB_SIZE					0x100000
#define COLORMAP_SIZE                                   0x100
#endif PRISM

#ifdef SUN3F
#define BW_ENABLE_MEM_BASE	((char *)		0x0FEA0000)
#define PRISM_CFB_BASE		((char *)		0x0FD00000)
#define PRISM_CFB_SIZE					0x100000
#define SUN3F_CFB_BASE		((char *)		0x0FD00000)
#define SUN3F_CFB_SIZE					0x100000
#define P4_REG                  ((char *)               0x0FE7C000)
#define RES_SWITCH		((char *)		0x0FE7F000)
#define OVERLAY_BASE		((char *)               0x0FE80000)
#define COLORMAP_BASE           ((char *)               0x0FE0E000)
#define COLORMAP_SIZE                                   0x1000
#endif SUN3F

#define INVPMEG_BASE    ((char *)                       0x0FEC0000)

/*
 * Monitor scratch RAM for stack, trap vectors, and expanded font
 * (one page = 8K)
 */
#define	STACK_BASE	((char *)			0x0FE60000)
#define	STACK_TOP	((char *)			0x0FE60C00)
#define	TRAPVECTOR_BASE	((char *)			0x0FE60C00)
#define	FONT_BASE	((char *)			0x0FE61000)

/*
 * RAM page used to hold the valid-main-memory-pages bitmap if memory
 * fails self-test.  This will be mapped invalid if memory was all OK.
 */
#define	MAINMEM_BITMAP	((char *)			0x0FE62000)

/*
 * Roughly 512K for mapping in devices & mainmem during boot
 * Mapped Invalid otherwise (to Invalid PMEG if a whole pmeg invalid).
 */
#define	BOOTMAP_BASE	((char *)			0x0FE64000)

/*
 * Location of PROM code.  The system executes out of these addresses.
 *
 * SEE NOTE AT TOP OF FILE IF YOU CHANGE THIS VALUE.
 */
#define	PROM_BASE	((struct sunromvec *)		0x0FEF0000)

/*
 * First hardware virtual address where DVMA is possible.
 * The Monitor does not normally use this address range, but does
 * use it during bootstrap, via the resalloc() routine.
 */
#define	DVMA_BASE	((char *)			0x0FF00000)

/*
 * One page of scratch RAM for general Monitor global variables.
 * We put it here so it can be accessed with short absolute addresses.
 *
 * SEE NOTE AT TOP OF FILE IF YOU CHANGE THIS VALUE.
 */
#define	MONBSS_BASE	MONSHORTPAGE

/*
 * The Monitor maps only as much main memory as it can detect; the rest
 * of the address space (up thru the special addresses defined above)
 * is mapped as invalid.
 *
 * The last pmeg in the page map is always filled with PME_INVALID
 * entries.  This pmeg number ("The Invalid Pmeg Number") can be used to
 * invalidate an entire segment's worth of memory.
 *		B E   C A R E F U L !
 * If you change a page map entry in this pmeg, you change it for thousands
 * of virtual addresses.  (The standard getpagemap/setpagemap routines
 * will cause a trap if you attempt to write a valid page map entry to this
 * pmeg, but you could do it by hand if you really wanted to mess things up.)
 *
 * Because there is four times as much virtual memory space in a single context
 * as there are total pmegs to map it with, much of the monitor's memory
 * map must be re-mappings of the same pmegs.  There's no reason to duplicate
 * useful addresses, and several reasons not to, so we map the extra
 * virtual address space with the Invalid Pmeg Number.  This means that
 * some of the address space has their own page map entries, and the other
 * part all shares the one Invalid Pmeg.  Remember this when trying to map 
 * things; if the address you want is segmapped to the Invalid Pmeg, you'd
 * better find it a pmeg before you set a page map entry.
 *
 * The Monitor always uses page map entry PME_INVALID to map an invalid
 * page, although the only relevant bits are the (nonzero) permission bits
 * and the (zero) Valid bit.  PME_INVALID is defined in ./structconst.h,
 * which is generated by the Monitor makefile.
 */
#define	SEG_INVALID	NUMPMEGS-1
#define EEPROM    ((struct eeprom *)0x0FE04000)
