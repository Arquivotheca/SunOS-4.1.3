#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)swab.c 1.1 92/07/30 SMI"; 
#endif

/* 	Xpg87 version of swab
 *	returns if length is negative
/*
 * 	Copyright (C) 1989 by Sun Microsystems, Inc.
 */

/*
 * Swab bytes
 * Jeffrey Mogul, Stanford
 */

void
swab(from, to, n)
	register char *from, *to;
	register int n;
{
	register unsigned long temp;
	
	if (n < 0)
		return;

	n >>= 1; n++;
#define	STEP	temp = *from++,*to++ = *from++,*to++ = temp
	/* round to multiple of 8 */
	while ((--n) & 07)
		STEP;
	n >>= 3;
	while (--n >= 0) {
		STEP; STEP; STEP; STEP;
		STEP; STEP; STEP; STEP;
	}
}
