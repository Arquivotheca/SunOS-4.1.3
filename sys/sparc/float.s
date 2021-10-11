/*	@(#)float.s 1.1 92/07/30 SMI	*/

/*
 *	Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <machine/asm_linkage.h>
#include <machine/trap.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <sys/signal.h>
#include "assym.s"

/*
 * Floating point trap handling.
 *
 *	The FPU may not be in the current configuration.
 *	If an fp_disabled trap is generated and the EF bit
 *	in the psr equals 1 (floating point was enabled)
 *	then there is not a FPU in the configuration
 *	and the global variable fp_exists is cleared.
 *
 *	When a user process is first started via exec,
 *	floating point operations will be disabled by default.
 *	Upon execution of the first floating point instruction,
 *	a fp_disabled trap will be generated; at which point
 *	a check is made to see if the FPU exists, (fp_exists > 0),
 *	if not the instruction is emulated in software, otherwise
 *	the uarea is updated signifying use of the FPU so that 
 *	future floating point context switches will save and restore
 *	floating point state. The trapped instruction will be
 *	restarted and processing will continue as normal.
 *
 *	When a operation occurs that the hardware cannot properly
 *	handle, an unfinshed fp_op exception will be generated.
 *	Software routines in the kernel will be	executed to
 *	simulate proper handling of such conditions.
 *	Exception handling will emulate all instructions
 *	in the floating point address queue.
 *
 *	At process context switch time we save fp state of 
 *	the current process if it was using the fpu.
 *	The next user of the FPU will get a fp_disabled
 *	trap which restores the state if it was already
 *	using it or initializes it if the trap was because
 *	the first use of the FPU by a process.
 *	An optimization in context switch causes a user whose
 *	context is already in the FPU to have the FPU reenabled
 *	automatically without taking fp_disabled trap.
 *
 *	The fp context of process is not stored in the uarea.
 *	Only a pointer to its state is kept in the uarea.
 *	The variable fp_ctxp points to the floating point
 *	context save area for the current context loaded in
 *	the fpu.
 *
 *	Forks currently always execute the child before returning
 *	to the parent,  therefore fp state of the parent is stored into
 *	its fp context durring the fork.  The child starts with the fpu
 *	enabled inheriting the parent's registers.  When the parent runs
 *	again it will restore the fp state from the context saved
 *	during the fork.
 *
 *	NOTE: This code DOES NOT SUPPORT KERNEL USE OF THE FPU
 */
	.seg	"text"
	.align	4
/*
 * syncfpu() synchronizes the fpu with the
 * the iu as it causes the processor to wait until all
 * floating point operations in the FPQ complete... 
 * by the time the kernel gets to the point of executing
 * syncfpu, most floating point operations should have completed
 * and generally this happens quickly unless there is pending
 * exception which has to be handled.
 * We are passed the pointer to trap's (or syscall's) register
 * structure, which we save in a global for use in fp_traps() in case we
 * do take a pending exception.  If we didn't, we'd process the
 * exception using the kernel's registers and not the user's, and could
 * panic if the user was doing an unimplemented fpop (1025966).
 */
	ENTRY(syncfpu)

	mov	%psr, %o5		! is floating point enabled
	sethi	%hi(PSR_EF), %o4
	btst	%o4, %o5
	bz	1f
	sethi	%hi(_fptraprp), %o5
	ld	[%o5 + %lo(_fptraprp)], %o3
	st	%o0, [%o5 + %lo(_fptraprp)]
	sethi	%hi(fsrholder), %o4
	!
	! synchronize with fpu, take an exception, if one is pending
	!
	st	%fsr, [%o4 + %lo(fsrholder)]
	retl
	st	%o3, [%o5 + %lo(_fptraprp)]

1:	retl
	.empty			! the following set PSR_EF is ok in delay slot 

