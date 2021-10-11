#ifndef lint
 static	char sccsid[] = "@(#)d_sqrt.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_sqrt(x)
double *x;
{
double sqrt();
return( sqrt(*x) );
}
