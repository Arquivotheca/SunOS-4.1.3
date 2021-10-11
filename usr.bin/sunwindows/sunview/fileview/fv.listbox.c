#ifndef lint
static  char sccsid[] = "@(#)fv.listbox.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */

#include <stdio.h>
#include <errno.h>

#ifdef SV1
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <suntool/scrollbar.h>
#else
#include <view2/view2.h>
#include <view2/panel.h>
#include <view2/canvas.h>
#include <view2/scrollbar.h>
#endif

#include "fv.port.h"
#include "fv.h"
#include "fv.extern.h"

#define MARGIN 		5
#define MAXCANVAS	2

static struct Lb_data {
	Canvas cnvs;			/* Canvas handle */
	int *item_chosen;		/* Selected item */
	int *nitems;			/* Number of items */
	char **item;			/* Item string array */
	ROUTINE callback_fn;		/* Called when item selected */
} Lb_glob[MAXCANVAS];

static struct Lb_data *Lb;		/* Current listbox data */
struct Lb_data *get_handle();


/*ARGSUSED*/
Canvas
fv_create_listbox(frame, canvas_rows, item_chosen, nitems, item, callback_fn)
	Frame frame;			/* Owner's Frame */
	int canvas_rows;
	int *item_chosen;
	int *nitems;			/* Number of items */
	char **item;			/* String array */
	ROUTINE callback_fn;		/* Callback fn for selection */
{
	register int i;			/* Index */
	Canvas cnvs;			/* Canvas */
	static Notify_value lb_event();
	static void lb_repaint();


	/* Handle selection and repainting for a group of strings in a 
	 * list box.  Only one item can be chosen, and at this time the
	 * listbox's parent may wish to be informed.  The number of 
	 * items in the list may grow.
	 * We use the canvas's handle to locate the correct canvas data.
	 */

	if ((cnvs = window_create(frame, CANVAS,
			CANVAS_FAST_MONO, TRUE,
			CANVAS_AUTO_SHRINK, FALSE,
			CANVAS_RETAINED, FALSE,
			CANVAS_HEIGHT, canvas_rows*Fv_fontsize.y,
#ifndef SV1
			WIN_CONSUME_EVENT, MS_LEFT,
#endif
			WIN_VERTICAL_SCROLLBAR,
			   scrollbar_create(
#ifdef SV1
				SCROLL_PLACEMENT, SCROLL_EAST,
#endif
				SCROLL_LINE_HEIGHT, Fv_fontsize.y,
				0),
			0)) == NULL)
	{
		fv_putmsg(TRUE, Fv_message[MEWIN], 0, 0);
		return(NULL);
	}

	/* Remember global data (in first available slot) */

	for (i = 0; i<MAXCANVAS; i++)
		if (Lb_glob[i].cnvs == NULL)
		{
#ifdef SV1
			Lb_glob[i].cnvs = cnvs;
#else
			Lb_glob[i].cnvs = vu_get(cnvs, CANVAS_NTH_PAINT_WINDOW, 0);
#endif
			Lb_glob[i].item_chosen = item_chosen;
			Lb_glob[i].nitems = nitems;
			Lb_glob[i].item = item;
			Lb_glob[i].callback_fn = callback_fn;
			break;
		}
	if (i == MAXCANVAS)
		return(NULL);

	/* Set repaint, Watch for events on canvas */

	window_set(cnvs, CANVAS_REPAINT_PROC, lb_repaint, 0);
	notify_interpose_event_func(Lb_glob[i].cnvs, lb_event, NOTIFY_SAFE);

	return(cnvs);
}



