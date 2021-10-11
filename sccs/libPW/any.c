#ifndef lint
static	char sccsid[] = "@(#)any.c 1.1 92/07/30 SMI"; /* from System III 3.1 */
#endif

/*
	If any character of `s' is `c', return 1
	else return 0.
*/

any(c,s)
register char c, *s;
{
	while (*s)
		if (*s++ == c)
			return(1);
	return(0);
}
