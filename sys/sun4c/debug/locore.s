/*	@(#)locore.s 1.1 92/07/30	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "assym.s"
#include <sys/errno.h>
#include <sys/param.h>
#include <machine/asm_linkage.h>
#include <machine/reg.h>
#include <machine/mmu.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/enable.h>
#include <machine/cpu.h>
#include <machine/trap.h>
#include <machine/eeprom.h>
#include "../../debug/debug.h"

/*
 * The debug stack. This must be the first thing in the data
 * segment (other than an sccs string) so that we don't stomp
 * on anything important. We get a red zone below this stack
 * for free when the text is write protected.
 */
#define	STACK_SIZE	0x8000
	.seg	"data"
	.global _estack
	.skip	STACK_SIZE
_estack:				! end (top) of debugger stack
fpuregs:
	.skip	33*4			! %f0 - %f31 plus %fsr

/*
 * The number of windows, set by fiximp when machine is booted.
 */
	.global _nwindows
/*
 * The parent callback routine.
 */
	.global _release_parent
_release_parent:
	.word	0

	.seg	"text"

/*
 * Trap vector macros.
 */
#define TRAP(H) \
	b (H); mov %psr,%l0; nop; nop;

/*
 * The constant in the last expression must be (nwindows-1).
 * See fiximp() below.
 */
#define WIN_TRAP(H) \
	mov %psr,%l0;  mov %wim,%l3;  b (H);  mov 7,%l6;

#define SYS_TRAP(T) \
	mov %psr,%l0;  sethi %hi(T),%l3;  b sys_trap;  or %l3,%lo(T),%l3;

#define BAD_TRAP	SYS_TRAP(_fault);

/*
 * Trap vector table.
 * This must be the first text in the boot image.
 *
 * When a trap is taken, we vector to DEBUGSTART+(TT*16) and we have
 * the following state:
 *	2) traps are disabled
 *	3) the previous state of PSR_S is in PSR_PS
 *	4) the CWP has been decremented into the trap window
 *	5) the previous pc and npc is in %l1 and %l2 respectively.
 *
 * Registers:
 *	%l0 - %psr immediately after trap
 *	%l1 - trapped pc
 *	%l2 - trapped npc
 *	%l3 - trap handler pointer (sys_trap only)
 *		or current %wim (win_trap only)
 *	%l6 - NW-1, for wim calculations (win_trap only)
 *
 * Note: DEBUGGER receives control at vector 0 (trap).
 */
	.seg	"text"
	.align 4

	.global _our_die_routine
	.global _start, _scb
_start:
_scb:
	TRAP(enter);				! 00
	BAD_TRAP;				! 01 text fault
	BAD_TRAP;				! 02 unimp instruction
	BAD_TRAP;				! 03 priv instruction
	TRAP(_fp_disabled);			! 04 fp disabled
	WIN_TRAP(_window_overflow);		! 05
	WIN_TRAP(_window_underflow);		! 06
	BAD_TRAP;				! 07 alignment
	BAD_TRAP;				! 08 fp exception
	SYS_TRAP(_fault);			! 09 data fault
	BAD_TRAP;				! 0A tag_overflow
	BAD_TRAP; BAD_TRAP;			! 0B - 0C
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 0D - 10
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 11 - 14 int 1-4
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 15 - 18 int 5-8
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 19 - 1C int 9-12
	BAD_TRAP; BAD_TRAP;			! 1D - 1E int 13-14
	SYS_TRAP(_level15);			! 1F int 15
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 20 - 23
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 24 - 27
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 28 - 2B
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 2C - 2F
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 30 - 34
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 34 - 37
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 38 - 3B
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 3C - 3F
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 40 - 44
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 44 - 47
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 48 - 4B
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 4C - 4F
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 50 - 53
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 54 - 57
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 58 - 5B
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 5C - 5F
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 60 - 64
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 64 - 67
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 68 - 6B
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 6C - 6F
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 70 - 74
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 74 - 77
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;	! 78 - 7B
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 7C - 7F
	!
	! software traps
	!
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 80 - 83
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 84 - 87
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 88 - 8B
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 8C - 8F
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 90 - 93
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 94 - 97
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 98 - 9B
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! 9C - 9F
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! A0 - A3
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! A4 - A7
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! A8 - AB
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! AC - AF
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! B0 - B3
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! B4 - B7
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! B8 - BB
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! BC - BF
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! C0 - C3
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! C4 - C7
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP; ! C8 - CB
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
	BAD_TRAP; BAD_TRAP;			! FC - FD
	TRAP(_trap);				! FE enter debugger
	TRAP(_trap);				! FF breakpoint

