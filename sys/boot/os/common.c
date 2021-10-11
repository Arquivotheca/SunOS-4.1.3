/*
 * @(#)common.c 1.1 92/07/30 Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Common code for various bootstrap routines.
 */
#include "../mon/sunromvec.h"
#define	skipblank(p)    { while (*(p) == ' ') (p)++; }

/* See boot.c */
int	verbosemode = 0;

/*
 * Hex string scanner for open().   Converts strings of the
 * form {[0-9]|[a-f]|[A_F]}*.    If none of these characters
 * are found before a delimiter ',' or illegal character, the
 * value accumulated so far is returned.   If a ',' is found,
 * the value returned points after the ','.   If the string
 * ends in '\0', the value returned points to the '\0'.  Other-
 * wise, the value returned points to the first non-hex char-
 * acter encountered.
 */
char *
gethex(p, ip)
	register char *p;
	int *ip;
{
	register int ac = 0;

	skipblank(p);
	while (*p) {
		if (*p >= '0' && *p <= '9')
			ac = (ac<<4) + (*p - '0');
		else if (*p >= 'a' && *p <= 'f')
			ac = (ac<<4) + (*p - 'a' + 10);
		else if (*p >= 'A' && *p <= 'F')
			ac = (ac<<4) + (*p - 'A' + 10);
		else
			break;
		p++;
	}
	skipblank(p);
#ifdef	not
	/* vfs_sys.c doesn't like this */
	if (*p == ',')
		p++;
	skipblank(p);
#endif	not

	*ip = ac;
	return (p);
}

feedback()	/* Show activity */
{
	static	int	n = 0;
	char	*ind = "|/-\\";

	if (verbosemode)
		putchar('.');
	else
		printf("%c\b", ind[n++ % 4]);
}

