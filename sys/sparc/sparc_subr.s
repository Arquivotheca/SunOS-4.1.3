/*	@(#)sparc_subr.s 1.1 92/07/30 SMI	*/

/*
 *	Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * General assembly language routines.
 * It is the intent of this file to contain routines that are
 * independent of the specific kernel architecture, and those that are
 * common across kernel architectures.
 * As architectures diverge, and implementations of specific
 * architecture-dependent routines change, the routines should be moved
 * from this file into the respective ../`arch -k`/subr.s file.
 * For example, this file used to contain getidprom(), but the sun4c
 * getidprom() diverged from the sun4 getidprom() so getidprom() was
 * moved.
 */

#include <machine/param.h>
#include <machine/asm_linkage.h>
#include <machine/psl.h>
#include <machine/mmu.h>
#include <machine/intreg.h>
#include <machine/enable.h>
#include "assym.s"

	.seg	"text"
	.align	4

/*
 * Macro to raise processor priority level.
 * Avoid dropping processor priority if already at high level.
 * Note, these macros return the priority field
 * as it appears in place in the psr
 */
#define	RAISE(level) \
	mov	%psr, %o0; \
	and	%o0, PSR_PIL, %g1; \
	cmp	%g1, (level << PSR_PIL_BIT); \
	bl,a	1f; \
	andn	%o0, PSR_PIL, %g1; \
	retl; \
	nop; \
1:	or	%g1, (level << PSR_PIL_BIT), %g1; \
	mov	%g1, %psr; \
	nop; \
	retl; \
	nop;

#define SETPRI(level) \
	mov	%psr, %o0; \
	andn	%o0, PSR_PIL, %g1; \
	or	%g1, (level << PSR_PIL_BIT), %g1; \
	mov	%g1, %psr; \
	nop; \
	retl; \
	nop;

	/*
	 * Berkley 4.3 introduced symbolically named interrupt levels
	 * as a way deal with priority in a machine independent fashion.
	 * Numbered priorities are machine specific, and should be
	 * discouraged where possible.
	 *
	 * Note, for the machine specific priorities there are
	 * examples listed for devices that use a particular priority.
	 * It should not be construed that all devices of that
	 * type should be at that priority.  It is currently were
	 * the current devices fit into the priority scheme based
	 * upon time criticalness.
	 *
	 * The underlying assumption of these assignments is that
	 * SPARC IPL 10 is the highest level from which a device
	 * routine can call wakeup.  Devices that interrupt from higher
	 * levels are restricted in what they can do.  If they need
	 * kernels services they should schedule a routine at a lower
	 * level (via software interrupt) to do the required
	 * processing.
	 *
	 * Examples of this higher usage:
	 *	Level	Usage
	 *	15	Asynchronous memory exceptions (Non-maskable)
	 *	14	Profiling clock (and PROM uart polling clock)
	 *	13	Audio device (on sun4c)
	 *	12	Serial ports
	 *	11	Floppy controller (on sun4c)
	 *
	 * The serial ports request lower level processing on level 6.
	 * Audio and floppy request lower level processing on level 4.
	 *
	 * Also, almost all splN routines (where N is a number or a
	 * mnemonic) will do a RAISE(), on the assumption that they are
	 * never used to lower our priority.
	 * The exceptions are:
	 *	spl8()		Because you can't be above 15 to begin with!
	 *	splzs()		Because this is used at boot time to lower our
	 *			priority, to allow the PROM to poll the uart.
	 *	spl1()		Used by queuerun() to lower priority
	 *	spl0()		Used to lower priority to 0.
	 *	splsoftclock()	Used by hardclock to lower priority.
	 */

	/* locks out all interrupts, including memory errors */
	ENTRY(spl8)
	SETPRI(15)

	/* just below the level that profiling runs */
#if defined(sun4c) || defined(sun4m)
	ALTENTRY(splaudio)	/* only meaningful on 4c, 4m */
