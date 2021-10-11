#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)lockf.c 1.1 92/07/30 SMI";
#endif

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/*
 * convert lockf() into fcntl() for SystemV compatibility
 */

lockf(fildes, function, size)
	int fildes;
	int function;
	long size;
{
	struct flock ld;
	register int cmd;

	cmd = F_SETLK;		/* assume non-blocking operation */
	ld.l_type = F_WRLCK;	/* lockf() only deals with exclusive locks */
	ld.l_whence = 1;	/* lock always starts at current position */
	ld.l_start = 0;		/* 0 offset from current position */
	ld.l_len = size;	/* may be negative */

	switch (function) {
	case F_TEST:
		if (fcntl(fildes, F_GETLK, &ld) != -1) {
			if (ld.l_type == F_UNLCK)
				return (0);
			else
				errno = EACCES;		/* EAGAIN ?? */
		}
		return (-1);

	default:
		errno = EINVAL;
		return (-1);

			/* the rest fall thru to the fcntl() at the end */
	case F_ULOCK:
		ld.l_type = F_UNLCK;
		break;

	case F_LOCK:
		cmd = F_SETLKW;	/* block, if not available */
		break;

	case F_TLOCK:
		break;
	}
	if (fcntl(fildes, cmd, &ld) == -1) {
		switch (errno) {
		/* this hack is purported to be for /usr/group compatibility */
		case ENOLCK:
			errno = EDEADLK;
		}
		return(-1);
	} else {
		return(0);
	}
}
