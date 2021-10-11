/*	@(#)swtch.s 1.1 92/07/30 SMI	*/

/*
 *	Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Process switching routines.
 */

#include <machine/param.h>
#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/pcb.h>
#include "assym.s"

	.seg	"text"
	.align	4

/*
 * whichqs tells which of the 32 queues qs have processes in them.
 * Setrq puts processes into queues, Remrq removes them from queues.
 * The running process is on no queue, other processes are on a queue
 * related to p->p_pri, divided by 4 actually to shrink the 0-127 range
 * of priorities into the 32 available queues.
 */

/*
 * setrq(p)
 *
 * Call should be made at spl6(), and p->p_stat should be SRUN
 */
	ENTRY(setrq)
	ld	[%o0 + P_RLINK], %g1	! firewall: p->p_rlink must be 0
	ldub	[%o0 + P_PRI], %g2	! interlock slot
	tst	%g1
	bz,a	1f
	srl	%g2, 2, %g4		! delay slot, p->p_pri / 4

	save	%sp, -SA(MINFRAME), %sp	! need to buy a window
	set	2f, %o0			! p->p_rlink not 0
	call	_panic
	nop
2:
	.asciz	"setrq"
	.align	4
1:
	sll	%g4, 3, %g2		! (p->p_pri / 4) * sizeof(*qs)
	set	_qs+4, %g3		! back ptr of queue head
	ld	[%g3 + %g2], %g3	! get qs[p->p_pri/4].ph_rlink
	mov	1, %g2			! interlock slot
	ld	[%g3], %g1		! insque(p, qs[p->p_pri].ph_rlink)
	st	%g3, [%o0 + 4]
	st	%g1, [%o0]
	st	%o0, [%g1 + 4]
	st	%o0, [%g3]
	sethi	%hi(_whichqs), %g7
	ld	[%g7 + %lo(_whichqs)], %g1
	sll	%g2, %g4, %g2		! interlock slot
	bset	%g2, %g1
	retl
	st	%g1, [%g7 + %lo(_whichqs)]

/*
 * remrq(p)
 *
 * Call should be made at spl6().
 */
	ENTRY(remrq)
	ldub	[%o0 + P_PRI], %g3
	sethi	%hi(_whichqs), %g7
	ld	[%g7 + %lo(_whichqs)], %g4
	srl	%g3, 2, %g3		! p->p_pri / 4
	mov	1, %g1			! test appropriate bit in whichqs
	sll	%g1, %g3, %g3
	btst	%g3, %g4
	bnz,a	1f
	ld	[%o0], %g1		! delay slot, remque(p)

	save	%sp, -SA(MINFRAME), %sp	! need to buy a window
	set	2f, %o0			! p not in appropriate queue
	call	_panic
	nop
2:
	.asciz	"remrq"
	.align	4
1:
	ld	[%o0 + 4], %g2
	st	%g1, [%g2]
	st	%g2, [%g1 + 4]
	cmp	%g1, %g2
	bne	3f			! queue not empty
	andn	%g4, %g3, %g4		! delay slot, queue empty,
	st	%g4, [%g7 + %lo(_whichqs)]
3:
	retl
	clr	[%o0 + P_RLINK]		! p->p_rlink = 0

	.global	_qrunflag, _runqueues

/*
 * When no processes are on the runq, swtch branches to idle
 * to wait for something to come ready.
 */
	.global	_idle
#ifdef LWP
	.global ___Nrunnable, _lwpschedule
#endif LWP
_idle:
	sethi	%hi(_whichqs), %g1
	ld	[%g1 + %lo(_whichqs)], %g1
	tst	%g1
	bz	1f			! none check other stuff
	nop

	!
	! Found something.
	!
	mov	%psr, %g1		! splhi
	bclr	PSR_PIL, %g1
	or	%g1, 10 << PSR_PIL_BIT, %g1
	mov	%g1, %psr
	nop				! psr delay
	b	sw_testq
	nop				! more psr delay

	!
	! Test for other async events.
	!
