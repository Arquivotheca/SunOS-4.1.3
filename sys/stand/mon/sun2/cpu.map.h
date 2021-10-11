/*      @(#)cpu.map.h 1.1 92/07/30 SMI; from UCB X.X XX/XX/XX       */
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Memory Mapping and Paging on the Sun-2
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
 * The address space available to a single process is 16 Megabytes.
 */
#define	ADRSPC_SIZE	0x1000000

/*
 * The context registers selects among 16 different address spaces.  Each
 * address space typically corresponds to a process and is 16 megabytes.
 * Each memory access is translated thru the address space indicated by
 * one of two context registers -- user or supervisor.  The distinction
 * is made based on the 68010's FC0-2 (function code) bits and thus can be
 * deliberately fooled by the "movs" instruction.
 *
 * The two context registers each occupy a byte, although only the low
 * bits are relevant.  The high-order bits are ignored on writes,
 * and return garbage on reads.  You can mask with CONTEXTMASK.
 */
typedef unsigned char context_t;
struct context_regs {
	context_t cx_super;		/* Supervisor access context number */
	context_t cx_user;		/* User access context number */
};

#define	NUMCONTEXTS	8
#define	CONTEXTMASK	(NUMCONTEXTS-1)	/* Relevant bits on read of cx reg */

/*
 * The segment map determines which large pieces of the 16M address space
 * are actually in use.  Each chunk of address space is mapped to a set of
 * PGSPERSEG page map entries.  Chunks which are not in use should all be
 * mapped to the same set of page map entries, which should prohibit
 * all accesses.
 *
 * There are SEGSPERCONTEXT segment map entries in each logical context.
 * There are NUMPMEGS total page map entry groups (pmegs) in the physical
 * page map.  Each segment map entry references a pmeg which contains
 * the PGSPERSEG pages defining that segment.
 *
 * Note that there is twice as much virtual address space (16M) as there
 * is physical space mappable at one time (8M).  You can't map in 16 megs
 * at once even if you have that much physical memory, since the page map
 * is the bottleneck here.  The solution is to "page" the page map entries
 * in and out of the physical pmegs on demand.
 */
typedef unsigned char segnum_t; /* Segment number */

#define	SEGSPERCONTEXT	512
#define	NUMPMEGS	256
#define	PGSPERSEG	16

/*
 * The following values are valid in the pm_type field of the page map.
 */
enum pm_types {
	/*
	 * Multibus CPU page types
	 */
	MPM_MEMORY	= 0,	/* Page is main memory or frame buffer */
	MPM_IO		= 1,	/* Page is main I/O */
	MPM_BUSMEM	= 2,	/* Page is memory on the Multibus */
	MPM_BUSIO	= 3,	/* Page is I/O on the Multibus */
	/* Multibus values of 4 thru 7 are reserved for future enhancements */

	/*
	 * VME CPU page types
	 */
	VPM_MEMORY	= 0,	/* Page is in main memory */
	VPM_IO		= 1,	/* Page is onboard I/O or frame buffer */
	VPM_VME0	= 2,	/* Page is in low 8M of VME bus */
	VPM_VME8	= 3,	/* Page is in high 8M of VME bus, or VME I/O */	
	/* VME values of 4 thru 7 are reserved for future enhancements */
#define	VPM_VME16	VPM_VME0	/* 16-bit-data.  Doesn't really
					   work (for top 8 megs). */
};
/*
 * The following are valid in the pm_permissions field.
 */
enum pm_perm {
        PMP_RO          = 0,    /* Page is read-only by all */
        PMP_RO_SUP      = 1,    /* Page is read-only by supervisor */
        PMP_ALL         = 2,    /* Page is read-write by all */
        PMP_SUP         = 3,    /* Page is read-write by supervisor */
};

/*
 * The page map gives fine-grain control over memory allocation.  Each page
 * map entry controls BYTESPERPG (a page) of memory.  Each page can be mapped
 * to memory, I/O, or a global bus (eg, Multibus), or can be inaccessible.
 * Independent protection against reading, writing, or executing, for the
 * supervisor and for the user, is provided.  If access is denied, the
 * page referenced and modified bits will not be changed, nor will the page
 * number or type fields be used; so they can be used by software.
 */
#define	BYTESPERPG	2048
#define	BYTES_PG_SHIFT	11

struct pgmapent {
	unsigned	pm_valid	:1;	/* This entry is valid */
	unsigned	pm_permissions	:6;	/* Access privileges */
	enum pm_types	pm_type		:3;	/* Type of page */
	unsigned 	pm_accessed	:1;	/* Page has been read */
	unsigned	pm_modified	:1;	/* Page has been written */
	unsigned	pm_reserved	:4;	/* Reserved: page # expansion */
	unsigned	pm_page		:16;	/* Page # in physical memory */
};

#define	PMREALBITS	0xFFF00FFF	/* Which are actually implemented */

/*
 * The following permissions may be OR-ed together to create the
 * pm_permissions field.
 */
#define	PMP_SUP_READ		0x20
#define	PMP_SUP_WRITE		0x10
#define	PMP_SUP_EXECUTE		0x08
#define	PMP_USER_READ		0x04
#define	PMP_USER_WRITE		0x02
#define	PMP_USER_EXECUTE	0x01
#define	PMP_NONE		0x00
#define	PMP_SUP_ALL		0x38
#define	PMP_USER_ALL		0x07
/* Equivalents for Sun-3 permissions */
#define	PMP_RO			0x2D
#define	PMP_RO_SUP		0x28
#define	PMP_ALL			0x3F
#define	PMP_SUP			0x38

