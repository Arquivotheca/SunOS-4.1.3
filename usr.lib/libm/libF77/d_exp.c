#ifndef lint
 static	char sccsid[] = "@(#)d_exp.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_exp(x)
double *x;
{
double exp();
return( exp(*x) );
}
