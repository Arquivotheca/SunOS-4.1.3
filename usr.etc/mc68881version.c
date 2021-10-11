#ifdef sccsid
static char     sccsid[] = "@(#)mc68881version.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc. 
 */

/*
 * mc68881version command - determines 
	whether 68881 is present 
	what mask level 
	what frequency 
 */

#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#define FREQUENCY_CONSTANT 3.73e+7	/* constant = time * frequency */

extern void 
clear81();
bigrem81();

extern int 
A93N();

static int      EMTSignalled;

static int 
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
 * Procedure to determine if a physical 68881 is present.
 * Returns 0 if absent, 1 if present. 
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

	if (minitfp() == 0) {
		printf(" No MC68881 available. \n");
		return;
	}
	if (A93N()) {
		printf(" MC68881 available; mask set appears to be A93N. \n");
	} else {
		printf(" MC68881 available; mask set appears to be A79J. \n");
	}
	getrusage(RUSAGE_SELF, &ru1);
	bigrem81();
	getrusage(RUSAGE_SELF, &ru2);
	printf(" Approximate MC68881 frequency %4.1f MHz. \n", FREQUENCY_CONSTANT / 
		(1.0e6 * (double) (ru2.ru_utime.tv_sec  - ru1.ru_utime.tv_sec) +
  		         (double) (ru2.ru_utime.tv_usec - ru1.ru_utime.tv_usec)));
}
