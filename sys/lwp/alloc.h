/* @(#) alloc.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */

#ifndef _lwp_alloc_h
#define	_lwp_alloc_h

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
extern int __CacheAlloc[];
extern int __allocinit();
extern int *__getchunk();
extern void __freechunk();

/*
 * Obtain a chunk of cached memory.
 */
#define	GETCHUNK(cast, val, cookie) {			\
	register int **p; 				\
							\
	p = &__MemType[(cookie)];			\
	val = cast *p;					\
	if (val != cast 0) {				\
		*p = (int *)**p;			\
		__CacheAlloc[cookie]--;			\
	} else						\
		val = cast __getchunk((cookie));	\
}

/*
 * get a chunk w__ithout using a non-volatile register temp (procedure
 * calls ok since they restore regs upon return).
 */
#define	NR_GETCHUNK(cast, val, cookie) {		\
	__Mem = &__MemType[cookie];			\
	val = cast *__Mem;				\
	if (val != cast 0) {				\
		*__Mem = (int *)**__Mem;		\
		__CacheAlloc[cookie]--;			\
	} else						\
		val = cast __getchunk((cookie));	\
}

#define	FREECHUNK(cookie, addr) {			\
		__freechunk((int)cookie, (caddr_t)addr);		\
}

/* size (in bytes) of a given memory type: constant with interrupt memory */
#define	MEMSIZE(cookie) (__CacheSize[(cookie)])

/*
 * macros to hide which malloc we're using
 * MEM_ALLOC must return an address that can be used as a stack.
 * Thus, we use valloc. In the kernel, kmem_alloc is sufficient since
 * we won't use mmap on stacks.
 */
#include <sys/kmem_alloc.h>
#define	MEM_ALLOC(bsize)	new_kmem_alloc((u_int)(bsize), KMEM_SLEEP)
#define	MEM_FREE(ptr, bsize) \
	kmem_free((caddr_t)(ptr), (u_int)(bsize))

#endif /* !_lwp_alloc_h */
