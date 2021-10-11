#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)putchar.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
/*
 * A subroutine version of the macro putchar
 */
#include <stdio.h>
#undef putchar

int
putchar(c)
register char c;
{
	return(putc(c, stdout));
}
