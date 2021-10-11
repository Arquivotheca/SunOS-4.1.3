#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)scrollbar_event.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
#endif

/*************************************************************************/
/*                         scrollbar_event.c                             */
/*           Copyright (c) 1985 by Sun Microsystems, Inc.                */
/*************************************************************************/

#include <suntool/scrollbar_impl.h> 

#define FORWARD_BUTTON		1
#define BACKWARD_BUTTON		2
#define NO_BUTTON 		3

static void		scrollbar_resize();
static void		post_scroll();
static void		compute_new_view_start();
static int		find_region();
static void		set_cursor_from_mouse();
static Notify_value	scrollbar_timer_proc();
static void		scrollbar_timer_start();
static void		scrollbar_timer_stop();

/**************************************************************************/
/* cursors to cache cursor images                                         */
/**************************************************************************/

extern	struct cursor scrollbar_client_saved_cursor;

/**************************************************************************/
/* timer declarations                                                     */
/**************************************************************************/

struct itimerval scrollbar_timer;
int		 scrollbar_t_on;
Event            scrollbar_t_event;
Scroll_motion    scrollbar_t_motion;

/**************************************************************************/
/* scrollbar_event                                                        */
/**************************************************************************/

static short button_active_cursor_image[16] = {
/* black diamond */
        0x0000,0x0000,0x0000,0x0180,0x03C0,0x07E0,0x0FF0,0x1FF8,
        0x1FF8,0x0FF0,0x07E0,0x03C0,0x0180,0x0000,0x0000,0x0000
};
mpr_static(scrollbar_button_active_mpr, 16, 16, 1, button_active_cursor_image);
struct cursor scrollbar_button_active_cursor = 
   {5,7,PIX_SRC|PIX_DST,&scrollbar_button_active_mpr};