/*
 * When the page type is PM_IO, the page number field selects
 * which of the main I/O device chips is being selected.  Low-order (non-
 * mapped) address bits connect to the address lines of the device and
 * determine which facility of the device is being accessed.
 *
 * Multibus I/O pages first.
 */
#define	MIOPG_PROM	0	/* Bootstrap proms */
#define	MIOPG_FLOAT	1	/* Intel 80287 Floating Point Processor */
#define	MIOPG_DES	2	/* AMD 8068 data ciphering processor */
#define	MIOPG_PARALLEL	3	/* Parallel input port (kbd/mouse) */
#define	MIOPG_SERIAL0	4	/* Dual serial Z8530 SCC */
#define	MIOPG_TIMER	5	/* AMD 9513 System Timing Controller */
#define	MIOPG_ROP	6	/* RasterOp Processor */
#define	MIOPG_CLOCK	7	/* National 58167 real-time clock */
/*
 * VME I/O pages
 */
#define	VIOPG_VIDEO	0	/* Frame buffer is first 128K of I/O space */
#define	VIOPG_VIDEOCTL	0x40	/* Video control register at loc 128K */
#define	VIOPG_PROM	0xFE0	/* Bootstrap proms */
#define	VIOPG_ETHER	0xFE1	/* Intel 82586 Ethernet interface */
#define	VIOPG_DES	0xFE2	/* AMD 8068 data ciphering processor */
#define	VIOPG_KBM	0xFE3	/* Dual serial Z8530 SCC for keyboard&mouse */
#define	VIOPG_SERIAL0	0xFE4	/* Dual serial Z8530 SCC */
#define	VIOPG_TIMER	0xFE5	/* AMD 9513 System Timing Controller */
/*			0xFE6	/* unused */
/*			0xFE7	/* unused */

/*
 * Other special page numbers -- Multibus
 */
/* Frame buffer board, including buffer, zscc for keyb/mouse, ctrl reg */
#define	MEMPG_VIDEO		(0x700000 >> BYTES_PG_SHIFT)
#define	MEMPG_VIDEO_ZSCC	(0x780000 >> BYTES_PG_SHIFT)
#define MEMPG_VIDEO_CTRL	(0x781800 >> BYTES_PG_SHIFT)

/*
 * Other special page numbers -- VME
 */
#define	VMEPG_16ADDR	0xFE0	/* Base of VME 16-bit address space */
				/* Base address (not pg#) of VME color */
#define	VME_COLOR_PHYS	0x400000
				/* Now do page type and page number */
#if VME_COLOR_PHYS < 0x800000
#define	VPM_VME_COLOR	VPM_VME0	/* Page type for VME color */
#else
#define	VPM_VME_COLOR	VPM_VME8	/* Page type for VME color */
#endif
#define	VMEPG_COLOR	((VME_COLOR_PHYS&0x7FFFFF) >> BYTES_PG_SHIFT)


/*
 * The maps are accessed from supervisor state by using the "movs" (Move
 * Spacey) instruction.  This moves a byte, word, or longword to/from a
 * register and a location in another address space.  The Sun-2 hardware
 * defines one of these address spaces (defined by function code values)
 * as the "maps address space".  The particular map accessed is determined
 * by the low-order 3 bits of the address used.  (The other low-order bits
 * are reserved for future use.)  The particular entry accessed
 * is determined by the high-order bits of the address used --
 * we access the entry that controls that address in user space.
 *
 * The following defines the encodings for the various address spaces used
 * by "movs".
 */
#define	FC_UD	1	/* User Data */
#define	FC_UP	2	/* User Program */
#define	FC_MAP	3	/* Sun-2 Memory Maps */
#define	FC_SD	5	/* Supervisor Data */
#define	FC_SP	6	/* Supervisor Program */
#define	FC_CPU	7	/* CPU Space (Int Ack, Co-processors) */

#define	SEGMAPADR(addr)	(char *)(((int)addr&MAPADDRMASK)+SMAPOFF)
#define	PAGEMAPADR(addr)(long *)(((int)addr&MAPADDRMASK)+PMAPOFF)

#define	MAPADDRMASK	0xFFFFF800	/* Masks random addresses for mapping */
#define	PMAPOFF		0	/* Page map offset within maps */
#define	CONTEXTOFF	6	/* Context registers are at loc 6 */
#define	SUPCONTEXTOFF	6	/* Supervisor context at loc 6 */
#define	USERCONTEXTOFF	7	/* User context just afterward */
#define	SMAPOFF		5	/* Segment map offset within maps */
#define	IDPROMOFF	8	/* ID Prom at 8, 8+0x800, etc */
#define	LEDOFF		0xB	/* LED's for diagnostics -- 0=lit */
#define	BUSERROFF	0xD	/* Bus Error Register - tells why */
#define	ENABLEOFF	0xF	/* System Enable Reg -- turns me on */
#ifdef MENU
#define UDMAENABLEOFF   0x5000000      /* User DVMA Enable Reg */
#endif MENU


/*
 * The following subroutines accept any address in the mappable range
 * (16 megs).  They access the map for the current user context.  They
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
