#ifndef lint
 static	char sccsid[] = "@(#)d_sin.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_sin(x)
double *x;
{
double sin();
return( sin(*x) );
}
