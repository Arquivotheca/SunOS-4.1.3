/* @(#)mallint.h 1.1 92/07/30 SMI*/

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

extern	Freehdr	_root;
extern	char	*_lbound, *_ubound;
extern	int	malloc_debug();

extern	struct mallinfo __mallinfo;
