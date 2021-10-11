#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)canvas.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <suntool/canvas_impl.h>
#include <sunwindow/win_keymap.h>

static int		canvas_set();
static caddr_t		canvas_get();

static Notify_value	canvas_handle_event();
static Notify_value	canvas_destroy();

static void		canvas_layout();

static void		compute_damage();
static void		clear_rectlist();

/* ARGSUSED */
caddr_t
canvas_window_object(win, avlist)
register Window		win;
Attr_avlist		avlist;
{
    register Pixwin	*pw = (Pixwin *) (LINT_CAST(window_get(win, WIN_PIXWIN)));
    register Canvas_info	*canvas;
    struct colormapseg cms;
    char *calloc();
    struct inputmask  mask;
    int  		fd;

    canvas = (Canvas_info *) (LINT_CAST(calloc(1, sizeof(Canvas_info))));
    if (!canvas)
	return NULL;

    status_set(canvas, retained);
    status_set(canvas, fixed_image);
    status_set(canvas, auto_clear);
    status_set(canvas, auto_expand);
    status_set(canvas, auto_shrink);

    /* Conserve memory on depth > 1 plane pixrects if only need bit map. */
    (void)pw_getcmsdata(pw, &cms, (int *)0);
    canvas->depth = (cms.cms_size==2 && pw->pw_pixrect->pr_depth<32)
	? 1 : pw->pw_pixrect->pr_depth;

    canvas->win_rect    = *(Rect *)(LINT_CAST(window_get(win, WIN_RECT)));
    fd = (int)window_get(win, WIN_FD);

    canvas_set_left(canvas, 0);
    canvas_set_top(canvas, 0);
    canvas_set_width(canvas, win_width(canvas));
    canvas_set_height(canvas, win_height(canvas));

    canvas->help_data = "sunview:canvas";

    canvas->pw = 
	pw_region(pw, 0, 0, canvas_width(canvas), canvas_height(canvas));
    if (!canvas->pw)
	return NULL;

    canvas->pw->pw_prretained = 
	mem_create(canvas_width(canvas), canvas_height(canvas), canvas->depth);
    if (!canvas->pw->pw_prretained)
	return NULL;

    (void)window_set(win,
	    WIN_NOTIFY_INFO,		PW_INPUT_DEFAULT,
	    WIN_OBJECT,			canvas, 
	    WIN_SET_PROC,		canvas_set,
	    WIN_GET_PROC,		canvas_get,
	    WIN_LAYOUT_PROC,		canvas_layout,
	    WIN_NOTIFY_EVENT_PROC,	canvas_handle_event,
	    WIN_NOTIFY_DESTROY_PROC,	canvas_destroy,
	    WIN_TYPE,			CANVAS_TYPE,
	    WIN_CONSUME_KBD_EVENTS, 	KBD_USE, KBD_DONE, 0,
    /* 
     * BUG: This is a workaround.  Canvas shouldn't event have to set the
     * 	    pick mask for the following three buttons.  In the case of
     *	    click-to-type, and when canvas has the cursor, but not the
     *	    input focus, events generated from these three buttons 
     *	    should simply drop through to the frame of the canvas.
     *	    There appears to be a bug in the event delivery algorithm
     *	    that is not consistent w/ the documentation.
     */
           WIN_CONSUME_PICK_EVENTS,     
		ACTION_PROPS, ACTION_OPEN, ACTION_FRONT,
                ACTION_BACK, ACTION_CLOSE,
		ACTION_HELP,
	        WIN_MOUSE_BUTTONS, LOC_WINENTER, LOC_WINEXIT, 
	        LOC_MOVE, KBD_REQUEST, 0,
	    0);
    return (caddr_t)canvas;
}


/* update the canvas when the window is
 * resized from window_set().
 */
