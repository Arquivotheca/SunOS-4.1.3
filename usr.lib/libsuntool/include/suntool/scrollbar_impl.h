/*      @(#)scrollbar_impl.h 1.1 92/07/30 SMI      */

#ifndef scrollbar_impl_defined
#define scrollbar_impl_defined

/*************************************************************************/
/*                         scrollbar_impl.h                              */
/*           Copyright (c) 1985 by Sun Microsystems, Inc.                */
/*************************************************************************/

#include <stdio.h>
#include <sundev/kbd.h>
#include <suntool/tool_hs.h>
#include <suntool/help.h>
#include <suntool/scrollbar.h>

/*************************************************************************/
/* scrollbar struct                                                      */
/*************************************************************************/

typedef struct scrollbar *scrollbar_handle;
typedef struct cursor    *cursor_handle;
typedef struct pixwin    *pixwin_handle;

struct scrollbar {
   caddr_t			notify_client;
   int				client_windowfd;
   pixwin_handle		pixwin;
   Rect				rect;
   caddr_t			object;
   long unsigned		object_length;
   long unsigned		view_start;
   long unsigned		view_length;
   long unsigned		old_view_start;
   Scroll_motion 		request_motion;
   long unsigned		request_offset;
   long unsigned		undo_mark;
   long unsigned		undo_request_offset;
   long unsigned		delay;
   Scrollbar_setting		bar_display_level;
   Scrollbar_setting		bubble_display_level;
   Scrollbar_setting            gravity;
   unsigned			bubble_margin;
   unsigned			normalize_margin;
   unsigned			gap;
   unsigned 			button_length;
   unsigned 			saved_button_length;
   unsigned			line_height;
   cursor_handle		forward_cursor;
   cursor_handle		backward_cursor;
   cursor_handle		absolute_cursor;
   cursor_handle		active_cursor;
   scrollbar_handle		next;
   int                        (*paint_buttons_proc)(); 
   int                        (*modify)(); 
   int				end_point_area;
   caddr_t			help_data;
   unsigned buttons		: 1;
   unsigned horizontal		: 1;
   unsigned advanced_mode	: 1;
   unsigned bubble_grey		: 1;
   unsigned bar_grey		: 1;
   unsigned border              : 1;
   unsigned rect_fixed		: 1;
   unsigned one_button_case	: 1;
   unsigned normalize		: 1;
   unsigned use_grid		: 1;
   unsigned bar_painted		: 1;
   unsigned bubble_painted	: 1;
   unsigned buttons_painted	: 1;
   unsigned border_painted	: 1;
   unsigned bubble_modified	: 1;
   unsigned attrs_modified	: 1;
   unsigned direction_not_yet_set	: 1;
   unsigned gravity_not_yet_set	: 1;
};

/*************************************************************************/
/* #defines                                                              */
/*************************************************************************/

#define SCROLLBAR_NULL_HANDLE			((scrollbar_handle) 0)
#define SCROLLBAR_INVALID_LENGTH		0xFFFFFFFF
#define SCROLLBAR_DEFAULT_PAGE_BUTTON_LENGTH	15
#define MIN_BUBBLE_LENGTH			4
#define SCROLLBAR_DEFAULT_END_POINT_AREA	6
#define SCROLLBAR_DEFAULT_REPEAT_TIME		10


#define SCROLLBAR_FOR_ALL(formal) 		\
		for (formal=scrollbar_head_sb; formal; formal=formal->next)
					
#define SCROLLBAR_INSIDE  	1	/* argument to scrollbar_repaint() */
#define SCROLLBAR_OUTSIDE 	2	/* argument to scrollbar_repaint() */

/*************************************************************************/
/* flag values                                                           */
/*************************************************************************/

#define bar_always(sb)		((sb)->bar_display_level == SCROLL_ALWAYS)
#define bar_active(sb)		((sb)->bar_display_level == SCROLL_ACTIVE)
#define bar_never(sb)		((sb)->bar_display_level == SCROLL_NEVER)
#define bubble_always(sb)	((sb)->bubble_display_level == SCROLL_ALWAYS)
#define bubble_active(sb)	((sb)->bubble_display_level == SCROLL_ACTIVE)
#define bubble_never(sb)	((sb)->bubble_display_level == SCROLL_NEVER)

/*************************************************************************/
/* external declarations of private functions                            */
/*************************************************************************/

extern int          scrollbar_nullproc();
extern int          scrollbar_compute_bubble_rect();
extern int          scrollbar_paint_buttons();
extern int          scrollbar_repaint();
extern Notify_value scrollbar_event();
extern Notify_value scrollbar_timed_out();

/*************************************************************************/
/* global variables                                                      */
/*************************************************************************/

extern struct itimerval scrollbar_timer;
extern scrollbar_handle scrollbar_active_sb, scrollbar_head_sb;
extern struct cursor    scrollbar_client_saved_cursor, 
			scrollbar_bar_saved_cursor;


#endif
