/*
 * cpu.map.h
 *
 * @(#)cpu.map.h 1.1 92/07/30 SMI
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 * Memory Mapping and Paging on the Sun-4
 *
 * This file is used for both stand-alone code (ROM Monitor, Diagnostics,
 * boot programs, etc.) and for the Unix kernel.  IF YOU HAVE TO CHANGE IT
 * to fit your application, MOVE THE CHANGE BACK TO THE PUBLIC COPY, and
 * make sure the change is upward-compatible.  The last thing we need is
 * many different copies of this file.
 *
 * The context register selects among NUMCONTEXTS different address spaces 
 * (contexts).  Each address space typically corresponds to a process and is 
 * 4G-bytes.  Each memory access is translated through the address space 
 * indicated by the context register.
 *
 * The context register occupies a byte in control space, although only
 * the low 4 bits are relevant.  The high-order bits are ignored on writes,
 * and return garbage on reads.  You can mask the upper 4 bits with CONTEXTMASK.
 */

#ifndef COBRA
#ifdef sunray
#define NUMCONTEXTS 64 /* sunray  */
#else sunray
#define NUMCONTEXTS 16 /* sunrise */
#endif sunray
#else COBRA
#define NUMCONTEXTS 8  /* cobra   */
#endif COBRA

#define	CONTEXTMASK (NUMCONTEXTS-1) /* Relevant bits on read of cx reg */

/*
 * The segment map determines which large pieces of the 4G-byte address space
 * are actually in use.  Each chunk of address space is mapped to a set of
 * PGSPERSEG page map entries (pmegs).  Chunks which are not in use should all 
 * be mapped to a single set of page map entries, which should prohibit
 * all accesses.
 *
 * There are SEGSPERCONTEXT segment map entries in each logical context.
 * There are NUMPMEGS total page map entry groups (pmegs) in the physical
 * page map.  Each segment map entry references a pmeg which contains
 * the PGSPERSEG (number of page table entries per pmeg) pages defining 
 * that segment.
 *
 * Note that there is much more virtual address space (4G-bytes) as there
 * is physical space mappable at one time (16384 x 8K ???).  You can't map in 
 * 4G-bytes at once even if you have that much physical memory, since the page 
 * map is the bottleneck here.  The solution is to "page" the page map entries
 * in and out of the physical pmegs on demand.
 */

#define	SEGSPERCONTEXT	4096

#ifdef COBRA
#define NUMPMEGS	256
#else COBRA
#define	NUMPMEGS	512
#endif COBRA

#define	PGSPERSEG	32   /* The number of pmegs associated *
                              * with each segment table entry. */
#ifndef FIRMWARE
/*
 * The following are valid in the pm_type field of the page map.
 */
enum pm_types {
  VPM_MEMORY=0,		/* Page is in main memory.                     */
  VPM_IO=1,		/* Page is on-board I/O.                       */
  VPM_VME16=2,		/* Page is on VME-bus, accessing 16-bit device.*/
  VPM_VME32=3,		/* Page is on VME-bus, accessing 32-bit device.*/
  VPM_MEMORY_NOCACHE=4,	/* Page is in main memory, but not cacheable.  */
  VPM_IO_NOCACHE=5	/* Page is in on-board I/O, but not cacheable. */
};
/*
 * The following are valid in the pm_permissions field.
 */
enum pm_perm {
  PMP_RO=0,	/* Page is read-only by all (user and supervisor). */
  PMP_RO_SUP=1,	/* Page is read-only by supervisor.                */
  PMP_ALL=2,	/* Page is read-write by all (user and supervisor).*/
  PMP_SUP=3,	/* Page is read-write by supervisor.               */
};
struct pgmapent {
  unsigned	pm_valid	:1;	/* This entry is valid.       */
  enum pm_perm	pm_permissions	:2;	/* Access privileges: write   */
					/* access? supervisor access? */
  enum pm_types	pm_type		:3;	/* don't cache and page type. */
  unsigned	pm_accessed	:1;	/* Page has been read.        */
  unsigned	pm_modified	:1;	/* Page has been written.     */
  unsigned	pm_reserved	:5;	/* Reserved.                  */
  unsigned	pm_page		:19;	/* Page # in physical memory. */
};
#endif FIRMWARE

/*
 * The page map gives fine-grain control over memory allocation.  Each page
 * map entry controls BYTESPERPG (a page) of memory.  Each page can be mapped
 * to memory, I/O, or a global bus (e.g. VME-bus), or can be inaccessible.
 * Each page can be protected against user access and can be made readable
 * or readable and writable.  If access is denied, the page referenced and 
 * modified bits will not be changed, nor will the page number or type fields 
 * be used; so they can be used by software.
 */
