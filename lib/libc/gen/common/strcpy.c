#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)strcpy.c 1.1 92/07/30 SMI"; /* from UCB 4.1 82/10/05 */
#endif

/*
 * Copy string s2 to s1.  s1 must be large enough.
 * return s1
 */

char *
strcpy(s1, s2)
	register char *s1, *s2;
{
	register char *os1;

	os1 = s1;
	while (*s1++ = *s2++)
		;
	return (os1);
}
