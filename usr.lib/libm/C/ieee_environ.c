#ifndef lint
static	char sccsid[] = "@(#)ieee_environ.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*	F77 interface to IEEE environment routines in libm. 	*/

#include <floatingpoint.h>

int ieee_flags_(action, mode, in, out, naction, nmode, nin, nout)
char action[], mode[], in[], out[];
int naction, nmode, nin, nout;
{
	int ret, i ; char *tmp;
	ret = ieee_flags(action, mode, in, &tmp) ;
    /* Find end of C string and copy the string from tmp to out. */
	for (i = 0 ; (i < nout) && (tmp[i] != 0) ; i++) out[i]=tmp[i];
    /* Pad C string with blanks. */
	while (i < nout) out[i++] = ' ' ;	
	return ret ;
}

int ieee_handler_(action, exception, hdl, naction, nexception)
char action[], exception[];
sigfpe_handler_type hdl;
int naction, nexception;
{
return ieee_handler(action, exception, hdl) ;
}

sigfpe_handler_type sigfpe_(code, hdl)
sigfpe_code_type *code;
sigfpe_handler_type hdl;
{
return sigfpe(*code, hdl);
}