#endif	/* sun4c || sun4m */
	ENTRY(spl7)
	RAISE(13)

	/* sun specific - highest priority onboard serial i/o zs ports */
	ALTENTRY(splzs)	
	SETPRI(12)	/* Can't be a RAISE, as it's used to lower us */


	/*
	 * should lock out clocks and all interrupts,
	 * as you can see, there are exceptions
	 */
	ALTENTRY(splhigh)

	/* the standard clock interrupt priority */
	ALTENTRY(splclock)

	/* highest priority for any tty handling */
	ALTENTRY(spltty)

	/* highest priority required for protection of buffered io system */
	ALTENTRY(splbio)

	/* machine specific */ 
	ENTRY2(spl6,spl5)
	RAISE(10)

	/*
	 * machine specific 
	 * for sun, some frame buffers must be at this priority
	 */
	ENTRY(spl4)
	RAISE(8)

	/* highest level that any network device will use */
	/* must be lower than highest interrupting ether device */
	ENTRY(splimp)
	RAISE(7)

	/*
	 * machine specific 
	 * for sun, devices with limited buffering: tapes, ethernet
	 */
	ENTRY(spl3)
	RAISE(6)

	/*
	 * machine specific - not as time critical as above
	 * for sun, disks
	 */
	ENTRY(spl2)
	RAISE(4)

	ENTRY(spl1)
	SETPRI(2)

	/* highest level that any protocol handler will run */
	ENTRY(splnet)
	RAISE(1)

	/* softcall priority */
	/* used by hardclock to LOWER priority */
	ENTRY(splsoftclock)
	SETPRI(1)

	/* allow all interrupts */
	ENTRY(spl0)
	SETPRI(0)

/*
 * splvm will raise the processor priority to the value implied by splvm_val
 * which is initialized by the autoconfiguration code to a sane default.
 */
	ENTRY(splvm)
	sethi	%hi(_splvm_val), %o0
	ld	[%o0 + %lo(_splvm_val)], %o0

	! fall through to splr
/*
 * splr(psr_pri_field)
 * splr is like splx but will only raise the priority and never drop it
 */
	ENTRY(splr)
	mov	%psr, %o1;		! get current psr value
	and	%o1, PSR_PIL, %o3	! interlock slot, mask out rest of psr
	and	%o0, PSR_PIL, %o0	! only look at priority, paranoid
	cmp	%o3, %o0		! if current pri < new pri, set new pri
	bl	pri_common		! use common code to set priority
	andn	%o1, PSR_PIL, %o2	! zero current priority level
	retl				! return old priority
	mov	%o3, %o0

/*
 * splx(psr_pri_field)
 * set the priority to given value
 * Note, this takes a sun4 priority number and 'ors' it into the psr
 * A spl() returns this type of cookie so that a splx() does the right thing
 */
	ENTRY(splx)
pri_set_common:
	mov	%psr, %o1		! get current priority level
	andn	%o1, PSR_PIL, %o2	! zero current priority level
	and	%o0, PSR_PIL, %o0	! only look at priority, paranoid
pri_common:
	or	%o2, %o0, %o2		! or in new priority level
	mov	%o2, %psr		! set priority
	and	%o1, PSR_PIL, %o0	! psr delay...
	retl
	.empty				! next instruction ok in delay slot

/*
 * no_fault()
 * turn off fault catching.
 */
	ENTRY(no_fault)
	sethi	%hi(_uunix), %o2
	ld	[%o2 + %lo(_uunix)], %o2
	retl
	clr	[%o2+U_LOFAULT]		! turn off lofault

/*
 * on_fault()
 * Catch lofault faults. Like setjmp except it returns one
 * if code following causes uncorrectable fault. Turned off
 * by calling no_fault().
 */
	ENTRY(on_fault)
	set	lfault, %o0		! use special jmp_buf
	set	catch_fault, %o1
	sethi	%hi(_uunix), %o2
	ld	[%o2 + %lo(_uunix)], %o2
	b	_setjmp			! let setjmp do the rest
	st	%o1, [%o2+U_LOFAULT]	! put catch_fault in u.u_lofault