/*
 * Debugger vector table.
 * Must follow trap table.
 */
dvec:
	b,a	enter			! dv_entry
	.word	_trap			! dv_trap
	.word	_pagesused		! dv_pages
	.word	_scbsync		! dv_scbsync
	.word	0

/*
 * Debugger entry point.
 * First we must figure out if we are running in correctly adjusted
 * addresses. If so, we must have been called by debugged code
 * and thus we just want to call the command interpreter. If not,
 * then we continue to run in the wrong adddress. All of the code
 * must be carefully written to be position independent, since
 * we are linked for running out of high addresses, but we get control
 * running in low addresses. We depend on the boot stack for work space.
 */

enter:
	mov	%o0, %o1		! o0 might be romp: pass it as o1 later
	mov	%o2, %l1		! o2 might be the parent callback routine
	mov	%o7, %o5		! save o7
acall:	call	lea			! %o7 = physical(acall)
	sethi	%hi(acall), %o3		! %o3 = virtual(acall)
lea:	or	%o3, %lo(acall), %o3	!
	cmp	%o3, %o7		! compare to see if virtual == physical
	set	_start, %o2		! %o2 = virtual(_start)
	sub	%o3, %o2, %o4		! %o4 = acall - _start
	sub	%o7, %o4, %o0		! %o0 = physical(start)
	bne	init			! if != then not relocated yet
	mov	%o5, %o7		! restore o7
	!
	! Enter debugger by doing a software trap.
	! We ASSUME that the software trap we have set up is still there.
	!
	t	TRAPBRKNO-1
	nop
	retl
	nop

	!
	! We have not been relocated yet.
	!
init:
	mov	%psr, %g1
	bclr	PSR_PIL, %g1		! PIL = 15
	bset	(15 << 8), %g1
	mov	%g1, %psr
	nop; nop; nop
	!
	! Preserve the romp across the call to early_startup().
	! Save it in %l0: since we're going to reset the world later,
	! we can trash our caller's registers.
	!
	mov	%o1, %l0
	set	_early_startup, %o3	! get offset of early_startup()
	sub	%o3, %o2, %o4		!  from _start
	add	%o4, %o0, %o4		! add to _start's current address
	jmpl	%o4, %o7		! call early_startup(real)
	nop
	set     jmpstart, %g1		! now that it is relocated, jump to it
	jmp     %g1
	nop

jmpstart:
	!
	! PHEW! Now we are running with correct addresses
	! and can use non-position-independent code.
	!
	! Save romp.
	!
	set	_romp, %o0
	st	%l0, [%o0]
	!
	! Save parent callback routine
	!
	set	_release_parent, %o0
	st	%l1, [%o0]
	!
	! Save monitor's level14 clock interrupt vector code and trap #0.
	!
	mov     %tbr, %l4               ! save monitor's tbr
	bclr    0xfff, %l4              ! remove tt
	or      %l4, TT(T_INT_LEVEL_14), %l4
	set     _scb, %l5
	or      %l5, TT(T_INT_LEVEL_14), %l5
	ldd	[%l4], %o0
	std	%o0, [%l5]
	ldd	[%l4+8], %o0
	std	%o0, [%l5+8]
	bclr    0xfff, %l4              ! remove tt
	or      %l4, TT(T_SYSCALL), %l4
	bclr    0xfff, %l5
	or      %l5, TT(T_SYSCALL), %l5
	ldd	[%l4], %o0
	std	%o0, [%l5]
	ldd	[%l4+8], %o0
	std	%o0, [%l5+8]

	!
	! Take over the world. Save the current stack pointer, as
	! we're going to need it for a little while longer.
	!
	mov	%sp, %g2
	set	_scb, %g1		! setup kadb tbr
	mov 	%g1, %tbr
	nop; nop; nop
	mov	0x2, %wim
	mov 	%psr, %g1
	bclr 	PSR_CWP, %g1
	mov 	%g1, %psr
	nop; nop; nop;			! psr delay
	sub	%g2, SA(MINFRAME), %sp

	!
	! Fix the implementation dependent parameters.
	!
