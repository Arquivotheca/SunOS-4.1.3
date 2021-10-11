#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)fread.c 1.1 92/07/30 SMI"; /* from S5R2 3.11 */
#endif

/*LINTLIBRARY*/
/*
 * This version reads directly from the buffer rather than looping on getc.
 * Ptr args aren't checked for NULL because the program would be a
 * catastrophic mess anyway.  Better to abort than just to return NULL.
 */
#include <stdio.h>
#include "stdiom.h"
#include <errno.h>

#define MIN(x, y)	(x < y ? x : y)

extern int _filbuf();
extern _bufsync();
extern char *memcpy();

int
fread(ptr, size, count, iop)
char *ptr;
int size, count;
register FILE *iop;
{
	register unsigned int nleft;
	register int n;

#ifdef POSIX
	if ( !(iop->_flag & (_IOREAD|_IORW)) ) {
		iop->_flag |= _IOERR;
		errno = EBADF;
		return (NULL);
	}
#endif POSIX
	if (size <= 0 || count <= 0) return 0;
	nleft = count * size;

	/* Put characters in the buffer */
	/* note that the meaning of n when just starting this loop is
	   irrelevant.  It is defined in the loop */
	for ( ; ; ) {
		if (iop->_cnt <= 0) { /* empty buffer */
			if (_filbuf(iop) == EOF)
				return (count - (nleft + size - 1)/size);
			iop->_ptr--;
			iop->_cnt++;
		}
		n = MIN(nleft, iop->_cnt);
		ptr = memcpy(ptr, (char *) iop->_ptr, n) + n;
		iop->_cnt -= n;
		iop->_ptr += n;
		_BUFSYNC(iop);
		if ((nleft -= n) == 0)
			return (count);
	}
}
