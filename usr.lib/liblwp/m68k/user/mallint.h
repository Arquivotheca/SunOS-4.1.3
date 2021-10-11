/* @(#) mallint.h 1.1 92/07/30 Copyr 1987 Sun Micro */
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
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * file: mallint.h
 * description:
 *
 * Definitions for malloc.c and friends (realloc.c, memalign.c)
 *
 * The node header structure.  Header info never overlaps with user
 * data space, in order to accommodate the following atrocity:
 *		free(p);
 *		realloc(p, newsize);
 * ... which was historically used to obtain storage compaction as
 * a side effect of the realloc() call, when the block referenced
 * by p was coalesced with another free block by the call to free().
 * 
 * To reduce storage consumption, a header block is associated with
 * free blocks only, not allocated blocks.
 * When a free block is allocated, its header block is put on 
 * a free header block list.
 *
 * This creates a header space and a free block space.
 * The left pointer of a header blocks is used to chain free header
 * blocks together.  New header blocks are allocated in chunks of
 * NFREE_HDRS.
 */

#ifndef _lwp_mallint_h
#define _lwp_mallint_h

#include <malloc.h>

typedef enum {false,true} bool;
typedef struct	freehdr	*Freehdr;
typedef struct	dblk	*Dblk;
typedef unsigned int	uint;

/*
 * Description of a header for a free block
 * Only free blocks have such headers.
 */
struct 	freehdr	{
	Freehdr	left;			/* Left tree pointer */
	Freehdr	right;			/* Right tree pointer */
	Dblk	block;			/* Ptr to the data block */
	uint	size;
};

#define NIL		((Freehdr) 0)
#define	NFREE_HDRS	512		/* Get this many headers at a time */
#define	SMALLEST_BLK	sizeof(struct dblk) 	/* Size of smallest block */
#define NULL		0

/*
 * Description of a data block.  
 * A data block consists of a length word, possibly followed by
 * a filler word for alignment, followed by the user's data.
 * To back up from the user's data to the length word, use
 * (address of data) - ALIGNSIZ;
 */

#ifdef sparc
#define ALIGNSIZ	sizeof(double)
struct	dblk	{
	uint	size;			/* Size of the block */
	uint	filler;			/* filler, for double alignment */
	char	data[ALIGNSIZ];		/* Addr returned to the caller */
};
#endif

#ifdef mc68000
#define ALIGNSIZ	sizeof(uint)
struct	dblk	{
	uint	size;			/* Size of the block */
	char	data[ALIGNSIZ];		/* Addr returned to the caller */
};
#endif


/*
 * weight(x) is the size of a block, in bytes; or 0 if and only if x
 *	is a null pointer.  Note that malloc() and free() should be
 *	prepared to deal with things like zero-length blocks, which
 *	can be introduced by errant programs.
 */

#define	weight(x)	((x) == NIL? 0: (x->size))
#define	roundup(x, y)   ((((x)+((y)-1))/(y))*(y))
#define	nextblk(p, size) ((Dblk) ((char *) (p) + (size)))
#define	max(a, b)	((a) < (b)? (b): (a))
#define	min(a, b)	((a) < (b)? (a): (b))
#define heapsize()	(_ubound - _lbound)
#define misaligned(p)	((unsigned)(p)&3)

extern	int	free();
extern	Freehdr	_root;
extern	char	*_lbound, *_ubound;

extern	int	malloc_debug();

extern	struct mallinfo __mallinfo;

#endif /*!_lwp_mallint_h*/
