#ifndef lint
static	char sccsid[] = "@(#)zeropad.c 1.1 92/07/30 SMI"; /* from System III 3.1 */
#endif

/*
	Replace initial blanks with '0's in `str'.
	Return `str'.
*/

char *zeropad(str)
char *str;
{
	register char *s;

	for (s=str; *s == ' '; s++)
		*s = '0';
	return(str);
}
