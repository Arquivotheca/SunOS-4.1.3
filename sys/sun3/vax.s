	.data
	.asciz	"@(#)vax.s 1.1 92/07/30 SMI"
	.even
	.text

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 *
 * Forked from version 1.42
 */

/*
 * Emulate VAX instructions on the 68020.
 */

#include <sys/param.h>
#include <machine/asm_linkage.h>
#include <machine/enable.h>
#include <machine/mmu.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include "fpa.h"
#include "assym.s"

.globl _uunix 

/*
 * Macro to raise prio level,
 * avoid dropping prio if already at high level.
 * NOTE - Assumes that we are never in "master" mode.
 */
#define	RAISE(level)	\
	movw	sr,d0;	\
	andw	#(SR_SMODE+SR_INTPRI),d0; 	\
	cmpw	#(SR_SMODE+(/**/level*0x100)),d0;	\
	jge	0f;	\
	movw	#(SR_SMODE+(/**/level*0x100)),sr;	\
0:	rts

#define	SETPRI(level)	\
	movw	sr,d0;	\
	movw	#(SR_SMODE+(/**/level*0x100)),sr;	\
	rts

	ENTRY(splimp)
	RAISE(3)

	ENTRY(splnet)
	RAISE(1)

	ENTRY(splclock)
	RAISE(5)

	ENTRY(splzs)
	SETPRI(6)

	ENTRY(spl7)
	SETPRI(7)

	ALTENTRY(spltty)
	ALTENTRY(splbio)
	ALTENTRY(splhigh)
	ENTRY2(spl6,spl5)
	SETPRI(5)

	ENTRY(spl4)
	SETPRI(4)

	ENTRY(spl3)
	SETPRI(3)

	ENTRY(spl2)
	SETPRI(2)

	ENTRY2(spl1,splsoftclock)
	SETPRI(1)

	ENTRY(spl0)
	SETPRI(0)

	ENTRY(splx)
	movw	sr,d0
	movw	sp@(6),sr
	rts

/*
 * splr is like splx but will only raise the priority and never drop it
 */
	ENTRY(splr)
	movw	sr,d0
	andw	#(SR_SMODE+SR_INTPRI),d0
	movw	sp@(6),d1
	andw	#(SR_SMODE+SR_INTPRI),d1
	cmpw	d1,d0
	jge	0f
	movw	d1,sr
0:
	rts

/*
 * splvm will raise the processor priority to the value implied by splvm_val
 * which is initialized by the autoconfiguration code to pritospl(SPLMB).
 */
	.globl	_splvm_val
	ENTRY(splvm)
	movw	sr,d0
	andw	#(SR_SMODE+SR_INTPRI),d0
	movw	_splvm_val,d1
	cmpw	d1,d0
	jge	0f
	movw	d1,sr
0:
	rts

	ENTRY(_insque)
	movl	sp@(4),a0
	movl	sp@(8),a1
	movl	a1@,a0@
	movl	a1,a0@(4)
	movl	a0,a1@
	movl	a0@,a1
	movl	a0,a1@(4)
	rts

	ENTRY(_remque)
	movl	sp@(4),a0
	movl	a0@,a1
	movl	a0@(4),a0
	movl	a1,a0@
	movl	a0,a1@(4)
	rts

	ENTRY(scanc)
	movl	sp@(8),a0	| string
	movl	sp@(12),a1	| table
	movl	sp@(16),d1	| mask
	movl	d2,sp@-
	clrw	d2
	movl	sp@(8),d0	| len
	subqw	#1,d0		| subtract one for dbxx
	jmi	1f
2:
	movb	a0@+,d2		| get the byte from the string
	movb	a1@(0,d2:w),d2	| get the corresponding table entry
	andb	d1,d2		| apply the mask
	dbne	d0,2b		| check for loop termination
1:
	addqw	#1,d0		| dbxx off by one
	movl	sp@+,d2
	rts

	ENTRY(movtuc)
	movl	a2,sp@-
	movl	sp@(12),a0	| from
	movl	sp@(16),a1	| to
	movl	sp@(20),a2	| table
	clrw	d1
	movl	sp@(8),d0	| len
	subqw	#1,d0		| subtract one for dbxx
	jmi	1f
2:
	movb	a0@+,d1		| get the byte from the first string
	movb	a2@(0,d1:w),d1	| get the corresponding table entry
	movb	d1,a1@+		| store it
	dbeq	d0,2b		| check for loop termination
	bne	1f		| terminated because the count ran out
	subql	#1,a1		| moved one too many bytes
1:
	movl	a1,d0		| final pointer minus original pointer
	subl	sp@(16),d0	| is number of characters moved
	movl	sp@+,a2
	rts

