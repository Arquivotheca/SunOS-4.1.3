#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)feof.c 1.1 92/07/30 SMI";
#endif

/*LINTLIBRARY*/
#include <stdio.h>

#undef feof
#define	__feof__(p)		(((p)->_flag&_IOEOF)!=0)

int
feof(fp)
register FILE *fp;
{
	return (__feof__(fp));
}
