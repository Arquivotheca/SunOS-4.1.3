	.data
	.asciz	"@(#)locore.s 1.1 92/07/30 SMI"
	.even
	.text

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/errno.h>
#include <sys/param.h>
#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/psl.h>

/*
 * The debug stack.  This must be the first thing in the data
 * segment (other than an sccs string) so that we don't stomp
 * on anything important.  We get a red zone below this stack
 * for free when the text is write protected.  Since the debugger
 * is loaded with the "-N" flag, we pad this stack by a page
 * because when the page level protection is done, we will lose
 * part of this stack.  Thus the usable stack will be at least
 * MIN_STACK_SZ bytes and at most MIN_STACK_SZ+NBPG bytes.
 */
#define	MIN_STACK_SZ	0x2000
	.data
	.globl _estack
	. = . + NBPG + MIN_STACK_SZ
_estack:				| end (top) of debugger stack
	.text


#define SAVEALL() \
	clrw	sp@-;\
	movl	sp,sp@-;\
	moveml	#0xfffe,sp@-;\
	movc	dfc,d1;\
	movc	sfc,d0;\
	moveml	#0xc000,sp@-

#define RESTOREALL() \
	moveml	sp@+,#0x0003;\
	movc	d0,sfc;\
	movc	d1,dfc;\
	moveml	sp@+,#0x7fff;\
	addqw	#6,sp

/*
 * Debugger receives control at the label `_start' which
 * must be at offset zero in this file; this file must
 * be the first thing in the boot image.  At start
 * is a struct dvec.
 */
	ENTRY(start)
	bra	0f			| dv_entry
	.long	_trap			| dv_trap
	.long	_pagesused		| dv_pages
	.long	_scbsync		| dv_scbsync
	.long	0

/*
 * First we must figure out if we are running in correctly adjusted
 * addresses.  If so, we must have been called via a jsr to _start
 * and thus we just want to call the command interpreter.  If not,
 * then we continue to run in the wrong adddress.  All of the code
 * must be carefully written to be position independent code, since
 * we are linked for running out of high addresses, but we get control
 * running in low addresses.  We run off the stack set up by the caller.
 */
#ifndef sun3x
#ifndef CTXSIZE
#define CTXSIZE		(NBSG * NSEGMAP)
#endif !CTXSIZE
#endif !sun3x

0:
	movl	a0,sp@-			| save a0
#ifndef sun3x
	movl	d0,sp@-			|  and d0
#endif sun3x
leax:	lea	pc@(_start-(leax+2)),a0	| a0 = true current location of _start
#ifndef sun3x
	movl	a0,d0			| get a copy that we can play with
	andl	#CTXSIZE-1,d0		| mask off unused virtual bits to
	movl	d0,a0			|   avoid believing them
	movl	sp@+,d0			| restore d0, we don't need it anymore
#endif sun3x
	cmpl	#_start,a0		| is desired value == current value?
	movl	sp@+,a0			| restore a0 - NOTE, cc not affected
	beq	docall			| if relocated already, go to docall()
					| first time through, still be careful
	lea	_startup,a1		| virtual startup address
	subl	#_start,a1		| subtract off virtual load address
	addl	a0,a1			| a1 has adjusted startup
	pea	a0@
	jsr	a1@			| CALL(startup)(real);
	addqw	#4,sp

	jmp	cont:l			| force non-PC rel branch
cont:

/*
 * PHEW!  Now we are running with correct addresses
 * and can use non-position independent code.
 */
	jsr	_main
	rts 				| should not return

/*
 * This is where we break into when we catch a breakpoint
 */
	ENTRY(trap)
	movw	#SR_HIGH,sr
	SAVEALL()
	jsr	_cmd

ret:
	RESTOREALL()
	rte


/*
 * This is where we break into when trace trap is taken
 */
	ENTRY(trace)
	tstl	_dotrace		| are we expecting this?
	jeq	ktraceret		| if not branch
	movw	#SR_HIGH,sr
	SAVEALL()
	jsr	_cmd
	tstl	d0			| test return value
	jeq	ret			| if 0, then simply return
	RESTOREALL()			| else restore regs and
					|     pass trace exception to debuggee

