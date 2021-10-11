#ifndef lint
#ifdef sccs
static char	sccsid[] = "@(#)alert.ecd.c 1.1 92/07/30";
#endif
#endif


#ifdef ecd.alert

/*
 * Copyright (c) 1986, 1987 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <ctype.h>
#include <varargs.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sundev/kbd.h>
#include <sundev/kbio.h>
#include <sunwindow/sun.h>
#include <sunwindow/defaults.h>
#include <sunwindow/window_hs.h>
#include <sunwindow/win_enum.h>
#include <suntool/fullscreen.h>
#include <suntool/sunview.h>
#include <suntool/tool_struct.h>
#include <suntool/panel.h>
#ifdef ecd.help
#include <suntool/help.h>
#endif
#include <suntool/alert.h>
#include <suntool/alert_impl.h>

/* ------------------------------------------------------------------ */
/* --------------------------- Globals ------------------------------ */
/* ------------------------------------------------------------------ */

static alert_handle		saved_alert;
static struct pixfont		*button_font;
static int			default_beeps;
static int			alert_use_audible_bell;
static int			alert_jump_cursor;
static int			alert_ignore_optional;
static int			alert_fd;
static Rect			alert_screen_rect;

static short alert_bang[] = {
#include <images/alert_bang64.pr>
};
mpr_static(alert_bang_pr, 64, 64, 1, alert_bang);

static short alert_qmark[] = {
#include <images/alert_qmark64.pr>
};
mpr_static(alert_qmark_pr, 64, 64, 1, alert_qmark);

static short alert_gray50_data[] = {
#include <images/square_50.pr>
};
mpr_static(alert_gray50_patch, 16, 16, 1, alert_gray50_data);

/* ------------------------------------------------------------------ */
/* --------------------------- DISPLAY PROCS ------------------------ */
/* ------------------------------------------------------------------ */

static int		alert_show();		/* display alert	 */
static void		alert_get_alert_size(); /* determine rect        */
static Pw_pixel_cache  *alert_drawbox();        /* draw outline & shadow */
static void		alert_layout();		/* draw items            */
static void		alert_build_button();	/* compute button pr     */
static void		alert_outline_button(); /* draw std button       */
static void		alert_yes_outline_button();  /* draw YES button  */

/* ------------------------------------------------------------------ */
/* --------------------------- UTILITY PROCS ------------------------ */
/* ------------------------------------------------------------------ */

static void		alert_select_button();
static void		alert_unselect_button();
static void		alert_preview_message_font();
static void		alert_preview_button_font();
static void 		alert_set_avlist();
static void		alert_default();
static void		alert_add_button_to_list();
static int		alert_text_width();
static int		alert_button_width();
static int		alert_get_fd();
static button_handle	alert_button_for_event();
static int		alert_offset_from_baseline();
static int		calc_max();
static button_handle	create_button_struct();
static void		free_button_structs();
static void		alert_copy_event();
static void		alert_do_bell();
static void		alert_blocking_wait();

/* ------------------------------------------------------------------ */
/* --------------------------- STATICS ------------------------------ */
/* ------------------------------------------------------------------ */

static int		left_marg = 9;
static int 		right_marg = 9;
static int		top_marg = 19;
static int		bottom_marg = 13;
static int		row_gap = 4;
static int		button_gap;
static int		min_button_gap = 12;
/*
 * In the case that the height of the image is larger than the height of 
 * the total text, short_text_line is the number of the lines in the text.
 * Otherwise, it's 0.
 */
static int		 short_text_line;
static struct pixfont	*defpf;
static int		 image_height;
static int		 image_width;

#define BORDER			3	/* white space surounding text */
#define SHADOW			6	/* thickness of shadow */
#define BUTTON_HEIGHT_EXTRA	8  	/* max extra needed for yes button */
#define YES_BUTTON_Y_DIFF	4	/* height diff between yes and no  */
#define ALERT_ACTION_DO_IT	'\015'  /* simple return */
#define MENU_BORDER		2

/*
 * Number of pixels of the image that is hanging outside the alert_box
 */
#define IMAGE_OUTSIDE_BOX		32

/* ------------------------------------------------------------------ */
/* ----------------------- Public Interface ------------------------- */
/* ------------------------------------------------------------------ */

/* VARARGS */

int
alert_prompt(client_frame, event, va_alist)
    Frame	 client_frame;
    Event	*event;
    va_dcl
{
    va_list			valist;
    int 			result;

#ifdef ecd.help
    for (;;) {
	va_start(valist);
	result = alert_show(client_frame, event, valist);
	va_end(valist);
	if (result != ALERT_HELP)
	    break;
	help_request(client_frame, saved_alert->help_data, event);
    }

#else
    va_start(valist);
    result = alert_show(client_frame, event, valist);
    va_end(valist);
#endif
    return (result);
}

/* ------------------------------------------------------------------ */
/* ----------------------PRIVATE PROCS------------------------------- */
/* ------------------------------------------------------------------ */

