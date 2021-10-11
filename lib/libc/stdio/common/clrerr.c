#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)clrerr.c 1.1 92/07/30 SMI"; /* from S5R2 1.3 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>
#undef clearerr

void
clearerr(iop)
register FILE *iop;
{
	iop->_flag &= ~(_IOERR | _IOEOF);
}
