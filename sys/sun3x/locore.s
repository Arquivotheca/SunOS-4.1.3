	.data
	.asciz	"@(#)locore.s 1.1 92/07/30"
	.even
	.text
|syncd to sun3/locore.s 1.70
|syncd to sun3/locore.s 1.88 (4.1)

/*
 *	Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/vmparam.h>
#include <sys/errno.h>
#include <netinet/in_systm.h>
#include <sundev/p4reg.h>

#include <machine/asm_linkage.h>
#include <machine/buserr.h>
#include <machine/clock.h>
#include <machine/cpu.h>
#include <machine/diag.h>
#include <machine/enable.h>
#include <machine/interreg.h>
#include <machine/memerr.h>
#include <machine/mmu.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include "fpa.h"

#include "assym.s"

/* 
 * Absolute external symbols
 */
	.globl	_msgbuf, _scb, _DVMA
/*
 * On the sun3X we put the message buffer and scb on the same page.
 * We set things up so that the first page of KERNELBASE is illegal
 * to act as a redzone during copyin/copyout type operations.  One of
 * the reasons the message buffer is allocated in low memory is to
 * prevent being overwritten during booting operations (besides
 * the fact that it is small enough to share a page with others).
 */
_msgbuf	= KERNELBASE + MMU_PAGESIZE	| base addr for message buffer
_scb	= _msgbuf + MSGBUFSIZE	| base addr for vector table
_DVMA	= 0xFFF00000		| 32 bit virtual address for system DVMA

#if (MSGBUFSIZE + SCBSIZE) > MMU_PAGESIZE
ERROR - msgbuf and scb larger than a page
#endif

/*
 * The interrupt stack.  This must be the first thing in the data
 * segment (other than an sccs string) so that we don't stomp
 * on anything important during interrupt handling.  We get a
 * red zone below this stack for free when the kernel text is
 * write protected.  Since the kernel is loaded with the "-N"
 * flag, we pad this stack by a page because when the page
 * level protection is done, we will lose part of this interrupt
 * stack.  Thus the true interrupt stack will be at least MIN_INTSTACK_SZ
 * bytes and at most MIN_INTSTACK_SZ+MMU_PAGESIZE bytes.  The interrupt entry
 * code assumes that the interrupt stack is at a lower address than
 * both eintstack and the kernel stack in the u area.
 */
#define	MIN_INTSTACK_SZ	0x1800
	.data
	.align	4
	.globl _intstack, eintstack, _ubasic, eexitstack
_intstack:				| bottom of interrupt stack
	. = . + MMU_PAGESIZE + MIN_INTSTACK_SZ
eintstack:				| end (top) of interrupt stack
	. = . + 4			| disambiguate to help debugging

_exitstack:				| bottom of exit stack
	. = . + MMU_PAGESIZE + MIN_INTSTACK_SZ
eexitstack:				| end (top) of exit stack
	. = . + 4			| disambiguate to help debugging

_ubasic:				| bottom of struct seguser
	. = . + KERNSTACK
_ustack:				| top of proc 0's stack, start u area
	. = . + USIZE
intu:
	. = . + USIZE			| fake uarea for kernel
					| when no process (as when exiting)
	.globl _uunix			| pointer to active u area
_uunix:	.long	_ustack			| initial uunix value

/*
 * Kernel page tables. Must be 16 byte aligned.
 * calculate _kl1pt and _kl2pts at runtime
 */
	.globl _kptstart
_kptstart:	.=.+(0x10 + KL1PT_SIZE + (KL2PT_SIZE*NKL2PTS))

/*
 * System software page tables
 */
#define vaddr(x)	((((x)-_Sysmap)/4)*MMU_PAGESIZE + SYSBASE)
#define SYSMAP(mname, vname, npte)	\
	.globl	mname;			\
mname:	.=.+(4*npte);			\
	.globl	vname;			\
vname = vaddr(mname);
	SYSMAP(_Sysmap   ,_Sysbase	,SYSPTSIZE	)
	SYSMAP(_CMAP1    ,_CADDR1	,1		) | local tmp
	SYSMAP(_CMAP2    ,_CADDR2	,1		) | local tmp
	SYSMAP(_mmap     ,_vmmap        ,1		)
	SYSMAP(_Mbmap    ,_mbutl        ,MBPOOLMMUPAGES)
	SYSMAP(_ESysmap	 ,_Syslimit	,0		) | must be last

	.globl  _Syssize
_Syssize = (_ESysmap-_Sysmap)/4

	/*
	 * Old names
	 */
	.globl	_Usrptmap, _usrpt
_Usrptmap = _Sysmap
_usrpt = _Sysbase

/*
 * Software copy of system enable register
 * This is always atomically updated
 */
	.data
	.globl	_enablereg
_enablereg:	.word	0			| UNIX's system enable register

/*
 * Scratch location for use by trap handler while aligning the stack
 */
	.align 4
trpscr:	.long	0
needsoftint: .long 0
	.text

/*
 * Macro to save all registers and switch to kernel context
 * 1: Save and switch context
 * 2: Save all registers
 * 3: Save user stack pointer
 */
#define SAVEALL() \
	clrw	sp@-;\
	moveml	d0-d7/a0-a7,sp@-;\
	movl	usp,a0;\
	movl	a0,sp@(R_SP)

/* Normal trap sequence */
/*
 * We may need to add a 16 bit pad below the pc here. The pc and sr have already
 * been pushed, so we need to move them down by a word, creating the word
 * pad. This will allow the trap routine to run with the stack longword aligned.
 * By clearing the pad, it will be ignored in trap() and popped off in rei.
 */
#define TRAP(type) \
	movl	sp,trpscr;\
	andl	#3,trpscr;\
	bne	9f;\
	subql	#2,sp;\
	movw	sp@(2),sp@;\
	movl	sp@(4),sp@(2);\
	clrw	sp@(6);\
9:	SAVEALL();\
	movl	#type,sp@-;\
	jra	trap

/*
 * System initialization
 * UNIX receives control at the label `_start' which
 * must be at offset zero in this file; this file must
 * be the first thing in the boot image.
 */
	.text
	.globl	_start
_start:

/*
 * We should reset the world here, but it screws the UART settings.
 *
 * Sets up to run out of temporary ptes so we get running at the right
 * addresses.
 *
 * We make the following assumptions about our environment
 * as set up by the monitor:
 *
 *	- we have enough memory mapped for the entire kernel + some more
 *	- the beginning of dvma space is mapped to real memory
 *	- all pages are writable
 *	- the monitor's scb is NOT in low memory
 *	- on systems w/ ecc/mixed memory, that the monitor has set the base
 *	    addresses and enabled all the memory cards correctly
 *	- MMU is set up in following way:
 *		XXX
 *
 * We will set the protections properly in startup().
 */

/*
 * vmunix is linked for running out of high addresses,
 * but gets control running in low addresses.  Thus,
 * before we set up the new mapping and start running with the correct
 * addresses, all of the code must be position independent.
 *
 * We continue to run off the stack set up by boot
 * until after we set up the u area (and the kernel stack).
 */
	movw	#SR_HIGH,sr		| lock out interrupts
	moveq	#FC_MAP,d0
	movc	d0,sfc			| set default sfc to FC_MAP
	movc	d0,dfc			| set default dfc to FC_MAP
	movl	#0x00008107,sp@-	| set up translation window for
	pmove	sp@,tt0			| low memory
	addqw	#4,sp
	movl	#_load_tmpptes,d2	| calculate real location of routine
	subl	#KERNELBASE,d2
	movl	d2,a2
	jsr	a2@			| set up tmp ptes
	movl	#rtptr,d2		| calculate real address of variable
	subl	#KERNELBASE,d2
	movl	d2,a2
	movl	d0,a2@(4)		| fill in address of table
	pmoveflush a2@,crp		| load hardware root ptr
	movl	#ICACHE_CLEAR+ICACHE_ENABLE+DCACHE_CLEAR+DCACHE_ENABLE,d0
	movc	d0,cacr			| clear (and enable) the caches
	jmp	cont:l			| force non-PC rel branch
cont:

