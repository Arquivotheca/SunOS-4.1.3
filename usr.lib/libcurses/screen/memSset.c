/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)memSset.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.7.1.5 */
#endif

#include "curses.h"
/*
 * additional memory routine to deal with memory areas in units of chtypes.
 */

void memSset (s, c, n)

#if u3b
chtype *s;
chtype c;
int n;
{
	register int count;			/* %r8 */
	register char *from;			/* %r7 */
	register char *to; 			/* %r6 */

	count = (n - 1) * sizeof (chtype);
	if (count > 0) {
		register chtype *sfrom = s;	/* %r5 */

		to = (char *)(sfrom + 1);
		from = (char *)sfrom;
		*sfrom = c;
		asm("	movblkb	%r8, %r7, %r6");
		/*      bcopy (count, to, from) */
	} else if (count == 0)
		*s = c;
}
#else
# if u3b2 || u3b15
chtype *s;				/* 0(%ap) */
chtype c;				/* 4(%ap) */
int n;					/* 8(%ap) */
{
	char *to;			/* 0(%fp) */
	char *from;			/* 4(%fp) */
	register int count;		/* %r8 */

	count = (n - 1) * sizeof (chtype);
	if (count > 0) {
		register chtype *sfrom = s;	/* %r7 */

		to = (char *)(sfrom + 1);
		from = (char *)sfrom;
		*sfrom = c;

/* Evidently one can not specify the regs for movblb. So I move */
/* them in myself below to the regs where they belong. */
		asm("	movw	%r8,%r2");	/* count */
		asm("	movw	0(%fp),%r1");	/* to */
		asm("	movw	4(%fp),%r0");	/* from */
		asm("	movblb");
	} else if (count == 0)
		*s = c;
}
# else
register chtype *s;
register chtype c;
register int n;
{
    while (n-- > 0)
	*s++ = c;
}
# endif /* u3b2 || u3b15 */
#endif /* u3b */

/* The vax block copy command doesn't work the way I want. */
/* If anyone finds a version that does, I'd like to know. */
#if __bad__vax__

/* this is the code within memcpy which shows how to do a block copy */

memcpy(to, from, count)

	char *to, *from;
	int count;
{
	if (count > 0) {
		asm("	movc3	12(ap),*4(ap),*8(ap)");
	}
}
#endif /* __bad__vax__ */
