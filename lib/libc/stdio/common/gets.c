#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)gets.c 1.1 92/07/30 SMI"; /* from S5R2 3.3 */
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

extern int _filbuf();
extern _bufsync();
extern char *memccpy();

char *
gets(ptr)
char *ptr;
{
	char *p, *ptr0 = ptr;
	register int n;

#ifdef POSIX
	if ( !(stdin->_flag & (_IOREAD|_IORW)) ) {
		stdin->_flag |= _IOERR;
		errno = EBADF;
		return (NULL);
	}
#endif POSIX
	for ( ; ; ) {
		if (stdin->_cnt <= 0) { /* empty buffer */
			if (_filbuf(stdin) == EOF) {
				if (ptr0 == ptr)
					return (NULL);
				break; /* no more data */
			}
			stdin->_ptr--;
			stdin->_cnt++;
		}
		n = stdin->_cnt;
		if ((p = memccpy(ptr, (char *) stdin->_ptr, '\n', n)) != NULL)
			n = p - ptr;
		ptr += n;
		stdin->_cnt -= n;
		stdin->_ptr += n;
		_BUFSYNC(stdin);
		if (p != NULL) { /* found '\n' in buffer */
			ptr--; /* step back over '\n' */
			break;
		}
	}
	*ptr = '\0';
	return (ptr0);
}
