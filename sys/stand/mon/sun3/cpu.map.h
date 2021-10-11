/*
 * cpu.map.h
 *
 * @(#)cpu.map.h 1.1 92/07/30 Copyr 1986 Sun Micro
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Memory Mapping and Paging on the Sun-3
 *
 * This file is used for both standalone code (ROM Monitor, 
 * Diagnostics, boot programs, etc) and for the Unix kernel.  IF YOU HAVE
 * TO CHANGE IT to fit your application, MOVE THE CHANGE BACK TO THE PUBLIC
 * COPY, and make sure the change is upward-compatible.  The last thing we 
 * need is seventeen different copies of this file, like we have with the
 * Sun-1 header files.
 */

#ifndef ADRSPC_SIZE

/*
 * The address space available to a single process is 256 Megabytes.
 */
#define	ADRSPC_SIZE	0x10000000

/*
 * The context register selects among 8 different address spaces.  Each
 * address space typically corresponds to a process and is 256 megabytes.
 * Each memory access is translated thru the address space indicated by
 * the context register.
 *
 * The context register occupies a byte in control space, although only
 * the low 3 bits are relevant.  The high-order bits are ignored on writes,
 * and return garbage on reads.  You can mask with CONTEXTMASK.
 */
typedef unsigned char context_t;

#define	NUMCONTEXTS	8
#define	CONTEXTMASK	(NUMCONTEXTS-1)	/* Relevant bits on read of cx reg */

/*
 * The segment map determines which large pieces of the 256MB address space
 * are actually in use.  Each chunk of address space is mapped to a set of
 * PGSPERSEG page map entries.  Chunks which are not in use should all be
 * mapped to a single set of page map entries, which should prohibit
 * all accesses.
 *
 * There are SEGSPERCONTEXT segment map entries in each logical context.
 * There are NUMPMEGS total page map entry groups (pmegs) in the physical
 * page map.  Each segment map entry references a pmeg which contains
 * the PGSPERSEG pages defining that segment.
 *
 * Note that there is much more virtual address space (256M) as there
 * is physical space mappable at one time (8M).  You can't map in 256 megs
 * at once even if you have that much physical memory, since the page map
 * is the bottleneck here.  The solution is to "page" the page map entries
 * in and out of the physical pmegs on demand.
 */
typedef unsigned char segnum_t; /* Segment number */

#define	SEGSPERCONTEXT	2048
#define	NUMPMEGS	256
#define	PGSPERSEG	16

/*
 * The following are valid in the pm_type field of the page map.
 */
enum pm_types {
	VPM_MEMORY	= 0,	/* Page is in main memory */
	VPM_IO		= 1,	/* Page is onboard I/O */
	VPM_VME16	= 2,	/* Page is on VMEbus, accessing 16-bit dev */
	VPM_VME32	= 3,	/* Page is on VMEbus, accessing 32-bit dev */
	VPM_MEMORY_NOCACHE = 4,	/* Page is in main memory, but not cacheable */
};


/*
 * The following are valid in the pm_permissions field.
 */
enum pm_perm {
	PMP_RO		= 0,	/* Page is read-only by all */
	PMP_RO_SUP	= 1,	/* Page is read-only by supervisor */
	PMP_ALL		= 2,	/* Page is read-write by all */
	PMP_SUP		= 3,	/* Page is read-write by supervisor */
};


/*
 * The page map gives fine-grain control over memory allocation.  Each page
 * map entry controls BYTESPERPG (a page) of memory.  Each page can be mapped
 * to memory, I/O, or a global bus (eg, VMEbus), or can be inaccessible.
 * Each page can be protected against user access and can be made readable
 * or read/write.  If access is denied, the
 * page referenced and modified bits will not be changed, nor will the page
 * number or type fields be used; so they can be used by software.
 */
#define	BYTESPERPG	8192
#define	BYTES_PG_SHIFT	13

struct pgmapent {
	unsigned	pm_valid	:1;	/* This entry is valid */
	enum pm_perm	pm_permissions	:2;	/* Access privileges */
	enum pm_types	pm_type		:3;	/* Type of page+don't cache */
	unsigned 	pm_accessed	:1;	/* Page has been read */
	unsigned	pm_modified	:1;	/* Page has been written */
	unsigned	pm_reserved	:5; 	/* served */
	unsigned	pm_page		:19;	/* Page # in physical memory */
};

