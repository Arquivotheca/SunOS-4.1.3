 /*	@(#)modsubr.c 1.1 92/07/30 SMI	*/

#include <stdio.h>
#include <varargs.h>

extern int errno;

/*VARARGS0*/
void
error(va_alist)
	va_dcl
{
	va_list args;
	char *fmt;

	va_start(args);
	fmt = va_arg(args, char *);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, ": ");
	perror("");
	exit(errno);
}

/*VARARGS0*/
void
fatal(va_alist)
	va_dcl
{
	va_list args;
	char *fmt;

	va_start(args);
	fmt = va_arg(args, char *);
	(void) vfprintf(stderr, fmt, args);
	va_end(args);
	exit(-1);
}
