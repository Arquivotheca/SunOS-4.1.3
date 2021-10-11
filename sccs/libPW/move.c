#ifndef lint
static	char sccsid[] = "@(#)move.c 1.1 92/07/30 SMI"; /* from System III */
#endif

/*
	Copies `n' characters from string `a' to string `b'.
*/

move(a,b,n)
register char *a,*b;
unsigned n;
{
	while(n--) *b++ = *a++;
}
