#ifndef	lint
static char	sccsid[] = "@(#)exE-4.1.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * Example E-4.1, page 149
 */
#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <cgipw.h>
 
/*
 *      SunView window handles
 */
Frame   frame;
Canvas  canvas;
 
/*
 *      CGIPW canvas
 */
Ccgiwin vpw;
 
/*
 *      Canvas event handler
 */

canvas_event_proc( window, event)
Window  window;		/* unused */
Event   *event;
{
        if (event_is_down( event))
                return;

        switch (event_id( event)) {

        case MS_LEFT:
                draw_box_at( event_x( event), event_y( event));
                break;

        case MS_MIDDLE:
		printf("print_canvas_event: \n");
		printf("\tcanvas x,y = (%d,%d)\n",
				event_x(event), event_y(event));
		canvas_window_event( canvas, event);
		printf("\tpixwin region x,y = (%d,%d)\n",
				event_x(event), event_y(event));
                break;

        case MS_RIGHT:
                window_done( frame);
		close_cgi_pw( &vpw);
		close_pw_cgi();
                exit(0);

        default:
                break;
        }
}



main()		/********** MAIN **********/
{
	Cint     name;			/* goes unused in this example */

        frame = window_create( NULL, FRAME, 0);
        canvas = window_create( frame, CANVAS,
                CANVAS_AUTO_SHRINK,     FALSE,
                WIN_EVENT_PROC,         canvas_event_proc,
                CANVAS_WIDTH,           1000,
                CANVAS_HEIGHT,          1000,
                WIN_VERTICAL_SCROLLBAR, scrollbar_create(0),
                WIN_HORIZONTAL_SCROLLBAR,scrollbar_create(0),
                0);

        open_pw_cgi();
        open_cgi_canvas( canvas, &vpw, &name);
        window_main_loop( frame);
}


/*
 *      draw_box_at()
 *
 *      Draw a rectangular box using the passed x,y point as
 *      the upper left corner of the box.
 */
draw_box_at(x,y)
int     x,y;
{
        Ccoor   lr,ul;
        Ccoor   triangle[3];
        Ccoorlist coorlist;

        ul.x = x;
        ul.y = y;
        lr.x = x + 10;
        lr.y = y + 10;
        cgipw_rectangle( &vpw, &lr, &ul);
        triangle[0].x = x+2;
        triangle[0].y = y+7;
        triangle[1].x = x+8;
        triangle[1].y = y + 7;
        triangle[2].x = x + 5;
        triangle[2].y = y + 2;
        coorlist.n = 3;
        coorlist.ptlist = triangle;
        cgipw_polygon( &vpw, &coorlist);
}
