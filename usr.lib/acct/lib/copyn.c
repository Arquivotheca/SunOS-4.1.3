/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)copyn.c 1.1 92/07/30 SMI" /* from S5R3 acct:lib/copyn.c 1.3 */
/*
 * Copy n bytes from s2 to s1
 * return s1
 */

char *
copyn(s1, s2, n)
register char *s1, *s2;
{
	register i;
	register char *os1;

	os1 = s1;
	for (i = 0; i < n; i++)
		*s1++ = *s2++;
	return(os1);
}
