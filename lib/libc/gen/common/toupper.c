#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)toupper.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
/*
 * Internationalized toupper/tolower set (code-set independant)
 * If arg is lower-case, return upper-case, otherwise return arg.
 */
#include <ctype.h>
extern char     _ctype_ul[];

int
toupper(c)
register int c;
{
	if (islower(c))
		return (_ctype_ul+1)[c];	
	return(c);
}
