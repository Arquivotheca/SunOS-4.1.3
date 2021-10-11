#ifndef lint
static	char sccsid[] = "@(#)i_nint.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

int i_nint(x)
float *x;
{
int ir_nint_();

return( ir_nint_(x) ) ;
}