fiximp:
	!
	! Find out how many windows we have and set the global variable.
	! We must be in window 0 for this to work.
	!
	set	_nwindows, %g2
	save				! CWP is now (nwindows - 1)
	mov	%psr, %g1
	and	%g1, PSR_CWP, %g1
	inc	%g1
	st	%g1, [%g2]
	restore
	!
	! Fix the number of windows in the trap vectors.
	! The last byte of the window handler trap vectors must be equal to
	! the number of windows in the implementation minus one.
	!
	dec	%g1			! last byte of trap vectors get NW-1
	set	_scb, %g2
	add	%g2, (5 << 4), %g2	! window overflow trap handler offset
	stb	%g1, [%g2 + 15]		! write last byte of trap vector
	add	%g2, 16, %g2		! now get window underflow trap
	stb	%g1, [%g2 + 15]		! write last byte of trap vector
	!
	! Call startup to do the rest of the startup work.
	!
	call	_startup
	nop
	!
	! call main to enter the debugger
	!
	call	_main
	nop
	mov	%psr, %g1
	bclr	PSR_PIL, %g1		! PIL = 14
	bset	(14 << 8), %g1
	mov	%g1, %psr
	nop;nop;nop
	t	TRAPBRKNO-1
	!
	! In the unlikely event we get here, return to the monitor.
	!
	set	_romp, %g1
	ld	[%g1], %g2
	ld	[%g2 + EXIT_TO_MON], %g1
	jmp	%g1
	clr	%o0

/*
 * exitto(addr)
 * int *addr;
 */
	ENTRY(_exitto)
	save	%sp, -SA(MINFRAME), %sp
        set     1, %o1
        sethi   %hi(_use_kern_tbr), %o0
        st      %o1, [%o0 + %lo(_use_kern_tbr)]

	set	_romp, %g1
	ld	[%g1], %g1
	ld	[%g1 + V_ROMVEC_VERSION], %g1	! check romp->v_romvec_version
	tst	%g1
	be	dont_kill_parent	! only callback if version > 0
	nop
	set	_release_parent, %g1	! call back boot's release function
	ld	[%g1], %g1
	tst	%g1
	be	dont_kill_parent	! only callback if function exists
	nop
	jmpl	%g1, %o7		! pass return addr in %o0 for
	add	%o7, 8, %o0		! romp->op_chain() function
dont_kill_parent:
	set	_our_die_routine, %o2	! pass our release func to callee
	set	dvec, %o1		! pass dvec address to callee
	set	_romp, %o0		! pass the romp to callee
	jmpl	%i0, %o7		! register-indirect call
	ld	[%o0], %o0
	ret
	restore

/*
 * This is where breakpoint traps go.
 * We assume we are in the normal condition after a trap.
 */
	ENTRY(trap)
	!
	! dump the whole cpu state (all windows) on the stack.
	!
	set	_regsave, %l3
	st	%l0, [%l3 + R_PSR]
	st	%l1, [%l3 + R_PC]
	st	%l2, [%l3 + R_NPC]
	mov	%wim, %l4
	st	%l4, [%l3 + R_WIM]
	mov	%g0, %wim		! zero wim so that we can move around
	mov	%tbr, %l4
	st	%l4, [%l3 + R_TBR]
	mov	%y, %l4
	st	%l4, [%l3 + R_Y]
	st	%g1, [%l3 + R_G1]
	st	%g2, [%l3 + R_G2]
	st	%g3, [%l3 + R_G3]
	st	%g4, [%l3 + R_G4]
	st	%g5, [%l3 + R_G5]
	st	%g6, [%l3 + R_G6]
	st	%g7, [%l3 + R_G7]
	add	%l3, R_WINDOW, %g7
	sethi	%hi(_nwindows), %g6
	ld	[%g6 + %lo(_nwindows)], %g6
	bclr	PSR_CWP, %l0		! go to window 0
	mov	%l0, %psr
	nop; nop; nop;			! psr delay
