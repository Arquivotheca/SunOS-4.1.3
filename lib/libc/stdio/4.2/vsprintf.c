#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)vsprintf.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>
#include <varargs.h>
#include <values.h>

extern int _doprnt();

/*VARARGS2*/
char *
vsprintf(string, format, ap)
char *string, *format;
va_list ap;
{
	FILE siop;

	siop._cnt = MAXINT;
	siop._base = siop._ptr = (unsigned char *)string;
	siop._flag = _IOWRT+_IOSTRG;
	(void) _doprnt(format, ap, &siop);
	*siop._ptr = '\0'; /* plant terminating null character */
	return(string);
}
