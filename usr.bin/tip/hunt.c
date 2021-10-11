#ifndef lint
static	char sccsid[] = "@(#)hunt.c 1.1 92/07/30 SMI"; /* from UCB 4.7 6/25/83 */
#endif

#include "tip.h"

extern char *getremote();
extern char *rindex();
extern int errno;

static	jmp_buf deadline;
static	int deadfl;

void
dead()
{

	deadfl = 1;
	longjmp(deadline, 1);
}

hunt(name)
	char *name;
{
	register char *cp, *dp;
	void (*f)();

	f = signal(SIGALRM, dead);
	while (cp = getremote(name)) {
		deadfl = 0;
		dp = rindex(cp, '/');
		uucplock = dp ? dp + 1 : cp;
		if (mlock(uucplock) < 0) {
			delock(uucplock);
			continue;
		}
		/*
		 * Straight through call units, such as the BIZCOMP,
		 * VADIC and the DF, must indicate they're hardwired in
		 *  order to get an open file descriptor placed in FD.
		 * Otherwise, as for a DN-11, the open will have to
		 *  be done in the "open" routine.
		 */
		if (!HW)
			break;
		if (setjmp(deadline) == 0) {
			alarm(10);
			errno = 0;
			if ((FD = open(cp, O_RDWR)) < 0 && errno != EBUSY) {
				fprintf(stderr, "tip: ");
				perror(cp);
			}
		}
		alarm(0);
		if (!deadfl && FD >= 0) {
			ioctl(FD, TIOCEXCL, 0);
			ioctl(FD, TIOCHPCL, 0);
			signal(SIGALRM, f);
			return ((int)cp);
		}
		delock(uucplock);
	}
	signal(SIGALRM, f);
	return (deadfl ? -1 : (int)cp);
}