1:
	st	%l0, [%g7 + 0*4]	! save locals
	st	%l1, [%g7 + 1*4]
	st	%l2, [%g7 + 2*4]
	st	%l3, [%g7 + 3*4]
	st	%l4, [%g7 + 4*4]
	st	%l5, [%g7 + 5*4]
	st	%l6, [%g7 + 6*4]
	st	%l7, [%g7 + 7*4]
	st	%i0, [%g7 + 8*4]	! save ins
	st	%i1, [%g7 + 9*4]
	st	%i2, [%g7 + 10*4]
	st	%i3, [%g7 + 11*4]
	st	%i4, [%g7 + 12*4]
	st	%i5, [%g7 + 13*4]
	st	%i6, [%g7 + 14*4]
	st	%i7, [%g7 + 15*4]
	add	%g7, WINDOWSIZE, %g7
	subcc	%g6, 1, %g6		! all windows done?
	bnz	1b
	restore				! delay slot, increment CWP
	!
	! Back in window 0.
	!
	set	_regsave, %g2		! need to get back to state of entering
	ld	[%g2 + R_WIM], %g1
	mov	%g1, %wim
	ld	[%g2 + R_PSR], %g1
	bclr	PSR_PIL, %g1		! PIL = 14
	bset	(14 << 8), %g1
	mov     %g1, %psr		! go back to orig window
	nop;nop;nop
	!
	! Now we must make sure all the window stuff goes to memory.
	! Flush all register windows to the stack.
	! But wait! If we trapped into the invalid window, we can't just
	! save because we'll slip under the %wim tripwire.
	! Do one restore first, so subsequent saves are guaranteed
	! to flush the registers. (Don't worry about the restore
	! triggering a window underflow, since if we're in a trap
	! window the next window up has to be OK.)
	! Do all this while still using the kernel %tbr: let it worry
	! about user windows and user stack faults.
	!
	restore
	mov	%psr, %g1		! get new CWP
	wr	%g1, PSR_ET, %psr	! enable traps
	nop;nop;nop
	save	%sp, -SA(MINFRAME), %sp	! now we're back in the trap window
	mov	%sp, %g3
	mov	%fp, %g4
	sethi	%hi(_nwindows), %g7
	ld	[%g7 + %lo(_nwindows)], %g7
	sub	%g7, 2, %g6		! %g6 = NWINDOW - 2
1:
	deccc	%g6			! all windows done?
	bnz	1b
	save	%sp, -WINDOWSIZE, %sp
	sub	%g7, 2, %g6		! %g6 = NWINDOW - 2
2:
	deccc	%g6			! all windows done?
	bnz	2b
	restore				! delay slot, increment CWP

	mov	%psr, %g1
	wr	%g1, PSR_ET, %psr	! disable traps while working
	mov	2, %wim			! setup wim
	bclr	PSR_CWP, %g1		! go to window 0
	bclr	PSR_PIL, %g1		! PIL = 14
	bset	(14 << 8), %g1
	wr	%g1, PSR_ET, %psr	! rewrite %psr, but keep traps disabled
	nop; nop; nop
	mov	%g3, %sp		! put back %sp and %fp
	mov	%g4, %fp
	mov	%g1, %psr		! now enable traps
	nop; nop; nop

	! save fpu
	sethi	%hi(_fpu_exists), %g1
	ld	[%g1 + %lo(_fpu_exists)], %g1
	tst	%g1			! don't bother if no fpu
	bz	1f
	nop
	set	_regsave, %l3		! was it on?
	ld	[%l3 + R_PSR], %l0
	set	PSR_EF, %l5		! FPU enable bit
	btst	%l5, %l0		! was the FPU enabled?
	bz	1f
	nop
	!
	! Save floating point registers and status.
	! All floating point operations must be complete.
	! Storing the fsr first will accomplish this.
	!
	set	fpuregs, %g7
	st	%fsr, [%g7+(32*4)]
	std	%f0, [%g7+(0*4)]
	std	%f2, [%g7+(2*4)]
	std	%f4, [%g7+(4*4)]
	std	%f6, [%g7+(6*4)]
	std	%f8, [%g7+(8*4)]
	std	%f10, [%g7+(10*4)]
	std	%f12, [%g7+(12*4)]
	std	%f14, [%g7+(14*4)]
	std	%f16, [%g7+(16*4)]
	std	%f18, [%g7+(18*4)]
	std	%f20, [%g7+(20*4)]
	std	%f22, [%g7+(22*4)]
	std	%f24, [%g7+(24*4)]
	std	%f26, [%g7+(26*4)]
	std	%f28, [%g7+(28*4)]
	std	%f30, [%g7+(30*4)]
