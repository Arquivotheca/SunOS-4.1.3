#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)putc.c 1.1 92/07/30 SMI";
#endif

/*LINTLIBRARY*/
#include <stdio.h>

#undef putc
#define __putc__(x, p)	(--(p)->_cnt >= 0 ?\
	(int)(*(p)->_ptr++ = (unsigned char)(x)) :\
	(((p)->_flag & _IOLBF) && -(p)->_cnt < (p)->_bufsiz ?\
		((*(p)->_ptr = (unsigned char)(x)) != '\n' ?\
			(int)(*(p)->_ptr++) :\
			_flsbuf(*(unsigned char *)(p)->_ptr, p)) :\
		_flsbuf((unsigned char)(x), p)))

int
putc(c, fp)
register char c;
register FILE *fp;
{
	return (__putc__(c, fp));
}
