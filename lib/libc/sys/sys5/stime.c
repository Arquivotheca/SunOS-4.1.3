#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)stime.c 1.1 92/07/30 SMI";
#endif

/*
	stime -- system call emulation for 4.2BSD

	last edit:	01-Jul-1983	D A Gwyn
*/

extern int	settimeofday();

int
stime( tp )
	long	*tp;			/* -> time to be set */
	{
	struct	{
		unsigned long	tv_sec;
		long		tv_usec;
		}	timeval;	/* for settimeofday() */
	struct	timezone {
		int		tz_minuteswest;
		int		tz_dsttime;
		};			/* also for settimeofday() */

	timeval.tv_sec = *tp;
	timeval.tv_usec = 0L;

	return settimeofday( &timeval, (struct timezone *)0 );
	}