static int
alert_show(client_frame, event, valist)
    Frame	 client_frame;
    Event	*event;
    va_list	valist;
{
    Event			ie;
    Alert_attribute		avlist[ATTR_STANDARD_SIZE];
    register alert_handle	alert;
    int				result, fd, opened;
    Pw_pixel_cache		*bitmap_under;
    struct rect 		rectsaved, rect;
    struct fullscreen 		*fs;

    button_handle	button;
    button_handle	current_button = NULL;
    int			is_highlighted = FALSE;
    int			old_mousex, old_mousey;
    unsigned short	this_event;
    
    /*
     * assuming that the image from the top of the alert_box proper, then
     * image_height is the number of pixels from the top of the alert_box to
     * lowest point of the image glyph.
     */
    if (!saved_alert)  {
	char	*font_name;
	int	my_fd;

	saved_alert = (struct alert *)calloc(1, sizeof(struct alert));
	if (!saved_alert) {
	    (void)printf(stderr, "Alert ERROR: malloc failed.");
	    return (ALERT_FAILED);
	}
	alert_use_audible_bell = (int)defaults_get_boolean(
	    "/SunView/Audible_Bell", (Bool)TRUE, (int *)NULL);
	alert_jump_cursor = (int)defaults_get_boolean(
	    "/SunView/Alert_Jump_Cursor", (Bool)TRUE, (int *)NULL);
	alert_ignore_optional = (int)defaults_get_boolean(
	    "/SunView/Ignore_Optional_Alerts", (Bool)FALSE , (int *)NULL);
	font_name = defaults_get_string(
	    "/Menu/Font",	
	    "/usr/lib/fonts/fixedwidthfonts/screen.b.14", 	
	    (int *)0);
	button_font = pf_open(font_name);
	default_beeps = defaults_get_integer(
		"/SunView/Alert_Bell", 1, (int *)NULL);
	defpf = pw_pfsysopen();
        if (!button_font)
            button_font = defpf;
    }

    alert = saved_alert;

    if ((opened = alert_get_fd(&alert->fd_for_fullscreen)) == 0) {
	return (ALERT_FAILED);
    }

    fd = alert->fd_for_fullscreen;

    (void) alert_default(alert);

    alert->event = (Event *)event;
    alert->client_frame = client_frame;

    attr_make (avlist, ATTR_STANDARD_SIZE, valist);
    (void)alert_set_avlist (alert, avlist, FALSE);
    
    /* only if ALERT_OPTIONAL && ALERT_BUTTON_YES && default option */
    if (alert->is_optional &&
		alert->yes_button_exists &&
		(alert_ignore_optional == TRUE))
        return (ALERT_YES);

    alert_do_bell(alert, fd);

    fs = fullscreen_init(fd); /* go fullscreen first */
    if (fs == 0) {
	return (ALERT_FAILED);  /* probably out of fds */
    }
    /* now determine the size of the alert */
    (void)alert_get_alert_size(alert, &rect, fs->fs_windowfd);
    if ((bitmap_under = alert_drawbox( /* then draw empty box */
		fs->fs_pixwin, &rect, &rectsaved, fs)) == NULL)
	goto RestoreState;
    /* now fill in the box with the image, text, buttons etc */
    (void)alert_layout(alert, &rect, fs->fs_pixwin);

    if (alert_jump_cursor && alert->yes_button_exists) {
	button_handle	curr;
      	old_mousex = win_get_vuid_value(fd, LOC_X_ABSOLUTE);
    	old_mousey = win_get_vuid_value(fd, LOC_Y_ABSOLUTE);
	for (curr = alert->button_info; curr != NULL; curr = curr->next) 
	    if (curr->is_yes)
	    	win_setmouseposition(fd,
	    	    curr->button_rect.r_left + curr->button_rect.r_width / 2,
	    	    curr->button_rect.r_top + curr->button_rect.r_height / 2
	    	    );
    }
    /* Stay in fullscreen until a button is pressed, or trigger used */
    for (;;) {
	unsigned short	trigger = alert->default_input_code;

    	if (input_readevent(fs->fs_windowfd, &ie) == -1)
	    break;
	this_event = event_action(&ie); /* get encoded event */
	
	button = alert_button_for_event(alert, &ie);

	if ((this_event == trigger) && ((trigger == (int)MS_LEFT) ? 
	    (event_is_up(&ie) && (current_button == NULL)) : 1)) { 
	    /* catch UP mouse left */
	    alert->result = ALERT_TRIGGERED;
	    (void) alert_copy_event(alert, &ie);
	    goto Done;
	} else if (((this_event == ACTION_STOP) || (this_event == WIN_STOP))
	        && saved_alert->no_button_exists) {
	    alert->result = ALERT_NO;
	    (void) alert_copy_event(alert, &ie);
	    goto Done;
        } else if ((this_event == ACTION_DO_IT
	         || this_event == ALERT_ACTION_DO_IT)
		    && saved_alert->yes_button_exists) {
	    alert->result = ALERT_YES;
	    (void) alert_copy_event(alert, &ie);
	    goto Done;
	} else if ((this_event == MS_LEFT) && (alert->button_info)) {
	    if (event_is_down(&ie) && (button != NULL)) {
		alert_select_button(fs->fs_pixwin, button);
		current_button = button;
		is_highlighted = TRUE;
	    } else if (event_is_up(&ie)) {
		if ((current_button && is_highlighted)
			&& (button != NULL)
	                && (button == current_button)) {
		    alert->result = button->value;
		    (void) alert_copy_event(alert, &ie);
		    goto Done;
		} else if ((current_button && is_highlighted)
		        && (button != NULL)
	                && (button != current_button)) {
		    alert->result = button->value;
		    (void) alert_copy_event(alert, &ie);
		    goto Done;
		}
	    }
#ifdef ecd.help
	} else if (this_event == ACTION_HELP) {
	    if (event_is_down(&ie)) {
		alert->result = ALERT_HELP;
		(void) alert_copy_event(alert, &ie);
		goto Done;
	    }
#endif
	} else if (this_event == LOC_DRAG) {/* mouse moved w/MS down */
	    if (((current_button != NULL) && is_highlighted)
		    && ((button == NULL))
		    &&  (button != current_button)) {
		alert_unselect_button(fs->fs_pixwin, current_button);
		current_button = NULL;
		is_highlighted = FALSE;
	    }
	    if ((button != current_button) && (is_highlighted == FALSE)) {
		alert_select_button(fs->fs_pixwin, button);
		current_button = button;
		is_highlighted = TRUE;
	    }
	}
    }
Done:
    pw_restore_pixels(fs->fs_pixwin, bitmap_under);
RestoreState:
    if (alert_jump_cursor && alert->yes_button_exists) 
	    win_setmouseposition(fd, old_mousex, old_mousey);
    fullscreen_destroy(fs);

    free_button_structs(alert->button_info);
    result = alert->result;

    return (result);
}

