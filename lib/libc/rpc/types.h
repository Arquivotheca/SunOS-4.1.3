/*	@(#)types.h 1.1 92/07/30 SMI	*/

/*
 * Rpc additions to <sys/types.h>
 */

#ifndef	__rpc_types_h
#define	__rpc_types_h

#define	bool_t	int
#define	enum_t	int
#define	__dontcare__	-1

#ifndef	FALSE
#	define	FALSE	(0)
#endif

#ifndef	TRUE
#	define	TRUE	(1)
#endif

#ifndef	NULL
#	define	NULL 0
#endif

#ifndef	KERNEL
#include <malloc.h>
#define	mem_alloc(bsize)	malloc(bsize)
#define	mem_free(ptr, bsize)	free(ptr)
#else
extern char *kmem_alloc();
#define	mem_alloc(bsize)	kmem_alloc((u_int)bsize)
#define	mem_free(ptr, bsize)	kmem_free((caddr_t)(ptr), (u_int)(bsize))
#endif

#include <sys/types.h>
#include <sys/time.h>

#endif	/* !__rpc_types_h */
