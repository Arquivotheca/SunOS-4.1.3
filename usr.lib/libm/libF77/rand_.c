#ifndef lint
 static	char sccsid[] = "@(#)rand_.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif

/*
 *
 * Routines to return random values
 *
 * calling sequence:
 *	double precision d, drand
 *	i = irand(iflag)
 *	x = rand(iflag)
 *	d = drand(iflag)
 * where:
 *	If arg is 1, generator is restarted. If arg is 0, next value
 *	is returned. Any other arg is a new seed for the generator.
 *	Integer values will range from 0 thru 2147483647.
 *	Real values will range from 0.0 thru 1.0
 *	(see random(3))
 */

#include <math.h>

#define	RANDMAX		2147483647

#define		STATESIZE	256
char	*initstate();
char	state[STATESIZE];

long irand_(iarg)
long *iarg;
{
	if (*iarg) initstate( (unsigned) *iarg,state,STATESIZE);

	return( random() );
}

FLOATFUNCTIONTYPE
rand_(iarg)
long *iarg;
{
	float result;
	if (*iarg) initstate( (unsigned) *iarg,state,STATESIZE);
	result = (float) ( ((double)random())  / ( (double)RANDMAX ) );
	RETURNFLOAT(result);
}

double drand_(iarg)
long *iarg;
{
	if (*iarg) initstate( (unsigned) *iarg,state,STATESIZE);
	return( (double)(random())/(double)RANDMAX );
}
