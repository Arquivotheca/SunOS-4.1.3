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
#include "condvar.h"
#include "monitor.h"
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
thread_t __Reaper = {CHARNULL, 0};	/* reaper thread */

#ifdef _MAP_NEW			/* new vm is available */
#ifndef	PROT_NONE
#define	PROT_NONE	0	/* no permissions */
#endif PROT_NONE

STATIC int PageSize;		/* page size (bytes) (always power of 2) */

/*
 * lwp_setstkcache(minstksz, numstks) -- PRIMITIVE.
 * establish a cache of preallocated redzone-protected
 * stacks. Return the actual size of the stacks (not including the redzone).
 * The overhead for initializing a stack is tacked on as well.
 */
int
lwp_setstkcache(minstksz, numstks)
	register int minstksz; /* minimum size (in bytes) of cached stacks */
	int numstks;	/* number of cached stacks */
{
	static stkalign_t reaperstk[1000];
	register int pagesize;
	extern int setredzones();
	extern void stkreaper();

	LWPINIT();
	LOCK();
	ERROR(CacheInit, LE_REUSE);
	CacheInit = TRUE;
	if (numstks == 0)
		numstks = 1;
	pagesize = PageSize;

	/* add in sizeof stkalign_t * for reaper info */
	minstksz += STKOVERHEAD + sizeof (stkalign_t *);
	if ((minstksz % pagesize) != 0) {	/* round up to page */
		minstksz &= ~(pagesize - 1);
		minstksz += pagesize;
	}
	StkAllocSize = minstksz;
	minstksz += pagesize;	/* add red zone page */
	StkType = __allocinit((unsigned)minstksz, numstks, setredzones, FALSE);
	CacheStacks = CHARNULL;
	UNLOCK();
	(void)lwp_create(&__Reaper, stkreaper, __MaxPrio,
	  LWPSERVER | LWPNOLASTRITES,
	  STACKTOP(reaperstk, sizeof(reaperstk)/sizeof(stkalign_t)),
	  0);
	return (StkAllocSize);
}

/*
 * lwp_newstk() -- PRIMITIVE.
 * return a new, cached, red-zone-protected stack, suitable for
 * use with lwp_create(). Note that lwp_create() saves sp
 * exactly as we specified, and THEN aligns it.
 * This lets us safely use the arguments we store on the stack
 * since we get their exact address on LASTRITES.
 * Since StkAllocSize mod pagesize == 0, we assume that
 * adding it to a universally-aligned (from GETCHUNK) piece of memory
 * gives something we can safely stuff a freestk_t into.
 */
stkalign_t *
lwp_newstk()
{
	register caddr_t sp;
	register stkalign_t *bottom;

	if (!CacheInit) {
		/* panic forces user to see error */
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
	UNLOCK();
	return ((stkalign_t *)sp);
}

/*
 * lwp_datastk() -- INTERNAL PRIMITIVE.
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
	if (data != CHARNULL)
		bcopy(data, sp, dsize);
	UNLOCK();
	return ((stkalign_t *)stack);
}

/*
 * set up redzones for a chunk of memory
 */
int
setredzones(start, number, size)
	int *start;	/* start of a chunk of cached memory */
	int number;	/* number of stacks */
	int size;	/* size of stack (including red zone) */
{
	register int i;
	register caddr_t sp;
	register caddr_t redpage;
	register int *next;
	extern int errno;
	int olderrno;
	int err;
	int tot;

	sp = (caddr_t)start + PageSize;	/* start past redzone */
	next = (int *)sp;
	for (i = 0; i < number; i++) {
		redpage = sp + (i * size) - PageSize;
		olderrno = errno;
		err = mprotect(redpage, PageSize, PROT_NONE);
		errno = olderrno;
		if (err < 0)
			perror("mprotect");
		if (i != (number - 1)) {
			tot = (int)next + size;
			*next = tot;
			next = (int *)tot;
		} else {
			*next = 0;
		}
	}
	return ((int)sp);
}

#else _MAP_NEW

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
	static stkalign_t reaperstk[1000];
	extern void stkreaper();

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
	StkType = __allocinit((unsigned)minstksz, numstks, IFUNCNULL, FALSE);
	CacheStacks = CHARNULL;
	UNLOCK();
	(void)lwp_create(&__Reaper, stkreaper, __MaxPrio,
	  LWPSERVER | LWPNOLASTRITES, STACKTOP(reaperstk, 1000), 0);
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

#endif _MAP_NEW

/*
 * Reap stack memory when thread dies.
 */
STATIC void
stkreaper()
{
	thread_t aid;
	eventinfo_t *event;
	int argsz;
	eventinfo_t lastrites;
	register caddr_t sp;
	register caddr_t frestk;
	register caddr_t prev;

	(void)agt_create(&aid, LASTRITES, (caddr_t)&lastrites);
	for(;;) {
		(void)msg_recv(&aid, (caddr_t *)&event, &argsz,
		  (caddr_t *)0, (int *)0, INFINITY);
		sp = (caddr_t)(event->eventinfo_code);
		if (sp != CHARNULL) {
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
		(void)msg_reply(aid);
	}
}

/*
 * initialize stack code
 */
__init_stks()
{
#ifdef _MAP_NEW
	PageSize = getpagesize();
#endif _MAP_NEW
}