catch_fault:
	save	%sp, -WINDOWSIZE, %sp	! goto next window so that we can rtn
	set	lfault, %o0		! use special jmp_buf
	sethi	%hi(_uunix), %o2
	ld	[%o2 + %lo(_uunix)], %o2
	b	_longjmp		! let longjmp do the rest
	clr	[%o2+U_LOFAULT]		! turn off lofault

.reserve lfault, (2*4), "data", 4	! special jmp_buf for on_fault


/*
 * Setjmp and longjmp implement non-local gotos using state vectors
 * type label_t.
 *
 * setjmp(lp)
 * label_t *lp;
 */
	ENTRY(setjmp)
	st	%o7, [%o0 + L_PC]	! save return address
	st	%sp, [%o0 + L_SP]	! save stack ptr
	retl
	clr	%o0			! return 0

/*
 * longjmp(lp)
 * label_t *lp;
 */
	ENTRY(longjmp)
	!
	! The following save is required so that an extra register
	! window is flushed.  Flush_windows flushes nwindows-2
	! register windows.  If setjmp and longjmp are called from
	! within the same window, that window will not get pushed
	! out onto the stack without the extra save below.  Tail call
	! optimization can lead to callers of longjmp executing
	! from a window that could be the same as the setjmp,
	! thus the need for the following save.
	!
	save    %sp, -SA(WINDOWSIZE), %sp
	call	_flush_windows		! flush all but this window
	nop
	ld	[%i0 + L_PC], %i7	! restore return addr
	ld	[%i0 + L_SP], %fp	! restore sp for dest on foreign stack
	ret				! return 1
	restore	%g0, 1, %o0		! takes underflow, switches stacks

#ifdef sun4
/*
 * Enable and disable video.
 */
	ENTRY(setvideoenable)
	tst	%o0
	bnz	_on_enablereg
	mov	ENA_VIDEO, %o0
	b,a	_off_enablereg
#endif sun4

#ifndef sun4m
/*
 * Enable DVMA.
 */
	ENTRY(enable_dvma)
	mov	ENA_SDVMA, %o0		! enable system DVMA

	! fall through to on_enablereg

/*
 * Turn on a bit in the system enable register.
 * on_enablereg((u_char)bit)
 */
	ENTRY(on_enablereg)
	mov	%psr, %o3
	or	%o3, PSR_PIL, %g1	! spl hi to lock enable reg update
	mov	%g1, %psr
	nop; nop;			! psr delay
	sethi	%hi(_enablereg), %o1	! software copy of enable register
	ldub	[%o1 + %lo(_enablereg)], %g1 ! get software copy
	set	ENABLEREG, %o2		! address of real version in hardware
	bset	%o0, %g1		! turn on bit
	stb	%g1, [%o1 + %lo(_enablereg)] ! update software copy
	stba	%g1, [%o2]ASI_CTL	! write out new enable register
	stb	%g1, [%o1 + %lo(_enablereg)] ! update soft copy, for cache off
	mov	%o3, %psr		! restore psr
	nop				! psr delay
	retl
	nop
#endif !sun4m

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
 * 330		40		11
 * 470		30		16
 *
 * sun4m	var.	calculated in machdep.c
 */
	ENTRY(usec_delay)
	sethi	%hi(_Cpudelay), %o5
	ld	[%o5 + %lo(_Cpudelay)], %o4 ! microsecond countdown counter
	orcc	%o4, 0, %o3		! set cc bits to nz
	
1:	bnz	1b			! microsecond countdown loop
	subcc	%o3, 1, %o3		! 2 instructions in loop

	subcc	%o0, 1, %o0		! now, for each microsecond...
	bg	1b			! go n times through above loop
	orcc    %o4, 0, %o3 
	retl
	nop

/*
 * movtuc(length, from, to, table)
 *
 * VAX movtuc instruction (sort of).
 */
	ENTRY(movtuc)
	tst	%o0
	ble	2f			! check length
	clr	%o4

	ldub	[%o1 + %o4], %g1	! get next byte in string
