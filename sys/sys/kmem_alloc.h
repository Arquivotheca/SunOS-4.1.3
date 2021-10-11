/*	@(#)kmem_alloc.h 1.1 92/07/30	SMI	*/

/*
 * Declarations and definitions for the kernel memory allocator
 */

#ifndef	__sys_kmem_alloc_h
#define	__sys_kmem_alloc_h

#include <sys/types.h>

#ifdef KERNEL
caddr_t	kmem_alloc();
caddr_t	kmem_zalloc();
caddr_t	kmem_fast_alloc();
caddr_t	kmem_fast_zalloc();
caddr_t kmem_resize();
caddr_t	new_kmem_alloc();
caddr_t	new_kmem_zalloc();
caddr_t	new_kmem_fast_alloc();
caddr_t	new_kmem_fast_zalloc();
caddr_t new_kmem_resize();
caddr_t getpages();
void	freepages();
extern caddr_t kernelmap_alloc();
extern caddr_t kernelmap_zalloc();
#endif /* KERNEL */

#define	KMEM_SLEEP	0
#define	KMEM_NOSLEEP	1

#endif	/* !__sys_kmem_alloc_h */
