#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)video_win.c 1.1 92/07/30 Copyr 1988 SMI";
#endif
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 *
 * Implements SunView abstraction for Flamingo-style video window.
 * Derived from canvas.c by Marc Ramsey, Technoir Associates.
 */

#include "video_impl.h"

static int		video_set();
static caddr_t		video_get();
static void		video_layout();
static void		video_check_position();
static Notify_value	video_handle_event();
static void		video_inform_resize();
static Notify_value	video_destroy();

/* ARGSUSED */
caddr_t
video_window_object(win, avlist)
register Window		win;
Attr_avlist		avlist;
{
    register Pixwin    *pw = (Pixwin *)(LINT_CAST(window_get(win, WIN_PIXWIN)));
    register Video_info	*video;
    char *calloc();
    struct inputmask  mask;
    Cursor cursor;
    Rect view;
    char avail_planegroups[PIX_MAX_PLANE_GROUPS];

    /* Make sure this device supports video planegroup */
    pr_available_plane_groups(pw->pw_pixrect, PIX_MAX_PLANE_GROUPS,
			          avail_planegroups);
    if ( !avail_planegroups[PIXPG_VIDEO]) {
	return NULL;
    }
    /* Need notification of all window movements. */
    win_setnotifyall(window_get(win, WIN_FD),TRUE);

    video = (Video_info *) (LINT_CAST(calloc(1, sizeof(Video_info))));
    if (!video)
	return NULL;

    video->win_rect    = *(Rect *)(LINT_CAST(window_get(win, WIN_RECT)));

    vid_set_win_defaults(pw->pw_pixrect, &view);
    video_set_x_offset(video, view.r_left);
    video_set_y_offset(video, view.r_top);
    video_set_width(video, view.r_width);
    video_set_height(video, view.r_height);
    video_set_screen_x(video, -1);
    video_set_screen_y(video, -1);

#ifdef ecd.help
    video->help_data = "sunview:video";
#endif

    (void)window_set(win,
	    WIN_NOTIFY_INFO,		PW_INPUT_DEFAULT,
	    WIN_OBJECT,			video, 
	    WIN_SET_PROC,		video_set,
	    WIN_GET_PROC,		video_get,
	    WIN_LAYOUT_PROC,		video_layout,
	    WIN_NOTIFY_EVENT_PROC,	video_handle_event,
	    WIN_NOTIFY_DESTROY_PROC,	video_destroy,
	    WIN_TYPE,			VIDEO_TYPE,
	    WIN_CONSUME_KBD_EVENTS, 	KBD_USE, KBD_DONE, 0,
            WIN_CONSUME_PICK_EVENTS,     
		ACTION_PROPS, ACTION_OPEN, ACTION_FRONT,
#ifdef ecd.help
		ACTION_HELP,
#endif
	        WIN_MOUSE_BUTTONS, LOC_WINENTER, LOC_WINEXIT, 
	        LOC_MOVE, KBD_REQUEST, 0,
	    0);
    return (caddr_t)video;
}


/* update the video when the window is
 * resized from window_set().
 */
/* ARGSUSED */
static void
video_layout(owner, video, op)
Window			owner;
register Video_info	*video;
Window_layout_op	op;
{

    switch (op) {
	case WIN_ADJUSTED: {
	    register Rect *new_win_rect = (Rect *) (LINT_CAST(
	    	window_get((Window)(LINT_CAST(video)), WIN_RECT)));

	    if (new_win_rect->r_width != win_width(video) ||
		new_win_rect->r_height != win_height(video)) {
		video->win_rect = *new_win_rect;
	    }
	    break;
	}
	
	default:
	    break;
    }
}


