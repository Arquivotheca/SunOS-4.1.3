#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)rmdir.c 1.1 92/07/30 SMI";
#endif

# include "../common/chkpath.h"

rmdir(d)
    char           *d;
{
	int ret;
	extern errno;

	CHK(d);
	ret = syscall(SYS_rmdir, d);
	if (errno == ENOTEMPTY)
		errno = EEXIST;
	return ret;
}
