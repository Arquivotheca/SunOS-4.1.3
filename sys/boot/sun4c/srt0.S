/*
 * srt0.s - standalone startup code
 *	.seg	"data"
 *	.asciz	"@(#)srt0.S 92/07/30 1.1 SMI"
 *	Copyright (c) 1986-1990 Sun Microsystems, Inc.
 */
 	.seg	"text"
 	.align	4

#include <machine/asm_linkage.h>
#include <machine/param.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/mmu.h>
#include <mon/idprom.h>

/* 
 * param.h defines VAC, DELAY, and CDELAY
 * if we are compiling for the bootblock undef VAC
 * to save on code size
 */

#ifdef BOOTBLOCK
#undef VAC
#endif

/*
 * Initial interrupt priority and how to get there.
 */
#define PSR_PIL_SHIFT	8
#define PSR_PIL_INIT	(13 << PSR_PIL_SHIFT)

/*
 * The following variables are machine-dependent and are set in fiximp.
 */
	.seg	"data"
	.global	_Cpudelay, _nwindows, _npmgrps
	.global	_vac, _vac_size, _vac_linesize, _segmask
/*
 * The following variables are more or less machine-independent
 * (or are set outside of fiximp).
 */
	.global _romp
_romp:
	.word	0
	.global	_cpudelay		! old macros:
_cpudelay:				! shift right amount to scale (n << 4)
	.word	0			! worst case value (25 Mhz)
idp:
	.word	0			! room for idprom format & mach type
	.word	0
	.global	_environ
_environ:
	.word	0			! satisfy references from libc
exitbuf:
	.skip	5*4			! (jump_buf)exitbuf for _exit routine

	.seg	"text"
	.align	8
	.global _our_die_routine
	.global	_end
	.global	_edata
	.global	__setjmp
	.global	__longjmp
	.global	_main
	.global	__exit
	.global	__exitto
	.global	_start
_start:

#ifdef BOOTBLOCK
	.global _begin, _bootb, _bbsize, _bbchecksum
	b,a	_begin
_bootb:
! should be NBLKENTRIES	(64), see installboot.c
	.skip	64*4			! room for boot_tab
_bbsize:	.word 0
_bbchecksum:	.word 0
_begin:
#endif BOOTBLOCK

! Each standalone program is responsible for its own stack. Our strategy
! is that each program which uses this runtime code creates a stack just
! below its relocation address. Previous windows may (and probably do)
! have frames allocated on the prior stack; leave them alone. Starting with
! this window, allocate our own stack frames for our windows. (Overflows
! or a window flush would then pass seamlessly from our stack to the old.)
! RESTRICTION: A program running at some relocation address must not exec
! another which will run at the very same address: the stacks would collide.
! This is currently not a problem, as all standalone programs (boot, copy,
! kadb, unix, et al.) run at different, carefully chosen addresses...which
! is, of course, a crock.
!
! Careful: don't touch %o0 until the save, since it holds the romp
! for Forth PROMs.
!
	set	_start, %o1
	save	%o1, -SA(MINFRAME), %sp	
acall:
	call	1f			! get the current pc into o7
					! (where _start is currently located)
	sethi	%hi(acall), %o3		! relocated address of call
1:
	sub	%o7,acall-_start,%o0	! address where _start is in memory
	or	%o3, %lo(acall), %o3	! relocated address of call
	cmp	%o3, %o7
	be	jmpstart
	.empty				! next instruction ok in delay slot
	set	_edata+4, %o2		! end of program, inclusive, except bss
	set	_start, %o1		! beginning of program
	sub	%o2, %o1, %o2		! size of program
2:
	ldd	[%o0], %o4		! relocate program
	inc	8, %o0			! bcopy
	std	%o4, [%o1]
	deccc	8, %o2
	bg	2b
	inc	8, %o1
	
	set	jmpstart, %l0		! now that it is relocated, jump to it
	jmp	%l0
	nop

