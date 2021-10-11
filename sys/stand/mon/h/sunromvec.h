/*
 * sunromvec.h
 *
 * @(#)sunromvec.h 1.1 92/07/30 SMI
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef _mon_sunromvec_h
#define _mon_sunromvec_h

#include <sys/types.h>
#include <sys/param.h>

/*
 * For sun4c and later, only Version 0 stuff is coming out of this file,
 * everything else is in <mon/openprom.h>.
 */

#if defined(sun4c) || defined(sun3x)
/*
 * This structure defines a segment of physical memory. To support
 * sparse physical memory, the PROMs construct a list of these structures
 * representing what memory is present. On other machines, the kernel will
 * fake up the physical memory list without this structure.
 */
struct physmemory {
        unsigned int address;
        unsigned int size;
	struct physmemory *next;
};
#endif sun4c || sun3x

#ifdef OPENPROMS
#include <mon/openprom.h>
#else OPENPROMS

/*
 * Autoconfig operations
 */
struct config_ops {
	int (*devr_next)(/* int nodeid */);
	int (*devr_child)(/* int nodeid */);
	int (*devr_getproplen)(/* int nodeid; caddr_t name; */);
	int (*devr_getprop)(/* int nodeid; caddr_t name; addr_t value; */);
	int (*devr_setprop)(/* int nodeid; caddr_t name; addr_t value; int len; */);
	int (*devr_nextprop)(/* int nodeid; caddr_t previous; */);
};

/*
 * This file defines the entire interface between the ROM 
 * Monitor and the programs (or kernels) that run under it.  
 * 
 * The main Sun-2 and Sun-3 interface consists of
 * the VECTOR TABLE at the front of the Boot PROM.
 * 
 * The main Sun-4 interface consists of (1) the VECTOR TABLE and (2) the 
 * TRAP VECTOR TABLE, near the front of the Boot PROM.  Beginning at address
 * "0x00000000", there is a 4K-byte TRAP TABLE containing 256 16-byte entries.
 * Each 16-byte TRAP TABLE entry contains the executable code associated with
 * that trap.  The initial 128 TRAP TABLE entries are dedicated to hardware
 * traps while, the final 128 TRAP TABLE entries are reserved for programmer-
 * initiated traps.  With a few exceptions, the VECTOR TABLE, which appeared
 * in Sun-2 and Sun-3 firmware, follows the TRAP TABLE.  Finally, the TRAP 
 * VECTOR TABLE follows the VECTOR TABLE.  Each TRAP VECTOR TABLE entry 
 * contains the address of the trap handler, which is eventually called to 
 * handle the trap condition.
 *
 * These tables are the ONLY knowledge the outside world has of this rom.
 * They are referenced by hardware and software.  Once located, NO ENTRY CAN
 * BE ADDED, DELETED or RE-LOCATED UNLESS YOU CHANGE THE ENTIRE WORLD THAT
 * DEPENDS ON IT!  Notice that, for Sun-4, EACH ENTRY IN STRUCTURE "sunromvec"
 * MUST HAVE A CORRESPONDING ENTRY IN VECTOR TABLE "vector_table", which 
 * resides in file "../sun4/traptable.s".
 * 
 * The easiest way to reference elements of these TABLEs is to say:
 *      *romp->xxx
 * as in:
 *      (*romp->v_putchar)(c);
 *
 * Various entries have been added at various times.  As of the Rev N, the
 * VECTOR TABLE includes an entry "v_romvec_version" which is an integer 
 * defining which entries in the table are valid.  The "V1:" type comments 
 * on each entry state which version the entry first appeared in.  In order
 * to determine if the Monitor your program is running under contains the 
 * entry, you can simply compare the value of "v_romvec_version" to the 
 * constant in the comment field.  For example,
 *      if (romp->v_romvec_version >= 1) {
 *        reference *romp->v_memorybitmap...
 *      } else {
 *        running under older version of the Monitor...
 *      }
 * Entries which do not contain a "Vn:" comment are in all versions.
 */
