#ifndef lint
 static	char sccsid[] = "@(#)d_asin.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_asin(x)
double *x;
{
double asin();
return( asin(*x) );
}
