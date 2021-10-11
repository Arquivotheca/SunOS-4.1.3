/* Copyright (C) 1986 Sun Microsystems Inc. */

#include <lwp/common.h>
#include <lwp/queue.h>
#include <lwp/cntxt.h>
#include <machlwp/machdep.h>
#include <lwp/lwperror.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/alloc.h>
#include <lwp/condvar.h>
#include <lwp/monitor.h>
#ifndef lint
SCCSID(@(#) process.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

extern int runthreads;
extern void return_stk();

/*
 * PRIMITIVES contained herein:
 * lwp_self()
 * lwp_create(tid, pc, prio, flags, stack, nargs, arg1, arg2, ..., argn)
 * lwp_destroy(tid)
 * lwp_suspend(tid)
 * lwp_resume(tid)
 * lwp_geterr()
 */

extern int __LwpRunCnt;		/* total number of lwp's in the system */
STATIC int LwpType;		/* cookie for context caches */
STATIC int LwpStatus;		/* exit status for pod */
thread_t __ThreadNull = {CHARNULL, 0};		/* the non-existent thread */
int __Nrunnable;		/* Number of runnable threads */
qheader_t __SuspendQ;
qheader_t __SleepQ;

/*
 * Automatic kill on process exit
 */
void
__lwpkill()
{
	thread_t corpse;

	/*
	 * If an interrupt causes this process to die
	 * in another way, we never return here.
	 * __Curproc will always be correct when
	 * this process is running so there is no race.
	 */
	LWPTRACE(tr_PROCESS, 1, ("process died automatically\n"));
	corpse.thread_id = (caddr_t)__Curproc;
	corpse.thread_key = __Curproc->lwp_lock;
	(void)lwp_destroy(corpse);
	__panic("return from __lwpkill");
}


/*
 * This process is really dead. We can safely remove all traces of
 * it that remain: its context.
 */
void
__freeproc(lwp)
	register lwp_t *lwp;	/* completely stiff corpse */
{
	register caddr_t laststack;

	LWPTRACE(tr_PROCESS, 2, ("__freeproc %x\n", lwp));
	lwp->lwp_run = RS_UNINIT;
	lwp->lwp_type = BADOBJECT_TYPE;

	/* free any special contexts */
	__freectxt(lwp);

	lwp->lwp_lock = INVALIDLOCK;	/* in case someone refs this mem */

	/*
	 * free the process descriptor itself. Once we do this,
	 * the lwp must not be on any queues (it will be on
	 * the free list after this).
	 */
	laststack = (caddr_t)lwp->lwp_stack;
	FREECHUNK(LwpType, lwp);
	return_stk(laststack);
}

/*VARARGS*/
/*
 * lwp_create() -- PRIMITIVE.
 * create a (runnable) lightweight process and start it running if
 * it's priority warrants it.
 * It is required that the new task start at a procedure entry
 * to make it easier to keep track of how to set up the stack (see machdep.c).
 * We assume that the stack points to the top of the (already allocated)
 * stack, and that the stack is mapped in.
 */

int
lwp_create(va_alist)
	va_dcl			/* arguments to new thread */
{
	va_list argptr;		/* argument pointer for varargs package */
	thread_t *tid;		/* thread being created */
	void (*pc)();		/* initial procedure defining the new task */
	int prio;		/* scheduling priority (higher preempts lower) */
	int flags;		/* options to create thread with */
	stkalign_t *stack;	/* top of client-supplied stack */
	int nargs;		/* number of arguments */

	register lwp_t *baby;	/* the new little process we are making */
	int funarg;

	LWPINIT();
	CLR_ERROR();
	va_start (argptr);
	tid = va_arg(argptr, thread_t *);
	funarg = va_arg(argptr, int); pc = (void(*)())funarg;
	prio = va_arg(argptr, int);
	flags = va_arg(argptr, int);
	stack = va_arg(argptr, stkalign_t *);
	nargs = va_arg(argptr, int);
	ERROR((nargs > MAXARGS), LE_INVALIDARG);
	ERROR(((prio > __MaxPrio) || (prio < MINPRIO)), LE_ILLPRIO);
	GETCHUNK((lwp_t *), baby, LwpType);
	ERROR((baby == LWPNULL), LE_NOROOM);
	baby->lwp_stack = (int)stack;
	stack = (stkalign_t *)STACKALIGN(stack);
	if (stack != (stkalign_t *)0) {
		MAKE_STACK(baby, argptr, stack, __lwpkill, pc, nargs);
	} else {	/* force suspension; can't run yet */
		flags |= LWPSUSPEND;
	}
	va_end (argptr);
	baby->lwp_full = FALSE;
	INIT_QUEUE(&baby->lwp_ctx);
	baby->lwp_type = PROCESS_TYPE;
	__LwpRunCnt++;
	LWPTRACE(tr_PROCESS, 2, ("numprocesses %d\n", __LwpRunCnt));
	baby->lwp_blockedon = NOBODY;
	baby->lwp_curmon = MONNULL;
	baby->lwp_suspended = FALSE;
	baby->lwp_prio = prio;
	baby->lwp_run = RS_UNINIT;
	baby->lwp_lwperrno = LE_NOERROR;
	baby->lwp_monlevel = 0;
	baby->lwp_flags = flags;
	baby->lwp_lock = UNIQUEID();
	baby->lwp_cancel = 0;
#ifdef	MULTIPROCESSOR
	lwp_setref(baby);
#endif	MULTIPROCESSOR
	if (tid != (thread_t *)0) {
		tid->thread_id = (caddr_t)baby;
		tid->thread_key = baby->lwp_lock;
	}
	if (flags & LWPSUSPEND) {
		INS_QUEUE(&__SuspendQ, baby);
		baby->lwp_run = RS_SUSPEND;
		baby->lwp_suspended = TRUE;
	} else {
		UNBLOCK(baby);

	/*
	 * In 4.1, a thread never calls lwp_create
	 */
		if (!runthreads)
			return (0);
		YIELD_CPU(prio);
	}
	return (0);
}

/*
 * lwp_resume -- PRIMITIVE.
 * make process eligible to run from suspended state and
 * run it if its priority warrants it.
 */
int
lwp_resume(tid)
	thread_t tid;
{
	register lwp_t *proc = (lwp_t *)(tid.thread_id);
	int s;

	CLR_ERROR();
	s = splclock();
	ERROR((tid.thread_key != proc->lwp_lock), LE_NONEXIST);
	proc->lwp_suspended = FALSE;
	if (proc->lwp_run == RS_SUSPEND) { /* not waiting for a message, etc. */
		REMX_ITEM(&__SuspendQ, proc);
		UNBLOCK(proc);
		if (!runthreads) {
			(void) splx(s);
			return (0);
		}
		YIELD_CPU(proc->lwp_prio);
	}
	(void) splx(s);
	return (0);
}

/*
 * lwp_self -- PRIMITIVE.
 * Set identity of current thread.
 */
int
lwp_self(tid)
	thread_t *tid;	/* will hold identity of current thread */
{
	LWPINIT();
	CLR_ERROR();
	tid->thread_id = (caddr_t)__Curproc;
	tid->thread_key = __Curproc->lwp_lock;
	return (0);
}

/*
 * lwp_destroy -- PRIMITIVE.
 * destroy a lwp or agent and free all resources it owns 
 * (such as monitor locks).
 */
int
lwp_destroy(body)
	thread_t body;
{
	register lwp_t *corpse = (lwp_t *)(body.thread_id);
	register int prio;
	register int kill_mutex;

	kill_mutex = splclock();
	CLR_ERROR();
	if (corpse == LWPNULL) {	/* self-destruction */
		body.thread_id =  (caddr_t)__Curproc;
		corpse = __Curproc;
		body.thread_key = corpse->lwp_lock;
	}
	ERROR((body.thread_key != corpse->lwp_lock), LE_NONEXIST);
	prio = corpse->lwp_prio;

	/*
	 * Remove this process from whatever queue it belonged to.
	 * Must dequeue BEFORE freeing the context itself.
	 * In effect, we make the process look like it's running from
	 * the point of view of the cleanup routines as the process
	 * will not be on any blocked queue. (It won't be on the __RunQ
	 * either though).
	 */
	switch (corpse->lwp_run) {
	    case RS_CONDVAR:
		REMX_ITEM(&(((condvar_t *)(corpse->lwp_blockedon))->cv_waiting),
		  corpse);
		break;
	    case RS_UNBLOCKED:
		REMX_ITEM(&__RunQ[prio], corpse);
		__Nrunnable--;
		break;
	   case RS_SUSPEND:
		REMX_ITEM(&__SuspendQ, corpse);
		break;
	    default:
		__panic("bad destroy");
	}
	LWPTRACE(tr_PROCESS, 3, ("Remove process, %d left\n", __LwpRunCnt));

	/*
	 * Before calling freeproc, be sure that I'm not depending on
	 * the client stack for anything.
	 * At this point, we're sure that the client will die.
	 * Since we are splclock nobody will try to save registers
	 * on behalf of a non-valid __Curproc.
	 */
	if (corpse == __Curproc) {
		SETSTACK();
	}
	__freeproc(corpse);

	--__LwpRunCnt;
	if (corpse == __Curproc) {
		__Curproc = &Nullproc; /* NB: if inter. now, this is the victim */
		(void) splx(kill_mutex);
		__schedule();
		/* NOTREACHED */
	} else {	/* removing somebody other than caller */
		LWPTRACE(tr_PROCESS, 4,
		  ("removing process in middle of run queue\n"));
		(void) splx(kill_mutex);
		if (!runthreads) {
			return (0);
		}
		WAIT();
	}
	return (0);
}

/*
 * lwp_geterr() -- PRIMITIVE.
 * return the error number associated with the current lwp.
 */
lwp_err_t
lwp_geterr()
{
	return (__Curproc->lwp_lwperrno);
}

/*
 * initialize context caches and process info
 */
void
__init_lwp()
{
	register int i;
	extern int maxasynchio_cached;

	__Nrunnable = 0;
	LwpStatus = 0;		/* default: normal exit status */
	LwpType = __allocinit(sizeof (lwp_t), maxasynchio_cached,
	    IFUNCNULL, FALSE);
	for (i = 0; i <= __MaxPrio; i++)
		INIT_QUEUE(&__RunQ[i]);
	INIT_QUEUE(&__SleepQ);
	INIT_QUEUE(&__SuspendQ);
	__LwpRunCnt = 0;
}