1:
	set	_scb, %g1		! setup kadb tbr
	mov	%g1, %tbr
	nop				! tbr delay
	call	_cmd
	nop				! tbr delay

	mov	%psr, %g1		! disable traps, goto window 0
	bclr	PSR_CWP, %g1
	mov	%g1, %psr
	nop; nop; nop;			! psr delay
	wr	%g1, PSR_ET, %psr
	nop; nop; nop;			! psr delay
	mov	%g0, %wim		! zero wim so that we can move around
	!
	! Restore fpu
	!
	! If there is not an fpu, we are emulating and the
	! registers are already in the right place, the u area.
	! If we have not modified the u area there is no problem.
	! If we have it is not the debuggers problem.
	!
	sethi	%hi(_fpu_exists), %g1
	ld	[%g1 + %lo(_fpu_exists)], %g1
	tst	%g1			! don't bother if no fpu
	bz	2f			! its already in the u area
	nop
	set	_regsave, %l3		! was it on?
	ld	[%l3 + R_PSR], %l0
	set	PSR_EF, %l5		! FPU enable bit
	btst	%l5, %l0		! was the FPU enabled?
	bz	2f
	nop
	set	fpuregs, %g7
	ldd	[%g7+(0*4)], %f0	! restore registers
	ldd	[%g7+(2*4)], %f2
	ldd	[%g7+(4*4)], %f4
	ldd	[%g7+(6*4)], %f6
	ldd	[%g7+(8*4)], %f8
	ldd	[%g7+(10*4)], %f10
	ldd	[%g7+(12*4)], %f12
	ldd	[%g7+(14*4)], %f14
	ldd	[%g7+(16*4)], %f16
	ldd	[%g7+(18*4)], %f18
	ldd	[%g7+(20*4)], %f20
	ldd	[%g7+(22*4)], %f22
	ldd	[%g7+(24*4)], %f24
	ldd	[%g7+(26*4)], %f26
	ldd	[%g7+(28*4)], %f28
	ldd	[%g7+(30*4)], %f30
	ld	[%g7+(32*4)], %fsr	! restore fsr
2:
	set	_regsave, %g7
	add	%g7, R_WINDOW, %g7
	sethi	%hi(_nwindows), %g6
	ld	[%g6 + %lo(_nwindows)], %g6
1:
	ld	[%g7 + 0*4], %l0	! restore locals
	ld	[%g7 + 1*4], %l1
	ld	[%g7 + 2*4], %l2
	ld	[%g7 + 3*4], %l3
	ld	[%g7 + 4*4], %l4
	ld	[%g7 + 5*4], %l5
	ld	[%g7 + 6*4], %l6
	ld	[%g7 + 7*4], %l7
	ld	[%g7 + 8*4], %i0	! restore ins
	ld	[%g7 + 9*4], %i1
	ld	[%g7 + 10*4], %i2
	ld	[%g7 + 11*4], %i3
	ld	[%g7 + 12*4], %i4
	ld	[%g7 + 13*4], %i5
	ld	[%g7 + 14*4], %i6
	ld	[%g7 + 15*4], %i7
	add	%g7, WINDOWSIZE, %g7
	subcc	%g6, 1, %g6		! all windows done?
	bnz	1b
	restore				! delay slot, increment CWP
	!
	! Should be back in window 0.
	!
	set	_regsave, %g7
	ld	[%g7 + R_PC], %l1	! restore pc
	ld	[%g7 + R_NPC], %l2	! restore npc
	ld	[%g7 + R_WIM], %g1
	mov	%g1, %wim
	ld	[%g7 + R_TBR], %g1
	srl	%g1, 4, %g1		! realign ...
	and	%g1, 0xff, %g1		!... and mask to get trap number
	set	(TRAPBRKNO | 0x80), %g2
	cmp	%g1, %g2		! compare to see this is a breakpoint
	bne	debugtrap		! a debugger trap return PC+1
	nop
	!
	! return from breakpoint trap
	!
	ld	[%g7 + R_PSR], %g1	! restore psr
	mov	%g1, %psr
	nop; nop; nop;			! psr delay
	ld	[%g7 + R_TBR], %g1
	mov	%g1, %tbr
	ld	[%g7 + R_Y], %g1
	mov	%g1, %y
	mov	%g7, %l3		! put state ptr in local
	ld	[%l3 + R_G1], %g1	! restore globals
	ld	[%l3 + R_G2], %g2
	ld	[%l3 + R_G3], %g3
	ld	[%l3 + R_G4], %g4
	ld	[%l3 + R_G5], %g5
	ld	[%l3 + R_G6], %g6
	ld	[%l3 + R_G7], %g7
	nop
	jmp	%l1			! return from trap
	rett	%l2
	!
	! return from debugger trap
	!
