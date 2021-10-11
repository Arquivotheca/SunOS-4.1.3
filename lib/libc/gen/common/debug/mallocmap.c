#ifndef lint
static	char sccsid[] = "@(#)mallocmap.c	1.1 (Sun) 7/30/92";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * file: mallocmap.c
 * description:
 *
 *	This file contains routines to print maps of the heap and
 *	the free space tree, for diagnosis of memory allocation bugs.
 */

#include "mallint.h"

/*
 * A block with a negative size, a size that is not a multiple
 * of ALIGNSIZ, a size greater than the current extent of the
 * heap, or a size which extends beyond the end of the heap is
 * considered bad.
 */

#define badblksize(p,size)\
( (size) < SMALLEST_BLK \
	|| (size) % ALIGNSIZ != 0 \
	|| (size) > heapsize() \
	|| ((char*)(p))+(size) > _ubound )


#undef	NULL
#define LBUFSIZ	256
#include <stdio.h>

static	depth =	0;
static	char	stdoutbuf[LBUFSIZ];

/* swindle to prevent setbuffer() from calling free() */

static
setlinebuf(f, lbuf)
	FILE *f ;
	char *lbuf;
{
	if (f->_base != (unsigned char *)lbuf) {
		f->_base = NULL; /* prevent setbuffer from calling free() */
		setbuffer(f, (unsigned char*)NULL, 0);
		setbuffer(f, lbuf, LBUFSIZ);
		f->_flag |= (_IOMYBUF|_IOLBF);
	}
}

static
setstdout()
{
	setlinebuf(stdout,stdoutbuf);
}

static
prtree(p)
	Freehdr p;
{
	int n;
	
	depth++;
	if (p != NIL){
		prtree(p->right);
		for (n = 0; n < depth; n++) {
			printf("   ");
		}
		printf("(%#lx): (left = %#lx, right = %#lx, len = %d)\n",
			p->block,
			(p->left ? p->left->block : 0),
			(p->right ? p->right->block : 0),
			p->size );
		prtree(p->left);
	}
	depth--;
}

/*
 * External routine to print the free space tree
 */

prfree()
{
	setstdout();
	printf("\n**** free space traversal:\n");
	prtree(_root);
	printf("\n");
}

/*
 * Determine if a block is in the free tree.
 * Uses a simple binary search.  Assumes blocksize
 * is valid.
 */

static bool
isfreeblk(blk, fp)
	register Dblk	blk;
	register Freehdr fp;
{
	uint	 len; 

	if (_root == NULL) {
		return(false);
	}
	len = blk->size;
	while (weight(fp) >= len) {	
		if (blk == fp->block)
			return(true);
		else if (blk < fp->block)
			fp = fp->left;
		else
			fp = fp->right;
	}
	return(false);
}

/*
 * mallocmap()
 *
 * description:
 *	Print out a memory map.  This routine walks sequentially through
 *	the heap and reports each block's address, size and status (free
 *	or busy).  A block must have a size that is no larger than the
 *	current extent of the heap.
 */

mallocmap()
{
	register char	*status;
	register Dblk	p;
	register uint	size;
	int	errstate;

	if (_lbound == NULL)	/* no allocation yet */
		return; 
	setstdout();
	p = (Dblk) _lbound;
	while (p < (Dblk) _ubound) {
		size = (int) p->size;
		if (badblksize(p,size)) {
			printf("%x: unreasonable size: %d\n", p, size);
			break;
		}
		status = ( isfreeblk(p, _root) ? "free" : "busy" );
		printf("%x: %d bytes: %s\n", p, size, status);
		p = nextblk(p, size);
	}
}
