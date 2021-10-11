/* Copyright (C) 1986 Sun Microsystems Inc. */

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
#ifndef lint
SCCSID(@(#) alloc.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

/*
 * These memory allocation routines allow fast access to
 * chunks of memory of a given size. Chunks are specified by
 * type (indicated by a magic cookie),
 * so it is possible to have several caches of the
 * same sized chunks. You incur malloc overhead when your
 * cache runs out.
 * Chunks are linked together by the first word of the chunk,
 * so be careful to not rely on the value contained in a chunk
 * after it has been freed.
 * My own pecadillo against using char * as a generic address:
 * if p->val = 1000 for example, a strong typecheck says the value
 * is too large. A pointer to a union {int a; char b; short c; etc.}
 * is probably more appropriate.
 *
 * The interrupt memory allocators will refuse to grow memory
 * beyond the initial allocation because of the possibility
 * of requesting more memory while malloc is being used (note that
 * save_event requires memory so even the LOCK's on malloc don't
 * save us (we would have deadlock)).
 * (We can't protect malloc with a monitor because interrupt code
 * cannot be made to block for any reason).
 *
 * No attempt is made to reclaim or compact memory once allocated.
 *
 */

int **__Mem;				/* for NR_GETCHUNK macro */

/* current free list for each size */
int *__MemType[MAXMEMTYPES+MAXINTMEMTYPES];

/* maximum cache will grow to */
STATIC int MaxCacheSz[MAXMEMTYPES+MAXINTMEMTYPES];

/* current count of elements on list */
int __CacheAlloc[MAXMEMTYPES+MAXINTMEMTYPES];

/* size of item */
int __CacheSize[MAXMEMTYPES+MAXINTMEMTYPES];

/* what to do after allocating new mem */
STATIC int (*CacheFuncs[MAXMEMTYPES+MAXINTMEMTYPES])();

/* for allocating magic cookies */
STATIC int MemCookie;

/*
 * 0 if non-interrupt memory, 1 if interrupt memory, 2 if attempt
 * to grow interrupt memory
 */
STATIC int __IsIntMem[MAXMEMTYPES+MAXINTMEMTYPES];

/* we ran out of memory chunks: get some more */
STATIC void
memgrow(cookie)
	int cookie;	/* type of memory to allocate */
{
	register int *start;
	register int itemsize;

	if (__IsIntMem[cookie] == 1) {
		__IsIntMem[cookie]++;	/* ok to get initial block of memory */
	} else if (__IsIntMem[cookie] == 2) {
		/*
		 * can't grow memory for interrupts since could have
		 * interrupted malloc for example.
		 */
		return;
	}
	LWPTRACE(tr_ALLOC, 1, ("growing %d\n", cookie));
	itemsize = __CacheSize[cookie];
	LWPTRACE(tr_ALLOC, 1, ("malloc'ing %d\n", 1*itemsize));
	start = (int *)MEM_ALLOC(itemsize);
	if (start == INTNULL) /* no more room */
		__panic("out of virtual memory");

	/* do any special processing on the memory, like set up red zones */
	if (CacheFuncs[cookie] != IFUNCNULL) {
		__MemType[cookie] =
		    (int *)CacheFuncs[cookie](start, 1, itemsize);
	} else {
		__MemType[cookie] = start;
		*start = 0;
	}
}

/*
 * Initialize a growable cache of memory chunks of a given size and return
 * cookie to reference this cache.
 * Align to general pointer (stack pointer) so a) at least integer-aligned
 * since each chunk contains an int* as the first object, and b)
 * each client gets a generally-aligned object.
 */
int
__allocinit(itemsize, nitems, func, intmem)
	unsigned int itemsize;	/* size of each chunk */
	int nitems;		/* max number of cache size */
	int (*func)();		/* what to do when getting new mem */
	bool_t intmem;		/* TRUE if interrupt memory */
{
	register int cookie = MemCookie++;

	if (cookie >= MAXMEMTYPES+MAXINTMEMTYPES)
		__panic("out of cache slots");

	/* Align */
	__CacheSize[cookie] = STACKALIGN(itemsize + sizeof (stkalign_t) - 1);
	MaxCacheSz[cookie] = nitems;
	CacheFuncs[cookie] = func;
	__IsIntMem[cookie] = (intmem) ? 1 : 0;
	__MemType[cookie] = 0;
	return (cookie);
}

/*
 * get chunk of a particular type (and therefore size) of memory.
 * Only invoked via macro in alloc.h that checks that memory
 * must be grown.
 */
int *
__getchunk(cookie)
	int cookie;	/* type of memory chunk sought */
{
	register int *p;

	p = __MemType[cookie];
	if (p == INTNULL) {	/* try to get more space */
		memgrow(cookie);
		p = __MemType[cookie];
	}
	if (p != INTNULL)
		__MemType[cookie] = (int *)*p;  /* next element in list */
	return (p);
}


void
__freechunk(cookie, addr)
	int cookie;
	caddr_t  addr;
{
	if (__CacheAlloc[cookie] < MaxCacheSz[cookie]) {
		*((int *)(addr)) = (int) __MemType[cookie];
		__MemType[cookie] = (int *)(addr);
		__CacheAlloc[cookie]++;
	} else  {
		MEM_FREE(addr, __CacheSize[cookie]);
	}
}