debugtrap:
	ld	[%g7 + R_PSR], %g1	! restore psr
	mov	%g1, %psr
	nop; nop; nop;			! psr delay
	ld	[%g7 + R_TBR], %g1
	mov	%g1, %tbr
	ld	[%g7 + R_Y], %g1
	mov	%g1, %y
	mov	%g7, %l3		! put state ptr in local
	ld	[%l3 + R_G1], %g1	! restore globals
	ld	[%l3 + R_G2], %g2
	ld	[%l3 + R_G3], %g3
	ld	[%l3 + R_G4], %g4
	ld	[%l3 + R_G5], %g5
	ld	[%l3 + R_G6], %g6
	ld	[%l3 + R_G7], %g7
	jmp	%l2			! return from trap
	rett	%l2 + 4

/*
 * tcode handler for scbsync()
 */
	.align  4
	.global _tcode
_tcode:
	sethi	%hi(_trap), %l3
	or	%l3, %lo(_trap), %l3
	jmp	%l3
	mov	%psr, %l0

LPSR = 0*4
LPC = 1*4
LNPC = 2*4
LSP = 3*4
LG1 = 4*4
LG2 = 5*4
LG3 = 6*4
LG4 = 7*4
LG5 = 8*4
LG6 = 9*4
LG7 = 10*4
REGSIZE = 11*4
/*
 * General debugger trap handler.
 * This is only used by traps that happen while the debugger is running.
 * It is not used for any debuggee traps.
 * Does overflow checking then vectors to trap handler.
 */
sys_trap:
	!
	! Prepare to go to C (batten down the hatches).
	! Save volatile regs.
	!
	sub	%fp, SA(MINFRAME+REGSIZE), %l7 ! make room for reg save area
	st	%g1, [%l7 + MINFRAME + LG1]
	st	%g2, [%l7 + MINFRAME + LG2]
	st	%g3, [%l7 + MINFRAME + LG3]
	st	%g4, [%l7 + MINFRAME + LG4]
	st	%g5, [%l7 + MINFRAME + LG5]
	st	%g6, [%l7 + MINFRAME + LG6]
	st	%g7, [%l7 + MINFRAME + LG7]
	st	%fp, [%l7 + MINFRAME + LSP]
	st	%l0, [%l7 + MINFRAME + LPSR]
	st	%l1, [%l7 + MINFRAME + LPC]
	st	%l2, [%l7 + MINFRAME + LNPC]
	!
	! Check for window overflow.
	!
	mov	0x01, %l5		! CWM = 0x01 << CWP
	sll	%l5, %l0, %l5
	mov	%wim, %l4		! get WIM
	btst	%l5, %l4		! compare WIM and CWM
	bz	st_have_window
	nop
	!
	! The next window is not empty. Save it.
	!
	sethi	%hi(_nwindows), %l6
	ld	[%l6 + %lo(_nwindows)], %l6
	dec	%l6			
	srl	%l4, 1, %g1		! WIM = %g1 = ror(WIM, 1, NWINDOW)
	sll	%l4, %l6, %l4		! %l6 = NWINDOW - 1 
	or	%l4, %g1, %g1
	save				! get into window to be saved
	mov	%g1, %wim		! install new WIM
	SAVE_WINDOW(%sp)
	restore				! get back to original window
	!
	! The next window is available.
	!
