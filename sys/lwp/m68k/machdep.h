/* @(#) machdep.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */
#ifndef __MACHDEP_H__
#define __MACHDEP_H__

#include <machlwp/lwpmachdep.h>
#include <machlwp/stackdep.h>

#define	STACKNULL	((stkalign_t *)0)

/* give the nugget a stack to use when no lwps are running */
#define	SETSTACK() 	__setstack()

#define	NUGSTACKSIZE	4000		/* stack size (bytes) for nugget */

#define	lwp_context	lwp_machstate.mc_generals
#define	lwp_temps	lwp_machstate.mc_temps

/* offsets to reach various nonvolatile regs in lwp_context */
#define	A6	10
#define	LSP	11		/* offset to reach stack */

/* offsets to reach various volatile regs in lwp_temps */
#define	TEMPPC	4
#define	TEMPPS	5

/*
 * Do a context switch to the context defined by __Curproc.
 */
#define	CSWTCH() {							\
	__swtch();							\
	/* NOTREACHED */						\
}

/*
 * Give up the processor unconditionally to the next runnable lwp.
 * Leave __HighPrio alone; best guess is <= __HighPrio for next priority
 * lwp to schedule. When the process is eventually unblocked, WAIT returns.
 */
extern int runthreads;
#define	WAIT() {							\
	if (!runthreads)						\
		panic("lwp: bad wait");					\
	__checkpoint();							\
}

extern stkalign_t *__NuggetSP;	/* Stack for nugget when no lwps are running */
extern void __init_machdep();	/* init code for this module */

/*
 * sp points to top (just past the highest legal stack address) of stack.
 * We start by doing a push, so the top address is not used
 * (that is, only addresses strictly below the stacktop are usable for stack).
 * Leave the stack where it's supposed to start at
 */
#define	MAKE_STACK(cntxt, argp, sp, exitaddr, startaddr, nargs) {	\
	register int i;							\
	register int *spp = (int *) sp;					\
									\
	spp -= nargs + 1;						\
	for (i = 0; i < nargs; i++) {					\
		*(++spp) = va_arg(argp, int);				\
	}								\
	spp -= nargs;							\
	*spp = (int) exitaddr;						\
	*--(spp) = (int) startaddr;					\
	cntxt->lwp_context[LSP] = (int) spp;				\
}

/*
 * Extra space (bytes) beyond client-specified allocation
 * needed for stack initialization:
 * exitaddr + startaddr + space for any nugget primitives. On a bare machine,
 * there might be a separate stack for the nugget and the client.)
 * Useful for the CHECK macro in lwp.h since
 * it gives the client space to call another procedure when
 * CHECK just barely succeeds.
 * This number is just a guess.
 */
#define	STKOVERHEAD	(NUGSTACKSIZE + (MAXARGS * sizeof(int)))

/* stop everything */
#define HALT()		asm("stop #0")		/* could go back to prom */

#include <machine/param.h>

#endif __MACHDEP_H__