0:
	ldub    [%o3 + %g1], %g1	! get corresponding table entry
	tst	%g1			! escape char?
	bnz	1f
	stb	%g1, [%o2 + %o4]	! delay slot, store it

	retl				! return (bytes moved)
	mov	%o4, %o0
1:
	inc	%o4			! increment index
	cmp	%o4, %o0		! index < length ?
	bl,a	0b
	ldub	[%o1 + %o4], %g1	! delay slot, get next byte in string
2:
	retl				! return (bytes moved)
	mov	%o4, %o0

#ifndef sun4m
/*
 * Disable DVMA.
 */
	ENTRY(disable_dvma)
	mov	ENA_SDVMA, %o0		! disable system DVMA

	! fall through to off_enablereg

/*
 * Turn off a bit in the system enable register.
 * off_enablereg((u_char)bit)
 */
	ENTRY(off_enablereg)
	mov	%psr, %o3
	or	%o3, PSR_PIL, %g1	! spl hi to lock enable reg update
	mov	%g1, %psr
	nop; nop;			! psr delay
	sethi	%hi(_enablereg), %o1	! software copy of enable register
	ldub	[%o1 + %lo(_enablereg)], %g1 ! get software copy
	set	ENABLEREG, %o2		! address of real version in hardware
	bclr	%o0, %g1		! turn off bit
	stb	%g1, [%o1 + %lo(_enablereg)] ! update software copy
	stba	%g1, [%o2]ASI_CTL	! write out new enable register
	mov	%o3, %psr		! restore psr
	nop				! psr delay
	retl
	nop
#endif !sun4m

/*
 * scanc(length, string, table, mask)
 *
 * VAX scanc instruction.
 */
	ENTRY(scanc)
	tst	%o0
	ble	1f			! check length
	clr	%o4
0:
	ldub	[%o1 + %o4], %g1	! get next byte in string
	cmp	%o4, %o0		! interlock slot, index < length ?
	ldub	[%o2 + %g1], %g1	! get corresponding table entry
	bge	1f			! interlock slot
	btst	%o3, %g1		! apply the mask
	bz,a	0b
	inc	%o4			! delay slot, increment index
1:
	retl				! return(length - index)
	sub	%o0, %o4, %o0

/*
 * if a() calls b() calls caller(),
 * caller() returns return address in a().
 */
	ALTENTRY(caller)
	retl
	mov	%i7, %o0

/*
 * if a() calls callee(), callee() returns the
 * return address in a();
 */
	ALTENTRY(callee)
	retl
	mov	%o7, %o0

/*
 * Get vector base register
 */
	ALTENTRY(gettbr)
	ALTENTRY(getvbr)
	retl				! leaf routine return
	mov	%tbr, %o0		! read trap base register

/*
 * Get processor state register
 */
        ALTENTRY(getpsr)
        retl                            ! leaf routine return
        mov     %psr, %o0               ! read program status register

/* 
 * return the current stack pointer
 */
	ALTENTRY(getsp)
	retl				! leaf routine return
	mov	%sp, %o0		! return sp

/*
 * insque(entryp, predp)
 *
 * Insert entryp after predp in a doubly linked list.
 */
	ENTRY(_insque)
	ld	[%o1], %g1		! predp->forw
	st	%o1, [%o0 + 4]		! entryp->back = predp
	st	%g1, [%o0]		! entryp->forw = predp->forw
	st	%o0, [%o1]		! predp->forw = entryp
	retl
	st	%o0, [%g1 + 4]		! predp->forw->back = entryp

/*
 * remque(entryp)
 *
 * Remove entryp from a doubly linked list
 */
	ENTRY(_remque)
	ld	[%o0], %g1		! entryp->forw
	ld	[%o0 + 4], %g2		! entryp->back
	st	%g1, [%g2]		! entryp->back = entryp->forw
	retl
	st	%g2, [%g1 + 4]		! entryp->forw = entryp->back

