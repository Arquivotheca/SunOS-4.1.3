/*	@(#)swtch.s 1.1 92/07/30 SMI 	*/

/*
 *	Copyright (c) 1990 by Sun Microsystems, Inc.
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
#include <machine/devaddr.h>
#include "assym.s"
#ifdef	MULTIPROCESSOR
#include "percpu_def.h"
#endif	MULTIPROCESSOR

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
 * Call should be made at spl6(),
 * pslock should be held
 * and p->p_stat should be SRUN
 */
	ENTRY(setrq)

#ifdef	MULTIPROCESSOR

! placing the idle process on the queue
! is fatal: other processors will not be
! able to follow the linked list through
! this proc area, getting their own idleproc
! instead.

	set	_idleproc, %g1
	cmp	%o0, %g1
	bne	1f
	nop

	retl
	nop
1:
#endif	MULTIPROCESSOR

	ld	[%o0 + P_RLINK], %g1	! firewall: p->p_rlink must be 0
	ldub	[%o0 + P_PRI], %g2	! interlock slot
	tst	%g1
	bz,a	1f
	srl	%g2, 2, %g4		! delay slot, p->p_pri / 4

	save	%sp, -SA(MINFRAME), %sp ! need to buy a window
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
	st	%g1, [%g7 + %lo(_whichqs)]

	retl
	nop

/*
 * remrq(p)
 *
 * Call should be made at spl6()
 * pslock should be held
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

	save	%sp, -SA(MINFRAME), %sp ! need to buy a window
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
	clr	[%o0 + P_RLINK]		! p->p_rlink = 0

	retl
	nop

#ifdef	MULTIPROCESSOR
/*
 * onrq(p)
 *
 * Call should be made at spl6()
 * pslock should be held
 */
	ENTRY(onrq)		! return true if proc is on the run queue

	ldub	[%o0 + P_PRI], %o1
	set	_qs, %o2
	srl	%o1, 2, %o1		! p->p_pri / 4
	sll	%o1, 3, %o1		! offset into rs
	add	%o1, %o2, %o1		! %o1 now has "sentinal"
	ld	[%o1], %o2	! get first
1:	cmp	%o2, %o0	! if it is our proc
	be,a	0f		!   return to caller
	mov	1, %o0		!     noting success.
	cmp	%o2, %o1	! if it is not the sentinal
	bne,a	1b		!   loop back
	ld	[%o2], %o2	!     and get the next proc
				! else it is the sentinal,
	mov	0, %o0		!   note failure
0:
	retl			!     and return to caller.
	nop
#endif	MULTIPROCESSOR


	.global _qrunflag, _runqueues

/*
 * When no processes are on the runq, swtch branches to idle
 * to wait for something to come ready.
 */
	.global _idle
#ifdef	LWP
	.global ___Nrunnable, _lwpschedule
#endif	LWP
_idle:
#ifdef	MULTIPROCESSOR
	save	%sp, -SA(MINFRAME), %sp
_testq:
#endif	MULTIPROCESSOR

	sethi	%hi(_whichqs), %g1
	ld	[%g1 + %lo(_whichqs)], %g1
	tst	%g1
	bz	2f			! none check other stuff
	nop

	sethi	%hi(_whichqs), %g1
	ld	[%g1 + %lo(_whichqs)], %g1
	tst	%g1			! check again inside lock
	bz	0f			! whichqs bits went away!
	nop

	!
	! Found something.
	!
	mov	%psr, %g1		! splhi
	bclr	PSR_PIL, %g1
	or	%g1, 13 << PSR_PIL_BIT, %g1
	mov	%g1, %psr
	nop				! psr delay
	b	sw_testq
	nop				! more psr delay
0:			

	!
	! Test for other async events.
	!
2:
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
	bnz	2f				! do not try to run lwpschedule
	nop
#endif	MULTIPROCESSOR

	sethi	%hi(___Nrunnable),%g2
	ld	[%g2+%lo(___Nrunnable)],%g2
	tst	%g2				! if nobody is runable,
	bz	2f				! don't waste time in lwpschedule
	nop
0:
	call	_lwpschedule
	nop
	call	_mmu_setctx
	mov	%g0, %o0