/* ARGSUSED */
static void
canvas_layout(owner, canvas, op)
Window			owner;
register Canvas_info	*canvas;
Window_layout_op	op;
{

    switch (op) {
	case WIN_ADJUSTED: {
	    register Rect *new_win_rect = (Rect *) (LINT_CAST(
	    	window_get((Window)(LINT_CAST(canvas)), WIN_RECT)));

	    if (new_win_rect->r_width != win_width(canvas) ||
		new_win_rect->r_height != win_height(canvas)) {

		canvas->win_rect = *new_win_rect;
		(void)canvas_resize_pixwin(canvas, TRUE);
		canvas_update_scrollbars(canvas, TRUE);
	    }
	    break;
	}

	default:
	    break;
    }
}


static int
canvas_set(canvas, avlist)
register Canvas_info	*canvas;
Attr_avlist		avlist;
{
    register Canvas_attribute	attr;
    register int		width = 0, height = 0;
    int				margin;
    short			new_pw_size = FALSE, new_canvas_size = FALSE;
    short			new_v_sb = FALSE, new_h_sb = FALSE;
    int				repaint=FALSE, show_updates;
    Scrollbar			v_sb, h_sb;
    int				ok = TRUE;
    void			pw_batch();

    for (attr = (Canvas_attribute) avlist[0]; attr;
	 avlist = attr_next(avlist), attr = (Canvas_attribute) avlist[0]) {
	switch (attr) {
	    case CANVAS_LEFT:
		break;

	    case CANVAS_TOP:
		break;

	    case CANVAS_WIDTH:
		width = (int) avlist[1];
		if (width == canvas_width(canvas))
		    width = 0;
		else
		    new_canvas_size = TRUE;
		break;

	    case CANVAS_HEIGHT:
		height = (int) avlist[1];
		if (height == canvas_height(canvas))
		    height = 0;
		else
		    new_canvas_size = TRUE;
		break;

	    case CANVAS_DEPTH:
		/*
		depth = (int) avlist[1];
		if (depth <= 0)
		    return FALSE;
		*/
		break;

	    case CANVAS_MARGIN:
		margin = (int) avlist[1];
		if (margin < 0)
		   ok = FALSE;
		else if (margin != canvas->margin) {
		    /* resize the view pixwin for the new margin */
		    canvas->margin = margin;
		    new_pw_size = TRUE;
		}
		break;

	    case CANVAS_REPAINT_PROC:
		canvas->repaint_proc = (Function)(LINT_CAST(avlist[1]));
		break;

	    case CANVAS_RESIZE_PROC:
		canvas->resize_proc = (Function)(LINT_CAST(avlist[1]));
		break;

	    case CANVAS_AUTO_CLEAR:
		if ((int) avlist[1])
		    status_set(canvas, auto_clear);
		else
		    status_reset(canvas, auto_clear);
		break;

	    case CANVAS_AUTO_EXPAND:
		if ((int) avlist[1] == status(canvas, auto_expand)) 
		    break;
		if (avlist[1])
		    status_set(canvas, auto_expand);
		else
		    status_reset(canvas, auto_expand);
		new_pw_size = TRUE;
		break;

	    case CANVAS_AUTO_SHRINK:
		if ((int) avlist[1] == status(canvas, auto_shrink)) 
		    break;
		if (avlist[1])
		    status_set(canvas, auto_shrink);
		else
		    status_reset(canvas, auto_shrink);
		new_pw_size = TRUE;
		break;

	    case CANVAS_FAST_MONO:
		if (avlist[1]) {
		    Pixwin	*pw = (Pixwin *) (LINT_CAST(window_get(
		    	(Window)(LINT_CAST(canvas)), WIN_PIXWIN)));

		    if (pw) {
			struct colormapseg cms;
			int original_pr_depth = pw->pw_pixrect->pr_depth;
			void pw_use_fast_monochrome();

			pw_use_fast_monochrome(pw);
			if (original_pr_depth != pw->pw_pixrect->pr_depth) {
				pw_use_fast_monochrome(canvas->pw);
				new_v_sb = (short)(canvas_sb(canvas,
				    SCROLL_VERTICAL));
				new_h_sb = (short)(canvas_sb(canvas,
				    SCROLL_HORIZONTAL));
				(void)pw_getcmsdata(pw, &cms, (int *)0);
				canvas->depth = ((cms.cms_size == 2 &&
				    pw->pw_pixrect->pr_depth<32)
				    ? 1 : pw->pw_pixrect->pr_depth);
			}
		    }
		}
		break;

#ifndef PRE_IBIS
	    case CANVAS_COLOR24:
		if (avlist[1]) {
		    Pixwin	*pw = (Pixwin *) (LINT_CAST(window_get(
		    	(Window)(LINT_CAST(canvas)), WIN_PIXWIN)));

		    if (pw) {
			struct colormapseg cms;
			int original_pr_depth = pw->pw_pixrect->pr_depth;
			void pw_use_color24();

			pw_use_color24(pw);
			/*if (original_pr_depth != pw->pw_pixrect->pr_depth) {*/
				pw_use_color24(canvas->pw);
				new_v_sb = (short)(canvas_sb(canvas,
				    SCROLL_VERTICAL));
				new_h_sb = (short)(canvas_sb(canvas,
				    SCROLL_HORIZONTAL));
				(void)pw_getcmsdata(pw, &cms, (int *)0);
				canvas->depth = ((cms.cms_size == 2 &&
				    pw->pw_pixrect->pr_depth<32)
				    ? 1 : pw->pw_pixrect->pr_depth);
			/*}*/
		    }
		}
		break;
#endif ndef PRE_IBIS

	    case CANVAS_RETAINED:
		if ((int) avlist[1] == status(canvas, retained))
		    break;
		if (avlist[1]) {
		    Pixwin	*pw = (Pixwin *) (LINT_CAST(window_get(
		    	(Window)(LINT_CAST(canvas)), WIN_PIXWIN)));
		    struct colormapseg cms;

		    /* Conserve memory on depth > 1 plane pixrects 
		     * if only need bit map.
		     */
		    (void)pw_getcmsdata(pw, &cms, (int *)0);
		    canvas->depth = (cms.cms_size == 2 &&
			pw->pw_pixrect->pr_depth<32)
			? 1 : pw->pw_pixrect->pr_depth;

		    status_set(canvas, retained);
		    canvas_set_pr(canvas, mem_create(canvas_width(canvas),
			canvas_height(canvas), canvas->depth));
		    if (!canvas_pr(canvas))
		       ok = FALSE;
		} 
		else {
		    status_reset(canvas, retained);
		    pw_batch(canvas->pw, PW_NONE);
		    if (canvas_pr(canvas)) {
			(void)pr_destroy(canvas_pr(canvas));
			canvas_set_pr(canvas, 0);
		    }
		}
		break;

	    case CANVAS_FIXED_IMAGE:
		if (avlist[1])
		    status_set(canvas, fixed_image);
		else
		    status_reset(canvas, fixed_image);
		break;

	    case WIN_VERTICAL_SCROLLBAR:
		v_sb = (Scrollbar) avlist[1];
		/* ignore zero scrollbar if already zero */
		new_v_sb = (v_sb || canvas_sb(canvas, SCROLL_VERTICAL));
		break;

	    case WIN_HORIZONTAL_SCROLLBAR:
		h_sb = (Scrollbar) avlist[1];
		/* ignore zero scrollbar if already zero */
		new_h_sb = (h_sb || canvas_sb(canvas, SCROLL_HORIZONTAL));
		break;

  	    case HELP_DATA:
  		canvas->help_data = (caddr_t) avlist[1];
		break;

	    default:
		/*
		return attr_check(attr, ATTR_PKG_CANVAS, "window_set()");
		*/
		break;
	}
    }
    if (new_pw_size)
	ok |= canvas_resize_pixwin(canvas, FALSE);

    if (new_canvas_size) {
	/* if auto expand is on, always expand
	 * the canvas to at least the edges of
	 * the viewing pixwin.
	 */
	if (status(canvas, auto_expand)) {
	    if (width)
		width = max(width, view_width(canvas));
	    if (height)
		height = max(height, view_height(canvas));
	}
	/* if auto shrink is on, always shrink
	 * the canvas to the edges of the viewing pixwin.
	 */
	if (status(canvas, auto_shrink)) {
	    width = min(width, view_width(canvas));
	    height = min(height, view_height(canvas));
	}
	ok |= canvas_resize_canvas(canvas, width, height, FALSE);
    }

    if (new_v_sb)
	canvas_setup_scrollbar(canvas, SCROLL_VERTICAL, v_sb);
    if (new_h_sb)
	canvas_setup_scrollbar(canvas, SCROLL_HORIZONTAL, h_sb);

    show_updates = (int) window_get((Window)(LINT_CAST(canvas)),
    			WIN_SHOW_UPDATES);

    /* If new h_sb or v_sb and client allows updates, ask
     * for repaint. If updates are not allowed set the dirty bit.
     */
    if (new_v_sb || new_h_sb) 
       if (show_updates) 
	   repaint = TRUE;
       else 
	   status_set(canvas, dirty);

    if (new_pw_size || new_canvas_size || new_v_sb || new_h_sb)
	canvas_update_scrollbars(canvas, !repaint && show_updates);

    if (repaint) {
	/* the canvas has changed and the client
	 * allows the updates to be shown, so repaint.
	 * Make sure we can repaint all of the dirty areas.
	 */
	(void)pw_exposed(canvas->pw);
	canvas_repaint_window(canvas);
    }

    return ok;
}


