/*	@(#)time.h 1.1 92/07/30 SMI; not from UCB */

#ifndef	__time_h
#define	__time_h

#include <sys/stdtypes.h>
/*
 * Structure returned by gmtime and localtime calls (see ctime(3)).
 */
struct	tm {
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
	char	*tm_zone;
	long	tm_gmtoff;
};

extern	struct tm *gmtime(), *localtime();
extern	char *asctime(), *ctime();
extern	void tzset(), tzsetwall();
extern  int dysize();
extern  time_t timelocal(), timegm();

#endif	/* !__time_h */
