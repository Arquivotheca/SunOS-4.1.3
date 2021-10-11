#ifndef lint
static char	sccsid[] = "@(#)cgipw.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI pixwin functions
 */

/*
int     open_pw_cgi 
int     open_cgi_pw 
Cerror _cgi_open_cgi_pw
int     close_cgi_pw 
int     close_pw_cgi 
*/

#include "cgipriv.h"
#include <fcntl.h>

Gstate _cgi_state;		/* CGI global state */
View_surface   *_cgi_view_surfaces[MAXVWS];	/* view surface information */
View_surface   *_cgi_vws;	/* current view surface */
int             _cgi_vwsurf_count;	/* number of view surfaces */
Outatt         *_cgi_att;	/* structure containing current attributes */

Clogical _cgi_get_r_screen();
Outatt         *_cgi_alloc_output_att();

extern int      gp1_rop();

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: open_pw_cgi                                                */
/*                                                                          */
/*               This function replaces open_cgi when using CGIPW mode.	    */
/****************************************************************************/
int             open_pw_cgi()
{
    int             err;

    err = _cgi_open_cgi(1, 1);	/* cgipw_mode==1, use_pw_size==1 */
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: open_cgi_pw                                                */
/*                                                                          */
/*               This function replaces open_vws when using CGIPW mode.	    */
/*               It gets CGI ready to use the user-created pixwin specified.*/
/****************************************************************************/
Cerror          open_cgi_pw(pw, desc, name)
Pixwin         *pw;
Ccgiwin        *desc;
Cint           *name;
{
    Cerror          err;

    err = _cgi_open_cgi_pw(pw, desc, name, (Canvas) 0);
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_open_cgi_pw					    */
/*                                                                          */
/*               Do all the work of associating an existing pixwin	    */
/*               with a view surface structure.				    */
/****************************************************************************/
Cerror          _cgi_open_cgi_pw(pw, desc, name, canvas_hndl)
Pixwin         *pw;
Ccgiwin        *desc;
Cint           *name;
Canvas          canvas_hndl;	/* !0 == this is the pixwin hndl to a canvas */
{
    register View_surface *cur_vws;	/* ptr to current view surf info */
    Cerror          err;
    Outatt         *cur_att;	/* will get attribute structure */

    if (!_cgi_state.cgipw_mode)	/* opened via open_pw_cgi? */
	err = ENOTCSTD;		/* Function not compatible with standard CGI */
    else
    {
	err = _cgi_check_state_5();
	if (!err)
	{
	    *name = _cgi_new_vwsurf_name();
	    if (*name < 0)
		err = EMAXVSOP;	/* Maximum no. of view surfaces already open. */
	    else
	    {
		/* Don't malloc unless everything else is OK */
		cur_vws = (View_surface *) calloc(1, sizeof(View_surface));
		if (cur_vws == (View_surface *) NULL)
		{
		    err = EMEMSPAC;
		    _cgi_free_name(*name);
		}
		else
		{
		    /*
		     * Initialize CGIPW surface's attribute from common_att.
		     * This will capture any attributes set before opening a
		     * CGIPW view surface. 
		     */
		    cur_att = _cgi_alloc_output_att(_cgi_state.common_att);
		    if (cur_att == (Outatt *) NULL)
		    {
			err = EMEMSPAC;
			_cgi_free_name(*name);
			free((char *) cur_vws);
		    }
		}
	    }
	}
    }

    if (!err)
    {				/* if mallocs all worked */
	Cos             incoming_cgi_state = _cgi_state.state;

	cur_vws->att = _cgi_att = cur_att;
	_cgi_view_surfaces[*name] = _cgi_vws = cur_vws;
	_cgi_state.state = VSAC;
	cur_vws->active = 1;
	cur_vws->sunview.windowfd = pw->pw_clipdata->pwcd_windowfd;
	IDENTITY_SCALE(cur_vws->xform.scale.x);
	IDENTITY_SCALE(cur_vws->xform.scale.y);
	cur_vws->xform.off.x = 0;
	cur_vws->xform.off.y = 0;
	cur_vws->xform.win_off.x = 0;
	cur_vws->xform.win_off.y = 0;
	cur_vws->sunview.canvas = canvas_hndl;
	cur_vws->sunview.depth = pw->pw_pixrect->pr_depth;
	cur_vws->sunview.pw = pw;
	cur_vws->sunview.orig_pw = pw;
	win_getsize(cur_vws->sunview.windowfd, &cur_vws->sunview.lock_rect);
	(void) _cgi_get_r_screen(cur_vws, &cur_vws->real_screen);
	cur_vws->vport = cur_vws->real_screen;
	/* if first view surface opened, reset cgi's state */
	if (incoming_cgi_state == CGOP)
	    _cgi_reset_internals();
	(void) reset_to_defaults();
	/*
	 * these scale factors must be reset from defaults because VDC space is
	 * not 32K square in cgipw mode 
	 */
	cur_att->pattern.dx = cur_vws->real_screen.r_width / 100;
	cur_att->pattern.dy = cur_vws->real_screen.r_width / 100;
	cur_att->text.attr.height = cur_vws->real_screen.r_width / 32;
	cur_vws->sunview.sig_function = 0;

	if (pw->pw_pixrect->pr_ops->pro_rop == gp1_rop)
	    if (_cgi_initialize_gp1(cur_vws))
		cur_vws->sunview.gp_att = (Gp1_attr *) NULL;

	/* Enable CGIPW mode pixwin to do CGI input. */
	if (!canvas_hndl)
	    (void) fcntl(cur_vws->sunview.windowfd, F_SETFL, FASYNC | FNDELAY);

	/* fix for PW bug: pw_lock forgets retained & draw only on screen */
	/* ## WCL 12/18/86 Can this be removed?  Does PW bug still exist? */
	pw_exposed(pw);		/* Note that this undoes pw_restrictclipping */
	desc->vws = _cgi_vws;
    }
    else
    {
	/*
	 * Explicitly invalidate the contents of the descriptor being returned
	 * to forestall later segmentation violations when the client calls
	 * CGIPW functions with this bogus descriptor. 
	 */
	*name = -1;
	desc->vws = (View_surface *) NULL;
    }
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: close_cgi_pw                                               */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
/* Forced to match erroneous documentation, page F-2. */
int             close_cgi_pw(desc)
Ccgiwin        *desc;		/* CGI pixwin descriptor */
{
    int             err, name;

    SETUP_CGIWIN(desc);
    for (name = 0; name < _cgi_vwsurf_count; name++)
	if (_cgi_view_surfaces[name] == desc->vws)
	{
	    err = close_vws(name);
	    desc->vws = (View_surface *) NULL;
	    return (err);
	}

    return (EVSNOTOP);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: close_pw_cgi                                               */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Cerror          close_pw_cgi()
{
                    return (_cgi_close_cgi());
}