static void
alert_copy_event(alert, event)
    register alert_handle alert;
    Event		 *event;
{
    if (saved_alert->event == (Event *) 0) {
	return;
    } else {
	saved_alert->event->ie_code = event->ie_code;
	saved_alert->event->ie_flags = event->ie_flags;
	saved_alert->event->ie_shiftmask = event->ie_shiftmask;
	saved_alert->event->ie_locx = event->ie_locx;
	saved_alert->event->ie_locy = event->ie_locy;
	saved_alert->event->ie_time.tv_sec = event->ie_time.tv_sec;
	saved_alert->event->ie_time.tv_usec = event->ie_time.tv_usec;
    }
/*
    saved_alert->event->ie_locx = (event_x(event) < 0) ?
	    (saved_alert->position_x - abs(event_x(event))) :
	    (saved_alert->position_x + event_x(event));
    saved_alert->event->ie_locy = (event_y(event) < 0) ? 
	    (saved_alert->position_y - abs(event_y(event))) :
	    (saved_alert->position_y + event_y(event));
*/
    if (saved_alert->client_frame) { /* make event client relative */
	Rect	client_frame_place;

	window_getrelrect(0,
	    window_fd(saved_alert->client_frame), &client_frame_place);
        saved_alert->event->ie_locx -= client_frame_place.r_left;
        saved_alert->event->ie_locy -= client_frame_place.r_top;
    }
}

/* ------------------------------------------------------------------ */
/* --------------------------- Statics ------------------------------ */
/* ------------------------------------------------------------------ */

static int
alert_preview_avlist_for_int_value(attr, avlist, default_if_not_present)
    Alert_attribute		attr;
    register Alert_attribute	*avlist;
    int				default_if_not_present;
{
    Alert_attribute	current_attr;
    int			value = default_if_not_present;

    for (; *avlist; avlist = alert_attr_next(avlist)) {
	current_attr = avlist[0];
	if (attr == current_attr) value = (int)avlist[1];
    }
    return (int)value;
}

static void
alert_preview_button_font(alert, avlist)
    register alert_handle	alert;
    register Alert_attribute	*avlist;
{
    Alert_attribute	attr;
    caddr_t		value;

    for (; *avlist; avlist = alert_attr_next(avlist)) {
        attr = avlist[0]; value = (caddr_t)avlist[1];
	switch (attr) {
	  case ALERT_BUTTON_FONT:
	    alert->button_font = (struct pixfont *) value;
	    break;
	  default:
	    break;
	}
    }
}

static void
alert_preview_message_font(alert, avlist)
    register alert_handle	alert;
    register Alert_attribute	*avlist;
{
    Alert_attribute	attr;
    caddr_t		value;

    for (; *avlist; avlist = alert_attr_next(avlist)) {
        attr = avlist[0]; value = (caddr_t)avlist[1];
	switch (attr) {
	  case ALERT_MESSAGE_FONT:
	    alert->message_font = (struct pixfont *) value;
	    break;
	  default:
	    break;
	}
    }
}

static void
alert_set_avlist(alert, avlist, caller_external)
    register alert_handle	alert;
    register Alert_attribute	*avlist;
    int				caller_external;
{
    int			i;
    int			yes_button_seen = FALSE;
    int			no_button_seen = FALSE;
    int			number_of_buttons_seen = 0;
    int			trigger_set = 0;
    Alert_attribute	attr;
    caddr_t		value;
    int			image_attr_seen = 0; /* use default if haven't */
    
    alert_preview_button_font(alert, avlist);
    alert_preview_message_font(alert, avlist);

    for (; *avlist; avlist = alert_attr_next(avlist)) {
        attr = avlist[0]; value = (caddr_t)avlist[1];
	switch (attr) {

	case WIN_X:
	    alert->client_offset_x = (int)value;
	    break;

	case WIN_Y:
	    alert->client_offset_y = (int)value;
	    break;

	case ALERT_POSITION: 
	    if ((int)value == (int)ALERT_SCREEN_CENTERED ||
		(int)value == (int)ALERT_CLIENT_CENTERED ||
		(int)value == (int)ALERT_CLIENT_OFFSET)
		alert->position = (int)value;
	    break;

	case ALERT_OPTIONAL:
	    alert->is_optional = ((int)value == 1) ? 1 : 0;
	    break;

	case ALERT_NO_BEEPING:
	    if ((int)value == 1) alert->dont_beep = 1;
	    break;

	case ALERT_NO_IMAGE:
	    if ((int)value == 1) alert->no_image = 1;
	    break;


	case ALERT_IMAGE:
	    alert->image_item = (struct pixrect *)value;
	    image_attr_seen = 1;
	    break;

	case ALERT_TRIGGER:
	    alert->default_input_code = (int)avlist[1];
	    trigger_set = 1;
	    break;

	case ALERT_MESSAGE_STRINGS_ARRAY_PTR: 
	    alert->message_items = (char **)value;
	    break;

	case ALERT_MESSAGE_STRINGS:
	    alert->message_items = (char **)&avlist[1];
	    break;

	case ALERT_BUTTON_YES: {
	    struct pixrect	*image;
	    button_handle	button;

	    if (!yes_button_seen) {
		yes_button_seen = TRUE;
	    } else {
	        fprintf(
		    stderr,
		    "alert: Only one ALERT_BUTTON_YES allowed. Attr ignored.\n");
		break;
	    }
	    button = (button_handle)create_button_struct();
	    button->string = (char *)avlist[1];
	    button->is_yes = TRUE;
	    button->value = ALERT_YES;
	    alert->yes_button_exists = TRUE;
	    number_of_buttons_seen++;
	    (void) alert_add_button_to_list(alert, button);
	    break;
	}

	case ALERT_BUTTON_NO: {
	    struct pixrect	*image;
	    button_handle	button;

	    if (!no_button_seen){
		no_button_seen = TRUE;
	    } else {
	        fprintf(
		    stderr,
		    "alert: Only one ALERT_BUTTON_NO allowed. Attr ignored.\n");
		break;
	    }
	    button = (button_handle)create_button_struct();
	    button->string = (char *)avlist[1];
	    button->is_no = TRUE;
	    button->value = ALERT_NO;
	    alert->no_button_exists = TRUE;
	    number_of_buttons_seen++;
	    (void) alert_add_button_to_list(alert, button);
	    break;
	}

	case ALERT_BUTTON: { 
	    button_handle	button;

	    button = (button_handle)create_button_struct();
	    button->string = (char *)avlist[1];
	    button->value = (int)avlist[2];
	    (void) alert_add_button_to_list(alert, button);
	    number_of_buttons_seen++;
	    break;
	}

	case ALERT_BUTTON_FONT:
	case ALERT_MESSAGE_FONT:
	    /* already previewed above */
	    break;

#ifdef ecd.help
	case HELP_DATA:
	    alert->help_data = value;
	    break;
#endif

	default:
	    fprintf(stderr, "alert: attribute not allowed.\n");
	    break;
	}
    }
    if ((!image_attr_seen) && (!alert->no_image)) { 
	/*  use defaults */
	alert->image_item = (struct pixrect *) (number_of_buttons_seen > 1)
	    ? &alert_qmark_pr : &alert_bang_pr;
    }
    if (alert->image_item) {
	image_width = alert->image_item->pr_size.x;
	image_height = alert->image_item->pr_size.y;
    } else
	image_height = image_width = 0;
    if ((number_of_buttons_seen == 0) && (trigger_set == 0)) 
	alert->default_input_code = (int)ACTION_STOP;
}