struct sunromvec {
  char               *v_initsp;        /* Initial Stack Pointer for hardware.*/
  void               (*v_startmon)();  /* Initial PC for hardware.           */
  int                *v_diagberr;      /* Bus error handler for diagnostics. */
  /* 
   * Configuration information passed to standalone code and UNIX. 
   */
  struct   bootparam **v_bootparam;    /* Information for boot-strapped pgm. */
  unsigned int       *v_memorysize;    /* Total physical memory in bytes.    */
  /* 
   * Single character input and output.
   */
  unsigned char      (*v_getchar)();   /* Get a character from input source. */
  void               (*v_putchar)();   /* Put a character to output sink.    */
  int                (*v_mayget)();    /* Maybe get a character, or "-1".    */
  int                (*v_mayput)();    /* Maybe put a character, or "-1".    */
  unsigned char      *v_echo;          /* Should "getchar" echo input?       */
  unsigned char      *v_insource;      /* Current source of input.           */
  unsigned char      *v_outsink;       /* Currrent output sink.              */
  /* 
   * Keyboard input and frame buffer output.
   */
  int                (*v_getkey)();    /* Get next key if one is available.  */
  void               (*v_initgetkey)();/* Initialization for "getkey".       */
  unsigned int       *v_translation;   /* Keyboard translation selector.     */
  unsigned char      *v_keybid;        /* Keyboard ID byte.                  */
  int                *v_screen_x;      /* V2: Screen x pos (R/O).            */
  int                *v_screen_y;      /* V2: Screen y pos (R/O).            */
  struct keybuf      *v_keybuf;        /* Up/down keycode buffer.            */

  char               *v_mon_id;        /* Revision level of the monitor.     */
  /* 
   * Frame buffer output and terminal emulation.
   */
  void               (*v_fwritechar)();/* Write a character to frame buffer. */
  int                *v_fbaddr;        /* Address of frame buffer.           */
  char               **v_font;         /* Font table for frame buffer.       */
  void               (*v_fwritestr)(); /* Quickly write a string to frame    *
                                        * buffer.                            */
  /* 
   * Re-boot interface routine.  Resets and re-boots system.  No return. 
   */
  void               (*v_boot_me)();   /* For example, boot_me("xy()vmunix").*/
  /* 
   * Command line input and parsing.
   */
  unsigned char      *v_linebuf;       /* The command line buffer.           */
  unsigned char      **v_lineptr;      /* Current pointer into "linebuf".    */
  int                *v_linesize;      /* Length of current command line.    */
  void               (*v_getline)();   /* Get a command line from user.      */
  unsigned char      (*v_getone)();    /* Get next character from "linebuf". */
  unsigned char      (*v_peekchar)();  /* Peek at next character without     *
                                        * advancing pointer.                 */
  int                *v_fbthere;       /* Is there a frame buffer or not?    *
                                        * 1=yes.                             */
  int                (*v_getnum)();    /* Grab hex number from command line. */
  /* 
   * Phrase output to current output sink.
   */
  int                (*v_printf)();    /* Similar to Kernel's "printf".      */
  void               (*v_printhex)();  /* Format N digits in hexadecimal.    */

  unsigned char      *v_leds;          /* RAM copy of LED register value.    */
  void               (*v_set_leds)();  /* Sets LEDs and RAM copy             */
  /* 
   * The nmi related information. 
   */
  void               (*v_nmi)();       /* Address for the Sun-4 level 14     *
                                        * interrupt vector.                  */
  void               (*v_abortent)();  /* Entry for keyboard abort.          */
  int                *v_nmiclock;      /* Counts in milliseconds.            */

  int                *v_fbtype;        /* Frame buffer type: see <sun/fbio.h>*/
  /* 
   * Assorted other things.
   */
  unsigned int       v_romvec_version; /* Version number of "romvec".        */
  struct   globram   *v_gp;            /* Monitor's global variables.        */
  struct zscc_device *v_keybzscc;      /* Address of keyboard in use.        */
  int                *v_keyrinit;      /* Millisecs before keyboard repeat.  */
  unsigned char      *v_keyrtick;      /* Millisecs between repetitions.     */
  unsigned int       *v_memoryavail;   /* V1: Size of usable main memory.    */
  long               *v_resetaddr;     /* where to jump on a RESET trap.     */
  long               *v_resetmap;      /* Page map entry for "resetaddr".    */
  void               (*v_exit_to_mon)();/* Exit from user program.           */
  unsigned char      **v_memorybitmap; /* V1: Bit map of main memory or NULL.*/
#ifndef sun3x
  void               (*v_setcxsegmap)();/* Set segment in any context.       */
#endif sun3x
  void               (**v_vector_cmd)();/* V2: Handler for the 'w' (vector)  *
                                        * command.                           */
#if defined(sun4) || defined(sun4c)
  unsigned long      *v_exp_trap_signal;/* V3: Location of the expected trap *
                                        * signal.  Was trap expected or not? */
  unsigned long      *v_trap_vector_table_base; /* V3: Address of the TRAP   *
                                        * VECTOR TABLE which exists in RAM.  */
#endif sun4 || sun4c
#ifdef sun4c
  struct physmemory  *v_physmemory;	/* ptr to memory list */
  unsigned int       *v_monmemory;	/* memory taken by monitor */
  struct config_ops  *v_config_ops;	/* configuration operations */
#endif sun4c

/* XXX - does sun4m really need this too? */
#if defined(sun3x) || defined(sun4m)
  int                **v_lomemptaddr;    /* address of low memory ptes      */
  int                **v_monptaddr;      /* address of debug/mon ptes       */
  int                **v_dvmaptaddr;     /* address of dvma ptes            */
  int		         **v_monptphysaddr;  /* Physical Addr of the KADB PTE's */
  int                **v_shadowpteaddr;  /* addr of shadow cp of DVMA pte's */
  struct physmemory  *v_physmemory;      /* Ptr to memory list for Hydra    */
#endif sun3x || sun4m
  int                dummy1z;
  int                dummy2z;
  int                dummy3z;
  int                dummy4z;
};

