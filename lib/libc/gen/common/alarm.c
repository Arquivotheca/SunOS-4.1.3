#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)alarm.c 1.1 92/07/30 SMI"; /* from UCB 4.1 83/06/10 */
#endif

/*
 * Backwards compatible alarm.
 */
#include <sys/time.h>

alarm(secs)
	int secs;
{
	struct itimerval it, oitv;
	register struct itimerval *itp = &it;

	timerclear(&itp->it_interval);
	itp->it_value.tv_sec = secs;
	itp->it_value.tv_usec = 0;
	if (setitimer(ITIMER_REAL, itp, &oitv) < 0)
		return (-1);
	if (oitv.it_value.tv_usec)
		oitv.it_value.tv_sec++;
	return (oitv.it_value.tv_sec);
}
