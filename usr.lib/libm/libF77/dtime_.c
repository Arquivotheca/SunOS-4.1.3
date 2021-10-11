#ifndef lint
 static	char sccsid[] = "@(#)dtime_.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 *
 * Returns the delta time since the last call to dtime.
 *
 * calling sequence:
 * 	real time(2)
 * 	call dtime(time)
 * where:
 * 	the 2 element array time will receive the user and system
 * 	elapsed time since the last call to dtime, or since the start
 * 	of execution.
 *
 * This routine can be called as function, and returns the sum of
 * user and system times. The time_array argument must always be given.
 *
 * The resolution for all timing is governed by _hz in /vmunix
 */

#include <sys/time.h>
#include <sys/resource.h>
#include <math.h>

#define	 RUSAGE_SELF	0

struct tb { float usrtime; float systime; };

float last_utime= 0. , last_stime =0. ;

FLOATFUNCTIONTYPE dtime_(dt) struct tb *dt;
{	struct rusage rusage;
	float f;
	float temp;

	getrusage(RUSAGE_SELF, &rusage);

	temp = (float) (rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec * .000001);
	dt->usrtime = temp - last_utime;
	last_utime = temp;

	temp = (float) (rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec * .000001);
	dt->systime = temp - last_stime;
	last_stime = temp;

	f = dt->usrtime + dt->systime;
	RETURNFLOAT(f);
}
