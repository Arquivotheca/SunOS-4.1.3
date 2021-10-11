#ifndef lint
static char	sccsid[] = "@(#)metainit.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
activate_vws
deactivate_vws
_cgi_reset_vws_based_state
*/

#include "cgipriv.h"

Gstate          _cgi_state;	/* CGI global state */
View_surface   *_cgi_vws;
Outatt         *_cgi_att;
View_surface   *_cgi_view_surfaces[MAXVWS];	/* view surface information */
int             _cgi_vwsurf_count;	/* number of view surfaces */

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: activate_vws	 					    */
/*                                                                          */
/*		activate viewsurface		  			    */
/****************************************************************************/

Cerror          activate_vws(name)
Cint            name;		/* view surface name */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_context(name);
    if (!err)
    {
	if (!_cgi_view_surfaces[name]->active)
	{
	    _cgi_view_surfaces[name]->active = 1;
	    _cgi_reset_vws_based_state();
	}
	else
	    err = EVSISACT;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: deactivate_vws	 					    */
/*                                                                          */
/*		deactivate viewsurface		  			    */
/****************************************************************************/

Cerror          deactivate_vws(name)
Cint            name;		/* view surface name */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_context(name);
    if (!err)
    {
	if (_cgi_view_surfaces[name]->active)
	{
	    _cgi_view_surfaces[name]->active = 0;
	    _cgi_reset_vws_based_state();
	}
	else
	    err = EVSNTACT;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_reset_vws_based_state				    */
/*                                                                          */
/*		Reset _cgi_state.state to VSAC or VSOP, as appropriate.	    */
/****************************************************************************/

_cgi_reset_vws_based_state()
{
    int             i;
    int             open = 0;
    int             active = 0;

    for (i = 0; i < _cgi_vwsurf_count; i++)
    {
	if (_cgi_view_surfaces[i] != (View_surface *) NULL)
	{
	    open++;
	    if (_cgi_view_surfaces[i]->active)
		active++;
	}
    }

    if (_cgi_state.state != CGCL)
    {
	if (open > 0)
	{
	    if (active > 0)
		_cgi_state.state = VSAC;
	    else
		_cgi_state.state = VSOP;
	}
	else
	{
	    /*
	     * If there are no open view surfaces, set global pointers
	     * to the state they would be in immediately after open_cgi().
	     */
	    _cgi_state.state = CGOP;
	    _cgi_vws = (View_surface *) 0;
	    _cgi_att = _cgi_state.common_att;
	}
    }
}
