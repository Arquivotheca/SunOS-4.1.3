#ifndef lint
 static	char sccsid[] = "@(#)d_abs.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_abs(x)
double *x;
{
if(*x >= 0)
	return(*x);
return(- *x);
}
