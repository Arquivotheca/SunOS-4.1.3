#ifndef lint
 static	char sccsid[] = "@(#)d_atn2.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_atn2(x,y)
double *x, *y;
{
double atan2();
return( atan2(*x,*y) );
}
