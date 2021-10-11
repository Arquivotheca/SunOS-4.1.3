/*	@(#)fcntl.h 1.1 92/07/30 SMI	*/

#ifndef	__sys_fcntl_h
#define	__sys_fcntl_h

#include <sys/fcntlcom.h>

#ifndef _POSIX_SOURCE
#define	O_NDELAY	_FNBIO	/* Non-blocking I/O (S5 style) */
#endif

#endif	/* !__sys_fcntl_h */
