#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)sprintf.c 1.1 92/07/30 SMI"; /* from S5R2 1.5 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>
#include <varargs.h>
#include <values.h>

extern int _doprnt();

/*VARARGS2*/
char *
sprintf(string, format, va_alist)
char *string, *format;
va_dcl
{
	FILE siop;
	va_list ap;

	siop._cnt = MAXINT;
	siop._base = siop._ptr = (unsigned char *)string;
	siop._flag = _IOWRT+_IOSTRG;
	va_start(ap);
	(void) _doprnt(format, ap, &siop);
	va_end(ap);
	*siop._ptr = '\0'; /* plant terminating null character */
	return(string);
}
