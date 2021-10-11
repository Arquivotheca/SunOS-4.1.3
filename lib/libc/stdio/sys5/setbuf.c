#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)setbuf.c 1.1 92/07/30 SMI"; /* from S5R2 2.2 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>

extern void free();
extern int isatty();
extern unsigned char (*_smbuf)[_SBFSIZ];
extern void _getsmbuf();

void
setbuf(iop, buf)
register FILE *iop;
char	*buf;
{
	register int fno = fileno(iop);  /* file number */

	if(iop->_base != NULL && iop->_flag & _IOMYBUF)
		free((char*)iop->_base);
	iop->_flag &= ~(_IOMYBUF | _IONBF | _IOLBF);
	if((iop->_base = (unsigned char*)buf) == NULL) {
		iop->_flag |= _IONBF; /* file unbuffered except in fastio */
		/* use small buffers reserved for this */
		iop->_base = _smbuf[fno];
		iop->_bufsiz = _SBFSIZ;
	}
	else {  /* regular buffered I/O, standard buffer size */
		if (isatty(fno))
			iop->_flag |= _IOLBF;
		iop->_bufsiz = BUFSIZ;
	}
	iop->_ptr = iop->_base;
	iop->_cnt = 0;
}