2:
#endif	LWP
#ifndef	MULTIPROCESSOR
	sethi	%hi(_qrunflag), %g1	! need to run stream queues?
	ldub	[%g1 + %lo(_qrunflag)], %g1
	tst	%g1
#ifndef SAS
	bz	_idle			! no
	nop
#else	SAS
	bnz	1f
	nop
/*
 * If we have nothing to do, snooze in the simulator.
 */
	call	_mpsas_idle_trap
	nop
	b	_idle
	nop
1:
#endif	SAS

	call	_runqueues		! go do it
	nop
	b	_idle
	nop

#else	MULTIPROCESSOR

/*
	if (initting)
		jump _testq;
	else
		return to idlework;
*/
2:
	sethi	%hi(_initing), %g1
	ld	[%g1+%lo(_initing)], %g1
	cmp	%g0, %g1
	bne	_testq
	nop

	ret
	restore

#endif	MULTIPROCESSOR

/*
 * swtch()
 */
	ENTRY(swtch)
	save	%sp, -SA(MINFRAME), %sp

	mov	%psr, %l0		! save PSR
	andn	%l0, PSR_PIL, %g1
	or	%g1, 13 << PSR_PIL_BIT, %g1
	mov	%g1, %psr		! spl hi
	nop

	mov	1, %g2
	sethi	%hi(_noproc), %g7
	st	%g2, [%g7+%lo(_noproc)]
	sethi	%hi(_runrun), %g7
	clr	[%g7+%lo(_runrun)]
	sethi	%hi(_masterprocp), %g7
	ld	[%g7+%lo(_masterprocp)], %g7
	tst	%g7			! if no proc skip saving state
	bz	sw_testq		! this happens after exit
	nop

	sethi	%hi(_uunix), %g5	! XXX - global u register?
	ld	[%g5+%lo(_uunix)], %g5
	ld	[%g5+PCB_FLAGS], %g1	! pcb_flags &= ~AST_SCHED
	set	AST_SCHED, %g2
	bclr	%g2, %g1
	st	%g1, [%g5+PCB_FLAGS]

	!
	! Loop through whichqs bits looking for a non-empty queue.
	! PSLOCK must be acquired before branching here.
	!

! %l0	saved psr
! %l1	whichqs
! %l2	marching bit
! %l3	offset into rs
! %l4	procp under consideration
! %l5	sentinal procp

sw_testq:
	sethi	%hi(_whichqs), %g7
	ld	[%g7 + %lo(_whichqs)], %l1
	mov	1, %l2
	mov	0, %l3
	btst	%l2, %l1
1:
	bnz	sw_foundq		! found one
	nop
sw_nextq:
	sll	%l2, 1, %l2
	add	%l3, 8, %l3
	tst	%l2
	bnz,a	1b
	btst	%l2, %l1		! delay slot

sw_goidle:
#ifndef	MULTIPROCESSOR
	!
	! Found no procs, goto idle loop
	!

	mov	%psr, %g1		! allow interrupts
	andn	%g1, PSR_PIL, %g1
	mov	%g1, %psr
	nop			! psr delay
	b	_idle
	nop

#else	MULTIPROCESSOR
	!
	! Stage the idle process, bypassing
	! some of the sanity checking.
	!
	sethi	%hi(_idleproc), %l4
	b	sw_stage_l4
	or	%l4, %lo(_idleproc), %l4
#endif	MULTIPROCESSOR

sw_foundq:
	set	_qs, %l5		! get qs proc list
	add	%l5, %l3, %l5		! sentinal = &qs[bit]
	ld	[%l5], %l4		! p=qs[bit].ph_link=highest pri process
	cmp	%l5, %l4		! is queue empty?
	bne,a	sw_foundp		!   [empty if self pointer]
	nop

sw_bad:

	set	4f, %o0
	call	_panic
	nop
4:
	.asciz	"swtch"
	.align	4

sw_foundp:
#ifdef	P_PAM
! note: the affinity mask test comes before the
! test for "same process", in case we are being
! forced off this processor by a PAM change.
sw_chk_pam:
	sethi	%hi(_cpuid), %g4
	ld	[%g4+%lo(_cpuid)], %g4
	mov	1, %g3
	sll	%g3, %g4, %g4		! form mask for this processor
	ld	[%l4+P_PAM], %g3	! get p->p_pam
	andcc	%g3, %g4, %g0		! can this proc run here?
	beq	sw_reject		! if not, get another.
	nop