/* ------------------------------------------------------------------ */

static void
alert_get_alert_size(alert, rect, fd)
    register alert_handle	alert;
    struct rect			*rect;
    int				fd;
{
    Rect		r;
    int			x, y, height, width, i, left, top, num_lines = 0;

    short_text_line = 0;
    x = 0;  y = 0; /* start calculations out at zero, add margins later */

    if (alert->message_items) {
	char	**strs;
	char	**tmp_strings;
	int	max_width = 0;
	int	str_width = 0;
        Pixfont	*mess_font = (alert->message_font) ?
				alert->message_font : defpf;

	strs = (char **)alert->message_items;
	tmp_strings = (char **)alert->message_items;
	while (*strs) {
	    str_width = alert_text_width(mess_font, (char *)*strs);
	    max_width = calc_max(str_width, max_width);
	    num_lines++;
	    strs++;
	}
	x += max_width;
	y += (num_lines*mess_font->pf_defaultsize.y);
    }

    /*
     * Make sure that the text part is at least as high as the
     * height of the image
     */
    if (y + row_gap > image_height)  {
	y += row_gap;
	short_text_line = 0;
    } else {
        short_text_line = num_lines;
	y = image_height + top_marg;
    }

    i = 0;
    if (alert->button_info) {
	int  		max_buttons_width = 0;
	int		width = 0;
	int		total = 0;
        button_handle	curr;
        struct pixfont *font = alert->button_font;

	for (curr = alert->button_info; curr != NULL; curr = curr->next) {
	    width = alert_button_width(font, curr);
	    max_buttons_width += width;
	    i++; 
	    }
	y += font->pf_defaultsize.y + BUTTON_HEIGHT_EXTRA;
	/*
	 * figure out the total pixels needed to fit buttons and text, which
	 * ever is longer.  This is just for the actual buttons and/or text,
	 * and not including the margins
	 */
	x = calc_max(max_buttons_width, x);
	total = x + left_marg + image_width/2 + right_marg + (4*BORDER);
	if (total >= alert_screen_rect.r_width) { 
  	  /* if buttons too wide, then they clip just like the text will.
  	   * this is not very nice, but TOO BAD!! it's a rough job attempting
  	   * to handle their wraping.
  	   */
	    button_gap = 0;
	} else {
	    if (i==1) {
		button_gap = calc_max(0, (x - max_buttons_width)/2);
	    } else { /* >1 */
		button_gap = calc_max(0, (x - max_buttons_width)/(i-1));
	    }
	    if (i>1 && button_gap == 0 && (((i-1)*min_button_gap) < total)) {
	    	button_gap = min_button_gap;
	    	x += (i-1) * min_button_gap;
	    }
	}
    }
    y += top_marg + bottom_marg + (2*BORDER);
    /* 
     * add in the borders and the IMAGE_OUTSIDE_BOX pixels that are outside of the 
     * alert_box, but is regarded as alert_box-metro area
     */
    x += left_marg + image_width + right_marg +
	(4*BORDER) + IMAGE_OUTSIDE_BOX;

    /*
     * discount the IMAGE_OUTSIDE_BOX pixels of the image for centering purpose
     */
    width = x;
    height = y;

    /* 
     * now try and position the alert and make it pretty 
     */
/*  if ((alert->client_frame == (Frame)0) 
	    || (window_get(alert->client_frame, FRAME_CLOSED)))
	alert->position = (int)ALERT_SCREEN_CENTERED;
*/
    if (alert->client_frame == (Frame)0)
	alert->position = (int)ALERT_SCREEN_CENTERED;
    if (alert->client_frame != (Frame)0)
	window_getrelrect(0, window_fd(alert->client_frame), &r);
    if ((alert->position == (int)ALERT_SCREEN_CENTERED)
	    || (alert->client_frame == (Frame)0)) {
	/*
	 * Disregard the part of image outside of the box when centering
	 */
	left = calc_max(0, (alert_screen_rect.r_width  - width - IMAGE_OUTSIDE_BOX)/2);
	top = calc_max(0, (alert_screen_rect.r_height - height)/2);
    } else {
	/*
	 * Disregard the part of image outside of the box when centering
	 */
	left = calc_max(0, r.r_left + (r.r_width  - width - IMAGE_OUTSIDE_BOX)/2);
	top = calc_max(0, r.r_top + (r.r_height - height)/2);

	if (alert->position == (int)ALERT_CLIENT_OFFSET) {
	    left = alert->client_offset_x + r.r_left-(calc_max(0, width/2));
	    top = alert->client_offset_y + r.r_top-(calc_max(0, height/2));
	}
/* try to reposition alert near frame for aesthetics, if near boarder */
	if ((left <= (r.r_left)) && (width >= r.r_width)) {
	    if ((left == 0)
		&& ((r.r_left + (r.r_width/2)) <= alert_screen_rect.r_width)) {
		left += (r.r_width/2);
	    }
	    if ((left < r.r_left) && ((left + width + 
	      (BORDER + SHADOW)) >= alert_screen_rect.r_width)) {
		left -= (r.r_width/2);
		if ((left + width + (BORDER + SHADOW)) >= 
			alert_screen_rect.r_width) { /* still overflows */
		    left = alert_screen_rect.r_width - width - 
			(BORDER + SHADOW);
		    if ((left - (r.r_width/2)) >= 0)
			left -= (r.r_width/2);
		}
	    }
	}
	if ((top <= (r.r_top)) && (height >= r.r_height)) {
	    if ((top == 0)
		&& ((r.r_top + (r.r_height/2)) <= alert_screen_rect.r_height)) {
		top += (r.r_height/2);
	    }
	    if ((top < r.r_top) && ((top + height + 
	      (BORDER + SHADOW)) > alert_screen_rect.r_height)) {
		top -= (r.r_height/2);
		if ((top + height + (BORDER + SHADOW)) >= 
			alert_screen_rect.r_height) { /* still overflows */
		    top = alert_screen_rect.r_height - height - 
			(BORDER + SHADOW);
		    if ((top - (r.r_height/2)) >= 0)
			top -= (r.r_height/2);
		}
	    }
	}

    }
    if (width > alert_screen_rect.r_width) left = 0;
    if (height > alert_screen_rect.r_height) top = 0;
    rect->r_top = top;
    rect->r_left = left;
    rect->r_width = x;
    rect->r_height = y;
}

