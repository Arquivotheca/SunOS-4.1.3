#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)canvas_demo.c 1.1 92/07/30";
#endif
#endif

/*
 * Canvas_demo -- demonstrate the Canvas Subwindow Package.
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#include <sys/types.h>
#include <sunwindow/attr.h>
#include <sunwindow/defaults.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <suntool/walkmenu.h>
#include <suntool/scrollbar.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

static Frame		base_frame;
static Frame 		props /* = 0 */;
static Canvas		canvas;
static Scrollbar	vertical_sb, horizontal_sb;
static Menu		menu;

static int		object_width, object_height;

static Panel_item	canvas_width, canvas_height, canvas_margin;
static Panel_item	canvas_vertical_sb, canvas_horizontal_sb;
static Panel_item	canvas_retained, canvas_auto_clear;
static Panel_item	canvas_auto_expand, canvas_auto_shrink;

static Panel_item	object_width_item, object_height_item;

static short icon_data[] = {
#include <images/canvas_demo.icon>
};
static mpr_static(canvas_demo_pr, 64, 64, 1, icon_data);


/* panel notify procs */
static void	modify_canvas();
static void	clear_canvas();
static void	draw_canvas();
static void	show_props();
static void	quit();

/* canvs procs */
static void	resize_canvas();
static void	repaint_canvas();
static void	handle_event();

/* utilities */
static void	init_panel();
static void	draw_something();
static void	draw_line();
char		*sprintf();

/* property sheet procs */
static void	create_props();
static void	fixed_image();
static void	modify_object();
static void	hide_props();


#ifdef STANDALONE
main(argc, argv)
#else
int canvas_demo_main(argc, argv)
#endif
int argc;
char **argv;
{
    static  Notify_value my_destroy_func();

    base_frame =
	window_create((Window)0, FRAME,
	    FRAME_LABEL,    	"CANVAS DEMO",
	    FRAME_ICON,	    	icon_create(ICON_IMAGE, &canvas_demo_pr, 0),
	    FRAME_NO_CONFIRM,	TRUE,
	    FRAME_ARGS,     	argc, argv,
	    0);

    (void) notify_interpose_destroy_func(base_frame, my_destroy_func);
    
    init_panel(base_frame);
    
    vertical_sb = scrollbar_create(SCROLL_LINE_HEIGHT, 5, (char *)0);
    horizontal_sb = scrollbar_create(SCROLL_LINE_HEIGHT, 5, (char *)0);

    menu =
        menu_create(
            MENU_ACTION_ITEM, "Clear", clear_canvas,
            MENU_ACTION_ITEM, "Draw",  draw_canvas,
            MENU_ACTION_ITEM, "Props", show_props,
            MENU_ACTION_ITEM, "Quit",  quit,
            0);
             
    canvas = 
        window_create(base_frame, CANVAS, 
	    WIN_VERTICAL_SCROLLBAR,	vertical_sb,
	    WIN_HORIZONTAL_SCROLLBAR,	horizontal_sb,
	    WIN_CONSUME_PICK_EVENT,	LOC_DRAG,
	    WIN_EVENT_PROC, 		handle_event,
	    CANVAS_WIDTH,		750,
	    CANVAS_HEIGHT,		750,
	    CANVAS_AUTO_SHRINK,		FALSE,
	    CANVAS_FIXED_IMAGE,		FALSE,
	    CANVAS_RESIZE_PROC,		resize_canvas,
	    0),

    object_width	= (int) window_get(canvas, CANVAS_WIDTH) - 20;
    object_height	= (int) window_get(canvas, CANVAS_HEIGHT) - 20;

    draw_something(canvas);

    window_main_loop(base_frame);

    EXIT(0);
}


