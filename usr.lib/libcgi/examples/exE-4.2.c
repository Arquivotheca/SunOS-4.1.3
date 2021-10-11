#ifndef lint
static char	sccsid[] = "@(#)exE-4.2.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
#endif	lint

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
/* SunCGI REFERENCE MANUAL, Rev. A, 9 May 1988, PN 800-1786-10 -- SunOS 4.0
 * Example E-4.2, page 151
 */
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <suntool/scrollbar.h>
#include <sunwindow/notify.h>
#include <cgipw.h>
#include <math.h>

Frame		frame;
Panel		panel;
Panel_item	button;
int		button_notify();
Canvas		canvas;
Pixwin		*pw;
Ccgiwin		desc;
Cint		name;
u_char		red[8], green[8], blue[8];


main()
{
	initialize_sunview();
	set_up_sunview_colors();
	initialize_cgipw();
	window_main_loop( frame);
}

int
initialize_sunview()	/* initialize Sunview */
{
	frame = window_create( NULL, FRAME, 0);

	panel = window_create(frame, PANEL, 0);
	button= panel_create_item( panel, PANEL_BUTTON,
		  PANEL_LABEL_IMAGE,	panel_button_image(panel, "Draw",4,0),
		  PANEL_NOTIFY_PROC,	button_notify,
		  0);
	window_fit_height( panel);
			
	canvas= window_create(frame, CANVAS,
			CANVAS_RETAINED,	 TRUE,
			CANVAS_WIDTH,		 750,
			CANVAS_HEIGHT,		 750,
			WIN_VERTICAL_SCROLLBAR,	 scrollbar_create(0),
			WIN_HORIZONTAL_SCROLLBAR,  scrollbar_create(0),
			CANVAS_FIXED_IMAGE,	    TRUE,
			CANVAS_AUTO_EXPAND,	    FALSE,
			CANVAS_AUTO_SHRINK,	    FALSE,
			0);

	pw = canvas_pixwin( canvas);
}

int
initialize_cgipw()	/* initialize cgi, view surface */
{
	open_pw_cgi();
	open_cgi_canvas( canvas, &desc, &name);
}

button_notify()
{
	Ccoor		center;
	Cint		radius;

	printf("we are in the panel button notify proc\n");
	pw_vector(pw, 000, 000, 100, 100, PIX_SRC| PIX_COLOR(1), 1);
	pw_vector(pw, 100, 100, 200, 200, PIX_SRC| PIX_COLOR(2), 1);
	pw_vector(pw, 200, 200, 300, 300, PIX_SRC| PIX_COLOR(3), 1);
	pw_vector(pw, 300, 300, 400, 400, PIX_SRC| PIX_COLOR(4), 1);
	pw_vector(pw, 400, 400, 500, 500, PIX_SRC| PIX_COLOR(5), 1);
	pw_vector(pw, 500, 500, 600, 600, PIX_SRC| PIX_COLOR(6), 1);
	pw_text( pw, 20, 20, PIX_SRC| PIX_COLOR(7), 0, "canvas text");

	interior_style( SOLIDI, ON);
	perimeter_color( 3);		/* set perimeter color */
	fill_color( 3);				/* set fill color */
	center.x = 400;
	center.y = 400;
	radius	 =  50;
	cgipw_circle( &desc, &center, radius);
}

set_up_sunview_colors()
{
    /* initialize Sunview colormap */
	red[0] = 255;	green[0] = 255;	 blue[0] = 255;	  /* white */
	red[1] = 000;	green[1] = 255;	 blue[1] = 000;	  /* green */
	red[2] = 000;	green[2] = 000;	 blue[2] = 255;	  /* blue */
	red[3] = 255;	green[3] = 255;	 blue[3] = 000;	  /* yellow */
	red[4] = 000;	green[4] = 255;	 blue[4] = 255;	  /* aqua */
	red[5] = 255;	green[5] = 000;	 blue[5] = 255;	  /* purple */
	red[6] = 255;	green[6] = 000;	 blue[6] = 000;	  /* red */
	red[7] = 000;	green[7] = 000;	 blue[7] = 000;	  /* black */

	pw_setcmsname( pw, "my_colors");
	pw_putcolormap(pw, 0, 8, red, green, blue);
}
