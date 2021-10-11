#ifndef lint
 static	char sccsid[] = "@(#)d_cosh.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_cosh(x)
double *x;
{
double cosh();
return( cosh(*x) );
}
