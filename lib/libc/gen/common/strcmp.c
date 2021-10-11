#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)strcmp.c 1.1 92/07/30 SMI"; /* from UCB 4.1 80/12/21 */
#endif

/*
 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */

strcmp(s1, s2)
	register char *s1, *s2;
{

	while (*s1 == *s2++)
		if (*s1++=='\0')
			return (0);
	return (*s1 - *--s2);
}