#define	PMREALBITS	0xFF07FFFF	/* Which are actually implemented */
#define PMREALBITS_M25  0xFF0007FF      /* Which for M25 */

/*
 * When the page type is PM_IO, the page number field selects
 * which of the main I/O device chips is being selected.  Low-order (non-
 * mapped) address bits connect to the address lines of the device and
 * determine which facility of the device is being accessed.
 */
#define	VIOPG_KBM	0x00	/* Dual serial Z8530 SCC for keyboard&mouse */
#define	VIOPG_SERIAL0	0x10	/* Dual serial Z8530 SCC */
#define	VIOPG_EEPROM	0x20	/* Non-volatile memory */
#define	VIOPG_CLOCK	0x30	/* Intersil 7170 time-of-day clock */
#define	VIOPG_MEMORY_ERR 0x40	/* Uncorrectable Memory Error registers */
#define	VIOPG_INTERRUPT	0x50	/* Interrupt control register */
#define	VIOPG_ETHER	0x60	/* Intel 82586 Ethernet interface */
#define	VIOPG_COLORMAP	0x70	/* Color Map for onboard video, PRISM */
#define	VIOPG_PROM	0x80	/* Bootstrap proms */
#define	VIOPG_AMD_ETHER	0x90	/* AMD Ethernet interface */
#define	VIOPG_SCSI	0xA0	/* Onboard SCSI interface */
/*			0xB0	/* Reserved */
/*			0xC0	/* Reserved */
/*			0xD0	/* Reserved */
#define	VIOPG_DES	0xE0	/* AMD 8068 data ciphering processor */
#define	VIOPG_ECC_CTRL	0xF0	/* ECC Control Register access */


/*
 * Other special page numbers.
 */

#ifdef PRISM
			/* these are the physical page numbers */
#define MEMPG_VIDEO	(0xFF000000 >> BYTES_PG_SHIFT)  /* prism bw frame buf */
#define MEMPG_BW_ENABLE (0xFE400000 >> BYTES_PG_SHIFT)  /* enable plane */
#define MEMPG_PRISM_CFB (0xFE800000 >> BYTES_PG_SHIFT)  /* prism color fb */
#endif PRISM

#ifdef SUN3F
#define MEMPG_VIDEO     (0xFF000000 >> BYTES_PG_SHIFT)  /* sun3f bw frame buf */
#define MEMPG_RES	(0xFF1C0000 >> BYTES_PG_SHIFT)  /* hi/low res switch */
#define MEMPG_P4_REG	(0xFF300000 >> BYTES_PG_SHIFT)  /* P4 bus register */
#define MEMPG_OVERLAY   (0xFF400000 >> BYTES_PG_SHIFT)  /* overlay plane */
#define MEMPG_BW_ENABLE (0xFF600000 >> BYTES_PG_SHIFT)  /* enable plane */
#define MEMPG_PRISM_CFB (0xFF800000 >> BYTES_PG_SHIFT)  /* prism color fb */
#define MEMPG_SUN3F_CFB (0xFF800000 >> BYTES_PG_SHIFT)  /* sun3f color fb */
#define MEMPG_COLORMAP  (0xFF200000 >> BYTES_PG_SHIFT)  /* sun3f colormap */
#define MEMPG_P4_MFB1   (0xFF400000 >> BYTES_PG_SHIFT)  /* P4 mono FB 1 */
#define MEMPG_P4_MFB2   (0xFF600000 >> BYTES_PG_SHIFT)  /* P4 mono FB 2 */
#define MEMPG_P4_CMAP1  (0xFF200000 >> BYTES_PG_SHIFT)  /* P4 color map 1 */
#define MEMPG_P4_OVRLAY1  (0xFF400000 >> BYTES_PG_SHIFT)  /* P4 overlay pl 1 */
#define MEMPG_P4_ENABLE1  (0xFF600000 >> BYTES_PG_SHIFT)  /* P4 enable  pl 1 */
#define MEMPG_P4_CFB1   (0xFF800000 >> BYTES_PG_SHIFT)  /* P4 color FB 1 */
#endif SUN3F


