#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)vfprintf.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>
#include <varargs.h>
#include <errno.h>

extern int _doprnt();

/*VARARGS2*/
int
vfprintf(iop, format, ap)
FILE *iop;
char *format;
va_list ap;
{
	unsigned char localbuf[BUFSIZ];
	register int count;

	if (!(iop->_flag & _IOWRT)) {
		/* if no write flag */
		if (iop->_flag & _IORW) {
			/* if ok, cause read-write */
			iop->_flag |= _IOWRT;
		} else {
			/* else error */
#ifdef POSIX
			errno = EBADF;
#endif POSIX
			return EOF;
		}
	}
	if (iop->_flag & _IONBF) {
		iop->_flag &= ~_IONBF;
		iop->_ptr = iop->_base = localbuf;
		iop->_bufsiz = BUFSIZ;
		count = _doprnt(format, ap, iop);
		fflush(iop);
		iop->_flag |= _IONBF;
		iop->_base = NULL;
		iop->_bufsiz = 0;
		iop->_cnt = 0;
	} else
		count = _doprnt(format, ap, iop);
	return(ferror(iop)? EOF: count);
}