/*
 * PHEW!  Now we are running with correct addresses
 * and can use non-position independent code.
 */
	jsr	_early_startup		| map stuff in plus other misc

/*
 * Set up the stack.  From now we continue to use the 68030 ISP
 * (interrupt stack pointer).  This is because the MSP (master
 * stack pointer) as implemented by Motorola is too painful to
 * use since we have to play lots of games and add extra tests
 * to set something in the master stack if we running on the
 * interrupt stack and we are about to pop off a throw away
 * stack frame.
 *
 * Thus it is possible to have naming conflicts.  In general,
 * when the term "interrupt stack" (no pointer) is used, it
 * is referring to the software implemented interrupt stack.
 * The "kernel stack" is the per user kernel stack in the
 * user area.  We handling switching between the two different
 * address ranges upon interrupt entry/exit.  We will use ISP
 * and MSP if we are referring to the hardstack stack pointers.
 */

	lea	_ustack, sp		| stack for proc 0

/*
 * Disable transparent translation of low memory, since
 * from here on we no longer use the boot-provided stack.
 */
	movl	#0x00000107,sp@-	| disable translation window for
	pmove	sp@,tt0			| low memory
	addqw	#4,sp

	/*
	 * See if we have a 68882 attached.
	 * _fppstate is 0 if no fpp,
	 * 1 if fpp is present and enabled,
	 * and -1 if fpp is present but disabled
	 * (not currently used).
	 */
	.data
	.globl	_fppstate
_fppstate:	.word 1		| mark as present until we find out otherwise
fnull:		.long 0		| null fpp internal state
	.text

flinevec = 0x2c
	movw	ENABLEREG,d0		| get the current enable register
	orw	#ENA_FPP,d0		| or in FPP enable bit
	movw	d0,ENABLEREG		| set in the enable register
	movl	sp,a1			| save sp in case of fline fault
	movc	vbr,a0			| get vbr
	movl	a0@(flinevec),d1	| save old f line trap handler
	movl	#ffault,a0@(flinevec)	| set up f line handler
	frestore fnull
	jra	1f			| no fault

ffault:					| handler for no fpp present
	movw	#0,_fppstate		| set global to say no fpp
	andw	#~ENA_FPP,d0		| clear ENA_FPP enable bit
	movl	a1,sp			| clean up stack

1:
	movl	d1,a0@(flinevec)	| restore old f line trap handler
	movw	d0,ENABLEREG		| set up enable reg
	movw	d0,_enablereg		| save soft copy of enable register

| dummy up a stack so process 1 can find saved registers
	lea	USRSTACK,a0		| init user stack pointer
	movl	a0,usp
/*
 * We add a 16 bit pad here so the stack is longword aligned for all
 * system calls and traps from user land. It will only get out of
 * alignment on traps that occur after we are already in the kernel. These
 * will be aligned by code in the trap handler.
 * Note that we have to put a nonzero value in the vor part
 * of the word. This is so rei doesn't mistake the dummy stack frame
 * for a stack pad and pop off too many bytes.
 */
	pea	0x10000			| non-zero pad + dummy fmt & vor
	pea	0			| push dummy pc--filled in by kern_proc
	movw	#SR_LOW, sp@-		| push sr (for proc made by kern_p
	lea	0,a6			| stack frame link 0 in main
	SAVEALL()
	subl	#4, sp@(R_SP)		| simulate system call call by pushing
					| syscall # on user stack
	jsr	_main			| simulate interrupt -> main
	/* NOTREACHED */
	/*
	 * If we did reach here, should "pop" syscall #
	 * "pushed" on usp.
	 */
	pea	2f
	jsr	_panic

2:	.asciz	"setrq"
	.even


/*
 * Entry points for interrupt and trap vectors
 */
	.globl	buserr, addrerr, coprocerr, fmterr, illinst, zerodiv, chkinst
	.globl	trapv, privvio, trace, emu1010, emu1111, spurious
	.globl	badtrap, brkpt, floaterr, level2, level3, level4, level5
	.globl	_level7, errorvec, mmuerr

buserr:
	TRAP(T_BUSERR)

addrerr:
	TRAP(T_ADDRERR)

coprocerr:
	TRAP(T_COPROCERR)

fmterr:
	TRAP(T_FMTERR)

illinst:
	TRAP(T_ILLINST)

zerodiv:
	TRAP(T_ZERODIV)

chkinst:
	TRAP(T_CHKINST)

trapv:
	TRAP(T_TRAPV)

privvio:
	TRAP(T_PRIVVIO)

trace:
	TRAP(T_TRACE)

emu1010:
	TRAP(T_EMU1010)

emu1111:
	TRAP(T_EMU1111)

spurious:
	TRAP(T_SPURIOUS)

badtrap:
	TRAP(T_M_BADTRAP)

brkpt:
	TRAP(T_BRKPT)

floaterr:
	TRAP(T_M_FLOATERR)

mmuerr:
	TRAP(T_M_MMUERR)

errorvec:
	TRAP(T_M_ERRORVEC)

level2:
	IOINTR(2)

level3:
	IOINTR(3)

level4:
	IOINTR(4)

	.data
	.globl	_ledcnt, _ledpat
_ledpat:
	.byte	~0x80; .byte	~0x40; .byte	~0x20; .byte	~0x10
	.byte	~0x08; .byte	~0x04; .byte	~0x02; .byte	~0x01
	.byte	~0x80; .byte	~0xC0; .byte	~0x60; .byte	~0x30
	.byte	~0x18; .byte	~0x0C; .byte	~0x06; .byte	~0x03
	.byte	~0x01; .byte	~0x80; .byte	~0xC0; .byte	~0xE0
	.byte	~0x70; .byte	~0x38; .byte	~0x1C; .byte	~0x0E
	.byte	~0x07; .byte	~0x03; .byte	~0x01; .byte	~0x80
	.byte	~0xC0; .byte	~0xE0; .byte	~0xF0; .byte	~0x78
	.byte	~0x3C; .byte	~0x1E; .byte	~0x0F; .byte	~0x07
	.byte	~0x03; .byte	~0x01; .byte	~0x80; .byte	~0xC0
	.byte	~0xE0; .byte	~0xF0; .byte	~0xF8; .byte	~0x7C
	.byte	~0x3E; .byte	~0x1F; .byte	~0x0F; .byte	~0x07
	.byte	~0x03; .byte	~0x01; .byte	~0x80; .byte	~0xC0
	.byte	~0xE0; .byte	~0xF0; .byte	~0xF8; .byte	~0xFC
	.byte	~0x7E; .byte	~0x3F; .byte	~0x1F; .byte	~0x0F
	.byte	~0x07; .byte	~0x03; .byte	~0x01; .byte	~0x80
	.byte	~0xC0; .byte	~0xE0; .byte	~0xF0; .byte	~0xF8
	.byte	~0xFC; .byte	~0xFE; .byte	~0x7F; .byte	~0x3F
	.byte	~0x1F; .byte	~0x0F; .byte	~0x07; .byte	~0x03
	.byte	~0x01; .byte	~0x80; .byte	~0xC0; .byte	~0xE0
	.byte	~0xF0; .byte	~0xF8; .byte	~0xFC; .byte	~0xFE
	.byte	~0xFF; .byte	~0x7F; .byte	~0x3F; .byte	~0x1F
	.byte	~0x0F; .byte	~0x07; .byte	~0x03; .byte	~0x01
	.byte	~0xAA; .byte	~0x55; .byte	~0xAA; .byte	~0x55
endpat:
	.even
ledptr:		.long	_ledpat
flag5:		.word	0
flag7:		.word	0
_ledcnt:	.word	10		| 5 per second min LED update rate
ledcnt:		.word	0
	.text

/*
 * This code assumes that the real time clock interrupts 100 times
 * a second and that we want to only call hardclock 50 times/sec
 * We update the LEDs with new values so at least a user can tell
 * that something it still running before calling hardclock().
 */
	.data
.globl	_clk_intr, _clock_type
_clk_intr:	.long 0			| for counting clock interrupts
	.text