st_have_window:
	mov	%l7, %sp		! install new sp
	mov	%tbr, %o0		! get trap number
	bclr	PSR_PIL, %l0		! PIL = 15
	bset	(15 << 8), %l0
	wr	%l0, PSR_ET, %psr	! enable traps
	srl	%o0, 4, %o0		! psr delay
	and	%o0, 0xff, %o0
	ld	[%l7 + MINFRAME + LPC], %o1
	ld	[%l7 + MINFRAME + LNPC], %o2
	mov	%l3, %g1
	jmpl	%g1, %o7		! call handler
	nop
	!
	! Return from trap.
	!
	ld	[%l7 + MINFRAME + LPSR], %l0 ! get saved psr
	bclr	PSR_PIL, %l0		! PIL = 14
	bset	(14 << 8), %l0
	mov	%psr, %g1		! use current CWP
	mov	%g1,%g3
	and	%g1, PSR_CWP, %g1
	andn	%l0, PSR_CWP, %l0
	or	%l0, %g1, %l0
	mov	%l0, %g1
	mov	%l0, %psr		! install old psr, disable traps
	nop;nop;nop
	!
	! Make sure that there is a window to return to.
	!
	mov	0x2, %g1		! compute mask for CWP + 1
	sll	%g1, %l0, %g1
	sethi	%hi(_nwindows), %g3
	ld	[%g3 + %lo(_nwindows)], %g3
	srl	%g1, %g3, %g2
	or	%g1, %g2, %g1
	mov	%wim, %g2		! cmp with wim to check for underflow
	btst	%g1, %g2
	bz	sr_out
	nop
	!
	! No window to return to. Restore it.
	!
	sll	%g2, 1, %g1		! compute new WIM = rol(WIM, 1, NWINDOW)
	dec	%g3			! %g3 is now NWINDOW-1
	srl	%g2, %g3, %g2
	or	%g1, %g2, %g1
	mov	%g1, %wim		! install it
	nop; nop; nop;			! wim delay
	restore				! get into window to be restored
	RESTORE_WINDOW(%sp)
	save				! get back to original window
	!
	! There is a window to return to.
	! Restore the volatile regs and return.
	!
sr_out:
	mov	%l0, %psr		! install old PSR_CC
	ld	[%l7 + MINFRAME + LG1], %g1
	ld	[%l7 + MINFRAME + LG2], %g2
	ld	[%l7 + MINFRAME + LG3], %g3
	ld	[%l7 + MINFRAME + LG4], %g4
	ld	[%l7 + MINFRAME + LG5], %g5
	ld	[%l7 + MINFRAME + LG6], %g6
	ld	[%l7 + MINFRAME + LG7], %g7
	ld	[%l7 + MINFRAME + LSP], %fp
	ld	[%l7 + MINFRAME + LPC], %l1
	ld	[%l7 + MINFRAME + LNPC], %l2
	jmp	%l1
	rett	%l2
	.empty

/*
 * Trap handlers.
 */

/*
 * Window overflow trap handler.
 * On entry, %l3 = %wim, %l6 = nwindows-1
 */
	ENTRY(window_overflow)
	!
	! Compute new WIM.
	!
	mov	%g1, %l7		! save %g1
	srl	%l3, 1, %g1		! next WIM = %g1 = ror(WIM, 1, NWINDOW)
	sll	%l3, %l6, %l4		! %l6 = NWINDOW-1
	or	%l4, %g1, %g1
	save				! get into window to be saved
	mov	%g1, %wim		! install new wim
	nop; nop; nop;			! wim delay
	!
	! Put it on the stack.
	!
	SAVE_WINDOW(%sp)
	restore				! go back to trap window
	mov	%l7, %g1		! restore g1
	jmp	%l1			! reexecute save
	rett	%l2

/*
 * Window underflow trap handler.
 * On entry, %l3 = %wim, %l6 = nwindows-1
 */
	ENTRY(window_underflow)
	sll	%l3, 1, %l4		! next WIM = rol(WIM, 1, NWINDOW)
	srl	%l3, %l6, %l5		! %l6 = NWINDOW-1
	or	%l5, %l4, %l5
	mov	%l5, %wim		! install it
	nop; nop; nop;			! wim delay
	restore				! (wim delay 3) get into last window
	restore				! get into window to be restored
	RESTORE_WINDOW(%sp)
	save				! get back to original window
	save
	jmp	%l1			! reexecute restore
	rett	%l2

