	.ident	"@(#)locore.s 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#undef	SYS_TRAP_CHECKS_CPUID
#undef	SYS_RTT_CHECKS_CPUID

#define	CACHE_BYTES	65536	/* for Ross 605 flush workaround code */

#include <sys/param.h>
#include <sys/vmparam.h>
#include <sys/errno.h>
#include <sys/resource.h>
#include <netinet/in_systm.h>
#include <machine/asm_linkage.h>
#include <machine/buserr.h>
#include <machine/clock.h>
#include <machine/cpu.h>
#include <machine/intreg.h>
#include <machine/async.h>
#include <machine/memerr.h>
#include <machine/eeprom.h>
#include <machine/mmu.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/scb.h>
#include <machine/auxio.h>

#include "assym.s"

#include <machine/iommu.h>

#ifdef IOC
#include <machine/iocache.h>
#endif IOC


#include "percpu_def.h"
#include "percpu.s"

	.seg	"data"

! MINFRAME and the register offsets in struct regs must add up to allow
! double word loading of struct regs. PCB_WBUF must also be aligned.

#if	(MINFRAME & 7) == 0 || (G2 & 1) == 0 || (O0 & 1) == 0
ERROR - struct regs not aligned
#endif
#if	(PCB_WBUF & 7)
ERROR - pcb_wbuf not aligned
#endif

/*
 * Absolute external symbols.
 *
 * On the sun4m we put the message buffer in the third and fourth pages.
 * We set things up so that the first 2 pages of KERNELBASE is illegal
 * to act as a redzone during copyin/copyout type operations. One of
 * the reasons the message buffer is allocated in low memory to
 * prevent being overwritten during booting operations (besides
 * the fact that it is small enough to share pages with others).
 */
	.global _DVMA, _msgbuf
_DVMA	= DVMABASE			! address of DVMA area
_msgbuf = KERNELBASE + (2 * NBPG)	! address of printf message buffer

#ifdef	PROVIDE_SYMBOLS
/*
 * these need to be chased, commented, and
 * perhaps even expanded to symbols for each
 * register in the mapped areas.
 */
_4m_iommu		= V_IOMMU_ADDR
_4m_sbusctl		= V_SBUSCTL_ADDR
_4m_memerr		= V_MEMERR_ADDR
_4m_auxio		= V_AUXIO_ADDR
#ifdef	VME
_4m_vmectl		= V_VMECTL_ADDR
#endif	VME
_4m_interrupt		= V_INTERRUPT_ADDR
_4m_counter		= V_COUNTER_ADDR
_4m_eeprom		= V_EEPROM_ADDR
_4m_prom		= 0xFFD00000
_4m_prom_scb		= 0xFFEFA000
#endif	PROVIDE_SYMBOLS

#if	MSGBUFSIZE > (2 * NBPG)
ERROR - msgbuf too large
#endif

#if	USIZE-KERNSTACK >= 4096
ERROR - user area too large
#endif

/*
 * Define some variables used by post-mortem debuggers
 * to help them work on kernels with changing structures.
 */
	.global UPAGES_DEBUG, KERNELBASE_DEBUG, VADDR_MASK_DEBUG
	.global PGSHIFT_DEBUG, SLOAD_DEBUG

UPAGES_DEBUG		= UPAGES
KERNELBASE_DEBUG	= KERNELBASE
VADDR_MASK_DEBUG	= 0xffffffff
PGSHIFT_DEBUG		= PGSHIFT
SLOAD_DEBUG		= SLOAD

/*
 * Since we don't fix the UADDR anymore, we can recycle that
 * address and use it's pmeg to map stuff. We choose to use it
 * to map the onboard devices. In order to hardwire an address
 * into an instruction, we'll use the same mnemnonic here.
 */
#define OBADDR	(0-(NBPG*UPAGES))

/*
 * The interrupt stack. This must be the first thing in the data
 * segment (other than an sccs string) so that we don't stomp
 * on anything important during interrupt handling. We get a
 * red zone below this stack for free when the kernel text is
 * write protected. The interrupt entry code assumes that the
 * interrupt stack is at a lower address than
 * both eintstack and the kernel stack in the u area.
 */

#ifndef MULTIPROCESSOR
	.align	8
	.global intstack, eintstack
intstack:				! bottom of interrupt stack
	.skip	INTSTACKSIZE
eintstack:				! end (top) of interrupt stack
	.global exitstack, eexitstack
exitstack:				! bottom of exit stack
	.skip	INTSTACKSIZE
eexitstack:				! end (top) of exit stack

	.global _ubasic			! the primordial uarea
_ubasic:
	.skip	KERNSTACK
	.global ekernstack
ekernstack:
	.global _p0uarea		! so we have a "struct user" available
_p0uarea:
	.skip	USIZE

	.global	intu
intu:					! a fake uarea for the kernel
	.skip	USIZE			! when a process is exiting
#else	MULTIPROCESSOR
! %%% TODO: dynamicly allocate percpu pages from physmem
	! allocate pages for each CPUs PERCPU area.  note that
	! cpu0 (ie. getprocessorid() returns 0) uses the first
	! PERCPU_PAGES, cpu 1 uses the next PERCPU_PAGES, etc.

!!!	.align	4096	! must be page aligned, but .align can't do it for us.
	.global _cpu0
_cpu0:
	.skip	PERCPU_PAGES*PAGESIZE*NCPU
#endif	MULTIPROCESSOR

	.global _kptstart
_kptstart:
#ifndef MULTIPROCESSOR
	! allocate space for a level-1 page table.
	.skip	(KL1PT_SIZE*NL1PTEPERPT)	!alignment slop
	.skip	(KL1PT_SIZE*NL1PTEPERPT)
#else	MULTIPROCESSOR
	! allocate a kernel level-1 page table for each CPU.
	! note that the PERCPU implementation depends upon these
	! level 1 page tables being aligned on a page boundary,
	! even though a level 1 page table is smaller than a
	! page.
	.skip	PAGESIZE		!alignment slop
	.skip	PAGESIZE*NCPU
#endif	MULTIPROCESSOR
	! allocate level 2 page tables.
	.skip	KL2PT_SIZE*NKL2PTS*NL2PTEPERPT
_kptend:

	.global _kclock14_vec, _mon_clock14_vec
	.align	8	! must be doubleword aligned
_kclock14_vec:
	.word	0,0,0,0
_mon_clock14_vec:
	.word	0,0,0,0

	.word	0	! paranoia.
/*
 * System software page tables
 */

#define	vaddr_h(x)	((((x)-_Heapptes)/4)*NBPG + HEAPBASE)
#define	HEAPMAP(mname, vname, npte)	\
	.global	mname;			\
mname:	.skip	(4*npte);		\
	.global	vname;			\
vname = vaddr_h(mname);

	HEAPMAP(_Heapptes 	,_Heapbase	,HEAPPAGES	)
	HEAPMAP(_Eheapptes	,_Heaplimit	,0		)
	HEAPMAP(_Bufptes 	,_Bufbase	,BUFPAGES	)
	HEAPMAP(_Ebufptes	,_Buflimit	,0		)

#define vaddr(x)	((((x)-_Sysmap)/4)*NBPG + SYSBASE)
#define SYSMAP(mname, vname, npte)	\
	.global mname;			\
mname:	.skip	(4*npte);		\
	.global vname;			\
vname = vaddr(mname);
	SYSMAP(_Sysmap	 ,_Sysbase	,SYSPTSIZE	)
	SYSMAP(_Mbmap	 ,_mbutl	,MBPOOLMMUPAGES )
	SYSMAP(_CMAP1	 ,_CADDR1	,1		) ! local tmp
	SYSMAP(_CMAP2	 ,_CADDR2	,1		) ! local tmp
	SYSMAP(_mmap	 ,_vmmap	,1		)
	SYSMAP(_ESysmap	 ,_Syslimit	,0		) ! must be last

	.global _Syssize
_Syssize = (_ESysmap-_Heapptes)/4

#ifdef	NOPROM
	.global _availmem
_availmem:
	.word	0
#endif	NOPROM

#ifndef MULTIPROCESSOR
	.global _uunix
_uunix:
	.word	_p0uarea
#endif	MULTIPROCESSOR

	.global	_bad_fault_at
_bad_fault_at:
	.word	0xDEADBEEF

#ifdef	LWP
#ifdef	MULTIPROCESSOR
	.global _force_lwps
_force_lwps:
	.word	0
#endif	MULTIPROCESSOR
#endif	LWP

/*
 * Opcodes for instructions in PATCH macros
 */
#define MOVPSRL0	0xa1480000
#define MOVL4		0xa8102000
#define BA		0x10800000
#define NO_OP		0x01000000

/*
 * Trap vector macros.
 *
 * A single kernel that supports machines with differing
 * numbers of windows has to write the last byte of every
 * trap vector with NW-1, the number of windows minus 1.
 * It does this at boot time after it has read the implementation
 * type from the psr.
 *
 * NOTE: All trap vectors are generated by the following macros.
 * The macros must maintain that a write to the last byte to every
 * trap vector with the number of windows minus one is safe.
 */
#define	TRAPCOM(H,T)	mov %psr, %l0; sethi %hi(H),%l3; jmp %l3+%lo(H); mov (T),%l4
#define TRAP(H)		TRAPCOM(H,0)
#define WIN_TRAP(H)	TRAPCOM(H,0)
#define SYS_TRAP(T)	TRAPCOM(sys_trap,T)
#define TRAP_MON(T)	TRAPCOM(trap_mon,T)

#ifdef  SIMUL
/*
 * For hardware simulator, want to double trap on any unknown trap
 * (which is usually a call into SAS)
*/
#define BAD_TRAP \
        mov %psr, %l0; mov %tbr, %l4; t 0; nop;
#else
#define BAD_TRAP        SYS_TRAP((. - _start) >> 4);
#endif


/*
 * Trap vector table.
 * This must be the first text in the boot image.
 *
 * The first entry in this list is only used once during initial
 * startup, and is replaced with a BAD_TRAP entry before we even
 * change our %tbr to point at this scb. It must be relative, as
 * it is used during initial startup to get to the real code before
 * we are mapped in at KERNELBASE.
 *
 * When a trap is taken, we vector to KERNELBASE+(TT*16) and we have
 * the following state:
 *	2) traps are disabled
 *	3) the previous state of PSR_S is in PSR_PS
 *	4) the CWP has been incremented into the trap window
 *	5) the previous pc and npc is in %l1 and %l2 respectively.
 *
 * Registers:
 *	%l0 - %psr immediately after trap
 *	%l1 - trapped pc
 *	%l2 - trapped npc
 *	%l3 - wim (sys_trap only)
 *	%l4 - system trap number (sys_trap only)
 *	%l6 - number of windows - 1
 *	%l7 - stack pointer (interrupts and system calls)
 *
 * Note: UNIX receives control at vector 0 (trap)
 */
	.seg	"text"
	.align	4

	.global _start, _scb
_start:
_scb:
	b entry ; nop ; nop ; nop		! 00 - reset
	SYS_TRAP(T_FAULT | T_TEXT_FAULT);	! 01 - instruction access
	TRAP(multiply_check);			! 02 - illegal instruction
	SYS_TRAP(T_PRIV_INSTR);			! 03 - privileged instruction
	SYS_TRAP(T_FP_DISABLED);		! 04 - floating point disabled
	WIN_TRAP(window_overflow);		! 05
	WIN_TRAP(window_underflow);		! 06
	SYS_TRAP(T_ALIGNMENT);			! 07 - alignment fault.
	SYS_TRAP(T_FP_EXCEPTION);		! 08
	SYS_TRAP(T_FAULT | T_DATA_FAULT);	! 09 - data fault
	BAD_TRAP;				! 0A - tag_overflow
	BAD_TRAP;				! 0B
	BAD_TRAP;				! 0C
	BAD_TRAP;				! 0D
	BAD_TRAP;				! 0E
	BAD_TRAP;				! 0F
	BAD_TRAP;				! 10
	SYS_TRAP(T_INTERRUPT | 1);		! 11
	SYS_TRAP(T_INTERRUPT | 2);		! 12
	SYS_TRAP(T_INTERRUPT | 3);		! 13
	SYS_TRAP(T_INTERRUPT | 4);		! 14
	SYS_TRAP(T_INTERRUPT | 5);		! 15
	SYS_TRAP(T_INTERRUPT | 6);		! 16
	SYS_TRAP(T_INTERRUPT | 7);		! 17
	SYS_TRAP(T_INTERRUPT | 8);		! 18
	SYS_TRAP(T_INTERRUPT | 9);		! 19
	SYS_TRAP(T_INTERRUPT | 10);		! 1A
	SYS_TRAP(T_INTERRUPT | 11);		! 1B
	SYS_TRAP(T_INTERRUPT | 12);		! 1C
	SYS_TRAP(T_INTERRUPT | 13);		! 1D
#ifndef GPROF
	SYS_TRAP(T_INTERRUPT | 14);		! 1E
#else	GPROF
	TRAP(test_prof);			! 1E
#endif	GPROF
	SYS_TRAP(T_INTERRUPT | 15);		! 1F

/*
 * The rest of the traps in the table up to 0x80 should 'never'
 * be generated by hardware.
 */
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 20 - 23
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 24 - 27 (24 - coprocessor disabled)
	BAD_TRAP; BAD_TRAP;			! 28 - 29 (28 - coprocessor exception)
	BAD_TRAP;				! 2A	(Integer divide by 0)
	SYS_TRAP(T_FAULT | T_DATA_STORE);	! 2B	(Data Store exception)
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 2C - 2F
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 30 - 33
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 34 - 37
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 38 - 3B
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 3C - 3F
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 40 - 43
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 44 - 47
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 48 - 4B
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 4C - 4F
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 50 - 53
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 54 - 57
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 58 - 5B
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 5C - 5F
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 60 - 63
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 64 - 67
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 68 - 6B
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 6C - 6F
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 70 - 73
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 74 - 77
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 78 - 7B
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 7C - 7F

/*
 * User generated traps
 */
	SYS_TRAP(T_SYSCALL);			! 80 - system call
	SYS_TRAP(T_BREAKPOINT);			! 81 - user breakpoint
	SYS_TRAP(T_DIV0);			! 82 - divide by zero
	SYS_TRAP(T_FLUSH_WINDOWS);		! 83 - flush windows
	TRAP(clean_windows);			! 84 - clean windows
	BAD_TRAP;				! 85 - range check
	TRAP(fix_alignment);			! 86 - do unaligned references
	SYS_TRAP(T_INT_OVERFLOW);		! 87 - integer overflow
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 88 - 8B
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 8C - 8F
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 90 - 93
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 94 - 97
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 98 - 9B
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 9C - 9F
	TRAP(getcc);				! A0 - get condition codes
	TRAP(setcc);				! A1 - set condition codes
			    BAD_TRAP; BAD_TRAP; ! A2 - A3
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! A4 - A7
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! A8 - AB
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! AC - AF
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! B0 - B3
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! B4 - B7
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! B8 - BB
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! BC - BF
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! C0 - C3
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP	! C4 - C7
	TRAP(halt_and_catch_fire);		! C8 - halt and catch fire
		  BAD_TRAP; BAD_TRAP; BAD_TRAP; ! C9 - CB
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! CC - CF
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! D0 - D3
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! D4 - D7
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! D8 - DB
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! DC - DF
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! E0 - E3
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! E4 - E7
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! E8 - EB
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! EC - EF
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! F0 - F3
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! F4 - F7
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! F8 - FB
	BAD_TRAP; BAD_TRAP; BAD_TRAP;		! FC - FE
	TRAP_MON(0xff)				! FF - breakpoint
