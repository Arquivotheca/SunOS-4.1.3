/*
 *	.seg	"data"
 *	.asciz	"@(#)misc.s 1.1 92/07/30"
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */
 	.seg	"text"
 	.align	4

#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/enable.h>
#include <machine/trap.h>

/*
 * Call a procedure though a pointer in a register.
 */
	RTENTRY(.ptr_call)
	jmp	%g1			! just jump to whatever's in %g1
	nop

/*
 * reset the vme bus via the enable register
 */
	ENTRY(reset_vme)
	set	ENABLEREG, %g1		! addr of enable register in ctl space
	lduba	[%g1]ASI_CTL,%g2	! read current value of enable register 
	bset	0x2, %g2
	stba	%g2, [%g1]ASI_CTL	! write out reset
	bclr	0x2, %g2
	retl
	stba	%g2, [%g1]ASI_CTL	! back to normal

/*
 * usec_delay(n)
 *	int n;
 * delay for n microseconds.  numbers <= 0 delay 1 usec
 *
 * the inner loop counts for current sun4s, initialized in startup():
 * inner loop is 2 cycles, the outer loop adds 3 more cycles.
 * Cpudelay*cycletime(ns)*2 + cycletime(ns)*3 >= 1000
 *
 * model	cycletime(ns)   Cpudelay
 * 110		66		7
 * 260		60		7
 * 60		50		9
 * 330		40		11
 * 470		30		16
 */
	ENTRY(usec_delay)
	sethi	%hi(_Cpudelay), %o5
	ld	[%o5 + %lo(_Cpudelay)], %o4	! microsecond countdown counter
	orcc	%o4, 0, %o3		! set cc bits to nz
	
1:	bnz	1b			! microsecond countdown loop
	subcc	%o3, 1, %o3		! 2 instructions in loop

	subcc	%o0, 1, %o0		! now, for each microsecond...
	bg	1b			! go n times through above loop
	mov	%o4, %o3
	retl
	nop