1:
#ifdef LWP
	sethi	%hi(___Nrunnable),%g2
	ld	[%g2+%lo(___Nrunnable)],%g2
	tst	%g2
	bz	2f
	nop
	call	_lwpschedule
	nop
	set	CONTEXT_REG, %g2
	stba	%g0, [%g2]ASI_CTL
2:
#endif LWP
	sethi	%hi(_qrunflag), %g1	! need to run stream queues?
	ldub	[%g1 + %lo(_qrunflag)], %g1
	tst	%g1
#ifndef SAS
	bz	_idle			! no
	nop
#else SAS
/*
 * Instead of idle-looping when we have nothing to do,
 * fake a bunch of clock ticks.
 */
	bnz	1f
	nop
	call	_fake_clockticks
	nop
	b,a	_idle
1:
#endif SAS

	call	_runqueues		! go do it
	nop
	b,a	_idle

/*
 * swtch()
 */
	ENTRY(swtch)
	save	%sp, -SA(MINFRAME), %sp
	mov	%psr, %l0		! save PSR
	andn	%l0, PSR_PIL, %g1
	or	%g1, 10 << PSR_PIL_BIT, %g1
	mov	%g1, %psr		! spl hi
	nop
	mov	1, %g2
	sethi	%hi(_noproc), %g6
	st	%g2, [%g6 + %lo(_noproc)]
	sethi	%hi(_runrun), %g6
	clr	[%g6 + %lo(_runrun)]
	sethi	%hi(_masterprocp), %l1
	ld	[%l1 + %lo(_masterprocp)], %l2
	tst	%l2			! if no proc skip saving state
	bz	sw_testq		! this happens after exit
	.empty				! hush assembler warnings

	set	_uunix, %g5		! XXX - global u register?
	ld	[%g5], %g5
	ld	[%g5+PCB_FLAGS], %g1	! pcb_flags &= ~AST_SCHED
	set	AST_SCHED, %g2
	bclr	%g2, %g1
	st	%g1, [%g5+PCB_FLAGS]

	!
	! Loop through whichqs bits looking for a non-empty queue.
	!
sw_testq:
	sethi	%hi(_whichqs), %g7
	ld	[%g7 + %lo(_whichqs)], %l1
	mov	1, %l2
	mov	0, %l3
	btst	%l2, %l1
1:
	bnz	sw_foundq		! found one
	nop

	sll	%l2, 1, %l2
	add	%l3, 8, %l3
	tst	%l2
	bnz,a	1b
	btst	%l2, %l1		! delay slot
	!
	! Found no procs, goto idle loop
	!
	mov	%psr, %g1		! allow interrupts
	andn	%g1, PSR_PIL, %g1
	mov	%g1, %psr
	b,a	_idle

sw_foundq:
	set	_qs, %l4		! get qs proc list
	ld	[%l4 + %l3], %l4	! p=qs[bit].ph_link=highest pri process
	ld	[%l4], %g1		! remque(p)
	cmp	%l4, %g1		! is queue empty?
	bne,a	2f
	ld	[%l4 + 4], %g2		! delay slot

sw_bad:
	set	4f, %o0
	call	_panic
	nop
4:
	.asciz	"swtch"
	.align	4

2:
	st	%g1, [%g2]
	st	%g2, [%g1 + 4]
	bclr	%l2, %l1		! whichqs if queue is now empty
	cmp	%g1, %g2
	be,a	3f			! queue now empty
	st	%l1, [%g7 + %lo(_whichqs)]
3:
	sethi	%hi(_noproc), %g6
	clr	[%g6 + %lo(_noproc)]
	ld	[%l4 + P_WCHAN], %g1	! firewalls
	ldub	[%l4 + P_STAT], %g2
	tst	%g1
	bz,a	4f			! p->p_wchan must be 0
	cmp	%g2, SRUN		! p->p_stat must be SRUN

	b,a	sw_bad
4:
	be,a	5f
	clr	[%l4 + P_RLINK]		! p->p_rlink = 0

	b,a	sw_bad
