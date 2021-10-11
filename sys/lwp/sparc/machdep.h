/* @(#) machdep.h 1.1 92/07/30 */
/* Copyright (C) 1987. Sun Microsystems, Inc. */
#ifndef __MACHDEP_H__
#define	__MACHDEP_H__

/*
 * XXX
 * The code here and in machdep.c currently has some grossness
 * due to compiler problems:
 * 1) double * references generate fp code. This wrecks the assumption
 * that the nugget doesn't use the fpa.
 * 2) += on a pointer to a fn casted to an int doesn't work correctly
 */

#include <machlwp/lwpmachdep.h>
#include <machlwp/stackdep.h>
#include <sun4/asm_linkage.h>

#define	STACKNULL	((stkalign_t *)0)

/*
 * Give the nugget a stack to use when no lwps are running
 * Flush is necessary in case we later remove stack, nugget
 * overflows its windows, and tries to flush.
 */
#define	SETSTACK() {							\
		extern void __setstack();				\
		__setstack();						\
}

#define NUGSTACKSIZE	(1024 * sizeof (stkalign_t))	/* stack for nugget */

/* handy access to context */
#define	mch_globals	lwp_machstate.sparc_globals.sparc_gdregs
#define	mch_oregs	lwp_machstate.sparc_oregs.sparc_odregs
#define	mch_xglobals	lwp_machstate.sparc_globals.sparc_gsregs
#define	mch_xoregs	lwp_machstate.sparc_oregs.sparc_osregs
#define	mch_g1		lwp_machstate.sparc_globals.sparc_gsregs[1]
#define	mch_o0		lwp_machstate.sparc_oregs.sparc_osregs[0]
#define	mch_psr		lwp_machstate.sparc_psr
#define	mch_pc		lwp_machstate.sparc_pc
#define	mch_y		lwp_machstate.sparc_y
#define	mch_sp		lwp_machstate.sparc_oregs.sparc_osregs[6]
#define	mch_npc		lwp_machstate.sparc_npc

/*
 * Do a context switch to the context defined by __Curproc.
 * AFTER the context is restored, an UNLOCK is done by __swtch().
 * Must be called while LOCKED.
 * __checkpoint() sets AllFlushed to TRUE since all windows are flushed
 * and we can context-switch without flushing windows.
 */
#define	CSWTCH() {							\
		__swtch();						\
	/* NOTREACHED */						\
}

/*
 * Give up the processor unconditionally to the next runnable lwp.
 * Leave __HighPrio alone; best guess is <= __HighPrio for next priority
 * lwp to schedule. When the process is eventually unblocked, WAIT returns.
 * WAIT returns with an UNLOCKED state.
 * WAIT() must be called while LOCKED.
 */
extern int runthreads;
#define	WAIT() {							\
	if (!runthreads)						\
		panic("lwp: bad wait");					\
	__checkpoint();							\
}

extern stkalign_t *__NuggetSP;	/* Stack for nugget when no lwps are running */
extern void __init_machdep();	/* init code for this module */
extern void __full_swtch();	/* context-switch to new process */

extern void __lowinit();	/* to store %i's into %o's and start thread */
#define	NARGREGS	6	/* 6 arguments are put in registers initially */
#define	FRAMEJUNK	7	/* assumed by compiler: for struct return
				   and register parameters.  */
#define	NSAVEDIREGS	8	/* number of %i's */
#define	NSAVEDLREGS	8	/* number of %l's */
#define	PC_OFFSET	15	/* offset from sp to saved pc (%i7) */
/*
 * Make the initial stack for a thread. sp is properly aligned initially
 * and points to the top of the stack (ready to start pushing args).
 * We make a frame that looks like:
 *
 *	HIGH ADDRESSES
 *	-------------------------
 *      | 16 words for		|
 *	| save area for lwpkill	|
 *	|-----------------------|
 *	| arguments past the 6th|
 *	|-----------------------|
 *	| FRAMEJUNK words 	|
 *	| assumed by compiler	|
 *	|-----------------------|
 *	| %i7 =	startaddr	|
 *	| %i6 =	sp		|
 *	| %i0-%i1 --> args 1-5	|
 *	|-----------------------|
 *	| 8 words for saved %l's|
 *	|-----------------------|	<--- sp(double aligned)
 *	LOW ADDRESSES
 *
 *	When switched in the first time, the i's are flushed
 *	from the save area to the %i's and __lowinit() is invoked.
 *	___lowinit() then simulates a call of startaddr by copying
 *	the %i's to the %o's and jumping to the address in %o7 (startaddr).
 *	The stack for the baby thread is the same as for lowinit, and
 *	the extra arguments are nicely in place.
 */
#define MAKE_STACK(cntxt, argp, sp, exitaddr, startaddr, nargs) {	\
	register int i;							\
	register int *s = (int *)sp;					\
	register int extra = 0;						\
									\
	s -= NSAVEDIREGS + NSAVEDLREGS;					\
	if (nargs > NARGREGS) {						\
		extra = nargs - NARGREGS;				\
		nargs = NARGREGS;					\
	}								\
	s -= (2 * SA(MINFRAME));					\
	cntxt->mch_sp = (int)(s);					\
	cntxt->mch_pc = (int)__lowinit;					\
	cntxt->mch_pc -= 8;						\
	s += NSAVEDLREGS;						\
	for (i = 0; i < NARGREGS; i++, s++) {				\
		if (nargs--)						\
			*s = va_arg(argp, int);				\
	}								\
	*s++ = cntxt->mch_sp + SA(MINFRAME);				\
	*s++ = (int)startaddr;						\
	s += FRAMEJUNK;							\
	for (i = 0; i < extra; i++) {					\
		*s++ = va_arg(argp, int);				\
	}								\
	LWPTRACE(tr_PROCESS,2,("MKSTK:lwp=0x%x, stk=0x%x, start at0x%x\n",	\
	    (long)cntxt, (long)s, (long)startaddr));			\
}

/*
 * Extra space (bytes) beyond client-specified allocation
 * needed for stack:
 * stack frame for the initial saved register window plus space for
 * any nugget primitives.  (On a bare machine or in the kernel, there might be a
 * separate stack for the nugget and the client.)
 * Useful for the CHECK macro in lwp.h since
 * it gives the client space to call another procedure when
 * CHECK just barely succeeds.
 * This number is just a guess.
 */
#define	STKOVERHEAD	NUGSTACKSIZE + (MAXARGS * sizeof(int))

/* stop everything */
#define	HALT()	{			\
	panic("lwp halt");		\
}

#include <machine/param.h>

#endif __MACHDEP_H__