level5:					| default clock interrupt
	tstl	_clock_type		| check clock type, INTERSIL7170=0
	bne	0f			| if not INTERSIL7170, skip
	tstb	CLKADDR+CLK_INTRREG	| read CLKADDR->clk_intrreg to clear
0:
	andb	#~IR_ENA_CLK5,INTERREG	| clear interrupt request
	orb	#IR_ENA_CLK5,INTERREG	| and re-enable
	tstl	_clock_type		| check clock type, INTERSIL7170=0
	bne	0f			| if not INTERSIL7170, skip
	tstb	CLKADDR+CLK_INTRREG	| clear interrupt register again,
					| if we lost interrupt we will
					| resync later anyway.
0:
| for 100 hz operation, comment out from here ...
	notw	flag5			| toggle flag
	jeq	0f			| if result zero skip ahead
	rte
0:
| ... to here
	moveml	d0-d1/a0-a2,sp@-	| save regs we trash

	movl	sp,a2			| save copy of previous sp
	cmpl	#eintstack,sp		| on interrupt stack?
	jls	1f			| yes, skip
	lea	eintstack,sp		| no, switch to interrupt stack
1:

	addql	#1, _clk_intr		| count clock interrupt
			/* check for LED update */
	movl	a2@(5*4+2),a1		| get saved pc
	cmpl	#idle,a1		| were we idle?
	beq	0f			| yes, do LED update
	subqw	#1,ledcnt
	bge	2f			| if positive skip LED update
0:
	movw	_ledcnt,ledcnt		| reset counter
	movb	_ledpat,d0
	cmpb	#-1,d0
	bne	3f
	movb	d0,DIAGREG		| d0 to diagnostic LEDs
	bras	2f
3:
	movl	ledptr,a0		| get pointer
	movb	a0@+,d0			| get next byte
	cmpl	#endpat,a0		| are we at the end?
	bne	1f			| if not, skip
	lea	_ledpat,a0		| reset pointer
1:
	movl	a0,ledptr		| save pointer
	movb	d0,DIAGREG		| d0 to diagnostic LEDs

2:					| call hardclock
	movw	a2@(5*4),d0		| get saved sr
	movl	d0,sp@-			| push it as a long
	movl	a1,sp@-			| push saved pc
	jsr	_hardclock		| call UNIX routine
	movl	a2,sp			| restore old sp
	moveml	sp@+,d0-d1/a0-a2	| restore all saved regs
	jra	rei_io			| all done

/*
 * Level 7 interrupts can be caused by parity/ECC errors or the
 * clock chip.  The clock chip is tied to level 7 interrupts
 * only if we are profiling.  Because of the way nmi's work,
 * we clear any level 7 clock interrupts first before
 * checking the memory error register.
 */
_level7:
#ifdef GPROF
	tstl	_clock_type		| check clock type, INTERSIL7170=0
	bne	0f			| if not INTERSIL7170, skip
	tstb	CLKADDR+CLK_INTRREG	| read CLKADDR->clk_intrreg to clear
0:
	andb	#~IR_ENA_CLK7,INTERREG	| clear interrupt request
	orb	#IR_ENA_CLK7,INTERREG	| and re-enable
#endif GPROF

	moveml	d0-d1/a0-a1,sp@-	| save C scratch regs
	movb	MEMREG,d0		| read memory error register
	andb	#ER_INTR,d0		| a parity/ECC interrupt pending?
	jeq	0f			| if not, jmp
	jsr	_memerr			| dump memory error info
	/*MAYBE REACHED*/		| if we do return to here, then
	jra	1f			| we had a non-fatal memory problem
0:

#ifdef GPROF
| for 100 hz profiling, comment out from here ...
	notw	flag7			| toggle flag
	jne	1f			| if result non-zero return
| ... to here
	jsr	kprof			| do the profiling
#else GPROF
	pea	0f			| push message printf
	jsr	_printf			| print the message
	addqw	#4,sp			| pop argument
	.data
0:	.asciz	"stray level 7 interrupt\012"
	.even
	.text
#endif GPROF
1:
	moveml	sp@+,d0-d1/a0-a1	| restore saved regs
	rte

/*
 * Called by trap #2 to do an on chip cache flush operation
 */
	.globl	flush
flush:
	movl	#ICACHE_CLEAR+ICACHE_ENABLE+DCACHE_CLEAR+DCACHE_ENABLE,d0
	movc	d0,cacr			| clear (and enable) the cache
	rte

	.globl	syscall, trap, rei
	.globl	_qrunflag, _queueflag, _queuerun
#ifdef LWP
	.globl	_lwpschedule
	.globl	___Nrunnable
#endif LWP

/*
 * Special case for syscall.
 * Everything in line because this is by far the most
 * common interrupt.
 */
syscall:
	subqw	#2,sp			| empty space
	moveml	d0-d7/a0-a7,sp@-	| save all regs
	movl	usp,a0			| get usp
	movl	a0,sp@(R_SP)		| save usp
	movl	_uunix, a3
	movl	#syserr,a3@(U_LOFAULT)	| catch a fault if and when
	movl	a0@,d0			| get the syscall code
syscont:
	clrl	a3@(U_LOFAULT)		| clear lofault
	movl	d0,sp@-			| push syscall code
	jsr	_syscall		| go to C routine
	addqw	#4,sp			| pop arg
#ifdef LWP
	tstl	___Nrunnable		| any runnable threads?
	jeq	0f			| no
	jsr	_lwpschedule		| run threads
	movl	_masterprocp, sp@-
	jsr	_setmmucntxt
	addqw	#4, sp
0:
#endif LWP
	movl	_uunix, a3		| reload a3 because syscall can trash
	orw	#SR_INTPRI,sr		| need to test atomically, rte will lower
	tstb	_qrunflag		| need to run stream queues?
	beq	7f			| no
	tstb	_queueflag		| already running queues?
	bne	7f			| yes
	addqb	#1,_queueflag		| mark that we're running the queues
	jsr	_queuerun		| run the queues
	clrb	_queueflag		| done running queues
7:
	bclr	#AST_STEP_BIT-24,a3@(PCB_FLAGS) | need to single step?
	jne	4f
	bclr	#AST_SCHED_BIT-24,a3@(PCB_FLAGS) | need to reschedule?
	jeq	3f			| no, get out
4:	bset	#TRACE_AST_BIT-24,a3@(PCB_FLAGS) | say that we're tracing for AST
	jne	3f			| if already doing it, skip
	btst	#SR_TRACE_BIT-8,sp@(R_SR) | test trace mode
	jeq	5f			| if wasn't set, continue
	bset	#TRACE_USER_BIT-24,a3@(PCB_FLAGS) | save fact that trace was set
5:
	bset	#IR_SOFT_INT1_BIT,INTERREG | trigger level 1 intr
3:
	movl	a3@(U_BERR_PC),d0	| saved user PC on bus error
	beq	2f			| skip if none
	cmpl	sp@(R_PC),d0		| if equal, should restart user code
	bne	2f			| not equal, skip the following
	clrl	a3@(U_BERR_PC)		| clear u.u_berr_pc
					| sp points to d0 of kernel stack
	lea	sp@(R_KSTK),a0		| a0 points to bottom of kernel stack
					| (a short beyond <0000, voffset>)
	movl	a3@(U_BERR_STACK),a1	| a1 points to saved area
	movl	a1@+,d0			| d0 has the size to restore
	subl	d0,a0			| a0 points to top of new kernel stack
					| (d0), also (to) in dbra loop below
| load back upper 5 bits of u.u_pcb.pcb_flags to be consistent w/ saved SR
	movl	a3@(PCB_FLAGS),d1	| get current pcb_flags
	andl	#NOAST_FLAG,d1		| clear upper 5 bits
	orl	a1@+,d1			| oring saved upper 5 bits; a1=+4
	movl	d1,a3@(PCB_FLAGS)	| set pcb_flags

					| a1 points to saved d0
	movl	a0,sp			| sp points to top of new kernel stack
	asrl	#1,d0			| d0 gets (short count)
0:	movw	a1@+,a0@+		| move an short
	dbra	d0,0b			| decrement (short count) and loop

