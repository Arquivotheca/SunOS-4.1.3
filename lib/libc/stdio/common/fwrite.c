#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)fwrite.c 1.1 92/07/30 SMI"; /* from S5R2 3.6 */
#endif

/*LINTLIBRARY*/
/*
 * This version writes directly to the buffer rather than looping on putc.
 * Ptr args aren't checked for NULL because the program would be a
 * catastrophic mess anyway.  Better to abort than just to return NULL.
 *
 * This version does buffered writes larger than BUFSIZ directly, when
 * the buffer is empty.
 */
#include <stdio.h>
#include "stdiom.h"
#include <errno.h>

#define MIN(x, y)       (x < y ? x : y)

extern char *memcpy();

int
fwrite(ptr, size, count, iop)
char *ptr;
int size, count;
register FILE *iop;
{
	register unsigned nleft;
	register int n;
	register unsigned char *cptr, *bufend;
	register unsigned char *prev_ptr;

	if (size <= 0 || count <= 0 || _WRTCHK(iop)) {
#ifdef POSIX
		errno = EBADF;
#endif POSIX
	        return (0);
	}

	bufend = iop->_base + iop->_bufsiz;
	nleft = count*size;

	/* if the file is unbuffered, or if the iop->ptr = iop->base, and there
	   is > BUFSZ chars to write, we can do a direct write */
	prev_ptr = iop->_ptr;
	if (iop->_base >= iop->_ptr)  {	/*this covers the unbuffered case, too*/
		if (((iop->_flag & _IONBF) != 0) || (nleft >= BUFSIZ))  {
			if ((n=write(fileno(iop),ptr,nleft)) != nleft)
			    {
				iop->_flag |= _IOERR;
				n = (n >= 0) ? n : 0;
			}
			return n/size;
		}
	}
	/* Put characters in the buffer */
	/* note that the meaning of n when just starting this loop is
	   irrelevant.  It is defined in the loop */
	for (; ; ptr += n) {
	        while ((n = bufend - (cptr = iop->_ptr)) <= 0)  /* full buf */
	                if (_xflsbuf(iop) == EOF)
	                        return (count - (nleft + size - 1)/size);
	        n = MIN(nleft, n);
	        (void) memcpy((char *) cptr, ptr, n);
	        iop->_cnt -= n;
	        iop->_ptr += n;
	        _BUFSYNC(iop);
		/* done; flush if linebuffered with a newline */
	        if ((nleft -= n) == 0)  { 
			if (iop->_flag & (_IOLBF | _IONBF)) {
	               		if ((iop->_flag & _IONBF) || (memchr(prev_ptr,
					'\n',iop->_ptr - prev_ptr) != NULL))  {
				     	(void) _xflsbuf(iop);
				}
			}
	                return (count);
	        }
	}
}
