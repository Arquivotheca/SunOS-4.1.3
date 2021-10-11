/*	@(#)time.h 1.1 92/07/30 SMI; from S5R2 1.1	*/

#ifndef	__time_h
#define	__time_h

#include <sys/stdtypes.h>

#ifndef NULL
#define	NULL	0
#endif

struct	tm {	/* see ctime(3) */
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

/*
 * Following 2 lines are required to make CLK_TCK work.
 * If they change here they have to change in <sys/unistd.h> as well.
 */
extern long	sysconf(/* int name */);
#define	_SC_CLK_TCK	3	/* clock ticks/sec */
/*
 * POSIX.1 uses CLK_TCK to specify units used by times(3).
 * POSIX.1a doesn't use a name for this and says CLK_TCK is obsolescent, but
 * we'll probably have to support it for a long time.
 */
#define	CLK_TCK		(sysconf(_SC_CLK_TCK))
/* 881207 ANSI C draft uses CLOCKS_PER_SEC to specify units used by clock(3). */
#define	CLOCKS_PER_SEC	1000000L

extern char *	asctime(/* const struct tm *t */);
extern char *	ctime(/* const time_t *t */);
extern struct tm * gmtime(/* const time_t *t */);
extern struct tm * localtime(/* const time_t *t */);
extern time_t	mktime(/* struct tm *timeptr */);
extern size_t	strftime(/* char *s, size_t maxsize, const char *format,
		    const struct tm *timeptr */);
extern time_t	time(/* time_t *t */);
extern void	tzset(/* void */);

extern char	*tzname[];
#ifndef	_POSIX_SOURCE
extern int	daylight;
extern long	timezone;
extern void	tzsetwall(/* void */);
#endif	!_POSIX_SOURCE

#endif	!__time_h