/*
 * THE FOLLOWING CONSTANT, "romp" MUST BE CHANGED ANYTIME THE VALUE OF 
 * "PROM_BASE" IN file "../sun2/cpu.addrs.h" (for Sun-2), file 
 * "../sun3/cpu.addrs.h" (for Sun-3) or file "../sun4/cpu.addrs.h" (for Sun-4)
 * IS CHANGED.  IT IS CONSTANT HERE SO THAT EVERY MODULE WHICH NEEDS AN ADDRESS
 * OUT OF STRUCTURE "sunromvec" DOES NOT HAVE TO INCLUDE the appropriate
 * "cpu.addrs.h" file.
 *
 * Since Sun-4 supports 32-bit addressing, rather than 28-bit addressing as is
 * supported by Sun-3, the value of "romp" had to be increased.  Furthermore, 
 * since the VECTOR TABLE, which appeared at the beginning of Sun-3 firmware,
 * now appears after the 4K-byte TRAP TABLE in Sun-4 firmware, the value of
 * "romp" was incresed by an additional 4K.
 *
 * If the value of "romp" is changed, several other changes are required.
 * A complete list of required changes is given below.
 *    (1) Makefile:             RELOC=
 *    (2) ../sun2/cpu.addrs.h:  #define PROM_BASE (for Sun-2)
 * or
 *    (2) ../sun3/cpu.addrs.h:  #define PROM_BASE (for Sun-3)
 * or
 *    (2) ../sun3x/cpu.addrs.h:  #define PROM_BASE (for Sun-3x)
 * or
 *    (2) ../sun4/cpu.addrs.h:  #define PROM_BASE (for Sun-4)
 *
 */
#ifdef sun4c
extern struct sunromvec *romp;
#else sun4c
#ifdef sun4
#define romp ((struct sunromvec *) 0xFFE81000) /* Used when running the  *
                                                * firmware out of ROM.   */
#else sun4
/*
 * Notce that the value of "romp" will be 
 * truncated based on the running hardware:
 *   Sun-2   0x00EF0000
 *   Sun-3   0x0FEF0000
 *   Sun-3x  0xFEFE00000
 * This was deliberately done for Sun-3 so that programs using this header
 * file (with the Sun-3 support) would continue to run on Sun-2 systems.
 */
#ifdef sun3x
#define romp ((struct sunromvec *) 0xFEFE0000)	/* Pegasus romp-> */
#else sun3x
#define romp ((struct sunromvec *) 0x0FEF0000)
#endif sun3x
#endif sun4 
#endif sun4c
#endif OPENPROMS

/*
 * The possible values for "*romp->v_insource" and "*romp->v_outsink" are 
 * listed below.  These may be extended in the future.  Your program should
 * cope with this gracefully (e.g. by continuing to vector through the ROM
 * I/O routines if these are set in a way you don't understand).
 */
#define INKEYB    0 /* Input from parallel keyboard. */
#define INUARTA   1 /* Input or output to Uart A.    */
#define INUARTB   2 /* Input or output to Uart B.    */
#define INUARTC   3 /* Input or output to Uart C.    */
#define INUARTD   4 /* Input or output to Uart D.    */
#define OUTSCREEN 0 /* Output to frame buffer.       */
#define OUTUARTA  1 /* Input or output to Uart A.    */
#define OUTUARTB  2 /* Input or output to Uart B.    */
#define OUTUARTC  3 /* Input or output to Uart C.    */
#define OUTUARTD  4 /* Input or output to Uart D.    */

