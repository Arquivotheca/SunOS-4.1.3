
#ifndef lint
static char sccsid[] = "@(#)video_panel.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <video.h>

Panel_item video_freeze_item, video_out_item;

static struct pixrect *on_screen_button,
		      *video_out_button,
		      *freeze_button,
		      *inverted_freeze_button;

extern Window video_window;

Window
create_video_panel(base_frame)
    Window base_frame;
{
    static Window video_frame, video_panel;
    Panel_item item;
    extern int video_out_notify(), video_live_notify(),
	       zoom_button_notify();


    video_frame = window_create(base_frame, FRAME,
			        FRAME_SHOW_SHADOW, FALSE,
				0);
    video_panel = window_create(video_frame, PANEL,
				PANEL_LAYOUT, PANEL_HORIZONTAL,
				PANEL_ITEM_X_GAP, 20,
				PANEL_ITEM_Y_GAP, 20,
				0);
    item = panel_create_item(video_panel, PANEL_CHOICE,
		      PANEL_LABEL_IMAGE,
		      panel_button_image(video_panel, "-  Zoom  +", 5, 0),
		      PANEL_CHOICE_STRINGS,
		          "2X",
			  "1/1",
			  "1/2",
			  "1/4",
			  "1/8",
			  "1/16",
			  0,
		      PANEL_VALUE, 1,
		      PANEL_DISPLAY_LEVEL, PANEL_NONE,
		      PANEL_NOTIFY_PROC, zoom_button_notify,
		      0);
    register_item(item, "Zoom", PANEL_CHOICE);

   freeze_button = panel_button_image(video_panel, "  Freeze ", 5, 0);
   inverted_freeze_button = mem_create(freeze_button->pr_width,
		                       freeze_button->pr_height,
		                       freeze_button->pr_depth);
    pr_rop(inverted_freeze_button, 0, 0,
	    freeze_button->pr_width, freeze_button->pr_height,
	    PIX_NOT(PIX_SRC), freeze_button, 0, 0);
    video_freeze_item = panel_create_item(video_panel, PANEL_CHOICE,
		      PANEL_CHOICE_STRINGS, "Frozen", "Live", 0,
		      PANEL_LABEL_IMAGE, freeze_button,
		      PANEL_DISPLAY_LEVEL,  PANEL_NONE,
		      PANEL_VALUE, 1,
		      PANEL_NOTIFY_PROC, video_live_notify,
		      0);
    register_item(video_freeze_item, "Video_live", PANEL_CHOICE);
    on_screen_button =
        panel_button_image(video_panel, "On  Screen", 5, 0);
    video_out_button = 
	panel_button_image(video_panel, "Video  Out", 5, 0);
    video_out_item = panel_create_item(video_panel, PANEL_CHOICE,
		      PANEL_CHOICE_STRINGS,
			"On screen", "Video out", 0,
		      PANEL_LABEL_IMAGE,
			  panel_button_image(video_panel, "On  Screen", 5, 0),
		      PANEL_VALUE, 0,
		      PANEL_DISPLAY_LEVEL, PANEL_NONE,
		      PANEL_NOTIFY_PROC, video_out_notify,
		      0);
    register_item(video_out_item, "Output_select", PANEL_CHOICE);
/*    arrange_items_veritcally(video_panel);*/
    
    window_fit(video_panel);
    window_fit(video_frame);
    return(video_frame);
}

