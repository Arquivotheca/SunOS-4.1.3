#ifndef lint
static	char sccsid[] = "@(#)memcpy.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/*LINTLIBRARY*/
/*
 * Copy s2 to s1, always copy n bytes.
 * Return s1
 */
char *
memcpy(s1, s2, n)
register char *s1, *s2;
register int n;
{
	register char *os1 = s1;

	while (--n >= 0)
		*s1++ = *s2++;
	return (os1);
}
