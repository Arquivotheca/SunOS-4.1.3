#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)puts.c 1.1 92/07/30 SMI"; /* from S5R2 3.3 */
#endif

/*LINTLIBRARY*/
/*
 * This version writes directly to the buffer rather than looping on putc.
 * Ptr args aren't checked for NULL because the program would be a
 * catastrophic mess anyway.  Better to abort than just to return NULL.
 */
#include <stdio.h>
#include "stdiom.h"

extern char *memccpy();

int
puts(ptr)
char *ptr;
{
	char *p;
	register int ndone = 0, n;
	register unsigned char *cptr, *bufend;

	if (_WRTCHK(stdout))
		return (EOF);

	bufend = stdout->_base + stdout->_bufsiz;

	for ( ; ; ptr += n) {
		while ((n = bufend - (cptr = stdout->_ptr)) <= 0) /* full buf */
			if (_xflsbuf(stdout) == EOF)
				return(EOF);
		if ((p = memccpy((char *) cptr, ptr, '\0', n)) != NULL)
			n = p - (char *) cptr;
		stdout->_cnt -= n;
		stdout->_ptr += n;
		_BUFSYNC(stdout);
		ndone += n;
		if (p != NULL) {
			stdout->_ptr[-1] = '\n'; /* overwrite '\0' with '\n' */
			if (stdout->_flag & (_IONBF | _IOLBF)) /* flush line */
				if (_xflsbuf(stdout) == EOF)
					return(EOF);
			return(ndone);
		}
	}
}