/*ARGSUSED*/
Notify_value
scrollbar_event(sb, event, arg, type)
scrollbar_handle   sb;
Event             *event;
caddr_t		   arg;
Notify_event_type  type;
{
   register Scroll_motion   motion;
   struct cursor           *cursor = NULL;
   struct cursor           *active_cursor;
   int                      undoing = FALSE;
   
   if (find_region(sb, event)==NO_BUTTON)
      active_cursor = sb->active_cursor;
   else
      active_cursor = &scrollbar_button_active_cursor;

   switch (event_action(event)) {
          
      case WIN_REPAINT:
         (void)scrollbar_paint_clear((Scrollbar)(LINT_CAST(sb)));
         return NOTIFY_DONE;
          
      case WIN_RESIZE:
         scrollbar_resize((Scrollbar)(LINT_CAST(sb)));
         return NOTIFY_DONE;
        
      case LOC_RGNENTER:
         /* Always fetch client's cursor */
	 (void)win_getcursor(sb->client_windowfd, &scrollbar_client_saved_cursor);

         set_cursor_from_mouse(sb, active_cursor);

         (void)scrollbar_repaint((Scrollbar)(LINT_CAST(sb)), SCROLLBAR_INSIDE);
         scrollbar_active_sb = sb;
	 (void)win_post_id_and_arg(sb->notify_client, SCROLL_ENTER, NOTIFY_SAFE, 
			     (char *)sb, NOTIFY_COPY_NULL, NOTIFY_RELEASE_NULL);
         return NOTIFY_DONE;

      case LOC_RGNEXIT:
         /* Always restore client's cursor */
	 (void)win_setcursor(sb->client_windowfd, &scrollbar_client_saved_cursor);
         (void)scrollbar_repaint((Scrollbar)(LINT_CAST(sb)), SCROLLBAR_OUTSIDE);
         scrollbar_active_sb = NULL;
	 (void)win_post_id_and_arg(sb->notify_client, SCROLL_EXIT, NOTIFY_SAFE,
	 		(char *)sb, NOTIFY_COPY_NULL, NOTIFY_RELEASE_NULL);
         scrollbar_timer_stop(sb);
         return NOTIFY_DONE;
       
      case LOC_MOVE:
         if (scrollbar_t_on) {
            scrollbar_t_event.ie_locx = event->ie_locx;
            scrollbar_t_event.ie_locy = event->ie_locy;
         } else
            set_cursor_from_mouse(sb, active_cursor);
         return NOTIFY_DONE;
       
      case KBD_REQUEST:
	 /* Scrollbar always refuses kbd focus request */
	 (void)win_refuse_kbd_focus(sb->client_windowfd);
         scrollbar_timer_stop(sb);
         return NOTIFY_DONE;
       
      case MS_LEFT:
         cursor = sb->forward_cursor;
         switch (find_region(sb, event)) {
            case FORWARD_BUTTON:
               motion = SCROLL_LINE_FORWARD;
               break;
            case BACKWARD_BUTTON:
               motion = SCROLL_LINE_FORWARD;
               break;
            case NO_BUTTON:
               motion = event_shift_is_down(event) ?
                        SCROLL_MAX_TO_POINT : SCROLL_POINT_TO_MIN;
               break;
         }
         break;
         
      case MS_MIDDLE:
         cursor = sb->absolute_cursor;
         switch (find_region(sb, event)) {
            case FORWARD_BUTTON:
               motion = event_shift_is_down(event) ?
                        SCROLL_PAGE_BACKWARD : SCROLL_PAGE_FORWARD;
               break;
            case BACKWARD_BUTTON:
               motion = event_shift_is_down(event) ?
                        SCROLL_PAGE_BACKWARD : SCROLL_PAGE_FORWARD;
               break;
            case NO_BUTTON:
               motion = SCROLL_ABSOLUTE;
               undoing = event_shift_is_down(event);
               break;
         }
         break;
         
      case MS_RIGHT:
         cursor = sb->backward_cursor;
         switch (find_region(sb, event)) {
            case FORWARD_BUTTON:
               motion = SCROLL_LINE_BACKWARD;
               break;
            case BACKWARD_BUTTON:
               motion = SCROLL_LINE_BACKWARD;
               break;
            case NO_BUTTON:
               motion = event_shift_is_down(event) ?
                        SCROLL_POINT_TO_MAX : SCROLL_MIN_TO_POINT;
               break;
         }
         break;

      case ACTION_HELP:
	 if (event_is_down(event))
	    help_request((caddr_t)(LINT_CAST(sb->notify_client)),
			 sb->help_data, event);
         return NOTIFY_DONE;

#ifdef notdef
      case KEY_LEFT(4):	/* UNDO */
         if (event_is_up(event))
            post_scroll(sb, event, SCROLL_ABSOLUTE, TRUE);
         return NOTIFY_DONE;
#endif              

      default:  {
	 Rect	rect;
	
	 /* Translate mouse x and y so that movements inside
 	  * the scrollbar are posted with co-ordinates relative
	  * to the window that encloses the scrollbar. 
	  */
	 rect = sb->rect;
 	 event->ie_locx += rect.r_left;
         event->ie_locy += rect.r_top;

 	 (void)win_post_id_and_arg(sb->notify_client, event_id(event), NOTIFY_SAFE, 
			     (char *)sb, NOTIFY_COPY_NULL, NOTIFY_RELEASE_NULL);

	 return NOTIFY_DONE;
	}
   }

   if (win_inputposevent(event)) {
      scrollbar_timer_start(sb, event, motion);
   } else {
      if (!scrollbar_t_on)
         post_scroll(sb, event, motion, undoing);
      cursor = active_cursor;
      scrollbar_timer_stop(sb);
   }

   if (cursor)
      (void)win_setcursor(sb->client_windowfd, cursor);

   return NOTIFY_DONE;
}

static 
find_region(sb, event)
scrollbar_handle   sb;
Event             *event;
{
   register short loc;
   register int   forward_button_limit;
   register int   backward_button_limit;

   if (!sb->buttons)
      return NO_BUTTON;

   if (sb->horizontal) {
      loc = event_x(event);
      forward_button_limit  = sb->button_length;
      backward_button_limit = sb->rect.r_width - sb->button_length;
   } else {
      loc = event_y(event);
      forward_button_limit  = sb->button_length;
      backward_button_limit = sb->rect.r_height - sb->button_length;
   }

   if (loc < forward_button_limit)
      return FORWARD_BUTTON; 
   else if (loc > backward_button_limit)
      return BACKWARD_BUTTON; 
   else
      return NO_BUTTON;
}

