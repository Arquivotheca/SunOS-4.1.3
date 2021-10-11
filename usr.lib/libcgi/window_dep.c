#ifndef lint
static char	sccsid[] = "@(#)window_dep.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI SunWindow dependent functions
 */

/*
int     open_cgi_canvas 
*/

#include "cgipriv.h"

View_surface   *_cgi_vws;		/* current view surface */
caddr_t       (*_cgi_win_get)();        /* -> "window_get" if canvas call */

extern caddr_t  window_get();		/* only "suntools" library reference */

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: open_cgi_canvas					    */
/*                                                                          */
/*               This function replaces open_vws when using a canvas in	    */
/*               CGIPW mode.  It gets CGI ready to use the user-created	    */
/*               canvas specified.					    */
/*               This routine contains the only (current) reference into    */
/*               the "suntools" library, and sets a pointer to that routine */
/*               here to avoid unnecessary dependency on that lib.          */
/****************************************************************************/
Cerror          open_cgi_canvas(canvas, desc, name)
Canvas          canvas;
Ccgiwin        *desc;
Cint           *name;
{
    Pixwin         *pw;
    Cerror          err = NO_ERROR;

    if (canvas == (Canvas) NULL
	|| (pw = canvas_pixwin(canvas)) == (Pixwin *) NULL)
    {
	err = ENOTCCPW;
    }
    else
    {
	_cgi_win_get = window_get;
	err = _cgi_open_cgi_pw(pw, desc, name, canvas);
    }
    return (_cgi_errhand(err));
}