#endif	P_PAM
sw_chk_new:
	sethi	%hi(_masterprocp), %g3
	ld	[%g3+%lo(_masterprocp)], %g3
	cmp	%l4, %g3		! if new is same as old,
	beq	sw_accept		! accept the process.
	nop

#ifdef	MULTIPROCESSOR
sw_chk_idl:
	set	_idleproc, %g1
	cmp	%l4, %g1		! if new is idleproc,
	beq	sw_reject		!   reject the process.
	nop
#endif	MULTIPROCESSOR

#ifdef	P_CPUID
sw_chk_cpuid:
	ld	[%l4 + P_CPUID], %g4
	tst	%g4			! if active somewhere ...
	bpos	sw_reject		!   reject the process.
	nop
1:
#endif	P_CPUID

! If this process' address space is
! active, reject it and try another.

#ifdef	A_HAT_CPU
sw_chk_hat:
	ld	[%l4 + P_AS], %g4
	tst	%g4
	beq	sw_hatok		! new proc has no as, contine testing
	nop
	ldub	[%g4 + A_HAT_CPU], %g3
	tst	%g3
	beq	sw_hatok		! new proc not active, continue checking
	nop
	GETMID(%g4)			! get module id, 8..11 (or 15)
	cmp	%g3, %g4		! remember, zero is "nobody".
	bne	sw_reject		! proc's hat active elsewhere, reject it.
	nop
sw_hatok:
#endif	A_HAT_CPU
! %%% Add other "reject" conditions here

	b	sw_accept		! All conditions passed - use this proc!
	nop

! For some reason, this process is not a candidate
! for execution, even though it is on the run queue.
! This can happen if the affinity mask denies this
! process can run on this processor, or
! if someone goes to sleep, but
! receives a wakeup before the cpu putting it to
! sleep manages to switch (ie. p_cpuid is nonnegative), or
! in the "vfork()" case where the parent or
! the child is heading toward sleep but not yet
! actually there.

sw_reject:
	ld	[%l4], %l4		! get next proc on queue
	cmp	%l4, %l5		! if there was a proc,
	bne	sw_foundp		!   see if it is a candidate
	nop				! else
	b	sw_nextq		!   check next queue
	nop

! This process has been cleared for takeoff.

sw_accept:
#ifdef	MULTIPROCESSOR
	!
	! turn off cpu_idle. if we are truely idle,
	! the idle process will turn it back on.
	!
	sethi	%hi(_cpu_idle), %g1
	st	%g0, [%g1+%lo(_cpu_idle)]
#endif	MULTIPROCESSOR

	ld	[%l4], %g1		! remque(%l4)
	ld	[%l4+4], %g2
	st	%g1, [%g2]
	st	%g2, [%g1 + 4]
	bclr	%l2, %l1		! whichqs if queue is now empty
	cmp	%g1, %g2
	be,a	3f			! queue now empty
	st	%l1, [%g7 + %lo(_whichqs)]
3:
	sethi	%hi(_noproc), %g7
	clr	[%g7 + %lo(_noproc)]
	ld	[%l4 + P_WCHAN], %g1	! firewalls
	ldub	[%l4 + P_STAT], %g2
	tst	%g1			! p->p_wchan must be 0
	bne	sw_bad
	nop
	cmp	%g2, SRUN		! p->p_stat must be SRUN
	bne	sw_bad
	nop
	clr	[%l4 + P_RLINK]		! p->p_rlink = 0

sw_stage_l4:			! common code (sw_goidle joins mainstream here)
	sethi	%hi(_masterprocp), %l1
	ld	[%l1 + %lo(_masterprocp)], %l2
	set	_cnt, %g1		! cnt.v_swtch++
	ld	[%g1 + V_SWTCH], %g2
	inc	1, %g2
	st	%g2, [%g1 + V_SWTCH]	! update ctx switch count

