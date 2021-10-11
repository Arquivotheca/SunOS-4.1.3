#ifndef lint
static char	sccsid[] = "@(#)inputsubs.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
#endif

/*
 * Copyright (c) 1985, 1986, 1987, 1988, 1989 by Sun Microsystems, Inc. 
 * Permission to use, copy, modify, and distribute this software for any 
 * purpose and without fee is hereby granted, provided that the above 
 * copyright notice appear in all copies and that both that copyright 
 * notice and this permission notice are retained, and that the name 
 * of Sun Microsystems, Inc., not be used in advertising or publicity 
 * pertaining to this software without specific, written prior permission. 
 * Sun Microsystems, Inc., makes no representations about the suitability 
 * of this software or the interface defined in this software for any 
 * purpose. It is provided "as is" without express or implied warranty.
 */

#include "cgidefs.h"
#include "cgiminicon.h"

/*
_cgi_int_dev_conv
*/

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_int_dev_conv	 			            */
/*                                                                          */
/*		                 _cgi_int_dev_conv			    */
/****************************************************************************/
int             _cgi_int_dev_conv(devclass, devnum)
Cdevoff         devclass;
Cint            devnum;
{
    int             num;
    switch (devclass)
    {
    case IC_STRING:
	{
	    num = OFF1 + devnum;
	    break;
	}
    case IC_STROKE:
	{
	    num = OFF2 + devnum;
	    break;
	}
    case IC_LOCATOR:
	{
	    num = OFF3 + devnum;
	    break;
	}
    case IC_VALUATOR:
	{
	    num = OFF4 + devnum;
	    break;
	}
    case IC_CHOICE:
	{
	    num = OFF5 + devnum;
	    break;
	}
    case IC_PICK:
	{
	    num = OFF6 + devnum;
	    break;
	}
    }
    return (num);
}
