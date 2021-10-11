#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)time.c 1.1 92/07/30 SMI"; /* from UCB 4.2 83/02/27 */
#endif

/*
 * Backwards compatible time call.
 */
#include <sys/types.h>
#include <sys/time.h>

time_t
time(t)
	time_t *t;
{
	struct timeval tt;

	if (gettimeofday(&tt, (struct timezone *)0) < 0)
		return (-1);
	if (t)
		*t = tt.tv_sec;
	return (tt.tv_sec);
}