static int
video_set(video, avlist)
register Video_info	*video;
Attr_avlist		avlist;
{
    register Pixwin	*pw = (Pixwin *) (LINT_CAST(window_get(
				   ((Window)(LINT_CAST(video))), WIN_PIXWIN)));
    register Pixrect	*pr = pw->pw_pixrect;
    register Video_attribute	attr;
    int 		new_video_offset = FALSE;
    int			left, right;

    for (attr = (Video_attribute) avlist[0]; attr;
	 avlist = attr_next(avlist), attr = (Video_attribute) avlist[0]) {
	switch (attr) {
	    case VIDEO_X_OFFSET:
		video_set_x_offset(video, (int) avlist[1]);
		new_video_offset = TRUE;
		break;

	    case VIDEO_Y_OFFSET:
		video_set_y_offset(video, (int) avlist[1]);
		new_video_offset = TRUE;
		break;

	    case VIDEO_RESIZE_PROC:
		video->resize_proc = (Function)(LINT_CAST(avlist[1]));
		break;

	    case VIDEO_LIVE:
		(void)vid_set_live(pr, (int)avlist[1]);
	        break;

	    case VIDEO_INPUT:
		(void)vid_set_input_format(pr, (int)avlist[1]);
	        break;

	    case VIDEO_COMP_OUT:
		(void)vid_set_comp_out(pr, (int)avlist[1]);
	        break;

	    case VIDEO_SYNC:
		(void)vid_set_sync_format(pr, (int)avlist[1]);
	        break;

	    case VIDEO_CHROMA_SEPARATION:
		(void)vid_set_chroma_sep(pr, (int)avlist[1]);
	        break;

	    case VIDEO_DEMOD:
		(void)vid_set_chroma_demod(pr, (int)avlist[1]);
	        break;

	    case VIDEO_R_GAIN:
		(void)vid_set_red_gain(pr, (int)avlist[1]);
	        break;

	    case VIDEO_G_GAIN:
		(void)vid_set_green_gain(pr, (int)avlist[1]);
	        break;

	    case VIDEO_B_GAIN:
		(void)vid_set_blue_gain(pr, (int)avlist[1]);
	        break;

	    case VIDEO_R_LEVEL:
		(void)vid_set_red_black_level(pr, (int)avlist[1]);
	        break;

	    case VIDEO_G_LEVEL:
		(void)vid_set_green_black_level(pr, (int)avlist[1]);
	        break;

	    case VIDEO_B_LEVEL:
		(void)vid_set_blue_black_level(pr, (int)avlist[1]);
	        break;

	    case VIDEO_CHROMA_GAIN:
		(void)vid_set_chroma_gain(pr, (int)avlist[1]);
	        break;

	    case VIDEO_LUMA_GAIN:
		(void)vid_set_luma_gain(pr, (int)avlist[1]);
	        break;

	    case VIDEO_OUTPUT:
		(void)vid_set_video_out(pr, (int)avlist[1]);
	        break;

	    case VIDEO_COMPRESSION:
		(void)vid_set_zoom(pr, (int)avlist[1]);
	        break;

	    case VIDEO_GENLOCK:
		(void)vid_set_gen_lock(pr, (int)avlist[1]);
	        break;

#ifdef ecd.help
  	    case HELP_DATA:
  		video->help_data = (caddr_t) avlist[1];
		break;

#endif 
	    default:
		/*
		return attr_check(attr, ATTR_PKG_VIDEO, "window_set()");
		*/
		break;
	}
    }

    if (new_video_offset)
	video_check_position(video);
    return TRUE;
}


