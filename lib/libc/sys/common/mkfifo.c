#if	!defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)mkfifo.c 1.1 92/07/30 SMI";
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include "../common/chkpath.h"

mkfifo(path, mode)
	char *path;
	mode_t mode;
{
	CHK(path);
	return mknod(path, S_IFIFO | (mode & (S_IRWXU|S_IRWXG|S_IRWXO)));
}
