#ifndef lint
static char sccsid[] = "@(#)path-util.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 Sun Microsystems, Inc.
 */

/*
 * Concatenate s2 on the end of s1 separated by a '/' (unless s1 is empty).
 * Check for spurious '/'s.  s1's space must be large enough.  Return s1.
 */
char *
nse_pathcat(s1, s2)
	register char *s1, *s2;
{
	register char *os1;

	os1 = s1;
	while (*s1++)
		;
	--s1;
	if (s1 != os1) {
		if ((*s2 != '/') && (s1[-1] != '/')) {
			*s1++ = '/';
		} else if ((*s2 == '/') && (s1[-1] == '/')) {
			++s2;
		}
	}
	while (*s1++ = *s2++)
		;
	return (os1);
}
