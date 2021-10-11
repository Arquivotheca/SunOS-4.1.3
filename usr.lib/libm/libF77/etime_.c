#ifndef lint
 static	char sccsid[] = "@(#)etime_.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 *
 * Return the elapsed execution time for this process.
 *
 * calling sequence:
 * 	real time(2)
 * 	call etime (time)
 * where:
 * 	the 2 element array, time, will receive the user and system
 * 	elapsed time since the start of execution.
 *
 *	This routine can be called as function, and returns the sum of
 *	user and system times. The time array argument must always be given.
 *
 * The resolution for all timing is governed by _hz in /vmunix
 */


#include <sys/time.h>
#include <sys/resource.h>
#include <math.h>

#define	 RUSAGE_SELF	0

struct tb { float usrtime; float systime; };

FLOATFUNCTIONTYPE etime_(et) struct tb *et;
{	struct rusage rusage;
	float f;

	getrusage(RUSAGE_SELF, &rusage);
	et->usrtime = (float) (rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec * .000001 );
	et->systime = (float) (rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec * .000001 );
	f= et->usrtime + et->systime;
	RETURNFLOAT(f);
}