video_out_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    Rect *rect, rect_safe; 
    extern Window *video_blanket;

    window_set(video_window, VIDEO_OUTPUT, value+1, 0);
    if (value) { /* Video out ==> not on screen */
	panel_set(video_out_item, PANEL_LABEL_IMAGE, video_out_button, 0);
	if (window_get(video_window, WIN_SHOW)) {
	    rect = (Rect *)window_get(video_window, WIN_RECT);
	    rect_safe = *rect;
	    window_set(video_window, WIN_SHOW, FALSE, 0);
	    window_set(video_blanket, WIN_SHOW, TRUE, 
/*			WIN_WIDTH, rect_safe.r_width,
			WIN_HEIGHT, rect_safe.r_height,*/
			WIN_X, rect_safe.r_left,
			WIN_Y, rect_safe.r_top,
			0);
	}
    } else {
	panel_set(video_out_item, PANEL_LABEL_IMAGE, on_screen_button, 0);
	if (window_get(video_blanket, WIN_SHOW)) {
	    rect = (Rect *)window_get(video_blanket, WIN_RECT);
	    rect_safe = *rect;
	    window_set(video_blanket, WIN_SHOW, FALSE, 0);
	    window_set(video_window, WIN_SHOW, TRUE, 
/*			WIN_WIDTH, rect_safe.r_width,
			WIN_HEIGHT, rect_safe.r_height,*/
			WIN_X, rect_safe.r_left,
			WIN_Y, rect_safe.r_top,
			0);
	}
    }
}

static int current_zoom=1;

zoom_button_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    Rect *item_rect;
    int center;
    extern Window video_frame;
    int w, h, shift;

    static int y_offset[] ={
	29, 29, 15, 7, 4, 2
    };


    item_rect = (Rect *)panel_get(item, PANEL_ITEM_RECT);
    center = (int)panel_get(item, PANEL_ITEM_X) + (item_rect->r_width/2);

    if (event && ((event_id(event) == BUT_FIRST))) { /* first mouse button */
	if (event_x(event) < center) { /* zoom out */
	    if (current_zoom < 5) {
		value = current_zoom + 1;
	    } else {
		value = 5;
	    }
	} else {
	    if (current_zoom > 0) {
		value = current_zoom - 1;
	    } else {
		value = 0;
	    }
	}
	panel_set(item, PANEL_VALUE, value, 0);
    }
    /* now it is a normal choice */
    window_set(video_window, VIDEO_COMPRESSION, value, 0);
    w = (int)window_get(video_window, WIN_WIDTH);
    h = (int)window_get(video_window, WIN_HEIGHT);
    
    shift = (current_zoom - value);
    if (shift > 0) {
	w = w << shift;
	h = h << shift;
    } else {
	w = w >> (-shift);
	h = h >> (-shift);
    }
    window_set(video_window,
	       VIDEO_Y_OFFSET, y_offset[value],
	       WIN_WIDTH, w,
	       WIN_HEIGHT, h,
	       0);
    window_fit(video_frame);
    current_zoom = value;
}

video_zoom_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    int w, h, shift;
    extern Window video_window, video_frame;
    static int y_offset[] ={
	58, 29, 15, 7, 4, 2
    };


    window_set(video_window, VIDEO_COMPRESSION, value, 0);
    w = (int)window_get(video_window, WIN_WIDTH);
    h = (int)window_get(video_window, WIN_HEIGHT);
    
    shift = (current_zoom - value);
    if (shift > 0) {
	w = w << shift;
	h = h << shift;
    } else {
	w = w >> (-shift);
	h = h >> (-shift);
    }
    w = w + 10; /* two margins */
    h = h + 16 + 10 + (int)window_get(video_frame, WIN_TOP_MARGIN);
    window_set(video_frame, WIN_WIDTH, w, WIN_HEIGHT, h, 0);
    window_set(video_window, VIDEO_Y_OFFSET, y_offset[value], 0);
    current_zoom = value;
}

video_live_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    extern struct pixrect *my_video_pixrect;

    switch (value) {
	case 0:	/* Freeze */
	    window_set(video_window, VIDEO_LIVE, value, 0);
	    panel_set(item, PANEL_LABEL_IMAGE, inverted_freeze_button,
	    PANEL_CHOICE_STRINGS, "Frozen", "Live", "Remove Interlace", 0,
	    0);
	    break;
	case 1: /* Live */
	    window_set(video_window, VIDEO_LIVE, value, 0);
	    panel_set(item, PANEL_LABEL_IMAGE, freeze_button,
		  PANEL_CHOICE_STRINGS, "Frozen", "Live", 0, 0);
	    break;
	case 2:	/* De-interlace */
	    de_interlace(my_video_pixrect);
	    panel_set(item, PANEL_VALUE, 0, 0);
	    break;
    }
}