/* ARGSUSED */
static caddr_t
canvas_get(canvas, attr, arg1, arg2, arg3)
register Canvas_info	*canvas;
Canvas_attribute	attr;
{

    switch (attr) {
	case CANVAS_PIXWIN:
	    return (caddr_t) canvas->pw;

	case CANVAS_FAST_MONO:
	    return (caddr_t) (canvas->pw->pw_pixrect->pr_depth == 1);

	case CANVAS_WIDTH:
	    return (caddr_t) canvas_width(canvas);

	case CANVAS_HEIGHT:
	    return (caddr_t) canvas_height(canvas);

	case CANVAS_LEFT:
	    return (caddr_t) canvas_left(canvas);

	case CANVAS_TOP:
	    return (caddr_t) canvas_top(canvas);

	case CANVAS_DEPTH:
	    return (caddr_t) canvas->depth;

	case CANVAS_REPAINT_PROC:
	    return (caddr_t) (LINT_CAST(canvas->repaint_proc));

	case CANVAS_RESIZE_PROC:
	    return (caddr_t) (LINT_CAST(canvas->resize_proc));

	case CANVAS_AUTO_CLEAR:
	    return (caddr_t) status(canvas, auto_clear);

	case CANVAS_AUTO_EXPAND:
	    return (caddr_t) status(canvas, auto_expand);

	case CANVAS_AUTO_SHRINK:
	    return (caddr_t) status(canvas, auto_shrink);

	case CANVAS_RETAINED:
	    return (caddr_t) status(canvas, retained);

	case CANVAS_FIXED_IMAGE:
	    return (caddr_t) status(canvas, fixed_image);

	case WIN_VERTICAL_SCROLLBAR:
	    return (caddr_t) canvas_sb(canvas, SCROLL_VERTICAL);

	case WIN_HORIZONTAL_SCROLLBAR:
	    return (caddr_t) canvas_sb(canvas, SCROLL_HORIZONTAL);

 	case HELP_DATA:
 	    return canvas->help_data;

	default:
	    return (caddr_t) 0;
    }
}