/*
 * FPU probe - try a floating point instruction to see if there
 * really is an FPU in the current configuration, exectued once
 * from autoconf when booting.
 */
	ENTRY(fpu_probe)
	set	PSR_EF, %g1		! enable floating-point
	mov	%psr, %g2		! read psr, save value in %g2
	or	%g1, %g2, %g3		! new psr with fpu enabled
	mov	%g3, %psr		! write psr
	nop;nop				! psr delay...
	sethi	%hi(_zeros), %g3
fpuprobe_ld:
	ld	[%g3 + %lo(_zeros)], %fsr ! probe for fpu, maybe trap

	!
	! If there is not an FPU, we will get a fp_disabled trap
	! when we try to load the fsr, which will clear the fpu_exists flag,
	! and skip over the ld

	! part of fix for 1041977 (fitoX fix can panic kernel)
	! snarf the FPU version, if it exists
	sethi	%hi(_fpu_exists), %g3
	ld	[%g3 + %lo(_fpu_exists)], %g3
	mov	7, %g1			! assume no FPU
	tst	%g3
	bz	1f
	sethi	%hi(_fpu_version), %g3

	! We know the fpu exists; we are still enabled for it
	sethi	%hi(fsrholder), %g1
	st	%fsr, [%g1 + %lo(fsrholder)]
	ld	[%g1 + %lo(fsrholder)], %g1	! snarf the FSR
	set	FSR_VERS, %o0
	and	%g1, %o0, %g1			! get version
	srl	%g1, FSR_VERS_SHIFT, %g1	! and shift it down

1:
	st	%g1, [%g3 + %lo(_fpu_version)]

	retl
	mov	%g2, %psr		! restore old psr, turn off FPU

/*
 * Floating Point Exceptions.
 * handled according to type:
 *	0) no_exception
 *		do nothing
 *	1) IEEE_exception
 *		re-execute the faulty instruction(s) using
 *		software emulation (must do every instruction in FQ)
 *	2) unfinished_fpop
 *		re-execute the faulty instruction(s) using
 *		software emulation (must do every instruction in FQ)
 *	3) unimplemented_fpop
 *		an unimplemented instruction, if it is legal,
 *		will cause emulation of the instruction (and all
 *		other instuctions in the FQ)
 *	4) sequence_error
 *		panic, this should not happen, and if it does it
 *		it is the result of a kernel bug
 *
 * This code assumes the trap preamble has set up the window evironment
 * for execution of kernel code.
 */
	.global	_fp_exception
_fp_exception:
	sethi	%hi(_fp_ctxp), %l5
	ld	[%l5 + %lo(_fp_ctxp)], %l5

	mov %l5, %g4

	st	%fsr, [%l5 + FPCTX_FSR]	! get floating point status
	ld	[%l5 + FPCTX_FSR], %g1
	set	FSR_FTT, %l4
	and	%g1, %l4, %g1		! mask out trap type
	srl	%g1, FSR_FTT_SHIFT, %l4	! use ftt after we dump queue

dumpfq:
	clr	%g2			! FQ offset
        set     FSR_QNE, %g3            ! the queue is not empty bit
	add	%l5, FPCTX_Q, %l3
1:
	ld	[%l5 + FPCTX_FSR], %g1	! test fsr
	btst	%g3, %g1		! is the queue empty?
	bz	2f			! yes, go figure out what to do


	! for Calvin FPU, if two instructions in FQ, and exception and
	! [%l3 + %g2] not in cache, gives wrong answer.  we preload cache
	! BUGID 1038405
	ld	[%l3 + %g2], %g0	! Calvin FPU FIX KLUDGE XXX HACK
	std	%fq, [%l3 + %g2]	! store an entry from FQ

	! the fpc that works with the ti8847 requires this
	nop; nop;

	add	%g2, 8, %g2		! increment offset in FQ
	b	1b
	st	%fsr, [%l5 + FPCTX_FSR]	! get floating point status
2:
	!
	! emulating floating point instructions
	!
	wr	%l0, PSR_ET, %psr	! enable traps
					! three cycle delay required, we are
					! not changing the wim so the next
					! three instructions are safe

