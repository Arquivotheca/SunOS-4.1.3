#if	!defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)times.c 1.1 92/07/30 SMI"; /* from UCB 4.2 83/06/02 */
#endif

#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#include "epoch.h"

static clock_t
scale60(tvp)
	register struct timeval *tvp;
{
	return (tvp->tv_sec * 60 + tvp->tv_usec / 16667);
}

clock_t
times(tmsp)
	register struct tms *tmsp;
{
	struct rusage ru;
	struct timeval now;

	if (getrusage(RUSAGE_SELF, &ru) < 0)
		return (clock_t)(-1);
	tmsp->tms_utime = scale60(&ru.ru_utime);
	tmsp->tms_stime = scale60(&ru.ru_stime);
	if (getrusage(RUSAGE_CHILDREN, &ru) < 0)
		return (clock_t)(-1);
	tmsp->tms_cutime = scale60(&ru.ru_utime);
	tmsp->tms_cstime = scale60(&ru.ru_stime);
	if (gettimeofday(&now, (struct timezone *)0) < 0)
		return (clock_t)(-1);
	now.tv_sec -= epoch;
	return (scale60(&now));
}