_escb:		! placeholder for "end of scb" for sanity check.

	!
	! here we have a cache-aligned string of non_writeable
	! zeros used mainly by bcopy hardware
	! and by fpu_probe
	!
	.global _zeros
_zeros:
	.word 0,0,0,0,0,0,0,0

/*
 * This small save area is for stashing information
 * directly from trap handlers so we can see it
 * on the logic analyzer. When we are not interesed
 * in seeing PSR,PC,NPC,WIM on the logic analyzer
 * these can go away.
 *
 * NOTE: this stuff is down here so it is at a
 * well-known address; it is only written using
 * mmu-bypass ASI accesses, to eliminate the chance
 * of maybe not having it mapped properly. This
 * also covers the fact that it is in the text
 * segment, and therefore is not mapped writable.
 */
	.global _trapinfo
	.align	8
_trapinfo:			! saved by trap type 1, 2, 9 debug code
	.word	0,0,0,0

/*
 * The global "romp" and "dvec" pointers,
 * as passed to entry by the caller. These
 * are set once during boot, and must not
 * be changed later. Placing them here
 * in the text segment makes that happen
 * quite nicely, thanks.
 */
	.global _romp, _dvec
_romp:	.word	0
_dvec:	.word	0

/*
 * The number of windows, set once on entry.  Note that it is
 * in the text segment so that it is write protected in startup().
 */
	.global _nwindows
_nwindows:
	.word	8

	.global _ctxtabptr
_ctxtabptr:			! physaddr of context table for ctx
	.word	0

	.global _afsrbuf
_afsrbuf:
	.word	0,0,0,0

/************************************************************************
 *	trigger_logan: do something unique.
 */
#ifdef VIKING
/* There might be other ways of getting a trigger from a viking module,
 * we need to put the code in, if that's the case.
 * If vikings also use the same method, we have to make sure the
 * MCNTL.AC bit is turned on when viking with E$ turn on
 */
#endif VIKING

	ALTENTRY(trigger_logan)
	retl
	lduha	[%g0]ASI_MEM, %g0

/************************************************************************
 *	halt_and_catch_fire: trigger a trap #0xC8,
 *	which vectors back here. guaranteed to
 *	cause this processor to go no further,
 *	probably stopping the simulation ;-)
 *	or triggering a watchdog.
 */
#ifdef VIKING
/* There might be other ways of getting a trigger from a viking module,
 * we need to put the code in, if that's the case.
 * If vikings also use the same method, we have to make sure the
 * MCNTL.AC bit is turned on when viking with E$ turn on
 */
#endif VIKING

/*
 * SECURITY NOTE: while this is super useful from a system
 * debugging standpoint, it opens up a bad "denial of service"
 * attack where any user can watchdog the system trivially.
 * Thus the added code that bypasses the watchdogger if
 * a user tries to trigger this via a software trap. The
 * actual trap goes to the RTENTRY point, and supervisor
 * calls from C code go to the ALTENTRY point (which gets
 * an underbar prepended for C).
 */
	RTENTRY(halt_and_catch_fire)
	andcc   %l0, PSR_PS, %g0
	bz,a    sys_trap
	mov     0xC8, %l4

	ALTENTRY(halt_and_catch_fire)
1:
	lduha	[%g0]ASI_MEM, %g0
	ta	0xC8
	b,a	1b

/************************************************************************
 * System initialization
 *
 * Our contract with the boot prom specifies that the MMU is on
 * and the first 16 meg of memory is mapped with a level-1 pte.
 * We are called with romp in %o0 and dvec in %o1; we start
 * execution directly from physical memory, so we need to get
 * up into our proper addresses quickly: all code before we
 * do this must be position independent.
 *
 * Our tasks are:
 *	save parameters
 *	construct mappings for KERNELBASE
 *	hop up into high memory
 *	initialize stack pointer
 *	initialize trap base register
 *	initialize window invalid mask
 *	initialize psr (with traps enabled)
 *	figure out all the module type stuff
 *	tear down the 1-1 mappings
 *	dive into main()
 *
 */
entry:
	!
	! save the incoming arguments in locals in case we need
	! to call the parent's callback routine
	!
	mov	%o0, %l0		! save arg (romp) until bss is clear
	mov	%o1, %l1		! save dvec

	tst	%o2			! if MUNIX (or old proms), this is zero
	be	1f			! only call back if %o2 != 0
	nop

	jmpl	%o2, %o7		! pass return addr in %o0 for
	add	%o7, 8, %o0		! romp->op_chain() function
1:
	mov	%l0, %g7		! now move the locals to globals
	mov	%l1, %g6		! so we can reset the psr/wim

	mov	0x02, %wim		! setup wim
	set	PSR_PIL|PSR_S, %g1
	mov	%g1, %psr		! initialze psr: supervisor, splmax
	wr	%g1, PSR_ET, %psr	! enable traps, too.
	nop ; nop ; nop			! psr delay

	! establish mappings for kernel in upper virtual memory space
	! for now, just copy whatever was in the zero slot in the
	! level-1 page table into the 0xF8 slot.
	!
	! this is only temporary, we will construct a good level-2 table
	! for this mapping later.
	!
	! In case of VIKING:
	! lda	[%g0]ASI_MOD, %o0	! find module type
	! if viking MBUS mode, don't do anything.
	! if viking CC mode, check if cacheable
	! then 1. disable traps; 2. set the AC bit; 3. do ASI_MEM; 
	! 4. clear AC bit 5. enable traps again
	!
	! XXX-Make test changes to routine so it looks for
	! whether we are viking (via %psr, since mcr doesnt have
	! the info), then whether we have E$ (via MB bit).  If so,
	! we set AC bit before doing ASI_MEM.
	! Purposely made simple for clarity in debug.
	! Look to optimize later.
	set	0x40000000, %l0		! IMPL = VIKING (bit in %psr)
	mov	%psr, %l1
	and	%l0, %l1, %l0
	cmp	%l0, %g0
	bz	1f			! not a VIKING
	nop

	lda	[%g0]ASI_MOD, %l0	! find module type
	set	CPU_VIK_MB, %l2
	and	%l0, %l2, %l0		! %l0 keeps the test value
	cmp	%l0, %g0		! viking/E$?
	bz	2f			! yes
	nop

1:	set     RMMU_CTP_REG, %g5       ! this works for all modules
	lda     [%g5]ASI_MOD, %g5       ! get the context table ptr reg
        sll     %g5, 4, %g5             ! convert to phys addr
	lda	[%g5]ASI_MEM, %g5	! get the level-1 table ptr
	srl	%g5, 4, %g5		! strip off PTP bits
	sll	%g5, 8, %g5		! convert to phys addr
	lda	[%g5]ASI_MEM, %g1	! get one-to-one mapping information
	add	%g5, (KERNELBASE>>24)<<2, %g4
	sta	%g1, [%g4]ASI_MEM	! duplicate at KERNELBASE
	b	4f
	nop

2:	andn	%l1, PSR_ET, %g1	! disable traps
	mov	%g1, %psr
	nop; nop; nop			! PSR delay
	set	RMMU_CTP_REG, %g5	! this works for all modules
	lda	[%g5]ASI_MOD, %g5	! get the context table ptr reg
	sll	%g5, 4, %g5		! convert to phys addr
	lda     [%g0]ASI_MOD, %l2       ! get MMU CSR, %l2 keeps the saved CSR
	set     CPU_VIK_AC, %l3         ! AC bit
	or      %l2, %l3, %l3           ! or in AC bit
	sta     %l3, [%g0]ASI_MOD       ! store new CSR

	lda	[%g5]ASI_MEM, %g5	! get the level-1 table ptr
	srl	%g5, 4, %g5		! strip off PTP bits
	sll	%g5, 8, %g5		! convert to phys addr
	lda	[%g5]ASI_MEM, %g1	! get one-to-one mapping information
	add	%g5, (KERNELBASE>>24)<<2, %g4
	sta	%g1, [%g4]ASI_MEM	! duplicate at KERNELBASE
	sta     %l2, [%g0]ASI_MOD       ! restore CSR;
	mov     %l1, %psr               ! restore psr; enable traps again
	nop; nop; nop                   ! PSR delay
	!
	! The correct virtual addresses are now mapped.
	! Do an absolute jump to set the pc to the
	! correct virtual addresses.
	!
4:	set	1f, %g1
	jmp	%g1
	nop
1:
	.global _firsthighinstr
