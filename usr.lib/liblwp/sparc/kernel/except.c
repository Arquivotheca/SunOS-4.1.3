/* Copyright (C) 1987 Sun Microsystems Inc. */

#include <lwp/common.h>
#include <lwp/queue.h>
#include <lwp/asynch.h>
#include <machlwp/machsig.h>
#include <machlwp/machdep.h>
#include <lwp/queue.h>
#include <lwp/cntxt.h>
#include <lwp/lwperror.h>
#include <lwp/message.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/alloc.h>
#include <machlwp/except.h>
#include <lwp/monitor.h>

#ifndef lint
SCCSID(@(#) except.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

/*
 * PRIMITIVES contained herein:
 * exc_handle(pattern, func, env)
 * exc_unhandle()
 * (*exc_bound(pattern, env))()
 * exc_notify(pattern)
 * exc_raise(process, pattern)
 * exc_on_exit(func, arg)
 * exc_uniqpatt()
 */

/* Globals to facilitate asm stuff. Must not be static. */
exccontext_t *__Cntxt;
int *__ProcRet;
int __ClientSp, __ClientRetAdr;

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
 * the call to exc_handle()
 * Special context (e.g., fpa regs) is NOT saved by exc_handle.
 */
int
exc_handle(pattern, func, env)
	int pattern;	/* pattern to match on excraise */
	caddr_t (*func)(); /* func bound to the exception for NOTIFY semantics */
	caddr_t env;	/* environment argument passed to func */
{
	LWPINIT();
	CLR_ERROR();
	LOCK();
	ERROR(((pattern == EXITPATTERN) || (pattern == -1)), LE_INVALIDARG);
	GETCHUNK((exccontext_t *), __Cntxt, ExcType);

	/* flush windows, get client return pc and stack pointer */
	__exc_getclient();
	__Cntxt->exc_sp = __ClientSp;	/* save %sp of exc_handle caller */

	/*
	 * Client called exc_handle.
	 * __ProcRet contains address where *client* will return to.
	 * We look on client's stack to find it.
	 * Note that __exc_getclient() had to flush the windows to ensure
	 * that the register save area of the client is current.
	 */
	__Cntxt->exc_pc = __ClientRetAdr;
	__ProcRet = (int *)__ClientSp + PC_OFFSET;

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
	if ((*__ProcRet + 8) != (int)__exccleanup) {
		__Cntxt->exc_retaddr = (int *)__ProcRet;
		__Cntxt->exc_return = *__ProcRet;
		__Cntxt->exc_refcnt = 0;
		*__ProcRet = (int)__exccleanup;
		*__ProcRet -= 8;
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

	__exc_getclient();
	__ProcRet = (int *)__ClientSp + PC_OFFSET;
	if ((*__ProcRet + 8) == (int)__exccleanup) {
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
		newcntxt->exc_retaddr = (int *)__ProcRet;
		newcntxt->exc_return = *__ProcRet;
		*__ProcRet = (int)__exccleanup; 
		*__ProcRet -= 8;
		FRONT_QUEUE(&__Curproc->lwp_exchandles, newcntxt);
	}
	UNLOCK();
	return (0);
}

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
	__exc_getclient();
	__ProcRet = (int *)__ClientSp + PC_OFFSET;
	ERROR(((*__ProcRet + 8) != (int)__exccleanup), LE_NONEXIST);
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
		/*
		 * make consistent so can look here
		 * to see if exit handler installed
		 */
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

/*
 * exc_raise() -- PRIMITIVE.
 * Raise an exception by searching for a matching handler.
 */
int
exc_raise(pattern)
	int pattern;	/* pattern to match the one set by exc_handle() */
{
	register exccontext_t *cntxt;
	register int i;
	int popcount = 0;
	caddr_t (*func)();
	caddr_t env;
	int patt;
	int sp;
	int pc;
	int ret;

	if ((pattern == EXITPATTERN) || (pattern == -1)) {
		SET_ERROR(__Curproc, LE_INVALIDARG);
		return (-1);
	}
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

	/* 
	 * If last handler in a procedure and not an
	 * exit handler, remove __exccleanup and return to the handler context.
	 * else, just return to the handler context.
	 * Note that we must be careful to not rely on restore to
	 * install the correct value from the register save area on
	 * window underflow since the current value of %i7 can get flushed
	 * to the window save area when doing the longjmp.
	 */
	sp = cntxt->exc_sp;
	pc = cntxt->exc_pc;
	if ((cntxt->exc_refcnt == 0) && (cntxt->exc_pattern != EXITPATTERN)) {
		/* make save area consistent so can tell if handler installed */
		*cntxt->exc_retaddr = cntxt->exc_return;
		ret = cntxt->exc_return;
		UNLOCK();
		__exc_jmpandclean(pattern, sp, pc, ret);
	} else {
		UNLOCK();
		__exc_longjmp(pattern, sp, pc);
	}
	/* NOTREACHED */
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
int
__exchelpclean()
{
	register exccontext_t *cntxt = EXCNULL;
	register int retloc;
	register qheader_t *q = &__Curproc->lwp_exchandles;
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
	UNLOCK();
	return (retloc); /* return addr to caller is put in %i7 in exccleanup */
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
 * What to do when a trap occurs. Called from low.s.
 * If any sort of handler was established, raise the exception.
 * Otherwise, destroy the offending thread.
 * When we have language-level exception handlers, we will need
 * exc_signal(func) to tell the kernel where the raise routine is.
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