/* ARGSUSED */
static caddr_t
video_get(video, attr, arg1, arg2, arg3)
register Video_info	*video;
Video_attribute	attr;
{
    Pixwin	*pw = (Pixwin *) (LINT_CAST(window_get(
				   ((Window)(LINT_CAST(video))), WIN_PIXWIN)));
    register Pixrect	*pr = pw->pw_pixrect;

    switch (attr) {
	case VIDEO_PIXRECT:
	    return ((caddr_t)vid_get_video_pixrect(pr));

	case VIDEO_X_OFFSET:
	    return (caddr_t) video_x_offset(video);

	case VIDEO_Y_OFFSET:
	    return (caddr_t) video_y_offset(video);

	case VIDEO_RESIZE_PROC:
	    return (caddr_t) (LINT_CAST(video->resize_proc));

	case VIDEO_LIVE:
	    return ((caddr_t)(LINT_CAST(vid_get_live(pr))));

	case VIDEO_INPUT:
	    return ((caddr_t)(LINT_CAST(vid_get_input_format(pr))));

	case VIDEO_SYNC:
	    return ((caddr_t) 0);

	case VIDEO_CHROMA_SEPARATION:
	    return ((caddr_t)(LINT_CAST(vid_get_chroma_sep(pr))));

	case VIDEO_DEMOD:
	    return ((caddr_t)(LINT_CAST(vid_get_chroma_sep(pr))));

	case VIDEO_R_GAIN:
	    return ((caddr_t)(LINT_CAST(vid_get_red_gain(pr))));

	case VIDEO_G_GAIN:
	    return ((caddr_t)(LINT_CAST(vid_get_green_gain(pr))));

	case VIDEO_B_GAIN:
	    return ((caddr_t)(LINT_CAST(vid_get_blue_gain(pr))));

	case VIDEO_R_LEVEL:
	    return ((caddr_t)(LINT_CAST(vid_get_red_black_level(pr))));

	case VIDEO_G_LEVEL:
	    return ((caddr_t)(LINT_CAST(vid_get_green_black_level(pr))));

	case VIDEO_B_LEVEL:
	    return ((caddr_t)(LINT_CAST(vid_get_blue_black_level(pr))));

	case VIDEO_CHROMA_GAIN:
	    return ((caddr_t)(LINT_CAST(vid_get_chroma_gain(pr))));

	case VIDEO_LUMA_GAIN:
	    return ((caddr_t)(LINT_CAST(vid_get_luma_gain(pr))));

	case VIDEO_OUTPUT:
	    return ((caddr_t)(LINT_CAST(vid_get_video_out(pr))));

	case VIDEO_COMPRESSION:
	    return ((caddr_t)(LINT_CAST(vid_get_zoom(pr))));

	case VIDEO_SYNC_ABSENT:
	    return ((caddr_t)(LINT_CAST(vid_get_sync_absent(pr))));

	case VIDEO_BURST_ABSENT:
	    return ((caddr_t)(LINT_CAST(vid_get_burst_absent(pr))));

	case VIDEO_INPUT_STABLE:
	    return ((caddr_t)0);

	case WIN_FIT_WIDTH:
	    return ((caddr_t)video_width(video));

	case WIN_FIT_HEIGHT:
	    return ((caddr_t)video_height(video));

#ifdef ecd.help
 	case HELP_DATA:
 	    return video->help_data;

#endif
	default:
	    return (caddr_t) 0;
    }
}


/* translate a video-space event to a
 * window-space event.
 */
Event *
video_window_event(video_client, event)
register Video         video_client;
register Event          *event;
{
    Video_info         *video;

    video = (Video_info *)(LINT_CAST(video_client));
    event_set_x(event, event_x(event) - video_x_offset(video));
    event_set_y(event, event_y(event) - video_y_offset(video));
    return event;
}


/* translate a window-space event to a
 * video-space event.
 */
Event *
video_event(video_client, event)
register Video         video_client;
register Event          *event;
{
    Video_info         *video;

    video = (Video_info *)(LINT_CAST(video_client));
    event_set_x(event, event_x(event) + video_x_offset(video));
    event_set_y(event, event_y(event) + video_y_offset(video));
    return event;
}