2:
	movl	sp@(R_SP),a0		| restore user SP
	movl	a0,usp
	movl	#ICACHE_CLEAR+ICACHE_ENABLE+DCACHE_CLEAR+DCACHE_ENABLE,d0
	movc	d0,cacr			| clear (and enable) the caches
1:
	moveml	sp@,d0-d7/a0-a6		| restore all but SP
	addw	#R_SR,sp		| pop all saved regs
	rte				| and return!

syserr:
	movl	#-1,d0			| set err code
	jra	syscont			| back to mainline

/*
 * We reset the sfc and dfc to FC_MAP in case we came in from
 * a trap while in the monitor since the monitor uses movs
 * instructions after dorking w/ sfc and dfc during its operation.
 */
trap:
	moveq	#FC_MAP,d0
	movc	d0,sfc	
	movc	d0,dfc
	jsr	_trap			| Enter C trap routine
	addqw	#4,sp			| Pop trap type

/*
 * Return from interrupt or trap, check for AST's.
 * d0 contains the size of info to pop (if any)
 */
rei:
	movl	d0, d2			| save d0 in safe place
	btst	#SR_SMODE_BIT-8,sp@(R_SR) | SR_SMODE ?
	bne	1f			| skip if system
#ifdef LWP
	tstl	___Nrunnable		| any runnable threads?
	jeq	0f			| no
	jsr	_lwpschedule		| run threads
	movl    _masterprocp, sp@-
	jsr	_setmmucntxt
	addqw	#4, sp
0:
#endif LWP
	orw	#SR_INTPRI,sr		| need to test atomically, rte will lower
	tstb	_qrunflag		| need to run stream queues?
	beq	7f			| no
	tstb	_queueflag		| already running queues?
	bne	7f			| yes
	addqb	#1,_queueflag		| mark that we're running the queues
	jsr	_queuerun		| run the queues
	clrb	_queueflag		| done running queues
7:
	movl	_uunix, a0
	bclr	#AST_STEP_BIT-24,a0@(PCB_FLAGS) | need to single step?
	jne	4f
	bclr	#AST_SCHED_BIT-24,a0@(PCB_FLAGS) | need to reschedule?
	jeq	3f			| no, get out
4:	bset	#TRACE_AST_BIT-24,a0@(PCB_FLAGS) | say that we're tracing for AST
	jne	3f			| if already doing it, skip
	btst	#SR_TRACE_BIT-8,sp@(R_SR) | test trace mode
	jeq	5f			| if wasn't set, continue
	bset	#TRACE_USER_BIT-24,a0@(PCB_FLAGS) | save fact that trace was set
5:
	bset	#IR_SOFT_INT1_BIT,INTERREG | trigger level 1 intr
3:	movl	sp@(R_SP),a0		| restore user SP
	movl	a0,usp
	movl	#ICACHE_CLEAR+ICACHE_ENABLE+DCACHE_CLEAR+DCACHE_ENABLE,a0
| ptrace, exec, etc. could overwrite instructions, so clear icache
	movc	a0,cacr			| clear (and enable) the caches
1:
	tstl	d2			| any cleanup needed?
	beq	2f			| no, skip
	movl	sp,a0			| get current sp
	addw	d2,a0			| pop off d2 bytes of crud
	tstw	sp@(R_VOR)		| see if there's a stack pad
	bne	3f			| if not, skip
	addqw	#2,a0			| pop off stack padding
3:
	clrw	a0@(R_VOR)		| dummy VOR
	movl	sp@(R_PC),a0@(R_PC)	| move PC
	movw	sp@(R_SR),a0@(R_SR)	| move SR
	movl	a0,sp@(R_SP)		| stash new sp value
	moveml	sp@,d0-d7/a0-a7		| restore all including SP
	addw	#R_SR,sp		| pop all saved regs
	rte				| and return!
2:
	tstw	sp@(R_VOR)		| see if there's a stack pad
	bne	4f			| if not, skip
	movl	sp@(R_PC),sp@(R_PC+2)	| put pc & sr back in real place
	movw	sp@(R_SR),sp@(R_SR+2)
	moveml	sp@,d0-d7/a0-a6		| restore all but SP
	addw	#(R_SR+2),sp		| pop all saved regs + stack pad
	rte				| and return!
4:
	moveml	sp@,d0-d7/a0-a6		| restore all but SP
	addw	#R_SR,sp		| pop all saved regs
	rte				| and return!

/*
 * Return from I/O interrupt, check for AST's.
 */
	.globl	rei_io
rei_iop:
	moveml	sp@+,d0-d2/a0-a2	| pop regs we saved in level5
rei_io:
	addql	#1,_cnt+V_INTR		| increment io interrupt count
rei_si:
	moveml	d0-d1/a0-a1,sp@-	| save C scratch regs
	btst	#SR_SMODE_BIT-8,sp@(16)	| SR_SMODE?  (SR is atop stack)
	jne	3f			| skip if system
	orw	#SR_INTPRI,sr		| need to test atomically, rte will lower
	tstb	_qrunflag		| need to run stream queues?
	beq	7f			| no
	tstb	_queueflag		| already running queues?
	bne	7f			| yes
	addqb	#1,_queueflag		| mark that we're running the queues
	jsr	_queuerun		| run the queues
	clrb	_queueflag		| done running queues
	orw	#SR_INTPRI,sr		| need to test atomically, rte will lower
7:
	movl	_uunix, a0
	bclr	#AST_STEP_BIT-24,a0@(PCB_FLAGS) | need to single step?
	jne	4f
	bclr	#AST_SCHED_BIT-24,a0@(PCB_FLAGS) | need to reschedule?
	jeq	3f			| no, get out
4:	bset	#TRACE_AST_BIT-24,a0@(PCB_FLAGS) | say that we're tracing for AST
	jne	3f			| if already doing it, skip
	bset	#SR_TRACE_BIT-8,sp@(16)	| set trace mode in SR atop stack
	jeq	5f			| if wasn't set, continue
	bset	#TRACE_USER_BIT-24,a0@(PCB_FLAGS) | save fact that trace was set
5:
	bset	#IR_SOFT_INT1_BIT,INTERREG | trigger level 1 intr
3:
	moveml	sp@+,d0-d1/a0-a1	| restore C scratch regs
	rte				| and return!

/*
 * Handle software interrupts that come in at level 1.  If it wasn't a
 * softint, then check any devices using level 1 autovector.
 * On the 68030, bad things happen if we set the trace bit (in rei, rei_si,
 * and syscall) in the trap frame before rte as was done on the sun-2/3.
 * Therefore we cause a software interrupt instead, and decide based on
 * the flags and the needsoftint flag whether we need to run the C softint
 * code or reschedule (or both).
 */
	.globl	level1

level1:
	bclr    #IR_SOFT_INT1_BIT,INTERREG| clear interrupt request 
	movl    d0,sp@-                 | save d0 
	jbsr    _spl7                   | disable interrupts to avoid race
	tstl    needsoftint             | a real software interrupt?
	jeq     2f 			| no soft interrupt
	clrl    needsoftint             | clear soft interrupt flag
	movl    d0,sp@-                 | save d0
	jbsr    _splx                   | restore priority
	addql   #4,sp                   | pop splx arg
	movl    sp@+,d0			| restore d0
	moveml  d0-d2/a0-a2,sp@-        | save regs we trash
	movl    sp,d2                   | save copy of previous sp
	cmpl    #eintstack,sp           | on interrupt stack?
	jls     0f                      | yes, skip
	lea     eintstack,sp            | no, switch to interrupt stack
0:
	jsr     _softint                | Call C
	tstl    d0              	| was there and soft interrupt
	beqs    1f			| no, look for devices
	movl    d2,sp                   | restore old sp
	jra     3f
2:
	movl    d0,sp@-                 |
	jbsr    _splx                   | restore priority
	addql   #4,sp                   | pop splx arg
	movl    sp@+,d0
	moveml	d0-d2/a0-a2,sp@- /* save regs we trash <d0,d1,d2,a0,a1,a2> */
	movl	sp,d2		/* save copy of previous sp */
	cmpl	#eintstack,sp	/* on interrupt stack? */
	jls	1f		/* yes, skip */
	lea	eintstack,sp	/* no, switch to interrupt stack */
