#include <lwp/common.h>
#include <lwp/queue.h>
#include <machlwp/machdep.h>
#include <lwp/lwperror.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/condvar.h>
#include <lwp/monitor.h>
#include <lwp/libc/libc.h>
#include <malloc.h>
#include <sys/mman.h>

static struct mon_t monnull = {CHARNULL, 0};
static struct cv_t cvnull = {CHARNULL, 0};

#ifndef lint
SCCSID(@(#) Locore.c 1.1 92/07/30);
#endif lint

lowinit()
{
	extern stkalign_t *lwp_newstk();
	extern void pod_setexit();
	extern void __asmdebug();
	extern void __dumpsp();
	extern void __lwpkill();
	extern void lwp_perror();
	extern stkalign_t *lwp_datastk();
#ifndef _MAP_NEW	
	extern bool_t CacheInit;
	CacheInit = FALSE;
#endif _MAP_NEW

	lowinit();
	(void)lwp_create((thread_t *)0, VFUNCNULL, 0, 0, STACKNULL, 0);
	__IdleStack[0] = __IdleStack[0];
	__NuggetSP = __NuggetSP;
	(void)mon_create(&monnull);
	(void)lwp_errstr();
	(void)mon_exit(monnull);
	(void)lwp_setpri(THREADNULL, 0);
	(void)lwp_sleep(POLL);
	(void)lwp_suspend(THREADNULL);
	(void)cv_create(&cvnull, monnull);
	(void)cv_destroy(cvnull);
	(void)cv_wait(cvnull);
	(void)cv_notify(cvnull);
	(void)cv_broadcast(cvnull);
	lwp_perror("");
	(void)lwp_checkstkset(THREADNULL, CHARNULL);
	(void)lwp_stkcswset(THREADNULL, CHARNULL);
	(void)lwp_fpset(THREADNULL);
	(void)lwp_libcset(THREADNULL);
	(void)lwp_ctxremove(THREADNULL, 0);
	debug_nugget(0, CPTRNULL);
	(void)mon_destroy(monnull);
	(void)lwp_join(THREADNULL);
	mallinfo();
	mallopt();
	(void)lwp_setstkcache(0, 0);
	(void)lwp_datastk(CHARNULL, 0, (caddr_t *)0);
	(void)lwp_newstk();
	__asmdebug(0, 1, 2, 3, 4, 5);
	__dumpsp();
	(void)lwp_ctxmemget(CHARNULL, THREADNULL, 0);
	(void)lwp_setregs(THREADNULL, (machstate_t *)0);
	(void)lwp_getregs(THREADNULL, (machstate_t *)0);
	__lwpkill();
}
