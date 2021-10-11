
#ifndef lint
static char sccsid[] = "@(#)panel_util.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <sunwindow/rect.h>

arrange_items_veritcally(panel)
    Panel panel;
{
    Panel_item panel_item;
    int x, y;
    Rect *rect;
    int gap;

    panel_item = (Panel_item)window_get(panel, PANEL_FIRST_ITEM);
    gap = (int)window_get(panel, PANEL_ITEM_Y_GAP);
    if (panel_item) {
	rect = (Rect *)panel_get(panel_item, PANEL_ITEM_RECT);
	x = (int)panel_get(panel_item, PANEL_ITEM_X);
	y = (int)panel_get(panel_item, PANEL_ITEM_Y)  + rect->r_height + gap;
    }
    while (panel_item = panel_get(panel_item, PANEL_NEXT_ITEM)) {
	panel_set(panel_item, PANEL_ITEM_X, x, PANEL_ITEM_Y, y, 0);
	rect = (Rect *)panel_get(panel_item, PANEL_ITEM_RECT);
	y += rect->r_height + gap;
    }
}

put_panel_item_below(item1, item2)
    Panel_item item1, item2;
{
    int gap;
    gap = (int)panel_get( (Panel)panel_get(item1, PANEL_PARENT_PANEL),
			  PANEL_ITEM_Y_GAP);

    panel_set(item2, PANEL_ITEM_X, (int)panel_get(item1, PANEL_ITEM_X),
		     PANEL_ITEM_Y,
			(int)panel_get(item1, PANEL_ITEM_Y) +
			((Rect *)panel_get(item1, PANEL_ITEM_RECT))->r_height +
			gap,
		     0);
}

put_panel_item_to_right(item1, item2)
    Panel_item item1, item2;
{
    int gap;

    gap = (int)panel_get( (Panel)panel_get(item1, PANEL_PARENT_PANEL),
    PANEL_ITEM_X_GAP);
    panel_set(item2, PANEL_ITEM_Y, (int)panel_get(item1, PANEL_ITEM_Y),
		     PANEL_ITEM_X,
			(int)panel_get(item1, PANEL_ITEM_X) +
			((Rect *)panel_get(item1, PANEL_ITEM_RECT))->r_width +
			gap,
		     0);
}

span_items(item, n_items)
    Panel_item item;
    int n_items;
{
    int panel_width;
    Panel_item next_item;
    int i, x, total_width, gap;
    Rect *rect;
    
    panel_width = (int)window_get(panel_get(item, PANEL_PARENT_PANEL), WIN_WIDTH);

    total_width = 0;
    next_item = item;

    do {
	rect = (Rect *)panel_get(item, PANEL_ITEM_RECT);
	total_width += rect->r_width;
    } while ((i++ < n_items) &&
	(next_item = (Panel_item) panel_get(next_item, PANEL_NEXT_ITEM)) );

    if (total_width < panel_width) {
	gap = (panel_width - total_width)/(n_items);
	x = gap/2;
    } else {
	gap = (total_width)/(n_items);
	x = 0;
    }

    next_item = item;
    for(i=0; i < n_items; i++) {
	panel_set(next_item, PANEL_ITEM_X, x, 0);
	if (!(next_item = panel_get(next_item, PANEL_NEXT_ITEM))) {
	    break;
	}
	x += gap;
    }
    
}

item_below_x(item)
    Panel_item item;
{
    return((int)panel_get(item, PANEL_ITEM_X));
}

item_below_y(item)
    Panel_item item;
{
    return((int)panel_get(item, PANEL_ITEM_Y) +
	   (int)panel_get((Panel_item)panel_get(item, PANEL_PARENT_PANEL), PANEL_ITEM_Y_GAP) +
	   ((Rect *)panel_get(item, PANEL_ITEM_RECT))->r_height);
}

item_right_of_y(item)
{
    return((int)panel_get(item, PANEL_ITEM_Y));
}

item_right_of_x(item)
    Panel_item item;
{
    return((int)panel_get(item, PANEL_ITEM_X) +
	   (int)panel_get(panel_get((Panel_item)item, PANEL_PARENT_PANEL), PANEL_ITEM_X_GAP) +
	   ((Rect *)panel_get(item, PANEL_ITEM_RECT))->r_width);
}
