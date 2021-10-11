#ifndef lint
static	char sccsid[] = "@(#)fdfopen.c 1.1 92/07/30 SMI"; /* from System III 3.1 */
#endif

/*
	Interfaces with /lib/libS.a
	First arg is file descriptor, second is read/write mode (0/1).
	Returns file pointer on success,
	NULL on failure (no file structures available).
*/

# include	<stdio.h>
# include	<sys/types.h>
# include	"../hdr/macros.h"

FILE *
fdfopen(fd, mode)
register int fd, mode;
{
#ifdef VMUNIX

	return (fdopen(fd, mode?"w":"r"));
#else
	register FILE *iop;

	if (fstat(fd, &Statbuf) < 0)
		return(NULL);
	for (iop = _iob; iop->_flag&(_IOREAD|_IOWRT); iop++)
		if (iop >= &_iob[_NFILE-1])
			return(NULL);
	iop->_flag &= ~(_IOREAD|_IOWRT);
	iop->_file = fd;
	if (mode)
		iop->_flag |= _IOWRT;
	else
		iop->_flag |= _IOREAD;
	return(iop);
#endif
}