static void
init_panel(base_frame_local)
Frame	base_frame_local;
{
    Panel	panel;

    panel = window_create(base_frame_local, PANEL, PANEL_LABEL_BOLD, TRUE, 0);

    canvas_width = 
        panel_create_item(panel, PANEL_SLIDER,
            PANEL_LABEL_STRING, "Width ",
            PANEL_SLIDER_WIDTH, 200,
	    PANEL_MIN_VALUE, 0,
	    PANEL_MAX_VALUE, 800,
	    PANEL_NOTIFY_PROC, modify_canvas,
	    0);

    canvas_height = 
        panel_create_item(panel, PANEL_SLIDER,
	    PANEL_LABEL_STRING, "Height",
	    PANEL_SLIDER_WIDTH, 200,
	    PANEL_MIN_VALUE, 0,
	    PANEL_MAX_VALUE, 800,
	    PANEL_NOTIFY_PROC, modify_canvas,
	    0);

    canvas_margin = 
        panel_create_item(panel, PANEL_SLIDER,
	    PANEL_LABEL_STRING, "Margin",
	    PANEL_SLIDER_WIDTH, 200,
	    PANEL_MIN_VALUE, 0,
	    PANEL_MAX_VALUE, 100,
	    PANEL_NOTIFY_PROC, modify_canvas,
	    0);

    canvas_vertical_sb = 
	panel_create_item(panel, PANEL_CYCLE,
	    PANEL_ITEM_X, ATTR_COL(0),
	    PANEL_ITEM_Y, ATTR_ROW(4),
	    PANEL_LABEL_STRING, "Vertical Scrollbar:   ",
	    PANEL_CHOICE_STRINGS, "NO", "YES", 0,
	    PANEL_VALUE, 1,
	    PANEL_NOTIFY_PROC, modify_canvas,
	    0);

    canvas_horizontal_sb = 
	panel_create_item(panel, PANEL_CYCLE,
	    PANEL_ITEM_X, ATTR_COL(0),
	    PANEL_ITEM_Y, ATTR_ROW(5),
	    PANEL_LABEL_STRING, "Horizontal Scrollbar: ",
	    PANEL_CHOICE_STRINGS, "NO", "YES", 0,
	    PANEL_VALUE, 1,
	    PANEL_NOTIFY_PROC, modify_canvas,
	    0);

    canvas_retained = 
	panel_create_item(panel, PANEL_CYCLE,
	    PANEL_ITEM_X, ATTR_COL(33),
	    PANEL_ITEM_Y, ATTR_ROW(4),
	    PANEL_LABEL_STRING, "Retained:   ",
	    PANEL_CHOICE_STRINGS, "NO", "YES", 0,
	    PANEL_VALUE, 1,
	    PANEL_NOTIFY_PROC, modify_canvas,
	    0);

    canvas_auto_clear = 
	panel_create_item(panel, PANEL_CYCLE,
	    PANEL_ITEM_X, ATTR_COL(33),
	    PANEL_ITEM_Y, ATTR_ROW(5),
	    PANEL_LABEL_STRING, "Auto Clear: ",
	    PANEL_CHOICE_STRINGS, "NO", "YES", 0,
	    PANEL_VALUE, 1,
	    PANEL_NOTIFY_PROC, modify_canvas,
	    0);

    canvas_auto_expand = 
	panel_create_item(panel, PANEL_CYCLE,
		PANEL_ITEM_X, ATTR_COL(57),
		PANEL_ITEM_Y, ATTR_ROW(4),
		PANEL_LABEL_STRING, "Auto Expand: ",
		PANEL_CHOICE_STRINGS, "NO", "YES", 0,
		PANEL_VALUE, 1,
		PANEL_NOTIFY_PROC, modify_canvas,
		0);

    canvas_auto_shrink = 
	panel_create_item(panel, PANEL_CYCLE,
	    PANEL_ITEM_X, ATTR_COL(57),
	    PANEL_ITEM_Y, ATTR_ROW(5),
	    PANEL_LABEL_STRING, "Auto Shrink: ",
	    PANEL_CHOICE_STRINGS, "NO", "YES", 0,
	    PANEL_VALUE, 0,
	    PANEL_NOTIFY_PROC, modify_canvas,
	    0);

    (void)panel_create_item(panel, PANEL_BUTTON, 
	    PANEL_ITEM_X, ATTR_COL(58),
	    PANEL_ITEM_Y, ATTR_ROW(0) + 5,
	    PANEL_LABEL_IMAGE, panel_button_image(panel, "CLEAR", 5, (Pixfont *)0),
	    PANEL_NOTIFY_PROC, clear_canvas,
	    0);

    (void)panel_create_item(panel, PANEL_BUTTON, 
	    PANEL_ITEM_X, ATTR_COL(68),
	    PANEL_ITEM_Y, ATTR_ROW(0) + 5,
	    PANEL_LABEL_IMAGE, panel_button_image(panel, "DRAW", 5, (Pixfont *)0),
	    PANEL_NOTIFY_PROC, draw_canvas,
	    0);

    (void)panel_create_item(panel, PANEL_BUTTON, 
	PANEL_ITEM_X, ATTR_COL(58),
	PANEL_ITEM_Y, ATTR_ROW(1) + 10,
	PANEL_LABEL_IMAGE, panel_button_image(panel, "PROPS", 5, (Pixfont *)0),
	PANEL_NOTIFY_PROC, show_props,
	0);

    (void)panel_create_item(panel, PANEL_BUTTON, 
	PANEL_ITEM_X, ATTR_COL(68),
	PANEL_ITEM_Y, ATTR_ROW(1) + 10,
	PANEL_LABEL_IMAGE, panel_button_image(panel, "QUIT", 5, (Pixfont *)0),
	PANEL_NOTIFY_PROC, quit,
	0);

    (void)window_fit_height(panel);
}