/* translate a canvas-space event to a
 * window-space event.
 */
Event *
canvas_window_event(canvas_client, event)
register Canvas		canvas_client;
register Event		*event;
{
    Canvas_info		*canvas;

    canvas = (Canvas_info *)(LINT_CAST(canvas_client));
    event_set_x(event, event_x(event) - canvas_x_offset(canvas) + 
		view_left(canvas));
    event_set_y(event, event_y(event) - canvas_y_offset(canvas) +
		view_top(canvas));
    return event;
}


/* translate a window-space event to a
 * canvas-space event.
 */
Event *
canvas_event(canvas_client, event)
register Canvas		canvas_client;
register Event		*event;
{
    Canvas_info		*canvas;

    canvas = (Canvas_info *)(LINT_CAST(canvas_client));
    event_set_x(event, event_x(event) + canvas_x_offset(canvas) - 
		view_left(canvas));
    event_set_y(event, event_y(event) + canvas_y_offset(canvas) -
		view_top(canvas));
    return event;
}


/* ARGSUSED */
static Notify_value
canvas_handle_event(canvas, event, arg, type)
register Canvas_info	*canvas;
register Event		*event;
Notify_arg		arg;
Notify_event_type	type;
{

    register short	has_position	= FALSE;
    short		pass_through	= TRUE;
    short	is_in_view;

    /* by default, we assume the event is one that
     * should be passed through to the client, and that it does
     * not have a position (e.g. WIN_REPAINT or ascii).
     */
    switch (event_action(event)) {
	case WIN_REPAINT:
	    /* if we still have never called the resize proc,
	     * call it now.
	     */
	    if (!status(canvas, first_resize)) {
		status_set(canvas, first_resize);
		canvas_inform_resize(canvas);
	    }
	    canvas_repaint_window(canvas);
	    break;

	case WIN_RESIZE: {
	    Rect	*rect = (Rect *) (LINT_CAST(window_get(
	    		    (Window)(LINT_CAST(canvas)), WIN_RECT)));
	    Function	resize_proc;

	    /* if this is the first resize event,
	     * make sure canvas_resize_pixwin() does
	     * not call the resize proc, then
	     * call it for sure below.
	     */
	    if (!status(canvas, first_resize)) {
		resize_proc = canvas->resize_proc;
		canvas->resize_proc = 0;
	    }
	    if (rect->r_width != win_width(canvas) || 
		rect->r_height != win_height(canvas)) {
		canvas->win_rect = *rect;
		(void) canvas_resize_pixwin(canvas, FALSE);
		canvas_update_scrollbars(canvas, FALSE);
	    }
	    /* if we still have never called the resize proc,
	     * call it now.
	     */
	    if (!status(canvas, first_resize)) {
		canvas->resize_proc = resize_proc;
		status_set(canvas, first_resize);
		canvas_inform_resize(canvas);
	    }
	    break;
	}

	case SCROLL_REQUEST:
	    canvas_scroll(canvas, (Scrollbar) arg);
	    break;

	case KBD_USE:
	    /* hilite the border if ascii events
	     * are enabled in the input mask.
	     */
	    if (window_get((Window)(LINT_CAST(canvas)),
	    		WIN_CONSUME_KBD_EVENT, WIN_ASCII_EVENTS))
		(void)tool_kbd_use((struct tool *)(LINT_CAST(
			window_get((Window)(LINT_CAST(canvas)),	WIN_OWNER))), 
			(char *)(LINT_CAST(canvas)));
	    break;

	case KBD_REQUEST:
	    /* accept the keyboard request only if ascii events
	     * are enabled in the input mask.
	     */
	    if (!window_get((Window)(LINT_CAST(canvas)),
	    	WIN_CONSUME_KBD_EVENT, WIN_ASCII_EVENTS))
 		(void)win_refuse_kbd_focus((int)(LINT_CAST(window_get(
			(Window)(LINT_CAST(canvas)), WIN_FD))));
	    break;

	case KBD_DONE:
	    (void)tool_kbd_done((struct tool *)(LINT_CAST(
	    	window_get((Window)(LINT_CAST(canvas)),	WIN_OWNER))), 
		(char *)(LINT_CAST(canvas)));
	    break;

	case LOC_RGNENTER:
	    /* let this event through when mouse moves from scrollbar 
	     * directly into canvas. 
	     */
	    if (status(canvas, in_view))
                return NOTIFY_DONE;
            else {
	        is_in_view = rect_includespoint(view_rect(canvas),
                             event_x(event), event_y(event));
                if (is_in_view) {
	            status_set(canvas, in_view);
                    break;
		}
	    }
	case LOC_RGNEXIT:
	    /* let this event through when mouse moves from canvas 
	     * directly into scrollbar. 
	     */
            if (!status(canvas, in_view))
                return NOTIFY_DONE;
            else {
                is_in_view = rect_includespoint(view_rect(canvas),
                             event_x(event), event_y(event)); 
                if (!is_in_view) {
                    status_reset(canvas, in_view);
                    break;
		}
	    }

	case LOC_MOVE:
	case LOC_STILL:
	case LOC_WINENTER:
	case LOC_WINEXIT:
	case LOC_DRAG:
	case LOC_TRAJECTORY:
	    /* all these have an (x, y) position */
	    has_position = TRUE;
	    break;

	/* 
	 * BUG: see the BUG comments in canvas_window_object
	 */
	case ACTION_PROPS:
	case ACTION_OPEN:
	case ACTION_FRONT:
	case ACTION_CLOSE:
	case ACTION_BACK:
	    tool_input(window_get((Window)canvas, WIN_OWNER), event,
	    		(Notify_arg)0, NOTIFY_SAFE);
	    break;
 	case ACTION_HELP:
 	    if (canvas->help_data != NULL) {
 		if (event_is_down(event))
 	            help_request((Window)(LINT_CAST(canvas)),
 				 canvas->help_data, event);
 		return NOTIFY_DONE;
 	    }
 	    has_position = TRUE;
 	    break;
	default:
	    if (event_is_button(event))
		has_position = TRUE;
	    break;
    }

    if (has_position) {

	/* this event has an (x, y) position, so see
	 * if it is in the canvas pixwin region.
	 */

	is_in_view = rect_includespoint(view_rect(canvas), 
	    event_x(event), event_y(event));

	/* map to region enter/exit if needed */
	if (is_in_view) {
	    if (!status(canvas, in_view)) {
		/* this event is in the view, and the last
		 * event was not, so map this event to a
		 * region-enter.
		 */
		event_set_id(event, LOC_RGNENTER);
		status_set(canvas, in_view);
	    }
	} else if (status(canvas, in_view)) {
	    /* this event is not in the view, and the last
	     * event was, so map this event to a
	     * region-exit.
	     */
	    event_set_id(event, LOC_RGNEXIT);
	    status_reset(canvas, in_view);
	} else
	    /* this event is not in the view, and the last
	     * event was also not in the view, so don't pass
	     * this event through.
	     */
	    pass_through = FALSE;
    }

    /* call the client event proc,
     * if any.
     */
    if (pass_through || win_is_io_grabbed((int)window_get(
    		(Window)(LINT_CAST(canvas)), WIN_FD)))
	window_event_proc(((Window)(LINT_CAST(canvas))), 
		canvas_event((Canvas)(LINT_CAST(canvas)), event), arg);

    return NOTIFY_DONE;
}


static Notify_value
canvas_destroy(canvas, stat)
register Canvas_info	*canvas;
Destroy_status		stat;
{
    Scrollbar	sb;

    if (stat == DESTROY_CHECKING)
       return NOTIFY_IGNORED;

    if (stat == DESTROY_CLEANUP) {
       if (sb = canvas_sb(canvas, SCROLL_VERTICAL))
         (void)notify_post_destroy(sb, stat, NOTIFY_IMMEDIATE);
       if (sb = canvas_sb(canvas, SCROLL_HORIZONTAL))
         (void)notify_post_destroy(sb, stat, NOTIFY_IMMEDIATE);
    }
    if (status(canvas, retained) && canvas_pr(canvas)) {
	(void)pr_destroy(canvas_pr(canvas));
	canvas_set_pr(canvas, 0);
    }
    (void)pw_close(canvas->pw);
    free((char *)(LINT_CAST(canvas)));

    return NOTIFY_DONE;
}
