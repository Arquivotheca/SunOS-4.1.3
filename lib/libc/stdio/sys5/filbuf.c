#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)filbuf.c 1.1 92/07/30 SMI"; /* from S5R2 2.1 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>
#include <errno.h>

extern _findbuf();
extern int read();
extern int fflush();

int
_filbuf(iop)
register FILE *iop;
{
	static void lbfflush();

	if ( !(iop->_flag & _IOREAD) ) {
		if (iop->_flag & _IORW) {
			iop->_flag |= _IOREAD;
		} else {
#ifdef POSIX
			errno = EBADF;
#endif POSIX
			return(EOF);
		}
	}

	if (iop->_flag&_IOSTRG)
		return(EOF);

	if (iop->_base == NULL)  /* get buffer if we don't have one */
		_findbuf(iop);

	/* if this device is a terminal (line-buffered) or unbuffered, then */
	/* flush buffers of all line-buffered devices currently writing */

	if (iop->_flag & (_IOLBF | _IONBF))
		_fwalk(lbfflush);

	iop->_ptr = iop->_base;
	iop->_cnt = read(fileno(iop), (char *)iop->_base,
	    (unsigned)((iop->_flag & _IONBF) ? 1 : iop->_bufsiz ));
	if (--iop->_cnt >= 0)		/* success */
		return (*iop->_ptr++);
	if (iop->_cnt != -1)		/* error */
		iop->_flag |= _IOERR;
	else {				/* end-of-file */
		iop->_flag |= _IOEOF;
		if (iop->_flag & _IORW)
			iop->_flag &= ~_IOREAD;
	}
	iop->_cnt = 0;
	return (EOF);
}

static void
lbfflush(iop)
FILE *iop;
{
	if (iop->_flag & _IOLBF)
		(void) fflush(iop);
}
