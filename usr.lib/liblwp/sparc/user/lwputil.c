/* Copyright (C) 1987 Sun Microsystems Inc. */
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
#include "common.h"
#include "queue.h"
#include "asynch.h"
#include "machsig.h"
#include "machdep.h"
#include "cntxt.h"
#include "lwperror.h"
#include "message.h"
#include "process.h"
#include "monitor.h"
#include "schedule.h"
#ifndef lint
SCCSID(@(#) lwputil.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

#ifndef STAND
#include <stdio.h>
#endif STAND

/*
 * PRIMITIVES contained herein:
 * debug_nugget(argc, argv) (* not publicly available *)
 * lwp_fpset(tid) -- enable fpa context switching for a given lwp
 * lwp_libcset(tid) -- enable libc context switching for a given lwp
 * lwp_checkstkset(tid, stackbottom)
 * lwp_stkcswset(tid, stackbottom)
 */

int __Trace[MAXFLAGS];	/* trace flags */

/* sparc floating point registers */
typedef struct sparcfpp_ctxt_t {
	double sparcfpp_regs[16];	/* fp regs -- MUST BE DOUBLE ALIGNED */
	long sparcfpp_fsr;		/* floating point status register */
} sparcfpp_ctxt_t;
STATIC int SparcfppCookie;

/* C library globals */
typedef struct libc_ctxt_t {
	int libc_errno;
} libc_ctxt_t;
STATIC int LibcCtx;

/* For stack checking on context switch */
typedef struct stkchk_ctxt_t {
	stkalign_t *stkchk_stack;		/* bottom of stack */
} stkchk_ctxt_t;
STATIC int StkCtx;

/* for CHECK macro support */
STATIC int ChkstkCtx;
stkalign_t *__CurStack;	/* global indicating stack bottom for current thread */

/* create special context to support CHECK macro */
void
lwp_chkstkenable()
{
	extern void __chkstk_restore();

	ChkstkCtx = lwp_ctxset(VFUNCNULL, __chkstk_restore,
	  sizeof (stkchk_ctxt_t), TRUE);
}

/*
 * lwp_checkstkset -- PRIMITIVE.
 * Enable CHECK macro checking by
 * keeping track of current stack limit in global __CurStack.
 */
int
lwp_checkstkset(lwp, stackbottom)
	thread_t lwp;
	caddr_t stackbottom;
{

	stkchk_ctxt_t cntxt;
	thread_t me;

	cntxt.stkchk_stack = (stkalign_t *)stackbottom;
	(void)lwp_ctxinit(lwp, ChkstkCtx);
	(void)lwp_ctxmemset((caddr_t)&cntxt, lwp, ChkstkCtx);
	(void)lwp_self(&me);
	if SAMETHREAD(me, lwp) {
		__CurStack = (stkalign_t *)stackbottom;
	}
	return (0);
}

/* restore __CurStack for current thread so CHECK can work */
void
__chkstk_restore(cntxt, old, new)
	stkchk_ctxt_t *cntxt;
	thread_t old;
	thread_t new;
{
#ifdef lint
	old = old;
	new = new;
#endif lint

	__CurStack = cntxt->stkchk_stack;
}

/* enable stack checking special contexts */
void
lwp_stkchkenable()
{
	extern void __stkchk_save();

	StkCtx = lwp_ctxset(__stkchk_save, VFUNCNULL,
	  sizeof (stkchk_ctxt_t), FALSE);
}

/*
 * lwp_stkcswset -- PRIMITIVE.
 * set a lwp to do stack checking on context switch
 */
int
lwp_stkcswset(lwp, stackbottom)
	thread_t lwp;
	caddr_t stackbottom;
{
	stkchk_ctxt_t cntxt;

	cntxt.stkchk_stack = (stkalign_t *)stackbottom;
	(void)lwp_ctxinit(lwp, StkCtx);
	(void)lwp_ctxmemset((caddr_t)&cntxt, lwp, StkCtx);
	return (0);
}

/*
 * When this thread is being switched out, we check its stack.
 * We know that redzones are 3 ints of zeroes.
 * Procedure calls are to non-zero
 * addresses so it is unlikely that consecutive zeroes will be found
 * on the stack unless we manage to run into a procedure that
 * initializes a lot of local variables to zero.
 * Probably a better check would involve writing
 * a pattern (say, 0, 1, 13).
 */
void
__stkchk_save(cntxt, old, new)
	stkchk_ctxt_t *cntxt;
	thread_t old;
	thread_t new;
{
	register int *k;
#ifdef lint
	new = new;
#endif lint

	k = (int *)(cntxt->stkchk_stack);
	if ((*k++ != 0) || (*k++ != 0) || (*k++ != 0)) {
		__panic("stack overwritten");
	}

	/*
	 * We should really use a lwp_getregs to obtain the
	 * actual stack pointer, but this is faster.
	 */
	if ((int *)((lwp_t *)old.thread_id)->mch_sp < k) {
		__panic("stack overflow"); 
	}

}

/*
 * set up a sparc floating point special context
 * XXX not optimised yet -- need to be able to handle exceptions
 * when flushing queue.
 */
void
lwp_sparcfppenable()
{
	extern void __sparcfpp_save();		/* in low.s */
	extern void __sparcfpp_restore();	/* in low.s */

	SparcfppCookie = lwp_ctxset(__sparcfpp_save, __sparcfpp_restore,
	    sizeof(sparcfpp_ctxt_t), FALSE);
}

/* 
 * lwp_fpset -- PRIMITIVE.
 * set a lwp to have floating point context
 * XX should try to do some auto configuring of this.
 */
int
lwp_fpset(tid)
	thread_t tid;
{
	(void)lwp_ctxinit(tid, SparcfppCookie);
}

/* enable libc special contexts */
void
lwp_libcenable()
{
	extern void __libc_save();
	extern void __libc_restore();

	LibcCtx = lwp_ctxset(__libc_save, __libc_restore,
	  sizeof (libc_ctxt_t), TRUE);
}

/* set a lwp to have libc context */
int
lwp_libcset(tid)
	thread_t tid;
{
	(void)lwp_ctxinit(tid, LibcCtx);
}

/*
 * routines for saving/restoring global library data.
 * Currently, only errno is preserved.
 */
void
__libc_save(cntxt, old, new)
	caddr_t cntxt;
	thread_t old;
	thread_t new;
{
	extern int errno;
#ifdef lint
	old = old;
	new = new;
#endif lint

	((libc_ctxt_t *)cntxt)->libc_errno = errno;
}

void
__libc_restore(cntxt, old, new)
	caddr_t cntxt;
	thread_t old;
	thread_t new;
{
	extern int errno;
#ifdef lint
	old = old;
	new = new;
#endif lint

	errno = ((libc_ctxt_t *)cntxt)->libc_errno;
}

/*
 * initialize utility library
 */
void
__init_util() {

	lwp_libcenable();
	lwp_chkstkenable();
	lwp_stkchkenable();
	lwp_sparcfppenable();
}

void
__panic(msg)
	char *msg;
{
	DISABLE();
	(void) fprintf(stderr, "nugget exiting: %s\n", msg);
	ENABLEALL();
	abort();
}

/*
 * debug_nugget() -- PRIMITIVE.
 * enable nugget debugging traces.
 * Not for client use.
 */
debug_nugget(argc, argv)
	int argc;
	char **argv;
{
	register int i;
	int flag, lev;

	setbuf(stdout, CHARNULL); /* so can redirect to file if little output */

	for (i = 0; i < MAXFLAGS; i++)
		__Trace[i] = 0;

	/* get debugging flags */
	while (--argc) {
		if ((argc == 1) && (atoi(argv[1]) == -1)) {
			for (i = 0; i < MAXFLAGS; i++)
				__Trace[i] = 99;
			break;
		}
		if (argc < 2)
			__panic("bad argc");
		flag = atoi(argv[argc-1]);
		lev = atoi(argv[argc--]);
		__Trace[flag] = lev;
	}
	__Trace[(int)tr_ABORT] = 0;
}