1:	movl    #_level1_vector,a2  /* get vector ptr */
4:	movl    a2@+,a1         /* get routine address */
	jsr     a1@             /* go there */
	tstl    d0              /* success? */
	beqs    4b              /* no, try next one */
	/* get location of per-device interrupt counter */
	subl #_level1_vector + 4, a2
	addl #_level1_intcnt, a2 
	addql   #1, a2@         /* count the per-device interrupt */
	movl    d2,sp           /* restore stack pointer */
	cmpl    #0x80000000,d0  /* spurious? */
	jeq     3f              /* yes */
	clrl    _level1_spurious    /* no, clr spurious cnt */
	jra     rei_iop
3:
	movl	_uunix, a1
	btst    #TRACE_AST_BIT-24,a1@(PCB_FLAGS) | was this an AST?
	moveml  sp@+,d0-d2/a0-a2        | restore saved regs
	jeq     rei_si
	jra     trace

/*
 * Turn on a software interrupt (H/W level 1).
 */
	ENTRY(siron)
	movl	#1,needsoftint		 | this is a real software int
	bset	#IR_SOFT_INT1_BIT,INTERREG | trigger level 1 intr
	rts

/*
 * return 1 if an interrupt is being serviced (on interrupt stack),
 * otherwise return 0.
 */
	ENTRY(servicing_interrupt)
	clrl	d0			| assume false
	cmpl	#eintstack,sp		| on interrupt stack?
	bhi	1f			| no, skip
	movl	#1,d0			| return true
1:
	rts

/*
 * Enable and disable DVMA.
 */
	ENTRY(enable_dvma)
1:
	orw	#ENA_SDVMA,_enablereg	| enable System DVMA
	movw	_enablereg,d0		| get it in a register
	movw	d0,ENABLEREG		| put enable register back
	cmpw	_enablereg,d0		| see if someone higher changed it
	bne	1b			| if so, try again
	rts

	ENTRY(disable_dvma)
1:
	andw	#~ENA_SDVMA,_enablereg	| disable System DVMA
	movw	_enablereg,d0		| get it in a register
	movw	d0,ENABLEREG		| put enable register back
	cmpw	_enablereg,d0		| see if someone higher changed it
	bne	1b			| if so, try again
	rts

/*
 * Transfer data to and from user space -
 * Note that these routines can cause faults
 * It is assumed that the kernel has nothing at
 * less than USRSTACK in the virtual address space.
 */

| Fetch user byte		_fubyte(address)
	ENTRY2(fubyte,fuibyte)
	movl	_uunix, a1
	movl	#fsuerr,a1@(U_LOFAULT)	| catch a fault if and when
	movl	sp@(4),a0		| get address
	cmpl	#USRSTACK-1,a0		| check address range
	jhi	fsuerr			| jmp if greater than
	movb	a0@,d0			| get the byte
	andl	#0xFF,d0
	clrl	a1@(U_LOFAULT)		| clear lofault
	rts


| Fetch user (short) word:	 _fusword(address)
	ENTRY(fusword)
	movl	_uunix, a1
	movl	#fsuerr,a1@(U_LOFAULT)	| catch a fault if and when
	movl	sp@(4),a0		| get address
	cmpl	#USRSTACK-2,a0		| check address range
	jhi	fsuerr			| jmp if greater than
	movw	a0@,d0			| get the word
	andl	#0xFFFF,d0
	clrl	a1@(U_LOFAULT)		| clear lofault
	rts

| Fetch user (long) word:	_fuword(address)
	ENTRY2(fuword,fuiword)
	movl	_uunix, a1
	movl	#fsuerr,a1@(U_LOFAULT)	| catch a fault if and when
	movl	sp@(4),a0		| get address
	cmpl	#USRSTACK-4,a0		| check address range
	jhi	fsuerr			| jmp if greater than
	movl	a0@,d0			| get the long word
	clrl	a1@(U_LOFAULT)		| clear lofault
	rts

| Set user byte:		_subyte(address, value)
	ENTRY2(subyte,suibyte)
	movl	_uunix, a1
	movl	#fsuerr,a1@(U_LOFAULT)	| catch a fault if and when
	movl	sp@(4),a0		| get address
	cmpl	#USRSTACK-1,a0		| check address range
	jhi	fsuerr			| jmp if greater than
	movb	sp@(8+3),a0@		| set the byte
	clrl	d0			| indicate success
	clrl	a1@(U_LOFAULT)		| clear lofault
	rts

| Set user short word:		_susword(address, value)
	ENTRY(susword)
	movl	_uunix, a1
	movl	#fsuerr,a1@(U_LOFAULT)	| catch a fault if and when
	movl	sp@(4),a0		| get address
	cmpl	#USRSTACK-2,a0		| check address range
	jhi	fsuerr			| jmp if greater than
	movw	sp@(8+2),a0@		| set the word
	clrl	d0			| indicate success
	clrl	a1@(U_LOFAULT)		| clear lofault
	rts

| Set user (long) word:		_suword(address, value)
	ENTRY2(suword,suiword)
	movl	_uunix, a1
	movl	#fsuerr,a1@(U_LOFAULT)	| catch a fault if and when
	movl	sp@(4),a0		| get address
	cmpl	#USRSTACK-4,a0		| check address range
	jhi	fsuerr			| jmp if greater than
	movl	sp@(8+0),a0@		| set the long word
	clrl	d0			| indicate success
	clrl	a1@(U_LOFAULT)		| clear lofault
	rts

fsuerr:
	movl	#-1,d0			| return error
	movl	_uunix, a0
	clrl	a0@(U_LOFAULT)		| clear lofault
	rts

/*
 * Copy a null terminated string from the user address space into
 * the kernel address space.
 *
 * copyinstr(udaddr, kaddr, maxlength, lencopied)
 *	caddr_t udaddr, kaddr;
 *	u_int	maxlength, *lencopied;
 */
	ENTRY(copyinstr)
	movl	sp@(4),a0			| a0 = udaddr
	cmpl	#USRSTACK,a0
	jcc	copystrfault			| jmp if udaddr >= USRSTACK
	movl	sp@(8),a1			| a1 = kaddr
	movl	sp@(12),d0			| d0 = maxlength
	jlt	copystrfault			| if negative size fault
	movl	a2, sp@-
	movl	_uunix, a2
	movl	#copystrfault,a2@(U_LOFAULT)	| catch a fault if and when
	movl	sp@+, a2
	jra	1f				| enter loop at bottom
0:
	movb	a0@+,a1@+			| move a byte and set CC's
	jeq	copystrok			| if '\0' done
1:
	dbra	d0,0b				| decrement and loop

copystrout:
	movl	#ENAMETOOLONG,d0		| ran out of space
	jra	copystrexit

copystrfault:
	movl	#EFAULT,d0			| memory fault or bad size
	jra	copystrexit

copystrok:
	moveq	#0,d0				| all ok	

/*
 * At this point we have:
 *	d0 = return value
 *	a0 = final address of `from' string
 *	sp@(4) = starting address of `from' string
 *	sp@(16) = lencopied pointer (NULL if nothing to be return here)
 */
copystrexit:
	movl	a2, sp@-
	movl	_uunix, a2
	clrl	a2@(U_LOFAULT)			| clear lofault
	movl	sp@+, a2
	movl	sp@(16),d1			| d1 = lencopied
	jeq	2f				| skip if lencopied == NULL
	subl	sp@(4),a0			| compute nbytes moved
	movl	d1,a1
	movl	a0,a1@				| store into *lencopied
2:
	rts

/*
 * Copy a null terminated string from the kernel
 * address space to the user address space.
 *
 * copyoutstr(kaddr, udaddr, maxlength, lencopied)
 *	caddr_t kaddr, udaddr;
 *	u_int	maxlength, *lencopied;
 */
	ENTRY(copyoutstr)
	movl	sp@(4),a0			| a0 = kaddr
	movl	sp@(8),a1			| a1 = udaddr
	cmpl	#USRSTACK,a1
	jcc	copystrfault			| jmp if udaddr >= USRSTACK
	movl	sp@(12),d0			| d0 = maxlength
	jlt	copystrfault			| if negative size fault
	movl	a2, sp@-
	movl	_uunix, a2
	movl	#copystrfault,a2@(U_LOFAULT)	| catch a fault if and when
	movl	sp@+, a2
	jra	1f				| enter loop at bottom
