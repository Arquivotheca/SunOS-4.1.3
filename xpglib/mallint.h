/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	@(#)mallint.h 1.1 92/07/30 SMI; */

/*
	template for the header
*/
struct header {
	struct header *nextblk;
	struct header *nextfree;
	struct header *prevfree;
#ifdef  pdp11
	struct header *dummy;	   /* pad to a multple of 4 bytes */
#endif
};
/*
	template for a small block
*/
struct lblk  {
	union {
		struct lblk *nextfree;  /* the next free little block in this
					   holding block.  This field is used
					   when the block is free */
		struct holdblk *holder; /* the holding block containing this
					   little block.  This field is used
					   when the block is allocated */
	}  header;
	char byte;		    /* There is no telling how big this
					   field freally is.  */
};
/* 
	template for holding block
*/
struct holdblk {
	struct holdblk *nexthblk;   /* next holding block */
	struct holdblk *prevhblk;   /* previous holding block */
	struct lblk *lfreeq;	/* head of free queue within block */
	struct lblk *unused;	/* pointer to 1st little block never used */
	int blksz;		/* size of little blocks contained */
#ifdef pdp11
	int pad;		/* pad to an even # of words */
#endif
	char space[1];		/* start of space to allocate.
				   This must be on a word boundary */
};

/*
	The following manipulate the free queue

		DELFREEQ will remove x from the free queue
		ADDFREEQ will add an element to the head
			 of the free queue.
		MOVEHEAD will move the free pointers so that
			 x is at the front of the queue
*/
#define ADDFREEQ(x)       (x)->prevfree = &(freeptr[0]);\
				(x)->nextfree = freeptr[0].nextfree;\
				freeptr[0].nextfree->prevfree = (x);\
				freeptr[0].nextfree = (x);\
				assert((x)->nextfree != (x));\
				assert((x)->prevfree != (x));
#define DELFREEQ(x)       (x)->prevfree->nextfree = (x)->nextfree;\
				(x)->nextfree->prevfree = (x)->prevfree;\
				assert((x)->nextfree != (x));\
				assert((x)->prevfree != (x));
#define MOVEHEAD(x)       freeptr[1].prevfree->nextfree = \
					freeptr[0].nextfree;\
				freeptr[0].nextfree->prevfree = \
					freeptr[1].prevfree;\
				(x)->prevfree->nextfree = &(freeptr[1]);\
				freeptr[1].prevfree = (x)->prevfree;\
				(x)->prevfree = &(freeptr[0]);\
				freeptr[0].nextfree = (x);\
				assert((x)->nextfree != (x));\
				assert((x)->prevfree != (x));
/*
	The following manipulate the busy flag
*/
#define BUSY	1
#define SETBUSY(x)      ((struct header *)((int)(x) | BUSY))
#define CLRBUSY(x)      ((struct header *)((int)(x) & ~BUSY))
#define TESTBUSY(x)     ((int)(x) & BUSY)
/*
	The following manipulate the small block flag
*/
#define SMAL	2
#define SETSMAL(x)      ((struct lblk *)((int)(x) | SMAL))
#define CLRSMAL(x)      ((struct lblk *)((int)(x) & ~SMAL))
#define TESTSMAL(x)     ((int)(x) & SMAL)
/*
	The following manipulate both flags.  They must be 
	type coerced
*/
#define SETALL(x)       ((int)(x) | (SMAL | BUSY))
#define CLRALL(x)       ((int)(x) & ~(SMAL | BUSY))
/*
	Other useful constants
*/
#define TRUE    1
#define FALSE   0
#define HEADSZ  sizeof(struct header)   /* size of unallocated block header */
/*      MINHEAD is the minimum size of an allocated block header */
#ifdef pdp11
#define MINHEAD 4
#else
#define MINHEAD sizeof(struct header *)
#endif
#define MINBLKSZ 12		/* min. block size must as big as
				   HEADSZ */
#define BLOCKSZ 2048		/* memory is gotten from sbrk in
				   multiples of BLOCKSZ */
#define GROUND  (struct header *)0
#define LGROUND (struct lblk *)0	/* ground for a queue within a holding
					   block	*/
#define HGROUND (struct holdblk *)0     /* ground for the holding block queue */
/* number of bytes to align to  (must be at least 4, because lower 2 bits
   are used for flags */
#define ALIGNSZ 4
#ifndef NULL
#define NULL    (char *)0
#endif
/*
	Structures and constants describing the holding blocks
*/
#define NUMLBLKS  100   /* default number of small blocks per holding block */
/* size of a holding block with small blocks of size blksz */
#define HOLDSZ(blksz)  (sizeof(struct holdblk) - sizeof(int) + blksz*numlblks)
#ifndef pdp11
#define FASTCT    0	  /* default number of block sizes that can be
				   allocated quickly */
#else
#define FASTCT 0
#endif
#define MAXFAST   ALIGNSZ*FASTCT  /* default maximum size block for fast
				     allocation */


#ifdef debug
#define CHECKQ  checkq();
static
checkq()
{
	register struct header *p;

	p = &(freeptr[0]);
	/* check forward */
	while(p != &(freeptr[1]))       {
		p = p->nextfree;
		assert(p->prevfree->nextfree == p);
	}
	/* check backward */
	while(p != &(freeptr[0]))       {
		p = p->prevfree;
		assert(p->nextfree->prevfree == p);
	}
}
#else
#define CHECKQ
#endif

