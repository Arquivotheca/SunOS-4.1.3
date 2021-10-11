/* @(#) alloc.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */
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
#ifndef _lwp_alloc_h
#define _lwp_alloc_h

/*
 * fast inline code to get/free some memory
 * usage:
 * GETCHUNK((cast), val, cookie) vs. val = (cast)getchunk(cookie);
 * NR_GETCHUNK((cast), val, cookie) (doesn't use registers)
 * FREECHUNK(cookie, chunk_address);
 */

extern int **__Mem;
extern int *__MemType[];
extern int __CacheSize[];
extern int CacheCurrent[];
extern int CacheAlloc[];
extern int __allocinit(/*itemsize, nitems, func*/);
extern int __alloc_guess(/*cookie, nitems*/);
extern int *__getchunk(/*cookie*/);

/*
 * Obtain a chunk of cached memory.
 */
#define GETCHUNK(cast, val, cookie) {			\
	register int **p = &__MemType[(cookie)];	\
	val = cast *p;					\
	if (val != cast 0)				\
		*p = (int *)**p;			\
	else						\
		val = cast __getchunk((cookie));	\
	CacheAlloc[cookie]--;				\
}

/*
 * get a chunk without using a non-volatile register temp (procedure
 * calls ok since they restore regs upon return).
 */
#define NR_GETCHUNK(cast, val, cookie) {		\
	__Mem = &__MemType[cookie];			\
	val = cast *__Mem;				\
	if (val != cast 0)				\
		*__Mem = (int *)**__Mem;		\
	else						\
		val = cast __getchunk((cookie));	\
}

#define FREECHUNK(cookie, addr) {			\
	*((int *)(addr)) = (int) __MemType[cookie];	\
	__MemType[cookie] = (int *)(addr);		\
	CacheAlloc[cookie]++;				\
}

/* size (in bytes) of a given memory type: constant with interrupt memory */
#define	MEMSIZE(cookie) (__CacheSize[(cookie)])

/*
 * macros to hide which malloc we're using
 * MEM_ALLOC must return an address that can be used as a stack.
 * Thus, we use valloc. In the kernel, kmem_alloc is sufficient since
 * we won't use mmap on stacks.
 */
extern caddr_t valloc();
#define MEM_ALLOC(bsize)		valloc((unsigned)(bsize))
#define MEM_FREE(ptr, bsize)		(void) free((caddr_t)(ptr))

#endif /*!_lwp_alloc_h*/
