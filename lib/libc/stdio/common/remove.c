#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)remove.c 1.1 92/07/30 SMI";
#endif

/*LINTLIBRARY*/
#include <stdio.h>

#undef remove

int
remove(fname)
register char *fname;
{
	return (unlink(fname));
}
