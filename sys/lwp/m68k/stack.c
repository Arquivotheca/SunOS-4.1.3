#include <lwp/common.h>
#include <lwp/queue.h>
#include <machlwp/machdep.h>
#include <lwp/cntxt.h>
#include <lwp/lwperror.h>
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
	extern int maxasynchio_cached;

	LWPINIT();
	ERROR(CacheInit, LE_REUSE);
	CacheInit = TRUE;
	if (numstks == 0) {
		numstks = 1;
	}

	/* add in sizeof stkalign_t * for reaper info */
	minstksz += (STKOVERHEAD + REDZONE + sizeof (stkalign_t *));
	StkAllocSize = minstksz;
	StkType = __allocinit((unsigned)minstksz, maxasynchio_cached,
	    IFUNCNULL, FALSE);
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
	GETCHUNK((caddr_t), sp, StkType);	/* sp points to low mem */
	bottom = (stkalign_t *)sp;
	sp += StkAllocSize - sizeof (freestk_t); /* room for info to free stk */
	((freestk_t *)sp)->free_bottom = bottom;
	((freestk_t *)sp)->free_next = 0;
	bzero((caddr_t)bottom, REDZONE);	/* set up software redzone */
	return ((stkalign_t *)sp);
}

/*
 * lwp_datastk() -- PRIMITIVE.
 * return a new, cached, red-zone-protected stack, suitable for
 * use with lwp_create(). Also, initialize area above stack with
 * "data" consisting of "dsize" bytes and return the location of
 * "data" in "addr". We align data universally.
 * Usage:
 *	sp = lwp_datastk(buf, sizeof (buf), &loc);
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
	GETCHUNK((caddr_t), sp, StkType);	/* sp points to low mem */
	bottom = (stkalign_t *)sp;

	/* subtract the stkalign_t to account for any alignment we may do */
	sp += StkAllocSize - sizeof (freestk_t) - dsize - sizeof (stkalign_t);

	/* align so freestk_t will fit ok. STACKALIGN is general. */
	sp = (caddr_t)STACKALIGN(sp);
	((freestk_t *)sp)->free_bottom = bottom;
	((freestk_t *)sp)->free_next = 0;
	stack = sp;
	sp += sizeof (freestk_t);

	/* align to any type */
	sp = (caddr_t)STACKALIGN(sp + sizeof (stkalign_t));
	*addr = sp;
	if (data != CHARNULL) {
		bcopy(data, sp, (u_int)dsize);
	}
	return ((stkalign_t *)stack);
}

void
return_stk(sp)
	caddr_t sp;
{
	register freestk_t *freestk;

	freestk = (freestk_t *)sp;	
	FREECHUNK(StkType, freestk->free_bottom);
}
