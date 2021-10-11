#ifndef lint
 static	char sccsid[] = "@(#)d_int.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_int(x)
double *x;
{
double floor();

return( (*x >= 0) ? floor(*x) : -floor(- *x) );
}
