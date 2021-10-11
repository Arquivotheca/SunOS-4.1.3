#ifndef lint
 static	char sccsid[] = "@(#)d_acos.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_acos(x)
double *x;
{
double acos();
return( acos(*x) );
}
