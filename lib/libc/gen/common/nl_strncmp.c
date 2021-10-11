/*	@(#)nl_strncmp.c 1.1 92/07/30 SMI;	*/

#include <stdio.h>

#define MAXSTR	256		/* use same value as used in strcoll */

int
nl_strncmp(s1, s2, n)
	char *s1;
	char *s2;
	int n;
{
	char ns1[MAXSTR+1];
	char ns2[MAXSTR+1];
	register int i;
	register char *p1, *p2;

	p1 = ns1;
	p2 = ns2;

	for (i = 0; i < n && i < MAXSTR; i++) {
		*p1++ = *s1++;
		*p2++ = *s2++;
	}
	*p1 = *p2 = '\0';

	return (strcoll(ns1, ns2));
}
