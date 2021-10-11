#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)fputs.c 1.1 92/07/30 SMI"; /* from S5R2 3.5 */
#endif

/*LINTLIBRARY*/
/*
 * This version writes directly to the buffer rather than looping on putc.
 * Ptr args aren't checked for NULL because the program would be a
 * catastrophic mess anyway.  Better to abort than just to return NULL.
 */
#include <stdio.h>
#include "stdiom.h"
#include <errno.h>

extern char *memccpy();
static char *memnulccpy();

int
fputs(ptr, iop)
char *ptr;
register FILE *iop;
{
	register int ndone = 0, n;
	register unsigned char *cptr, *bufend;
	register char *p;
	register char c;

	if (_WRTCHK(iop)) {
		iop->_flag |= _IOERR;
#ifdef POSIX
		errno = EBADF;
#endif POSIX
		return (EOF);
	}
	bufend = iop->_base + iop->_bufsiz;

	if ((iop->_flag & _IONBF) == 0)  {
		if (iop->_flag & _IOLBF) {
			for ( ; ; ptr += n) {
				while ((n = bufend - (cptr = iop->_ptr)) <= 0)  
					/* full buf */
					if (_xflsbuf(iop) == EOF)
						return(EOF);
				if ((p = memnulccpy((char *) cptr, ptr, '\n', n)) != NULL) {
					/*
					 * Copy terminated either because we
					 * saw a newline or we saw a NUL (end
					 * of string).
					 */
					c = *(p - 1);	/* last character moved */
					if (c == '\0')
						p--;	/* didn't write '\0' */
					n = p - (char *) cptr;
				}
				iop->_cnt -= n;
				iop->_ptr += n;
				_BUFSYNC(iop);
				ndone += n;
				if (p != NULL) {
					/*
					 * We found either a newline or a NUL.
					 * If we found a newline, flush the
					 * buffer.
					 * If we found a NUL, we're done.
					 */
					if (c == '\n') {
						if (_xflsbuf(iop) == EOF)
							return(EOF);
					} else {
						/* done */
						return(ndone);
					}
		       		}
			}
		} else {
			for ( ; ; ptr += n) {
				while ((n = bufend - (cptr = iop->_ptr)) <= 0)  
					/* full buf */
					if (_xflsbuf(iop) == EOF)
						return(EOF);
				if ((p = memccpy((char *) cptr, ptr, '\0', n)) != NULL)
					n = (p - (char *) cptr) - 1;
				iop->_cnt -= n;
				iop->_ptr += n;
				_BUFSYNC(iop);
				ndone += n;
				if (p != NULL)  { 
					/* done */
		       			return(ndone);
		       		}
			}
		}
	}  else  {
		/* write out to an unbuffered file */
		register unsigned int cnt = strlen(ptr);
		int retval;

		retval = write(iop->_file, ptr, cnt);
		return(retval);
	}
}

/*
 * Copy s2 to s1, stopping if character c or a NUL is copied.
 * Copy no more than n bytes.
 * Return a pointer to the byte after character c or NUL in the copy,
 * or NULL if c or NUL is not found in the first n bytes.
 */
static char *
memnulccpy(s1, s2, c, n)
register char *s1, *s2;
register int c, n;
{
	register int cmoved;

	while (--n >= 0) {
		cmoved = *s2++;
		if ((*s1++ = cmoved) == '\0' || cmoved == c)
			return (s1);
	}
	return (0);
}