/*
 * Misc subroutines.
 */

/*
 * Get trap base register.
 *
 * char *
 * gettbr()
 */
	ENTRY(gettbr)
	mov	%tbr, %o0
	srl	%o0, 12, %o0
	retl
	sll	%o0, 12, %o0

/*
 * Set trap base register.
 *
 * void
 * settbr(t)
 *	struct scb *t;
 */
	ENTRY(settbr)
	srl	%o0, 12, %o0
	sll	%o0, 12, %o0
	retl
	mov	%o0, %tbr

/*
 * Return current sp value to caller
 *
 * char *
 * getsp()
 */
	ENTRY(getsp)
	retl
	mov	%sp, %o0

/*
 * Get system enable register
 *
 * char
 * getenablereg()
 */
	ENTRY(getenablereg)
	set	ENABLEREG, %o0
	lduba	[%o0]ASI_CTL, %o0
	retl
	nop

/*
 * Set priority level hi.
 *
 * splhi()
 */
	ENTRY(splhi)
	mov	%psr, %o0
	or	%o0, PSR_PIL, %g1
	mov	%g1, %psr
	nop				! psr delay
	retl
	nop

/*
 * Set priority level 13.
 *
 * spl13()
 */
	ENTRY(spl13)
	mov     %psr, %o0
	bclr    PSR_PIL, %o0            ! PIL = 13
	mov     %g0, %g1
	bset    (13 << 8), %g1
	or      %g1, %o0, %g1
	mov     %g1, %psr
	nop                             ! psr delay
	retl
	nop 

	.seg	"data"
	.align	4
	.global	_fpu_exists
_fpu_exists:
	.word	1			! assume FPU exists

	.seg	"text"
	.align	4
/*
 * FPU probe - try a floating point instruction to see if there
 * really is an FPU in the current configuration.
 */
	ENTRY(fpu_probe)
	mov	%psr, %g1		! read psr
	set	PSR_EF, %g2		! enable floating-point
	bset	%g2, %g1
	mov	%g1, %psr		! write psr
	nop				! psr delay, three cycles
	sethi	%hi(zero), %g2		! wait till the fpu bit is fixed
	or	%g2, %lo(zero), %g2
	ld	[%g2], %fsr		! This causes less trouble with SAS.
	retl				! if no FPU, we get fp_disabled trap
	nop				! which will clear the fpu_exists flag

zero:	.word	0

/*
 * floating point disabled trap.
 * if FPU does not exist, emulate instruction
 * otherwise, enable floating point
 */
	.global _fp_disabled
_fp_disabled:
	!
	! if we get an fp_disabled trap and the FPU is enabled
	! then there is not an FPU in the configuration
	!
	set	PSR_EF, %l5		! FPU enable bit
	btst	%l5, %l0		! was the FPU enabled?
	sethi	%hi(_fpu_exists), %l6
	bz,a	1f			! fp was disabled, fix up state
	ld	[%l6 + %lo(_fpu_exists)], %l5	! else clear fpu_exists
	!
	! fp_disable trap when the FPU is enabled; should only happen
	! once from autoconf when there is not an FPU in the board
	!
	clr	[%l6 + %lo(_fpu_exists)] ! FPU does not exist in configuration
	set 	PSR_EF, %l4
	bclr	%l4, %l0
	mov	%l0, %psr
	nop;nop;nop
1:
	jmp	%l2			! return from trap skip inst
	rett	%l2 + 4


	ENTRY(flush_windows)
	sethi	%hi(_nwindows), %g7
	ld	[%g7 + %lo(_nwindows)], %g7
	sub	%g7, 2, %g6
1:
	deccc	%g6			! all windows done?
	bnz	1b
	save	%sp, -WINDOWSIZE, %sp
	sub	%g7, 2, %g6
2:
	deccc	%g6			! all windows done?
	bnz	2b
	restore				! delay slot, increment CWP
	retl
	nop

	/*
	 * asm_trap(x)
	 * int x;
	 *
	 * Do a "t x" instruction
	 */
	ENTRY(asm_trap)
	retl
	t	%o0
