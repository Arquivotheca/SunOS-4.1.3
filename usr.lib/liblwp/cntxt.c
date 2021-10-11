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
#include <lwp/common.h>
#include <lwp/queue.h>
#include <lwp/asynch.h>
#include <machlwp/machsig.h>
#include <machlwp/machdep.h>
#include <lwp/queue.h>
#include <lwp/lwperror.h>
#include <lwp/cntxt.h>
#include <lwp/message.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/alloc.h>
#include <lwp/condvar.h>
#include <lwp/monitor.h>
#include <lwp/agent.h>
#include <machlwp/except.h>
#ifndef lint
SCCSID(@(#) cntxt.c 1.1 92/07/30 Copyr 1986 Sun Micro);
#endif lint

int CtxType;	/* memory type for context list entry in a lwp context */

/* list of save and restore routines and context sizes for all special contexts */
STATIC cntxt_actions_t SwtchActs[MAXSWTCHACTS];

/* table to keep track of the special context to be saved lazily */
STATIC context_t *LazyCntxts[MAXSWTCHACTS];

/* table to keep track of the pids belonging to lazy contexts */
STATIC lwp_t *LazyPids[MAXSWTCHACTS];

/*
 * This file contains code for manipulating special context
 * information that is saved/restored across context switches
 * only for those lwps that request it (via lwp_cntxinit).
 * Each context kind should provide routines to
 * save into and restore from the context.
 * Context is saved and restored lazily: only if a new thread uses
 * the same special context type is an actual swutch done.
 * Pure data may be stored in a context by specifying
 * null save and restore routiines.
 *
 * PRIMITIVES contained herein:
 * lwp_ctxinit(tid, ctx)
 * lwp_ctxremove(tid, ctx)
 * lwp_ctxset(saveproc, restoreproc, ctxsize, optimize)
 * lwp_ctxmemget(mem, tid, ctx)
 * lwp_ctxmemset(mem, tid, ctx)
 */

/*
 * lwp_ctxinit -- PRIMITIVE.
 * Announce that a given lwp is to have a special context
 * save/restored across its context switches.
 * We zero the memory.
 */
int
lwp_ctxinit(tid, ctx)
	thread_t tid;		/* thread with special contexts */
	int ctx;		/* type of context */
{
	lwp_t *pid = (lwp_t *)(tid.thread_id);
	register context_t *context;
	register caddr_t ctxmem;
	register __queue_t *cq;
	register int size;

	LWPINIT();
	LOCK();
	for (cq = FIRSTQ(&pid->lwp_ctx); cq != QNULL; cq = cq->q_next) {
		if (ctx == ((context_t *)cq)->ctx_type) {
			TERROR(LE_INUSE);
		}
	}
	GETCHUNK((context_t *), context, CtxType);
	context->ctx_type = ctx;
	GETCHUNK((caddr_t), ctxmem, SwtchActs[ctx].cntxt_memtype);
	context->ctx_mem = ctxmem;
	size = MEMSIZE(SwtchActs[ctx].cntxt_memtype);
	bzero(ctxmem, (u_int)size);
	INS_QUEUE(&pid->lwp_ctx, context);
	UNLOCK();
	return (0);
}

/*
 * lwp_ctxremove -- PRIMITIVE.
 * Remove a previously-established special context for a given thread.
 */
int
lwp_ctxremove(tid, ctx)
	thread_t tid;		/* thread with special contexts */
	int ctx;		/* type of context */
{
	register lwp_t *pid = (lwp_t *)(tid.thread_id);
	register __queue_t *cq;

	LOCK();
	CLR_ERROR();
	for (cq = FIRSTQ(&pid->lwp_ctx); cq != QNULL; cq = cq->q_next) {
		if (ctx == ((context_t *)cq)->ctx_type) {
			/*
			 * inefficient since this loops too, but shouldn't
			 * matter much since this operation is rare and
			 * queues should be short.
			 */
			REMX_ITEM(&pid->lwp_ctx, cq);
			FREECHUNK(ctx, ((context_t *)cq)->ctx_mem);
			if (LazyCntxts[ctx] == ((context_t *)cq)) {
				LazyCntxts[ctx] = CONTEXTNULL;
				LazyPids[ctx] = LWPNULL;
			}
			FREECHUNK(CtxType, ((context_t *)cq));
			UNLOCK();
			return (0);
		}
	}
	TERROR(LE_NONEXIST);
}

/*
 * lwp_ctxmemget -- PRIMITIVE.
 * Return the memory associated with a context.
 */
int
lwp_ctxmemget(mem, tid, ctx)
	register caddr_t mem;
	thread_t tid;
	int ctx;
{
	lwp_t *pid = (lwp_t *)(tid.thread_id);
	register __queue_t *q;
	register caddr_t ctxmem;
	register int size;

	q = FIRSTQ(&pid->lwp_ctx);
	while (q != QNULL) {
		if (ctx == ((context_t *)q)->ctx_type) {
			ctxmem = ((context_t *)q)->ctx_mem;
			size = MEMSIZE(SwtchActs[ctx].cntxt_memtype);
			bcopy(ctxmem, mem, (u_int)size);
			break;
		}
		q = q->q_next;
	}
	return (0);
}


/*
 * lwp_ctxmemset -- PRIMITIVE.
 * Initialize the memory associated with a context.
 */
int
lwp_ctxmemset(mem, tid, ctx)
	register caddr_t mem;
	thread_t tid;
	int ctx;
{
	lwp_t *pid = (lwp_t *)(tid.thread_id);
	register __queue_t *q;
	register caddr_t ctxmem;
	register int size;

	q = FIRSTQ(&pid->lwp_ctx);
	while (q != QNULL) {
		if (ctx == ((context_t *)q)->ctx_type) {
			ctxmem = ((context_t *)q)->ctx_mem;
			size = MEMSIZE(SwtchActs[ctx].cntxt_memtype);
			bcopy(mem, ctxmem, (u_int)size);
			break;
		}
		q = q->q_next;
	}
	return (0);
}

/*
 * Define a switchable context.
 * The cookie returned is used to subsequently tell the nugget that a given
 * lwp will use this kind of context.
 */
int
lwp_ctxset(saveproc, restoreproc, ctxsize, optimize)
	void (*saveproc)();
	void (*restoreproc)();
	unsigned int ctxsize;
	int optimize;		/* FALSE if switching is forced each time */
{
	static int index = 0;

	register int cookie = index++;

	CLR_ERROR();
	if (cookie >= MAXSWTCHACTS) {
		SET_ERROR(__Curproc, LE_NOROOM);
		return (-1);
	}
	SwtchActs[cookie].cntxt_save = saveproc;
	SwtchActs[cookie].cntxt_restore = restoreproc;
	SwtchActs[cookie].cntxt_memtype =
	  __allocinit(ctxsize, MAXCTX, IFUNCNULL, FALSE);
	SwtchActs[cookie].cntxt_optimize = (bool_t)optimize;
	LazyCntxts[cookie] = CONTEXTNULL;
	LazyPids[cookie] = LWPNULL;
	return (cookie);
}

void
__init_ctx()
{
	CtxType = __allocinit(sizeof (context_t), MAXCONTEXT, IFUNCNULL, FALSE);
}

/*
 * called when a process is destroyed to clean up any special contexts
 */
void
__freectxt(lwp)
	register lwp_t *lwp;
{
	register qheader_t *q = &(lwp->lwp_ctx);
	register context_t *ctx;
	register int type;

	while (!ISEMPTYQ(q)) {
		REM_QUEUE((context_t *), q, ctx);
		type = ctx->ctx_type;
		FREECHUNK(SwtchActs[type].cntxt_memtype, ctx->ctx_mem);
		if (LazyCntxts[type] == ctx) {
			LazyCntxts[type] = CONTEXTNULL;
			LazyPids[type] = LWPNULL;
		}
		FREECHUNK(CtxType, ctx);
	}
}

/*
 * Save and restore special contexts using lazy evaluation.
 */
void
__ctx_change(old, new)
	lwp_t *old;
	lwp_t *new;
{
	register __queue_t *q;
	register int type;
	register context_t *lazy;
	register void (*func)();
	thread_t oldt, newt;

	newt.thread_id = (caddr_t)new;
	if (new != LWPNULL)
		newt.thread_key = new->lwp_lock;

	/*
	 * save special contexts belonging to process being switched out
	 * by being lazy and just recording the address of the context
	 * to be saved and the thread using it.
	 * Need to do this loop even if old = new, since could have
	 * added contexts during thread execution.
	 */
	if (old != LWPNULL) {
		for (q = FIRSTQ(&((old)->lwp_ctx)); q != QNULL; q = q->q_next) {
			type = ((context_t *)q)->ctx_type;
			func = SwtchActs[type].cntxt_save;
			if (func == VFUNCNULL)
				continue;
			if (!SwtchActs[type].cntxt_optimize) { /* immed. save */
				oldt.thread_id = (caddr_t)old;
				oldt.thread_key = old->lwp_lock;
				func(((context_t *)q)->ctx_mem, oldt, newt);
				LazyCntxts[type] = CONTEXTNULL;
			} else {
				LazyCntxts[type] = (context_t *)q;
				LazyPids[type] = old;
			}
		}
	}

	/*
	 * Restore special contexts belonging to process being switched in.
	 * It's possible that restore is done in the case save was never called.
	 */
	if (new != LWPNULL) {
		for (q = FIRSTQ(&((new)->lwp_ctx)); q != QNULL; q = q->q_next) {
			type = ((context_t *)q)->ctx_type;
			lazy = LazyCntxts[type];
			func = SwtchActs[type].cntxt_save;
			oldt.thread_id = (caddr_t)LazyPids[type];
			if (LazyPids[type] != LWPNULL)
				oldt.thread_key = LazyPids[type]->lwp_lock;
			if ((func != VFUNCNULL) &&
			    (lazy != (context_t *)q) && 
			    (old != LWPNULL) &&
			    (lazy != CONTEXTNULL)) {
				func(lazy->ctx_mem, oldt, newt);
			}

			/*
			 * if thread destroyed, lazy is CONTEXTNULL
			 * and the new process still needs context
			 */
			func = SwtchActs[type].cntxt_restore;
			if ((func != VFUNCNULL) &&
			  (!SwtchActs[type].cntxt_optimize) ||
			    (lazy != (context_t *)q) || 
			    (old == LWPNULL)) {
				func(((context_t *)q)->ctx_mem, oldt, newt);
			}
		}
	}
}
