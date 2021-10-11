#ifndef lint
static	char sccsid[] = "@(#)cat.c 1.1 92/07/30 SMI"; /* from System III 3.1 */
#endif

/*
	Concatenate strings.
 
	cat(destination,source1,source2,...,sourcen,0);
 
	returns destination.
*/

#include <varargs.h>

/*VARARGS1*/
char *cat(dest,va_alist)
	char *dest;
	va_dcl
{
	va_list argp;
	register char *d, *s;

	d = dest;
	va_start(argp);
	while (s = va_arg(argp, char*)) {
		while (*d++ = *s++) ;
		d--;
	}
	return(dest);
}
