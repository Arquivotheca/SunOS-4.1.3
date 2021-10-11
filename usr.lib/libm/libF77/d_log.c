#ifndef lint
 static	char sccsid[] = "@(#)d_log.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_log(x)
double *x;
{
double log();
return( log(*x) );
}