static void
alert_layout(alert, rect, pw)
    register alert_handle	alert;
    struct rect			*rect;
    struct pixwin		*pw;
{
    Rect		r;
    int			x, y, width, height, i, j, remembered_x;
    int			y_before_buttons, y_after_buttons;
    int			alert_too_wide = 0;
    int			alert_too_tall = 0;
    button_handle	curr;
    int			offset;
    /*
     * image_offset is the number of pixels of the image that
     * is outside the alert_box.  It changes when the image change
     */
    int			image_offset = IMAGE_OUTSIDE_BOX;

    x = rect->r_left + image_offset;
    y = rect->r_top + top_marg + BORDER;

    pw_lock(pw, &alert_screen_rect);

    if (alert->image_item) {
	pw_write(pw, x - alert->image_item->pr_size.x/2 - (2 * MENU_BORDER), y,
	    alert->image_item->pr_size.x, alert->image_item->pr_size.y,
	    PIX_SRC, alert->image_item, 0, 0);
	pr_replrop(pw->pw_clipdata->pwcd_prsingle,
	    x + alert->image_item->pr_size.x/2 - (2 * MENU_BORDER), y + SHADOW,
	    SHADOW, alert->image_item->pr_size.y, PIX_SRC, 
	    &alert_gray50_patch, x,  y);
	pr_replrop(pw->pw_clipdata->pwcd_prsingle,
	    x - (2 * MENU_BORDER), y + alert->image_item->pr_size.y,
	    alert->image_item->pr_size.x/2 + SHADOW,  SHADOW, PIX_SRC, 
	    &alert_gray50_patch, x,  y);
    }
    x += image_width/2 + left_marg + BORDER;
    if (alert->message_items) {
	char	**strs;
	int	num_lines;
        Pixfont	*font = (alert->message_font) ? alert->message_font : defpf;
        int	offset = alert_offset_from_baseline(font);

	y = rect->r_top + top_marg + row_gap;
	strs = (char **)alert->message_items;
	while (*strs) {
	    pw_text(pw, x, (offset >= 0) ? y + offset : y - offset,
	        PIX_SRC, font, (char *)*strs);
	    num_lines++;
	    y += font->pf_defaultsize.y;
	    strs++;
	}
    }

    x = rect->r_left + image_offset + image_width/2 + left_marg + BORDER;
    if (short_text_line)
	y = rect->r_top + BORDER + top_marg*2 + image_height;
    else 
	y += row_gap;

    if (alert->button_info && (alert->number_of_buttons==1)) {
	x += button_gap;
	alert_build_button(pw, rect,
	    x, (alert->button_info->is_yes) ? y : y + (YES_BUTTON_Y_DIFF / 2),
	    alert->button_info, alert->button_font);
    } else if (alert->button_info && (alert->number_of_buttons>1)) {
	for (curr = alert->button_info; curr != NULL; curr = curr->next) {
	    alert_build_button(pw, rect, x,
	    	(curr->is_yes) ? y : y + (YES_BUTTON_Y_DIFF / 2),
	    	curr, alert->button_font);
	    x += curr->button_rect.r_width + button_gap;
	    i++;
        }
    }
    pw_unlock(pw);
}

/* ------------------------------------------------------------------ */
/* ----------------------   Misc Utilities   ------------------------ */
/* ------------------------------------------------------------------ */

static void
alert_default(alert)
    alert_handle    alert;
{
    alert->beeps = default_beeps; 
    alert->default_input_code = '\0'; /* ASCII NULL */
    alert->button_font = button_font;
    alert->message_font = (Pixfont *) 0;
    alert->position = (int)ALERT_CLIENT_CENTERED;
    alert->client_offset_x = FALSE;
    alert->client_offset_y = FALSE;
    alert->dont_beep = FALSE;
    alert->no_image = FALSE;
    alert->yes_button_exists = FALSE;
    alert->no_button_exists = FALSE;
    alert->is_optional = FALSE;
    alert->event = (Event *) 0;
    alert->button_info = (button_handle) 0;
    alert->number_of_buttons = 0;
#ifdef ecd.help
    alert->help_data = "sunview:alert";
#endif
    button_gap = 0;
}

/* ------------------------------------------------------------------ */

static int
calc_max(a, b) /* calculate rather than macro */
    int		a, b;
{
    return ((a>b) ? a : b);
}

/* ------------------------------------------------------------------ */

static void
alert_add_button_to_list(alert, button)
    register alert_handle	alert;
    button_handle		button;
{
    button_handle	curr;

    if (alert->button_info) {
	for (curr = alert->button_info; curr; curr = curr->next)
		if (curr->next == NULL) {
		    curr->next = button;
		    break;
		}
    } else
	alert->button_info = button;
    alert->number_of_buttons++;
}

/* ------------------------------------------------------------------ */

static button_handle
create_button_struct()
{
    button_handle	pi = NULL;

    pi = (button_handle) calloc(1, sizeof(struct buttons));
    if (!pi)
	fprintf(stderr, "alert: Malloc failed in create_button_struct.\n");
    return pi;
}


/* ------------------------------------------------------------------ */

static void
free_button_structs(first)
    button_handle	first;
{
    button_handle	current;
    button_handle	next;
    
    if (!first)
	return;
    for (current=first; current != NULL; current=current->next) {
	next = current->next;
	free(current);
    }
}

/* ------------------------------------------------------------------ */
/* font char/pixel conversion routines                                */
/* ------------------------------------------------------------------ */

static int
alert_offset_from_baseline(font)
Pixfont *font;
{
    if (font == NULL)
    	return (0);
    return (font->pf_char[32].pc_home.y); /* space char */
}

