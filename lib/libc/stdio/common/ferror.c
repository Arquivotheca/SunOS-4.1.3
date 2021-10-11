#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)ferror.c 1.1 92/07/30 SMI";
#endif

/*LINTLIBRARY*/
#include <stdio.h>

#undef ferror
#define	__ferror__(p)	(((p)->_flag&_IOERR)!=0)

int
ferror(fp)
register FILE *fp;
{
	return (__ferror__(fp));
}