_firsthighinstr:
	!		*** now running in high virtual memory ***
	!
	! save the romp and dvec
	!
	sethi	%hi(_romp), %g1
	st	%g7, [%g1 + %lo(_romp)]
	sethi	%hi(_dvec), %g1
	st	%g6, [%g1 + %lo(_dvec)]

	! Switch to a large stack (prom's may not be big enuf)
	! Must be atomic. May not be the final location ...

	set	ekernstack - SA(MINFRAME + REGSIZE), %g1
#ifdef	MULTIPROCESSOR
	set	_cpu0, %g2
	add	%g1, %g2, %g1		! convert to address inside cpu0
	set	VA_PERCPUME, %g2
	sub	%g1, %g2, %g1
#endif	MULTIPROCESSOR
	mov	%g1, %sp
	mov	0, %fp

#ifdef	MULTIPROCESSOR
	!
	! point the kernel lock at us.
	! Note that the MID may be wrong, we do not
	! know what the %tbr bits are, but at least
	! it will be consistant.
	!
	call	_klock_init
	nop
#endif	MULTIPROCESSOR

! using the boot prom's %tbr, call the temporary load of ptes

	call	_module_setup		! setup correct module routines
	lda	[%g0]ASI_MOD, %o0	! find module type

!
! clear status of any registers that might be latching errors.
!
	call	_mmu_getsyncflt		! clear module I-SFSR
	mov	1, %g6
	call	_mmu_getsyncflt		! clear module D-SFSR
	mov	9, %g6
	sethi	%hi(_afsrbuf), %o0
	call	_mmu_getasyncflt	! clear module AFSR
	or	%o0, %lo(_afsrbuf), %o0

	call	_load_tmpptes		! Routine to load temporary ptes
	nop

! Since we have called module_setup we have the value
! of cache set.  Use it to determine if have E$.
! Set AC bit whenever have viking/e$ and doing ASI_MEM.
!
! %o0 keeps the level-1 table ptr with PTP bits
! switch to our mapping
	set	RMMU_CTP_REG, %g5	! this works for all modules
	lda	[%g5]ASI_MOD, %g5	! get the context table ptr reg
	sll	%g5, 4, %g4		! convert to phys addr

	sethi	%hi(_cache), %l3
	ld	[%l3 + %lo(_cache)], %l3
	cmp	%l3, CACHE_PAC_E
	bne,a	1f
	sta	%o0, [%g4]ASI_MEM

	mov	%psr, %l1
	andn	%l1, PSR_ET, %l3
	mov	%l3, %psr
	nop; nop; nop
	lda	[%g0]ASI_MOD, %l3
	set	CPU_VIK_AC, %l4
	or	%l3, %l4, %l4
	sta	%l4, [%g0]ASI_MOD
	sta	%o0, [%g4]ASI_MEM	! put our level-1 table ptr into ctx tbl
	sta	%l3, [%g0]ASI_MOD
	mov	%l1, %psr
	nop; nop; nop

1:	call	_mmu_setctx		! switch to our mapping
	mov	%g0, %o0
	call	_mmu_flushall
	nop


	call	_early_startup
	nop

#ifdef	MULTIPROCESSOR
	! The PerCPU area is now properly mapped, so we can
	! switch our stack over to it.

	set	ekernstack - SA(MINFRAME + REGSIZE), %g1
	mov	%g1, %sp
#ifdef	VAC
	! since the temporary stack used an alias for the real stack
	! that was probably not aligned, we need to flush the old
	! stuff from the cache to avoid "cache data bombs".

	set	_cpu0, %o0
	set	PERCPU_PAGES*PAGESIZE, %o1
	call	_vac_flush
	nop
#endif
#endif	MULTIPROCESSOR

	!
	! find our trap table. This register remains "live" until
	! we use it to set up %tbr below.
	!
#ifndef	PC_vscb
	set	scb, %g2
#else	PC_vscb
	set	VA_PERCPU+PC_vscb, %g2
#endif	PC_vscb

	!
	! Patch vector 0 trap to "zero" in case it happens again.
	! We use "trap 0xF" as our prototype for BADTRAP.
	!
	ldd	[%g2+0xF0], %o2
	ldd	[%g2+0xF8], %o4
	andn	%o5, 0xFF, %o5		! set trap type to zero
	std	%o2, [%g2+0x00]
	std	%o4, [%g2+0x08]

	!
	! Find the the number of implemented register windows.
	! The last byte of every trap vector must be equal to
	! the number of windows in the implementation minus one.
	! The trap vector macros (above) depend on it!
	!
	mov	%g0, %wim		! note psr has cwp = 0

	save				! decrement cwp, wraparound to NW-1
	mov	%psr, %g1
	and	%g1, PSR_CWP, %g1	! we now have nwindows-1
	restore				! get back to orignal window
	mov	2, %wim			! reset initial wim

	inc	%g1
	sethi	%hi(_nwindows), %g4	! inialize the nwindows variable
	st	%g1, [%g4 + %lo(_nwindows)]

#ifdef	NOPROM
	!
	! If we are in the simulator ask it for the memory size.
	! probably should shift to having the right stuff in romp.
	!
	lda	[%g0]ASI_SAS, %l0	! Load from 0 in ASI_SAS (0x30) will
	sethi	%hi(_availmem), %g1	! return the available amount of memory
	st	%l0, [%g1 + %lo(_availmem)]
#endif

	!
	! Switch to our trap base register
	!
	mov	%tbr, %l4		! save monitor's tbr
	bclr	0xfff, %l4		! remove tt

	!
	! Save monitor's level14 clock interrupt vector code.
	!
	set	_mon_clock14_vec, %o1
	ldd	[%l4+TT(T_INT_LEVEL_14)], %o2
	ldd	[%l4+TT(T_INT_LEVEL_14)+8], %o4
	std	%o2, [%o1]
	std	%o4, [%o1+8]
	!
	! Save monitor's breakpoint vector code, and
	! install it in our trap table.
	!
	set	mon_breakpoint_vec, %o1
	ldd	[%l4+TT(ST_MON_BREAKPOINT+T_SOFTWARE_TRAP)], %o2
	ldd	[%l4+TT(ST_MON_BREAKPOINT+T_SOFTWARE_TRAP)+8], %o4
	std	%o2, [%o1]
	std	%o4, [%o1+8]
	std	%o2, [%g2+TT(ST_MON_BREAKPOINT+T_SOFTWARE_TRAP)]
	std	%o4, [%g2+TT(ST_MON_BREAKPOINT+T_SOFTWARE_TRAP)+8]

#ifdef	MULTIPROCESSOR
	! disable traps while munging %tbr

	mov	%psr, %g1
	andn	%g1, PSR_ET, %g1
	mov	%g1, %psr
	nop ; nop ; nop			! psr delay

#endif	MULTIPROCESSOR

	!
	! Switch from boot prom's trap table to our own
	!
	mov	%g2, %tbr

#ifdef	MULTIPROCESSOR
	!
	! Now that we have the correct %tbr,
	! reinitialize the klock to have the
	! correct owner.
	!
	call	_klock_init
	nop
#endif	MULTIPROCESSOR

	!
	! initialize psr: supervisor, traps enabled, splmax.
	!
	set	PSR_S|PSR_PIL, %g1
	mov	%g1, %psr		! supv,  splmax;
	wr	%g1, PSR_ET, %psr	! enable traps, too.
	nop ; nop ; nop			! psr delay

! invalidate the dual (one-to-one) mapping for low memory,
! we do not need it any more.

! In case of VIKING:
! lda	[%g0]ASI_MOD, %o0	! find module type
! if viking MBUS mode, don't do anything.
! if viking CC mode, check if cacheable
! then 1. disable traps; 2. set the AC bit; 3. do ASI_MEM; 4. clear AC bit
! 5. enable traps again
!
! Use the value of cache variable to determine if have vik/e$
! if so, set the AC bit before doing ASI_MEM.
	sethi	%hi(_cache), %l3
	ld	[%l3 + %lo(_cache)], %l3
	cmp	%l3, CACHE_PAC_E
	bz	2f
	nop

1:	set     RMMU_CTP_REG, %g5       ! this works for all modules
	lda     [%g5]ASI_MOD, %g5       ! get the context table ptr reg
        sll     %g5, 4, %g5             ! convert to phys addr
	lda	[%g5]ASI_MEM, %g5	! get the level-1 table ptr
	srl	%g5, 4, %g5		! strip off PTP bits
	sll	%g5, 8, %g5		! convert to phys addr
	call	_mmu_flushall
	sta	%g0, [%g5]ASI_MEM
	b	3f
	nop

2:	mov	%psr, %l1
	andn	%l1, PSR_ET, %g1	! disable traps
	mov	%g1, %psr
	nop; nop; nop			! PSR delay
	set     RMMU_CTP_REG, %g5       ! this works for all modules
	lda	[%g5]ASI_MOD, %g5	! context table ptr reg
	sll	%g5, 4, %g5		! convert to phys addr
	lda     [%g0]ASI_MOD, %l2       ! get MMU CSR, %l2 keeps the saved CSR
	set     CPU_VIK_AC, %l3         ! AC bit
	or      %l2, %l3, %l3           ! or in AC bit
	sta     %l3, [%g0]ASI_MOD       ! store new CSR

	lda	[%g5]ASI_MEM, %g5	! get the level-1 table ptr
	srl	%g5, 4, %g5		! strip off PTP bits
	sll	%g5, 8, %g5		! convert to phys addr
	sta	%g0, [%g5]ASI_MEM	! null out 1-1 mapping
	call	_mmu_flushall
	sta     %l2, [%g0]ASI_MOD       ! restore CSR;
	mov     %l1, %psr               ! restore psr; enable traps again
	nop; nop; nop                   ! PSR delay
	!
	! Dummy up fake user registers on the stack,
	! so we can dive into user code as if we were
	! returning from a trap.
	!
3:	set	USRSTACK-WINDOWSIZE, %g1
	st	%g1, [%sp + MINFRAME + SP*4] ! user stack pointer
	set	PSL_USER, %g1
	st	%g1, [%sp + MINFRAME + PSR*4] ! psr
	set	USRTEXT, %g1
	st	%g1, [%sp + MINFRAME + PC*4] ! pc
	add	%g1, 4, %g1
	st	%g1, [%sp + MINFRAME + nPC*4] ! npc

	!
	! enable interrupts in sipr
	! by writing all-ones to the
	! sipr mask "clear" location.
	!
	sethi	%hi(V_INT_REGS+0x4008), %g1
	sub	%g0, 1, %g2
	st	%g2, [%g1+%lo(V_INT_REGS+0x4008)]

	!
	! Call main. We will return as process 1 (init).
	!
	call	_main
	add	%sp, MINFRAME, %o0
	!
	! Proceed as if this was a normal user trap.
	!
	b	sys_rtt			! fake return from trap
	nop

	ALTENTRY(cpuid_bad)
	b	_halt_and_catch_fire
	nop

/*
 * Generic system trap handler.
 *
 *	%l0	trap-psr
 *	%l1	trap-pc
 *	%l2	trap-npc
 *	%l3	reserved for %wim
 *	%l4	trap-type
 *	%l5	
 *	%l6	reserved for nwindows - 1
 *	%l7	reserved for stack
 */
	.global _sys_trap
	.global sys_trap
_sys_trap:
sys_trap:
	sethi	%hi(_nwindows), %l6
	ld	[%l6+%lo(_nwindows)], %l6
	dec	%l6

#ifdef	SYS_TRAP_CHECKS_CPUID
	set	_cpuid, %l5
	lda	[%l5]ASI_FLPR, %l7
	and	%l7, 3, %l7
	cmp	%l7, 2			! cpuid not mapped
	bne	1f
	nop

	sta	%g0, [%l5]ASI_FLPR	! flush tlb

	set	CACHE_BYTES-1, %l3
	and	%l5, %l3, %l7		! modulo cache size
	set	PAGEMASK, %l3
	and	%l7, %l3, %l7		! round down to base of page
	set	KERNELBASE + CACHE_BYTES, %l3
	ld	[%l3 + %l7], %g0	! replace the cache line

	ld	[%l5], %l7
	GETCPU(%l5)
	cmp	%l5, %l7
	bne	_cpuid_bad
	nop

!!! add more tests here, as desired.

1:
#endif	SYS_TRAP_CHECKS_CPUID
	!
	! Prepare to go to C (batten down the hatches).
	!
	mov	0x01, %l5		! CWM = 0x01 << CWP
	sll	%l5, %l0, %l5
	mov	%wim, %l3		! get WIM

	btst	PSR_PS, %l0		! test pS
	bz	st_user			! branch if user trap
	btst	%l5, %l3		! delay slot, compare WIM and CWM

	!
	! Trap from supervisor.
	! We can be either on the system stack or interrupt stack.
	!
	sub	%fp, MINFRAME+REGSIZE, %l7 ! save sys globals on stack
	SAVE_GLOBALS(%l7 + MINFRAME)
	/*
	 * call mmu_getsyncflt to get the status reg in %g6 and
	 * address reg in %g7.	In harvard archetectures,
	 * mmu_getsyncflt determines which MMU had the fault by
	 * checking the input parameter which indicates whether it was
	 * a text or data fault.  Note that this assumes that only one
	 * data fault at a time can occurr on CPUs with parallel MMUs.
	 *
	 * NOTE - %g6 and %g7 are used to store the SFSR and
	 * SFAR until used later (see below).  The reason for
	 * reading these synchronous fault registers now is that
	 * the window overflow processing that we are about to do
	 * may also cause another fault. This requres some care in
	 * validating code that might be executed in the meantime;
	 * some sections of code assume %g6 and %g7 may be stepped on.
	 */
	btst	T_FAULT, %l4
	bz,a	2f
	clr	%g6		!don't leave %g6 unset if SFSR not read
	mov	%o7, %g3	!save o7 in g3
	call	_mmu_getsyncflt
	mov	%l4, %g6	!indicates text or data fault.
	mov	%g3, %o7	!restore o7.
2:
#ifdef	TRAPWINDOW
	!
	! store the window at the time of the trap into a static area.
	!
	set	_trap_window, %g1
	mov	%wim, %g2
	st	%g2, [%g1+96]
	mov	%psr, %g2
	restore
	st %o0, [%g1]; st %o1, [%g1+4]; st %o2, [%g1+8]; st %o3, [%g1+12]
	st %o4, [%g1+16]; st %o5, [%g1+20]; st %o6, [%g1+24]; st %o7, [%g1+28]
	st %l0, [%g1+32]; st %l1, [%g1+36]; st %l2, [%g1+40]; st %l3, [%g1+44]
	st %l4, [%g1+48]; st %l5, [%g1+52]; st %l6, [%g1+56]; st %l7, [%g1+60]
	st %i0, [%g1+64]; st %i1, [%g1+68]; st %i2, [%g1+72]; st %i3, [%g1+76]
	st %i4, [%g1+80]; st %i5, [%g1+84]; st %i6, [%g1+88]; st %i7, [%g1+92]
	mov	%g2, %psr
	nop; nop; nop;
#endif	TRAPWINDOW
	st	%fp, [%l7 + MINFRAME + SP*4] ! stack pointer
	st	%l0, [%l7 + MINFRAME + PSR*4] ! psr
	st	%l1, [%l7 + MINFRAME + PC*4] ! pc
	!
	! If we are in last trap window, all windows are occupied and
	! we must do window overflow stuff in order to do further calls
	!
	btst	%l5, %l3		! retest
	bz	st_have_window		! if ((CWM&WIM)==0) no overflow
	st	%l2, [%l7 + MINFRAME + nPC*4] ! npc
	b,a	st_sys_ovf

st_user:
	!
	! Trap from user. Save user globals and prepare system stack.
	! Test whether the current window is the last available window
	! in the register file (CWM == WIM).
	!
	set	_masterprocp, %l7
	ld	[%l7], %l7
	ld	[%l7 + P_STACK], %l7	! initial kernel sp for this process

	SAVE_GLOBALS(%l7 + MINFRAME)
	SAVE_OUTS(%l7 + MINFRAME)
	/*
	 * call mmu_getsyncflt to get the status reg in %g6 and
	 * address reg in %g7.	In harvard archetectures,
	 * mmu_getsyncflt determines which MMU had the fault by
	 * checking the input parameter which indicates whether it was
	 * a text or data fault.  Note that this assumes that only one
	 * data fault at a time can occurr on CPUs with parallel MMUs.
	 *
	 * NOTE - %g6 and %g7 are used to store the SFSR and
	 * SFAR until used later (see below).  The reason for
	 * reading these synchronous fault registers now is that
	 * the window overflow processing that we are about to do
	 * may also cause another fault. This requres some care in
	 * validating code that might be executed in the meantime;
	 * some sections of code assume %g6 and %g7 may be stepped on.
	 */
	btst	T_FAULT, %l4
	bz,a	1f
	clr	%g6		!don't leave %g6 unset if SFSR not read
	mov	%o7, %g3	!save o7 in g3
	call	_mmu_getsyncflt
	mov	%l4, %g6	!indicates text or data fault.
	mov	%g3, %o7
1:
	!
	st	%l0, [%l7 + MINFRAME + PSR*4]	! psr
	set	_uunix, %g5
	ld	[%g5], %g5
	st	%l1, [%l7 + MINFRAME + PC*4]	! pc
	st	%l2, [%l7 + MINFRAME + nPC*4]	! npc

	!
	! If we are in last trap window, all windows are occupied and
	! we must do window overflow stuff in order to do further calls
	!
	btst	%l5, %l3		! retest
	bz	1f			! if ((CWM&WIM)==0) no overflow
	clr	[%g5 + PCB_WBCNT]	! delay slot, save buffer ptr = 0

	not	%l5, %g2		! UWM = ~CWM
	mov	-2, %g3			! gen window mask from NW-1 in %l6
	sll	%g3, %l6, %g3
	andn	%g2, %g3, %g2
	b	st_user_ovf		! overflow
	srl	%l3, 1, %g1		! delay slot,WIM = %g1 = ror(WIM, 1, NW)

	!
	! Compute the user window mask (u.u_pcb.pcb_uwm), which is a mask of
	! window which contain user data. It is all the windows "between"
	! CWM and WIM.
	!
1:
	subcc	%l3, %l5, %g1		! if (WIM >= CWM)
	bneg,a	2f			!    u.u_pcb.pcb_uwm = (WIM-CWM)&~CWM
	sub	%g1, 1, %g1		! else
2:					!    u.u_pcb.pcb_uwm = (WIM-CWM-1)&~CWM
	bclr	%l5, %g1
	mov	-2, %g3			! gen window mask from NW-1 in %l6
	sll	%g3, %l6, %g3
	andn	%g1, %g3, %g1
	set	_uunix, %g5		! XXX - global u register?
	ld	[%g5], %g5
	st	%g1, [%g5 + PCB_UWM]

st_have_window:
	!
	! The next window is open.
	!
	mov	%l7, %sp		! setup previously computed stack
	!
	! Process trap according to type
	!
	btst	T_INTERRUPT, %l4	! interrupt
	bnz	interrupt
	cmp	%l4, T_SYSCALL		! syscall
	be	syscall
	btst	T_FAULT, %l4		! fault

fixfault:
	bnz,a	fault
	bclr	T_FAULT, %l4
	cmp	%l4, T_FP_EXCEPTION	! floating point exception
	be	_fp_exception
	cmp	%l4, T_FP_DISABLED	! fpu is disabled
	be	_fp_disabled
	cmp	%l4, T_FLUSH_WINDOWS	! flush user windows to stack
	bne	1f
	wr	%l0, PSR_ET, %psr	! enable traps

	nop				! psr delay
	!
	! Flush windows trap.
	!
	call	_flush_user_windows	! flush user windows
	nop
	!
	! Don't redo trap instruction.
	!
	ld	[%sp + MINFRAME + nPC*4], %g1
	st	%g1, [%sp + MINFRAME + PC*4]  ! pc = npc
	add	%g1, 4, %g1
	b	sys_rtt
	st	%g1, [%sp + MINFRAME + nPC*4] ! npc = npc + 4

1:
	!
	! All other traps. Call C trap handler.
	!
	mov	%l4, %o0		! trap(t, rp)
	clr	%o2			!  addr = 0
	clr	%o3			!  be = 0
	mov	S_OTHER, %o4		!  rw = S_OTHER
	call	_trap			! C trap handler
	add	%sp, MINFRAME, %o1
	b,a	sys_rtt			! return from trap
/* end systrap */

/*
 * Sys_trap overflow handling.
 * Psuedo subroutine returns to st_have_window.
 */
st_sys_ovf:
	!
	! Overflow from system.
	! Determine whether the next window is a user window.
	! If u.u_pcb.pcb_uwm has any bits set, then it is a user window
	! which must be saved.
	!
#ifdef	PERFMETER
	sethi	%hi(_overflowcnt), %g5
	ld	[%g5 + %lo(_overflowcnt)], %g2
	inc	%g2
	st	%g2, [%g5 + %lo(_overflowcnt)]
#endif	PERFMETER
	set	_uunix, %g5		! XXX - global u register?
	ld	[%g5], %g5
	ld	[%g5 + PCB_UWM], %g2	! if (u.u_pcb.pcb_uwm)
	tst	%g2			!	user window
	bnz	st_user_ovf
	srl	%l3, 1, %g1		! delay slot,WIM = %g1 = ror(WIM, 1, NW)
	!
	! Save supervisor window. Compute the new WIM and change current window
	! to the window to be saved.
	!
	sll	%l3, %l6, %l3		! %l6 == NW-1
	or	%l3, %g1, %g1
	save				! get into window to be saved
	mov	%g1, %wim		! install new WIM
	!
	! Save window on the stack.
	!
st_stack_res:
	SAVE_WINDOW(%sp)
	b	st_have_window		! finished overflow processing
	restore				! delay slot, back to original window

st_user_ovf:
	!
	! Overflow. Window to be saved is a user window.
	! Compute the new WIM and change the current window to the
	! window to be saved.
	!
	sll	%l3, %l6, %l3		! %l6 == NW-1
	or	%l3, %g1, %g1
	bclr	%g1, %g2		! turn off uwm bit for window
	set	_uunix, %g5		! XXX - global u register?
	ld	[%g5], %g5
	st	%g2, [%g5 + PCB_UWM]	! we are about to save
	save				! get into window to be saved
	mov	%g1, %wim		! install new WIM
	!
	! In order to save the window onto the stack, the stack
	! must be aligned on a word boundary, and the part of the
	! stack where the save will be done must be present.
	! We first check for alignment.
	!
	btst	0x7, %sp		! test sp alignment
	bnz	st_stack_not_res	! stack misaligned, catch it later

	! branch to code which is dependent upon the particular type
	! of SRMMU module.  this code will attempt to save the
	! overflowed window onto the stack, without first verifying
	! the presence of the stack.

	! Most of the time the stack is resident in main memory, so we
	! don't verify its presence before attempting to save the
	! window onto the stack.  Rather, we simply set the no-fault
	! bit of the SRMMU's control register, so that doing the save
	! won't cause another trap in the case where the stack is not
	! present.  By checking the synchronous fault status register,
	! we can determine whether the save actually worked (ie. stack
	! was present), or whether we first need to fault in the
	! stack.

	! Other sun4 trap handlers first probe for the stack, and
	! then, if the stack is present, they store to the stack.
	! This approach CANNOT be used with a multiprocessor system
	! because of a race condition: between the time that the stack
	! is probed, and the store to the stack is done, the stack
	! could be stolen by the page daemon.

	! Of course, we have to be sure that this really is
	! in userland, or the user could overflow data into
	! supvervisor space ...

	set	KERNELBASE, %g1
	cmp	%g1, %sp
	bgu	_mmu_sys_ovf		! ok, go do it.
	nop
	mov	0x8E, %g1		!   [fake up SFSR]
	b	st_stack_not_res
	mov	%sp, %g2		!   [fake up SFAR]


	! here is where we return after attempting to save the
	! overflowed window onto the stack.  check to see if the
	! stack was actually present.

	.global st_chk_flt
st_chk_flt:

	btst	MMU_SFSR_FAV, %g1		! did a fault occurr?
	bz,a	st_have_window
	restore

	! a fault occurred, so the stack is not resident.
st_stack_not_res:
	!
	! User stack is not resident, save in u area for processing in sys_rtt.
	!
	sethi	%hi(_uunix), %g5	! XXX - global u register?
	ld	[%g5 + %lo(_uunix)], %g5
	ld	[%g5 + PCB_WBCNT], %g1
	sll	%g1, 2, %g1		! convert to spbuf offset

	add	%g1, %g5, %g2
	st	%sp, [%g2 + PCB_SPBUF]	! save sp
	sll	%g1, 4, %g1		! convert wbcnt to pcb_wbuf offset

	sethi	%hi(_uunix), %g5	! XXX - global u register?
	ld	[%g5 + %lo(_uunix)], %g5
	add	%g1, %g5, %g2
	set	PCB_WBUF, %g4
	add	%g4, %g2, %g2
	SAVE_WINDOW(%g2)
	srl	%g1, 6, %g1		! increment u.u_pcb.pcb_wbcnt
	add	%g1, 1, %g1

	sethi	%hi(_uunix), %g5	! XXX - global u register?
	ld	[%g5 + %lo(_uunix)], %g5
	st	%g1, [%g5 + PCB_WBCNT]
	b	st_have_window		! finished overflow processing
	restore				! delay slot, back to original window
/* end sys_trap overflow */

	.seg	"data"
	.align	4

	.global _overflowcnt, _underflowcnt
_overflowcnt:
	.word	0
_underflowcnt:
	.word	0

	.seg	"text"
	.align	4

/*
 * Return from sys_trap routine.
 */
	.global sys_rtt
sys_rtt:
	! code for fItoX bug removed from sun4c (not needed)
	! and, since sun4m inherits from sun4c, it is still gone.

#ifdef	SYS_RTT_CHECKS_CPUID
	set	_cpuid, %g1
	lda	[%g1]ASI_FLPR, %l0
	and	%l0, 3, %l0
	cmp	%l0, 2			! cpuid not mapped
	bne	1f
	nop

	ld	[%g1], %l0
	GETCPU(%g1)
	cmp	%g1, %l0
	bne	_cpuid_bad
	nop

!!! add more tests here, as desired.

1:
#endif	SYS_RTT_CHECKS_CPUID

	! Return from trap.
	!
	ld	[%sp + MINFRAME + PSR*4], %l0 ! get saved psr
	btst	PSR_PS, %l0		! test pS for return to supervisor
	bnz	sr_sup
	mov	%psr, %g1

#ifdef	LWP
	.global ___Nrunnable, _lwpschedule
#endif	LWP

sr_user:
	!
	! Return to User.
	! Turn off traps using the current CWP (because
	! we are returning to user). Test for streams actions.
	! Test for LWP, AST for resched. or prof, or streams.
	!
	sethi	%hi(_uunix), %g5	! XXX - global u register?
	ld	[%g5 + %lo(_uunix)], %g5
	and	%g1, PSR_CWP, %g1	! insert current CWP in old psr
	andn	%l0, PSR_CWP, %l0
	or	%l0, %g1, %l0
	mov	%l0, %psr		! install old psr, disable traps
	nop; nop;			! psr delay
#ifdef	LWP
#ifdef	MULTIPROCESSOR
	mov	1, %g2
	GETCPU(%g1)
	sll	%g2, %g1, %g2
	sethi	%hi(_force_lwps), %g1		! check the lwp force bits;
	ld	[%g1 + %lo(_force_lwps)], %g1
	andcc	%g1, %g2, %g0			! if our bit is on,
	bnz	0f				! always run lwpschedule
	nop

	tst	%g1				! if anyone else's is on,
	bnz	1f				! do not try to run lwpschedule
	nop

	sethi	%hi(_runthreads), %g1
	ld	[%g1 + %lo(_runthreads)], %g2
	tst	%g2				! if threads are running,
	bnz	1f				! do not bother running them.
	nop
#endif	MULTIPROCESSOR

	sethi	%hi(___Nrunnable), %g2
	ld	[%g2 + %lo(___Nrunnable)], %g2
	tst	%g2				! if nobody is runnable,
	bz	1f				! don't bother checking.
	nop
0:
	!
	! Runnable thread for async I/O
	!
	wr	%l0, PSR_ET, %psr	! turn on traps
	nop				! psr delay
#ifdef	MULTIPROCESSOR
	call	_klock_enter
	nop
#endif	MULTIPROCESSOR
	call	_flush_user_windows
	nop
	call	_lwpschedule
	nop

	!
	! Use kernel context (for now) since lwpschedule may
	! change the context reg. This causes a fault.
	call	_mmu_setctx
	mov	%g0, %o0
	b	sys_rtt			! try again
	nop
1:
#endif	LWP
	sethi	%hi(_qrunflag), %g1
	ldub	[%g1 + %lo(_qrunflag)], %g1
	sethi	%hi(_queueflag), %l5	! interlock slot
	tst	%g1			! need to run stream queues?
	bz,a	3f			! no
	ld	[%g5 + PCB_FLAGS], %g1	! delay slot, test for ast.

	ldub	[%l5 + %lo(_queueflag)], %g1
	tst	%g1			! already running queues?
	bnz,a	3f			! yes
	ld	[%g5 + PCB_FLAGS], %g1	! delay slot, test for ast.

	!
	! Enable traps, so we can call C code.
	! Keep the SPL really high for now.
	!
	andn	%l0, PSR_PIL, %g2	! splhi
	or	%g2, 13 << PSR_PIL_BIT, %g2
	mov	%g2, %psr		! change priority (IU bug)
	wr	%g2, PSR_ET, %psr	! turn on traps

#ifdef	MULTIPROCESSOR
	nop		! psr delay
	call	_klock_knock
	nop
	tst	%o0
	bz	9f
	nop
#endif	MULTIPROCESSOR
	!
	! Run the streams queues.
	!
	mov	1, %g1			! mark that we're running the queues
	call	_queuerun
	stb	%g1, [%l5 + %lo(_queueflag)]

	stb	%g0, [%l5 + %lo(_queueflag)] ! done running queues

	b	sys_rtt
	nop

#ifdef	MULTIPROCESSOR
9:
	!
	! unable to acquire the lock.
	! reload the globals we lost
	! and proceed with the rtt.
	!
	wr	%l0, %psr		! disable traps
	nop ; nop ; nop		! psr delay

	sethi	%hi(_uunix), %g5	! XXX - global u register?
	ld	[%g5 + %lo(_uunix)], %g5
	ld	[%g5 + PCB_FLAGS], %g1	! delay slot, test for ast.

#endif	MULTIPROCESSOR

3:
	srl	%g1, AST_SCHED_BIT, %g1
	btst	1, %g1
	bz,a	1f
	ld	[%g5 + PCB_WBCNT], %g3	! delay slot, user regs been saved?

	!
	! Let trap handle the AST.
	!
	wr	%l0, PSR_ET, %psr	! turn on traps
	mov	T_AST, %o0
	call	_trap			! trap(T_AST, rp)
	add	%sp, MINFRAME, %o1
	b,a	sys_rtt

	!
	! Return from trap, for yield_child
	!
	.global _fork_rtt
_fork_rtt:
	sethi	%hi(_masterprocp), %l6
	ld	[%l6 + %lo(_masterprocp)], %o0
	ld	[%o0 + P_STACK], %l7
	mov	%sp, %o2
	call	_sys_rttchk		! sys_rttchk(procp)
	mov	%l7, %o1

	ld	[%sp + MINFRAME + PSR*4], %l0 ! get saved psr
	btst	PSR_PS, %l0		! test pS for return to supervisor
	bnz	sr_sup
	mov	%psr, %g1

	sethi	%hi(_uunix), %g5
	ld	[%g5 + %lo(_uunix)], %g5
	and	%g1, PSR_CWP, %g1	! insert current CWP in old psr
	andn	%l0, PSR_CWP, %l0
	or	%l0, %g1, %l0
	mov	%l0, %psr		! install old psr, disable traps
	nop				! psr delay
	b	3b
	ld	[%g5 + PCB_FLAGS], %g1	! delay slot, test for ast.

1:
	!
	! If user regs have been saved to the window buffer we must clean it.
	!
	tst	%g3
	bz,a	2f
	ld	[%g5 + PCB_UWM], %l4	! delay slot, user windows in reg file?

	!
	! User regs have been saved into the u area.
	! Let trap handle putting them on the stack.
	!
	mov	%l0, %psr		! in case of changed priority (IU bug)
	wr	%l0, PSR_ET, %psr	! turn on traps
	mov	T_WIN_OVERFLOW, %o0
	call	_trap			! trap(T_WIN_OVERFLOW, rp)
	add	%sp, MINFRAME, %o1
	b,a	sys_rtt

2:
	!
	! We must insure that the rett will not take a window underflow trap.
	!
	RESTORE_OUTS(%sp + MINFRAME)	! restore user outs
	tst	%l4
	bnz	sr_user_regs
	ld	[%sp + MINFRAME + PC*4], %l1 ! restore user pc

	!
	! The user has no windows in the register file.
	! Try to get one from his stack.
	!
#ifdef	PERFMETER
	sethi	%hi(_underflowcnt), %l6
	ld	[%l6 + %lo(_underflowcnt)], %l3
	inc	%l3
	st	%l3, [%l6 + %lo(_underflowcnt)]
#endif	PERFMETER
	sethi	%hi(_nwindows), %l6
	ld	[%l6+%lo(_nwindows)], %l6
	dec	%l6			! get NW-1 for rol calculation
	mov	%wim, %l3		! get wim
	sll	%l3, 1, %l4		! next WIM = rol(WIM, 1, NW)
	srl	%l3, %l6, %l5		! %l6 == NW-1
	or	%l5, %l4, %l5
	mov	%l5, %wim		! install it
	!
	! In order to restore the window from the stack, the stack
	! must be aligned on a word boundary, and the part of the
	! stack where the save will be done must be present.
	! We first check for alignment.
	!
	btst	0x7, %fp		! test fp alignment
	bz	sr_assume_stack_res
	nop

	!
	! A user underflow with a misaligned sp.
	! Fake a memory alignment trap.
	!
	mov	%l3, %wim		! restore old wim
sr_align_trap:
	mov	%l0, %psr		! in case of changed priority (IU bug)
	wr	%l0, PSR_ET, %psr	! turn on traps
	mov	T_ALIGNMENT, %o0
	call	_trap			! trap(T_ALIGNMENT, rp)
	add	%sp, MINFRAME, %o1
	b,a	sys_rtt

	! branch to code which is dependent upon the particular
	! type of SRMMU module.	 this code will attempt to restore
	! the underflowed window from the stack, without first
	! verifying the presence of the stack.
	! Most of the time the stack is resident in main memory,
	! so we don't verify its presence before attempting to restore
	! the window from the stack.  Rather, we simply set the
	! no-fault bit of the SRMMU's control register, so that
	! doing the restore won't cause another trap in the case
	! where the stack is not present.  By checking the
	! synchronous fault status register, we can determine
	! whether the restore actually worked (ie. stack was present),
	! or whether we first need to fault in the stack.
	! Other sun4 trap handlers first probe for the stack, and
	! then, if the stack is present, they restore from the stack.
	! This approach CANNOT be used with a multiprocessor system
	! because of a race condition: between the time that the
	! stack is probed, and the restore from the stack is done, the
	! stack could be stolen by the page daemon.
sr_assume_stack_res:
	set	KERNELBASE, %g1
	cmp	%g1, %fp
	bgu	1f
	nop
	mov	0x0E, %g1		!   [fake up SFSR]
	b	sr_stack_not_res
	mov	%fp, %g2		!   [fake up SFAR]

1:
	sethi	%hi(_v_mmu_sys_unf), %g1
	ld	[%g1 + %lo(_v_mmu_sys_unf)], %g1
	jmp	%g1
	nop

	! here is where we return after attempting to restore the
	! underflowed window from the stack.  check to see if the
	! stack was actually present.
	.global sr_chk_flt
sr_chk_flt:
	btst	MMU_SFSR_FAV, %g1		! did a fault occurr?
	bz	sr_user_regs
	nop

sr_stack_not_res:
	!
	! Restore area on user stack is not resident.
	! We punt and fake a page fault so that trap can bring the page in.
	!
	mov	%l3, %wim		! restore old wim
	nop; nop; nop;
	mov	T_DATA_FAULT, %o0
	add	%sp, MINFRAME, %o1
	mov	%g2, %o2		! save fault address
	mov	%l0, %psr		! in case of changed priority (IU bug)
	wr	%l0, PSR_ET, %psr	! enable traps
	mov	%g1, %o3
	call	_trap			! trap(T_DATA_FAULT,
	mov	S_WRITE, %o4		! rp, addr, be, S_WRITE)

	b,a	sys_rtt
sr_user_regs:
	!
	! User has at least one window in the register file.
	!
	sethi	%hi(_uunix), %g5	! XXX - global u register?
	ld	[%g5 + %lo(_uunix)], %g5
	ld	[%sp + MINFRAME + nPC*4], %l2 ! user npc
	ld	[%g5 + PCB_FLAGS], %l3
	!
	! check user pc alignment.  This can get messed up either using
	! ptrace, or by using the '-T' flag of ld to place the text
	! section at a strange location (bug id #1015631)
	!
	or	%l1, %l2, %g2
	btst	0x3, %g2		! if either PC or NPC badly aligned,
	bnz	sr_align_trap		! die a horrible death.
	nop

	btst	CLEAN_WINDOWS, %l3
	bz,a	3f
	mov	%l0, %psr		! install old PSR_CC

	!
	! Maintain clean windows.
	!
	mov	%wim, %g2		! put wim in global
	mov	0, %wim			! zero wim to allow saving
	mov	%l0, %g3		! put original psr in global
	b	2f			! test next window for invalid
	save
	!
	! Loop through windows past the trap window
	! clearing them until we hit the invlaid window.
	!
1:
	clr	%l1			! clear the window
	clr	%l2
	clr	%l3
	clr	%l4
	clr	%l5
	clr	%l6
	clr	%l7
	clr	%o0
	clr	%o1
	clr	%o2
	clr	%o3
	clr	%o4
	clr	%o5
	clr	%o6
	clr	%o7
	save
2:
	mov	%psr, %g1		! get CWP
	srl	%g2, %g1, %g1		! test WIM bit
	btst	1, %g1
	bz,a	1b			! not invalid window yet
	clr	%l0			! clear the window

	!
	! Clean up trap window.
	!
	mov	%g3, %psr		! back to trap window, restore PSR_CC
	mov	%g2, %wim		! restore wim
	nop; nop;			! psr delay

! can't use globals after this.
	RESTORE_GLOBALS(%sp + MINFRAME) ! restore user globals
	mov	%l1, %o6		! put pc, npc in unobtrusive place
	mov	%l2, %o7
	clr	%l0			! clear the rest of the window
	clr	%l1
	clr	%l2
	clr	%l3
	clr	%l4
	clr	%l5
	clr	%l6
	clr	%l7
	clr	%o0
	clr	%o1
	clr	%o2
	clr	%o3
	clr	%o4
	clr	%o5
#ifndef	MULTIPROCESSOR
	jmp	%o6			! return
	rett	%o7
#else	MULTIPROCESSOR
	b,a	_klock_rett_o6o7
#endif	MULTIPROCESSOR

3:
! can't use globals after this.
	RESTORE_GLOBALS(%sp + MINFRAME) ! restore user globals

#ifndef	MULTIPROCESSOR
	jmp	%l1			! return
	rett	%l2
#else	MULTIPROCESSOR
	b,a	_klock_rett
#endif	MULTIPROCESSOR

sr_sup:
	!
	! Returning to supervisor.
	! We will restore the trap psr. This has the effect of disabling
	! traps and changing the CWP back to the original trap CWP. This
	! completely restores the PSR so that if we get a trap between a
	! rdpsr and a wrpsr its OK. We only do this for supervisor return
	! since users can't manipulate the psr.
	!
	sethi	%hi(_nwindows), %g5
	ld	[%g5 + %lo(_nwindows)], %g5 ! number of windows on this machine
	ld	[%sp + MINFRAME + SP*4], %fp ! get sys sp
	xor	%g1, %l0, %g1		! test for CWP change
	btst	PSR_CWP, %g1
	bz,a	sr_samecwp
	mov	%l0, %psr		! install old psr, disable traps
	!
	! The CWP will be changed. We must save sp and the ins
	! and recompute WIM. We know we need to restore the next
	! window in this case.
	!
	mov	%l0, %g3		! save old psr
	mov	%sp, %g4		! save sp, ins for new window
	std	%i0, [%sp +(8*4)]	! normal stack save area
	std	%i2, [%sp +(10*4)]
	std	%i4, [%sp +(12*4)]
	std	%i6, [%sp +(14*4)]
	mov	%g3, %psr		! old psr, disable traps, CWP, PSR_CC
	mov	0x4, %g1		! psr delay, compute mask for CWP + 2
	sll	%g1, %g3, %g1		! psr delay, won't work for NW == 32
	srl	%g1, %g5, %g2		! psr delay
	or	%g1, %g2, %g1
	mov	%g1, %wim		! install new wim
	mov	%g4, %sp		! reestablish sp
	ldd	[%sp + (8*4)], %i0	! reestablish ins
	ldd	[%sp + (10*4)], %i2
	ldd	[%sp + (12*4)], %i4
	ldd	[%sp + (14*4)], %i6
	restore				! restore return window
	RESTORE_WINDOW(%sp)
	b	sr_out
	save

sr_samecwp:
	!
	! There is no CWP change.
	! We must make sure that there is a window to return to.
	!
	mov	0x2, %g1		! compute mask for CWP + 1
	sll	%g1, %l0, %g1		! XXX won't work for NW == 32
	srl	%g1, %g5, %g2		! %g5 == NW, from above
	or	%g1, %g2, %g1
	mov	%wim, %g2		! cmp with wim to check for underflow
	btst	%g1, %g2
	bz	sr_out
	mov	%l0, %psr		! install old PSR_CC
	!
	! No window to return to. Restore it.
	!
	sll	%g2, 1, %g1		! compute new WIM = rol(WIM, 1, NW)
	dec	%g5			! %g5 == NW-1
	srl	%g2, %g5, %g2
	or	%g1, %g2, %g1
	mov	%g1, %wim		! install it
	nop; nop; nop;			! wim delay
	restore				! get into window to be restored
	RESTORE_WINDOW(%sp)
	save				! get back to original window
sr_out:
	RESTORE_GLOBALS(%sp + MINFRAME) ! restore system globals
	ld	[%sp + MINFRAME + PC*4], %l1 ! delay slot, restore sys pc
	ld	[%sp + MINFRAME + nPC*4], %l2 ! sys npc

	mov	%l2, %l2		!VIKING HW BUG WORKAROUND
					! REMOVE WHEN VIKING ARE > 1.2 REV

#ifndef	MULTIPROCESSOR
	jmp	%l1			! return to trapped instruction
	rett	%l2
#else	MULTIPROCESSOR
	b,a	_klock_rett
#endif	MULTIPROCESSOR

/* end sys_rtt*/

/*
 * System call handler.
 */
syscall:
	wr	%l0, PSR_ET, %psr	! enable traps (no priority change)
	nop				! psr delay
	call	_syscall		! syscall(rp)
	add	%sp, MINFRAME, %o0	! ptr to reg struct
	b,a	sys_rtt
/* end syscall */

/*
 * Fault handler.
 */
fault:
1:
	/*
	 * The synchronous fault status register (SFSR) and
	 * synchronous fault address register (SFAR) were obtained
	 * above by calling mmu_getsyncflt, and have been stored
	 * in %g6 and %g7, respectively.
	 * For the call to trap below, move the status reg into
	 * %o3, and the address reg into %o2
	 */

	mov	%g6, %o3		! SFSR
	cmp	%l4, T_TEXT_FAULT	! text fault?
	be,a	2f
	mov	%l1, %o2		! use saved pc as the fault address
	mov	%g7, %o2		! SFAR

	! check for FT_NONE, if it is zero just return 

2:	srl	%o3, FS_FTSHIFT, %g2	! get the fault type
	and	%g2, FS_FTMASK, %g2
	cmp	%g2, %g0
	bz	sys_rtt		! if FT type is zero we have nothing do
	nop
	
4:	srl	%o3, FS_ATSHIFT, %g2	!Get the access type bits
	and	%g2, FS_ATMASK, %g2
	set	1,%o4			! set to S_READ
	cmp	%g2, AT_UREAD
	be	3f
	nop
	cmp	%g2, AT_SREAD
	be	3f
	nop
	add	%o4, 1, %o4
	cmp	%g2, AT_UWRITE
	be	3f
	nop
	cmp	%g2, AT_SWRITE
	be	3f
	nop
	! Must be an execute cycle
	add	%o4, 1, %o4

	! don't believe the  sfar for instruction faults 
	mov	%l1, %o2		! since fav is zero, 
					! put pc as the faulting address.
					! 
3:
6:	wr	%l0, PSR_ET, %psr	!enable traps (no priority change)
	/*
	 *  %o0, %o1 is setup by SYS_TRAP, 
	 *  %o2 from fav/%l1 (pc), %o3 from fsr, 
	 *  %o4 from looking at fsr 
	 */

	mov	%l4, %o0	!trap(t, rp, addr, fsr, rw)

	call	_trap		! C Trap handler
	add	%sp, MINFRAME, %o1

	b,a	sys_rtt		! return from trap


/* end fault */

/*
 * Interrupt vector table
 */
	.seg	"data"
	.align	4
	!
	! all interrupts are vectored via the following table
	! we can't vector directly from the scb because
	! we haven't done window setup
	!
	! first sixteen entries are for the sixteen
	! levels of hardware interrupt.
#ifdef	MULTIPROCESSOR
	! the lock has been acquired before we call
	! for hardware level 1 through 13 service;
	! if acquisition failed, the holder of the
	! lock will see the pending interrupt when
	! he lowers his SPL.
#endif	MULTIPROCESSOR
	! the next sixteen entries are for the
	! sixteen levels of software interrupt.
#ifdef	MULTIPROCESSOR
	! the lock has been acquired before we call
	! for software level 1 through 9* service;
	! if acquisition failed, we forward it to
	! the current holder of the kernel lock. 
	! (* "9" can be changed in klock_asm.s)
#endif	MULTIPROCESSOR
	.global _int_vector
_int_vector:
/* first 16 words: hard interrupt service */
	.word	level0		! level 0, should not happen
	.word	level1		! level 1
	.word	level2		! level 2, sbus/vme level 1
	.word	level3		! level 3, sbus/vme level 2
	.word	level4		! level 4, onboard scsi
	.word	level5		! level 5, sbus/vme level 3
	.word	level6		! level 6, onboard ether
	.word	level7		! level 7, sbus/vme level 4
	.word	level8		! level 8, onboard video
	.word	level9		! level 9, sbus/vme level 5, module interrupt
	.word	level10		! level 10, normal clock
	.word	level11		! level 11, sbus/vme level 6, floppy
	.word	level12		! level 12, serial i/o
	.word	level13		! level 13, sbus/vme/vme level 7, audio
	.word	level14		! level 14, high level clock
	.word	level15		! level 15, memory error
/* second 16 words: soft interrupt service */
	.word	softlvl0	! level 0, should not happen.
	.word	softlvl1	! level 1, softclock interrupts
	.word	softlvl2	! level 2
	.word	softlvl3	! level 3
	.word	softlvl4	! level 4, floppy and audio soft ints
	.word	softlvl5	! level 5
	.word	softlvl6	! level 6, serial driver soft interrupt
	.word	softlvl7	! level 7
	.word	softlvl8	! level 8
	.word	softlvl9	! level 9
	.word	softlvl10	! level 10
	.word	softlvl11	! level 11
	.word	softlvl12	! level 12
	.word	softlvl13	! level 13
	.word	softlvl14	! level 14
	.word	softlvl15	! level 15

#ifdef	VME
/*
 * Vme vectors are compatible with the sun3 family in which
 * there were possible valid vectors from 64 to 255 inclusive.
 * This requires 192 vectors, each vector is two words long
 * the first word being the interrupt routine address and the
 * second word is the arg.
 *
 * Vectors 0xC8-0xFF (200-255) are reserved for customer use.
 */

/*
 * The vme vectoring uses the following table of routines
 * and arguments. The values for the vectors in the following
 * table are loaded by autoconf at boot time.
 */
#define ERRV	.word 0, _zeros

	.seg	"data"
	.align	4

	.global	_vme_vector
_vme_vector:			! vector numbers
	ERRV; ERRV; ERRV; ERRV  ! 0x40 - 0x43  sc0  | sc?
	ERRV; ERRV; ERRV; ERRV  ! 0x44 - 0x47  xdc0 | xdc1 | xdc2 | xdc3
	ERRV; ERRV; ERRV; ERRV  ! 0x48 - 0x4B  xyc0 | xyc1 | xyc?
	ERRV; ERRV; ERRV; ERRV  ! 0x4C - 0x4F  isc0 | isc1 | isc2 | isc3
	ERRV; ERRV; ERRV; ERRV  ! 0x50 - 0x53  isc4 | isc5 | isc6 | isc7
	ERRV; ERRV; ERRV; ERRV  ! 0x54 - 0x57  future disk controllers
	ERRV; ERRV; ERRV; ERRV  ! 0x58 - 0x5B  future disk controllers
	ERRV; ERRV; ERRV; ERRV  ! 0x5C - 0x5F  future disk controllers
	ERRV; ERRV; ERRV; ERRV  ! 0x60 - 0x63  tm0  | tm1  | tm?
	ERRV; ERRV; ERRV; ERRV  ! 0x64 - 0x67  xtc0 | xtc1 | xtc?
	ERRV; ERRV; ERRV; ERRV  ! 0x68 - 0x6B  future tape controllers
	ERRV; ERRV; ERRV; ERRV  ! 0x6C - 0x6F  future tape controllers
	ERRV; ERRV; ERRV; ERRV  ! 0x70 - 0x73  ec?
	ERRV; ERRV; ERRV; ERRV  ! 0x74 - 0x77  ie0 | ie1 | ie2 | ie3
	ERRV; ERRV; ERRV; ERRV  ! 0x78 - 0x7B  fddi0 | fddi1 | fddi2 | fddi3
	ERRV; ERRV; ERRV; ERRV  ! 0x7C - 0x7F  ie4 | future ethernet devices
	ERRV; ERRV; ERRV; ERRV  ! 0x80 - 0x83  vpc0 | vpc1 | vpc?
	ERRV; ERRV; ERRV; ERRV  ! 0x84 - 0x87  vp?
	ERRV; ERRV; ERRV; ERRV  ! 0x88 - 0x8B  mti0 | mti1 | mti2 | mti3
	ERRV; ERRV; ERRV; ERRV  ! 0x8C - 0x8F  SunLink SCP (Systech DCP-8804)
	ERRV; ERRV; ERRV; ERRV  ! 0x90 - 0x93  Sun-3 zs0 (8 even vectors)
	ERRV; ERRV; ERRV; ERRV  ! 0x94 - 0x97  Sun-3 zs1 (8 odd vectors)
	ERRV; ERRV; ERRV; ERRV  ! 0x98 - 0x9B  Sun-3 zs0 (8 even vectors)
	ERRV; ERRV; ERRV; ERRV  ! 0x9C - 0x9F  Sun-3 zs1 (8 odd vectors)
	ERRV; ERRV; ERRV; ERRV  ! 0xA0 - 0xA3  future serial
	ERRV; ERRV; ERRV; ERRV  ! 0xA4 - 0xA7  pc0  | pc1  | pc2  | pc3
	ERRV; ERRV; ERRV; ERRV  ! 0xA8 - 0xAB  cg2 | future frame buffers
	ERRV; ERRV; ERRV; ERRV  ! 0xAC - 0xAF  gp1 | future graphics processors
	ERRV; ERRV; ERRV; ERRV  ! 0xB0 - 0xB3  sky0 | ?
	ERRV; ERRV; ERRV; ERRV  ! 0xB4 - 0xB7  SunLink / channel attach
	ERRV; ERRV; ERRV; ERRV  ! 0xB8 - 0xBB  (token bus) tbi0 | tbi1 | ?
	ERRV; ERRV; ERRV; ERRV  ! 0xBC - 0xBF  Reserved for Sun
	ERRV; ERRV; ERRV; ERRV  ! 0xC0 - 0xC3  Reserved for Sun
	ERRV; ERRV; ERRV; ERRV  ! 0xC4 - 0xC7  Reserved for Sun
	ERRV; ERRV; ERRV; ERRV  ! 0xC8 - 0xCB  Reserved for User
	ERRV; ERRV; ERRV; ERRV  ! 0xCC - 0xCF  Reserved for User
	ERRV; ERRV; ERRV; ERRV  ! 0xD0 - 0xD3  Reserved for User
	ERRV; ERRV; ERRV; ERRV  ! 0xD4 - 0xD7  Reserved for User
	ERRV; ERRV; ERRV; ERRV  ! 0xD8 - 0xDB  Reserved for User
	ERRV; ERRV; ERRV; ERRV  ! 0xDC - 0xDF  Reserved for User
	ERRV; ERRV; ERRV; ERRV  ! 0xE0 - 0xE3  Reserved for User
	ERRV; ERRV; ERRV; ERRV  ! 0xE4 - 0xE7  Reserved for User
	ERRV; ERRV; ERRV; ERRV  ! 0xE8 - 0xEB  Reserved for User
	ERRV; ERRV; ERRV; ERRV  ! 0xEC - 0xEF  Reserved for User
	ERRV; ERRV; ERRV; ERRV  ! 0xF0 - 0xF3  Reserved for User
	ERRV; ERRV; ERRV; ERRV  ! 0xF4 - 0xF7  Reserved for User
	ERRV; ERRV; ERRV; ERRV  ! 0xF8 - 0xFB  Reserved for User
	ERRV; ERRV; ERRV; ERRV  ! 0xFC - 0xFF  Reserved for User
#endif	VME

/*
 * names of vectored interrupt devices -- for vmstat.
 * Names must correspond to "VEC_MIN" thru "VEC_MAX".
 */

	.seg	"data"
.globl	_intrnames
.globl	_eintrnames
_intrnames:
	.asciz	"sc0", "sc1", "sc2", "sc3"			! 0x40 - 0x43  sc0  | sc?
	.asciz 	"xdc0", "xdc1", "xdc2", "xdc3"			! 0x44 - 0x47  xdc0 | xdc1 | xdc2 | xdc3
	.asciz	"xyc0", "xyc1", "xyc2", "xyc3"			! 0x48 - 0x4B  xyc0 | xyc1 | xyc?
	.asciz 	"isc0", "isc1", "isc2", "isc3"			! 0x4C - 0x4F  isc0 | isc1 | isc2 | isc3
	.asciz 	"isc4", "isc5", "isc6", "isc7"			! 0x50 - 0x53  isc4 | isc5 | isc6 | isc7
	.asciz 	"?disk0", "?disk1", "?disk2", "?disk3"		! 0x54 - 0x57  future disk controllers
	.asciz 	"?disk0", "?disk1", "?disk2", "?disk3"		! 0x58 - 0x5B  future disk controllers
	.asciz 	"?disk0", "?disk1", "?disk2", "?disk3"		! 0x5C - 0x5F  future disk controllers
	.asciz	"tm0", "tm1", "tm2", "tm3"			! 0x60 - 0x63  tm0  | tm1  | tm?
	.asciz	"xtc0", "xtc1", "xtc2", "xtc3"			! 0x64 - 0x67  xtc0 | xtc1 | xtc?
	.asciz	"?tape0", "?tape1", "?tape2", "?tape3"		! 0x68 - 0x6B  future tape controllers
	.asciz	"?tape0", "?tape1", "?tape2", "?tape3"		! 0x6C - 0x6F  future tape controllers
	.asciz	"ec0", "ec1", "ec2", "ec3"			! 0x70 - 0x73  ec?
	.asciz	"ie0", "ie1", "ie2", "ie3"			! 0x74 - 0x77  ie0 | ie1 | ie2 | ie3
	.asciz	"fddi0", "fddi1", "fddi2", "fddi3"		! 0x78 - 0x7B  fddi0 | fddi1 | fddi2 | fddi3
	.asciz	"ie4", "?ether1", "?ether2", "?ether3"		! 0x7C - 0x7F  ie4 | future ethernet devices
	.asciz	"vpc0", "vpc1", "vpc2", "vpc3"			! 0x80 - 0x83  vpc0 | vpc1 | vpc?
	.asciz	"vp0", "vp1", "vp2", "vp3"			! 0x84 - 0x87  vp?
	.asciz	"mti0", "mti1", "mti2", "mti3"			! 0x88 - 0x8B  mti0 | mti1 | mti2 | mti3
	.asciz	"SCP", "SCP", "SCP", "SCP"			! 0x8C - 0x8F  SunLink SCP (Systech DCP-8804)
	.asciz	"zs0", "zs0", "zs0", "zs0"			! 0x90 - 0x93  Sun-3 zs0 (8 even vectors)
	.asciz	"zs1", "zs1", "zs1", "zs1"			! 0x94 - 0x97  Sun-3 zs1 (8 odd vectors)
	.asciz	"zs0", "zs0", "zs0", "zs0"			! 0x98 - 0x9B  Sun-3 zs0 (8 even vectors)
	.asciz	"zs1", "zs1", "zs1", "zs1"			! 0x9C - 0x9F  Sun-3 zs1 (8 odd vectors)
	.asciz	"?serial0", "?serial1", "?serial2", "?serial3"	! 0xA0 - 0xA3  future serial
	.asciz	"pc0", "pc1", "pc2", "pc3"			! 0xA4 - 0xA7  pc0  | pc1  | pc2  | pc3
	.asciz	"cgtwo0", "?frbuf", "?frbuf", "?frbuf"		! 0xA8 - 0xAB  cg2 | future frame buffers
	.asciz	"gpone0", "?gp", "?gp", "?gp"			! 0xAC - 0xAF  gp1 | future graphics processors
	.asciz	"sky0", "???", "???", "???"			! 0xB0 - 0xB3  sky0 | ?
	.asciz	"slchan", "slchan", "slchan", "slchan"		! 0xB4 - 0xB7  SunLink / channel attach
	.asciz	"tbi0", "tbi1", "tbi2", "tbi3"			! 0xB8 - 0xBB  (token bus) tbi0 | tbi1 | ?
	.asciz	"?Sun", "?Sun", "?Sun", "?Sun"			! 0xBC - 0xBF  Reserved for Sun
	.asciz	"?Sun", "?Sun", "?Sun", "?Sun"			! 0xC0 - 0xC3  Reserved for Sun
	.asciz	"?Sun", "?Sun", "?Sun", "?Sun"			! 0xC4 - 0xC7  Reserved for Sun
	.asciz	"?User", "?User", "?User", "?User"		! 0xC8 - 0xCB  Reserved for User
	.asciz	"?User", "?User", "?User", "?User"		! 0xCC - 0xCF  Reserved for User
	.asciz	"?User", "?User", "?User", "?User"		! 0xD0 - 0xD3  Reserved for User
	.asciz	"?User", "?User", "?User", "?User"		! 0xD4 - 0xD7  Reserved for User
	.asciz	"?User", "?User", "?User", "?User"		! 0xD8 - 0xDB  Reserved for User
	.asciz	"?User", "?User", "?User", "?User"		! 0xDC - 0xDF  Reserved for User
	.asciz	"?User", "?User", "?User", "?User"		! 0xE0 - 0xE3  Reserved for User
	.asciz	"?User", "?User", "?User", "?User"		! 0xE4 - 0xE7  Reserved for User
	.asciz	"?User", "?User", "?User", "?User"		! 0xE8 - 0xEB  Reserved for User
	.asciz	"?User", "?User", "?User", "?User"		! 0xEC - 0xEF  Reserved for User
	.asciz	"?User", "?User", "?User", "?User"		! 0xF0 - 0xF3  Reserved for User
	.asciz	"?User", "?User", "?User", "?User"		! 0xF4 - 0xF7  Reserved for User
	.asciz	"?User", "?User", "?User", "?User"		! 0xF8 - 0xFB  Reserved for User
	.asciz	"?User", "?User", "?User", "?User"		! 0xFC - 0xFF  Reserved for User
_eintrnames:

/*
 * Places (initialized to 0) to count vectored interrupts in.
 * Used by vmstat.
 */
	.seg	"bss"
	.align 4

	.globl _intrcnt
	.globl _eintrcnt
_intrcnt:
	.skip	4 * (VEC_MAX - VEC_MIN + 1)
_eintrcnt:

	.seg	"text"
	.align	4

	.seg	"text"

/*
 * Generic interrupt handler.
 */
	.global interrupt
interrupt:
	set	eintstack, %g1		! on interrupt stack?
	cmp	%sp, %g1
	bgu,a	1f
	sub	%g1, SA(MINFRAME), %sp	! get on it.
#ifdef	MULTIPROCESSOR
/*
 * MULTIPROCESSOR kernels move the intstack up into
 * the percpu area, which is after -- not before --
 * the various U areas. So, also put us onto the int
 * stack if we are less than "intstack".
 */
	set	intstack, %l5		! on interrupt stack?
	cmp	%l5, %sp
	bgu,a	1f
	sub	%g1, SA(MINFRAME), %sp	! get on it.
#endif	MULTIPROCESSOR
1:
	andn	%l0, PSR_PIL, %l5	! compute new psr with proper PIL
	and	%l4, T_INT_LEVEL, %l4
	sll	%l4, PSR_PIL_BIT, %g1
	or	%l5, %g1, %l0
	!
	! If we just took a memory error we don't want to turn interrupts
	! on just yet in case there is another memory error waiting in the
	! wings. So disable interrupts if the PIL is 15.
	!
	cmp	%g1, PSR_PIL
	bne	2f
	sll	%l4, 2, %l6		! convert level to word offset
	set	IR_ENA_INT, %o0
	call	_set_intmask		! disable interrupts
!!! %%% this is a bug, but before we fix it
!!! %%% we need to figure out when to turn
!!! %%% interrupt enable back on.
	mov	0, %o1			! by turning on the ENA bit.
!!!	mov	1, %o1			! by turning on the ENA bit.
2:
	!
	! check hard/soft interrupt info.
	! if soft pending, add 16 to
	! the interrupt level to put us in
	! the range for soft interrupts,
	! else if hard pending, just go on.
	! else if neither pending, then
	! someone took the interrupt away;
	! this happens (but rarely).
	!
	GETCPU(%o0)			! get cpu index 0..3
	sll	%o0, 12, %o0		! shift to +0x1000 per MID
	set	V_INT_REGS, %g1
	add	%g1, %o0, %g1		! will need this base addr later, too.
	ld	[%g1], %o0		! get module int pending reg
	set	0x10000, %o1
	sll	%o1, %l4, %o1		! calculate softint bit
	andcc	%o0, %o1, %g0
	bne,a	0f			! if (softint is set)
	add	%l6, 64, %l6		!   use the softint vector table;
	srl	%o1, 16, %o1		! else { calculate hardint bit
	andcc	%o0, %o1, %g0		!   if (hardint is not set) {
	beq,a	sys_rtt			!     return via sys_rtt; (fix %sp)
	mov	%l7, %sp		!   }	/* is the fix %sp needed? */
0:					! }

	!
	! For some interrupts, we need to ack the
	! fact that we got it in the MIPR.
	!
	set	0xFFFF8000, %l3		! ints we have to ack
	andcc	%o1, %l3, %g0		! do we ack this int?
	bnz,a	0f			! if we do,
	  st	%o1, [%g1+4]		!   ack the int
0:
	bnz,a	0f			! if we do,
	  ld	[%g1], %g0		!   interlock MIPR to avoid repeated ints
0:

#ifdef	MULTIPROCESSOR
	!
	! See if we are supposed to handle the interrupt
	! on this cpu. We may need to acquire the klock, or
	! forward the interrupt to the klock owner.
	!
	call	_klock_intlock
	mov	%o1, %o0		! parameter is ITR bit
	tst	%o0			! returns zero if
	bz	0f			!   we are to do this int
	nop

	mov	%l0, %psr		! set level (IU bug)
	wr	%l0, PSR_ET, %psr	! enable traps
	b	sys_rtt			!   we are to ignore this int.
	mov	%l7, %sp		! reset stack pointer (???)
0:
#endif	MULTIPROCESSOR

	!
	! Get handler address for level.
	!
	set	_int_vector, %g1
	ld	[%g1 + %l6], %l3	! grab vector

	!
	! Read System Interrupt Pending Register
	!
	sethi	%hi(V_INT_REGS+0x4000), %l4
	ld	[%l4+%lo(V_INT_REGS+0x4000)], %l4

	!
	! On board interrupt.
	! Due to a bug in the IU, we cannot increase the PIL and
	! enable traps at the same time. In effect, ET changes
	! slightly before the new PIL becomes effective.
	! So we write the psr twice, once to set the PIL and
	! once to set ET.
	!
	mov	%l0, %psr		! set level (IU bug)
	wr	%l0, PSR_ET, %psr	! enable traps
	nop				! psr delay
	call	%l3			! interrupt handler
	nop
int_rtt:
	sethi	%hi(_cnt+V_INTR), %g2	! cnt.v_intr++
	ld	[%g2 + %lo(_cnt+V_INTR)], %g1
	inc	%g1
	st	%g1, [%g2 + %lo(_cnt+V_INTR)]
	!
 	! Device drivers are not aware that it takes time
	! between when they write to a device register,
	! and when the write takes effect, so the interrupt
	! may still be pending. That is why we flush
 	! the M-to-S and VME write buffers unconditionally.
	! Flushing the buffers conditionally by checking the
	! system interrupt pending register cannot guarantee that
	! ALL device stores have indeed reached the device.
	!
	call    _flush_poke_writebuffers
	nop
	b	sys_rtt			! restore previous stack pointer
	mov	%l7, %sp		! reset stack pointer
/* end interrupt */

/*
 * Spurious trap... 'should not happen'
 * called only from level-0 interrupt
 * was "_spurious"
 */
level0:
softlvl0:
	set	1f, %o0
	call	_printf
	nop
	b,a	int_rtt
	.seg	"data"
1:	.asciz	"level zero interrupt!"
	.seg	"text"

/*
 * Macros for autovectored interrupts.
 */

/*
 * SYNCWB: synchronize write buffers
 *
 * Sun-4M requires that we synchronize write buffers
 * before dispatching to device drivers, so they
 * do not need to know to do this. Reading the
 * M-to-S Async Fault Status Register three times
 * is sufficient.
 *
 */
#define SYNCWB								\
	call	_flush_writebuffers					;\
	nop

/*
 * NSSET: set up parameters for the call to not_serviced
 * for this interrupt level to be "SPARC Level N"; if we
 * identify the interrupt as being from OBIO, SBUS, or VME
 * then we will change these registers accordingly.
 */
#define NSSET(LEVEL)							\
	set	_level/**/LEVEL/**/_spurious, %l0			;\
	set	LEVEL, %l1						;\
	set	_busname_vec, %l2

/*
 * IOINTR: Generic hardware interrupt.
 *	branch to int_rtt after successful processing
 *	if nobody in list claims interrupt,
 *	report spurious and branch to int_rtt.
 *	NOTE: parameters for spurious report
 *	may have already been filled in by SBINTR,
 *	OBINTR, or VBINTR in %l0,%l1,%l2.
 */
#define IOINTR(LEVEL) \
	set	_level/**/LEVEL, %l5 /* get vector ptr */;\
1:	ld	[%l5 + AV_VECTOR], %g1 /* get routine address */;\
	tst	%g1		/* end of vector? */			;\
	bz	2f		/* if it is, */				;\
	nop			/*   report spurious */			;\
	call	%g1		/* go there */				;\
	nop								;\
	tst	%o0		/* success? */				;\
	bz,a	1b		/* no, try next one */			;\
	add	%l5, AUTOVECSIZE, %l5	/* delay slot, find next */	;\
	ld	[%l5 + AV_INTCNT], %g1	/* increment counter */		;\
	inc	%g1							;\
	st	%g1, [%l5 + AV_INTCNT]					;\
	sethi	%hi(_level/**/LEVEL/**/_spurious), %g1			;\
	b	int_rtt		/* done */				;\
	clr	[%g1 + %lo(_level/**/LEVEL/**/_spurious)]		;\
2:				/* spurious interrupt */		;\
	mov	%l0, %o0						;\
	mov	%l1, %o1						;\
	call	_not_serviced						;\
	mov	%l2, %o2						;\
	b,a	int_rtt

/*
 * OBINTR: onboard interrupt
 *	branch to int_rtt after successful processing
 *	drop out bottom if nobody in list claims interrupt.
 */

#define OBINTR(LEVEL,MASK)						\
	set	MASK, %l5						;\
	andcc	%l4, %l5, %g0		/* see if masked bit(s) set */	;\
	bz	3f			/* if not, skip around. */	;\
	.empty				/* next safe in trailer */	;\
	set	_olvl/**/LEVEL, %l5 /* get vector ptr */		;\
1:	ld	[%l5 + AV_VECTOR], %g1 /* get routine address */	;\
	tst	%g1		/* end of vector? */			;\
	bz	2f		/* if it is, */				;\
	nop			/*   report spurious */			;\
	call	%g1		/* go there */				;\
	nop								;\
	tst	%o0		/* success? */				;\
	bz,a	1b		/* no, try next one */			;\
	add	%l5, AUTOVECSIZE, %l5	/* delay slot, step to next */	;\
	ld	[%l5 + AV_INTCNT], %g1	/* increment interrupt ctr */	;\
	inc	%g1							;\
	st	%g1, [%l5 + AV_INTCNT]					;\
	sethi	%hi(_olvl/**/LEVEL/**/_spurious), %g1			;\
	b	int_rtt		/* all done! */				;\
	clr	[%g1 + %lo(_olvl/**/LEVEL/**/_spurious)]		;\
2:				/* unclaimed onboard */			;\
	set	_olvl/**/LEVEL/**/_spurious, %l0			;\
	set	LEVEL, %l1						;\
	set	_busname_ovec, %l2					;\
3:				/* not onboard */

/*
 * SBINTR: SBus interrupt
 *	if no sbus int pending, drop out bottom
 *      if anyone on sbus list claims it
 *         branch to int_rtt
 *	set parameters for "spurious sbus level LEVEL" report
 */
#define SBINTR(LEVEL)							\
	srl	%l4, 7, %g1						;\
	andcc	%g1, 1<<(LEVEL-1), %g0	/* see if sbus int pending */	;\
	bz	3f			/* if not, skip around. */	;\
	.empty				/* next safe in trailer */	;\
	set	_slvl/**/LEVEL, %l5 /* get vector ptr */		;\
1:				/* try another entry */			;\
	ld	[%l5 + AV_VECTOR], %g1 /* get routine address */	;\
	tst	%g1		/* end of vector? */			;\
	bz	2f		/* if it is, */				;\
	nop			/*   report spurious */			;\
	call	%g1		/* go there */				;\
	nop								;\
	tst	%o0		/* success? */				;\
	bz,a	1b		/* no, try next one */			;\
	add	%l5, AUTOVECSIZE, %l5	/* delay slot, find next */	;\
	ld	[%l5 + AV_INTCNT], %g1	/* increment interrupt ctr */	;\
	inc	%g1							;\
	st	%g1, [%l5 + AV_INTCNT]					;\
	sethi	%hi(_slvl/**/LEVEL/**/_spurious), %g1			;\
	b	int_rtt			/* REALLY all done! */		;\
	clr	[%g1 + %lo(_slvl/**/LEVEL/**/_spurious)]		;\
2:				/* unclaimed sbus */			;\
	set	_slvl/**/LEVEL/**/_spurious, %l0			;\
	set	LEVEL, %l1						;\
	set	_busname_svec, %l2					;\
3:				/* not an SBus interrupt */

#ifdef	VME
/*
 * VBINTR: VME interrupt
 *	if no vme int pending, drop out bottom
 *	get vme vector
 *	if someone interested in this vector, call him;
 *	   assume he handled it, branch to int_rtt
 *	if anyone on vme list claims it, branch to int_rtt
 *	set parameters for "spurious vme level LEVEL" report
 */
#define VBINTR(LEVEL)							\
	andcc	%l4, 1<<(LEVEL-1), %g0	/* see if vme int pending */	;\
	bz	3f			/* if not, skip around. */	;\
	sub	%g0, 1, %o1	/* if load traps, we use this value. */	;\
	sethi	%hi(V_VMEBUS_VEC+(LEVEL<<1)+1), %l5			;\
	ldub	[%l5+%lo(V_VMEBUS_VEC+(LEVEL<<1)+1)], %o1 /* ack int */ ;\
	cmp	%o1, VEC_MAX	/* if vector number too high, */	;\
	bg	0f		/* ... not a vectored interrupt. */	;\
	subcc	%o1, VEC_MIN, %o2	/* tables start at VEC_MIN */	;\
	bl	0f		/* vector too low, ignore. */		;\
	sll	%o2, 3, %g1	/* eight bytes per vector */		;\
	set	_vme_vector, %l5					;\
	add	%g1, 4, %g2	/* offset for handler argument */	;\
	ld	[%l5+%g1], %g1	/* get function that will handle it */	;\
	tst	%g1	/* NULL == nobody claimed this vector */	;\
	beq	0f		/* if so, just wander level list */	;\
	ld	[%l5+%g2], %o0	/* get address of handler arg */	;\
	ld	[%o0], %o0	/* ... get handler argument */		;\
	sll	%o2, 2, %g5	/* four bytes per count */		;\
	set	_intrcnt, %l5	/* locate vec-int table */		;\
	ld	[%l5+%g5], %g4						;\
	inc	%g4		/* update count for this vector */	;\
	st	%g4, [%l5+%g5]						;\
	call	%g1		/* call vector handler */		;\
	nop								;\
	b	1f		/* .. clean up and return */		;\
	sethi	%hi(_vlvl/**/LEVEL/**/_spurious), %g1			;\
0:				/* no vme vector, try auto chain */	;\
	set	_vlvl/**/LEVEL, %l5 /* get vector ptr */		;\
	ld	[%l5 + AV_VECTOR], %g1 /* get routine address */	;\
	tst	%g1		/* end of vector? */			;\
	bz	2f		/* if it is, */				;\
	nop			/*   report spurious */			;\
	call	%g1		/* go there */				;\
	nop								;\
	tst	%o0		/* success? */				;\
	bz,a	0b		/* no, try next one */			;\
	add	%l5, AUTOVECSIZE, %l5	/* delay slot, find next */	;\
	ld	[%l5 + AV_INTCNT], %g1	/* increment interrupt ctr */	;\
	inc	%g1							;\
	st	%g1, [%l5 + AV_INTCNT]					;\
	sethi	%hi(_vlvl/**/LEVEL/**/_spurious), %g1			;\
1:				/* int handled */			;\
	b       int_rtt                 /* REALLY all done. */          ;\
	clr	[%g1 + %lo(_vlvl/**/LEVEL/**/_spurious)]		;\
2:				/* unclaimed vme */			;\
	set	_vlvl/**/LEVEL/**/_spurious, %l0			;\
	set	LEVEL, %l1						;\
	set	_busname_vvec, %l2					;\
3:				/* not a vme interrupt */
#endif	VME

/*
 * SOINTR: software interrupt
 * call ALL attached routines
 * branch to int_rtt when done.
 * NOTE: scans the "_xlvl" list AND the "_level" list.
 * NOTE: silently ignore spurious soft ints.
 */
#define SOINTR(LEVEL)							\
	set	_xlvl/**/LEVEL, %l5 /* get vector ptr */		;\
1:	ld	[%l5 + AV_VECTOR], %g1 /* get routine address */	;\
	tst	%g1	/* all done? */					;\
	beq	2f							;\
	nop								;\
	call	%g1		/* go there */				;\
	add	%l5, AUTOVECSIZE, %l5	/* delay slot, find next */	;\
	tst	%o0							;\
	beq	1b							;\
	ld	[%l5 + AV_INTCNT], %g1	/* increment interrupt ctr */	;\
	st	%g1, [%l5 + AV_INTCNT]					;\
	b	1b		/* go get another */			;\
	nop								;\
2:	set	_level/**/LEVEL, %l5 /* get vector ptr */		;\
1:	ld	[%l5 + AV_VECTOR], %g1 /* get routine address */	;\
	tst	%g1	/* all done? */					;\
	beq	2f							;\
	nop								;\
	call	%g1		/* go there */				;\
	add	%l5, AUTOVECSIZE, %l5	/* delay slot, find next */	;\
	tst	%o0							;\
	beq	1b							;\
	ld	[%l5 + AV_INTCNT], %g1	/* increment interrupt ctr */	;\
	st	%g1, [%l5 + AV_INTCNT]					;\
	b	1b		/* go get another */			;\
	nop								;\
2:	sethi	%hi(_level/**/LEVEL/**/_spurious), %g1			;\
	clr	[%g1 + %lo(_level/**/LEVEL/**/_spurious)]		;\
	b,a	int_rtt

#ifdef	MULTIPROCESSOR
	.seg	"data"
	.align	4
lcfile:
	.asciz	"__FILE__"
	.align	4
	.seg	"text"
	.align	4

#ifdef	KLOCK_PARANOID
#define	KLREQ					\
	set	lcfile, %o0		;	\
	set	__LINE__, %o1		;	\
	call	_klock_require		;	\
	mov	%g0, %o2
#endif	KLOCK_PARANOID

#endif	MULTIPROCESSOR

#ifndef	KLREQ
#define	KLREQ
#endif

softlvl1:
	KLREQ
	SOINTR(1)
softlvl2:
	KLREQ
	SOINTR(2)
softlvl3:
	KLREQ
	SOINTR(3)
softlvl4:
	KLREQ
	SOINTR(4)
softlvl5:
	KLREQ
	SOINTR(5)
softlvl6:
	KLREQ
	SOINTR(6)
softlvl7:
	KLREQ
	SOINTR(7)
softlvl8:
	KLREQ
	SOINTR(8)
softlvl9:
	KLREQ
	SOINTR(9)
softlvl10:
	SOINTR(10)
softlvl11:
	SOINTR(11)
softlvl12:
	SOINTR(12)
softlvl13:
	SOINTR(13)
softlvl14:
	SOINTR(14)
softlvl15:
#ifdef	PC_prom_mailbox
!
! given a prom mailbox message, branch to the
! proper prom service (via the prom library).
! prom services will return to the above label.
!
	set	_prom_mailbox, %o0	! find our mailbox
	andn	%o0, 0xFFF, %o1		! Truncate to page base
	lda	[%o1]ASI_FLPR, %o1
	and	%o1, 3, %o1
	subcc	%o1, 2, %o1		! if mbox ptr mapped in,
	bne	1f
	nop

	ld	[%o0], %o0		! and mbox ptr not null,
	tst	%o0
	beq	1f
	nop

	ldub	[%o0], %o0		! get mailbox message

	cmp	%o0, 0xFB		! 0xFB: someone did an op_exit
	bne	2f
	nop
	call	_prom_stopcpu		! prom wants us to "stop"
	mov	%g0, %o0
	b	1f
	nop
2:
	cmp	%o0, 0xFC		! 0xFC: someone did an op_enter
	bne	2f
	nop
	call	_prom_idlecpu		! prom wants us to "idle"
	mov	%g0, %o0
	b	1f
	nop
2:
	cmp	%o0, 0xFD		! 0xFD: someone hit a breakpoint
	bne	2f
	nop
	call	_prom_idlecpu		! prom wants us to "idle"
	mov	%g0, %o0
	b	1f
	nop
2:
	cmp	%o0, 0xFE		! 0xFE: someone watchdogged
	bne	2f
	nop
	call	_prom_stopcpu		! prom wants us to "stop"
	mov	%g0, %o0
	b	1f
	nop
2:
	!! add more prom mailbox codes here.

1:	! no prom calls, or prom call done.

#endif	PC_prom_mailbox
#ifdef	MULTIPROCESSOR
!
! Let the crosscall mechanism have a crack at its data.
!
	call	_xc_serv
	nop
#endif	MULTIPROCESSOR
!
! Do not count this interrupt in cnt.v_intr
!
	b	sys_rtt
	mov	%l7, %sp

/*
 * Level 1 interrupts, from out of the blue.
 */
level1:
	KLREQ
	SYNCWB
	NSSET(1)
	IOINTR(1)

/*
 * Level 2 interrupts, from Sbus level 1, or VMEbus level 1.
 */
level2:
	KLREQ
	SYNCWB
	NSSET(2)
	SBINTR(1)
#ifdef	VME
	VBINTR(1)
#endif	VME
	IOINTR(2)

/*
 * Level 3 interrupts, from Sbus level 2, or VMEbus level 2
 */
level3:
	KLREQ
	SYNCWB
	NSSET(3)
	SBINTR(2)
#ifdef	VME
	VBINTR(2)
#endif	VME
	IOINTR(3)

/*
 * Level 4 interrupts, from onboard SCSI
 */
level4:
	KLREQ
	SYNCWB
	NSSET(4)
	OBINTR(4, SIR_SCSI)
	IOINTR(4)

/*
 * Level 5 interrupts, from Sbus/VME level 3
 */
level5:
	KLREQ
	SYNCWB
	NSSET(5)
	SBINTR(3)
#ifdef	VME
	VBINTR(3)
#endif	VME
	IOINTR(5)

/*
 * Level 6 interrupts, from onboard ethernet
 */
level6:
	KLREQ
	SYNCWB
	NSSET(6)
	OBINTR(6, SIR_ETHERNET)
	IOINTR(6)

/*
 * Level 7 interrupts, from SBus/VME level 4
 */
level7:
	KLREQ
	SYNCWB
	NSSET(7)
	SBINTR(4)
#ifdef	VME
	VBINTR(4)
#endif	VME
	IOINTR(7)

/*
 * Level 8 interrupts, from onboard video
 */
level8:
	KLREQ
	SYNCWB
	NSSET(8)
	OBINTR(8, SIR_VIDEO)
	IOINTR(8)

/*
 * Level 9 interrupts, from SBus/VME level 5 and module interrupt
 */
level9:
	KLREQ
	SYNCWB
	NSSET(9)
	OBINTR(9, SIR_MODULEINT)
	SBINTR(5)
#ifdef	VME
	VBINTR(5)
#endif	VME
	IOINTR(9)

/*
 * Level 10 interrupts, from onboard clock
 *
 * REORGANIZATION NOTE:
 *	level 10 hardint should be handled
 *	by the "real time clock driver", which
 *	should be linked into the level-10
 *	onboard vector. In turn, it should
 *	trigger the LED update either by directly
 *	calling the "led display driver" or by
 *	pending a level-10 software interrupt.
 *	The softint route is more general.
 *	Making these separate device modules
 *	will make it easier to work with possible
 *	alternate implementations (like different
 *	numbers of LEDs, or different clock chips).
 *
 *	Not Yet, tho.
 */
#if	0
level10:
	KLREQ
	NSSET(10)
	OBINTR(10, SIR_REALTIME)
	IOINTR(10)
#endif

/*
 * Level 11 interrupts, from SBus/VME level 6 or floppy
 * NOTE: FLOPPY DRIVER SHOULD BE MOVED FROM TRAP LEVEL, IF POSSIBLE
 */
level11:
	KLREQ
	SYNCWB
	NSSET(11)
	OBINTR(11, SIR_FLOPPY)
	SBINTR(6)
#ifdef	VME
	VBINTR(6)
#endif	VME
	IOINTR(11)

/*
 * Level 12 interrupts, from onboard serial ports
 */
level12:
	KLREQ
	SYNCWB
	NSSET(12)
	OBINTR(12, SIR_KBDMS|SIR_SERIAL)
	IOINTR(12)

/*
 * Level 13 interrupts, from SBus/VME level 7, or onboard audio
 * NOTE: AUDIO DRIVER SHOULD BE MOVED FROM TRAP LEVEL, IF POSSIBLE
 */
level13:
	KLREQ
	SYNCWB
	NSSET(13)
	OBINTR(13, SIR_AUDIO)
	SBINTR(7)
#ifdef	VME
	VBINTR(7)
#endif	VME
	IOINTR(13)

level14:
	IOINTR(14)

level15:
	/*NSSET(15)*/
	call	_l15_async_fault
	mov	%l4, %o0
	b,a	int_rtt
	/*IOINTR(15)*/

	.seg	"text"
/*
 * This code assumes that the real time clock interrupts 100 times
 * per second, for SUN4M we call hardclock at that rate.
 */
	.seg	"bss"
	.align	4
.globl	_clk_intr
_clk_intr:
	.skip 4
	.seg "text"

	.global level10
level10:
	KLREQ
	set	COUNTER_ADDR+CNTR_LIMIT10, %l5 ! read limit10 reg
	ld	[%l5], %g1		! to clear intr

	sethi	%hi(_clk_intr), %g2	! count clock interrupt
	ld	[%lo(_clk_intr) + %g2], %g3
	inc	%g3
	st	%g3, [%lo(_clk_intr) + %g2]

	ld	[%l7 + MINFRAME + PC*4], %o0 ! pc
#ifdef	MEASURE_KERNELSTACK
	ld	[%l7 + MINFRAME + SP*4], %o2 ! sp /*XXX-instrument*/
#endif	MEASURE_KERNELSTACK
	call	_hardclock
	ld	[%l7 + MINFRAME + PSR*4], %o1 ! psr

	b,a	int_rtt			! return to restore previous stack
/* end level10 */

/*
 * Level 14 interrupts can be caused by the clock when
 * kernel profiling is enabled. It is handled immediately
 * in the trap window.
 */
#ifdef	GPROF
	.global test_prof
test_prof:
#ifndef	PC_utimers
	GETCPU(%l4)			! get cpu index, 0..3
	sll	%l4,12,%l4		! offset 0x1000 each
	set	COUNTER_ADDR, %l3	! address of counter reg.
	add	%l3,%l4,%l3		! add module number
	ld	[%l3],%g0		! read limit register
#else	PC_utimers
	sethi	%hi(_utimers), %l3	! read my timer limit register
	ld	[%l3 + %lo(_utimers)], %g0
#endif	PC_utimers

	sethi	%hi(_mon_clock_on), %l3 ! see if profiling is enabled
	ldub	[%l3 + %lo(_mon_clock_on)], %l3
	tst	%l3
	bz	kprof			! profiling on, do it.
	nop
	b	sys_trap		! do normal interrupt processing
	mov	(T_INTERRUPT | 14), %l4
#endif	GPROF

/*
 * Flush all windows to memory, except for the one we entered in.
 * We do this by doing NWINDOW-2 saves then the same number of restores.
 * This leaves the WIM immediately before window entered in.
 * This is used for context switching.
 */
	ENTRY(flush_windows)
	save	%sp, -WINDOWSIZE, %sp
	save	%sp, -WINDOWSIZE, %sp
	save	%sp, -WINDOWSIZE, %sp
	save	%sp, -WINDOWSIZE, %sp
	save	%sp, -WINDOWSIZE, %sp
	save	%sp, -WINDOWSIZE, %sp
#if 0
	save	%sp, -WINDOWSIZE, %sp	! one for the road
	restore				! one for the road
#endif
	restore
	restore
	restore
	restore
	restore
	ret
	restore

/*
 * flush user windows to memory.
 * This is a leaf routine that only wipes %g1, %g2, and %g5.
 * Some callers may depend on this behavior.
 */
	ENTRY(flush_user_windows)
	sethi	%hi(_uunix), %g5	! XXX - global u register?
	ld	[%g5 + %lo(_uunix)], %g5
	ld	[%g5 + PCB_UWM], %g1	! get user window mask
	tst	%g1			! do save until mask is zero
	bz	3f
	clr	%g2
1:
	save	%sp, -WINDOWSIZE, %sp
	sethi	%hi(_uunix), %g5	! XXX - global u register?
	ld	[%g5 + %lo(_uunix)], %g5
	ld	[%g5 + PCB_UWM], %g1	! get user window mask
	tst	%g1			! do save until mask is zero
	bnz	1b
	add	%g2, 1, %g2
2:
	subcc	%g2, 1, %g2		! restore back to orig window
	bnz	2b
	restore
3:
	retl
	nop

/*
 * Throw out any user windows in the register file.
 * Used by setregs (exec) to clean out old user.
 * Used by sigcleanup to remove extraneous windows when returning from a
 * signal.
 */
	ENTRY(trash_user_windows)
	sethi	%hi(_uunix), %g5		! XXX - global u register?
	ld	[%g5 + %lo(_uunix)], %g5
	ld	[%g5 + PCB_UWM], %g1	! get user window mask
	tst	%g1
	bz	3f			! user windows?
	nop
	!
	! There are old user windows in the register file. We disable traps
	! and increment the WIM so that we don't overflow on these windows.
	! Also, this sets up a nice underflow when first returning to the
	! new user.
	!
	mov	%psr, %g4
	or	%g4, PSR_PIL, %g1	! spl hi to prevent interrupts
	mov	%g1, %psr
	nop; nop; nop			! psr delay
	ld	[%g5 + PCB_UWM], %g1	! get user window mask
	clr	[%g5 + PCB_UWM]		! throw user windows away
	sethi	%hi(_nwindows), %g5
	ld	[%g5+%lo(_nwindows)], %g5
	b	2f
	dec	%g5			! %g5 == NW-1

1:
	srl	%g2, 1, %g3		! next WIM = ror(WIM, 1, NW)
	sll	%g2, %g5, %g2		! %g5 == NW-1
	or	%g2, %g3, %g2
	mov	%g2, %wim		! install wim
	bclr	%g2, %g1		! clear bit from UWM
2:
	tst	%g1			! more user windows?
	bnz,a	1b
	mov	%wim, %g2		! get wim

	mov	%g4, %psr		! enable traps
	nop				! psr delay
3:
	sethi	%hi(_uunix), %g5	! XXX - global u register?
	ld	[%g5 + %lo(_uunix)], %g5
	retl
	clr	[%g5 + PCB_WBCNT]		! zero window buffer cnt

/*
 * Clean out register file.
 */
clean_windows:
	sethi	%hi(_uunix), %l5	! XXX - global u register?
	ld	[%l5 + %lo(_uunix)], %l5
	ld	[%l5 + PCB_FLAGS], %l4	! set CLEAN_WINDOWS in pcb_flags
	mov	%wim, %l3
	bset	CLEAN_WINDOWS, %l4
	st	%l4, [%l5 + PCB_FLAGS]
	srl	%l3, %l0, %l3		! test WIM bit
	btst	1, %l3
	bnz,a	cw_out			! invalid window, just return
	mov	%l0, %psr		! restore PSR_CC

	mov	%g1, %l5		! save some globals
	mov	%g2, %l6
	mov	%g3, %l7
	mov	%wim, %g2		! put wim in global
	mov	0, %wim			! zero wim to allow saving
	mov	%l0, %g3		! put original psr in global
	b	2f			! test next window for invalid
	save
	!
	! Loop through windows past the trap window
	! clearing them until we hit the invlaid window.
	!
1:
	clr	%l1			! clear the window
	clr	%l2
	clr	%l3
	clr	%l4
	clr	%l5
	clr	%l6
	clr	%l7
	clr	%o0
	clr	%o1
	clr	%o2
	clr	%o3
	clr	%o4
	clr	%o5
	clr	%o6
	clr	%o7
	save
2:
	mov	%psr, %g1		! get CWP
	srl	%g2, %g1, %g1		! test WIM bit
	btst	1, %g1
	bz,a	1b			! not invalid window yet
	clr	%l0			! clear the window

	!
	! Clean up trap window.
	!
	mov	%g3, %psr		! back to trap window, restore PSR_CC
	mov	%g2, %wim		! restore wim
	nop; nop;			! psr delay
	mov	%l5, %g1		! restore globals
	mov	%l6, %g2
	mov	%l7, %g3
	mov	%l2, %o6		! put npc in unobtrusive place
	clr	%l0			! clear the rest of the window
	clr	%l1
	clr	%l2
	clr	%l3
	clr	%l4
	clr	%l5
	clr	%l6
	clr	%l7
	clr	%o0
	clr	%o1
	clr	%o2
	clr	%o3
	clr	%o4
	clr	%o5
	clr	%o7
	jmp	%o6			! return to npc
	rett	%o6 + 4

cw_out:
	nop				! psr delay
	jmp	%l2			! return to npc
	rett	%l2 + 4

/*
 * Enter the monitor -- called from console abort
 */
	ENTRY(montrap)
	save	%sp, -SA(MINFRAME), %sp ! get a new window
	call	_flush_windows		! flush windows to stack
	nop

	call	%i0			! go to monitor
	nop
	ret
	restore

/*
 * return the condition codes in %g1
 */
getcc:
	sll	%l0, 8, %g1		! right justify condition code
	srl	%g1, 28, %g1
	jmp	%l2			! return, skip trap instruction
	rett	%l2+4

/*
 * set the condtion codes from the value in %g1
 */
setcc:
	sll	%g1, 20, %l5		! mov icc bits to their position in psr
	set	PSR_ICC, %l4		! condition code mask
	andn	%l0, %l4, %l0		! zero the current bits in the psr
	or	%l5, %l0, %l0		! or new icc bits
	mov	%l0, %psr		! write new psr
	nop ; nop ; nop			! psr delay
	jmp	%l2			! return, skip trap instruction
	rett	%l2+4

/*
 * some user has to do unaligned references, yuk!
 * set a flag in the pcb so that when alignment traps happen
 * we fix it up instead of killing the user
 * Note: this routine is using the trap window
 */
fix_alignment:
	sethi	%hi(_uunix), %l5
	ld	[%l5 + %lo(_uunix)], %l5
	ld	[%l5 + PCB_FLAGS], %l4	! get pcb_flags
	bset	FIX_ALIGNMENT, %l4
	st	%l4, [%l5 + PCB_FLAGS]
	jmp	%l2			! return, skip trap instruction
	rett	%l2+4

/*
 * icode1
 * When a process is created by main to do init, it starts here.
 * We hack the stack to make it look like a system call frame.
 * Then, icode exec's init.
 */
	ENTRY(icode1)
	call	_icode
	add	%sp, MINFRAME, %o0	! pointer to struct regs
	b,a	sys_rtt

/*
 * Glue code for traps that should take us to the monitor/kadb if they
 * occur in kernel mode, but that the kernel should handle if they occur
 * in user mode.
 */
	.global _kadb_tcode, _trap_ff_tcode, _trap_fe_tcode

/* tcode to replace trap vectors if kadb steals them away */
_trap_ff_tcode:
	mov	%psr, %l0
	sethi	%hi(trap_mon), %l4
	jmp	%l4 + %lo(trap_mon)
	mov	0xff, %l4
_trap_fe_tcode:
	mov	%psr, %l0
	sethi	%hi(trap_kadb), %l4
	jmp	%l4 + %lo(trap_kadb)
	mov	0xfe, %l4
/*
 * This code assumes that:
 * 1. the monitor uses trap ff to breakpoint us
 * 2. kadb steals both ff and fe when we call scbsync()
 * 3. kadb uses the same tcode for both ff and fe.
 * XXX The monitor shouldn't use the same trap as kadb!
 */
trap_mon:
	btst	PSR_PS, %l0		! test pS
	bz	sys_trap		! user-mode, treat as bad trap
	nop

	mov	%l0, %psr		! restore psr
	nop				! psr delay
	b	mon_breakpoint_vec
	nop				! psr delay

trap_kadb:
	btst	PSR_PS, %l0		! test pS
	bz	sys_trap		! user-mode, treat as bad trap
	nop

	mov	%l0, %psr		! restore psr
	nop				! psr delay
	b	_kadb_tcode
	nop				! psr delay

	.align	8	! mon_breakpoint_vec MUST BE DOUBLE ALIGNED.
	.global	mon_breakpoint_vec
mon_breakpoint_vec:
	SYS_TRAP(0xff)			! gets overlaid with data from prom.
_kadb_tcode:
	SYS_TRAP(0xfe)			! gets overlaid with data from prom.

#if defined(SAS)
	ALTENTRY(mpsas_idle_trap)
	retl
	t	211	! stall here until something fun happens.
#endif

	ALTENTRY(check_cpuid)	! int check_cpuid(int *exp, int *act): 0=ok

	mov	%psr, %g1
	andn	%g1, PSR_ET, %o5
	mov	%o5, %psr		! disable traps
	nop ; nop ; nop

	mov	0, %o2

	set	_cpuid, %o5

        sethi   %hi(_cache), %o3
	ld      [%o3 + %lo(_cache)], %o3
	cmp	%o3, CACHE_PAC
	bge,a	2f			! PAC (viking), skip ross check
	ld	[%o5], %o3		! from pac, get cpuid

	lda	[%g0]ASI_MOD, %o4
	srl	%o4, 15, %o4
	and	%o4, 15, %o4
	cmp	%o4, 15			! if we are a Ross 604,
	be	0f			! no tests needed.
	nop

	ld	[%o5], %o4		! from vac
	set	CACHE_BYTES-1, %g2
	and	%o5, %g2, %o3		! modulo cache size
	set	PAGEMASK, %g2
	and	%o3, %g2, %o3		! round down to base of page
	set	KERNELBASE + CACHE_BYTES, %g2
	ld	[%g2 + %o3], %g0	! replace the line
	ld	[%o5], %o3		! from memory
	cmp	%o3, %o4
	bne	1f			! ERROR: flushing vac changed cpuid
	mov	1, %o2
!	inc	%o2

2:	mov	%o3, %o4		! possible tlb hit
	sta	%g0, [%o5]ASI_FLPR	! flush tlb
	ld	[%o5], %o3		! force tlbe miss
	cmp	%o3, %o4
	bne	1f			! ERROR: flushing tlb changed cpuid
	mov	2, %o2			! change inc to mov because VIKING
					! skips the flushing vac test
!	inc	%o2

	mov	%o3, %o4
	GETCPU(%o3)
	cmp	%o3, %o4
	bne	1f			! ERROR: cpuid does not match tbr
	mov	3, %o2
!	inc	%o2

        sethi   %hi(_cache), %o4
	ld      [%o4 + %lo(_cache)], %o4
	tst	%o4
	bz	0f			! skip the next test if non-cached or
					! Viking MBus mode.
	nop
	cmp	%o4, CACHE_PAC
	be	0f
	nop
	cmp	%o4, CACHE_PAC_E	! if VIking CC mode, load MID from
					! MXCC
	be	3f
	nop

	mov	%o3, %o4
	lda	[%g0]ASI_MOD, %o3
	srl	%o3, 15, %o3
	and	%o3, 3, %o3		! cpu number from MCR.MID
	cmp	%o3, %o4
	bne	1f			! ERROR: tbr does not match MCR.MID
	mov	4, %o2
!	inc	%o2
	b	0f
	nop

3:	mov	%o3, %o4
	set	MXCC_PORT, %o3
	lda	[%o3]ASI_MXCC, %o3
	srl	%o3, 24, %o3
	and	%o3, 3, %o3		! cpu number from MXCC's Mbus port reg.
	cmp	%o3, %o4
	bne	1f			! ERROR: tbr does not match MXCC.MID
	mov	4, %o2

!!! add more tests here as desired.

0:				! branch here on success
	mov	0, %o2
1:				! branch here on failure

	nop ; nop ; nop
	mov	%g1, %psr		! re-enable traps
	nop ; nop ; nop

	st	%o3, [%o0]		! stash expected value
	st	%o4, [%o1]		! stash actual value
	retl
	mov	%o2, %o0		! return error code

/*
 * Turn on or off bits in the auxiliary i/o register.
 * We must lock out interrupts, since we don't have an atomic or/and to mem.
 * set_auxioreg(bit, flag)
 *      int bit;                bit mask in aux i/o reg
 *      int flag;               0 = off, otherwise on
 */
        ALTENTRY(set_auxioreg)
        mov     %psr, %g2
        or      %g2, PSR_PIL, %g1       ! spl hi to protect aux i/o reg update
        mov     %g1, %psr
        nop;                            ! psr delay
        tst     %o1
        set     AUXIO_REG, %o2          ! aux i/o register address
        ldub    [%o2], %g1              ! read aux i/o register
        bnz,a   1f
        bset    %o0, %g1                ! on
        bclr    %o0, %g1                ! off
1:
        or      %g1, AUX_MBO, %g1       ! Must Be Ones
        stb     %g1, [%o2]              ! write aux i/o register
        mov     %g2, %psr               ! splx
        nop                             ! psr delay
        retl
        nop 
/* end set_auxioreg */

/*
 * Trigger routine for C2 lab logi analyzer
 */
	ENTRY(trigger)

	set	0xE0000000, %o0
	retl
	lda	[%o0]0x2f, %g0