/*
 * _whichqs tells which of the 32 queues _qs have processes in them
 * setrq puts processes into queues, remrq removes them from queues
 * The running process is on no queue, other processes are on a
 * queue related to p->p_pri, divided by 4 actually to shrink the
 * 0-127 range of priorities into the 32 available queues.
 */

/*
 * setrq(p), using semi-fancy 68020 instructions.
 *
 * Call should be made at spl6(), and p->p_stat should be SRUN
 */
	ENTRY(setrq)
	movl	sp@(4),a0	| get proc pointer
	tstl	a0@(P_RLINK)	| firewall: p->p_rlink must be 0
	jne	1f
	moveq	#31,d1
	clrl	d0
	movb	a0@(P_PRI),d0	| get the priority
	lsrl	#2,d0		| divide by 4
	subl	d0,d1
	lea	_qs,a1
	movl	a1@(4,d1:l:8),a1| qs[d1].ph_rlink, 8 == sizeof (qs[0])
	movl	a1@,a0@		| insque(p, blah)
	movl	a1,a0@(4)
	movl	a0,a1@
	movl	a0@,a1
	movl	a0,a1@(4)
	bfset	_whichqs{d0:#1}	| set appropriate bit in whichqs
	rts
1:
	pea	2f
	jsr	_panic

2:	.asciz	"setrq"
	.even

/*
 * remrq(p), using semi-fancy 68020 instructions
 *
 * Call should be made at spl6().
 */
	ENTRY(remrq)
	movl	sp@(4),a0
	clrl	d0
	movb	a0@(P_PRI),d0
	lsrl	#2,d0		| divide by 4
	bftst	_whichqs{d0:#1}
	jeq	1f
	movl	a0@,a1		| remque(p);
	movl	a0@(4),a0
	movl	a1,a0@
	movl	a0,a1@(4)
	cmpl	a0,a1
	jne	2f		| queue not empty
	bfclr	_whichqs{d0:#1}	| queue empty, clear the bit
2:
	movl	sp@(4),a0
	clrl	a0@(P_RLINK)
	rts
1:
	pea	3f
	jsr	_panic

3:	.asciz	"remrq"
	.even

/*
 * swtch(), using semi-fancy 68020 instructions
 * Only for heavyweight processes.
 */
	.globl	_qrunflag, _runqueues
	ENTRY(swtch)
	movw	sr,sp@-		| save processor priority
	movl	#1,_noproc
	clrl	_runrun
	movl	_uunix, a0
	bclr	#AST_SCHED_BIT-24, a0@(PCB_FLAGS)
2:
	bfffo	_whichqs{#0:#32},d0 | test if any bit on
	cmpl	#32,d0
	jne	3f		| found one
	tstb	_qrunflag	| need to run stream queues?
	beq	1f		| no
	movw	#SR_LOW,sr	| run queues at priority 0
	jsr	_runqueues	| go do it
	jra	2b		| scan run queue immediately
1:
#ifdef LWP
	.globl ___Nrunnable, _lwpschedule

	tstl	___Nrunnable		| any runnable threads?
	jeq	0f			| no
	movw	#SR_LOW,sr		| run queues at priority 0
	jsr	_lwpschedule
	movl	_masterprocp, sp@-
	jsr	_setmmucntxt
	addqw	#4,sp
	jra	2b
0:
#endif LWP
	stop	#SR_LOW		| must allow interrupts here

	.globl	idle
idle:				| wait here for interrupts
	jra	2b		| try again

3:
	movw	#SR_HIGH,sr	| lock out all so _whichqs == _qs
	moveq	#31,d1
	subl	d0,d1
	bfclr	_whichqs{d0:#1}
	jeq	2b		| proc moved via lbolt interrupt
	lea	_qs,a0
	movl	a0@(0,d1:l:8),a0| get qs[d1].ph_link = p = highest pri process
	movl	a0,d1		| save it
	movl	a0@,a1		| remque(p);
	cmpl	a0,a1		| is queue empty?
	jne	4f
8:
	pea	9f
	jsr	_panic
9:
	.asciz	"swtch"
	.even
4:
	movl	a0@(4),a0
	movl	a1,a0@
	movl	a0,a1@(4)
	cmpl	a0,a1
	jeq	5f		| queue empty
	bfset	_whichqs{d0:#1}	| queue not empty, set appropriate bit
5:
	movl	d1,a0		| restore p
	clrl	_noproc
	tstl	a0@(P_WCHAN)	|| firewalls
	jne	8b		||
	cmpb	#SRUN,a0@(P_STAT) ||
	jne	8b		||
	clrl	a0@(P_RLINK)	||
	addql	#1,_cnt+V_SWTCH
	cmpl	_masterprocp,a0	| restore of current proc is easy
	jeq	6f
	movl	a0,sp@-
	jsr	_resume
	addqw	#4,sp
6:
	movw	sp@+,sr		| restore processor priority
	rts


/*
 * fpprocp is the pointer to the proc structure whose external
 * state (i.e. registers) was last loaded into the 68881.
 * XXX - temporarily this optimization is not used until
 * we have per process enabling and disabling of '81 since
 * an frestore of null state resets the external '81 state.
 */
	.data
	.globl	_fpprocp
_fpprocp:	.long 0			| struct proc *fpprocp;
	.text

/*
 * masterprocp is the pointer to the proc structure for the currently
 * mapped u area.  It is used to set up the mapping for the u area
 * by the debugger since the u area is not in the Sysmap.
 */
	.data
	.globl	_masterprocp
_masterprocp:	.long	0		| struct proc *masterprocp;
	.text

/*
 * resume(p)
 *
 * Saves all the needed register state into the "old" user area given
 * by uunix and then restores all the register state from the user
 * area given by process p and sets uunix to point to that u area.
 * Resume will also try to set up the address space for the process.
 */
#define	SAVREGS		d2-d7/a2-a7

	ENTRY(resume)
	movl	_uunix, a0
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
	movl	_uunix, a0		| restore a0
	movl	d0,a2@(P_RSSIZE)	| store with process
1:
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
	jra	3f			| skip reg save
2:
	moveml	a2@(FPA_STATE),SAVEFPA	| load 12 FPA regs
	moveml	SAVEFPA,a0@(U_FPA_STATUS) | save to u.u_fpa_status
3:
	andb	#~ENA_FPA,_enablereg	| turn off FPA enable bit so FPA is
	movb	_enablereg,d0		| protected from unexpected accesses
	movsb	d0,ENABLEREG
1:
#endif NFPA > 0

	movl	sp@(4),a2		| a2 contains proc pointer
	orw	#SR_INTPRI,sr		| ok, now mask interrupts
					| since must hve atomic set of
					| masterprocp and uunix
	movl	a2,_masterprocp		| set proc pointer for new process
	lea	eintstack,sp		| use the interrupt stack
	movl	a2@(P_UAREA), _uunix	| switch u areas

/*
 * Check to see if we already have context.  If so, then set up
 * to use the context otherwise to default to the kernel context.
 */
	movl	a2@(P_AS),d0		| get p->p_as
	jeq	0f			| skip ahead if NULL (kernel process)
	movl	d0,a0
	movl	a0@(A_HAT_CTX),d0	| get as->a_hat.hat_ctx
	jeq	0f			| skip ahead if NULL (no ctx allocated)
	movl	d0,a0
	movw	_ctx_time,a0@(C_TIME)
	addqw	#1,_ctx_time		| ctx->c_time = ctx_time++;
	movb	a0@(C_NUM),d0		| get ctx->c_num
	bra	1f
0:
	moveq	#KCONTEXT,d0		| no context - use kernel context
1:
	movsb	d0,CONTEXT_REG		| switch context

	movl	_uunix, a0
	tstw	_fppstate		| is fpp present and enabled?
	jle	1f			| branch past fpp and fpa code if not
	tstb	a0@(U_FP_ISTATE)	| null state? (vers == FPIS_VERSNULL)
	jeq	0f			| branch if so
| XXX - fpprocp not used until we get rid of frestores with null state
|	cmpl	_fpprocp,a2		| check if we were last proc using fpp
|	beq	0f			| if so, jump and skip loading ext regs
	fmovem	a0@(U_FPS_REGS),fp0-fp7	| restore fp data registers
	fmovem	a0@(U_FPS_CTRL),fpc/fps/fpi | restore control registers
0:
	frestore a0@(U_FP_ISTATE)	| restore internal state

#if NFPA > 0
	tstw	a0@(U_FPA_FLAGS)	| test if this process uses FPA
	jeq	1f			| no, skip restoring FPA context
	movl	_uunix, sp@-
	jsr	_fpa_restore		| yes, restore FPA context
	addqw	#4, sp
	movl	_uunix, a0
#endif NFPA > 0
1:

/*
 * a0 points to current u area.  u area and kernel stack
 * should already be set before resume() was called.
 */
	moveml	a0@(PCB_REGS+4),SAVREGS	| restore data/address regs
/*
 * Note:  we just changed stacks from the interrupt stack
 * we were temporarily using to the new process stack!
 */
	movw	a0@(PCB_SR+2),sr
	rts
