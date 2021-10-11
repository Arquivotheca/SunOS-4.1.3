/* @(#)machdep.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */
/*
 * This source code is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify this source code without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * THIS PROGRAM CONTAINS SOURCE CODE COPYRIGHTED BY SUN MICROSYSTEMS, INC.
 * AND IS LICENSED TO SUNSOFT, INC., A SUBSIDIARY OF SUN MICROSYSTEMS, INC.
 * SUN MICROSYSTEMS, INC., MAKES NO REPRESENTATIONS ABOUT THE SUITABLITY
 * OF SUCH SOURCE CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT
 * EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  SUN MICROSYSTEMS, INC. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO SUCH SOURCE CODE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN
 * NO EVENT SHALL SUN MICROSYSTEMS, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT,
 * INCIDENTAL, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM USE OF SUCH SOURCE CODE, REGARDLESS OF THE THEORY OF LIABILITY.
 * 
 * This source code is provided with no support and without any obligation on
 * the part of Sun Microsystems, Inc. to assist in its use, correction, 
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS
 * SOURCE CODE OR ANY PART THEREOF.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California 94043
 */
#ifndef _lwp_machdep_h
#define _lwp_machdep_h

#include "lwpmachdep1.h"
#include "stackdep.h"

#define	STACKNULL	((stkalign_t *)0)
#define INTSTKSZ	(MINSIGSTKSZ + 1000) /* intr. stack (stkalign_t's) */

/* give the nugget a stack to use when no lwps are running */
#define	SETSTACK() {asm("movl ___NuggetSP, sp");}

#define	NUGSTACKSIZE	4000		/* stack size (bytes) for nugget */
#define	IDLESTACK	1000		/* stack size for idle thread */
extern stkalign_t __IdleStack[];	/* stack for idle thread */

#define	lwp_context	lwp_machstate.mc_generals
#define	lwp_temps	lwp_machstate.mc_temps

/* offsets to reach various nonvolatile regs in lwp_context */
#define	A6	10
#define	SP	11		/* offset to reach stack */

/* offsets to reach various volatile regs in lwp_temps */
#define	TEMPPC	4
#define	TEMPPS	5

/*
 * Do a context switch to the context defined by __Curproc.
 * AFTER the context is restored, an UNLOCK is done by __swtch().
 * Must be called while LOCKED.
 */
#define	CSWTCH() {							\
	__swtch();							\
	/* NOTREACHED */						\
}

/*
 * Give up the processor unconditionally to the next runnable lwp.
 * Leave __HighPrio alone; best guess is <= __HighPrio for next priority
 * lwp to schedule. When the process is eventually unblocked, WAIT returns.
 * WAIT returns with an UNLOCKED state.
 * WAIT() must be called while LOCKED.
 */
#define	WAIT() {							\
	ASSERT(IS_LOCKED());						\
	__checkpoint();							\
}

extern stkalign_t *__NuggetSP;	/* Stack for nugget when no lwps are running */
extern void __init_machdep();	/* init code for this module */
extern int __real_sig();	/* handle interrupt */
extern void __full_swtch();	/* context-switch to new process */

/*
 * sp points to top (just past the highest legal stack address) of stack.
 * We start by doing a push, so the top address is not used
 * (that is, only addresses strictly below the stacktop are usable for stack).
 * Leave the stack where it's supposed to start at
 */
#define	MAKE_STACK(cntxt, argp, sp, exitaddr, startaddr, nargs) {	\
	register int i;							\
									\
	(int *)sp -= nargs + 1;						\
	for (i = 0; i < nargs; i++) {					\
		*(++((int *)sp)) = va_arg(argp, int);			\
	}								\
	(int *)sp -= nargs;						\
	*(int *)sp = (int) exitaddr;					\
	*--((int *)sp) = (int) startaddr;				\
	cntxt->lwp_context[SP] = (int)sp;				\
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

extern void __idle();   	/* lwp to wait for interrupts */
extern int __sigtrap();		/* handle synchronous traps */

/* stop everything */
#define	HALT()	{			\
	_exit(pod_getexit());		\
}

#endif /*!_lwp_machdep_h*/