static char	*sys_errlist[] = {
	"Error 0",
	"Not owner",				/* 1 - EPERM */
	"No such file or directory",		/* 2 - ENOENT */
	"No such process",			/* 3 - ESRCH */
	"Interrupted system call",		/* 4 - EINTR */
	"I/O error",				/* 5 - EIO */
	"No such device or address",		/* 6 - ENXIO */
	"Arg list too long",			/* 7 - E2BIG */
	"Exec format error",			/* 8 - ENOEXEC */
	"Bad file number",			/* 9 - EBADF */
	"No children",				/* 10 - ECHILD */
	"No more processes",			/* 11 - EAGAIN */
	"Not enough memory",			/* 12 - ENOMEM */
	"Permission denied",			/* 13 - EACCES */
	"Bad address",				/* 14 - EFAULT */
	"Block device required",		/* 15 - ENOTBLK */
	"Device busy",				/* 16 - EBUSY */
	"File exists",				/* 17 - EEXIST */
	"Cross-device link",			/* 18 - EXDEV */
	"No such device",			/* 19 - ENODEV */
	"Not a directory",			/* 20 - ENOTDIR */
	"Is a directory",			/* 21 - EISDIR */
	"Invalid argument",			/* 22 - EINVAL */
	"File table overflow",			/* 23 - ENFILE */
	"Too many open files",			/* 24 - EMFILE */
	"Inappropriate ioctl for device",	/* 25 - ENOTTY */
	"Text file busy",			/* 26 - ETXTBSY */
	"File too large",			/* 27 - EFBIG */
	"No space left on device",		/* 28 - ENOSPC */
	"Illegal seek",				/* 29 - ESPIPE */
	"Read-only file system",		/* 30 - EROFS */
	"Too many links",			/* 31 - EMLINK */
	"Broken pipe",				/* 32 - EPIPE */

/* math software */
	"Argument too large",			/* 33 - EDOM */
	"Result too large",			/* 34 - ERANGE */

/* non-blocking and interrupt i/o */
	"Operation would block",		/* 35 - EWOULDBLOCK */
	"Operation now in progress",		/* 36 - EINPROGRESS */
	"Operation already in progress",	/* 37 - EALREADY */

/* ipc/network software */

	/* argument errors */
	"Socket operation on non-socket",	/* 38 - ENOTSOCK */
	"Destination address required",		/* 39 - EDESTADDRREQ */
	"Message too long",			/* 40 - EMSGSIZE */
	"Protocol wrong type for socket",	/* 41 - EPROTOTYPE */
	"Option not supported by protocol",	/* 42 - ENOPROTOOPT */
	"Protocol not supported",		/* 43 - EPROTONOSUPPORT */
	"Socket type not supported",		/* 44 - ESOCKTNOSUPPORT */
	"Operation not supported on socket",	/* 45 - EOPNOTSUPP */
	"Protocol family not supported",	/* 46 - EPFNOSUPPORT */
	"Address family not supported by protocol family",
						/* 47 - EAFNOSUPPORT */
	"Address already in use",		/* 48 - EADDRINUSE */
	"Can't assign requested address",	/* 49 - EADDRNOTAVAIL */

	/* operational errors */
	"Network is down",			/* 50 - ENETDOWN */
	"Network is unreachable",		/* 51 - ENETUNREACH */
	"Network dropped connection on reset",	/* 52 - ENETRESET */
	"Software caused connection abort",	/* 53 - ECONNABORTED */
	"Connection reset by peer",		/* 54 - ECONNRESET */
	"No buffer space available",		/* 55 - ENOBUFS */
	"Socket is already connected",		/* 56 - EISCONN */
	"Socket is not connected",		/* 57 - ENOTCONN */
	"Can't send after socket shutdown",	/* 58 - ESHUTDOWN */
	"Too many references: can't splice",	/* 59 - ETOOMANYREFS */
	"Connection timed out",			/* 60 - ETIMEDOUT */
	"Connection refused",			/* 61 - EREFUSED */
	"Too many levels of symbolic links",	/* 62 - ELOOP */
	"File name too long",			/* 63 - ENAMETOOLONG */
	"Host is down",				/* 64 - EHOSTDOWN */
	"Host is unreachable",			/* 65 - EHOSTUNREACH */
	"Directory not empty",			/* 66 - ENOTEMPTY */
	"Too many processes",			/* 67 - EPROCLIM */
	"Too many users",			/* 68 - EUSERS */
	"Disc quota exceeded",			/* 69 - EDQUOT */
	"Stale NFS file handle",		/* 70 - ESTALE */
	"Too many levels of remote in path",	/* 71 - EREMOTE */
	"Not a stream device",			/* 72 - ENOSTR */
	"Timer expired",			/* 73 - ETIME */
	"Out of stream resources",		/* 74 - ENOSR */
	"No message of desired type",		/* 75 - ENOMSG */
	"Not a data message",			/* 76 - EBADMSG */
	"Identifier removed",			/* 77 - EIDRM */
	"Deadlock situation detected/avoided",	/* 78 - EDEADLK */
	"No record locks available",		/* 79 - ENOLCK */

/* RFS errors */

	"Machine is not on the network", 	/* 80 - ENONET */
	"Object is remote", 			/* 81 - ERREMOTE */
	"Link has been severed", 		/* 82 - ENOLINK */
	"Advertise error ", 			/* 83 - EADV */
	"Srmount error ", 			/* 84 - ESRMNT */
	"Communication error on send", 		/* 85 - ECOMM */
	"Protocol error", 			/* 86 - EPROTO */
	"Multihop attempted", 			/* 87 - EMULTIHOP */
	"EDOTDOT!!!!", 				/* 88 - EDOTDOT -can't happen */
	"Remote address changed",		/* 89 - EREMCHG */
};
int	sys_nerr = { sizeof sys_errlist/sizeof sys_errlist[0] };

void
errno_print(errno)
int	errno;
{
	if ((errno < 0) || (errno >= sys_nerr))
		printf("Unknown errno");
	else
		printf("%s", sys_errlist[errno]);
	printf(" (errno %d)", errno);
}