static Notify_value
lb_event(cnvs, evnt, arg, typ)		/* List box event handler */
	Canvas cnvs;
	Event *evnt;
	Notify_arg arg;
	Notify_event_type typ;
{
	register int i, y;
	int w;
	PAINTWIN pw;

	if (event_id(evnt) == MS_LEFT && event_is_up(evnt) && 
		(Lb=get_handle(cnvs)))
	{
		y = event_y(evnt)
#ifdef SV1
			+(int)scrollbar_get(
			window_get(cnvs, WIN_VERTICAL_SCROLLBAR),
			SCROLL_VIEW_START, 0)
#endif
			;

		if ( (i = y / Fv_fontsize.y) < *Lb->nitems )
		{
			/* Yes, something has been selected... */
#ifdef SV1
			pw = (PAINTWIN)canvas_pixwin(cnvs);
			w = (int)window_get(cnvs, WIN_WIDTH);
#else
			pw = cnvs;
			w = (int)vu_get(cnvs, WIN_WIDTH);	/* ? */
#endif

			if (*Lb->item_chosen!=EMPTY)
			{
				/* Turn off old selection */

				y = *Lb->item_chosen * Fv_fontsize.y + 
					(Fv_fontsize.y>>2);
				pw_rop(pw, 0, y, w, Fv_fontsize.y,
					PIX_NOT(PIX_SRC)^PIX_DST, 
					(Pixrect *)0, 0, 0);
			}

			/* Turn on new */

			y = i * Fv_fontsize.y+(Fv_fontsize.y>>2);
			pw_rop(pw, 0, y, w, Fv_fontsize.y,
				PIX_NOT(PIX_SRC)^PIX_DST, 
				(Pixrect *)0, 0, 0);
			*Lb->item_chosen = i;
			if (Lb->callback_fn)
				Lb->callback_fn();
		}
	}

	return(notify_next_event_func(cnvs, evnt, arg, typ));
}



static void
lb_repaint(cnvs, pw, area)		/* Repaint list box */
	Canvas cnvs;
	PAINTWIN pw;
	Rectlist *area;
{
	register int i, j;			/* Indeces */
	Rect *rn = &area->rl_bound;		/* Bounding area */

#ifdef SV1
	if ((Lb=get_handle(cnvs))==NULL) 
#else
	if ((Lb=get_handle(pw))==NULL) 
#endif
		return;

	i = rn->r_top / Fv_fontsize.y;
	if (i)
		i--;
	if (i >= *Lb->nitems)
		return;
	j = (rn->r_top+rn->r_height) / Fv_fontsize.y + 1;
	if (j >= *Lb->nitems)
		j = *Lb->nitems-1;

	while (i<=j)
	{
		pw_text(pw, MARGIN, (i+1)*Fv_fontsize.y, PIX_SRC, Fv_font,
			Lb->item[i]);
		i++;
	}

	/* Repair damaged selection */

	if (*Lb->item_chosen != EMPTY)
		pw_rop(pw, rn->r_left, 
			(*Lb->item_chosen * Fv_fontsize.y)+(Fv_fontsize.y>>2),
			rn->r_width, Fv_fontsize.y,
			PIX_NOT(PIX_SRC)^PIX_DST, 
			(Pixrect *)0, 0, 0);
}


#ifdef NEVER
fv_box_canvas(panel, x, y, w, h)
	Panel panel;
	int x, y, w, h;
{
	/* Draw box around canvas */

	panel_create_item(panel, PANEL_LINE,
		PANEL_VALUE_X, x,
		PANEL_VALUE_Y, y-1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_VALUE_DISPLAY_LENGTH, w,
		0);
	panel_create_item(panel, PANEL_LINE,
		PANEL_VALUE_X, x-1,
		PANEL_VALUE_Y, y-1,
		PANEL_LAYOUT, PANEL_VERTICAL,
		PANEL_VALUE_DISPLAY_LENGTH, h+1,
		0);
	panel_create_item(panel, PANEL_LINE,
		PANEL_VALUE_X, x,
		PANEL_VALUE_Y, h+y,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_VALUE_DISPLAY_LENGTH, w,
		0);
	panel_create_item(panel, PANEL_LINE,
		PANEL_VALUE_X, w+x,
		PANEL_VALUE_Y, y,
		PANEL_LAYOUT, PANEL_VERTICAL,
		PANEL_VALUE_DISPLAY_LENGTH, h,
		0);
}
#endif

fv_lb_delete(cnvs)
	Canvas cnvs;
{
	if (Lb = get_handle(cnvs))
		Lb->cnvs = NULL;
}

struct Lb_data *
get_handle(cnvs)
	Canvas cnvs;
{
	int i;

	/* Check that we've got correct canvas handle... */

	if (Lb == NULL || Lb->cnvs != cnvs)
	{
		for (i = 0; i<MAXCANVAS; i++)
			if (Lb_glob[i].cnvs == cnvs)
			{
				Lb = &Lb_glob[i];
				return(Lb);
			}
		(void)fprintf(stderr, Fv_message[MELISTBOX]);
		return(NULL);
	}
	return(Lb);
}
