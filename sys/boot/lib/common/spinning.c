/*
 * @(#)spinning.c 1.1 92/07/30 Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include <sys/types.h>
#include <stand/saio.h>
#include <mon/sunromvec.h>

char msg_spinup[] = "\n\007Waiting for disk to spin up...\n\n\
Please start it, if necessary, -OR- press any key to quit.\n\n";
char msg_noctlr[] = "No controller at mbio %x\n";

#ifdef OPENPROMS
#define	millitime() prom_gettime()
extern	int	prom_mayget();
#else
#define	millitime() (*romp->v_nmiclock)
#define	prom_mayget	(*romp->v_mayget)
#endif !OPENPROMS
/*
 * Returns 0 if disk is apparently not spinning, after waiting awhile.
 * Returns 1 if disk is already spinning on entry to this routine.
 * Returns 2 if disk starts spinning sometime after entry to this routine.
 * Returns <0 if (*isready)() returns a negative result at any time.
 *
 * If on initial entry, after a suitable delay, the disk is not spinning,
 * we produce a message to remind the user to turn on the disk.
 */
int
isspinning(isready, addr, data)
	int (*isready)();
	char *addr;
	int data;
{
	register int r;
	long t;

	/*
	 * Wait for disk to spin up, if necessary.
	 */
	r = (*isready)(addr, data);
	if (r < 0)
		return (r);		/* Error */
	if (r)
		return (1);		/* Already spinning */
	DELAY(1000000);			/* RTZ can take a long time */
	r = (*isready)(addr, data);
	if (r < 0)
		return (r);
	if (r)
		return (2);		/* Spun up while we waited */
	while (prom_mayget() != -1)		/* clear typeahead */
		;
	printf(msg_spinup);
	t = (3*60*1000) + millitime(); /* quitting time in 3 minutes */

	for (;;) {
		r = (*isready)(addr, data);	/* Test disk */
		if (r < 0)
			return (r);
		if (r)
			return (2);
		if (prom_mayget() != -1)	/* Test keyboard */
			return (0);
		if (millitime() > t) {	/* Test timer */
			printf("Giving up...\n");
			return (0);
		}
	}					/* Loop */
}
