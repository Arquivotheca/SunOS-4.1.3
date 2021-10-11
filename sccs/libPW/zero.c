#ifndef lint
static	char sccsid[] = "@(#)zero.c 1.1 92/07/30 SMI"; /* from System III 3.1 */
#endif

/*
	Zero `cnt' bytes starting at the address `ptr'.
	Return `ptr'.
*/

char	*zero(p,n)
register char *p;
register int n;
{
	char *op = p;
	while (--n >= 0)
		*p++ = 0;
	return(op);
}