#ifdef CARRERA
#define MEMPG_VIDEO     (0xFF000000 >> BYTES_PG_SHIFT)  /* carrera frame buf */
#endif CARRERA

#ifdef SIRIUS
#define MEMPG_VIDEO     (0xFF000000 >> BYTES_PG_SHIFT)  /* carrera frame buf */
#endif SIRIUS

#ifdef M25
#define MEMPG_VIDEO  	(0x00100000 >> BYTES_PG_SHIFT)  /* FB in main mem */
#endif M25

#define	VMEPG_24ADDR	(0xFF000000 >> BYTES_PG_SHIFT)	/* 24-bit addr VME */
#define	VMEPG_16ADDR	(0xFFFF0000 >> BYTES_PG_SHIFT)	/* 16-bit addr VME */
#define	VME_COLOR_PHYS	0xFF400000	/* Base addr (not pg#) of VME color */
#define	VPM_VME_COLOR	VPM_VME16	/* Page type for VME color */
#define	VMEPG_COLOR	(VME_COLOR_PHYS >> BYTES_PG_SHIFT)

#if defined(PRISM) || defined(SUN3F)
#define	MONBEG	MONSTART-PRISM_CFB_SIZE
#else
#define MONBEG  MONSTART
#endif PRISM || SUN3F


/*
 * The maps are accessed from supervisor state by using the "movs" (Move
 * Spacey) instruction.  This moves a byte, word, or longword to/from a
 * register and a location in another address space.  The Sun-3 hardware
 * defines one of these address spaces (defined by function code values)
 * as the "control address space", including various control registers
 * as well as the maps.  The particular map accessed is determined
 * by the high-order 4 bits of the address used.  The particular entry accessed
 * is determined by the middle-order bits of the address used --
 * we access the entry that controls that page.  Which particular byte(s) of
 * the map entry are accessed is controlled by the low-order bits of the
 * address used, as usual.
 *
 * The following defines the encodings for the various address spaces used
 * by "movs".
 */
#define	FC_UD	1	/* User Data */
#define	FC_UP	2	/* User Program */
#define	FC_MAP	3	/* Sun-3 Memory Maps */
#define	FC_SD	5	/* Supervisor Data */
#define	FC_SP	6	/* Supervisor Program */
#define	FC_CPU	7	/* CPU Space (Int Ack, Co-processors, ...) */

#define	SEGMAPADR(addr)	(char *)(((int)addr&MAPADDRMASK)+SMAPOFF)
#define	PAGEMAPADR(addr)(long *)(((int)addr&MAPADDRMASK)+PMAPOFF)

#define	IDPROMOFF	0x00000000	/* ID Prom */
#define	PMAPOFF		0x10000000	/* Page map offset within maps */
#define	SMAPOFF		0x20000000	/* Segment map offset within maps */
#define	CONTEXTOFF	0x30000000	/* Context registers */
#define	ENABLEOFF	0x40000000	/* System Enable Reg -- turns me on */
#define	UDMAENABLEOFF	0x50000000	/* User DVMA Enable Reg */
#define	BUSERROFF	0x60000000	/* Bus Error Register - tells why */
#define	LEDOFF		0x70000000	/* LED's for diagnostics -- 0=lit */
#define	SERIALOFF	0xF0000000	/* Serial port bypass for diagnostics */

#define	MAPADDRMASK	0x0FFFE000	/* Keeps bits relevant to map entry */


/*
 * The following subroutines accept any address in the mappable range
 * (256 megs).  They access the map for the current context.  They
 * assume that we are currently running in supervisor state.
 *
 * We can't declare getpgmap() as returning a struct, because our C compiler
 * is brain damaged and returns a pointer to a static area if you return a
 * struct.  We therefore return an int and require the caller to set up
 * unions and other assorted random hacks because the language
 * implementation doesn't support structures returned from reentrant routines.
 */ 

extern /*struct pgmapent*/ getpgmap();	/* (addr) */
extern                     setpgmap();	/* (addr, entry) */
extern segnum_t getsegmap();		/* (addr) */
extern          setsegmap();		/* (addr, entry) */
extern context_t getsupcontext();	/* () */
extern           setsupcontext();	/* (entry) */
extern context_t getusercontext();	/* () */
extern           setusercontext();	/* (entry) */

#endif ADRSPC_SIZE
