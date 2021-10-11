#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)nice.c 1.1 92/07/30 SMI"; /* from UCB 4.1 83/05/30 */
#endif

#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>

/*
 * Backwards compatible nice.
 */
int
nice(incr)
	int incr;
{
	register int prio;
	int serrno;

	/* put in brain-damaged upper range checking */
	if ((incr > 40) && (geteuid() != 0)) {
		errno = EPERM;
		return (-1);
	}

	serrno = errno;
	errno = 0;
	prio = getpriority(PRIO_PROCESS, 0);
	if (prio == -1 && errno)
		return (-1);
	prio += incr;
	if (prio < -20)
		prio = -20;
	else if (prio > 19)
		prio = 19;
	if (setpriority(PRIO_PROCESS, 0, prio) == -1) {
		/*
		 * 4.3BSD stupidly returns EACCES on an attempt by a
		 * non-super-user process to lower a priority; map
		 * it to EPERM.
		 */
		if (errno == EACCES)
			errno = EPERM;
		return (-1);
	}
	errno = serrno;
	return (prio);
}