/*
 * Structure set up by the boot command to pass arguments to the booted program.
 */
struct bootparam {
  char            *bp_argv[8];     /* String arguments.                     */
  char            bp_strings[100]; /* String table for string arguments.    */
  char            bp_dev[2];       /* Device name.                          */
  int             bp_ctlr;         /* Controller Number.                    */
  int             bp_unit;         /* Unit Number.                          */
  int             bp_part;         /* Partition/file Number.                */
  char            *bp_name;        /* File name.  Points into "bp_strings". */
  struct boottab  *bp_boottab;     /* Points to table entry for device.     */
};

/*
 * This table entry describes a device.  It exists in the PROM.  A pointer to
 * it is passed in "bootparam".  It can be used to locate ROM subroutines for 
 * opening, reading, and writing the device.  NOTE: When using this interface, 
 * only ONE device can be open at any given time.  In other words, it is not
 * possible to open a tape and a disk at the same time.
 */
struct boottab {
  char           b_dev[2];        /* Two character device name.          */
  int            (*b_probe)();    /* probe(): "-1" or controller number. */
  int            (*b_boot)();     /* boot(bp): "-1" or start address.    */
  int            (*b_open)();     /* open(iobp): "-"1 or "0".            */
  int            (*b_close)();    /* close(iobp): "-"1 or "0".           */
  int            (*b_strategy)(); /* strategy(iobp, rw): "-1" or "0".    */
  char           *b_desc;         /* Printable string describing device. */
  struct devinfo *b_devinfo;      /* Information to configure device.    */
};

enum MAPTYPES { /* Page map entry types. */
  MAP_MAINMEM, 
  MAP_OBIO, 
  MAP_MBMEM, 
  MAP_MBIO,
  MAP_VME16A16D, 
  MAP_VME16A32D,
  MAP_VME24A16D, 
  MAP_VME24A32D,
  MAP_VME32A16D, 
  MAP_VME32A32D
};

/*
 * This table gives information about the resources needed by a device.  
 */
struct devinfo {
  unsigned int      d_devbytes;   /* Bytes occupied by device in IO space.  */
  unsigned int      d_dmabytes;   /* Bytes needed by device in DMA memory.  */
  unsigned int      d_localbytes; /* Bytes needed by device for local info. */
  unsigned int      d_stdcount;   /* How many standard addresses.           */
  unsigned long     *d_stdaddrs;  /* The vector of standard addresses.      */
  enum     MAPTYPES d_devtype;    /* What map space device is in.           */
  unsigned int      d_maxiobytes; /* Size to break big I/O's into.          */
};

/*
 * This following defines the memory map interface
 * between the ROM Monitor and the Unix kernel.
 *
 * The ROM Monitor requires that nobody mess with parts of virtual memory if 
 * they expect any ROM Monitor services.  The following rules apply to all of
 * the virtual addresses between MONSTART and MONEND.
 *   (1) Do not write to these addresses.
 *   (2) Do not read from (depend on the contents of) these addresses, except
 *       as documented here or in <mon/sunromvec.h>.
 *   (3) Do not re-map these addresses.
 *   (4) Do not change or double-map the pmegs that these addresses map through.
 *   (5) Do not change or double-map the main memory that these addresses map 
 *       to.
 *   (6) You are free to change or double-map I/O devices which these addresses
 *       map to.
 *   (7) These rules apply in all map contexts.
 */
#ifndef	OPENPROMS
#ifdef sun2 
#define MONSTART 0x00E00000
#define MONEND   0x00F00000
#endif sun2

#ifdef sun3 
#define MONSTART 0x0FE00000
#define MONEND   0x0FF00000
#endif sun3 

#ifdef sun3x
#define MONSTART 0xFEF00000
#define MONEND   0xFF000000
#endif sun3x

/* XXX - is this right for sun4m? */
#if defined(sun4) || defined(sun4c) || defined(sun4m)
#define MONSTART 0xFFD00000
#define MONEND   0xFFF00000
#endif sun4 || sun4c || sun4m
#endif	OPENPROMS