static void
set_cursor_from_mouse(sb, active_cursor)
scrollbar_handle	sb;
struct cursor		*active_cursor;
{
   register int	fd = sb->client_windowfd;

   /* figure which cursor to install from mouse state */
   if (win_get_vuid_value(fd, MS_LEFT))
      active_cursor = sb->forward_cursor;
   else if (win_get_vuid_value(fd, MS_MIDDLE))
      active_cursor = sb->absolute_cursor;
   else if (win_get_vuid_value(fd, MS_RIGHT))
      active_cursor = sb->backward_cursor;
   /* ..else keep active_cursor unchanged */

   /* set cursor if not NULL */
   if (active_cursor)
      (void)win_setcursor(fd, active_cursor);
}

/**************************************************************************/
/* post_scroll                                                            */
/* undoing policy: undo to the last position which was departed from      */
/* via an "undoable" motion.  Undoable motions are: thumbing, undoing.    */
/**************************************************************************/

static void
post_scroll(sb, event, motion, undoing)
scrollbar_handle         sb;
Event	                *event;
Scroll_motion            motion;
int                      undoing;
{
   register int bar_length, request_offset, old_request_offset;

   /* compute bar length, offset of request into bar (incl. page buttons) */
   old_request_offset = sb->request_offset;
   if (sb->horizontal) {
      request_offset = event_x(event);
      bar_length = sb->rect.r_width;
   } else {
      request_offset = event_y(event);
      bar_length = sb->rect.r_height;
   }

   /* if thumbing, save request_offset and view_start for future undo */
   if (!undoing && motion == SCROLL_ABSOLUTE) {
      sb->undo_request_offset = sb->request_offset;
      sb->undo_mark           = sb->view_start;

      /* don't scroll if double-thumbing in same spot! */
      if (sb->request_motion==SCROLL_ABSOLUTE
      && request_offset==old_request_offset)
         return;
   }

   sb->request_motion = motion;
   sb->request_offset = request_offset;

   /* if undoing, restore view_start from undo_mark, else compute it */
   /* First save view_start as old_view_start.  When if the newly    */
   /* computed view_start is different, set the bubble_modified flag */
   /* so that the next scrollbar_paint erases the old bubble.        */
   sb->old_view_start = sb->view_start;
   if (undoing) {
      sb->request_offset      = sb->undo_request_offset;
      sb->view_start          = sb->undo_mark;
      sb->undo_mark           = sb->old_view_start;
      sb->undo_request_offset = old_request_offset;
   } else
      compute_new_view_start(sb, bar_length);
   if (sb->view_start != sb->old_view_start)
      sb->bubble_modified = TRUE;

   (void)win_post_id_and_arg(sb->notify_client, SCROLL_REQUEST, NOTIFY_SAFE, 
   		       (char *)sb, NOTIFY_COPY_NULL, NOTIFY_RELEASE_NULL);
}

/**************************************************************************/
/* compute_new_view_start                                                 */
/* In the relative motions, we take into account that the scrollbar's     */
/* view_start, view_length and object_length are in client units, which   */
/* may be other than pixels -- chars or lines, for example.               */
/**************************************************************************/

