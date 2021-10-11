#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)fileno.c 1.1 92/07/30 SMI";
#endif

/*LINTLIBRARY*/
#include <stdio.h>

#undef fileno
#define	__fileno__(p)	((p)->_file)

int
fileno(fp)
register FILE *fp;
{
	return (__fileno__(fp));
}
