/*
 * cpu.addrs.h
 *
 * @(#)cpu.addrs.h 1.1 92/07/30 SMI
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 * Memory Addresses of interest in the Sun-4 Monitor memory map.
 *
 * Since "sunromvec.h" is exported to the world, a change invalidates all 
 * object programs that use it (e.g. stand-alone diagnostics, demos, boot 
 * code, etc.) until they are re-compiled with the new version of include
 * file "sunromvec.h".  As a transition aid, it is often useful to specially
 * map the old "sunromvec" location(s) so they also work.
 */

/*
 * According to the architecture manual, the three uppermost (most 
 * significant) virtual address bits must be equal (either "000" or 
 * "111").  Therfore, virtual addresses 0x20000000 through 0xdfffffff,
 * inclusive, are illegal.
 */
#define INVALID_ADDRESS_START 0x20000000
#define INVALID_ADDRESS_END   0xE0000000

/*
 * The following define the base addresses in mappable memory where
 * the various devices and forms of memory are mapped in when the EPROM
 * Monitor powers up.  On-going operation of the monitor requires that
 * these locations remain mapped as they were.
 */
#define	KEYBMOUSE_BASE	((struct zscc_device *)	 0xFFD00000) /*   On     */
#define	SERIAL0_BASE	((struct zscc_device *)	 0xFFD02000) /*  board   */
#define	EEPROM_BASE	((unsigned char *)	 0xFFD04000) /*  input   */
#define	CLOCK_BASE	((struct intersil7170 *) 0xFFD06000) /*   and    */
#define	MEMORY_ERR_BASE	((struct memreg *)	 0xFFD08000) /*  output  */
#define	INTERRUPT_BASE	((unsigned char *)	 0xFFD0A000) /* devices. */
#define	ETHER_BASE	((struct obie_device *)	 0xFFD0C000)
#define	ETHER_CR_BASE	((unsigned char *)	 0xF6000000)
#ifndef COBRA
#define	COLORMAP_BASE	((struct obcolormap *)	 0xFFD0E000)
#endif  COBRA

#ifndef FIRMWARE
#define	COLORMAP_BASE	((struct obcolormap *)	 0xFFD0E000)
#endif FIRMWARE

#define	AMD_ETHER_BASE	((struct amd_ether *)	 0xFFD10000)
#define	SCSI_BASE	((struct scsichip *)	 0xFFD12000)
#define	DES_BASE	((struct deschip *)	 0xFFD14000)

#ifdef COBRA
#define P4_REG     	((unsigned int *)        0xFFD16000) /* P4 video ID */
#else COBRA
#define	ECC_CTRL_BASE	((struct ecc_ctrl *)  	 0xFFD16000)
#endif COBRA

#define ETHER_CR_INT   (0x10) /* Ethernet Control Reg Enable Interrupt bit.  */
#define ETHER_CR_CAT   (0x20) /* Ethernet Control Reg Channel Attention bit. */
#define ETHER_CR_LOOPB (0x30) /* Ethernet Control Reg Loop Back bit.         */
#define ETHER_CR_RESET (0x40) /* Ethernet Control Reg RESET bit.             */

#define	VIDEOMEM_BASE ((char *) 0xFFD40000) /* Video memory. */

#define ETHER_SCP_OFF 0xfffffff6 /* Virtual address for ethernet SCP. */

#ifdef COBRA
#define P4_OVERLAY_BASE		VIDEOMEM_BASE
#define BW_ENABLE_MEM_BASE	((char *)	0xFFD60000)  /*  TEMP addr */
#define BW_ENABLE_MEM_SIZE			0x40000
#define COLOR_FB_BASE		((char *)	0xFFC00000)
#define COLORMAP_BASE		((char *)	0xFFD0E000)
#endif COBRA

#ifdef FIRMWARE
/*
 * The physical addresses are used in help menu (bmchelp.c) only.
 */
#define PA_KEYBMOUSE		0xF0000000
#define PA_SERIAL0_BASE		0xF1000000
#define PA_EEPROM_BASE		0xF2000000
#define PA_CLOCK_BASE		0xF3000000
#define PA_MEMORY_ERR_BASE	0xF4000000
#define PA_INTERRUPT_BASE	0xF5000000
#define PA_ETHER_BASE		0xF6000000
#define PA_COLORMAP_BASE	0xF7000000
#define PA_SCSI_BASE		0xa000000
#define PA_DES_BASE		0xFE000000
#define PA_ECC_CTRL_BASE	0xFF1E0000
#define PA_VIDEOMEM_BASE	0xFD000000

#endif FIRMWARE

/* 
 * Handy place to map the invalid PMEG.  Also red zone for stack. 
 */
#define	INVPMEG_BASE ((char *) 0xFFD80000)

/*
 * Monitor scratch RAM.  One page for the STACK.  
 * One page for the TRAP VECTOR TABLE and FONT TABLE.
 */
#define STACK_BASE          ((char *) 0xFFDC0000)
#define STACK_TOP           ((char *) 0xFFDC2000)
#define TRAP_VECTOR_BASE             (0xFFDC2000)
#define FONT_BASE           ((char *) 0xFFDC2400)

#define NUMBER_OF_TRAPS               256
/*
 * These definitions allow easy and accurate access into the TRAP VECTOR TABLE.
 */