static void
compute_new_view_start(sb, bar_length)
register scrollbar_handle	sb;
int				bar_length;
{
   int	align_to_max	= FALSE;
   int	scrolling_up	= TRUE;
   int	desired_scroll	= 0;
   int	new_offset 	= sb->view_start; /*note use of int, not unsigned*/
   int	min_offset 	= 0;
   int	max_offset 	= sb->object_length;
   int	margin_offset 	= sb->normalize_margin;
   int	gap_offset 	= sb->gap;
   int	line_offset 	= sb->line_height;

   /* If gap not yet set, use margin. */
   if (gap_offset==SCROLLBAR_INVALID_LENGTH)
      gap_offset = margin_offset;

   switch (sb->request_motion) {

      case SCROLL_ABSOLUTE: {
	 /* factor button lengths out; save bar_length for normalization */
	 int  offset_into_bar = sb->request_offset - sb->button_length;
	 int  bar_size        = bar_length - 2*sb->button_length;
	 int  end_point_area  = sb->end_point_area;
	 int  bubble_top_offset;
	 int  bubble_extent;
	 struct rect   bubble;

	 if (end_point_area > bar_size/2)
	    end_point_area = bar_size/2;
	 
	 /* adjust for top/bottom gravity */
	 if (offset_into_bar <= end_point_area) {
	    new_offset = 0;
	    break;
	 } else if (bar_size - offset_into_bar <= end_point_area) {
	    new_offset = max_offset - (3*(sb->view_length/4));
	 /* desired_scroll = -(sb->line_height); */
	    break;
	 }

	 /* figure where the top of the bubble should go */
	 (void)scrollbar_compute_bubble_rect(sb, &bubble);
	 if (sb->horizontal)
	    bubble_extent = bubble.r_width;
	 else
	    bubble_extent = bubble.r_height;
	 bubble_top_offset = offset_into_bar - (bubble_extent / 2);
	 if (bubble_top_offset < 0)
	    bubble_top_offset = 0;

	 /* finally, compute the new view_start */
	 new_offset = ((double)sb->object_length * bubble_top_offset) /
                       bar_size;
	 break;
      }

      case SCROLL_FORWARD: /* POINT_TO_MIN */
	 desired_scroll = sb->request_offset;
	 break;

      case SCROLL_BACKWARD: /* MIN_TO_POINT */
	 desired_scroll = -(sb->request_offset);
	 scrolling_up = FALSE; 
	 break;

      case SCROLL_MAX_TO_POINT:
         if (!sb->advanced_mode) {
	    sb->request_offset = bar_length - sb->request_offset;
	    sb->request_motion = SCROLL_FORWARD;
         }
	 desired_scroll = bar_length - sb->request_offset;
	 align_to_max = TRUE;
	 break;

      case SCROLL_POINT_TO_MAX:
         if (!sb->advanced_mode) {
	    sb->request_offset = bar_length - sb->request_offset;
	    sb->request_motion = SCROLL_BACKWARD;
         }
	 desired_scroll = -(bar_length - sb->request_offset);
	 align_to_max = TRUE;
	 scrolling_up = FALSE; 
	 break;

      case SCROLL_PAGE_FORWARD:
         if (!sb->advanced_mode) {
	    sb->request_offset = bar_length;
	    sb->request_motion = SCROLL_FORWARD;
         }
	 new_offset += sb->view_length;
	 break;

      case SCROLL_PAGE_BACKWARD:
         if (!sb->advanced_mode) {
	    sb->request_offset = bar_length;
	    sb->request_motion = SCROLL_BACKWARD;
         }
	 new_offset -= sb->view_length;
	 scrolling_up = FALSE; 
	 break;

      case SCROLL_LINE_FORWARD:
	 desired_scroll = sb->line_height;
	 if (sb->use_grid && !new_offset) /* special case for top of object */
	    desired_scroll += (margin_offset - gap_offset);
         if (!sb->advanced_mode) {
	    sb->request_offset = desired_scroll;
	    sb->request_motion = SCROLL_FORWARD;
         }
	 break;

      case SCROLL_LINE_BACKWARD:
         if (!sb->advanced_mode) {
	    sb->request_offset = sb->line_height;
	    sb->request_motion = SCROLL_BACKWARD;
         }
	 desired_scroll = -(sb->line_height);
	 scrolling_up = FALSE; 
	 break;
   }
   if (desired_scroll)	/* int cast rqd due to c's brain damaged mult! */
      new_offset += (desired_scroll * (int)sb->view_length) / bar_length;
/*
 * Grid Normalization:
 *
 *	+---------------------------------------+ margin_offset
 *	+---------------------------------------+ -------
 *	|					|    |
 *	|					| line_offset
 *	|					|    |
 *	|.......................................| -------
 *	|					|
 *	|					|
 *------|---------------------------------------|---- new_offset
 *	|.......................................|
 *	|					|
 *	|					|
 *
 *
 *  We assume the view space is regularly divided by cells of equal size
 *  (line_height) with an initial gap and external margin, both measured
 *  in pixels.  Note that the line_height is assumed to contain the
 *  inter-object white space gap.  This is because the line ht is used
 *  as the amount to move on a line up/down scroll.
 *
 *  Normalization simply "rounds" the new_offset to be at an integral
 *  number of line_heights from the start of the view.  The "rounding
 *  rule" is to round up to the next line when scrolling down,
 *  and round down when scrolling up.  This preserves partial objects:
 *  A motion to a partial object will not inadvertantly scroll it out
 *  of the view window.
 *
 *  Fine point:  Note that we do not subtract margin from the new_offset
 *  when calculating the number of lines new_offset corresponds to.  This
 *  is so that a point within margin of line "n" will show that line,
 *  not line n-1.
 */
   if (/*sb->line_height &&*/ sb->use_grid) {
      if (sb->view_length != bar_length) {
         margin_offset = (margin_offset * sb->view_length) / bar_length;
         line_offset   = (line_offset   * sb->view_length) / bar_length;
         gap_offset    = (gap_offset    * sb->view_length) / bar_length;
      }

      if (line_offset) {
         min_offset = margin_offset;
         max_offset = (max_offset - margin_offset) / line_offset;
         max_offset = (max_offset * line_offset) + margin_offset - gap_offset;
         if (align_to_max)
            new_offset += sb->view_length;
         if (!scrolling_up)
            new_offset += line_offset - 1;
         new_offset = (new_offset - margin_offset + gap_offset) / line_offset;
         new_offset = (new_offset * line_offset) + margin_offset - gap_offset;
         if (align_to_max)
            new_offset -= sb->view_length;
      }

#ifdef notdef
      if (line_offset) {
         min_offset = margin_offset;
         max_offset = ((max_offset-margin_offset)/line_offset) * line_offset;
         if (align_to_max)
            new_offset += sb->view_length;
         if (!scrolling_up)
            new_offset += line_offset - 1;
         new_offset = (new_offset /*-margin*/ / line_offset) * line_offset;
         if (align_to_max)
            new_offset -= sb->view_length;
      }
#endif
   }

   if (new_offset <= min_offset)
      new_offset = 0;
   else if (new_offset > max_offset)
      new_offset = max_offset;

   sb->view_start = new_offset;
}
/**************************************************************************/
/* scrollbar_resize                                                       */
/**************************************************************************/

