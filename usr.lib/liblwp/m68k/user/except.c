/* Copyright (C) 1986 Sun Microsystems Inc. */
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
#include "schedule.h"
#include "alloc.h"
#include "except.h"
#include "monitor.h"
#ifndef lint
SCCSID(@(#) except.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

/*
 * PRIMITIVES contained herein:
 * exc_handle(pattern, func, env)
 * exc_unhandle()
 * (*exc_bound(pattern, env))()
 * exc_notify(pattern)
 * exc_raise(pattern)
 * exc_on_exit(func, arg)
 * exc_uniqpatt()
 */

/* Globals to facilitate asm stuff. Must not be static. */
exccontext_t *__Cntxt;
caddr_t *__ExcRegs;
int *__ProcRet;
int __Argptr;
STATIC int ExcType;		/* cookie for exception context caches */

/*
 * exc_uniqpatt() -- PRIMITIVE.
 * return a unique pattern that doesn't conflict with any
 * preassigned patterns. BUGS: wraparound voids uniqueness guarantee.
 * We know that 0 (EXITPATTERN), -1 (error) and -2 (CATCHALL) are
 * reserved. Also, special meaning is reserved for 1..NUMSIG-1
 * for synchronous traps.
 * If -1 is returned, a non-unique pattern was returned.
 * Subsequently, all patterns returned may be duplicates.
 */
int
exc_uniqpatt()
{
	static int newpat = NUMSIG - 1;

	newpat++;
	if (newpat == CATCHALL) {
		newpat = NUMSIG - 1;
		SET_ERROR(__Curproc, LE_REUSE);
		return (-1);
	}
	return (newpat);
}

/*
 * exc_handle() -- PRIMITIVE.
 * Establish an exception handler. A procedure calling
 * exc_handle will be set up to return to a special
 * cleanup procedure that pops all exception contexts
 * before returning control to the original return point of the procedure.
 * The context saved by the handler will allow exc_raise()
 * to act as a non-local return to the point just after
 * the call to exc_handle(). exc_raise() actually accomplishes
 * control transfer via a context switch to the exc_handled context.
 * Special context (e.g., fpa regs) is NOT saved by exc_handle.
 * Only non-volatile regs are saved/restored by exc_handle/exc_raise.
 */
int
exc_handle(pattern, func, env)
	int pattern;		/* pattern to match on excraise */
	caddr_t (*func)();	/* func bound to exception for NOTIFY semantics */
	caddr_t env;		/* environment argument passed to func */
{
	/* no locals to simplify things (i.e., registers are unchanged) */

	LWPINIT();
	CLR_ERROR();
	LOCK();
	ERROR(((pattern == EXITPATTERN) || (pattern == -1)), LE_INVALIDARG);

	/* don't use local registers (proc call ok cause regs restored on ret) */
	NR_GETCHUNK((exccontext_t *), __Cntxt, ExcType);
	__ExcRegs = (caddr_t *)__Cntxt->exc_regs;

	/*
	 * assume procedure foo() calls procedure bar() and bar() contains
	 * a call to exc_handle. Then the stack looks like:
	 *
	 * 		args to bar
	 *  		return address back to foo	<== __ProcRet
	 * 	|-->a6	valid in foo <== ArgPtr
	 *	|	locals in foo
	 *	|	args to exc_handle() <== sp in foo (foo pops the args)
	 *	|	ret addr just after call to exc_raise in bar() <== SP+4
	 *	|<--a6	valid in bar	<== SP, A6
	 */	

	/* save all volatile registers at point of handle call */
	asm ("movl ___ExcRegs, a0");
	asm ("moveml #0xfcfc, a0@");
	asm ("movl a6@(0), ___Argptr");	/* caller's value of a6 */
	__Cntxt->exc_regs[A6] = __Argptr;

	/* get return address (to return from exc_handle call) */
	__Cntxt->exc_pc = *(int *) (__Cntxt->exc_regs[SP] + 4);

	/*
	 * Set saved stack to point before call to exc_handle().
	 * When exc_handle returns, the sp will be here.
	 */
	__Cntxt->exc_regs[SP] += 2 * sizeof (int);

	/*
	 * foo calls bar which calls exc_handle().
	 * __ProcRet points to the address where foo will return to 
	 * when bar returns.
	 * Note that it's important that create_lwp take a procedure,
	 * not just a starting address, because a6 must be set up correctly
	 * for the handler to be accessible via a6 in the initial procedure.
	 */
	__ProcRet = (int *)(__Argptr + 4); /* addr of where return address is */
	__Cntxt->exc_pattern = pattern;
	__Cntxt->exc_func = func;
	__Cntxt->exc_env = env;

	/*
	 * If the exit routine (which is installed like a breakpoint)
	 * isn't already established, do so by saving
	 * the correct return address and installing the exit routine
	 * exccleanup(). Otherwise, just bump the ref. count of
	 * exception handlers set within the current procedure.
	 * A ref count of 0 indicates the first handler in a procedure.
	 * Thus, the stack of handlers will look like (for example):
	 * 4 3 2 1 0 3 2 1 0 .... 1 0
	 * The ref count is only used to conveniently find the
	 * initial exception handler in a procedure.
	 */
	if (*__ProcRet != (int) __exccleanup) {
		__Cntxt->exc_return = *__ProcRet;
		__Cntxt->exc_retaddr = (int *)__ProcRet;
		__Cntxt->exc_refcnt = 0;
		*__ProcRet = (int) __exccleanup; 
	} else {
		__Cntxt->exc_refcnt = 1 +
		 ((exccontext_t *)FIRSTQ(&__Curproc->lwp_exchandles))->exc_refcnt;
	}
	FRONT_QUEUE(&__Curproc->lwp_exchandles, __Cntxt);
	UNLOCK();
	return (0);
}

/*
 * exc_on_exit -- PRIMITIVE.
 * Establish an exit handler for a procedure.
 * Each procedure may establish at most one exit handler
 * which will be invoked when the procedure exits (either
 * via a return or as a result of an exception).
 * The exit handler may not contain exc_raise's.
 * Further, the argument may not contain the address of a local variable
 * in the scope of the procedure.
 * Note that all local context (stack) is gone when the exit handler
 * is invoked. Also, any results returned by an exit handler are ignored:
 * the value returned by the returning procedure is preserved.
 */
int
exc_on_exit(func, arg)
	void (*func)();
	caddr_t arg;
{
	register exccontext_t *cntxt;
	register exccontext_t *newcntxt;

	LWPINIT();
	CLR_ERROR();
	LOCK();
	GETCHUNK((exccontext_t *), newcntxt, ExcType);
	newcntxt->exc_pattern = EXITPATTERN;
	newcntxt->exc_func = (caddr_t (*)())func;
	newcntxt->exc_env = arg;
	newcntxt->exc_refcnt = 0;
	asm ("movl a6@, ___ProcRet");
	__ProcRet++;
	if (*__ProcRet == (int) __exccleanup) {
		/*
		 * make this the first entry in the stack of handlers
		 * for this process by adjusting ref counts.
		 */
		for (cntxt = (exccontext_t *)FIRSTQ(&__Curproc->lwp_exchandles);
	  	  cntxt != EXCNULL; cntxt = cntxt->exc_next) {
			if (cntxt->exc_refcnt++ == 0) {
				newcntxt->exc_next = cntxt->exc_next;
				cntxt->exc_next = newcntxt;
				newcntxt->exc_return = cntxt->exc_return;
				newcntxt->exc_retaddr = cntxt->exc_retaddr;
				break;
			}
		}
	} else {
		newcntxt->exc_return = *__ProcRet;
		newcntxt->exc_retaddr = (int *)__ProcRet;
		*__ProcRet = (int) __exccleanup; 
		FRONT_QUEUE(&__Curproc->lwp_exchandles, newcntxt);
	}
	UNLOCK();
	return (0);
}

int __RetAddr;	/* asm help */

/*
 * exc_unhandle() -- PRIMITIVE.
 * Pop the most recent handler from handler stack and remove
 * __exccleanup if no more handlers remain in the procedure.
 * It is illegal to remove an exit handler this way.
 */
int
exc_unhandle()
{
	register exccontext_t *cntxt;

	CLR_ERROR();
	LOCK();

	/* 
	 * check that there is at least one handler left in
	 * the procedure by looking at the return address.
	 */
	asm ("movl a6@, a0");
	asm ("movl a0@(4), ___RetAddr");
	ERROR((__RetAddr != (int)__exccleanup), LE_NONEXIST);
	REM_QUEUE((exccontext_t *), &__Curproc->lwp_exchandles, cntxt);
	if (cntxt == EXCNULL)
		__panic("garbage exception context");
	if (cntxt->exc_pattern == EXITPATTERN) {
		FRONT_QUEUE(&__Curproc->lwp_exchandles, cntxt);
		TERROR(LE_NONEXIST);
	}

	/*
	 * if last one, (note that no exit handler can be set here)
	 * restore normal return address (replacing __exccleanup).
	 */
	if (cntxt->exc_refcnt == 0) {
		*cntxt->exc_retaddr = cntxt->exc_return;
	}
	FREECHUNK(ExcType, cntxt);
	UNLOCK();
	return (0);
}

/*
 * exc_bound() -- PRIMITIVE.
 * returns function bound to handler and sets the reference
 * param "env" to the vaalue bound by the handler.
 * env is useful for indicating context of local variables.
 */
caddr_t
(*exc_bound(pattern, env))()
	register int pattern;	/* pattern bound to handler */
	caddr_t *env;		/* environment to be returned to caller */
{
	register __queue_t *q;
	register caddr_t (*func)();

	CLR_ERROR();
	LOCK();
	q = FIRSTQ(&__Curproc->lwp_exchandles);
	while ((q != QNULL) && (((exccontext_t *)q)->exc_pattern != pattern)) {
		q = q->q_next;
	}
	if (q == QNULL) {
		UNLOCK();
		return (CFUNCNULL);
	} else {
		*env = (((exccontext_t *)q)->exc_env);
		func = ((exccontext_t *)q)->exc_func;
		UNLOCK();
		return (func);
	}
}

/* 
 * exc_notify() -- PRIMITIVE.
 * Utility for using excbound: if there is a non-null function
 * bound to the handler bound to "pattern", invoke it with the
 * bound argument. Otherwise, raise an exception with the pattern.
 * Returns two kinds of results: int for exc_raise (the pattern) or
 * anything (whatever the handler function returns).
 * The (int) cast is therefore only to make lint happy, not what really is
 * returned.
 */
int
exc_notify(pattern)
	int pattern;
{
	register caddr_t (*func)();
	caddr_t env;

	CLR_ERROR();
	func = exc_bound(pattern, &env);
	if (func != CFUNCNULL)
		return ((int)func(env));
	else
		return (exc_raise(pattern));
}

/* Do the real work of exc_raise with room above on the stack to work with */
STATIC int
excraise(pattern)
	int pattern;	/* pattern to match the one set by exc_handle() */
{
	register int *proc;
	register exccontext_t *cntxt;
	register int *c;
	register int i;
	int popcount = 0;
	int *sp;
	caddr_t (*func)();
	caddr_t env;
	int patt;
	extern int __excreturn();

	CLR_ERROR();
	LOCK();

	/* look for a match, but don't skip backout handlers */
	for (cntxt = (exccontext_t *)FIRSTQ(&__Curproc->lwp_exchandles);
	  cntxt != EXCNULL; cntxt = cntxt->exc_next) {
		if ((cntxt->exc_pattern != pattern) && 
		  (cntxt->exc_pattern != CATCHALL)) {
			popcount++;	/* don't zap now in case no match */
		} else {
			break;
		}
	}
	ERROR((cntxt == EXCNULL), LE_NONEXIST);

	/* 
	 * if last handler in a procedure and not an
	 * exit handler, remove __exccleanup
	 */
	if ((cntxt->exc_refcnt == 0) && (cntxt->exc_pattern != EXITPATTERN)) {
		*cntxt->exc_retaddr = cntxt->exc_return;
	}

	/*
	 * alter the actual context so CSWTCH() will work.
	 */
	sp = (int *)cntxt->exc_regs[SP];
	c = cntxt->exc_regs;
	proc = __Curproc->lwp_context;
	for (i = 0; i < MC_GENERALREGS; i++)
		proc[i] = c[i];

	/* 
	 * we will push 3 things for __excreturn (see low.s) to use
	 * so subtract 3 items:
	 * __excreturn, pattern, return address.
	 * __excreturn is the address to go to upon CSWTCH;
	 * excpattern will be popped by __excreturn into d0,
	 * and __excreturn will rts to exc_pc.
	 * Important that pattern be on stack
	 * since it could be lost in a global if
	 * an asynchrouous event caused an exception to occur.
	 */
	__Curproc->lwp_context[SP] = (int)sp - (3 * sizeof (int));
	*(--sp) = cntxt->exc_pc;
	*(--sp) = pattern;
	*(--sp) = (int) __excreturn;

	/* destroy intervening contexts incl. this one */
	for (i = 0; i <= popcount; i++) {
		REM_QUEUE((exccontext_t *), &__Curproc->lwp_exchandles, cntxt);
		func = cntxt->exc_func;
		env = cntxt->exc_env;
		patt = cntxt->exc_pattern;
		FREECHUNK(ExcType, cntxt);
		if (patt == EXITPATTERN) {
			/*
			 * Do exit handling en route to a handler above.
			 * Note that we're on the client stack.
			 * If excraise were implemented as a system call,
			 * (i.e., on a sep. stack here),
			 * should context switch to this context and
			 * have the cleanup code reraise the exception
			 * (int lwp_needreraise to hold pattern added to cntxt.)
			 * UNLOCK so can call nugget cleanly (e.g., mon_exit).
			 */
			UNLOCK();
			(void) (*func)(env);
			LOCK();
		}
	}

	/* now a normal context switch will put us in the handler context */
	CSWTCH();
	/* NOTREACHED */
}

/*
 * exc_raise() -- PRIMITIVE.
 * Raise an exception by searching for a matching handler, installing
 * its context in the process's context, and doing a context switch.
 * exc_raise will munge the stack starting with the point where
 * the exc_handle call was made. This could overwrite the stack
 * where exc_raise is executing, so we make a dummy procedure
 * call to guarantee room on the stack for exc_raise to execute safely.
 */
int
exc_raise(pattern)
	int pattern;
{

	int dummy[EXCSTKSZ];
#ifdef lint
	dummy[0] = dummy[0];
#endif lint

	if ((pattern == EXITPATTERN) || (pattern == -1)) {
		SET_ERROR(__Curproc, LE_INVALIDARG);
		return (-1);
	}
	return (excraise(pattern));
}

/*
 * __exccleanup is an assembly language entry point
 * that is called when a procedure that established
 * one or more exception handlers returns. (it is called only once,
 * regardless of how many handlers the procedure set up).
 * It's function is to call exchelpclean().
 * It must provide a way so it (__exccleanup) can return
 * a different place (the original procedure return point).
 * Note that exchelpclean() is called as a byproduct of the
 * original procedure returning normally so the stack and
 * registers are all as they should be. Thus, from the
 * point of view of the original procedure, it looks like
 * it called __exccleanup immediately upon return.
 * Finally, note that it's important that contexts are
 * NOT allocated on the stack since we would be clobbering
 * them with the locals, etc. here. We could get around
 * this by writing the whole thing in assembler, not calling
 * any procedures (including this helper procedure), and carefully
 * avoid messing with the stack.
 * However, in environments (e.g., sys V) where interrupts can't
 * be saved on an alternate stack, we could have interrupts
 * clobbering the stack (and thus any cntxt info) on the sly.
 */

void
__exchelpclean()
{
	register exccontext_t *cntxt = EXCNULL;
	register int retloc;
	register qheader_t *q = &__Curproc->lwp_exchandles;
	register int *retaddr;
	int patt;
	caddr_t env;
	int refcnt;
	caddr_t (*func)();

	CLR_ERROR();
	LOCK();

	/*
	 * Discard all unused handlers (including CATCHALL).
	 * Must be at least one, since we're here cleaning up.
	 */
	while (!ISEMPTYQ(q)) {
		REM_QUEUE((exccontext_t *), q, cntxt);
		func = cntxt->exc_func;
		env = cntxt->exc_env;
		patt = cntxt->exc_pattern;
		refcnt = cntxt->exc_refcnt;
		retaddr = cntxt->exc_retaddr;
		retloc = cntxt->exc_return;
		FREECHUNK(ExcType, cntxt);
		if (patt == 0) {
			UNLOCK();
			(void) (*func)(env);
			LOCK();
		}
		if (refcnt == 0)
			break;
	}

	/* 
	 * return address must be on stack so we don't
	 * lose it (a global can be overwritten if not locked)
	 * in case of an interrupt. exchelpclean() left room where
	 * the original return address was for it to be restored.
	 * We restore the original return address there, return to
	 * __exccleanup to restore any registers used by exchelpclean(),
	 * and then execute the original return.
	 * At this point, we have the initial context (refcnt == 0)
	 * to get the right return adddress from.
	 */
	*retaddr = retloc;
	UNLOCK();
}

/* clean up any exception handlers held by a dying process */
void
__exceptcleanup(corpse)
	lwp_t *corpse;
{
	register exccontext_t *cntxt;

	while (!ISEMPTYQ(&corpse->lwp_exchandles)) {
		REM_QUEUE((exccontext_t *), &corpse->lwp_exchandles, cntxt);
		LWPTRACE(tr_EXCEPT, 1, ("freeing exception %x\n", cntxt));
		FREECHUNK(ExcType, cntxt);
	}
}

/* allocate a cache of exception contexts */
void
__init_exc()
{
	ExcType =
	  __allocinit(sizeof (exccontext_t), NUMHANDLERS, IFUNCNULL, FALSE);
}

/*
 * What to do when a trap occurs. Called from low.s
 * If any sort of handler was established, raise the exception.
 * Otherwise, destroy the offending thread.
 */
int
__exc_trap(trapid)
	int trapid;	/* UNIX signal causing trap */
{
	register caddr_t (*func)();
	caddr_t env;
	int result;

	func = exc_bound(trapid, &env);
	if (func != CFUNCNULL)
		return ((int)func(env));
	result = exc_raise(trapid);
	if (result != -1) {
		return (result);
	} else {
		(void)lwp_destroy(SELF);
		/* NOTREACHED */
	}
}