0:
	movb	a0@+,a1@+			| move a byte and set CCs
	jeq	copystrok			| if '\0' done
1:
	dbra	d0,0b				| decrement and loop
	jra	copystrout			| ran out of space

/*
 * Copy a null terminated string from one point to another in
 * the kernel address space.
 *
 * copystr(kfaddr, kdaddr, maxlength, lencopied)
 *	caddr_t kfaddr, kdaddr;
 *	u_int	maxlength, *lencopied;
 */
	ENTRY(copystr)
	movl	sp@(4),a0			| a0 = kfaddr
	movl	sp@(8),a1			| a1 = kdaddr
	movl	sp@(12),d0			| d0 = maxlength
	jlt	copystrfault			| if negative size fault
	jra	1f				| enter loop at bottom
0:
	movb	a0@+,a1@+			| move a byte and set CCs
	jeq	copystrok			| if '\0' done
1:
	dbra	d0,0b				| decrement and loop
	jra	copystrout			| ran out of space

/*
 * copyout(kaddr, udaddr, n)
 *	caddr_t kaddr, udaddr;
 *	u_int n;
 *
 * We let kcopy do most of the work after verifying that udaddr is
 * really a user address so that we can possibly use the bcopy hardware.
 */
	ENTRY(copyout)
	cmpl	#USRSTACK,sp@(8)	| check starting address for udaddr
	jcc	cpctxerr		| jmp to cpctxerr on err (>= unsigned)
	jmp	_kcopy			| let kcopy do the rest

/*
 * copyin(udaddr, kaddr, n)
 * 	caddr_t udaddr, kaddr;
 *	u_int n;
 *
 * We let kcopy do most of the work after verifying that udaddr is
 * really a user address so that we can possibly use the bcopy hardware.
 */
	ENTRY(copyin)
	cmpl	#USRSTACK,sp@(4)	| check starting address for udaddr
	jcc	cpctxerr		| jmp to cpctxerr on err (>= unsigned)
	jmp	_kcopy			| let kcopy do the rest

cpctxerr:
	movl	#EFAULT,d0		| return error
	movl	_uunix, a0
	clrl	a0@(U_LOFAULT)		| clear lofault
	rts

/*
 * fetch user longwords  -- used by syscall -- faster than copyin
 * Doesn't worry about alignment of transfer, let the 68020 worry
 * about that - we won't be doing more than 8 long words anyways.
 * fulwds(uadd, sadd, nlwds)
 */
	ENTRY(fulwds)
	movl	_uunix, a0
	movl	#cpctxerr,a0@(U_LOFAULT)	| catch a fault if and when
	movl	sp@(4),a0		| user address
	movl	sp@(8),a1		| system address
	movl	sp@(12),d0		| number of words
	cmpl	#USRSTACK,a0		| check starting address
	jcs	1f			| enter loop at bottom if < unsigned
	jra	cpctxerr		| error
0:	movl	a0@+,a1@+		| get longword
1:	dbra	d0,0b			| loop on count
	clrl	d0			| indicate success
	movl	_uunix, a0
	clrl	a0@(U_LOFAULT)		| clear lofault
	rts

/*
 * Get/Set vector base register
 */
	ENTRY(getvbr)
	movc	vbr,d0
	rts

	ENTRY(setvbr)
	movl	sp@(4),d0
	movc	d0,vbr
	rts

/*
 * Enter the monitor -- called for console abort
 */
	ENTRY(montrap)
	jsr	_start_mon_clock	| enable monitor polling interrupt
	movl	sp@(4),a0		| address to trap to
	clrw	sp@-			| dummy VOR
	pea	0f			| return address
	movw	sr,sp@-			| current sr
	jra	a0@			| trap to monitor
0:
	jsr	_stop_mon_clock		| disable monitor polling interrupt
	rts

/*
 * Trap to halt unix and break to monitor
 */

	ENTRY(_halt)
	trap	#14
	rts

/*
 * Read the ID prom.  This is mapped from ID_PROM for IDPROMSIZE
 * bytes in the FC_MAP address space for byte access only.  Assumes
 * that the sfc has already been set to FC_MAP.
 */
	ENTRY(getidprom)
	movl	sp@(4),a0		| address to copy bytes to
	lea	ID_PROM,a1		| select id prom
	movl	#(IDPROMSIZE-1),d1	| byte loop counter
0:	movb	a1@+,a0@+		| get a byte
	dbra	d1,0b			| and loop
	rts

/*
 * Read the id information from the eeprom.
 */
	ENTRY(getideeprom)
	movl	sp@(4),a0		| address to copy bytes to
	lea	ID_EEPROM,a1		| select id part of eeprom
	movl	#(IDPROMSIZE-1),d1	| byte loop counter
0:	movb	a1@+,a0@+		| get a byte
	dbra	d1,0b			| and loop
	rts

/*
 * Enable and disable video.
 */
	ENTRY(setvideoenable)
1:	
	tstl	sp@(4)			| is bit on or off
	jeq	2f
	orw	#ENA_VIDEO,_enablereg	| enable video
	jra	3f
2:
	andw	#~ENA_VIDEO,_enablereg	| disable video
3:
	movw	_enablereg,d0		| get it in a register
	movw	d0,ENABLEREG		| put enable register back
	cmpw	_enablereg,d0		| see if someone higher changed it
	bne	1b			| if so, try again
	rts

/*
 * Enable and disable video interrupt.
 * Do not set P4_REG_RESET or cgeight will go black forever!
 */
	ENTRY(setintrenable)
	tstl	sp@(4)			| is bit on or off
	jeq	1f
	orb	#IR_ENA_VID4,INTERREG
	rts
1:	
        jbsr    _spl4                   | disable this level to avoid race
        andb    #~IR_ENA_VID4,INTERREG  | disable video interrupt
        movl    d0, sp@-                | push spl4 result for splx arg.
        jbsr    _splx                   | restore priority
        addql   #4, sp                  | pop splx arg
	rts

#ifdef FPU

/*
 * Set the fpp registers to the u area values
 */
	ENTRY(setfppregs)
	tstw	_fppstate		| is fpp present and enabled?
	jle	1f			| branch if not
	movl	_uunix, a0
	fmovem	a0@(U_FPS_REGS),fp0-fp7	| set fp data registers
	fmovem	a0@(U_FPS_CTRL),fpc/fps/fpi | set control registers
1:
	rts

/*
 * Save the internal FPP state at the address specified in the argument.
 */
	ENTRY(fsave)
	tstw	_fppstate		| is fpp present and enabled?
	jle	1f			| branch if not
	movl	sp@(4), a0		| a0 gets destination address
	fsave	a0@			| save internal state
1:
	rts

/*
 * fpiar() -- return the contents of the FPP fpiar register
 */
	ENTRY(fpiar)
	clrl	d0			| return 0 if fpp not present
	tstw	_fppstate		| is fpp present and enabled?
	jle	1f			| branch if not
	fmovel	fpiar,d0		| return FPIAR
1:
	rts

/*
 * Restore the internal FPP (68881/68882) state from a buffer.
 */
	ENTRY(frestore)
	tstw	_fppstate		| is fpp present and enabled?
	jle	1f			| branch if not
	movl sp@(4),a0			| source address
	frestore a0@			| restore internal state
1:
	rts

