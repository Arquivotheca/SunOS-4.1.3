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
#include <lwp/cntxt.h>
#include <lwp/lwperror.h>
#include <lwp/message.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/alloc.h>
#include <lwp/condvar.h>
#include <lwp/monitor.h>
#ifndef lint
SCCSID(@(#) alloc.c 1.1 92/07/30 Copyr 1986 Sun Micro);
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

/* guess on how much to malloc next */
STATIC int CacheGuess[MAXMEMTYPES+MAXINTMEMTYPES];

int CacheCurrent[MAXMEMTYPES+MAXINTMEMTYPES];

int CacheAlloc[MAXMEMTYPES+MAXINTMEMTYPES];

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
	register int i;
	register int nitems;
	register int itemsize;
	int tot;

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
	nitems = CacheGuess[cookie];
	CacheCurrent[cookie] += nitems;
	CacheAlloc[cookie] += nitems;
	itemsize = __CacheSize[cookie];

	LWPTRACE(tr_ALLOC, 1, ("malloc'ing %d\n", nitems*itemsize));
	start = (int *)MEM_ALLOC(nitems * itemsize);
	if (start == INTNULL) /* no more room */
		__panic("out of virtual memory");

	/* do any special processing on the memory, like set up red zones */
	if (CacheFuncs[cookie] != IFUNCNULL) {
		__MemType[cookie] =
		   (int *)CacheFuncs[cookie](start, nitems, itemsize);
	} else {
		__MemType[cookie] = start;

		/* link together using first word */
		for (i = 0; i < nitems - 1; i++) {
			tot = (int)start + itemsize;
			*start = tot;
			start = (int *)tot;
		}
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
	int nitems;		/* number of chunks to allocate */
	int (*func)();		/* what to do when getting new mem */
	bool_t intmem;		/* TRUE if interrupt memory */
{
	register int cookie = MemCookie++;

	if (cookie >= MAXMEMTYPES+MAXINTMEMTYPES)
		__panic("out of cache slots");

	/* Align */
	__CacheSize[cookie] = STACKALIGN(itemsize + sizeof(stkalign_t) - 1);
	CacheGuess[cookie] = nitems;
	CacheFuncs[cookie] = func;
	__IsIntMem[cookie] = (intmem) ? 1 : 0;
	memgrow(cookie);
	return (cookie);
}

int
__alloc_guess(cookie, nitems)
	int cookie;
	int nitems;
{
	CacheGuess[cookie] = nitems;
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
