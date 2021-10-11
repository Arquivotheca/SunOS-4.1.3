#ifndef lint
 static	char sccsid[] = "@(#)d_sinh.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_sinh(x)
double *x;
{
double sinh();
return( sinh(*x) );
}