ktraceret:				| go to kernel's trace routine
	movl	_ktrace,sp@-		| push address to goto
	rts				| and get there as if nothing happened

/*
 * This is where we call to enter debugger (i.e. from
 * monitor command or an explicit call from kernel).
 */
	.data
pcsave:	.long 0
	.text
docall:
	movl	sp@+,pcsave		| copy return pc
	clrw	sp@-			| zero out fake fmt value
	movl	pcsave,sp@-		| push return pc
	movw	sr,sp@-
	movw	#SR_HIGH,sr
	SAVEALL()
	jsr	_cmd
	jra	ret

	ENTRY(fault)
	movw	#SR_HIGH,sr
	SAVEALL()
	/*
	 * Reset the sfc and dfc to FC_MAP to avoid problems if the
	 * monitor was dorking with these registers.  We don't have
	 * to worry about the current contents of sfc and dfc as
	 * they are restored to the original value before returning
	 * to the program being debugged.
	 */
	lea	FC_MAP,a0
	movc	a0,dfc
	movc	a0,sfc
	jsr	_faulterr		| call fault handler
	jsr	_cmd			| not resolved, go into debugger
	jra	ret			| try to return back now

	ENTRY(getvbr)
	movc	vbr,d0
	rts


/*
 * peekl(addr)
 *
 * peekl() and pokel() are like the standard peek (peekc) and
 * poke (pokec) functions except they use longwords and set
 * errno to EFAULT on failure (so that succeeding w/ -1 looks
 * different than a failure).
 */

	ENTRY(peekl)
	movl	sp@(4),a0	| Get address to probe
	movc	vbr,a1		| get vbr
	movl	a1@(8),d1	| save bus error handler
	movl	#BEhand,a1@(8)	| set up our own handler
	movl	sp,a1		| save current stack pointer
	movl	a0,d0
	btst	#0,d0		| See if odd address
	bne	BEhand		| Yes, the probe fails.
	movl	a0@,d0		| Read a longword.
PAexit:
	movc	vbr,a1		| get vbr
	movl	d1,a1@(8)	| restore bus error handler
	rts

BEhand:
	movl	a1,sp		| Restore stack after bus error
	moveq	#-1,d0		| Set result of -1, indicating fault.
	movl	#EFAULT,_errno	| Indicate failure using errno
	bra	PAexit

	ENTRY(pokel)
	movl	sp@(4),a0	| Get address to probe
	movc	vbr,a1		| get vbr
	movl	a1@(8),d1	| save bus error handler
	movl	#BEhand,a1@(8)	| set up our own handler
	movl	sp,a1		| save current stack pointer
	movl	sp@(8),a0@	| Write a longword
| A fault in the movl will vector us to BEhand above.
	moveq	#0,d0		| It worked; return 0 as result.
	bra	PAexit		| restores bus error handler and returns - above

/*
 * Peekc is so named to avoid a naming conflict
 * with adb which has a variable named peekc
 */
	ENTRY(Peekc)
	movl	sp@(4),a0	| Get address to probe
	movc	vbr,a1		| get vbr
	movl	a1@(8),d1	| save bus error handler
	movl	#BEhand,a1@(8)	| set up our own handler
	movl	sp,a1		| save current stack pointer
	moveq	#0,d0		| Clear upper half
	movb	a0@,d0		| Read a byte
| A fault in the movb will vector us to BEhand above.
	bra	PAexit		| restores bus error handler and returns - above

/*
 * Return current sp value to caller
 */
	ENTRY(getsp)
	movl	sp,d0
	rts

#if defined(sun3) || defined(sun3x)
/*
 * Flush '20 or '30 instruction cache
 */
	ENTRY(flush20)
	movc	cacr,d0			| get current cache control register
#ifdef sun3
	orl	#CACHE_CLEAR,d0		| or in clear flag
#endif sun3
#ifdef sun3x
	orl	#ICACHE_CLEAR,d0	| or in clear flag
#endif sun3x
	movc	d0,cacr			| clear cache (regardless of state)
	rts
#endif sun3 || sun3x

	ENTRY(_exitto)
	movl	sp@(4),a0		| Address to call.
	jsr	a0@			| Jump to callee using caller's stack.
	rts