#ifdef	MULTIPROCESSOR
	nop			! psr delay
	call	_klock_enter
	nop
#endif	MULTIPROCESSOR
	cmp	%l4, FTT_SEQ		! sanity check for bogus exceptions
	blt,a	fpeok
	mov	FPCTX_Q, %l4		! offset into stored queue entries
	!
	! Sequence error or unknown ftt exception.
	!
	set	badfpexcpmsg, %o0	! panic
	call	_panic
	mov	%l4, %o1		! mov ftt to o1 for stack backtrace

fpeok:
	srl	%g2, 3, %l6		! number of entries stored from fpq
	st	%l6, [%l5 + FPCTX_QCNT]	! store number of entries (for debug)

	! run the floating point q
	call	_fp_runq
	add	%sp, MINFRAME, %o0

fp_ret:
	!
	! clear interrupting condition in fsr
	!
	ld	[%l5 + FPCTX_FSR], %o0
	set	(FSR_FTT|FSR_QNE), %o1	! clear ftt bits and qne
	andn	%o0, %o1, %o0
	st	%o0, [%l5 + FPCTX_FSR]
	b	sys_rtt
	ld	[%l5 + FPCTX_FSR], %fsr	! ld new fsr to set condition codes

/*
 * fp_enable(fp)
 *      struct fpu *fp;
 *
 * Initialization when there is a hardware fpu.
 * Clear the fsr and initialize registers to NaN (-1)
 * The caller is supposed to update the return psr 
 * so when the return to usrerland is made, the fpu is enabled.
 */

       ENTRY(fp_enable)
        mov     %psr, %o1               ! enable the fpu
        set     PSR_EF, %o2
        or      %o1, %o2, %o3
        mov     %o3, %psr
        nop;nop;nop                     ! psr delay
	LOAD_FPREGS(%o0)			! read in fpu state
	retl
	ld	[%o0 + FPCTX_FSR], %fsr


#
/*
 * fp_dumpregs(fp_ctx_pointer)
 *
 * This routine is called when a fork is done to cause the current
 * register set inside the fpu is put into the new context.
 */
	ENTRY(fp_dumpregs)
	st	%fsr, [%o0 + FPCTX_FSR]	! store fsr in current fpctx
	STORE_FPREGS(%o0)		! store rest of regsiters
	retl
	nop

!void _fp_read_pfreg(pf, n)
!	FPU_REGS_TYPE	*pf;	/* Old freg value. */
!	unsigned	n;	/* Want to read register n. */
!
!{
!	*pf = %f[n];
!}
!
!void
!_fp_write_pfreg(pf, n)
!	FPU_REGS_TYPE	*pf;	/* New freg value. */
!	unsigned	n;	/* Want to read register n. */
!
!{
!	%f[n] = *pf;
!}

	.global	__fp_read_pfreg
__fp_read_pfreg:
	sll	%o1, 3, %o1		! Table entries are 8 bytes each.
	set	stable, %g1		! g1 gets base of table.
	jmp	%g1 + %o1		! Jump into table
	nop				! Can't follow CTI by CTI.

	.global	__fp_write_pfreg
__fp_write_pfreg:
	sll	%o1, 3, %o1		! Table entries are 8 bytes each.
	set	ltable, %g1		! g1 gets base of table.
	jmp	%g1 + %o1		! Jump into table
	nop				! Can't follow CTI by CTI.

#define STOREFP(n) jmp %o7+8 ; st %f/**/n, [%o0]

stable:
	STOREFP(0)
	STOREFP(1)
	STOREFP(2)
	STOREFP(3)
	STOREFP(4)
	STOREFP(5)
	STOREFP(6)
	STOREFP(7)
	STOREFP(8)
	STOREFP(9)
	STOREFP(10)
	STOREFP(11)
	STOREFP(12)
	STOREFP(13)
	STOREFP(14)
	STOREFP(15)
	STOREFP(16)
	STOREFP(17)
	STOREFP(18)
	STOREFP(19)
	STOREFP(20)
	STOREFP(21)
	STOREFP(22)
	STOREFP(23)
	STOREFP(24)
	STOREFP(25)
	STOREFP(26)
	STOREFP(27)
	STOREFP(28)
	STOREFP(29)
	STOREFP(30)
	STOREFP(31)

