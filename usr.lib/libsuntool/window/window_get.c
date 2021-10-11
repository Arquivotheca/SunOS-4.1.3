#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)window_get.c 1.1 92/07/30 Copyright 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*-
	WINDOW wrapper

	window_get.c, Sun Aug 8 15:38:39 1985

 */

/* ------------------------------------------------------------------------- */

#include <stdio.h>

#include <sys/types.h>
#include <sys/time.h>

#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>

#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_struct.h>

#include <suntool/tool_struct.h>

#include <suntool/window_impl.h>

/* ------------------------------------------------------------------------- */

/* 
 * Public
 */


/* 
 * Package private
 */

Pkg_private caddr_t window_get();

Pkg_extern int win_appeal_to_owner();


/* 
 * Private
 */

Private int get_mask_bit();


/* ------------------------------------------------------------------------- */


short win_getheight();
short win_getwidth();


/*VARARGS2*/
Pkg_private caddr_t
window_get(window, attr, d1, d2, d3, d4, d5)
	Window window;
	Window_attribute attr;
{   
    register caddr_t v = 0;
    int vp;
    struct window *owner;
    static struct inputmask input_mask; /* Share static data below */
    register struct window *win;
    static Win_alarm alarm;
    
    win = client_to_win(window);
    if (!win) return v;
    
    if (ATTR_PKG_WIN != ATTR_PKG(attr))
	return
	    win->get_proc
		? (win->get_proc)(win->object, attr, d1, d2, d3, d4, d5)
		: (caddr_t)0;
	
    switch (attr) {

      case WIN_CLIENT_DATA:
	v = (caddr_t)win->client_data;
	break;

      case WIN_COLUMNS:
	(void)win_appeal_to_owner(FALSE, win, (caddr_t)WIN_GET_WIDTH, (caddr_t)&vp);
	vp -= win->left_margin + win->right_margin;
	v = (caddr_t)(vp / (actual_column_width(win) + win->column_gap));
	break;

      case WIN_COMPATIBILITY:
	v = (caddr_t)!win->well_behaved;
	break;
	  
      case WIN_CREATED:
	v = (caddr_t)win->created;
	break;	    

      case WIN_CURSOR:
	{   
	    static caddr_t cursor;	/* Let default to NULL */
	    char *cursor_create();
	    
	    if (!cursor) cursor = (caddr_t)cursor_create((char *)0);
	    (void)win_getcursor(win->fd, (struct cursor *)(LINT_CAST(cursor)));
	    v = (caddr_t)cursor;
	}
	break;

      case WIN_DEVICE_NAME:
	{
	    static char	name[WIN_NAMESIZE];

	    (void)win_fdtoname(win->fd, name);
	    v = (caddr_t) name;
	    break;
	}

      case WIN_DEVICE_NUMBER:
	v = (caddr_t)win_fdtonumber(win->fd);
	break;
      
      case WIN_DEFAULT_EVENT_PROC:
      case_win_default_event_proc:
	v = (caddr_t) (LINT_CAST(win->default_event_proc));
	break;
      
      case WIN_EVENT_PROC:
	v = (caddr_t)(LINT_CAST(win->event_proc));
	if (!v) goto case_win_default_event_proc;
	break;
	
      case WIN_FD:
	v = (caddr_t)win->fd;
	break;

      case WIN_FIT_HEIGHT:
	if (!win->get_proc) break;
	v = (win->get_proc)(win->object, WIN_FIT_HEIGHT);
	if (v < 0) goto case_win_height;
	break;
      
      case WIN_FIT_WIDTH:
	if (!win->get_proc) break;
	v = (win->get_proc)(win->object, WIN_FIT_WIDTH);
	if (v < 0) goto case_win_width;
	break;
      
      case WIN_FONT:
	v = (caddr_t)win->font;
	break;

      case WIN_GET_PROC:
	v = (caddr_t)(LINT_CAST(win->get_proc));
	break;

      case WIN_HEIGHT:
      case_win_height:
	(void)win_appeal_to_owner(FALSE, win, (caddr_t)WIN_GET_HEIGHT, (caddr_t)&vp);
	v = (caddr_t)vp;  /* Move to register */
	break;

      case WIN_HORIZONTAL_SCROLLBAR:
	if (win->get_proc) v = (win->get_proc)(win->object, attr);
	break;

      case WIN_IMPL_DATA:
	v = (caddr_t)win->impl_data;
	break;

      case WIN_LAYOUT_PROC:
	v = (caddr_t)(LINT_CAST(win->layout_proc));
	break;
	
      case WIN_MENU:
	v = win->menu;
	break;

      case WIN_MOUSE_XY: /* ?? */
	break;

      case WIN_NAME:
	v = (caddr_t)win->name;
	break;

      case WIN_NOTIFY_DESTROY_PROC:
	v = (caddr_t)(LINT_CAST(
		notify_get_destroy_func(win->object)));
	break;

      case WIN_NOTIFY_EVENT_PROC:
	v = (caddr_t)(LINT_CAST(
		notify_get_event_func(win->object, NOTIFY_SAFE)));
	break;

      case WIN_OBJECT:
	v = (caddr_t)win->object;
	break;

      case WIN_OWNER:
	if (win->owner) v = (caddr_t)win->owner->object;
	break;

      case WIN_PERCENT_HEIGHT:
	owner = win->owner ? win->owner : win;
	v = (caddr_t)
	    ((win_getheight(win->fd) + owner->column_gap) * 100 /
	     ((int)window_get(owner->object, WIN_HEIGHT)
	      - owner->top_margin - owner->bottom_margin - TOOL_BORDERWIDTH));
	break;
	
      case WIN_PERCENT_WIDTH:
	v = (caddr_t)
	    ((win_getwidth(win->fd) + owner->row_gap) * 100 /
	     ((int)window_get(owner->object, WIN_WIDTH)
	      - owner->left_margin - owner->right_margin));
	break;
	
      case WIN_PIXWIN:
	v = (caddr_t)win->pixwin;
	break;

      case WIN_PRESET_PROC:
	v = (caddr_t)(LINT_CAST(win->preset_proc));
	break;

      case WIN_POSTSET_PROC:
	v = (caddr_t)(LINT_CAST(win->postset_proc));
	break;

      case WIN_RECT:
	{ 
	    static Rect rec;
	    (void)win_appeal_to_owner(FALSE, win, (caddr_t)WIN_GET_RECT, (caddr_t)&rec);
	    v = (caddr_t)&rec;
	}
	break;

      case WIN_ROWS:
	(void)win_appeal_to_owner(FALSE, win, (caddr_t)WIN_GET_HEIGHT, (caddr_t)&vp);
	vp -= win->top_margin + win->bottom_margin;
	v = (caddr_t)(vp / (actual_row_height(win) + win->row_gap));
	break;

      case WIN_SCREEN_RECT:
	{   
	    static struct screen screen;
	    (void)win_screenget(win->fd, &screen);
	    v = (caddr_t)&screen.scr_rect;
	}		
	break;

      case WIN_SET_PROC:
	v = (caddr_t)(LINT_CAST(win->set_proc));
	break;

      case WIN_SHOW:
	v = (caddr_t)win->show;
	
	break;

      case WIN_SHOW_UPDATES:
	v = (caddr_t)win->show_updates;
	break;
	
      case WIN_TYPE:
	v = (caddr_t)win->type;
	break;

      case WIN_VERTICAL_SCROLLBAR:
	if (win->get_proc) v = (win->get_proc)(win->object, attr);
	break;

      case WIN_WIDTH:
      case_win_width:
	(void)win_appeal_to_owner(FALSE, win, (caddr_t)WIN_GET_WIDTH, (caddr_t)&vp);
	v = (caddr_t)vp;  /* Move to register */
	break;

      case WIN_X:
	(void)win_appeal_to_owner(FALSE, win, (caddr_t)WIN_GET_X, (caddr_t)&vp);
	v = (caddr_t)vp;  /* Move to register */
	break;

      case WIN_Y:
	(void)win_appeal_to_owner(FALSE, win, (caddr_t)WIN_GET_Y, (caddr_t)&vp);
	v = (caddr_t)vp;  /* Move to register */
	break;

      case WIN_KBD_INPUT_MASK:
	(void)win_get_kbd_mask(win->fd, &input_mask);
	v = (caddr_t)&input_mask;
	break;

      case WIN_PICK_INPUT_MASK:
	(void)win_get_pick_mask(win->fd, &input_mask);
	v = (caddr_t)&input_mask;
	break;

      case WIN_TOP_MARGIN:
	v = (caddr_t)win->top_margin;
	break;	    
	    
      case WIN_BOTTOM_MARGIN:
	v = (caddr_t)win->bottom_margin;
	break;	    
	    
      case WIN_LEFT_MARGIN:
	v = (caddr_t)win->left_margin;
	break;	    
	    
      case WIN_RIGHT_MARGIN:
	v = (caddr_t)win->right_margin;
	break;	    
	    
      case WIN_ROW_HEIGHT:
	v = (caddr_t)actual_row_height(win);
	break;	    
	    
      case WIN_COLUMN_WIDTH:
	v = (caddr_t)actual_column_width(win);
	break;	    
	    
      case WIN_ROW_GAP:
	v = (caddr_t)win->row_gap;
	break;	    
	    
      case WIN_COLUMN_GAP:
	v = (caddr_t)win->column_gap;
	break;	    
	    
      /* added by thoeber on 10/4/85 */
      case WIN_EVENT_STATE:
	v = (caddr_t)win_get_vuid_value(win->fd, (unsigned short) d1);
	break;

      /* added by thoeber on 10/4/85 */
      case WIN_KBD_FOCUS:
	if (win_get_kbd_focus(win->fd) == win_fdtonumber(win->fd))
	   v = (caddr_t)TRUE;
	else
	   v = (caddr_t)FALSE;
	break;

      /* added by thoeber on 10/9/85 */
      case WIN_CONSUME_KBD_EVENT:
        {
	   struct inputmask mask;
	   (void)win_get_kbd_mask(win->fd, &mask);
	   v = (caddr_t)get_mask_bit(&mask, (Window_input_event)d1, win->fd);
	   break;
	}

      /* added by thoeber on 10/9/85 */
      case WIN_CONSUME_PICK_EVENT:
        {
	   struct inputmask mask;
	   (void)win_get_pick_mask(win->fd, &mask);
	   v = (caddr_t)get_mask_bit(&mask, (Window_input_event)d1, win->fd);
	   break;
	}
	break;

      case WIN_ALARM:
 
        win_get_alarm(&alarm);
        v = (caddr_t) &alarm;
        break;

      default:
	if (ATTR_PKG_WIN == ATTR_PKG(attr))
	    (void)fprintf(stderr,
	"window_get: Window attribute not allowed.\n%s\n",
	            attr_sprint((char *)NULL, (unsigned)attr));
	    break;

    }
    return v;
}