static int
alert_text_width(font, str)
    Pixfont	*font;
    char	*str;
{
    struct pr_size    size;
    
    size = pf_textwidth(strlen(str), font, str);
    return (size.x);
} 

static int
alert_button_width(font, button)
    Pixfont		*font;
    button_handle	button;
{
    int text_width = alert_text_width(font, button->string);
    return (text_width + ((button->is_yes) ? 16 : 12));
}

static Pw_pixel_cache *
alert_drawbox(pw, rectp, rectsavep, fs)
	struct pixwin	   *pw;
	struct	rect	   *rectp;
	struct	rect	   *rectsavep;
	struct	fullscreen *fs;
{
	struct rect	 temp_rect;
	Pw_pixel_cache	*bitmap_under;
	Pw_pixel_cache	*pw_save_pixels();
	int		 image_x_offset = 
			 	IMAGE_OUTSIDE_BOX - (2 * MENU_BORDER);
	
	struct	pixrect *pr;
	struct rect	rect;
	int	x,y;		/* XXX workaround */
	
	temp_rect = *rectp;
	rect = *rectp;
	rect.r_width += SHADOW;
	rect.r_height += SHADOW;
	if ((bitmap_under = pw_save_pixels(pw, &rect)) == 0)
		return(0);
	*rectsavep = rect;
	rect.r_width -= SHADOW;
	rect.r_height -= SHADOW;

	/*
	 * shrink the surface to be prepared by the number of pixels that
	 * are hanging outside of the alert_box.  This way, the pw_preparesurface
	 * is called(clearing the underlying bits), some bits underneath the
	 * "Big alert_box"(defined by the bounding rect that includes the
	 * image don't simply get cleared.
	 */
	temp_rect.r_left += image_x_offset;
	temp_rect.r_width -= image_x_offset;
	/* draw shadow of prompt box */
	pw_preparesurface(pw, &temp_rect);
	rect.r_left += SHADOW;
	rect.r_top += SHADOW;
	pw_preparesurface(pw, &temp_rect);
	rect.r_left -= SHADOW;
	rect.r_top -= SHADOW;

	pw_lock(pw, &alert_screen_rect); /* protect underling pr */

	/* restore stuff bitmap_under shadow */
	pr = pw_primary_cached_mpr(fs->fs_pixwin, bitmap_under);
	pw_write(pw, rect.r_left+rect.r_width, rect.r_top + SHADOW,
	    SHADOW, rect.r_height, PIX_SRC, pr,
	    rect.r_width, SHADOW);
	pw_write(pw, rect.r_left+SHADOW, rect.r_top + rect.r_height,
	    rect.r_width,  SHADOW, PIX_SRC, pr,
	    SHADOW, rect.r_height);

	/* 
	 * XXX Workaround for pw_replrop bug.  Get pixwin by hand instead
	 */
	/* pw_replrop(pw, rect.r_left + SHADOW, rect.r_top + SHADOW,
	    rect.r_width, rect.r_height, PIX_SRC|PIX_DST, &alert_gray50_patch, 0, 0);
	 */
	
	/* draw drop shadow as source instead of OR (|PIX_DST) */
	x = pw->pw_clipdata->pwcd_clipping.rl_bound.r_left;
	y = pw->pw_clipdata->pwcd_clipping.rl_bound.r_top;
	/* this rect includes a left-offset for the image */
	pr_replrop(pw->pw_clipdata->pwcd_prsingle,
	    rect.r_left + rect.r_width - x,
	    rect.r_top + SHADOW - y,
	    SHADOW, rect.r_height, PIX_SRC, 
	    &alert_gray50_patch,
	    rect.r_left + rect.r_width - x + image_x_offset, 
	    rect.r_top + SHADOW - y);
	pr_replrop(pw->pw_clipdata->pwcd_prsingle,
	    rect.r_left + SHADOW - x + image_x_offset, 
	    rect.r_top + rect.r_height - y,
	    rect.r_width - SHADOW - image_x_offset,  SHADOW, PIX_SRC, 
	    &alert_gray50_patch,
	    rect.r_left + SHADOW - x + image_x_offset, 
	    rect.r_top + rect.r_height - y);

	/* draw border of prompt box */
	pw_writebackground(pw, 
	    rect.r_left + image_x_offset, 
	    rect.r_top,
	    rect.r_width - image_x_offset, 
	    rect.r_height, PIX_SET);
	rect_marginadjust(&rect, - (2 * MENU_BORDER));
	pw_writebackground(pw, 
	    rect.r_left + image_x_offset, 
	    rect.r_top,
	    rect.r_width - image_x_offset, 
	    rect.r_height, PIX_CLR);

	pw_unlock(pw);

	*rectp = rect;
	return(bitmap_under);
}

static void
alert_build_button(pw, rect, x, y, button, font)
    struct pixwin		*pw;
    struct rect			*rect;
    int				x, y;
    button_handle		button;
    struct pixfont		*font;
{
    struct pr_prpos 	where;	/* where to write the string */
    struct pr_size  	size;	/* size of the pixrect */
    int			width, height;
    char 		*string = button->string;
    int			special = button->is_yes;
    
    size = pf_textwidth(strlen(string), font, string);
    width = size.x;

    where.pr = (special) ?
	mem_create(width + 16, size.y + 8, 1)
      : mem_create(width + 12, size.y + 4, 1);

    if (!where.pr)
       return;

    where.pos.x = (special) ?
	 8 + (width - size.x) / 2
       : 6 + (width - size.x) / 2;
    where.pos.y = (special) ?
	 4 + panel_fonthome(font)
       : 2 + panel_fonthome(font);

    (void)pf_text(where, PIX_SRC, font, string);
   
    if (special) {
	alert_yes_outline_button(where.pr);
    } else alert_outline_button(where.pr);

    width += ((special) ? 16 : 12);
    height = size.y + ((special) ? 8 : 4);

    button->button_rect.r_top = y;
    button->button_rect.r_left = x;
    button->button_rect.r_width = width;
    button->button_rect.r_height = height;

    pw_write(pw, x, y, width, height, PIX_SRC, where.pr, 0, 0);
}