#define LOADFP(n) jmp %o7+8 ; ld [%o0],%f/**/n

ltable:
	LOADFP(0)
	LOADFP(1)
	LOADFP(2)
	LOADFP(3)
	LOADFP(4)
	LOADFP(5)
	LOADFP(6)
	LOADFP(7)
	LOADFP(8)
	LOADFP(9)
	LOADFP(10)
	LOADFP(11)
	LOADFP(12)
	LOADFP(13)
	LOADFP(14)
	LOADFP(15)
	LOADFP(16)
	LOADFP(17)
	LOADFP(18)
	LOADFP(19)
	LOADFP(20)
	LOADFP(21)
	LOADFP(22)
	LOADFP(23)
	LOADFP(24)
	LOADFP(25)
	LOADFP(26)
	LOADFP(27)
	LOADFP(28)
	LOADFP(29)
	LOADFP(30)
	LOADFP(31)

	.global	__fp_write_pfsr
__fp_write_pfsr:
	retl
	ld	[%o0], %fsr

badfpexcpmsg:
	.asciz	"unexpected floating point exception %x\n"
fpemptyqmsg:
	.asciz	"fp exception with empty queue, fsr 0x%x\n"
badfpdisabledmsg:
	.asciz	"fp_disabled: no FPU but not fpu_probe"
	.align	4

	.seg	"data"
	.align	8
fpjunk:
	.double	0
	.double	0

	/* in the case of MP, we assume either both processors have a 
	 * FPU or they don't.
	 */
	.global	_fpu_exists
_fpu_exists:
	.word	1			! assume FPU exists

	
fsrholder:
	.word	0			! dummy place to write fsr

	! part of fix for 1041977 (fitoX fix can panic kernel)
	.global _fpu_version
_fpu_version:
	.word	-1			! place to store FPU version

	.seg	"text"

/*
 * Floating point disabled trap, run after trap preamble.
 * If FPU does not exist, emulate instruction otherwise,
 * enable floating point.
 * 
 */
	.global _fp_disabled
_fp_disabled:
	!
	! lower priority, allow other interrupts, overflows, ...
	!
	wr	%l0, PSR_ET, %psr	! enable traps

#ifdef	MULTIPROCESSOR
	nop		! psr delay
	call	_klock_enter
	nop
#endif	MULTIPROCESSOR

        !
        ! fp_disable trap when the FPU is enabled; should only happen
        ! once from autoconf when there is not an FPU in the board.
        !
        set     fpuprobe_ld, %l7
        cmp     %l1, %l7                ! was trap in fpu_probe, above?
        bne	2f                      ! if not let fp_is_disabled handle it 
        set	PSR_EF, %l5		! set pSR_EF in delay slot

        sethi	%hi(_fpu_exists), %l6
        clr     [%l6 + %lo(_fpu_exists)]        ! FPU does not exist

        !
        ! Get the system trap handler's version of the psr and fix it up
        ! so that sys_rtt will restore a psr with fpu disabled.
        !
        ld      [%sp + MINFRAME + PSR*4], %l6
        bclr    %l5, %l6                        ! clear enable fp bit in psr
        st      %l6, [%sp + MINFRAME + PSR*4]
        !
        ! skip the probe instruction
        !
        ld      [%sp + MINFRAME + nPC*4], %l6   ! skip the probe instruction
        st      %l6, [%sp + MINFRAME + PC*4]    ! pc = npc
        add     %l6, 4, %l6
        b       sys_rtt                         ! return from trap
        st      %l6, [%sp + MINFRAME + nPC*4]   ! npc += 4
2:
	call	_fp_is_disabled
	add	%sp, MINFRAME, %o0

	ba	sys_rtt
	nop
