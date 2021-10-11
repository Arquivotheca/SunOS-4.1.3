#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)ulimit.c 1.1 92/07/30 (C) 1983 SMI";
#endif

#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>

extern	errno;

ulimit(cmd, newlimit)
	int cmd;
	long newlimit;
{
	struct rlimit rlimit;

	switch (cmd) {

	case 1:
		if (getrlimit(RLIMIT_FSIZE, &rlimit) < 0)
			return(-1);
		return (rlimit.rlim_cur / 512);

	case 2:
		rlimit.rlim_cur = rlimit.rlim_max = newlimit * 512;
		return (setrlimit(RLIMIT_FSIZE, &rlimit));

	case 3:
		if (getrlimit(RLIMIT_DATA, &rlimit) < 0)
			return(-1);
		return (rlimit.rlim_cur);

	case 4:
		return (getdtablesize());

	default:
		errno = EINVAL;
		return (-1);
	}
}