/* returns 0 if fails to open fd */
static int
alert_get_fd(fd)
    int	*fd;
{
    int		windowfd, result, parentlink, parentfd;
    Inputmask	new_kbd_mask, new_pick_mask;
    char	name[WIN_NAMESIZE];

    if (alert_fd) {
	*fd = alert_fd;
	return(1);
    }

    windowfd = win_getnewwindow();
    if (windowfd < 0)
	return(0);
    if (we_getparentwindow(name))
	return(0);
    if ((parentfd = open(name, O_RDONLY, 0)) < 0) 
	return(0);
    parentlink = win_nametonumber(name);
    (void)win_setlink(windowfd, WL_PARENT, parentlink);
    (void)win_getrect(parentfd, &alert_screen_rect);
    close(parentfd);
    (void)win_setrect(windowfd, &alert_screen_rect);

    (void)input_imnull(&new_kbd_mask);
    new_kbd_mask.im_flags |= IM_ASCII | IM_META | IM_NEGMETA | IM_NEGEVENT;
    win_setinputcodebit(&new_kbd_mask, WIN_STOP);
#ifdef ecd.help
    (void)win_keymap_set_smask(windowfd, ACTION_HELP);
    win_keymap_set_imask_from_std_bind(&new_kbd_mask, ACTION_HELP);
#endif
    (void)win_set_kbd_mask(windowfd, &new_kbd_mask);

    (void)input_imnull(&new_pick_mask);
    new_pick_mask.im_flags |= IM_NEGEVENT;
    win_setinputcodebit(&new_pick_mask, WIN_STOP);
    win_setinputcodebit(&new_pick_mask, MS_LEFT);
    win_setinputcodebit(&new_pick_mask, MS_MIDDLE);
    win_setinputcodebit(&new_pick_mask, MS_RIGHT);;
    win_setinputcodebit(&new_pick_mask, LOC_DRAG);;
    (void)win_set_pick_mask(windowfd, &new_pick_mask);

    *fd = windowfd;
    alert_fd = windowfd;
    return (1);
}

static void
alert_outline_button(pr)
register Pixrect *pr;
/* outline_button draws an outline of a button in pr.
*/
{
   int	x_left		= 0;
   int	x_right		= pr->pr_size.x - 1;
   int	y_top		= 0;
   int	y_bottom	= pr->pr_size.y - 1;
   int	x1		= 3;
   int	x2		= x_right - 3;
   int	y1		= 3;
   int	y2		= y_bottom - 3;

   /* horizontal lines */
   (void)pr_vector(pr, x1, y_top, x2, y_top, PIX_SRC, 1);
   (void)pr_vector(pr, x1, y_top + 1, x2, y_top + 1, PIX_SRC, 1);

   (void)pr_vector(pr, x1, y_bottom, x2, y_bottom, PIX_SRC, 1);
   (void)pr_vector(pr, x1, y_bottom - 1, x2, y_bottom - 1, PIX_SRC, 1);

   /* vertical lines */
   (void)pr_vector(pr, x_left, y1, x_left, y2, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 1, y1, x_left + 1, y2, PIX_SRC, 1);

   (void)pr_vector(pr, x_right, y1, x_right, y2, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 1, y1, x_right - 1, y2, PIX_SRC, 1);

   /* left corners */
   (void)pr_vector(pr, x_left,     y1,     x1,     y_top, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 1, y1,     x1 + 1, y_top, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 1, y1 + 1, x1 + 2, y_top, PIX_SRC, 1);

   (void)pr_vector(pr, x_left,     y2,     x1,     y_bottom, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 1, y2,     x1 + 1, y_bottom, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 1, y2 - 1, x1 + 2, y_bottom, PIX_SRC, 1);

   /* right corners */
   (void)pr_vector(pr, x_right,     y1,     x2,     y_top, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 1, y1,     x2 - 1, y_top, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 1, y1 + 1, x2 - 2, y_top, PIX_SRC, 1);

   (void)pr_vector(pr, x_right,     y2,     x2,     y_bottom, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 1, y2,     x2 - 1, y_bottom, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 1, y2 - 1, x2 - 2, y_bottom, PIX_SRC, 1);
}