/* panel notify procs */

static void
modify_canvas(item, event)
Panel_item	item;
Event		*event;
{
    (void)window_set(canvas, 
	CANVAS_WIDTH,		panel_get_value(canvas_width),
	CANVAS_HEIGHT,		panel_get_value(canvas_height),
	CANVAS_MARGIN,		panel_get_value(canvas_margin),
	CANVAS_AUTO_CLEAR,	panel_get_value(canvas_auto_clear),
	CANVAS_AUTO_EXPAND,	panel_get_value(canvas_auto_expand),
	CANVAS_AUTO_SHRINK,	panel_get_value(canvas_auto_shrink),
	0);

    if (panel_get_value(canvas_retained) != 
	window_get(canvas, CANVAS_RETAINED)) {
	    (void)window_set(canvas, 
		CANVAS_RETAINED, panel_get_value(canvas_retained),
		0);
	if (window_get(canvas, CANVAS_RETAINED))
	    (void)window_set(canvas, CANVAS_REPAINT_PROC, 0, 0);
	else
	    (void)window_set(canvas, CANVAS_REPAINT_PROC, repaint_canvas, 0);
	clear_canvas(item, event);
	draw_something(canvas);
    }

    if ((int) panel_get_value(canvas_vertical_sb)) {
	if (!window_get(canvas, WIN_VERTICAL_SCROLLBAR))
	    (void)window_set(canvas, WIN_VERTICAL_SCROLLBAR, vertical_sb, 0);
    } else
	(void)window_set(canvas, WIN_VERTICAL_SCROLLBAR, 0, 0);

    if ((int) panel_get_value(canvas_horizontal_sb)) {
	if (!window_get(canvas, WIN_HORIZONTAL_SCROLLBAR))
	    (void)window_set(canvas, WIN_HORIZONTAL_SCROLLBAR, horizontal_sb, 0);
    } else
	(void)window_set(canvas, WIN_HORIZONTAL_SCROLLBAR, 0, 0);

    /* make sure we have the latest width and height */

    if (window_get(canvas, CANVAS_WIDTH) != panel_get_value(canvas_width))
	(void)panel_set_value(canvas_width, window_get(canvas, CANVAS_WIDTH));

    if (window_get(canvas, CANVAS_HEIGHT) != panel_get_value(canvas_height))
	(void)panel_set_value(canvas_height, window_get(canvas, CANVAS_HEIGHT));
}


