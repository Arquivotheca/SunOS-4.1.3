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
#include "lwperror.h"
#include "cntxt.h"
#include "message.h"
#include "process.h"
#include "schedule.h"
#include "alloc.h"
#include "condvar.h"
#include "monitor.h"
#include "agent.h"
#include "except.h"
#include <malloc.h>
#include <sys/mman.h>

static struct mon_t monnull = {CHARNULL, 0};
static struct cv_t cvnull = {CHARNULL, 0};

#ifndef lint
SCCSID(@(#) Locore.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

lowinit()
{

	extern stkalign_t *lwp_newstk();
	extern void pod_setexit();
	extern void __real_sig();
	extern void __lwpkill();
	extern void (*sigset())();	/* XXX */
	extern void lwp_perror();
	extern stkalign_t *lwp_datastk();
#ifndef _MAP_NEW	
	extern bool_t CacheInit;
	CacheInit = FALSE;
#endif _MAP_NEW

	sigset();		/* XXX */
	(void) sighold();	/* XXX */
	(void) sigrelse();	/* XXX */
	(void) sigignore();	/* XXX */
	lowinit();
	(void)lwp_create((thread_t *)0, VFUNCNULL, 0, 0, STACKNULL, 0);
	__IdleStack[0] = __IdleStack[0];
	__NuggetSP = __NuggetSP;
	__sigtrap();
	(void)mon_create(&monnull);
	(void)cv_create(&cvnull, monnull);
	(void)lwp_errstr();
	__fake_sig();
	(void)mon_exit(monnull);
	(void)pod_setmaxpri(0);
	(void)pod_getmaxsize();
	(void)pod_getmaxpri();
	(void)lwp_enumerate(&THREADNULL, 0);
	(void)lwp_getstate(THREADNULL, (statvec_t *)0);
	(void)lwp_resched(0);
	(void)lwp_setpri(THREADNULL, 0);
	(void)lwp_sleep(POLL);
	(void)lwp_suspend(THREADNULL);
	(void)lwp_resume(THREADNULL);
	pod_setexit(0);
	(void)cv_destroy(cvnull);
	(void)cv_wait(cvnull);
	(void)cv_notify(cvnull);
	(void)cv_send(cvnull, THREADNULL);
	(void)cv_broadcast(cvnull);
	(void)cv_enumerate(&cvnull, 0);
	(void)cv_waiters(cvnull, &THREADNULL, 0);
	(void)agt_trap(0);
	(void)agt_destroy(THREADNULL);
	(void)agt_enumerate(&THREADNULL, 0);
	(void)agt_msgs(THREADNULL, CPTRNULL, 0);
	(void)msg_send(THREADNULL, CHARNULL, 0, CHARNULL, 0);
	(void)msg_enumsend(&THREADNULL, 0);
	(void)msg_enumrecv(&THREADNULL, 0);
	(void)msg_status(THREADNULL, CPTRNULL, 0);
	lwp_perror("");
	(void)lwp_checkstkset(THREADNULL, CHARNULL);
	(void)lwp_stkcswset(THREADNULL, CHARNULL);
	(void)lwp_datastk(CHARNULL, 0, (caddr_t *)0);
	(void)lwp_ctxremove(THREADNULL, 0);
	(void)lwp_fpset(THREADNULL);
	(void)lwp_libcset(THREADNULL);
	debug_nugget(0, CPTRNULL);
	(void)mon_destroy(monnull);
	(void)mon_enter(monnull);
	(void)mon_enumerate(&monnull, 0);
	(void)mon_exit1(&monnull);
	(void)mon_waiters(monnull, &THREADNULL, &THREADNULL, 0);
	(void)mon_cond_enter(monnull);
	(void)mon_break(monnull);
	(void)exc_uniqpatt();
	(void)exc_handle(0, CFUNCNULL, CHARNULL);
	(void)exc_on_exit(VFUNCNULL, CHARNULL);
	(void)exc_unhandle();
	(void)exc_notify(0);
	(void)lwp_join(THREADNULL);
	(void)__exchelpclean();
	(void)__exc_trap(0);
	mallinfo();
	mallopt();
	(void)lwp_setstkcache(0, 0);
	(void)lwp_newstk();
	(void)__real_sig(0, 0, (struct sigcontext *)0, CHARNULL, (int *)0, 0L);
	(void)pod_exit(0);
	(void)lwp_ctxmemget(CHARNULL, THREADNULL, 0);
	(void)lwp_ping(THREADNULL);
	(void)lwp_setregs(THREADNULL, (machstate_t *)0);
	(void)lwp_getregs(THREADNULL, (machstate_t *)0);
	__lwpkill();
	__Mem = __Mem;
}
