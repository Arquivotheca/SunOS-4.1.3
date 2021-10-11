#ifndef lint
static	char sccsid[] = "@(#)i_dnnt.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

int i_dnnt(x)
double *x;
{
int nint();

return( nint(*x) ) ;
}
