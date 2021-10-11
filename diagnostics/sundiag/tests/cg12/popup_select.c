
#ifndef lint
static  char sccsid[] = "@(#)popup_select.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <esd.h>

static short ok_image[] = {
#include <ok.icon>
};
mpr_static(ok_icon, 64, 64, 1, ok_image);

static
Panel_item		list_item;


/**********************************************************************/
int
popup_select(tl, r, font)
/**********************************************************************/
struct test *tl;
Rect *r;
Pixfont *font;

{
    extern Frame init_popup();
    Frame	popup;
    int		answer;

    /* create the popup window */
    popup = init_popup("    Select test(s):", tl, r, font);

    /* make the user answer */
    answer = (int) window_loop(popup);

    /* destroy the popup window*/
    (void)window_set(popup, FRAME_NO_CONFIRM, TRUE, 0);
    (void)window_destroy(popup);

    return answer;
}


/**********************************************************************/
static Frame
init_popup(message, tl, sr, font)
/**********************************************************************/
char	*message;
struct test *tl;
Rect *sr;
Pixfont *font;

{
    extern void		ok_proc();
    Frame		popup_frame;
    Panel		panel;
    int			left, top, width;
    int			swidth, sheight;
    int			i;
    Rect		*r;
    Pixrect		*pr;
    struct test		*tp;


    swidth = sr->r_width;
    sheight = sr->r_height;

    popup_frame = window_create((Window)NULL, FRAME, FRAME_SHOW_LABEL, FALSE, 0);

    panel = window_create(popup_frame, PANEL, 0);

    list_item = panel_create_item(panel, PANEL_TOGGLE,
                        PANEL_LAYOUT, PANEL_VERTICAL,
                        PANEL_LABEL_STRING, message,
                        PANEL_LABEL_FONT, font,
                        PANEL_CHOICE_FONTS, font, 0,
                        PANEL_CHOICE_OFFSET, 25,
			PANEL_VALUE, get_test_selection(tl,
							LIST_DEFAULT),
                        0);
    
    for (tp = tl, i = 0; tp ; tp = tp->nexttest, i++) {
        (void)panel_set(list_item, PANEL_CHOICE_STRING, i, tp->testname, 0);
    }

    r = (Rect *) panel_get(list_item, PANEL_ITEM_RECT);
 
    pr = &ok_icon;
    width = pr->pr_width;
    /* center the  ok button under the message */
    left = (r->r_width - width) / 2;
    if (left < 0)
	left = 0;
    top = rect_bottom(r) + 15;

    (void)panel_create_item(panel, PANEL_BUTTON, 
			PANEL_ITEM_X, left, PANEL_ITEM_Y, top,
			PANEL_LABEL_IMAGE, pr,
			PANEL_NOTIFY_PROC, ok_proc,
			0);

    window_fit(panel);
    window_fit(popup_frame);

    /* center the confirmer frame on the screen */

    left = (swidth - r->r_width) / 2;
    top = (sheight - r->r_height) / 2;

    if (left < 0)
	left = 0;
    if (top < 0)
	top = 0;

    (void)window_set(popup_frame, WIN_X, left, WIN_Y, top, 0);

    return popup_frame;
}


/* ok notify proc */
/*ARGSUSED*/
/**********************************************************************/
static void
ok_proc(item, event)
/**********************************************************************/
Panel_item	item;
Event		*event;
{
    window_return(panel_get_value(list_item));
}
