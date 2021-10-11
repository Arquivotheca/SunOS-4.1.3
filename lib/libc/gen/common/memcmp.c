#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)memcmp.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/*LINTLIBRARY*/
/*
 * Compare n bytes:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */
int
memcmp(s1, s2, n)
register char *s1, *s2;
register int n;
{
	int diff;

	if (s1 != s2)
		while (--n >= 0)
			if (diff = *s1++ - *s2++)
				return (diff);
	return (0);
}
