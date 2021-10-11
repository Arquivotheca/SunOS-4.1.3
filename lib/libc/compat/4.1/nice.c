#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)nice.c 1.1 92/07/30 SMI"; /* from UCB 4.1 83/05/30 */
#endif

#include <sys/time.h>
#include <sys/resource.h>

/*
 * Backwards compatible nice.
 */
int
nice(incr)
	int incr;
{
	int prio;
	extern int errno;
	int serrno;

	serrno = errno;
	errno = 0;
	prio = getpriority(PRIO_PROCESS, 0);
	if (prio == -1 && errno)
		return (-1);
	if (setpriority(PRIO_PROCESS, 0, prio + incr) == -1)
		return (-1);
	errno = serrno;
	return (0);
}
