#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)memccpy.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/*LINTLIBRARY*/
/*
 * Copy s2 to s1, stopping if character c is copied. Copy no more than n bytes.
 * Return a pointer to the byte after character c in the copy,
 * or NULL if c is not found in the first n bytes.
 */
char *
memccpy(s1, s2, c, n)
register char *s1, *s2;
register int c, n;
{
	while (--n >= 0)
		if ((*s1++ = *s2++) == c)
			return (s1);
	return (0);
}
