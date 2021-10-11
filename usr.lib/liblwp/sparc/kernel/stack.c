#include <lwp/common.h>
#include <lwp/queue.h>
#include <lwp/asynch.h>
#include <machlwp/machsig.h>
#include <machlwp/machdep.h>
#include <lwp/cntxt.h>
#include <lwp/lwperror.h>
#include <lwp/message.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/alloc.h>
#include <lwp/condvar.h>
#include <lwp/monitor.h>
#include <sys/mman.h>

#ifndef lint
SCCSID(@(#) stack.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

typedef struct freestk_t {
	caddr_t free_next;		/* next stack in chain */
	stkalign_t *free_bottom;	/* address of memory for freeing */
} freestk_t;
STATIC caddr_t CacheStacks;	/* List of allocated stacks for freeing */
STATIC bool_t CacheInit = FALSE;	/* TRUE if cache stack allocated */
STATIC int StkType;		/* cookie for process stack caches */
STATIC int StkAllocSize;	/* actual size (bytes) of cached stacks */
#ifndef KERNEL
thread_t __Reaper = {CHARNULL, 0};	/* reaper thread */
#endif KERNEL
void return_stk();
#define	REDZONE	(sizeof (stkalign_t *) * 3)	/* bytes in software redzone */

/*
 * establish a cache of non-redzone-protected stacks. Add enough room
 * for any overhead.
 */
int
lwp_setstkcache(minstksz, numstks)
	register int minstksz; /* minimum size (in bytes) of cached stacks */
	int numstks;	/* number of cached stacks */
{
#ifndef KERNEL
	static stkalign_t reaperstk[1000];
	extern void stkreaper();
#endif !KERNEL

	LWPINIT();
	LOCK();
	ERROR(CacheInit, LE_REUSE);
	CacheInit = TRUE;
	if (numstks == 0) {
		numstks = 1;
	}

	/* add in sizeof stkalign_t * for reaper info */
	minstksz += (STKOVERHEAD + REDZONE + sizeof (stkalign_t *));
	StkAllocSize = minstksz;
	StkType = __allocinit((unsigned)minstksz, 32, IFUNCNULL, FALSE);
	__alloc_guess(StkType, 1);
	CacheStacks = CHARNULL;
	UNLOCK();
#ifndef KERNEL
	(void)lwp_create(&__Reaper, stkreaper, __MaxPrio,
	  LWPSERVER | LWPNOLASTRITES, STACKTOP(reaperstk, 1000), 0);
#endif
	return (minstksz);
}

/*
 * return a new, cached, software-red-zone-protected stack.
 * the redzone checking is enabled by routines in lwputil.c
 * and is eitherd done at context switch time or at procedure call
 * time (the latter by using the CHECK macro).
 */
stkalign_t *
lwp_newstk()
{
	register caddr_t sp;
	register stkalign_t *bottom;

	if (!CacheInit) {
		__panic("stack cache not initialized");	/* use default?? */
		/* NOTREACHED */
	}
	LOCK();
	GETCHUNK((caddr_t), sp, StkType);	/* sp points to low mem */
	bottom = (stkalign_t *)sp;
	sp += StkAllocSize - sizeof (freestk_t); /* room for info to free stk */
	((freestk_t *)sp)->free_bottom = bottom;
	((freestk_t *)sp)->free_next = CacheStacks;
	CacheStacks = sp;
	bzero((caddr_t)bottom, REDZONE);	/* set up software redzone */
	UNLOCK();
	return ((stkalign_t *)sp);
}

/*
 * lwp_datastk() -- PRIMITIVE.
 * return a new, cached, red-zone-protected stack, suitable for
 * use with lwp_create(). Also, initialize area above stack with
 * "data" consisting of "dsize" bytes and return the location of
 * "data" in "addr". We align data universally.
 * Usage:
 *	sp = lwp_datastk(buf, sizeof(buf), &loc);
 *	lwp_create(&th, pc, prio, 0, sp, 1, loc);
 */
stkalign_t *
lwp_datastk(data, dsize, addr)
	register caddr_t data;
	register int dsize;
	caddr_t *addr;
{
	register caddr_t sp, stack;
	register stkalign_t *bottom;

	if (!CacheInit) {
		__panic("stack cache not initialized");	/* use default?? */
		/* NOTREACHED */
	}
	LOCK();
	GETCHUNK((caddr_t), sp, StkType);	/* sp points to low mem */
	bottom = (stkalign_t *)sp;

	/* subtract the stkalign_t to account for any alignment we may do */
	sp += StkAllocSize - sizeof(freestk_t) - dsize - sizeof(stkalign_t);

	/* align so freestk_t will fit ok. STACKALIGN is general. */
	sp = (caddr_t)STACKALIGN(sp);
	((freestk_t *)sp)->free_bottom = bottom;
	((freestk_t *)sp)->free_next = CacheStacks;
	stack = CacheStacks = sp;
	sp += sizeof(freestk_t);

	/* align to any type */
	sp = (caddr_t)STACKALIGN(sp + sizeof(stkalign_t));
	*addr = sp;
	if (data != CHARNULL) {
		bcopy(data, sp, (u_int)dsize);
	}
	UNLOCK();
	return ((stkalign_t *)stack);
}

#ifndef KERNEL
/*
 * Reap stack memory when thread dies.
 */
STATIC void
stkreaper()
{
	thread_t aid;
	eventinfo_t *event;
	int argsz;
	event_t lastrites;
	register caddr_t sp;

	(void)agt_create(&aid, LASTRITES, (caddr_t)&lastrites);
	for(;;) {
		(void)msg_recv(&aid, (caddr_t *)&event, &argsz,
		  (caddr_t *)0, (int *)0, INFINITY);
		sp = (caddr_t)(event->eventinfo_code);
		if (sp != CHARNULL) 
			return_stk(sp);	
		(void)msg_reply(aid);
	}
}
#endif !KERNEL

void
return_stk(sp)
	register caddr_t sp;
{
	register caddr_t frestk, prev;

	LOCK();		/* cheaper than using monitor */
	/* verify that this is really an allocated stack */
	for (prev = CHARNULL, frestk = CacheStacks; 
	    frestk != CHARNULL;
	    prev = frestk,
	    frestk = ((freestk_t *)frestk)->free_next) {
		if (frestk == sp) {
			if (prev == CHARNULL)
				CacheStacks = ((freestk_t *)frestk)->free_next;
			else
				((freestk_t *)prev)->free_next =
				  ((freestk_t *)frestk)->free_next;
			FREECHUNK(StkType,
			  ((freestk_t *)frestk)->free_bottom);
			break;
		}
	}
	UNLOCK();
}

/*
 * initialize stack code
 */
__init_stks()
{
}