/* ARGSUSED */
static void
clear_canvas(item, event)
Panel_item	item;
Event		*event;
{
    Pixwin	*pw	= canvas_pixwin(canvas);

    (void)pw_writebackground(pw,0,0,(int)(LINT_CAST(window_get(canvas, CANVAS_WIDTH))),
	(int)(LINT_CAST(window_get(canvas, CANVAS_HEIGHT))), PIX_CLR);
}


/* ARGSUSED */
static void
draw_canvas(item, event)
Panel_item	item;
Event		*event;
{
    draw_something(canvas);
}


/* ARGSUSED */
static void
show_props(item, event)
Panel_item	item;
Event		*event;
{
    if (!props)
	create_props();
    (void)panel_set_value(object_width_item, object_width);
    (void)panel_set_value(object_height_item, object_height);
    (void)window_set(props, WIN_SHOW, TRUE, 0);
}

/* ARGSUSED */
static void
quit(item, event)
Panel_item	item;
Event		*event;
{
    /* quit without user confirmation */
    (void)window_set(base_frame, FRAME_NO_CONFIRM, TRUE, 0);
    (void)window_destroy(base_frame);
}


/* canvas procs */

static void
resize_canvas(canvas_local, width, height)
Canvas	canvas_local;
int	width, height;
{
    (void)panel_set_value(canvas_width, width);
    (void)panel_set_value(canvas_height, height);

    if (!window_get(canvas_local, CANVAS_FIXED_IMAGE)) {
	object_width = width - 20;
	object_height = height - 20;
    }
}


/* ARGSUSED */
static void
repaint_canvas(canvas_local, pw, rect_list)
Canvas		canvas_local;
Pixwin		*pw;
Rectlist	*rect_list;
{
    draw_something(canvas_local);
}


/* ARGSUSED */
static void
handle_event(canvas_local, event, arg)
Canvas	canvas_local;
Event	*event;
caddr_t	arg;
{
    Pixwin	*pw	= canvas_pixwin(canvas_local);
    short	draw	= FALSE;
    short	erase	= FALSE;
    
    if (event_is_up(event))
        return;

    switch (event_action(event)) {
        case MS_LEFT:
            draw = TRUE;
            break;
            
        case MS_MIDDLE:
	    erase = TRUE;
	    break;
	    
        case LOC_DRAG:
            if (window_get(canvas_local, WIN_EVENT_STATE, MS_LEFT))
                draw = TRUE;
            else if (window_get(canvas_local, WIN_EVENT_STATE, MS_MIDDLE))
                erase = TRUE;
	    break;

	case MS_RIGHT:
	    /* translate the event to window space,
	     * then show the menu.
	     */
	    (void) menu_show(menu, canvas_local, 
	               canvas_window_event(canvas_local, event), 0);	
	    break;
	
	default:
	    break;
    }
    
    if (draw)
        (void)pw_put(pw, event_x(event), event_y(event), 1);
    if (erase)
        (void)pw_put(pw, event_x(event), event_y(event), 0);
}
	        

/* utilities */

static void
draw_something(canvas_local)
Canvas	canvas_local;
{
    Pixwin	*pw	= canvas_pixwin(canvas_local);

    draw_line(pw, 10, 10, object_width, 10);
    draw_line(pw, object_width, 10, object_width, object_height);
    draw_line(pw, object_width, object_height, 10, object_height);
    draw_line(pw, 10, object_height, 10, 10);

    draw_line(pw, 10, 10, object_width, object_height);
    draw_line(pw, 10, object_height, object_width, 10);
}

static void
draw_line(pw, x1, y1, x2, y2)
Pixwin	*pw;
int	x1, y1, x2, y2;
{

    char	buffer[32];

    (void)sprintf(buffer, "(%d, %d)", x1, y1);
    (void)pw_text(pw, x1, y1, PIX_SRC, (struct pixfont *)0, buffer);
    (void)pw_vector(pw, x1, y1, x2, y2, PIX_SRC, 1);
}