/* below draws a double, then single outlined button image */
static void
alert_yes_outline_button(pr)
register Pixrect *pr;
{
   int	x_left		= 0;
   int	x_right		= pr->pr_size.x - 1;
   int	y_top		= 0;
   int	y_bottom	= pr->pr_size.y - 1;
   int	x1		= 3;
   int	x2		= x_right - 3;
   int	y1		= 3;
   int	y2		= y_bottom - 3;

   /* top horizontal lines */
   (void)pr_vector(
	pr, x1,   y_top,     x2,   y_top,     PIX_SRC, 1); /*top, single*/
   (void)pr_vector(
	pr, x1,   y_top+1,   x2,   y_top+1,   PIX_SRC, 1); /*no blank line*/
   (void)pr_vector(
	pr, x1+1, y_top + 2, x2-1, y_top + 2, PIX_SRC, 1);/*1st dbl line*/
   (void)pr_vector(
	pr, x1+1, y_top + 3, x2-1, y_top + 3, PIX_SRC, 1);/*2nd dbl line*/

   /* bottom horizontal lines */
   (void)pr_vector(
	pr, x1,   y_bottom,     x2,   y_bottom,     PIX_SRC, 1); /*bottom single*/
   (void)pr_vector(
	pr, x1,   y_bottom - 1, x2,   y_bottom - 1, PIX_SRC, 1); /*no blank line*/
   (void)pr_vector(	
	pr, x1+1, y_bottom - 2, x2-1, y_bottom - 2, PIX_SRC, 1);/*1st above*/
   (void)pr_vector(
	pr, x1+1, y_bottom - 3, x2-1, y_bottom - 3, PIX_SRC, 1);/*2nd above*/

   /* left vertical lines (left to right)*/
   (void)pr_vector(pr, x_left,     y1, x_left,       y2, PIX_SRC, 1);
   (void)pr_vector(pr, x_left+1,   y1, x_left+1,     y2, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 2, y1+1, x_left + 2, y2-1, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 3, y1+1, x_left + 3, y2-1, PIX_SRC, 1);

   /* right vertical lines (right to left)*/
   (void)pr_vector(pr, x_right,     y1,   x_right,     y2, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 1, y1,   x_right - 1, y2, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 2, y1+1, x_right - 2, y2-1, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 3, y1+1, x_right - 3, y2-1, PIX_SRC, 1);

   /* left top corners */
   (void)pr_vector(pr, x_left,   y1,   x1,   y_top,   PIX_SRC, 1); /*single */
   (void)pr_vector(pr, x_left+1, y1,   x1,   y_top+1, PIX_SRC, 1); 
   (void)pr_vector(pr, x_left+1, y1+1, x1+1, y_top+1, PIX_SRC, 1);
   (void)pr_vector(pr, x_left+2, y1+1, x1+1, y_top+2, PIX_SRC, 1); 
   (void)pr_vector(pr, x_left+3, y1+1, x1+2, y_top+2, PIX_SRC, 1);
   (void)pr_vector(pr, x_left+3, y1+2, x1+3, y_top+2, PIX_SRC, 1);

   /* left bottom corners */
   (void)pr_vector(pr, x_left,   y2,   x1,   y_bottom,   PIX_SRC, 1); /*single */
   (void)pr_vector(pr, x_left+1, y2,   x1,   y_bottom-1, PIX_SRC, 1); 
   (void)pr_vector(pr, x_left+1, y2-1, x1+1, y_bottom-1, PIX_SRC, 1); 
   (void)pr_vector(pr, x_left+2, y2-1, x1+1, y_bottom-2, PIX_SRC, 1);
   (void)pr_vector(pr, x_left+3, y2-1, x1+2, y_bottom-2, PIX_SRC, 1);
   (void)pr_vector(pr, x_left+3, y2-2, x1+3, y_bottom-2, PIX_SRC, 1);

   /* right top corners */
   (void)pr_vector(pr, x_right,   y1,   x2,   y_top,   PIX_SRC, 1); /*single */
   (void)pr_vector(pr, x_right-1, y1,   x2,   y_top+1, PIX_SRC, 1); 
   (void)pr_vector(pr, x_right-1, y1+1, x2-1, y_top+1, PIX_SRC, 1); 
   (void)pr_vector(pr, x_right-2, y1+1, x2-1, y_top+2, PIX_SRC, 1);
   (void)pr_vector(pr, x_right-3, y1+1, x2-2, y_top+2, PIX_SRC, 1);
   (void)pr_vector(pr, x_right-3, y1+2, x2-3, y_top+2, PIX_SRC, 1);

   /* right bottom corners */
   (void)pr_vector(pr, x_right,   y2,   x2,   y_bottom,   PIX_SRC, 1); /*single */
   (void)pr_vector(pr, x_right-1, y2,   x2,   y_bottom-1, PIX_SRC, 1); 
   (void)pr_vector(pr, x_right-1, y2-1, x2-1, y_bottom-1, PIX_SRC, 1); 
   (void)pr_vector(pr, x_right-2, y2-1, x2-1, y_bottom-2, PIX_SRC, 1);
   (void)pr_vector(pr, x_right-3, y2-1, x2-2, y_bottom-2, PIX_SRC, 1);
   (void)pr_vector(pr, x_right-3, y2-2, x2-3, y_bottom-2, PIX_SRC, 1);
}

static void
alert_select_button(pw, button)
    struct pixwin	*pw;
    button_handle	button;
{
    if (!button)
	return; /* just in case this sneaked by above */
    pw_writebackground(pw,
	button->button_rect.r_left, button->button_rect.r_top,
	button->button_rect.r_width, button->button_rect.r_height,
        PIX_NOT(PIX_DST)); /* invert button rectangle */
}

static void
alert_unselect_button(pw, button)
    struct pixwin	*pw;
    button_handle	button;
{
    (void)alert_select_button(pw, button);
}

static button_handle
alert_button_for_event(alert, event)
    register alert_handle	alert;
    Event			*event;
{
    button_handle	curr;
    int			x, y;
    int			action = event_action(event);

    if (alert->button_info == NULL)
	return (NULL);

    if ((action != MS_LEFT) && (action != LOC_DRAG))
	return (NULL);

    x = event->ie_locx; y = event->ie_locy;
    for (curr = alert->button_info; curr; curr = curr->next) {
	if (       (x >= curr->button_rect.r_left)
		&& (x <= (curr->button_rect.r_left
		    + curr->button_rect.r_width))
		&& (y >= curr->button_rect.r_top)
		&& (y <= (curr->button_rect.r_top
		    + curr->button_rect.r_height)) ) { 
	    return (curr);
	}
    }
    return ((button_handle) 0);
}

static void
alert_do_bell(alert, fd)
    alert_handle	alert;
    int			fd;
{
    struct timeval wait;

    if (!alert_use_audible_bell)
	return;
	
    wait.tv_sec = 0;
    wait.tv_usec = 100000;

    if (!alert->dont_beep && (alert->beeps > 0)) {
	int i = alert->beeps;
	int cmd, kbdfd;
	struct screen screen;

	(void)win_screenget(fd, &screen);
	if ((kbdfd = open(screen.scr_kbdname, O_RDWR, 0)) < 0)
	    return;
    	while (i--) {
	    cmd = KBD_CMD_BELL;
	    (void) ioctl(kbdfd, KIOCCMD, &cmd);
	    (void)alert_blocking_wait(wait);
	    cmd = KBD_CMD_NOBELL;
	    (void) ioctl(kbdfd, KIOCCMD, &cmd);
	}
	(void)close(kbdfd);
    }

}

/* below stolen from win_bell */
static void
alert_blocking_wait(wait_tv)
    struct timeval wait_tv;
{
    extern struct timeval ndet_tv_subt(); /* From notifier code */
    struct timeval start_tv, now_tv, waited_tv;
    int bits;

    /* Get starting time */
    (void)gettimeofday(&start_tv, (struct timezone *)0);
    /* Wait */
    while (timerisset(&wait_tv)) {
    	/* Wait for awhile in select */
    	bits = 0;
    	(void) select(0, &bits, &bits, &bits, &wait_tv);
    	/* Get current time */
    	(void)gettimeofday(&now_tv, (struct timezone *)0);
    	/* Compute how long waited */
    	waited_tv = ndet_tv_subt(now_tv, start_tv);
    	/* Subtract time waited from time left to wait */
    	wait_tv = ndet_tv_subt(wait_tv, waited_tv);
    }
}

#endif ecd.panel
