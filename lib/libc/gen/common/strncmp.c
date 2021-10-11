#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)strncmp.c 1.1 92/07/30 SMI"; /* from UCB 4.1 80/12/21 */
#endif

/*
 * Compare strings (at most n bytes):  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */

strncmp(s1, s2, n)
	register char *s1, *s2;
	register int n;
{

	while (--n >= 0 && *s1 == *s2++)
		if (*s1++ == '\0')
			return(0);
	return (n<0 ? 0 : *s1 - *--s2);
}