#ifndef sun4m
/*
 * Turn on or off bits in the interrupt register.
 * We must lock out interrupts, since we don't have an atomic or/and to mem.
 * set_intreg(bit, flag)
 *	int bit;		bit mask in interrupt reg
 *	int flag;		0 = off, otherwise on
 */
	ALTENTRY(set_intreg)
	ALTENTRY(set_intmask)		! alt entry point, mask is here.
	mov	%psr, %g2
	or	%g2, PSR_PIL, %g1	! spl hi to protect intreg update
	mov	%g1, %psr
	nop;				! psr delay
	tst	%o1
	set	INTREG_ADDR, %o3	! interrupt register address
	ldub	[%o3], %g1		! read interrupt register
	bnz,a	1f
	bset	%o0, %g1		! on
	bclr	%o0, %g1		! off
1:
	stb	%g1, [%o3]		! request a level 1 interrupt
	mov	%g2, %psr		! splx
	nop				! psr delay
	retl
	nop

#endif

/*
 * return 1 if an interrupt is being serviced (on interrupt stack),
 * otherwise return 0.
#ifdef	MULTIPROCESSOR
 * The interrupt stack resides in per-cpu storage high in the
 * virtual address space, higher than the user stacks and lower
 * than the kernel stack. Use a range check instead of just
 * assuming any stack ptr less than eintstack is on this stack.
#endif	MULTIPROCESSOR
 */
	ALTENTRY(servicing_interrupt)
	clr	%o0			! assume no
#ifdef	MULTIPROCESSOR
	set	intstack, %g1
	cmp	%sp, %g1		! check if on int stack
	blu	1f			! if no, return.
	.empty			! first half of following set ok in trailer.
#endif	MULTIPROCESSOR
	set	eintstack, %g1
	cmp	%sp, %g1		! check if on int stack
	bleu,a	1f			! no, we annul the next instruction
	mov	1,%o0
1:	
	retl				! leaf routine return
	nop				!
/*
 * Turn on a software interrupt (H/W level 1).
 */
	ALTENTRY(siron)
	set	IR_SOFT_INT1, %o0
	b	_set_intreg
	mov	1, %o1

#ifdef sun4
/*
 * Enable and disable video interrupt. (sun4 only)
 * setintrenable(value)
 *	int value;		0 = off, otherwise on
 */
	ALTENTRY(setintrenable)
	mov	%o0, %o1
	b	_set_intreg
	mov	IR_ENA_VID8, %o0
#endif sun4

/***********************************************************************
 *	Hardware Access Routines
 */

/*
 * Make the "LDSTUB" instruction available
 * for C code to use. Parameter is ptr to char;
 * sets byte to 0xFF and returns the old value.
 */
	ALTENTRY(ldstub)
	mov	%o0, %o1
	retl
	ldstub	[%o1], %o0

/*
 * Make the "SWAPL" instruction available
 * for C code to use. First parm is new value,
 * second parm is ptr to int; returns old value.
 */
	ALTENTRY(swapl)

#ifdef	JUNK_AROUND_SWAPL
	mov	%psr, %o4
	andn	%o4, PSR_ET, %o3
	mov	%o3, %psr		! disable traps
	nop ; nop ; nop		! complete paranoia
	mov	1, %o5			! what we will write to ECC register ...
	ld	[%o1], %g0		! prevent tlb miss on swap
#endif
	swap	[%o1], %o0

#ifdef	JUNK_AROUND_SWAPL
	sta	%o5, [%g0]0x2F		! force some MBus activity (write ECC reg)
	nop ; nop ; nop		! complete paranoia
	mov	%o4, %psr		! restore trap enable bit
	nop ; nop ; nop		! complete paranoia
#endif
	retl
	nop

/*
 * Generic "test and set" function
 */
	ALTENTRY(tas)
	mov	%o0, %o1
	b	_swapl
	sub	%g0, 1, %o0

/*
 * Generic "test and clear" function
 */
	ALTENTRY(tac)
	mov	%o0, %o1
	b	_swapl
	mov	%g0, %o0

/*
 * stack register access
 */
	ALTENTRY(stackpointer)
	retl
	mov	%o6, %o0

	ALTENTRY(framepointer)
	retl
	mov	%i6, %o0