#define	BYTESPERPG	8192
#define	BYTESPERSEG	(BYTESPERPG*PGSPERSEG)
#define	BYTES_PG_SHIFT	13

#ifdef COBRA
#define PMREALBITS	0xFF007FFF
#else COBRA
#define	PMREALBITS	0xFF07FFFF /* Which are actually implemented.  Mask *
                                    * out 5 Reserved bits.                  */
#endif COBRA

/*
 * When the page type is PM_IO, the PAGE NUMBER field selects
 * which of the main I/O device chips is being selected.  Low-order (non-
 * mapped) address bits connect to the address lines of the device and
 * determine which facility of the device is being accessed.
 * 
 * Type 1 devices and their corresponding page number.
 */
#define	VIOPG_KBM	 0x78000 /* Dual serial Z8530 SCC for keyboard&mouse */
#define	VIOPG_SERIAL0	 0x78800 /* Dual serial Z8530 SCC                    */
#define	VIOPG_EEPROM	 0x79000 /* Non-volatile memory (EEPROM)             */
#define	VIOPG_CLOCK	 0x79800 /* Intersil 7170 time-of-day clock          */
#define	VIOPG_MEMORY_ERR 0x7a000 /* CPU Memory Error registers               */
#define	VIOPG_INTERRUPT	 0x7a800 /* Interrupt control register               */
#define	VIOPG_ETHER	 0x7b000 /* Intel 82586 Ethernet interface           */
#define	VIOPG_COLORMAP	 0x7b800 /* Color Map for onboard video someday      */
#define	VIOPG_PROM	 0x7c000 /* Bootstrap proms (EPROM)                  */
#define	VIOPG_AMD_ETHER	 0x7c800 /* AMD Ethernet interface                   */
#define VIOPG_SCSI	 0x7d000 /* Onboard SCSI interface                   */
/*			 0x7d800  * Reserved                                 */
/*			 0x7e000  * Reserved                                 */

#ifdef COBRA
#define VIOPG_VIDEO      0x7da00 /* Monochrome FB in P4 bus.                 */
#define VIOPG_VID_ID	 0x7d980 /* P4 bus video ID register.		     */
#else COBRA
#define VIOPG_VIDEO      0x7e800 /* video RAM                                */
#endif COBRA

#define	VIOPG_DES	 0x7f000 /* AMD 8068 data ciphering processor        */
#define	VIOPG_ECC_CTRL	 0x7f8f0 /* ECC Control Register access              */

/*
 * For new "setupmap" routine in file "mapmem.c".
 *
 * 0xd40 is the status bits in page table entry:
 *	valid, write allowed, don't cache, type 1, not accessed, not modified.
 *
 * 0x700:
 *	invalid page.
 */
#define VIOPAGE_INVALID    0x70000000   /* Invalid page table entry.          */
#define	VIOPAGE_KBM	   0xd4078000   /* Dual serial Z8530 SCC for          *
                                         * keyboard&mouse                     */
#define	VIOPAGE_SERIAL0	   0xd4078800   /* Dual serial Z8530 SCC              */
#define	VIOPAGE_EEPROM	   0xd4079000   /* Non-volatile memory (EEPROM)       */
#define	VIOPAGE_CLOCK	   0xd4079800   /* Intersil 7170 time-of-day clock    */
#define	VIOPAGE_MEMORY_ERR 0xd407a000   /* CPU Memory Error registers         */
#define	VIOPAGE_INTERRUPT  0xd407a800   /* Interrupt control register         */
#define	VIOPAGE_ETHER	   0xd407b000   /* Intel 82586 Ethernet interface     */
#ifndef COBRA
#define	VIOPAGE_COLORMAP   0xd407b800   /* Color Map for onboard video someday*/
#endif COBRA
#define	VIOPAGE_PROM	   0xd407c000   /* Bootstrap proms (EPROM)            */
#define	VIOPAGE_AMD_ETHER  0xd407c800   /* AMD Ethernet interface             */
#define VIOPAGE_SCSI	   0xd407d000   /* Onboard SCSI interface             */
/*			   0xd407d800    * Reserved                           */
/*			   0xd407e000    * Reserved                           */

