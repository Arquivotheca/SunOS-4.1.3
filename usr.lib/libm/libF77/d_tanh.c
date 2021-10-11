#ifndef lint
 static	char sccsid[] = "@(#)d_tanh.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_tanh(x)
double *x;
{
double tanh();
return( tanh(*x) );
}
