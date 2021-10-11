/*	@(#)times.h 1.1 92/07/30 SMI; from UCB 4.2 81/02/19	*/

/*
 * Structure returned by times()
 */

#ifndef	__sys_times_h
#define	__sys_times_h

#include <sys/types.h>

struct tms {
	clock_t	tms_utime;		/* user time */
	clock_t	tms_stime;		/* system time */
	clock_t	tms_cutime;		/* user time, children */
	clock_t	tms_cstime;		/* system time, children */
};

#ifndef	KERNEL
clock_t times(/* struct tms *tmsp */);
#endif

#endif	/* !__sys_times_h */