#ifdef COBRA
#define VIOPAGE_VID_ID	   0xd407d980    /* video ID register in P4 bus.     */
#define VIOPAGE_VIDEO	   0xd407da00	 /* video RAM in P4 bus.	     */
#define VIOPAGE_OVRLAY	   VIOPAGE_VIDEO /* overlay plane in P4 color board. */
#define VIOPAGE_BW_ENA	   0xd407db00    /* overlay enable plane in P4 color.*/
#ifdef FIRMWARE
#define VIOPAGE_CFB        0xd407dc00	 /* color frame buffer in P4 color.  */
#endif  FIRMWARE
#define VIOPAGE_COLORMAP   0xd407d900    /* color map in P4 color board.     */
#else COBRA
#define VIOPAGE_VIDEO      0xd407e800   /* video RAM                         */
#endif COBRA

#define	VIOPAGE_DES	   0xd407f000   /* AMD 8068 data ciphering processor  */
#define	VIOPAGE_ECC_CTRL   0xd407f8f0   /* ECC Control Register access        */

#ifndef COBRA				/* no VME in cobra		    */
/*
 * Other special page numbers.
 */
#define	VMEPG_24ADDR	(0xFF000000 >> BYTES_PG_SHIFT)	/* 24-bit addr VME  */
#define	VMEPG_16ADDR	(0xFFFF0000 >> BYTES_PG_SHIFT)	/* 16-bit addr VME  */
#define VME_COLOR_PHYS 0xFF400000	 /* Base addr (not pg#) of VME color */
#define VPM_VME_COLOR	VPM_VME16        /* Page type for VME color          */
#define VMEPG_COLOR	(VME_COLOR_PHYS >> BYTES_PG_SHIFT)
#else COBRA
#define VME_COLOR_PHYS 0xFF400000    /* Base addr (not pg#) of VME color */
#endif COBRA

#ifndef FIRMWARE
#ifdef COBRA
#define VIOPAGE_ENA_PLANE  0xd407ea00   /* cobra video enable plane	      */
#define VIOPAGE_CFB	   0xd407ec00   /* cobra color frame buffer           */
#endif COBRA
#endif FIRMWARE

#ifdef COBRA
#define MONBEG  0xFFC00000		/* 2Mbytes before MONSTART to
					 * accomodate color frame buffer and
					 * enable plane. */
#else COBRA
#define MONBEG  MONSTART
#endif COBRA


/*
 * The maps are accessed from supervisor state.
 *
 * The following defines the encodings for the various address spaces used
 * by "movs".
 */

#define	SEGMAPADR(addr)	(char *)(((int)addr&MAPADDRMASK))
#define	PAGEMAPADR(addr)(long *)(((int)addr&MAPADDRMASK))

#define	IDPROMOFF	0x00000000	/* ID Prom */
#define	CONTEXTOFF	0x30000000	/* Context registers */
#define	ENABLEOFF	0x40000000	/* System Enable Reg -- turns me on */

#ifndef COBRA				/* no user DVMA in cobra */
#define	UDMAENABLEOFF	0x50000000	/* User DVMA Enable Reg */
#endif COBRA

#define	BUSERROFF	0x60000000	/* Bus Error Register - tells why */
#define	LEDOFF		0x70000000	/* LED's for diagnostics -- 0=lit */
#define	SERIALOFF	0xF0000000	/* Serial port bypass for diagnostics */

#define	MAPADDRMASK	0xFFFFE000	/* Keeps bits relevant to map entry */

/*
 * Definitions for Normal Mode Memory Test 
 * and Extended Test System's Memory Test.
 */
#define MEG                 0x100000
#define LOW_VIRTUAL_ADDRESS 0x800000
#define CACHE_STATUS_BITS   0xd0000000 /* Not Cacheable. */
#define PROM_STATUS_BITS    0xC0000000 /* Cacheable.     */


/*
 * The following subroutines accept any address in the mappable range.
 * They access the map for the current context.  They assume that we are 
 * currently running in supervisor state.
 *
 * We can't declare getpgmap() as returning a struct, because our C compiler
 * is brain damaged and returns a pointer to a static area if you return a
 * struct.  We therefore return an int and require the caller to set up
 * unions and other assorted random hacks because the language implementation 
 * doesn't support structures returned from reentrant routines.
 */ 

extern int            getpgmap();       /* (addr)                             */
extern int            setpgmap();       /* (addr, entry)                      */
extern unsigned short getsegmap();      /* (addr) (unsigned short is returned)*/
extern int            setsegmap();	/* (addr, entry)                      */
extern unsigned char  getsupcontext();	/* () (unsigned char is returned)     */
extern int            setsupcontext();	/* (entry)                            */
extern unsigned char  getusercontext();	/* () (unsigned char is returned)     */
extern int            setusercontext();	/* (entry)                            */