#ifdef	A_HAT_CPU
! update new procp->p_as->a_hat.hat_oncpu: now active here.
	ld	[%l4 + P_AS], %g4
	tst	%g4
	beq	1f			! null "as", skip onward.
	nop
	GETMID(%g3)			! get module id, 8..11 (or 15)
	stb	%g3, [%g4 + A_HAT_CPU]	! stash it in as->a_hat.hat_oncpu
1:
#endif	A_HAT_CPU

	cmp	%l2, %l4		! if (p == masterprocp) ...
	be	6f			!	skip resume() call
	nop
	call	_resume
	mov	%l4, %o0		! resume(p)
6:
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
#ifndef	MULTIPROCESSOR
	.global _masterprocp
_masterprocp:
	.word	0			! struct proc *masterprocp
#endif	MULTIPROCESSOR

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
	sethi	%hi(_nwindows), %o1
	ld	[%o1+%lo(_nwindows)], %o3
	dec	%o3			! get NW-1
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
	mov	%l0, %psr		! restore priority
	b	4f
	nop			! psr delay

	! store state for current proc
1:	sethi	%hi(_uunix), %g5	! XXX - global u register?
	ld	[%g5+%lo(_uunix)], %g5

	ld	[%g5 + PCB_FPCTXP], %l3 ! is floating point being used
	tst	%l3			! by the current process?
	bz	5f			! if not skip saving FPU state
	st	%l0, [%g5 + PCB_PSR]	! save psr (for PSR_PIL)

	sethi	%hi(_fpu_exists), %l4	! fpu is present flag
	ld	[%l4 + %lo(_fpu_exists)], %l4
	tst	%l4			! well, do we have FPU hardware?
	bz	5f			! no fpu, nothing to do
	nop
	st	%fsr, [%l3 + FPCTX_FSR] ! save current fpu state
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
	!
	sethi	%hi(_kadb_defer), %g1
	mov	1, %g2
	st	%g2, [%g1 + %lo(_kadb_defer)]
	sethi	%hi(_uunix), %g5
	ld	[%g5 + %lo(_uunix)], %g5
	st	%i7, [%g5 + PCB_PC]	! save ret pc and sp in pcb
	st	%fp, [%g5 + PCB_SP]

#ifdef	P_CPUID
	!
	! set the P_CPUID field in the new proc area.
	!
	GETCPU(%g2)
	st	%g2, [%i0 + P_CPUID]
#endif	P_CPUID

	!
	! switch to kernel context
	!
	call	_mmu_setctx
	mov	%g0, %o0

#ifdef	MULTIPROCESSOR
	! call hat_map_percpu so that the level-1 page table 
	! corresponding to the new uunix has its per-CPU area
	! set for the correct CPU.
	call	_hat_map_percpu
	ld	[%i0 + P_UAREA], %o0	! pass uarea as 1st parm
