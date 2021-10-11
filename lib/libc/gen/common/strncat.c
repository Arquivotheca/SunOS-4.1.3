#ifndef lint
static	char sccsid[] = "@(#)strncat.c 1.1 92/07/30 SMI"; /* from UCB 4.1 80/12/21 */
#endif

/*
 * Concatenate s2 on the end of s1.  S1's space must be large enough.
 * At most n characters are moved.
 * Return s1.
 */

char *
strncat(s1, s2, n)
	register char *s1, *s2;
	register n;
{
	register char *os1;

	os1 = s1;
	while (*s1++)
		;
	--s1;
	while (*s1++ = *s2++)
		if (--n < 0) {
			*--s1 = '\0';
			break;
		}
	return (os1);
}
