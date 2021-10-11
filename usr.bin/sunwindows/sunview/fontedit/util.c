#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)util.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <signal.h>
#include "other_hs.h"
#include "fontedit.h"
#include "externs.h"

		    
/*
 * Draw a box in the given "color".
 */

fted_FTdraw_rect(window, r, color)
struct pixwin	*window;	/* where to draw the box */
struct rect	*r;		/* box to be drawn */
int		color;		/* color to draw it in */
{
	register int x, y, h, w;

	x = r->r_left;
	y = r->r_top;
	w = x + r->r_width;
	h = y + r->r_height;

	pw_vector(window, x, y, w, y, PIX_SRC, color);
	pw_vector(window, w, y, w, h, PIX_SRC, color);
	pw_vector(window, w, h, x, h, PIX_SRC, color);
	pw_vector(window, x, h, x, y, PIX_SRC, color);
}

fted_message_user (format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
char   *format, *arg1, *arg2, *arg3, *arg4, *arg5, *arg6, *arg7, *arg8;
{
    char    buf[256];

    sprintf (buf, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    panel_set(fted_msg_item, 
	      PANEL_LABEL_STRING, 	buf, 
	      0);
}
