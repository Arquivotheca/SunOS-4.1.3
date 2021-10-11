	.data
	.asciz	"@(#)vax.s 1.1 92/07/30 Copyr 1985 Sun Micro"
	.even
	.text
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Emulate VAX instructions on the 68020.
 */

#ifdef	NEVER
#include <sys/param.h>
#endif	/* NEVER */
#include <machine/asm_linkage.h>
#ifdef	NEVER
#include <machine/enable.h>
#include <machine/mmu.h>
#endif	/* NEVER */
#include <machine/psl.h>
#ifdef	NEVER
#include "fpa.h"
#include "assym.s"
#endif	/* NEVER */
#include <sys/errno.h>

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
	RAISE(0)

	ENTRY(splie)
	RAISE(3)

	ENTRY(splclock)
	RAISE(5)

	ENTRY(splzs)
	SETPRI(6)

	ENTRY(spl7)
	SETPRI(7)

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

#ifdef	NEVER
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
 */
#ifdef STREAMS
	.globl	_qrunflag, _runqueues
#endif
	ENTRY(swtch)
	movw	sr,sp@-		| save processor priority
	movl	#1,_noproc
	clrl	_runrun
	bclr	#AST_SCHED_BIT-24,_u+PCB_P0LR:w
2:
	bfffo	_whichqs{#0:#32},d0 | test if any bit on
	cmpl	#32,d0
	jne	3f		| found one
#ifdef STREAMS
	tstb	_qrunflag	| need to run stream queues?
	beq	1f		| no
	movw	#SR_LOW,sr	| run queues at priority 0
	jsr	_runqueues	| go do it
	jra	2b		| scan run queue immediately
1:
#endif
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
	movl	a0,sp@-
	jsr	_resume
	addqw	#4,sp
	movw	sp@+,sr		| restore processor priority
	rts


/*
 * fpprocp is the pointer to the proc structure whose external
 * state (i.e. registers) was last loaded into the 68881.
 * XXX - temporary this is optimization is not used until
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
 * Assumes that there is only one real page to worry about (UPAGES = 1),
 * that the kernel stack starts below the u area in the middle of
 * a page, that the redzone is below that and is always marked
 * for no access across all contexts, the default sfc and dfs are
 * set to FC_MAP, and that we are running on the u area kernel stack
 * when we are called.
 */
#define	SAVREGS		d2-d7/a2-a7

	ENTRY(resume)
	tstw	_fppstate		| is fpp present and enabled?
	jle	1f			| branch if not
	fsave	_u+U_FP_ISTATE:w	| save internal state
	tstb	_u+U_FP_ISTATE:w	| null state? (vers == FPIS_VERSNULL)
	jeq	1f			| branch if so
	fmovem	fpc/fps/fpi,_u+U_FPS_CTRL:w | save control registers
	fmovem	fp0-fp7,_u+U_FPS_REGS:w	| save fp data registers
| XXX - fpprocp not used until we get rid of frestores with null state
|	movl	_masterprocp,_fpprocp	| remember whose regs are still loaded
1:
	movl	sp@,_u+PCB_REGS:w	| save return pc in pcb
	movw	sr,_u+PCB_SR+2:w	| save the current psw in u area
	moveml	SAVREGS,_u+PCB_REGS+4:w	| save data/address regs
#if NFPA > 0
/*
 * fpa_save() is inlined here to moveml 12 regs in two instructions.
 * We use a2 to contain the fpa address so that it is preserved across
 * the call to fpa_shutdown and we pick out SAVEFPA regs so that a2
 * is not used and we have a total of 12 regs.
 */
#define	SAVEFPA		d0-d7/a3-a6
#define	FPA_MAXLOOP	512

	tstw	_u+U_FPA_FLAGS:w	| test if this process uses FPA
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
	moveml	SAVEFPA,_u+U_FPA_STATUS:w | save to u.u_fpa_status
3:
	andb	#~ENA_FPA,_enablereg	| turn off FPA enable bit so FPA is
	movb	_enablereg,d0		| protected from unexpected accesses
	movsb	d0,ENABLEREG
1:
#endif NFPA > 0
	cmpl	#_u,sp			| check to see if sp is above u area
	jhi	1f			| jmp if so
	pea	0f			| panic - we tried to switch while
	jsr	_panic			|   on the overflow stack area
	/* NOTREACHED */
	.data
0:	.asciz	"kernel stack overflow"
	.even
	.text
1:
	movl	sp@(4),a2		| a2 contains proc pointer
	movl	a2,_masterprocp		| set proc pointer for new process
	orw	#SR_INTPRI,sr		| ok, now mask interrupts
	lea	eintstack,sp		| use the interrupt stack
#ifdef SUN3_260
	movl	#USIZE,sp@-		| push u size
	pea	_u			| push virtual addr of _u
	jsr	_vac_flush		| flush struct user
					| skip pop of args
#endif SUN3_260
	moveq	#KCONTEXT,d0
	movsb	d0,CONTEXTBASE		| invalidate context
	movl	a2@(P_ADDR),a0		| get p_addr (address of pte)
	movl	a0@,d0			| get p_addr[0] (the u pte)
	movsl	d0,U_MAPVAL		|   and set pme using precalculated addr
/*
 * Check to see if we already have context.  If so and
 * SPTECHG bit is not on then set up the next context.
 */
	tstl	a2@(P_CTX)		| check p->p_ctx
	jeq	1f			| if zero, skip ahead
	btst	#SPTECHG_BIT-24,a2@(P_FLAG)	| check (p->p_flag & SPTECHG)
	jne	1f			| if SPTECHG bit is on, skip ahead
	movl	a2@(P_CTX),a1
	movw	a1@(CTX_CONTEXT),d0	| get context number in d0
	movsb	d0,CONTEXTBASE		| set up the context
	addql	#1,_ctxtime		| Increment context clock
	movl	_ctxtime,a1@(CTX_TIME)	| Set context LRU to updated clock
1:

	tstw	_fppstate		| is fpp present and enabled?
	jle	1f			| branch past fpp and fpa code if not
	tstb	_u+U_FP_ISTATE:w	| null state? (vers == FPIS_VERSNULL)
	jeq	0f			| branch if so
| XXX - fpprocp not used until we get rid of frestores with null state
|	cmpl	_fpprocp,a2		| check if we were last proc using fpp
|	beq	0f			| if so, jump and skip loading ext regs
	fmovem	_u+U_FPS_REGS:w,fp0-fp7	| restore fp data registers
	fmovem	_u+U_FPS_CTRL:w,fpc/fps/fpi | restore control registers
0:
	frestore _u+U_FP_ISTATE:w	| restore internal state

#if NFPA > 0
	tstw	_u+U_FPA_FLAGS:w	| test if this process uses FPA
	jeq	1f			| no, skip restoring FPA context
	jsr	_fpa_restore		| yes, restore FPA context
#endif NFPA > 0
1:

	moveml	_u+PCB_REGS+4:w,SAVREGS	| restore data/address regs
| Note: we just changed stacks
	movw	_u+PCB_SR+2:w,sr
	tstl	_u+PCB_SSWAP:w
	jeq	1f
	movl	_u+PCB_SSWAP:w,sp@(4)	| blech...
	clrl	_u+PCB_SSWAP:w
	movw	#SR_LOW,sr
	jmp	_longjmp
1:
	rts
#endif	/* NEVER */

/*
 * Copy a null terminated string from one point to another in
 * the kernel address space.
 *
 * copystr(kfaddr, kdaddr, maxlength, lencopied)
 *      caddr_t kfaddr, kdaddr;
 *      u_int   maxlength, *lencopied;
 */
        ENTRY(copystr)
        movl    sp@(4),a0                       | a0 = kfaddr
        movl    sp@(8),a1                       | a1 = kdaddr
        movl    sp@(12),d0                      | d0 = maxlength
        jlt     copystrfault                    | if negative size fault
        jra     1f                              | enter loop at bottom
0:
        movb    a0@+,a1@+                       | move a byte and set CCs
        jeq     copystrok                       | if '\0' done
1:
        dbra    d0,0b                           | decrement and loop
        jra     copystrout                      | ran out of space

copystrout:
        movl    #ENAMETOOLONG,d0                | ran out of space
        jra     copystrexit

copystrfault:
        movl    #EFAULT,d0                      | memory fault or bad size
        jra     copystrexit

copystrok:
        moveq   #0,d0                           | all ok

/*
 * At this point we have:
 *      d0 = return value
 *      a0 = final address of `from' string
 *      sp@(4) = starting address of `from' string
 *      sp@(16) = lencopied pointer (NULL if nothing to be return here)
 */
copystrexit:
#ifdef	NEVER
        clrl    _u+U_LOFAULT                    | clear lofault
#endif	/* NEVER */
        movl    sp@(16),d1                      | d1 = lencopied
        jeq     2f                              | skip if lencopied == NULL
        subl    sp@(4),a0                       | compute nbytes moved
        movl    d1,a1
        movl    a0,a1@                          | store into *lencopied
2:
        rts