/*
 * Return the primitive returned by reading the coprocessor response CIR.
 * Useful on coprocessor protocol error to find cause of error.
 * If the protocol violation is detected by the FPCP due to an unexpected
 * access, the operation being executed previously is aborted and the FPCP
 * assumes the idle state when the exception acknowledge is received.
 * Thus the primitive read fromn th te response CIR is null (bit-15,CA=0).
 * However, if the protocol violation is detected by the MPU due to
 * illegal primitive, the FPCP response CIR contains that primitive.
 * Since the 68881/68882 aren't supposed to be able to return a illegal
 * primitive, this indicates a nasty hardware problem.
 */
	ENTRY(get_cir)
	tstw	_fppstate		| is fpp present and enabled?
	jle	1f			| branch if not
	movc	sfc, d1
	moveq	#FC_CPU,d0	| CPU space to talk to coprocessor
	movc	d0,sfc
	movsw	0x00022000,d0	| read response CIR
	movc	d1, sfc		| restore old sfc
1:
	rts

#endif FPU

/*
 * Enable bit in both Unix _enablereg and hard ENABLEREG.
 * on_enablereg(bit) turns on enable register by oring it and bit.
 * E.g. on_enablereg((u_char)ENA_FPA)
 */
	ENTRY(on_enablereg)
	movw	sp@(6),d0		| get word
	orw	d0,_enablereg		| turn on a bit
	movw	_enablereg,ENABLEREG    | put it in the hardware
	rts

/*
 * Disable bit in both Unix _enablereg and hard ENABLEREG. 
 * off_enablereg(bit) turns off enable register by anding it and bit.
 * E.g. off_enablereg((u_char)ENA_FPA)
 */ 
	ENTRY(off_enablereg) 
	movw	sp@(6),d0		| get word
	notw	d0			| ~bit
	andw	d0,_enablereg		| turn off a bit
	movw	_enablereg,ENABLEREG	| put it in the hardware
	rts

/*
 * if a() calls b() calls caller(), caller() returns return address in a().
 *
 * func_t caller()
 */
	ENTRY(caller)
	movl	a6@(4),d0
	rts

/*
 * if a() calls callee(), callee() returns the return address in a();
 *
 * func_t callee()
 */
	ENTRY(callee)
	movl	sp@,d0
	rts

/* STUFF WHICH ORIGINALLY APPEARED IN 4.1 */

/*
 * start_child(parentp, parentu, childp, nfp, new_context, seg)
 *      struct proc *parentp, *childp;
 *      struct user *parentu;
 *      int *new_context;
 *      struct file **nfp;
 *	struct seguser *seg;
 * This saves the parent's state such that when resume'd, the
 * parent will return to the caller of startchild.
 * It then calls conschild to create the child process
 *
 * We always resume the parent in the kernel.
 * The child usually resumes in userland.
 *
 * FPA exceptions not dealt with (not needed until this is a
 * generic context switching mechanism) (XXX)
 */
#define	SAVREGS		d2-d7/a2-a7
.globl _cons_child
.globl _masterprocp
	ENTRY(start_child)
	movl	_uunix, a0
|
| save 68881 floating point state of parent if used
|
	tstw	_fppstate		| is fpp present and enabled?
	jle	1f			| branch if not
	fsave	a0@(U_FP_ISTATE)	| save internal state
	tstb	a0@(U_FP_ISTATE)	| null state? (vers == FPIS_VERSNULL)
	jeq	1f			| branch if so
	fmovem	fpc/fps/fpi,a0@(U_FPS_CTRL) | save control registers
	fmovem	fp0-fp7,a0@(U_FPS_REGS)	| save fp data registers
| XXX - fpprocp not used until we get rid of frestores with null state
|	movl	_masterprocp,_fpprocp	| remember whose regs are still loaded
1:

|
| save parent's kernel regs in pcb so when parent is resumed,
| it will appear that he just returned from resume.
|
	movl	sp@,a0@(PCB_REGS)	| save return pc in pcb
	movw	sr,a0@(PCB_SR+2)	| save the current psw in u area
	moveml	SAVREGS,a0@(PCB_REGS+4)	| save data/address regs
	tstl	_masterprocp		| have a current process?
	jeq	1f			| skip if not
	movl	_masterprocp,a2		| does it
	movl	a2@(P_AS),d0		|   have an address space?
	jeq	1f			| skip if not
	movl	d0,sp@-			| find out memory claim for
	jsr	_rm_asrss		|   as associated with this process
	addqw	#4,sp			| clean up stack
	movl	d0,a2@(P_RSSIZE)	| store with process
	movl	_uunix,a0		| restore a0
1:

|
| save FPA state of parent if used
|
#if NFPA > 0
/*
 * fpa_save() is inlined here to moveml 12 regs in two instructions.
 * We use a2 to contain the fpa address so that it is preserved across
 * the call to fpa_shutdown and we pick out SAVEFPA regs so that a2
 * is not used and we have a total of 12 regs.
 */
#define	SAVEFPA		d0-d7/a3-a6
#define	FPA_MAXLOOP	512

	tstw	a0@(U_FPA_FLAGS)	| test if this process uses FPA
	jeq	1f			| FPA is not used skip saving FPA regs
| Begin of assembly fpa_save
	movl	#FPA_MAXLOOP-1,d1	| loop at most FPA_MAXLOOP times
	movl	_fpa,a2			| get fpa adddress
0:
	movl	a2@(FPA_PIPE_STATUS),d0	| get status
	andl	#FPA_STABLE,d0		| test for stability
	bne	2f			| != 0 means pipe is stable
	dbra	d1,0b			| busy, try again
	jsr	_fpa_shutdown		| timed out, shut it down
	jra	1f			| skip reg save
2:
	moveml	a2@(FPA_STATE),SAVEFPA	| load 12 FPA regs
	moveml	SAVEFPA,a0@(U_FPA_STATUS) | save to u.u_fpa_status
1:
#endif NFPA > 0

	movl	sp, a1
	movl	a1@(24), sp@-		| seg
	movl	a1@(20), sp@-		| new_context
	movl	a1@(16), sp@-		| nfp
	movl	a1@(12), sp@-		| childp
	movl	a1@(8), sp@-		| parentu
	movl	a1@(4), sp@-		| parentp
	jsr	_cons_child		| create child and switch u areas
					| and reset masterprocp
	/* NOTREACHED */

/*
 * yield_child(parent_ar0, child_stack, childp, parent_pid, childu, seg)
 * essentially the second half of resume.
 * Need to push state onto child's stack such that ksp
 * will be at the top of the child's stack when the rte happens.
 * MUST be called at splclock().
 */
	ENTRY(yield_child)
	movl	sp@(12), _masterprocp
	movl	_masterprocp, a1

/*
 * Check to see if we already have context.  If so, then set up
 * to use the context otherwise to default to the kernel context.
 */
	movl	a1@(P_AS),d0		| get p->p_as
	jeq	0f			| skip ahead if NULL (kernel process)
	movl	d0,a0
	movb	a0@(A_HAT_VLD),d0	|   p->p_as->a_hat.hat_valid
	jeq	0f			| skip ahead if NULL (no ctx allocated)

		| XXX - this stuff needs more work...
	movl	a0@(A_HAT_PFN),d0	| get level 1 pfn
	andl	#HAT_PFNMASK,d0		| mask off valid byte
	movl	#MMU_PAGESHIFT,d1	| put shift count in d1 (it's > 8)
	asll	d1,d0			| shift up to get u area addr
	bra	1f
0:
	moveq	#0,d0			| use kernel's "context"
1:
	addl	#PCB_L1PT,d0		| add offset to get level 1 page table
	movl	d0,rtptr+4		| fill software copy with address
	pmoveflush rtptr,crp		| load hardware register (flushes ATC)
		| XXX - end of this stuff needs more work...

	movl	_uunix, a0
	tstw	_fppstate		| is fpp present and enabled?
	jle	1f			| branch past fpp and fpa code if not
	tstb	a0@(U_FP_ISTATE)	| null state? (vers == FPIS_VERSNULL)
	jeq	0f			| branch if so
| XXX - fpprocp not used until we get rid of frestores with null state
|	cmpl	_fpprocp,a1		| check if we were last proc using fpp
|	beq	0f			| if so, jump and skip loading ext regs
	fmovem	a0@(U_FPS_REGS),fp0-fp7	| restore fp data registers
	fmovem	a0@(U_FPS_CTRL),fpc/fps/fpi | restore control registers
0:
	frestore a0@(U_FP_ISTATE)	| restore internal state