5:
	sethi	%hi(_masterprocp), %l1
	ld	[%l1 + %lo(_masterprocp)], %l2
	set	_cnt, %g1		! cnt.v_swtch++
	ld	[%g1 + V_SWTCH], %g2
	cmp	%l2, %l4		! if (p == masterprocp) ...
	be	6f			!	skip resume() call
	inc	1, %g2			! delay slot
	mov	%l4, %o0		! resume(p)
	call	_resume
	.empty
6:
	st	%g2, [%g1 + V_SWTCH]	! delay slot, update ctx switch count

	mov	%psr, %g2		! restore processor priority
	andn	%g2, PSR_PIL, %g2
	and	%l0, PSR_PIL, %l0
	or	%l0, %g2, %l0
	mov	%l0, %psr
	nop			! psr delay
	ret
	restore

/*
 * masterprocp is the pointer to the proc structure for the currently
 * mapped u area. It is used to set up the mapping for the u area
 * by the debugger since the u area is not in the Sysmap.
 */
	.seg	"data"
	.global	_masterprocp
_masterprocp:
	.word	0			! struct proc *masterprocp

/*
 * XXX The following is a hack so that we don't have to splzs() to
 * XXX protect the u_pcb from being clobbered by a BREAK that causes
 * XXX kadb to be entered.  We also don't want to find masterprocp and
 * XXX uunix inconsistent.
 * XXX Instead, we set kadb_defer before the critical region and clear
 * XXX it afterwards.
 * XXX Where zsa_xsint would normally CALL_DEBUG, it first checks
 * XXX kadb_defer, and if set it sets kadb_want instead.
 * XXX We check kadb_want (after clearing kadb_defer, to avoid races)
 * XXX and, if set, we call kadb.
 */
	.global _kadb_defer
_kadb_defer:
	.word	0			! int kadb_defer

	.global _kadb_want
_kadb_want:
	.word	0			! int kadb_want

	.seg	"text"

/*
 * resume(p)
 */
	ENTRY(resume)
	save	%sp, -SA(MINFRAME), %sp
	mov	%psr, %l0
	sethi	%hi(_masterprocp), %l1
	ld	[%l1 + %lo(_masterprocp)], %l2
	tst	%l2			! if there is a current proc,
	bnz	1f			! save its state,
					! if no proc, skip saving state
					! this happens after exit
	! trash_windows, no current proc
	or	%l0, PSR_PIL, %o4	! spl hi
	set	_scb, %o1		! get NW-1, which is
	ldub	[%o1 + 31], %o3		! last byte of text fault trap vector
	and	%l0, 0x1f, %o2		! mask cwp
	cmp	%o2, %o3		! compare cwp to NW-1
	be	2f			! calculate wim
	mov	1, %o3			! if equal new wim is 1, wraparound
	inc	%o2
	sll	%o3, %o2, %o3		! if less create wim of sll(1,cwp+1)
2:
	mov	%o4, %psr		! raise priority
	nop;nop				! psr delay
	mov	(10<<PSR_PIL_BIT), %l6	! clock priority is 10
	mov	%o3, %wim		! install new wim
	b	4f
	mov	%l0, %psr		! restore priority

	! store state for current proc
1:	set	_uunix, %g5		! XXX - global u register?
	ld	[%g5], %g5

	ld	[%g5 + PCB_FPCTXP], %l3	! is floating point being used
	tst	%l3			! by the current process?
	bz	5f			! if not skip saving FPU state
	st	%l0, [%g5 + PCB_PSR]	! save psr (for PSR_PIL)

	sethi	%hi(_fpu_exists), %l4	! fpu is present flag
	ld	[%l4 + %lo(_fpu_exists)], %l4
	tst	%l4			! well, do we have FPU hardware?
	bz	5f			! no fpu, nothing to do
	nop
	st	%fsr, [%l3 + FPCTX_FSR]	! save current fpu state
	STORE_FPREGS(%l3)		! assumes %l3 points at fpreg save area
5:
	ld	[%l2 + P_AS], %o0	! does it
	tst	%o0			!   have an address space?
	bz	3f			! skip if not
	mov	(10<<PSR_PIL_BIT), %l6	! clock priority is 10

	call	_rm_asrss		! find out memory claim for
	nop				!   as associated with this process

	st	%o0, [%l2 + P_RSSIZE]	! store with process