Private int 
get_mask_bit(mask, code, fd)
	Inputmask		 *mask;
	Window_input_event	 code;
	int fd;
{
   int v;

   switch (code) {

      case WIN_NO_EVENTS:
	 v = (mask == 0);
	 break;

      case WIN_ASCII_EVENTS:
         v = mask->im_flags & (IM_ASCII | IM_META);
	 break;

      case WIN_UP_ASCII_EVENTS:
         v = mask->im_flags & (IM_NEGASCII | IM_NEGMETA);
	 break;

      case WIN_EUC_EVENTS:
         v = mask->im_flags & (IM_EUC | IM_META);
	 break;

      case WIN_UP_EUC_EVENTS:
         v = mask->im_flags & (IM_NEGEUC | IM_NEGMETA);
	 break;

      case WIN_UP_EVENTS:
         v = mask->im_flags & IM_NEGEVENT;
	 break;

      case WIN_MOUSE_BUTTONS:
	 v = (win_getinputcodebit(mask, MS_LEFT) &&
	      win_getinputcodebit(mask, MS_MIDDLE) &&
	      win_getinputcodebit(mask, MS_RIGHT)
	     );
	 break;

      case WIN_LEFT_KEYS:
	 v = (win_getinputcodebit(mask, KEY_LEFT(1)) &&
	      win_getinputcodebit(mask, KEY_LEFT(2)) &&
	      win_getinputcodebit(mask, KEY_LEFT(3)) &&
	      win_getinputcodebit(mask, KEY_LEFT(4)) &&
	      win_getinputcodebit(mask, KEY_LEFT(5)) &&
	      win_getinputcodebit(mask, KEY_LEFT(6)) &&
	      win_getinputcodebit(mask, KEY_LEFT(7)) &&
	      win_getinputcodebit(mask, KEY_LEFT(8)) &&
	      win_getinputcodebit(mask, KEY_LEFT(9)) &&
	      win_getinputcodebit(mask, KEY_LEFT(10)) &&
	      win_getinputcodebit(mask, KEY_LEFT(11)) &&
	      win_getinputcodebit(mask, KEY_LEFT(12)) &&
	      win_getinputcodebit(mask, KEY_LEFT(13)) &&
	      win_getinputcodebit(mask, KEY_LEFT(14)) &&
	      win_getinputcodebit(mask, KEY_LEFT(15))
	     );
	 break;

      case WIN_RIGHT_KEYS:
	 v = (win_getinputcodebit(mask, KEY_RIGHT(1)) &&
	      win_getinputcodebit(mask, KEY_RIGHT(2)) &&
	      win_getinputcodebit(mask, KEY_RIGHT(3)) &&
	      win_getinputcodebit(mask, KEY_RIGHT(4)) &&
	      win_getinputcodebit(mask, KEY_RIGHT(5)) &&
	      win_getinputcodebit(mask, KEY_RIGHT(6)) &&
	      win_getinputcodebit(mask, KEY_RIGHT(7)) &&
	      win_getinputcodebit(mask, KEY_RIGHT(8)) &&
	      win_getinputcodebit(mask, KEY_RIGHT(9)) &&
	      win_getinputcodebit(mask, KEY_RIGHT(10)) &&
	      win_getinputcodebit(mask, KEY_RIGHT(11)) &&
	      win_getinputcodebit(mask, KEY_RIGHT(12)) &&
	      win_getinputcodebit(mask, KEY_RIGHT(13)) &&
	      win_getinputcodebit(mask, KEY_RIGHT(14)) &&
	      win_getinputcodebit(mask, KEY_RIGHT(15))
	     );
	 break;

      case WIN_TOP_KEYS:
	 v = (win_getinputcodebit(mask, KEY_TOP(1)) &&
	      win_getinputcodebit(mask, KEY_TOP(2)) &&
	      win_getinputcodebit(mask, KEY_TOP(3)) &&
	      win_getinputcodebit(mask, KEY_TOP(4)) &&
	      win_getinputcodebit(mask, KEY_TOP(5)) &&
	      win_getinputcodebit(mask, KEY_TOP(6)) &&
	      win_getinputcodebit(mask, KEY_TOP(7)) &&
	      win_getinputcodebit(mask, KEY_TOP(8)) &&
	      win_getinputcodebit(mask, KEY_TOP(9)) &&
	      win_getinputcodebit(mask, KEY_TOP(10)) &&
	      win_getinputcodebit(mask, KEY_TOP(11)) &&
	      win_getinputcodebit(mask, KEY_TOP(12)) &&
	      win_getinputcodebit(mask, KEY_TOP(13)) &&
	      win_getinputcodebit(mask, KEY_TOP(14)) &&
	      win_getinputcodebit(mask, KEY_TOP(15))
	     );
	 break;

      case WIN_BOTTOM_KEYS:
	 v = (win_getinputcodebit(mask, KEY_BOTTOM(1)) &&
	      win_getinputcodebit(mask, KEY_BOTTOM(2)) &&
	      win_getinputcodebit(mask, KEY_BOTTOM(3)) &&
	      win_getinputcodebit(mask, KEY_BOTTOM(4)) &&
	      win_getinputcodebit(mask, KEY_BOTTOM(5)) &&
	      win_getinputcodebit(mask, KEY_BOTTOM(6)) &&
	      win_getinputcodebit(mask, KEY_BOTTOM(7)) &&
	      win_getinputcodebit(mask, KEY_BOTTOM(8)) &&
	      win_getinputcodebit(mask, KEY_BOTTOM(9)) &&
	      win_getinputcodebit(mask, KEY_BOTTOM(10)) &&
	      win_getinputcodebit(mask, KEY_BOTTOM(11)) &&
	      win_getinputcodebit(mask, KEY_BOTTOM(12)) &&
	      win_getinputcodebit(mask, KEY_BOTTOM(13)) &&
	      win_getinputcodebit(mask, KEY_BOTTOM(14)) &&
	      win_getinputcodebit(mask, KEY_BOTTOM(15))
	     );
	 break;

      case WIN_IN_TRANSIT_EVENTS:
         v = mask->im_flags & IM_INTRANSIT;
	 break;

      default: 
         if (isworkstationdevid((int) code))
            v = win_getinputcodebit(mask, (int) code);
         else
            v = win_keymap_get_smask(fd, (u_short) code);
         break;

   }
   return (v != 0);
}