#endif	MULTIPROCESSOR

	!
	! Flush various write buffers before changing uunix
	! in order to facilitate asynchronous fault handling.
	call	_flush_writebuffers
	nop

	!
	! Switch to new user and proc areas.
	!

	ld	[%i0 + P_UAREA], %g5		! get new user ptr
	sethi	%hi(_uunix), %g2		! XXX - global uarea ptr?
	ld	[%l1 + %lo(_masterprocp)], %l6	! get old proc ptr
	st	%g5, [%g2+%lo(_uunix)]		! set new user ptr
	st	%i0, [%l1 + %lo(_masterprocp)]	! set new proc ptr

	!
	! We snarf the PC and SP here so we can end the critical
	! region that much sooner.
	!
	ld	[%g5 + PCB_PC], %i7
	ld	[%g5 + PCB_SP], %fp

	!
	! End critical region where we can't let BREAK call kadb.
	! (We just changed uunix, so if we take a BREAK we'll store the
	! registers for kadb on the new pcb, which isn't in use now).
	! So we clear the flag, and then check to see if we owe kadb a
	! call.
	!
	sethi	%hi(_kadb_defer), %g1	! clear defer first
	st	%g0, [%g1 + %lo(_kadb_defer)]
	sethi	%hi(_kadb_want), %g1	! then check want
	ld	[%g1 + %lo(_kadb_want)], %g2
	tst	%g2			! in case BREAK'd between
	bz	1f
	ld	[%i0 + P_AS], %l0	! check p->p_as

	call	_call_debug_from_asm
	st	%g0, [%g1 + %lo(_kadb_want)]
1:

	! If this process has an address space
	! and a context, get into it.

	tst	%l0			! if ((as = p->p_as) &&
	bz	1f
	nop

	ldub	[%l0 + 0x10], %g1
	andcc	%g1, 0x80, %g1		!     (as->a_hat.hat_ptvalid) &&
	bz	1f
	nop

	ld	[%l0 + A_HAT_CTX], %l0	!     (ctx = as->a_hat.hat_ctx))
	tst	%l0
	bz	1f
	nop

	call	_mmu_setctx		!	mmu_setctx(
	ld	[%l0 + C_NUM], %o0	!		   ctx->c_num);
1:

	!
	! %sp must be snarfed before we clear P_CPUID.
	!
	call	_flush_windows		! completely release the old stack
	sub	%fp, WINDOWSIZE, %sp	! establish sp, for interrupt overflow

#if defined(P_CPUID) || defined(A_HAT_CPU)
	!
	! If the old process was not null
	! and was different from the new one,
	! destage the old one.
	!
	tst	%l6
	be	1f			! if old proc was not null
	cmp	%l6, %i0
	be	1f			! and if it was a different one,
#ifdef	A_HAT_CPU
	!
	! If we are not sharing an addres space,
	! mark the old address space as "nowhere".
	!
	ld	[%l6 + P_AS], %g1
	tst	%g1
	beq	2f
	ld	[%i0 + P_AS], %g3
	cmp	%g1, %g3		! if old and new "as" are different,
	bne,a	2f			! (yes, branch if different!)
	stb	%g0, [%g1 + A_HAT_CPU]	!    set as->a_hat.hat_oncpu to "Nowhere"
2:
#endif	A_HAT_CPU
#ifdef	P_CPUID
	!
	! Make the old proc's P_CPUID field negative
	! so someone else can stage it.
	! NOTE: we leave the last bits alone so we can find
	! out where the process most recently ran.
	! NOTE: if registers get tight, this can be done
	! using an LDSTUB.
	!
	sub	%g0, 1, %g1
	stb	%g1, [%l6 + P_CPUID]	! yes, only store first byte.
#endif	P_CPUID
1:
#endif	/* P_CPUID || A_HAT_CPU */

	!
	! if the new process does not have any fp ctx
	!	we do nothing with regards to the fpu
	! if the new process does have fp ctx
	!	we enable the fpu and load its context now
	!	if we have fpu hardware
	!
	sethi	%hi(_uunix), %g5	! XXX - global uarea ptr?
	ld	[%g5+%lo(_uunix)], %g5	! reload user ptr

	ld	[%g5 + PCB_FPCTXP], %g1 ! new process fp ctx pointer
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

#ifdef MULTIPROCESSOR


	! If we are multiprocessor, do not check for the current loaded
	! fpu context (fp_ctxp). The process could have moved from processor
	! to processor. Therefore there will be no optimization of the fpu
	! context load in MP case.
	! 
	! We could running on a UP system such Viking but with a MP kernel.
	! In this case look for uniprocessor state. If so, then check for
	! possible fpu context load optimization. 

	sethi	%hi(_uniprocessor), %l6
	ld	[%l6+%lo(_uniprocessor)], %l6
	
	cmp	%l6, %g0
	bz	8f
	nop

#endif MULTIPROCESSOR

	cmp	%g1, %l1		! if the fp ctx belongs to new process
	be	3f			! nothing to do
	.empty				! the label below is OK
				! maybe branch to 2 instead
8:
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
	nop; nop; nop			! psr delay, (paranoid)
	ld	[%g1 + FPCTX_FSR], %fsr ! restore fsr
	LOAD_FPREGS(%g1)
3:
	ld	[%i0 + P_STACK], %l1
	add	%l1, (MINFRAME + PSR*4), %l1
	ld	[%l1], %l3		! read the psr value from the stack
	or	%l0, %l6, %l0		! enable fpu, new psr
	or	%l3, %l6, %g1		! enable fpu, psr stored in stack
	st	%g1, [%l1]		! update stack, sys_rtt will use it
2:
	!
	! Return to new proc. Restore will cause underflow.
	!
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
	jmp	%l5			! cons_child (...)
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
	ld	[%i0 + nPC*4], %o5	! skip trap instruction
	andn	%o4, %l2, %o4		! clear carry bit
	st	%o4, [%i1 + MINFRAME + PSR*4]
	st	%o5, [%i1 + MINFRAME + PC*4]	! pc = npc
	add	%o5, 4, %o5		! npc += 4
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
	mov	%i3, %o4		! parent pid childs ret val r0
	mov	1, %o5			! 1 childs ret value r1
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
	nop ; nop			! psr delay (paranoid)

#ifdef	P_CPUID
	!
	! set P_CPUID in new proc
	!
	GETCPU(%g2)
	st	%g2, [%i2 + P_CPUID]
#endif	P_CPUID

#ifdef	A_HAT_CPU
	!
	! If there is an associated AS,
	! update hat_oncpu.
	!
	ld	[%i2 + P_AS], %o1
	tst	%o1
	bz	1f			! if address space not null,
	nop
	GETMID(%g1)			! get module id, 8..11 or 15
	stb	%g1, [%o1 + A_HAT_CPU]	!   stash mid in as->a_hat.hat_oncpu
1:
#endif	A_HAT_CPU

#ifdef	MULTIPROCESSOR
	! call hat_map_percpu so that the level-1 page table 
	! corresponding to the new uunix has its per-CPU area
	! set for the correct CPU.
	call	_hat_map_percpu
	mov	%i4, %o0		! pass uarea as 1st parm
#endif	MULTIPROCESSOR

	!
	! Flush various write buffers before changing uunix
	! in order to facilitate asynchronous fault handling.
	!
	call	_flush_writebuffers
	nop

	!
	! change to new user and proc areas
	!
	sethi	%hi(_masterprocp), %o0
	sethi	%hi(_uunix), %o1		! XXX - global uarea pointer?
	ld	[%o0+%lo(_masterprocp)], %l5	! get old proc ptr
	st	%i4, [%o1+%lo(_uunix)]		! set new user ptr
	st	%i2, [%o0+%lo(_masterprocp)]	! set new proc ptr

	!
	! End critical region where we can't let BREAK call kadb.
	! (We just changed uunix, so if we take a BREAK we'll store the
	! registers for kadb on the new pcb, which isn't in use now).
	! So we clear the flag, and then check to see if we owe kadb a
	! call.
	!
	sethi	%hi(_kadb_defer), %g1	! clear defer first
	st	%g0, [%g1 + %lo(_kadb_defer)]
	sethi	%hi(_kadb_want), %g1	! then check want
	ld	[%g1 + %lo(_kadb_want)], %g2
	tst	%g2			! in case BREAK'd between
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
	tst	%l4
	mov	%g0, %o0		! default to context zero
	bz	1f			! if no address space, skip ahead
	ld	[%i1 + 6*4], %l6	! (delay) get proc start address

	ld	[%l4 + A_HAT_CTX], %l4	! check (p->p_as->a_hat.hat_ctx)
	tst	%l4
	bnz,a	1f			! if it has a context assigned,
	ld	[%l4 + C_NUM], %o0	!   get the context number
1:
	call	_mmu_setctx		! shift to child's context (or kernel's ...)
	mov	%i1, %fp		! (trailer) install the new childs stack

	!
	! %sp must be snarfed before we clear P_CPUID.
	!
	call	_flush_windows		! completely release the old stack
	sub	%fp, WINDOWSIZE, %sp	! establish sp, for interrupt overflow

#if defined(A_HAT_CPU) || defined(P_CPUID)
	!
	! Now, if the parent and child have different address spaces,
	! we can and should mark the parent's address space as
	! "nowhere" so the parent can be staged. Doing this before
	! switching away from the parent's context number is fatal.
	!
	tst	%l5			! if oldproc is null,
	beq	9f			! handle it gracefully.
	nop				! (should never happen)

	cmp	%i2, %l5		! if oldproc is "me",
	beq	9f			! handle it gracefully.
	nop				! (happens when creating proc 0)

#ifdef	A_HAT_CPU
	ld	[%i2+P_AS], %l1		! child address space
	ld	[%l5+P_AS], %l3		! parent address space

	tst	%l3			! if parent has an address space,
	beq	8f
	cmp	%l1, %l3		! and parent and child
	bne,a	8f			! are not sharing p_as,
	stb	%g0, [%l3+A_HAT_CPU]	!   mark parent AS as
8:					! "nowhere".
#endif	A_HAT_CPU

#ifdef	P_CPUID
	!
	! Make the old proc's P_CPUID field negative
	! so someone else can stage it.
	! NOTE: we leave the last bits alone so we can find
	! out where the process most recently ran.
	! NOTE: if registers get tight, this can be done
	! using an LDSTUB.
	!
	sub	%g0, 1, %l4
	stb	%l4, [%l5+P_CPUID]	! yes, only the first byte.
#endif	P_CPUID
9:
#endif	A_HAT_CPU || P_CPUID

	mov	%psr, %g1		! spl 0
	andn	%g1, PSR_PIL, %g1
	mov	%g1, %psr
	ld	[%i1], %i0		! %l0 has arg, set arg to new proc
	jmp	%l6
	restore				! restore causes underflow

/*
 * finishexit(vaddr)
 * Done with the current stack;
 * switch to the interrupt stack, segu_release, and swtch.
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

!
! If we are on the interrupt stack, panic.
!
#ifdef	MULTIPROCESSOR
	set	intstack, %o2
	cmp	%sp, %o2		! check for < intstack
	bl	0f			! if true, was not on int stk
	nop
#endif	MULTIPROCESSOR
	set	eintstack, %o3
	cmp	%sp, %o3		! check for > eintstack
	bg	0f			! if true, was not on int stk
	nop

	set	3f, %o0			! else, we were on int stack
	call	_panic			! so just go panic.
	nop
3:	.asciz	"finishexit"
	.align	4

0:
!
! Switch to the exit stack.
!
	set	exitstack, %o3
	sub	%o3, SA(MINFRAME), %sp	! switch to exit stack
	set	intu, %o2		! temporary uarea
	sethi	%hi(_masterprocp), %o5
	sethi	%hi(_uunix), %o1
	ld	[%o5+%lo(_masterprocp)], %l5	! get old proc ptr
	st	%o2, [%o1+%lo(_uunix)]		! set new user ptr
	st	%g0, [%o5+%lo(_masterprocp)]	! set new proc ptr

	mov	1, %o1
	sethi	%hi(_noproc), %o2		! no process now
	st	%o1, [%o2+%lo(_noproc)]

	call	_segu_release
	mov	%i0, %o0		! arg to segu_release

#if defined(P_CPUID) || defined(A_HAT_CPU)
	!
	! If the old proc was not null,
	! destage it.
	!
	tst	%l5
	be	9f
	nop
#ifdef	A_HAT_CPU
	!
	! If it had an AS, flag it as nowhere.
	!
	ld	[%l5 + P_AS], %g1
	tst	%g1
	bne,a	8f
	stb	%g0, [%g1 + A_HAT_CPU]
8:
#endif	A_HAT_CPU

#ifdef	P_CPUID
	!
	! Make the its P_CPUID field negative
	! so someone else can stage it.
	! NOTE: we leave the last bits alone so we can find
	! out where the process most recently ran.
	! NOTE: if registers get tight, this can be done
	! using an LDSTUB.
	!
	sub	%g0, 1, %g1
	stb	%g1, [%l5 + P_CPUID]
#endif	P_CPUID
9:
#endif	P_CPUID || A_HAT_CPU
	mov	%l4, %psr		! restore previous priority
	nop				! psr delay
	b	_swtch
	nop

/*
 * Setmmucntxt(procp)
 *
 * initialize mmu context for a process, if it has one
 */
	ENTRY(setmmucntxt)
	ld	[%o0 + P_AS], %o1
	tst	%o1
	bz	1f			! if proc has an address space,
	mov	%g0, %o0		! (delay) default context number is zero

	ld	[%o1 + A_HAT_CTX], %o1
	tst	%o1
	bz	1f			! and address space has a context,
	mov	%g0, %o0

	ld      [%o1 + C_NUM], %o0      ! get context number
1:
	b,a	_mmu_setctx		! change context number.
