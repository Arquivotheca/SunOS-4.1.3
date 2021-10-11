	.data
	.asciz	"@(#)setjmp.s 1.1 92/07/30 SMI"
	.even
	.text

|	Copyright 1984-1989 Sun Microsystems, Inc.

#include <machine/asm_linkage.h>
#include "assym.s"

| Longjmp and setjmp implement non-local gotos
| using state vectors of type label_t (13 longs).
| Registers saved are the PC, d2-d7, and a2-a7.

SAVREGS = 0xFCFC

	ENTRY(setjmp)
	movl	sp@(4),a1		| get label_t address
sj:	moveml	#SAVREGS,a1@(4)		| save data/address regs
	movl	sp@,a1@			| save PC of caller
	clrl	d0			| return zero
	rts

	ENTRY(longjmp)
	movl	sp@(4),a1		| get label_t address
lj:	moveml	a1@(4),#SAVREGS		| restore data/address regs
| Note: we just changed stacks
	movl	a1@,sp@			| restore PC
	movl	#1,d0			| return one
	rts

| syscall_setjmp just saves a6 and sp
| it is called from syscall, where speed is important

	ENTRY(syscall_setjmp)
	movl	sp@(4),a1		| get label_t address
	movl	a6,a1@(4+(10*4))	| save a6
	movl	sp,a1@(4+(11*4))	| save sp
	movl	sp@,a1@			| save PC of caller
	clrl	d0			| return zero
	rts

/*
 * on_fault()
 * Catch lofault faults.  Like setjmp except it returns 1 if code following
 * causes uncorrectable fault.  Turned off by calling no_fault().
 */

	.bss
	.align 4
lfault:	.skip 13 * 4			| special label_t for on_fault
	.text

        ENTRY(on_fault)
	movl	_uunix, a0		| put catch_fault in u.u_lofault
	movl	#catch_fault, a0@(U_LOFAULT)
	movl	#lfault, a1		| use special label_t
	bra	sj			| let setjmp do the rest

catch_fault:
	movl	_uunix, a0		| turn off lofault
	clrl	a0@(U_LOFAULT)
	movl	#lfault, a1		| use special label_t
	bra	lj			| let longjmp do the rest

/*
 * no_fault()
 * Turn off fault catching.
 */
        ENTRY(no_fault)
	movl	_uunix, a0
	clrl	a0@(U_LOFAULT)
	rts
