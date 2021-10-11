#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)setvbuf.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>

extern void free();
extern unsigned char (*_smbuf)[_SBFSIZ];
extern char *malloc();
extern void _getsmbuf();

int
setvbuf(iop, buf, type, size)
register FILE *iop;
register char	*buf;
register int type;
register int size;
{
	register int fno = fileno(iop);  /* file number */

	if(iop->_base != NULL && iop->_flag & _IOMYBUF)
		free((char*)iop->_base);
	iop->_flag &= ~(_IOMYBUF | _IONBF | _IOLBF);
	switch (type)  {
	    /*note that the flags are the same as the possible values for type*/
	    case _IONBF:
		/* file is unbuffered except in fastio */
		iop->_flag |= _IONBF;
		/* use small buffers reserved for this */
		iop->_base = _smbuf[fno];
		iop->_bufsiz = _SBFSIZ;
		break;
	    case _IOLBF:
	    case _IOFBF:
		if (size < 0)
			return -1;
		iop->_flag |= type;
		size = (size == 0) ? BUFSIZ : size;
		/* 
		* need eight characters beyond bufend for stdio slop
		*/
		if (size <= 8) {
		    size = BUFSIZ;
		    buf = NULL;
		}
		if (buf == NULL) {
			size += 8;
			buf = malloc((unsigned)size);
			iop->_flag |= _IOMYBUF;
		}
		iop->_base = (unsigned char *)buf;
		iop->_bufsiz = size - 8;
		break;
	    default:
		return -1;
	}
	iop->_ptr = iop->_base;
	iop->_cnt = 0;
	return 0;
}
