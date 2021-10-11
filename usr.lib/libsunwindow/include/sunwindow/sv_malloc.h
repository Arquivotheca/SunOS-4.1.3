#ifndef	sv_malloc_defined
#define sv_malloc_defined
#ifndef lint
#ifdef sccs
static char  sccsid[] = "@(#)sv_malloc.h 1.1 92/07/30";
#endif
#endif

/*
 *  Copyright (c) 1989 by Sun Microsystems Inc.
 */





char	* sv_malloc(/* int requested_size */),
	* sv_calloc(/* int how_many_blocks, int size_per_block */),
	* sv_realloc(/* char * ptr, unsigned int requested_size */);





#endif sv_malloc_defined
