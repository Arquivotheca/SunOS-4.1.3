#ifndef lint
 static	char sccsid[] = "@(#)pow_dd.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double pow_dd(ap, bp)
double *ap, *bp;
{
double pow();

return(pow(*ap, *bp) );
}
