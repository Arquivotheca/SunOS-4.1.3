#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)setbuffer.c 1.1 92/07/30 SMI"; /* from UCB 4.2 83/02/27 */
#endif

#include <stdio.h>

extern void free();
extern int isatty();
extern unsigned char (*_smbuf)[_SBFSIZ];
extern void _getsmbuf();

void
setbuffer(iop, buf, size)
	register FILE *iop;
	char *buf;
	int size;
{
	register int fno = fileno(iop);  /* file number */

	if (iop->_base != NULL && iop->_flag&_IOMYBUF)
		free((char *)iop->_base);
	iop->_flag &= ~(_IOMYBUF|_IONBF|_IOLBF);
	if ((iop->_base = (unsigned char *)buf) == NULL) {
		iop->_flag |= _IONBF; /* file unbuffered except in fastio */
		/* use small buffers reserved for this */
		iop->_base = _smbuf[fno];
		iop->_bufsiz = _SBFSIZ;
	} else {
		/* regular buffered I/O, specified buffer size */
		if (size <= 0)
			return;
		iop->_bufsiz = size;
	}
	iop->_ptr = iop->_base;
	iop->_cnt = 0;
}

/*
 * set line buffering
 */
setlinebuf(iop)
	register FILE *iop;
{
	register unsigned char *buf;
	extern char *malloc();

	fflush(iop);
	setbuffer(iop, (unsigned char *)NULL, 0);
	buf = (unsigned char *)malloc(128);
	if (buf != NULL) {
		setbuffer(iop, buf, 128);
		iop->_flag |= _IOLBF|_IOMYBUF;
	}
}
