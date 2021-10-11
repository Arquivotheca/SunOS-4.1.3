#ifndef lint
#ifdef sccs
static        char sccsid[] = "@(#)frame_shadow.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 *  frame_shadow: Create frame shadow
 */

/* ------------------------------------------------------------------------- */

#include <stdio.h>

#include <sys/time.h>
#include <sys/types.h>

#include <pixrect/pixrect.h>

#include <sunwindow/attr.h>
#include <sunwindow/notify.h>
#include  <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>

#include <suntool/tool_struct.h>

#include <suntool/frame.h>
#include <suntool/window.h>
#include <suntool/wmgr.h>
#include <suntool/walkmenu.h>

#include "suntool/tool_impl.h"
#include "suntool/frame_impl.h"


/* ------------------------------------------------------------------------- */

#define SHADOW_X_OFFSET		6
#define SHADOW_Y_OFFSET		6

extern struct pixrect menu_gray50_pr;

static void repaint_shadow(), resize_shadow();
static Notify_value s_event_proc(), f_event_proc();

Pkg_private Frame
shadow_frame(frame)
    Frame frame;
{   
    Frame  shadow = window_create(frame, 	FRAME, 
    			   WIN_WIDTH,		0,
			   FRAME_SHOW_SHADOW, 	FALSE, 
			   0);

    if (shadow != NULL) {			
	((struct toolplus *)(LINT_CAST(frame)))->shadow = shadow;
	((struct toolplus *)(LINT_CAST(frame)))->has_active_shadow = TRUE;
	notify_interpose_event_func(frame, f_event_proc, NOTIFY_SAFE);
	notify_interpose_event_func(shadow, s_event_proc, NOTIFY_SAFE);
    }
    else {
	((struct toolplus *)(LINT_CAST(frame)))->shadow = NULL;
	((struct toolplus *)(LINT_CAST(frame)))->has_active_shadow = FALSE;
	notify_interpose_event_func(frame, f_event_proc, NOTIFY_SAFE);
    }
    return(shadow);
}


static Notify_value
f_event_proc(frame, event, arg, type)
    Frame frame;
    Event *event;
    Notify_arg arg;
    Notify_event_type type;
{
    Notify_value value = notify_next_event_func(frame, event, arg, type);

    switch (event_action(event)) {
      case WIN_REPAINT:
      	repaint_shadow(frame);
      	break;
      case MS_MIDDLE:
      case MS_RIGHT:
      case WIN_RESIZE:
	resize_shadow(frame);
	break;
    }
    return value;
}

static Notify_value
s_event_proc(shadow, event, arg, type)
    Frame shadow;
    Event *event;
    Notify_arg arg;
    Notify_event_type type;
{

    switch (event_action(event)) {
      case WIN_REPAINT:
	repaint_shadow(window_get(shadow, WIN_OWNER));
	break;
      case MS_LEFT:
      case MS_MIDDLE:
      case MS_RIGHT:
	notify_post_event_and_arg(window_get(shadow, WIN_OWNER), 
				  event, type, arg, NULL, NULL);
	break;
    }
    return NOTIFY_DONE;
}

static void
resize_shadow(frame)
    Frame	frame;
{    
    register Frame shadow;
    
    if (!window_get(frame,FRAME_SHOW_SHADOW, 0))
        return;
    
    if ((shadow = (Frame)window_get(frame, FRAME_SHADOW)) == NULL)
	return;  
    
    if (window_get(frame, FRAME_CLOSED) || !window_get(frame, WIN_SHOW)) {
	window_set(shadow, WIN_WIDTH, 0, 0);
    } else {
	Rect 	fr, sr;
	int	f_fd = window_fd(frame);
	int	f_wn = win_fdtonumber(f_fd);
	int	s_fd = window_fd(shadow);

	fr = *(Rect *)window_get(frame, WIN_RECT);
	sr = *(Rect *)window_get(shadow, WIN_RECT);
	fr.r_left = 0;
        fr.r_top = 0;
	sr.r_left -= SHADOW_X_OFFSET;
        sr.r_top -= SHADOW_Y_OFFSET;
	if (!rect_equal(&fr, &sr) ||
	    win_getlink(s_fd, WL_YOUNGERSIB) != f_wn) {
	    
	    fr.r_left += SHADOW_X_OFFSET;
	    fr.r_top += SHADOW_Y_OFFSET;
	    window_set(shadow, WIN_RECT, &fr, WIN_SHOW, TRUE, 0);

	    wmgr_setlevel(s_fd, win_getlink(f_fd, WL_OLDERSIB), f_wn);
	}
	
    }
}

static void
repaint_shadow(frame)
    Frame 	frame;
{   
    register Frame shadow;
    
    if (!window_get(frame,FRAME_SHOW_SHADOW, 0))
        return;
    
    if ((shadow = (Frame)window_get(frame, FRAME_SHADOW)) == NULL)
	return;  
    
    if (window_get(frame, FRAME_CLOSED) || !window_get(frame, WIN_SHOW)) {
	window_set(shadow, WIN_SHOW, FALSE, WIN_WIDTH, 0, 0);
    } else {
	Pixwin 		*pw;
	Rect 		sr;
	int		s_fd = window_fd(shadow);
	
	resize_shadow(frame);
	
	pw = (Pixwin *)window_get(shadow, WIN_PIXWIN);
	sr = *(Rect *)window_get(shadow, WIN_RECT);	    
	pw_replrop(pw, 0, 0, sr.r_width, sr.r_height, 
		   PIX_SRC, &menu_gray50_pr, 
		   0, 0);
		   
	win_lockdata(s_fd);    
	win_getrect(s_fd, &sr);
	win_partialrepair(s_fd, &sr); 
	win_unlockdata(s_fd); 	

    }
}

#ifdef debug
print_hierarchy(msg, f_fd, s_fd)
    char *msg;
{   
    printf("%s:\n", msg);
    printf("frame:  [%d] P= %d, Y= %d, O= %d\n", 
	   win_fdtonumber(f_fd), 
	   win_getlink(f_fd, WL_PARENT),
	   win_getlink(f_fd, WL_YOUNGERSIB), 
	   win_getlink(f_fd, WL_OLDERSIB));
    printf("shadow: [%d] P= %d, Y= %d, O= %d\n", 
	   win_fdtonumber(s_fd), 
	   win_getlink(s_fd, WL_PARENT),
	   win_getlink(s_fd, WL_YOUNGERSIB), 
	   win_getlink(s_fd, WL_OLDERSIB));
}
#endif debug