/* ARGSUSED */
static Notify_value
video_handle_event(video, event, arg, type)
register Video_info	*video;
register Event		*event;
Notify_arg		arg;
Notify_event_type	type;
{

    /* by default, we assume the event is one that
     * should be passed through to the client, and that it does
     * not have a position (e.g. WIN_REPAINT or ascii).
     */
    switch (event_action(event)) {
	case WIN_REPAINT: {
	    register Pixwin	*pw = (Pixwin *)
			(LINT_CAST(window_get(video, WIN_PIXWIN)));

	    video_check_position(video);
	    /* if we still have never called the resize proc,
	     * call it now.
	     */
	    if (!status(video, first_resize)) {
		status_set(video, first_resize);
		video_inform_resize(video);
    		pw_use_video(pw);
	    }
	    break;
	}

	case WIN_RESIZE: {
	    register Pixwin	*pw = (Pixwin *)
			(LINT_CAST(window_get(video, WIN_PIXWIN)));

	    video_check_position(video);
	    video_inform_resize(video);
	    break;
	}

	case KBD_USE:
	    /* hilite the border if ascii events
	     * are enabled in the input mask.
	     */
	    if (window_get((Window)(LINT_CAST(video)),
	    		WIN_CONSUME_KBD_EVENT, WIN_ASCII_EVENTS))
		(void)tool_kbd_use((struct tool *)(LINT_CAST(
			window_get((Window)(LINT_CAST(video)),	WIN_OWNER))), 
			(char *)(LINT_CAST(video)));
	    break;

	case KBD_REQUEST:
	    /* accept the keyboard request only if ascii events
	     * are enabled in the input mask.
	     */
	    if (!window_get((Window)(LINT_CAST(video)),
	    	WIN_CONSUME_KBD_EVENT, WIN_ASCII_EVENTS))
 		(void)win_refuse_kbd_focus((int)(LINT_CAST(window_get(
			(Window)(LINT_CAST(video)), WIN_FD))));
	    break;

	case KBD_DONE:
	    (void)tool_kbd_done((struct tool *)(LINT_CAST(
	    	window_get((Window)(LINT_CAST(video)),	WIN_OWNER))), 
		(char *)(LINT_CAST(video)));
	    break;

	/* 
	 * BUG: see the BUG comments in video_window_object
	 */
	case ACTION_PROPS:
	case ACTION_OPEN:
	case ACTION_FRONT:
	    tool_input(window_get((Window)video, WIN_OWNER), event,
	    		(Notify_arg)0, NOTIFY_SAFE);
	    break;
#ifdef ecd.help
 	case ACTION_HELP:
 	    if (video->help_data != NULL) {
 		if (event_is_down(event))
 	            help_request((Window)(LINT_CAST(video)),
 				 video->help_data, event);
 		return NOTIFY_DONE;
 	    }
 	    break;
#endif
	default:
	    break;
    }

    /* call the client event proc,
     * if any.
     */
    window_event_proc(((Window)(LINT_CAST(video))), 
		video_event((Video)(LINT_CAST(video)), event), arg);

    return NOTIFY_DONE;
}


/*
 * This routine is called to determine if the position at which
 * video is inserted needs to be updated.  This will happen when
 * the window is moved, or VIDEO_{X,Y}_OFFSET are changed.
 */
static void
video_check_position(video)
register Video_info    *video;
{
    register Pixwin    *pw = (Pixwin *)
				(LINT_CAST(window_get(video, WIN_PIXWIN)));
    struct pr_pos adjusted;

    /*
     * The adjusted position reflects the windows current position on
     * screen and the current video offset values.
     */
    adjusted.x = pw->pw_clipdata->pwcd_screen_x - video_x_offset(video);
    adjusted.y = pw->pw_clipdata->pwcd_screen_y - video_y_offset(video);
    if (video_screen_x(video) != adjusted.x ||
	video_screen_y(video) != adjusted.y) {
	   video_set_screen_x(video, adjusted.x);
	   video_set_screen_y(video, adjusted.y);
	   vid_set_xy(pw->pw_pixrect, &adjusted);
	   /*
	    * vid_set_xy forces the video enable plane clear, as its
	    * contents will be incorrect if the window driver has
	    * attempted fixup.  This enables all exposed areas.
	    */
	   pw_exposed(pw);
	   pw_preparesurface(pw, RECT_NULL);
    }
}

/* inform the client about the new canvas size */

static void
video_inform_resize(video)
register Video_info    *video;
{
    Rect *rect = (Rect *)(LINT_CAST(window_get((Video)video, WIN_RECT)));

    /* don't call the resize proc if none or window is
     * not installed.
     */
    if (!video->resize_proc || !status(video, first_resize))
        return;

    (*video->resize_proc)(video, rect->r_width, rect->r_height);
}


static Notify_value
video_destroy(video, stat)
register Video_info	*video;
Destroy_status		stat;
{
    Pixwin	*pw = (Pixwin *) (LINT_CAST(window_get(
				   ((Window)(LINT_CAST(video))), WIN_PIXWIN)));

    if (stat == DESTROY_CHECKING)
       return NOTIFY_IGNORED;

/*    pw_video_release(pw); */
    free((char *)(LINT_CAST(video)));

    return NOTIFY_DONE;
}
