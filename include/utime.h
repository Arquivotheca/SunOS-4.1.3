/*	@(#)utime.h 1.1 92/07/30 SMI; from S5R2 1.1	*/

#ifndef	__utime_h
#define	__utime_h

#include <sys/stdtypes.h>

/*
 * POSIX utime.h, utime() calls utimes()
 */

struct	utimbuf {
	time_t	actime;		/* set the access time */
	time_t	modtime;	/* set the modification time */
};

int	utime(/* char *name, struct utimbuf *p */);

#endif	/* !__utime_h */
