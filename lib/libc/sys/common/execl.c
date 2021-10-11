#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)execl.c 1.1 92/07/30 SMI"; 
#endif

#include <varargs.h>

/*
 *	execl(name, arg0, arg1, ..., argn, (char *)0)
 *	environment automatically passed.
 */
/*VARARGS1*/
execl(name, va_alist)
	register char *name;
	va_dcl
{
	extern char **environ;
	va_list args;

	va_start(args);
	return (execve(name, (char **)args, environ));
}
