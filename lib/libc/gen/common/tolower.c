#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)tolower.c 1.1 92/07/30 SMI"; 
#endif
#include <ctype.h>

extern char     _ctype_ul[];

/*LINTLIBRARY*/
/*
 * If arg is upper-case, return lower-case, otherwise return arg.
 */

int
tolower(c)
register int c;
{
	if (isupper(c))
		return (_ctype_ul+1)[c];
	return(c);
}