/* property sheet procs */

static void
create_props()
{
    Panel	panel;

    props = window_create(base_frame, FRAME, 0);
    panel = window_create(props, PANEL, 0);

    (void)panel_create_item(panel, PANEL_CYCLE,
	PANEL_ITEM_X, ATTR_COL(1),
	PANEL_ITEM_Y, ATTR_ROW(0),
	PANEL_LABEL_STRING, "Fixed Image: ",
	PANEL_CHOICE_STRINGS, "NO", "YES", 0,
	PANEL_VALUE, 0,
	PANEL_NOTIFY_PROC, fixed_image,
	0);

    (void)panel_create_item(panel, PANEL_BUTTON, 
	PANEL_ITEM_X, ATTR_COL(30),
	PANEL_ITEM_Y, ATTR_ROW(0),
	PANEL_LABEL_IMAGE, panel_button_image(panel, "DONE", 0, (Pixfont *)0),
	PANEL_NOTIFY_PROC, hide_props,
	0);

    object_width_item = 
	panel_create_item(panel, PANEL_SLIDER,
	    PANEL_LABEL_STRING, "Width ",
	    PANEL_SLIDER_WIDTH, 200,
	    PANEL_MIN_VALUE, 0,
	    PANEL_MAX_VALUE, 800,
	    PANEL_CLIENT_DATA, 0,
	    PANEL_SHOW_ITEM, FALSE,
	    PANEL_NOTIFY_PROC, modify_object,
	    0);

    object_height_item = 
	panel_create_item(panel, PANEL_SLIDER,
	    PANEL_LABEL_STRING, "Height",
	    PANEL_SLIDER_WIDTH, 200,
	    PANEL_MIN_VALUE, 0,
	    PANEL_MAX_VALUE, 800,
	    PANEL_CLIENT_DATA, 1,
	    PANEL_SHOW_ITEM, FALSE,
	    PANEL_NOTIFY_PROC, modify_object,
	    0);
    
    (void)window_fit(panel);
    (void)window_fit(props);
}


/* ARGSUSED */
static void
fixed_image(item, is_fixed, event)
Panel_item	item;
int		is_fixed;
Event		*event;
{
    (void)window_set(canvas, CANVAS_FIXED_IMAGE, is_fixed, 0);
    (void)panel_set(object_width_item, PANEL_SHOW_ITEM, is_fixed, 0);
    (void)panel_set(object_height_item, PANEL_SHOW_ITEM, is_fixed, 0);
}


static void
modify_object(item, value, event)
Panel_item	item;
int		value;
Event		*event;
{
    if (panel_get(item, PANEL_CLIENT_DATA))
	object_height = value;
    else
	object_width = value;
    clear_canvas(item, event);
    draw_something(canvas);
}

/* ARGSUSED */
static void
hide_props(item, event)
Panel_item	item;
Event		*event;
{
    (void)window_set(props, WIN_SHOW, FALSE, 0);
}

static Notify_value
my_destroy_func(client, status)
    Notify_client	client;
    Destroy_status	status;
{
    int		windowfd = (int)window_get(
    		    (Window)(LINT_CAST(client)), WIN_FD);

    if (status == DESTROY_CHECKING) {
	int	result;
	Event	event;

	result = alert_prompt(
		(Frame)client,
		&event,
		ALERT_MESSAGE_STRINGS,
		    "Are you sure you want to Quit?",
	            0,
		ALERT_BUTTON_YES,	"Confirm",
		ALERT_BUTTON_NO,	"Cancel",
		ALERT_OPTIONAL,		1,
		ALERT_NO_BEEPING,	1,
		0);
	if (result == ALERT_YES) {
	    return(notify_next_destroy_func(client, status));
	} else {
	    (void) notify_veto_destroy(
	    	(Notify_client)(LINT_CAST(client)));
	    return(NOTIFY_DONE);
	}
    }
    return(notify_next_destroy_func(client, status));
}
