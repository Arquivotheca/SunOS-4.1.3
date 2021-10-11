#ifndef lint
static char	sccsid[] = "@(#)lookup.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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

/*
color_table
cgipw_color_table
*/

#include "cgipriv.h"

View_surface   *_cgi_vws;	/* current view surface */

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: color_table	 					    */
/*                                                                          */
/*		sets color lookup table entries				    */
/****************************************************************************/

Cerror          color_table(istart, clist)
Cint            istart;		/* starting address */
Ccentry        *clist;		/* color triples and number of entries */
{
    int             ivw, err;
/*     char    cmsname[DEVNAMESIZE];	wds removed 850723 */
    err = _cgi_err_check_4();
    if (!err)
	err = _cgi_check_color(istart);
    if (!err)
	err = _cgi_check_color(istart + clist->n - 1);
    if (!err)
    {
	ivw = 0;
	while (_cgi_bump_vws(&ivw))
	{
/*      pw_putcolormap (_cgi_vws->sunview.pw, istart, nt, ra, ga, ba);   */
/*      pw_getcmsname (_cgi_vws->sunview.pw, cmsname);  */
	    pw_putcolormap(_cgi_vws->sunview.pw, istart,
			   clist->n, clist->ra, clist->ga, clist->ba);
/*          pw_setcmsname (_cgi_vws->sunview.pw, cmsname);  */
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_color_table 					    */
/*                                                                          */
/*		sets color lookup table entries				    */
/****************************************************************************/

Cerror          cgipw_color_table(desc, istart, clist)
Ccgiwin        *desc;
Cint            istart;		/* starting address */
Ccentry        *clist;		/* color triples and number of entries */
{
    int             err;
    err = _cgi_err_check_4();
    if (!err)
	err = _cgi_check_color(istart);
    if (!err)
	err = _cgi_check_color(istart + clist->n - 1);
    if (!err)
    {
	pw_putcolormap(desc->vws->sunview.pw, istart,
		       clist->n, clist->ra, clist->ga, clist->ba);
    }
    return (err);
}
