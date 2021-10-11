#ifndef lint
static	char sccsid[] = "@(#)patoi.c 1.1 92/07/30 SMI"; /* from System III 3.1 */
#endif

/*
	Function to convert ascii string to integer.  Converts
	positive numbers only.  Returns -1 if non-numeric
	character encountered.
*/

patoi(s)
register char *s;
{
	register int i;

	i = 0;
	while(*s >= '0' && *s <= '9') i = 10 * i + *s++ - '0';

	if(*s) return(-1);
	return(i);
}