#if !defined(sun4) && !defined(sun4c)
/*
 * The one page at MONSHORTPAGE must remain mapped to the same piece
 * of main memory.  The pmeg used to map it there can be changed if so
 * desired.  This page should not be read from or written in.  It is
 * used for ROM Monitor globals, since it is addressable with short
 * absolute instructions.  (We give a 32-bit value below so the
 * compiler/assembler will use absolute short addressing.  The hardware
 * will accept either 0 or 0xF as the top 4 bits.)
 */
#define MONSHORTPAGE 0xFFFFE000
#define MONSHORTSEG  0xFFFE0000
#endif !sun4 && !sun4c

/* XXX - is this really right for sun4m? */
#if defined(sun4) || defined(sun4c) || defined(sun4m)
/*
 * The one page at GLOBAL_PAGE must remain mapped to the same piece of main 
 * memory.  The pmeg used to map it there can be changed if desired.  This page
 * should not be read from or written into.  It is used for Monitor globals.
 */
#define GLOBAL_PAGE 0xFFEFE000
#endif sun4 || sun4c || sun4m
#ifdef sun3x
#define GLOBAL_PAGE 0xFEF72000
#endif sun3x

/*
 * For virtual addresses outside the above range, you can re-map the addresses 
 * as desired (but see MONSHORTPAGE).  Any pmeg not referenced by segments 
 * between MONSTART and MONEND can be used freely (but see MONSHORTPAGE).  
 * When a stand-alone program is booted, available main memory will be mapped
 * contiguously, using ascending physical page addresses, starting at virtual
 * address "0x00000000".  The complete set of available memory  may not be  
 * mapped due to a lack of pmegs.  The complete set is defined by globals 
 * "v_memorysize", "v_memoryavail" and "v_memorybitmap".  For non-Sun-4
 * machines, we guarantee that at least MAINMEM_MAP_SIZE page map entries
 * will be available for mapping starting from location "0x0".  For Sun-4, 
 * (at least at the time of Sunrise and Cobra), we guarantee that at least
 * 4 megabytes will be mapped beginning with location "0x0".  When a 
 * stand-alone program is booted, the number of segment table entries required
 * to map DVMA_MAP_SIZE bytes will be allocated.  The corresponding pages will 
 * be mapped into DVMA space.
 */
#ifdef sun2 
#define MAINMEM_MAP_SIZE 0x00600000
#define DVMA_MAP_SIZE    0x00080000
#endif sun2 

#ifdef sun3 
#define MAINMEM_MAP_SIZE 0x00800000
#define DVMA_MAP_SIZE    0x00080000
#endif sun3 

#ifdef sun3x
#define MAINMEM_MAP_SIZE 0x00800000	/* Eight Megabytes */
#define DVMA_MAP_SIZE    0x00100000	/* One Megabyte */
#endif

#ifdef sun4
#ifdef sunray
#define MAINMEM_MAP_SIZE 0x02000000
#else sunray
#define MAINMEM_MAP_SIZE 0x00800000
#endif sunray
#define DVMA_MAP_SIZE    0x00080000
#endif

#ifdef sun4c
#define MAINMEM_MAP_SIZE 0x00400000
#define DVMA_MAP_SIZE    0x00080000
#endif

/* XXX - Is this right for sun4m? */
#ifdef sun4m
#define MAINMEM_MAP_SIZE 0x04000000	/* 64 megabytes! */
#define DVMA_MAP_SIZE    0x00080000
#endif	sun4m

/*
 * The following are included for compatability with previous versions
 * of this header file.  Names containing capital letters have been
 * changed to conform with "Bill Joy Normal Form".  This section provides
 * the translation between the old and new names.  It can go away once
 * Sun-1 applications have been converted over.
 */
#define RomVecPtr       romp
#define v_SunRev        v_mon_id
#define v_MemorySize    v_memorysize
#define v_EchoOn        v_echo
#define v_InSource      v_insource
#define v_OutSink       v_outsink
#define v_InitGetkey    v_initgetkey
#define v_KeybId        v_keybid
#define v_Keybuf        v_keybuf
#define v_FBAddr        v_fbaddr
#define v_FontTable     v_font
#define v_message       v_printf
#define v_KeyFrsh       v_nmi
#define AbortEnt        v_abortent
#define v_RefrCnt       v_nmiclock
#define v_GlobPtr       v_gp
#define v_KRptInitial   v_keyrinit
#define v_KRptTick      v_keyrtick
#define v_ExitOp        v_exit_to_mon
#define v_fwrstr        v_fwritestr
#define v_linbuf        v_linebuf

#endif /*!_mon_sunromvec_h*/