#define HARD_RESET                    (TRAP_VECTOR_BASE)
#define INSTRUCTION_ACCESS_EXCEPTION  (TRAP_VECTOR_BASE+4)
#define ILLEGAL_INSTRUCTION           (TRAP_VECTOR_BASE+2*4)
#define PRIVELEGED_INSTRUCTION        (TRAP_VECTOR_BASE+3*4)
#define FP_DISABLED                   (TRAP_VECTOR_BASE+4*4)
#define WINDOW_OVERFLOW               (TRAP_VECTOR_BASE+5*4)
#define WINDOW_UNDERFLOW              (TRAP_VECTOR_BASE+6*4)
#define MEMORY_ADDRESS_NOT_ALIGNED    (TRAP_VECTOR_BASE+7*4)
#define FP_EXCEPTION                  (TRAP_VECTOR_BASE+8*4)
#define DATA_ACCESS_EXCEPTION         (TRAP_VECTOR_BASE+9*4)
#define TAG_OVERFLOW                  (TRAP_VECTOR_BASE+10*4)
#define INTERRUPT_LEVEL_1             (TRAP_VECTOR_BASE+17*4)
#define INTERRUPT_LEVEL_2             (TRAP_VECTOR_BASE+18*4)
#define INTERRUPT_LEVEL_3             (TRAP_VECTOR_BASE+19*4)
#define INTERRUPT_LEVEL_4             (TRAP_VECTOR_BASE+20*4)
#define INTERRUPT_LEVEL_5             (TRAP_VECTOR_BASE+21*4)
#define INTERRUPT_LEVEL_6             (TRAP_VECTOR_BASE+22*4)
#define INTERRUPT_LEVEL_7             (TRAP_VECTOR_BASE+23*4)
#define INTERRUPT_LEVEL_8             (TRAP_VECTOR_BASE+24*4)
#define INTERRUPT_LEVEL_9             (TRAP_VECTOR_BASE+25*4)
#define INTERRUPT_LEVEL_10            (TRAP_VECTOR_BASE+26*4)
#define INTERRUPT_LEVEL_11            (TRAP_VECTOR_BASE+27*4)
#define INTERRUPT_LEVEL_12            (TRAP_VECTOR_BASE+28*4)
#define INTERRUPT_LEVEL_13            (TRAP_VECTOR_BASE+29*4)
#define INTERRUPT_LEVEL_14            (TRAP_VECTOR_BASE+30*4)
#define INTERRUPT_LEVEL_15            (TRAP_VECTOR_BASE+31*4)
#define SETBUS                        (TRAP_VECTOR_BASE+254*4)
#define ABORT                         (TRAP_VECTOR_BASE+255*4)

/*
 * RAM page used to hold the valid main memory pages bitmap if memory
 * fails self-test.  This will be mapped invalid if memory was all OK.
 */
#define	MAINMEM_BITMAP ((char *) 0xFFDC2000) /* ??? WRONG! chg later. */

/*
 * Roughly 512K for mapping in devices and main memory during boot.
 * Mapped invalid otherwise (to Invalid PMEG if a whole pmeg invalid).
 */
#define	BOOTMAP_BASE ((char *) 0xFFDC4000) /* ??? WRONG! chg later. */

/*
 * Location of EPROM code.  The system executes out of these addresses.
 * If the value of "PROM_BASE", defined below, is changed, several other
 * changes are required.  A complete list of required changes is given below.
 *      (1) Makefile:             RELOC=
 *      (2) ../h/sunromvec.h:     #define romp
 */
#define	PROM_BASE ((struct sunromvec *) 0xFFE80000)

/*
 * First hardware virtual address where DVMA is possible.
 * The Monitor does not normally use this address range, but 
 * does use it during bootstrap, via the resalloc() routine.
 */
#define	DVMA_BASE ((char *) 0xFFF00000)

/*
 * The Monitor maps only as much main memory as it can detect.  The rest
 * of the address space (up through the special addresses defined above)
 * is mapped as invalid.
 *
 * The last pmeg in the page map is always filled with PME_INVALID
 * entries.  This pmeg number ("The Invalid Pmeg Number") can be used to
 * invalidate an entire segment's worth of memory.
 *		B E   C A R E F U L !
 * If you change a page map entry in this pmeg, you change it for thousands
 * of virtual addresses.  (The standard "getpagemap"/"setpagemap" routines
 * will cause a trap if you attempt to write a valid page map entry to this
 * pmeg, but you could do it by hand if you really wanted to mess things up.)
 *
 * Because there is eight times as much virtual memory space in a single 
 * context as there are total pmegs to map it with, much of the monitor's
 * memory map must be re-mappings of the same pmegs.  Specifically, 
 * NSEGMAP/NUMPMEGS different segment table entries map to each pmeg 
 * of the page table.  There is no reason to duplicate useful addresses, 
 * and several reasons not to, so we map the extra virtual address space 
 * with the Invalid Pmeg Number.  This means that some of the address space 
 * has their own page map entries, and the other part all shares the one 
 * Invalid pmeg.  Remember this when trying to map things; if the address 
 * you want is seg-mapped to the Invalid pmeg, you had better find it a 
 * pmeg before you set a page map entry.
 *
 * The Monitor always uses page map entry PME_INVALID to map an invalid
 * page, although the only relevant bits are the (nonzero) permission bits
 * and the (zero) Valid bit.  PME_INVALID is defined in ./structconst.h,
 * which is generated by the Monitor makefile.
 */
#define	SEG_INVALID NUMPMEGS-1
#define EEPROM ((struct eeprom *) 0xFFD04000)
