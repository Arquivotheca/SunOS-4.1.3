/*	@(#)stak.h 1.1 92/07/30 SMI; from S5R3.1 1.7	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	UNIX shell
 */

/* To use stack as temporary workspace across
 * possible storage allocation (eg name lookup)
 * a) get ptr from `relstak'
 * b) can now use `pushstak'
 * c) then reset with `setstak'
 * d) `absstak' gives real address if needed
 */
#define		relstak()	(staktop-stakbot)
#define		absstak(x)	(stakbot+Rcheat(x))
#define		setstak(x)	(staktop=absstak(x))
#define		pushstak(c)	(*staktop++=(c))
#define		zerostak()	(*staktop=0)

/* Used to address an item left on the top of
 * the stack (very temporary)
 */
#define		curstak()	(staktop)

/* `usestak' before `pushstak' then `fixstak'
 * These routines are safe against heap
 * being allocated.
 */
#define		usestak()	{locstak();}

/* for local use only since it hands
 * out a real address for the stack top
 */
extern char		*locstak();

/* Will allocate the item being used and return its
 * address (safe now).
 */
#define		fixstak()	endstak(staktop)

/* For use after `locstak' to hand back
 * new stack top and then allocate item
 */
extern char		*endstak();

/* Copy a string onto the stack and
 * allocate the space.
 */
extern char		*cpystak();

/* Copy a string onto the stack, checking for stack overflow
 * as the copy is done.  Same calling sequence as "movstr".
 */
extern char		*movstrstak();

/* Move bytes onto the stack, checking for stack overflow
 * as the copy is done.  Same calling sequence as the C
 * library routine "memcpy".
 */
extern char		*memcpystak();

/* Allocate given ammount of stack space */
extern char		*getstak();

/* Grow the data segment to include a given location */
extern void		growstak();

/* A chain of ptrs of stack blocks that
 * have become covered by heap allocation.
 * `tdystak' will return them to the heap.
 */
extern struct blk	*stakbsy;

/* Base of the entire stack */
extern char		*stakbas;

/* Top of entire stack */
extern char		*brkend;

/* Base of current item */
extern char		*stakbot;

/* Top of current item */
extern char		*staktop;

/* Used with tdystak */
extern char		*savstak();
