#ifdef sccsid
static char     sccsid[] = "@(#)mc68881version.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc. 
 */

/*
 * mc68881version command - determines whether 68881/2 is present what mask
 * level what frequency 
 */

#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#define FREQUENCY_CONSTANT 3.73e+7	/* constant = time * frequency */

extern void clear81(), bigrem81(), slowsaxpy(), fastsaxpy();

extern int A93N(), M3A93N();

static int      EMTSignalled;

/*ARGSUSED*/
static void
EMTSpecial(sig, code, scp)
	int             sig, code;
	struct sigcontext *scp;

/*
 * Special EMT handler that sets EMTSignalled if invoked and increments pc by
 * 6 = length of instruction fmoveml	<zeros>,fpcr/fpsr 
 */

{
	EMTSignalled = 1;
	scp->sc_pc += 6;
}

static int
minitfp()
/*
 * Procedure to determine if a physical 68881 is present. Returns 0 if
 * absent, 1 if present. 
 */

{
	struct sigvec   new, old;

	new.sv_handler = EMTSpecial;
	new.sv_mask = 0;
	new.sv_onstack = 0;
	sigvec(SIGEMT, &new, &old);
	EMTSignalled = 0;
	clear81();
	sigvec(SIGEMT, &old, &new);
	return (1 - EMTSignalled);
}

main()
{
	struct rusage   ru1, ru2;
	static double   a[1000], b[1000];
	int             i, is68882;
	double          fasttime, slowtime;
	static char     mb81g[] = "B81G or 6A93N", mb96m[] = "B96M", ma93n[] = "A93N or B79C", ma79j[] = "A79J";
	char           *mask;

	if (minitfp() == 0) {
		printf(" No MC68881 or 68882 available. \n");
		return;
	}
	for (i = 0; i < 1000; i++) {	/* Initialize data */
		a[i] = -i;
		b[i] = 1000 - i;
	}
	getrusage(RUSAGE_SELF, &ru1);
	for (i = 0; i < 800; i++)
		slowsaxpy(a, b, 3.14, 1000);
	getrusage(RUSAGE_SELF, &ru2);
	slowtime = (1.0e6 * (double) (ru2.ru_utime.tv_sec - ru1.ru_utime.tv_sec) +
		    (double) (ru2.ru_utime.tv_usec - ru1.ru_utime.tv_usec));
	for (i = 0; i < 1000; i++) {	/* Initialize data */
		a[i] = -i;
		b[i] = 1000 - i;
	}
	getrusage(RUSAGE_SELF, &ru1);
	for (i = 0; i < 800; i++)
		fastsaxpy(a, b, 3.14, 1000);
	getrusage(RUSAGE_SELF, &ru2);
	fasttime = (1.0e6 * (double) (ru2.ru_utime.tv_sec - ru1.ru_utime.tv_sec) +
		    (double) (ru2.ru_utime.tv_usec - ru1.ru_utime.tv_usec));
	is68882 = (slowtime - fasttime) / fasttime > 0.03;
	if (is68882)
		printf(" An MC68882 is available. \n");
	else
		printf(" An MC68881 is available. \n");
	if (A93N()) {
		if (M3A93N())
			mask = is68882 ? mb96m : mb81g;
		else
			mask = ma93n;
	} else {
		mask = ma79j;
	}
	printf(" Mask set appears to be %s or equivalent. \n", mask);
	getrusage(RUSAGE_SELF, &ru1);
	bigrem81();
	getrusage(RUSAGE_SELF, &ru2);
	printf(" Frequency appears to be approximately %4.1f MHz. \n", FREQUENCY_CONSTANT /
	     (1.0e6 * (double) (ru2.ru_utime.tv_sec - ru1.ru_utime.tv_sec) +
	      (double) (ru2.ru_utime.tv_usec - ru1.ru_utime.tv_usec)));
	exit(0);
	/* NOTREACHED */
}
