/* Copyright (C) 1986 Sun Microsystems Inc. */

#include <lwp/common.h>
#include <lwp/queue.h>
#include <machlwp/machdep.h>
#include <lwp/cntxt.h>
#include <lwp/lwperror.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/monitor.h>
#include <lwp/condvar.h>
#include <machine/param.h>
#include <sys/kmem_alloc.h>
#include <sys/asynch.h>
#ifndef lint
SCCSID(@(#) lwputil.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

/*
 * PRIMITIVES contained herein:
 * debug_nugget(argc, argv) (* not publicly available *)
 * lwp_checkstkset(tid, stackbottom)
 * lwp_stkcswset(tid, stackbottom)
 */

int __Trace[MAXFLAGS];	/* trace flags */

/* For stack checking on context switch */
typedef struct stkchk_ctxt_t {
	stkalign_t *stkchk_stack;		/* bottom of stack */
} stkchk_ctxt_t;
STATIC int StkCtx;

/* for CHECK macro support */
STATIC int ChkstkCtx;
stkalign_t *__CurStack;	/* global indicating stack bottom for current thread */
/* create special context to  support CHECK macro */
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
/* ARGSUSED */
void
__chkstk_restore(cntxt, old, new)
	stkchk_ctxt_t *cntxt;
	thread_t old;
	thread_t new;
{
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
/* ARGSUSED */
void
__stkchk_save(cntxt, old, new)
	stkchk_ctxt_t *cntxt;
	thread_t old;
	thread_t new;
{
	register int *k;

	k = (int *)(cntxt->stkchk_stack);
	if ((*k++ != 0) || (*k++ != 0) || (*k++ != 0)) {
		__panic("stack overwritten");
	}

	/*
	 * We should really use a lwp_getregs to obtain the
	 * actual stack pointer, but this is faster.
	 */
#ifdef sparc
	if ((int *)((lwp_t *)old.thread_id)->mch_sp < k) {
#else sparc
	if ((int *)((lwp_t *)old.thread_id)->lwp_context[LSP] < k) {
#endif sparc
		__panic("stack overflow");
	}
}

/*
 * initialize utility library
 */
void
__init_util() {

	lwp_chkstkenable();
	lwp_stkchkenable();
}

void
__panic(msg)
	char *msg;
{
	(void) printf("nugget exiting: %s\n", msg);
	panic("LWP PANIC");
}

/*
 * debug_nugget() -- PRIMITIVE.
 * enable nugget debugging traces.
 * Not for client use.
 */
debug_nugget(flags)
	int flags;
{
	register int i;

	for (i = 0; i < MAXFLAGS; i++)
		__Trace[i] = 0;

	/* get debugging flags */
	for (i = 0; i < 32; i++) {
		if (flags & (01 << i)) {
			__Trace[i] = 99;
		}
	}
	__Trace[(int)tr_ABORT] = 0;
}

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/vm.h>
#ifdef QUOTA
#include <ufs/quota.h>
#endif
#include <sys/systm.h>

label_t *sleepqsave;
label_t kernelret;
mon_t UnixMonitor;
struct user *oldu, *DummyU;
struct proc *oldproc;
int schedpri = 0;
extern int maxasynchio_cached;

/* 
 * perproc_maxunreaped
 * 	Represents the number of unreaped aiodone_t's * associated
 *	with a process.
 *
 * maxunreaped
 * 	Represents the number of unreaped aioddone_t's in the system
 */
int maxunreaped = 0; 
int perproc_maxunreaped = 0;
int maxasynchio;	/* patchable version of MAXASYNCHIO */
lwpinit()
{
	extern void __lwp_init();

	maxasynchio = MAXASYNCHIO;
	maxunreaped = 50 * MAXASYNCHIO;
	perproc_maxunreaped = maxunreaped / 10;
	__lwp_init();
	unixenable();

	/*
	 * We use this monitor as a dummy for cv_create.
	 */
	(void)mon_create(&UnixMonitor);
	(void)lwp_setstkcache(KERNSTACK, maxasynchio_cached);
	cminit();
}

int runthreads = 0;

extern int __Nrunnable;
/*
 * called by locore.s routines to run threads.
 */
lwpschedule()
{
	extern int __LwpRunCnt;
	extern int __Nrunnable;
	int s;

	schedpri = s = splclock();	/* u.u_procp must be set atomically */
	if (runthreads) {
		(void) splx(s);
		return;
	}
	runthreads = 1;
	oldu = uunix;
	oldproc = masterprocp;
	(void) splx(s);

	if (setjmp(&kernelret) == 0) {
		__schedule();
	}

	s = splclock();
	uunix = oldu;
	masterprocp = oldproc;
	runthreads = 0;
	(void) splx(s);
}

#define	NOT_SLEEP -1
#define	UNINIT_CNTXT -2
#define	CMNULL	((struct chanmap *)0)
#define	CMHASH 32	 /* Must be power of 2 */
#define	HASH(x)			\
	(((int) x >> 5) & (CMHASH-1))

typedef struct unix_ctxt_t {
	struct user *unix_uarea;
	struct proc *unix_procp;
	label_t unix_qsave;
	int unix_intpri;
	int unix_slppri;
} unix_ctxt_t;
int UnixCtxt;

struct chanmap {
	struct chanmap *cm_next;
	caddr_t cm_chan;
	cv_t cm_cv;
	int cm_refcnt;
};

struct chanhash {
	struct chanmap *ch_forw;
	struct chanmap *ch_back;
};
struct chanhash *cm_list;
struct chanmap *cm_free;
void uncmsleep();
void del_chanmap();
struct chanmap * add_chanmap();

cminit()
{
	register struct chanmap *cm;
	register int i;

	cm_free = cm =
	    (struct chanmap *)new_kmem_alloc
	    (sizeof (struct chanmap) * maxasynchio, KMEM_SLEEP);

	cm_list =
	    (struct chanhash *)new_kmem_alloc
	    (sizeof (struct chanhash) * CMHASH, KMEM_SLEEP);

	for (i = 0; i < maxasynchio-1; i++, cm++)
		cm->cm_next = cm + 1;
	cm->cm_next = CMNULL;

	for (i= 0; i < CMHASH; i++)
		cm_list[i].ch_forw = cm_list[i].ch_back = CMNULL;
}

/*
 * cmsleep is only called when the caller is an actual LWP, so there
 * is a valid Curproc (!= &curproc). As the result of cv_wait, the
 * thread's unix context sleep priority is set to slppri.
 */
int
cmsleep(chan, pri)
	caddr_t chan;
	int pri;
{
	register struct chanmap *cm;
	register int oldps;
	thread_t tid;
	unix_ctxt_t ut;
	int slppri;
	
	if (panicstr)
		return (0);

	if (runthreads != 1)
		panic("not sleeping on a thread\n");
	oldps = splclock();
	slppri = pri & PMASK;
	(void) lwp_self(&tid);
	if (((lwp_t *)tid.thread_id)->lwp_cancel && slppri > PZERO) {
		goto psig;
	}

	(void) lwp_ctxmemget((caddr_t)&ut, tid, UnixCtxt);
	ut.unix_slppri = slppri;
	(void) lwp_ctxmemset((caddr_t)&ut, tid, UnixCtxt);

	cm = add_chanmap(chan);
	if (cv_wait(cm->cm_cv) < 0) { /* pri set to spl0 before switching */
		lwp_perror("cv_wait");
		panic("cmsleep: cv_wait aborted");
	}
	if (((lwp_t *)tid.thread_id)->lwp_cancel && slppri > PZERO) {
		goto psig;
	}
	ut.unix_slppri = NOT_SLEEP;
	(void) lwp_ctxmemset((caddr_t)&ut, tid, UnixCtxt);
	(void) splx(oldps); /* restore the pri. when cmsleep was entered */
	return (0);

psig:
	if (pri & PCATCH) {
		ut.unix_slppri = NOT_SLEEP;
		(void) lwp_ctxmemset((caddr_t)&ut, tid, UnixCtxt);
		(void) splx(oldps);
		return (1);
	}
	ut.unix_slppri = NOT_SLEEP;
	(void) lwp_ctxmemset((caddr_t)&ut, tid, UnixCtxt);
	(void) splx(oldps);
	longjmp(sleepqsave);
	/*NOTREACHED*/
}

/* wakeup on chan */
cmwakeup(chan, num)
	caddr_t chan;
	int num;
{
	register struct chanmap *cm;
	register struct chanmap *cprev;
	register int oldps;
	int index;
	extern char *panicstr;

	if (panicstr)
		return;
	/*
	 * wake everybody; we don't know if its hwp or lwp
	 * sleep just calls this blindly.
	 * We don't get the monitor unless we need to though.
	 */
	oldps = splclock();
	index = HASH(chan);
	cprev = CMNULL;
	cm = cm_list[index].ch_forw;
	while (cm != CMNULL) {
		if (cm->cm_chan == chan) {
			if (num == 0) {
				(void) cv_broadcast(cm->cm_cv);
				cm->cm_refcnt = 0;
			} else {
				(void) cv_notify(cm->cm_cv);
				cm->cm_refcnt--;
			}
			if (cm->cm_refcnt == 0) {
				del_chanmap(index, cprev);
				(void) cv_destroy(cm->cm_cv);
			}
			break;	/* chans are unique in list */
		}
		cprev = cm;
		cm = cm->cm_next;
	}
	(void) splx(oldps);
}

/*
 * If the thread is sleeping at an interruptible priority then
 * mark the thread as unwanted and make it runnable. It will wake up
 * in cmsleep.
 */
void
mark_unwanted(tid)
	thread_t tid;
{
	lwp_t *t = (lwp_t *)tid.thread_id;
	unix_ctxt_t ut;

	t->lwp_cancel = 1;
	if ((t->lwp_run == RS_CONDVAR) &&
	    !lwp_ctxmemget((caddr_t)&ut, tid, UnixCtxt) &&
	    (ut.unix_slppri > PZERO))
		uncmsleep(tid);
}

void
uncmsleep(tid)
	thread_t tid;
{
	register condvar_t *cv;
	register struct lwp_t *p;
	register struct chanmap *victim;
	register struct chanmap *cprev = CMNULL;
	register struct chanmap *cm;
	int index;

	p = (lwp_t *)tid.thread_id;
	(caddr_t) cv = p->lwp_blockedon;
	(caddr_t) victim = cv->cv_cm;

	/*
	 * Make the thread runnable; When it wakes up
	 * in cmsleep the suicide bit will be immediately
	 * noticed
	 */
	REMX_ITEM(&cv->cv_waiting, p);
	UNBLOCK(p);
	victim->cm_refcnt--;

	/*
	 * If this was the only/last thread waiting
	 * do the cleaup of cmwakeup
	 */
	if (victim->cm_refcnt == 0) {
		(void)cv_destroy(victim->cm_cv);
		index = HASH(victim->cm_chan);
		for (cm = cm_list[index].ch_forw; cm != CMNULL;
		    cprev = cm, cm = cm->cm_next) {
			if (cm == victim) {
				del_chanmap(index, cprev);
				return;
			}
		}
	}
}

void
del_chanmap(index, cprev)
	int index;
	struct chanmap *cprev;
{
	struct chanmap *cm;

	if (cprev == CMNULL) {
		cm = cm_list[index].ch_forw;
		cm_list[index].ch_forw = cm_list[index].ch_forw->cm_next;
	} else {
		cm = cprev->cm_next;
		cprev->cm_next = cprev->cm_next->cm_next;
	}
	if (cm->cm_next == CMNULL)
		cm_list[index].ch_back = cprev;
	cm->cm_next = cm_free;
	cm_free = cm;
}

struct chanmap *
add_chanmap(chan)
	caddr_t chan;
{
	int index;
	struct chanmap *cm;
	cv_t cv;

	index = HASH(chan);
	if ((cm = cm_list[index].ch_forw) != CMNULL) {
		while (cm != CMNULL && cm->cm_chan != (caddr_t)chan)
			cm = cm->cm_next;
		if (cm != CMNULL && cm->cm_chan == (caddr_t)chan) {
			cm->cm_refcnt++;
			return (cm);
		}
	}
	if ((cm = cm_free) == CMNULL)
		panic("no chan maps");
	cm_free = cm->cm_next;
	if (cm_list[index].ch_forw == CMNULL) {
		cm_list[index].ch_forw =
		    cm_list[index].ch_back = cm;
	} else {
		cm_list[index].ch_back->cm_next = cm;
		cm_list[index].ch_back = cm;
	}
	cm->cm_next = CMNULL;
	cm->cm_chan = (caddr_t)chan;
	(void) cv_create(&cv, UnixMonitor);
	((condvar_t *)cv.cond_id)->cv_cm = (caddr_t)cm;
	cm->cm_cv = cv;
	cm->cm_refcnt = 1;
	return (cm);
}

/*
 * define special context to install controlling address space
 * when a thread is switched in.
 */
unixenable()
{
	extern void __unix_restore();
	extern void __unix_save();

	UnixCtxt = lwp_ctxset(__unix_save, __unix_restore,
	    sizeof (unix_ctxt_t), 0);
}


/*
 * Set up the unix context associated with a thread; note
 * that the interrupt priority is not set here since we
 * are called from clock priority.
 */
int
unixset(tid)
	thread_t tid;
{
	unix_ctxt_t unixinit;
	int s;

	s = splclock();
	unixinit.unix_uarea = DummyU = uunix;
	unixinit.unix_procp = uunix->u_procp;
	unixinit.unix_slppri = NOT_SLEEP;
	(void) lwp_ctxinit(tid, UnixCtxt);
	(void) lwp_ctxmemset((caddr_t)&unixinit, tid, UnixCtxt);
	(void) splx(s);
}


/* ARGSUSED */
void
__unix_save(cntxt, old, new)
	unix_ctxt_t *cntxt;
	thread_t old;
	thread_t new;
{
}

/* ARGSUSED */
void
__unix_restore(cntxt, old, new)
	unix_ctxt_t *cntxt;
	thread_t old;
	thread_t new;
{
	int oldps = splclock();
	struct proc *tp;

	tp = cntxt->unix_procp;
	if ((tp->p_flag & SLOAD) == 0)
		(void) swapin(tp);
	uunix = DummyU = cntxt->unix_uarea;
	masterprocp = tp;
	sleepqsave = &cntxt->unix_qsave;
	setmmucntxt(masterprocp);
	(void) splx(oldps);
}
