
#ifndef lint
static char sccsid[] = "@(#)slider.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

#include <suntool/sunview.h>
#include <suntool/panel.h>

static Rect *item_rect;
static int slider_locked;
static Panel_item locked_item;
static int win_fd;

Notify_value
slider_fixer(item, event)
    Panel_item item;
    Event *event;
{
    if (slider_locked) {
	if ((event_id(event) == LOC_DRAG) || (event_is_up(event)) ) {
	    if (event_x(event) < item_rect->r_left + 5) {
		event_x(event) = item_rect->r_left + 5;
	    } else if (event_x(event) > item_rect->r_left + item_rect->r_width - 5) {
		event_x(event) = item_rect->r_left + item_rect->r_width - 5;
	    }
	    if (event_y(event) < item_rect->r_top + 5) {
		event_y(event) = item_rect->r_top + 5;
	    } else if (event_y(event) > item_rect->r_top + item_rect->r_height - 5) {
		event_y(event) = item_rect->r_top + item_rect->r_height - 5;
	    }
	    panel_default_handle_event(locked_item, event);
	}
    } else {
	panel_default_handle_event(item, event);
    }
}

fix_slider(item, event, fd)
    Panel_item item;
    Event *event;
    int fd;
{
    if (event) {
	if (event_is_down(event)) {
	    if (!slider_locked) {
		win_fd = fd;
		win_grabio(win_fd);
		item_rect = (Rect *) panel_get(item, PANEL_ITEM_RECT);
		locked_item = item;
		slider_locked = 1;
	    }
	} else if (event_is_up(event)) {
	    if (slider_locked) {
		slider_locked = 0;
		win_releaseio(win_fd);
	    }
	}
    }
}