3:
	!
	! Flush the register windows to the stack.
	! This saves all the windows except the one we're in now.
	! The WIM will be immediately ahead of us after this.
	!
	call	_flush_windows
	nop
4:
	andn	%l0, PSR_PIL, %l0	! clear priotity field
	or	%l0, %l6, %l0		! spl clock, or in new priority
	mov	%l0, %psr		! raise priority

	!
	! Begin critical region where we can't let BREAK call kadb
	! (this also serves as a psr delay)
	!
	sethi	%hi(_kadb_defer), %g1
	mov	1, %g2
	st	%g2, [%g1 + %lo(_kadb_defer)]
	sethi	%hi(_uunix), %g5
	ld	[%g5 + %lo(_uunix)], %g5
	st	%i7, [%g5 + PCB_PC]	! save ret pc and sp in pcb
	st	%fp, [%g5 + PCB_SP]
	st	%i0, [%l1 + %lo(_masterprocp)]

	!
	! Switch to new u area.
	!
	set	CONTEXT_REG, %l3	! setup kernel context (0)
	stba	%g0, [%l3]ASI_CTL

	ld	[%i0 + P_UAREA], %g5		! address of uarea for new proc
	set	_uunix, %g2			! interlock, set uunix - XXX
	st	%g5, [%g2]			! install new uarea

	!
	! We snarf the PC and SP here so we can end the critical
	! region that much sooner.
	ld	[%g5 + PCB_PC], %i7
	ld	[%g5 + PCB_SP], %fp
	!
	! End critical region where we can't let BREAK call kadb.
	! (We just changed uunix, so if we take a BREAK we'll store the
	! registers for kadb on the new pcb, which isn't in use now).
	! So we clear the flag, and then check to see if we owe kadb a
	! call.
	!
	sethi	%hi(_kadb_defer), %g1		! clear defer first
	st	%g0, [%g1 + %lo(_kadb_defer)]
	sethi	%hi(_kadb_want), %g1		! then check want
	ld	[%g1 + %lo(_kadb_want)], %g2
	tst	%g2				! in case BREAK'd between
	bz	1f
	ld	[%i0 + P_AS], %l0	! check p->p_as

	mov	%g5, %l1		! save %g5 (uptr) over call
	call	_call_debug_from_asm
	st	%g0, [%g1 + %lo(_kadb_want)]
	mov	%l1, %g5		! restore %g5
1:

	!
	! Check to see if we already have context. If so then set up the
	! context. Otherwise we leave the proc in the kernels context which
	! will cause it to fault if it ever gets back to userland.
	!
	tst	%l0
	bz	1f			! if zero, skip ahead
	nop				! delay slot

	ld	[%l0 + A_HAT_CTX], %l0	! check (p->p_as->a_hat.hat_ctx)
	tst	%l0
	bz	1f			! if zero, skip ahead
	nop				! delay slot

	!
	! Switch to the procs context.
	!
	ldub	[%l0 + C_NUM], %g1	! get context number
	stba	%g1, [%l3]ASI_CTL	! set it up
	sethi	%hi(_ctx_time), %l1
	lduh	[%l1 + %lo(_ctx_time)], %g1
	add	%g1, 1, %g2
	sth	%g1, [%l0 + C_TIME]	! Set context clock to LRU clock
	sth	%g2, [%l1 + %lo(_ctx_time)]! ++ctx_time
	ldub    [%l0], %g1		! get the context bits
	andn    %g1, C_CLEAN_BITMASK, %g1 ! zero the context's clean bit
	stb     %g1, [%l0]
1:
	!
	! if the new process does not have any fp ctx
	!	we do nothing with regards to the fpu
	! if the new process does have fp ctx
	!	we enable the fpu and load its context now
	!	if we have fpu hardware
	!
	ld	[%g5 + PCB_FPCTXP], %g1	! new process fp ctx pointer
	tst	%g1			! if new process fp ctx printer == 0
	bz	2f			! it hasn't used the fpu yet
	ld	[%g5 + PCB_PSR], %l0

	sethi	%hi(_fp_ctxp), %g2	! old fp ctx pointer
	ld	[%g2 + %lo(_fp_ctxp)], %l1

	sethi	%hi(_fpu_exists), %l2
	ld	[%l2 + %lo(_fpu_exists)], %l3
	tst	%l3
	bz	2f
	st	%g1, [%g2 + %lo(_fp_ctxp)] ! install new fp context

	!
	! if this is the same process that last used the fpu
	! we can skip loading its state
	!
	cmp	%g1, %l1		! if the fp ctx belongs to new process
	be	3f			! nothing to do
			! maybe branch to 2 instead

	!
	! New process is different than last process that used the fpu
	! Load the new process floating point state
	! The fpu must be enabled via the psr as the last
	! process may not have used it
	!
	set	PSR_EF, %l6
	mov	%psr, %g3
	or	%g3, %l6, %g3
	mov	%g3, %psr

	nop; nop; nop				! psr delay, (paranoid)
	ld	[%g1 + FPCTX_FSR], %fsr		! restore fsr
	LOAD_FPREGS(%g1)
3:
	ld	[%i0 + P_STACK], %l1	! initial kernel sp for this process
	add	%l1, (MINFRAME + PSR*4), %l1 ! psr addr on process stack
	ld	[%l1], %l3		! read the psr value from the stack
	or	%l0, %l6, %l0		! enable fpu, new psr
	or	%l3, %l6, %g1		! enable fpu, psr stored in stack
	st	%g1, [%l1]		! update stack, sys_rtt will use it
2:
	!
	! Return to new proc. Restore will cause underflow.
	!
	sub	%fp, WINDOWSIZE, %sp	! establish sp, for interrupt overflow
	mov	%psr, %g2
	and	%l0, PSR_PIL, %l0	! restore PSR_PIL
	andn	%g2, PSR_PIL, %g2
	or	%l0, %g2, %l0
	mov	%l0, %psr
	nop				! psr delay
	ret
	restore

/*
 * start_child(parentp, parentu, childp, nfp, new_context)
 *	struct proc *parentp, *childp;
 *	struct user *parentu;
 *	int *new_context;
 *	struct file **nfp;
 * This saves the parent's state such that when resume'd, the
 * parent will return to the caller of startchild.
 * Equivalent to the first half of resume.
 * It then calls conschild to create the child process
 *
 * We always resume the parent in the kernel.
 * The child usually resumes in userland.
 *
 * start_child is only called from newproc
 */
	ENTRY(start_child)
	save	%sp, -SA(MINFRAME), %sp
	mov	%psr, %l0
	sethi	%hi(_masterprocp), %l1
	ld	[%l1 + %lo(_masterprocp)], %l2

	!
	! Begin critical region where we can't let BREAK call kadb
	! (This critical region lasts through cons_child, ends in yield_child.)
	!
	sethi	%hi(_kadb_defer), %g1
	mov	1, %g2
	st	%g2, [%g1 + %lo(_kadb_defer)]

	!
	! arrange for the parent to resume the caller of start child
	!
	set	_uunix, %l3		! get u pointer, XXX, global u register
	ld	[%l3], %l4
	st	%l0, [%l4 + PCB_PSR]	! save psr, ret pc, and sp in pcb
	st	%i7, [%l4 + PCB_PC]
	st	%fp, [%l4 + PCB_SP]

	ld	[%l2 + P_AS], %o0	! does it have an address space?
	sethi	%hi(_cons_child), %l5	! interlock slot
	tst	%o0
	bz	1f			! skip if not
	or	%l5, %lo(_cons_child), %l5

	call	_rm_asrss		! find out memory claim for
	nop				!   as associated with this process
	st	%o0, [%l2 + P_RSSIZE]	! store with process
1:
	jmp	%l5
	restore				! give back register window
					! args already in the 'out' register

/*
 * yield_child(parent_ar0, child_stack, childp, parent_pid, childu, seg)
 *
 * Should be called at splclock()!
 * used in fork() instead of newproc/sleep/swtch/resume
 * cons up a child process stack and start it running
 * basically copy the needed parts of the kernel stack frame of
 * the parent, fixing appropriate entries for the child
 */
	ENTRY(yield_child)
	save	%sp, -WINDOWSIZE, %sp

	call	_flush_windows		! get the all registers on the stack
	sethi	%hi(PSR_C), %l2		! delay slot, set C bit in %l2
					! the parent will eventually return
					! via resume to cons_child which
					! will return to newproc
	!
	! Saved %l5 flags child is kernel proc. kernel procs never return to
	! user land, so we skip setting up the child's u_ar0 regs. This is
	! required, since if the parent is itself a kernel proc, it's
	! u_ar0 may not be meaningful.
	!
	ld	[%i1 + 5*4], %l5
	tst	%l5
	bnz	1f
	nop

	! struct regs psr, pc, ncp, y
	ld	[%i0 + PSR*4], %o4
	ld	[%i0 + nPC*4], %o5		! skip trap instruction
	andn	%o4, %l2, %o4			! clear carry bit
	st	%o4, [%i1 + MINFRAME + PSR*4]
	st	%o5, [%i1 + MINFRAME + PC*4]	! pc = npc
	add	%o5, 4, %o5			! npc += 4
	st	%o5, [%i1 + MINFRAME + nPC*4]
	ld	[%i0 + Y*4], %o4
	st	%o4, [%i1 + MINFRAME + Y*4]

	! struct regs, user globals
	ld	[%i0 + G1*4], %o4
	st	%o4, [%i1 + MINFRAME + G1*4]
	ldd	[%i0 + G2*4], %o4
	std	%o4, [%i1 + MINFRAME + G2*4]
	ldd	[%i0 + G4*4], %o4
	std	%o4, [%i1 + MINFRAME + G4*4]
	ldd	[%i0 + G6*4], %o4
	std	%o4, [%i1 + MINFRAME + G6*4]

	! struct regs, user outs
	mov	%i3, %o4			! parent pid childs ret val r0
	mov	1, %o5				! 1 childs ret value r1
	std	%o4, [%i1 + MINFRAME + O0*4]
	ldd	[%i0 + O2*4], %o4
	std	%o4, [%i1 + MINFRAME + O2*4]
	ldd	[%i0 + O4*4], %o4
	std	%o4, [%i1 + MINFRAME + O4*4]
	ldd	[%i0 + O6*4], %o4
	std	%o4, [%i1 + MINFRAME + O6*4]
1:
	! fake a return from syscall() to return to the child
	mov	%psr, %g1		! spl hi
	or	%g1, PSR_PIL, %g2
	mov	%g2, %psr

	set	_masterprocp, %o0	! install child proc as master
	st	%i2, [%o0]
	set	_uunix, %o1		! install child uarea - XXX
	st	%i4, [%o1]		! global uarea pointer?

	!
	! End critical region where we can't let BREAK call kadb.
	! (We just changed uunix, so if we take a BREAK we'll store the
	! registers for kadb on the new pcb, which isn't in use now).
	! So we clear the flag, and then check to see if we owe kadb a
	! call.
	!
	sethi	%hi(_kadb_defer), %g1		! clear defer first
	st	%g0, [%g1 + %lo(_kadb_defer)]
	sethi	%hi(_kadb_want), %g1		! then check want
	ld	[%g1 + %lo(_kadb_want)], %g2
	tst	%g2				! in case BREAK'd between
	bz	1f
	ld	[%i2 + P_AS], %l4	! check p->p_as

	call	_call_debug_from_asm
	st	%g0, [%g1 + %lo(_kadb_want)]
1:

	!
	! Check to see if we already have context. If so then set up the
	! context. Otherwise we leave the proc in the kernels context which
	! will cause it to fault if it ever gets back to userland.
	!
	set	CONTEXT_REG, %l3	! setup context reg pointer
	tst	%l4
	bz	1f			! if zero, skip ahead

	! delay slot, get new process start address
	ld	[%i1 + 6*4], %l5	! %l6 has proc start address

	ld	[%l4 + A_HAT_CTX], %l4	! check (p->p_as->a_hat.hat_ctx)
	tst	%l4
	bnz	2f			! if zero, skip ahead
	.empty				! hush assembler warnings
1:	mov	%i1, %fp		! install the new childs stack

	stba	%g0, [%l3]ASI_CTL	! if no context, use kernels

3:	sub	%fp, WINDOWSIZE, %sp	! establish sp, for interrupt overflow
	mov	%psr, %g1		! spl 0
	andn	%g1, PSR_PIL, %g1
	mov	%g1, %psr
	ld	[%i1], %i0		! %l0 has arg, set arg to new proc
	jmp	%l5
	restore				! restore causes underflow

	!
	! Switch to the procs context.
	!
2:	ldub	[%l4 + C_NUM], %g1	! get context number
	stba	%g1, [%l3]ASI_CTL	! set it up
	sethi	%hi(_ctx_time), %l1
	lduh	[%l1 + %lo(_ctx_time)], %g1
	add	%g1, 1, %g2
	sth	%g1, [%l4 + C_TIME]	! Set context clock to LRU clock
	ldub    [%l4], %g1              ! get the context bits
	andn    %g1, C_CLEAN_BITMASK, %g1 ! zero the context's clean bit
	stb     %g1, [%l4]
	b	3b
	sth	%g2, [%l1 + %lo(_ctx_time)]! ++ctx_time

/*
 * finishexit(vaddr)
 * Done with the current stack;
 * switch to the exit stack, segu_release, and swtch.
 */
	ENTRY(finishexit)
	save	%sp, -SA(MINFRAME), %sp

	! XXX - previous calls caused windows to be flushed
	! so the following flush may not be needed
	call	_flush_windows		! dump reigster file now
	nop				! registers needed for post mortem

	mov	%psr, %l4
	or	%l4, PSR_PIL, %o5
	mov	%o5, %psr		! spl hi
	set	eintstack, %o3		! Are we on interrupt stack?
	cmp	%sp, %o3		! If so, panic
	ble	1f
	.empty
	set	eexitstack, %o3
	sub	%o3, SA(MINFRAME), %sp	! switch to exit stack

	set	intu, %o2		! temporary uarea
	set	_uunix, %o1
	st	%o2, [%o1]

	set	_noproc, %o2		! no process now
	mov	1, %o1
	st	%o1, [%o2]

	call	_segu_release
	mov	%i0, %o0		! arg to segu_release

	set	_masterprocp, %o5
	clr	[%o5]
	mov	%l4, %psr		! restore previous priority
	nop
	b,a	_swtch
	/* NOTREACHED */
1:
	set	3f, %o0
	call	_panic
	nop
3:	.asciz	"finishexit"
	.align	4

/*
 * Setmmucntxt(procp)
 *
 * initialize mmu context for a process, if it has one
 */
	ENTRY(setmmucntxt)
	ld	[%o0 + P_AS], %o1	! check p->p_as
	set	CONTEXT_REG, %o4
	tst	%o1
	bz	1f			! if zero, skip ahead
	mov	%g0, %o0

	ld	[%o1 + A_HAT_CTX], %o1	! check (p->p_as->a_hat.hat_ctx)
	tst	%o1
	bz	1f			! if zero, skip ahead
	mov	%g0, %o0

	!
	! Switch to the procs context.
	!
	ldub	[%o1 + C_NUM], %o0	! get context number
	sethi	%hi(_ctx_time), %o5
	lduh	[%o5 + %lo(_ctx_time)], %o2
	add	%o2, 1, %o3
	sth	%o2, [%o1 + C_TIME]	! Set context clock to LRU clock
	sth	%o3, [%o5 + %lo(_ctx_time)]! ++ctx_time
        ldub    [%o1], %g1              ! get the context bits
        andn    %g1, C_CLEAN_BITMASK, %g1 ! zero the context's clean bit
        stb     %g1, [%o1]
1:
	retl
	stba	%o0, [%o4]ASI_CTL	! set it up
