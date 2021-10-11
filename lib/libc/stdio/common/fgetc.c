#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)fgetc.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>
#include <errno.h>

int
fgetc(fp)
register FILE *fp;
{
#ifdef POSIX
	if ( !(fp->_flag & (_IOREAD|_IORW)) ) {
		fp->_flag |= _IOERR;
		errno = EBADF;
		return (EOF);
	}
#endif POSIX
	return(getc(fp));
}