/*ARGSUSED*/
static void
scrollbar_resize(sb)
scrollbar_handle sb;
{
}

/************************************************************************/
/* timer notify proc                                                    */
/************************************************************************/

/*ARGSUSED*/
static Notify_value
scrollbar_timed_out(client, which)
Notify_client client;
int           which;
{
   scrollbar_t_on  = TRUE;
   post_scroll((scrollbar_handle)(LINT_CAST(client)), &scrollbar_t_event,
                scrollbar_t_motion, FALSE);
   return NOTIFY_DONE;
}

static void
scrollbar_timer_start(sb, event, motion)
scrollbar_handle	sb;
Event			*event;
Scroll_motion		motion;
{
   int		tenths, usecs, secs;
   
   tenths = sb->delay;
   if (!tenths)
       return;

   usecs  = (tenths % 10)*100000;
   secs   = tenths / 10;

   scrollbar_timer.it_value.tv_usec = usecs;
   scrollbar_timer.it_value.tv_sec  = secs;
   scrollbar_timer.it_interval.tv_usec = 1;
   scrollbar_timer.it_interval.tv_sec  = 0;

   scrollbar_t_event  = *event;
   scrollbar_t_motion = motion;

   (void)notify_set_itimer_func((Notify_client)sb, scrollbar_timed_out,
      ITIMER_REAL, &scrollbar_timer, (struct itimerval *)NULL);
}

static void
scrollbar_timer_stop(sb)
scrollbar_handle	sb;
{
   scrollbar_t_on  = FALSE;
   (void)notify_set_itimer_func((Notify_client)sb, scrollbar_timed_out, 
   	ITIMER_REAL, (struct itimerval *)NULL, (struct itimerval *)NULL);
}