#if NFPA > 0
	tstw	a0@(U_FPA_FLAGS)	| test if this process uses FPA
	jeq	1f			| no, skip restoring FPA context
	movl	sp@(20), sp@-		| childu
	jsr	_fpa_restore		| yes, restore FPA context
	addqw	#4, sp
#endif NFPA > 0
1:

	movl	#ICACHE_CLEAR+ICACHE_ENABLE+DCACHE_CLEAR+DCACHE_ENABLE,d0
	movc	d0,cacr			| clear (and enable) the cache;
					| we're switching contexts.

	movl	sp@(8), a2		| child's stack top
	tstl	a2@(-4)			| kernel process?
	jne	userproc		| no

| kernel process. Align on even boundary since we never return
| to user land only to push an odd # of shorts on the stacks
| upon reentry into the kernel.
| Note that the argument needs to be swapped since we use 0-non-0 to
| tell if a kernel process or not and use the stack top
| for this info.
	movl    a2@(-8), a2@(-4)	| swap argument into correct place
	movl	#exit1, a2@(-8)		| fall thru to exit
	subl	#0x10, sp@(8)		| point to rte stuff (long aligned)
	jra	changectx

| user process. Align on odd boundary since reentry into the kernel
| pushes an odd number of shorts.
| See userstackset() for details. We also have to get the user's
| sp; we get this from the state saved at system call entry time
| in ar0-indexed registers.
userproc:
	movl	sp@(4), a0		| parent's frame in a0
	movl	a0@(R_SP), a1		| parent's usp
	addl	#4, a1			| pop syscall argument
	movl	a1, usp			| load usp
	subl	#0xa, sp@(8)		| point to rte stuff (short aligned)

changectx:
	moveml	a0@(8), d2-d7/a0-a6	| restore parent's regs except d0/d1/sp
	movl	sp@(16), d0		| rval1	(pid)
	movl	sp@(20), _uunix		| switch u area
	moveq	#1, d1			| rval2	(1)
	movl	sp@(8), sp		| install child's stack
	rte

.globl	_exit
exit1:
	movl	#0, sp@-		| argument to exit
	jsr	_exit			| exit(0)
	/* NOTREACHED */

/*
 * When a process is created by main to do init, it starts here.
 * We hack the stack to make it look like a system call frame.
 * Then, icode exec's init.  We "resume" init directly via rei.
 * Leave extra short on kernel stack so traps come in long aligned.
 */
.globl	_icode
.globl	_icode1
_icode1:
	pea	0x10000 		| non-zero pad + dummy fmt & vor
	pea	USRTEXT			| push pc
	movw	#SR_USER, sp@-		| push sr
	lea	0, a6			| stack frame link 0 in main
	SAVEALL()
	jsr	_icode
	clrl	d0			| fake return value from trap
	jra	rei

/*
 * finishexit(vaddr)
 * Done with the current stack;
 * switch to the exit stack, clear masterprocp, segu_release, and swtch.
 */
.globl	_segu_release
	ENTRY(finishexit)
	orw	#SR_INTPRI,sr			| lock out interrupts
	movl	sp@(4), a0			| get arg for segu_release
	cmpl	#eintstack, sp			| shouldn't be on interrupt
						| stack: will lose info if so
	jls	1f				| yes: too bad
	lea	eexitstack, sp			| switch to exit stack
	movl	#intu, _uunix			| need some u area -- e.g. swtch
	movl	#1, _noproc			| no process active now
	clrl	_masterprocp			| no more "current process"
	andw	#~SR_INTPRI,sr			| ok to take interrupts now
	movl	a0, sp@-			| arg to segu_release
	jsr	_segu_release			| free u area and stack
	addqw	#4,sp				| pop segu_release's arg
	jsr	_swtch				| context switch
	/* NOTREACHED */

1:	pea	2f
	jsr	_panic				| panic("finishexit");
	/* NOTREACHED */

	.data
2:	.asciz	"finishexit"
	.even
	.text

/*
 * setmmucntxt(parentp)
 */
	ENTRY(setmmucntxt)
	movl	sp@(4), a1		| new process
	movl	a1@(P_AS),d0		| get p->p_as
	jeq	0f			| skip ahead if NULL (kernel process)
	movl	d0,a0
	movl	a0@(A_HAT_VLD),d0	| get as->a_hat.hat_valid
	jeq	0f			| skip ahead if NULL (no ctx allocated)
		| XXX needs more work...
	movl	a0@(A_HAT_PFN),d0	| get level 1 pfn
	andl	#HAT_PFNMASK,d0		| mask off valid byte
	movl	#MMU_PAGESHIFT,d1	| put shift count in d1 (it's > 8)
	asll	d1,d0			| shift up to get u area addr
	bra	1f
0:
	moveq	#0,d0			| use kernel's "context"
1:
	addl	#PCB_L1PT,d0		| add offset to get level 1 page table
	movl	d0,rtptr+4		| fill software copy with address
	pmoveflush rtptr,crp		| load hardware register (flushes ATC)

	/*
	 * Now that the u areas are at different virtual addresses,
	 * we don't need to flush the on-chip data cache here because
	 * of any mappings changing in the kernel's address space.
	 * However, we need to flush this cache here in case there
	 * is anything from the user's address space in the cache.
	 */
	movl	#ICACHE_ENABLE+DCACHE_CLEAR+DCACHE_ENABLE,d0
	movc	d0,cacr			| clear the data cache

	rts

/* END OF STUFF WHICH ORIGINALLY APPEARED IN 4.1 */

/*
 * Flush MMU TLB entry.
 */
	ENTRY(atc_flush_entry)
	movl	sp@(4),a0		| get the address to flush
	pflush	#0,#0,a0@		| flush, ignoring function codes
	rts

/*
 * Flush MMU TLB.
 */
	ENTRY(atc_flush_all)
	pflusha				| flushes entire TLB
	rts

	.data
	.align 4			| space for root pointer construction
		.globl	rtptr
rtptr:	.long	0x7FFF0003		| no limit, points to long descriptor
	.long	0			| address filled in later
	.text

/*
 * Get MMU root pointer.
 */
	ENTRY(mmu_getrootptr)
	movl	rtptr+4,d0
	rts

/*
 * Set MMU root pointer.
 */
	ENTRY(mmu_setrootptr)
	movl	sp@(4),rtptr+4		| fill software copy with address
	pmoveflush rtptr,crp		| load hardware register
	rts

/*
 * Get the MMU status register from a fault.
 */
	ENTRY(mmu_geterror)
	movl	sp@(4),a0		| get virtual address
	movl	sp@(8),d0		| get function code
	tstl	sp@(0xc)		| test if cycle was a read
	bne	1f			| go to write case if not
	ptestr	d0,a0@,#3		| test the read case
	bra	2f
1:
	ptestw	d0,a0@,#3		| test the write case
2:
	pmove	psr,sp@(4)		| get status out of mmu
	movw	sp@(4),d0		| status is return value
	rts

/*
 * Flush the on chip data cache.
 */
	ENTRY(vac_flush)
	movl	#ICACHE_ENABLE+DCACHE_CLEAR+DCACHE_ENABLE,d0
	movc	d0,cacr			| clear the data cache
	rts

#ifdef PARITY
/*
 * Put parity error for testing parity recovery.
 */
	ENTRY(pperr)
	movl	sp@(4),a0
	movb	a0@,d0
	movb	d0,a0@
	orb     #0x20,0xfedf9000:l
	movb	d0,a0@
	andb    #0xdf,0xfedf9000:l
	rts
#endif PARITY

/*
 * Define some variables used by post-mortem debuggers
 * to help them work on kernels with changing structures.
 */
	.globl UPAGES_DEBUG, KERNELBASE_DEBUG, VADDR_MASK_DEBUG
	.globl PGSHIFT_DEBUG, SLOAD_DEBUG

UPAGES_DEBUG		= UPAGES
KERNELBASE_DEBUG	= KERNELBASE
VADDR_MASK_DEBUG	= 0xffffffff
PGSHIFT_DEBUG		= PGSHIFT
SLOAD_DEBUG		= SLOAD