jmpstart:
	set	_romp, %o0		! Stash the romp we've been saving
	st	%i0, [%o0] 
	/*
	 * Set the machine-dependent variables.
	 * Most of the work gets done in fiximp(), but we need to 
	 * set nwindows here as there's no property for it. (XXX)
	 *
	 * Play fast and loose with the CWP. Do everything here with
	 * interrupts disabled, and don't write in any local registers.
	 * Since we're dorking with the psr anyway, set up the
	 * interrupt levels and enable traps while we're at it.
	 * (Then we can do nifty things like call C functions.)
	 */
	mov	%psr, %o0
	/*
	 * Set the psr into a known state:
	 * supervisor mode, interrupt level >= 13, traps enabled
	 */
	andn	%o0, PSR_PIL, %g1
	set	PSR_S|PSR_PIL_INIT|PSR_ET, %o1
	or	%g1, %o1, %g1		! new psr is ready to go
	/*
	 * Temporarily go to window 0 to ferret out nwindows.
	 */
	andn	%o0, PSR_CWP, %g2	! CWP = 0
	mov	%wim, %g3
	mov	%g0, %wim
	mov	%g2, %psr		! we're in window 0 now
	nop; nop; nop
	save				! decrement cwp, wraparound to NW-1
	mov	%psr, %g2
	mov	%g1, %psr		! get back...
	mov	%g3, %wim		! ...and put back wim
	and	%g2, PSR_CWP, %g2	! mask off NW-1
	inc	%g2			! %g2 = nwindows
	sethi	%hi(_nwindows), %g1
	st	%g2, [%g1 + %lo(_nwindows)]
	set	_edata, %o0		! beginning of bss
	set	_end+4, %o1		! end of bss
	call	_bzero			! zero the bss
	sub	%o1, %o0, %o1		! size of bss
	call	_fiximp
	nop
#ifdef VAC
	call	init_cache
	nop
#endif
	call	_l14enable		! turn on level 14 interrupts
	nop
!
! Mark this spot.
!
	sethi	%hi(exitbuf), %o0
	call	__setjmp
	or	%o0, %lo(exitbuf), %o0
	tst	%o0			! returned 1: we exit now.
	bnz	out
	nop
	call	_main
	mov	0, %o0			! delay slot
! Fall through...
!
! __exit means exit NOW. We may be arbitrarily deep in the windows, so longjmp.
!

#ifndef VAC

__exit:
	set	exitbuf, %o0
	call	__longjmp
	mov	1, %o1
out:
	ret				! ret to prom
	restore

__exitto:
	save	%sp, -SA(MINFRAME), %sp
	set	_romp, %o0		! pass the romp to the callee
	set	_our_die_routine, %o2	! pass our release func to callee
	clr	%o1			! Clear DVEC
	jmpl	%i0, %o7		! call thru register
	ld	[%o0], %o0
	ret
	restore

#else !VAC

__exit:
	save	%sp, -SA(MINFRAME), %sp
	set	_vac, %o0		! check vac flag
	ld	[%o0], %o0		
	tst	%o0
	bz	1f
	call	_turnoff_cache
	nop
1:
	set	exitbuf, %o0
	call	__longjmp
	mov	1, %o1
out:
	ret
	restore

__exitto:
	save	%sp, -SA(MINFRAME), %sp
	!
	! turn off the cache if it's currently on
	!
	set	_vac, %o0		! check vac flag
	ld	[%o0], %o0		
	tst	%o0
	bz	no_cache
	nop
	call	_turnoff_cache
	nop
no_cache:
	set	_romp, %o0		! pass the romp to the callee
	set	_our_die_routine, %o2	! pass our release func to callee
	jmpl	%i0, %o7		! call thru register
	ld	[%o0], %o0
	set	_vac, %o0		! check vac flag
	ld	[%o0], %o0		
	tst	%o0
	bz	leave_cache_off
	nop
	call	_turnon_cache
	nop
leave_cache_off:
	ret
	restore

init_cache:
	save	%sp, -SA(MINFRAME), %sp
	set	_vac, %o0		! check vac flag
	ld	[%o0], %o0		
	tst	%o0
	bz	1f
	.empty
	set	_start, %o0		! this doesn't get the stack
	sethi	%hi(_end), %o1
	call	_cache_prog		! clear the NC bits for prog's pages
	or	%o1, %lo(_end), %o1
	call	_turnon_cache
	nop
1:
	ret
	restore

#endif VAC
