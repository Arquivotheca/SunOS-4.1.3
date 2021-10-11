#ifndef lint
#ifdef sccs
static        char sccsid[] = "@(#)panel_scroll.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*****************************************************************************/
/*                           panel_scroll.c                                  */
/*               Copyright (c) 1985 by Sun Microsystems, Inc.                */
/*****************************************************************************/

#include <suntool/panel_impl.h>

static paint_scrolled_panel();
static partial_paint();
static does_intersect();
static panel_adjust_scrollbar_rects();

static void normalize_top(), 	normalize_bottom(),
            normalize_left(), 	normalize_right();
static top_pair(), left_pair();

/****************************************************************************/
/* panel_scroll                                                             */
/****************************************************************************/

void
panel_scroll(panel, sb)
panel_handle	panel;
Scrollbar	sb;
{
   Scroll_motion	motion;
   int			caret_was_on = panel->caret_on;
   int			offset, line_ht;
   int			normalize, align_to_max, scrolling_up, vertical;
   panel_item_handle	low_ip, high_ip;

   if (caret_was_on) {
      panel_caret_on(panel, FALSE);
   }

   motion    = (Scroll_motion) scrollbar_get(sb, SCROLL_REQUEST_MOTION);
   vertical  = (Scrollbar_setting) scrollbar_get(sb, SCROLL_DIRECTION)
   		== SCROLL_VERTICAL;
   offset    = (int) scrollbar_get(sb, SCROLL_VIEW_START);
   normalize = (int) scrollbar_get(sb, SCROLL_NORMALIZE);
   line_ht   = (int) scrollbar_get(sb, SCROLL_LINE_HEIGHT);

   switch (motion) {
      case SCROLL_ABSOLUTE:
      case SCROLL_FORWARD:
      case SCROLL_PAGE_FORWARD:
      case SCROLL_LINE_FORWARD:
	 align_to_max = FALSE; 
	 scrolling_up = TRUE; 
	 break;

      case SCROLL_BACKWARD:
	 align_to_max = FALSE; 
	 scrolling_up = FALSE; 
	 break;

      case SCROLL_MAX_TO_POINT:
	 align_to_max = TRUE; 
	 scrolling_up = TRUE; 
	 break;

      case SCROLL_POINT_TO_MAX:
	 align_to_max = TRUE; 
	 scrolling_up = FALSE; 
	 break;

      case SCROLL_PAGE_BACKWARD:
	 align_to_max = FALSE; 
	 scrolling_up = FALSE; 
	 break;

      case SCROLL_LINE_BACKWARD:
	 align_to_max = FALSE; 
	 scrolling_up = FALSE; 
	 break;
   } 

   if (!line_ht) {
      if (motion==SCROLL_LINE_FORWARD || motion==SCROLL_LINE_BACKWARD) {
         normalize = TRUE;		/* object-normalize 0 line heights! */
         if (vertical)  {
            (void) top_pair(panel, &low_ip, &high_ip);
            if (scrolling_up && high_ip)
               offset = high_ip->rect.r_top + high_ip->rect.r_height + 1;
            else if (!scrolling_up && low_ip)
               offset = low_ip->rect.r_top - 1;
         } else {
            (void) left_pair(panel, &low_ip, &high_ip);
            if (scrolling_up && high_ip)
               offset = high_ip->rect.r_left + high_ip->rect.r_width + 1;
            else if (!scrolling_up && low_ip)
               offset = low_ip->rect.r_left - 1;
         }
         if (offset < 0)   /*be sure we didn't go negative*/
            offset = 0;
      }
   }

/*
            if (high_ip && low_ip)
               if (scrolling_up)
                  offset = high_ip->rect.r_top + high_ip->rect.r_height + 1;
               else
                  offset = low_ip->rect.r_top - 1;
            else if (low_ip)
               offset = panel->v_end;
            else
               offset = 0;
*/
/*
            if (high_ip && low_ip)
               if (scrolling_up)
                  offset = high_ip->rect.r_left + high_ip->rect.r_width + 1;
               else
                  offset = low_ip->rect.r_left - 1;
            else if (low_ip)
               offset = panel->h_end;
            else
               offset = 0;
*/
   if (vertical) {
      panel->v_offset = offset;
      if (normalize) {
         if (!line_ht) {
            if (align_to_max)
               normalize_bottom(panel, scrolling_up);
            else
               normalize_top(panel, scrolling_up);
         }
      }
      offset = panel->v_offset;
   } else {
      panel->h_offset = offset;
      if (normalize) {
         if (!line_ht) {
            if (align_to_max)
               normalize_right(panel, scrolling_up);
            else
               normalize_left(panel, scrolling_up);
         }
      }
      offset = panel->h_offset;
   }

   (void)scrollbar_set(sb, SCROLL_VIEW_START, offset, 0);
   paint_scrolled_panel(panel, sb);
   
   if (caret_was_on)
      panel_caret_on(panel, TRUE);
}

/*
 * Object normalization methodology:
 * A line is drawn through all the objects at the current view_offset.
 * The two rectangles (ip's) straddling this line, just above & below,
 * are obtained.  The offset is then modified to be just above or below
 * this rectangle [by the margin length] depending on the direction
 * of scrolling and on whether or not the topmost ip intersects offset.
 *
 *		low_ip
 *		+-------+
 *		|	| <----intersects=TRUE
 *---------------------------------------------------- offset
 *		|	|	+-------+
 *		+-------+	|	|
 *				+-------+
 *				high_ip
 *
 *
 */
static
top_pair(panel, low_ip, high_ip)
panel_handle		panel;
panel_item_handle	*low_ip, *high_ip;
{
   register panel_item_handle	ip;
   register int		low_top  = -1;
   register int		high_top = panel->v_end;
   register int		top;
   int			target   = panel->v_offset;
   int			intersects = FALSE;

/*fix for sb neg truncation: pin at 0:target = -1 => low_ip = null*/
   if (target == 0)
      target = -1;
      
   *high_ip = NULL;
   *low_ip  = NULL;
   for (ip = panel->items; ip; ip = ip->next) {
      if (!hidden(ip)) {
	 top = ip->rect.r_top;
	 if (top <= target) {
	    if (top > low_top) {
	       low_top = top;
	       *low_ip = ip;
	       intersects = (top + ip->rect.r_height > target);
	    }
	 } else {
	    if (top < high_top) {
	       high_top = top;
	       *high_ip = ip;
	    }
	 }
      } 
   }
/*
   if (!*high_ip)
      *high_ip = *low_ip;
   else if (!*low_ip)
      *low_ip = *high_ip;
*/
   return intersects;
}

static void
normalize_top(panel, scrolling_up)
panel_handle	panel;
int		scrolling_up;
{
   panel_item_handle 	low_ip, high_ip;
   register int		top;
   int			intersects;

   intersects = top_pair(panel, &low_ip, &high_ip);
   
   if (high_ip && low_ip) {
      top = high_ip->rect.r_top;
      if (intersects)
         top = low_ip->rect.r_top;
   } else if (low_ip)
      top = low_ip->rect.r_top;
   else
      top = 0;
   
   top -= panel->v_margin;
   if (top <= panel->v_margin)
      top = 0;
 
   panel->v_offset = top;
}
/*
 * Bottom aligned normalization methodology:
 * Like the above but with the bottoms of the rectangles used in the
 * calculations of position and the high ip determining the intersection.
 *				low_ip
 *				+-------+
 *				|	|
 *		high_ip		+-------+
 *		+-------+
 *		|	| <----intersects=TRUE
 *---------------------------------------------------- offset
 *		|	|
 *		+-------+
 *
 */
static void
normalize_bottom(panel, scrolling_up)
panel_handle	panel;
int		scrolling_up;
{
   register panel_item_handle	ip;
   register int		low_bottom  = 0;
   register int		high_bottom = panel->v_end;
   register int		top, bottom;
   int			target      = panel->v_offset + view_height(panel);
   int			intersects = FALSE;

   for (ip = panel->items; ip; ip = ip->next) {
      if (!hidden(ip)) {
         top    = ip->rect.r_top;
	 bottom = top + ip->rect.r_height;
	 if (bottom >= target) {
	    if (bottom < high_bottom) {
	       high_bottom = bottom;
	       intersects  = (top < target);
	    }
	 } else {
	    if (bottom > low_bottom)
	       low_bottom = bottom;
	 }
      } 
   }
   
   bottom = low_bottom;
   if (!scrolling_up && intersects)
      bottom = high_bottom;
   
   top = bottom + panel->v_margin - view_height(panel);
   if (top <= panel->v_margin)
      top = 0;
 
   panel->v_offset = top;
}

static
left_pair(panel, low_ip, high_ip)
panel_handle		panel;
panel_item_handle	*low_ip, *high_ip;
{
   register panel_item_handle	ip;
   register int		low_left  = -1;
   register int		high_left = panel->h_end;
   register int		left;
   int			target    = panel->h_offset;
   int			intersects = FALSE;

/*fix for sb neg truncation: pin at 0:target = -1 => low_ip = null*/
   if (target == 0)
      target = -1;
      
   *high_ip = NULL;
   *low_ip  = NULL;
   for (ip = panel->items; ip; ip = ip->next) {
      if (!hidden(ip)) {
	 left = ip->rect.r_left;
	 if (left <= target) {
	    if (left > low_left) {
	       low_left = left;
	       *low_ip = ip;
	       intersects = (left + ip->rect.r_width > target);
	    }
	 } else {
	    if (left < high_left) {
	       high_left = left;
	       *high_ip = ip;
	    }
	 }
      } 
   }
/*
   if (!*high_ip)
      *high_ip = *low_ip;
   else if (!*low_ip)
      *low_ip = *high_ip;
*/
   return intersects;
}

static void
normalize_left(panel, scrolling_up)
panel_handle	panel;
int		scrolling_up;
{
   panel_item_handle	low_ip, high_ip;
   register int		left;
   int			intersects;

   intersects = left_pair(panel, &low_ip, &high_ip);
   
   if (high_ip && low_ip) {
      left = high_ip->rect.r_left;
      if (intersects)
         left = low_ip->rect.r_left;
   } else if (low_ip)
      left = low_ip->rect.r_left;
   else
      left = 0;

   left -= panel->h_margin;
   if (left <= panel->h_margin)
      left = 0;
 
   panel->h_offset = left;
}

static void
normalize_right(panel, scrolling_up)
panel_handle	panel;
int		scrolling_up;
{
   register panel_item_handle	ip;
   register int		low_right  = 0;
   register int		high_right = panel->h_end;
   register int		left, right;
   int			target     = panel->h_offset + view_width(panel);
   int			intersects;

   for (ip = panel->items; ip; ip = ip->next) {
      if (!hidden(ip)) {
         left  = ip->rect.r_left;
	 right = left + ip->rect.r_width;
	 if (right >= target) {
	    if (right < high_right) {
	       high_right = right;
	       intersects  = (left < target);
	    }
	 } else {
	    if (right > low_right)
	       low_right = right;
	 }
      } 
   }
   
   right = low_right;
   if (!scrolling_up && intersects)
      right = high_right;
   
   left = right + panel->h_margin - view_width(panel);
   if (left <= panel->h_margin)
      left = 0;
 
   panel->h_offset = left;
}

/****************************************************************************/
/* paint_scrolled_panel                                                     */
/* Redisplays scrolled panel with minimum repaint, shifting the portion of  */
/* the panel which is visible in both the old and new positions.            */
/****************************************************************************/

static
paint_scrolled_panel(panel, sb)
register panel_handle    panel;
Scrollbar       	sb;
{
   register unsigned	new_offset, old_offset;
   register int     	visible_end, delta;
   register int     	width, height, xsrc, ysrc, xdest, ydest;
   Scrollbar_setting    direction;
   Rect			rect;

   old_offset = (unsigned) scrollbar_get(sb, SCROLL_LAST_VIEW_START);
   new_offset = (unsigned) scrollbar_get(sb, SCROLL_VIEW_START);
   direction  = (Scrollbar_setting) scrollbar_get(sb, SCROLL_DIRECTION);

   delta = new_offset > old_offset ? 
	   new_offset - old_offset : old_offset - new_offset;

   if (direction == SCROLL_VERTICAL) {
      visible_end    = view_height(panel);
      width          = BIG;
      height         = visible_end;
      xsrc           = 0;
      ysrc           = delta;
      xdest          = 0;
      ydest          = delta;
   } else {
      visible_end    = view_width(panel);
      width          = visible_end;
      height         = BIG;
      xsrc           = delta;
      ysrc           = 0;
      xdest          = delta;
      ydest          = 0;
   }

   /* because view_pixwin is not registered with the
    * Agent, we must clear the damegedid by doing a
    * pw_exposed().  Otherwise, pw_copy() will report
    * a bogus fixup list.
    */
   (void)pw_exposed(panel->view_pixwin);

   /* lock around the whole view pixwin */
   rect = *panel->view_pixwin->pw_clipdata->pwcd_regionrect;
   rect.r_left = rect.r_top = 0;
   (void)pw_lock(panel->view_pixwin, &rect);

   if (delta > visible_end) {
      (void)pw_unlock(panel->view_pixwin);
      (void)pw_writebackground(panel->view_pixwin, 0,0,
          view_width(panel), view_height(panel), PIX_CLR);
      (void) panel_paint((Panel) panel, PANEL_NO_CLEAR);
      return;
   } 

   if (new_offset > old_offset) {
      (void)pw_copy(panel->view_pixwin, 0, 0, width, height,
	      PIX_SRC, panel->view_pixwin, xsrc, ysrc);
      partial_paint(panel, visible_end-delta, visible_end, direction);
   } else if (new_offset < old_offset) {
      (void)pw_copy(panel->view_pixwin, xdest, ydest, width, height,
	      PIX_SRC, panel->view_pixwin, 0, 0);
      partial_paint(panel, 0, delta, direction);
   }

   (void)pw_unlock(panel->view_pixwin);

   (void)pw_restrict_clipping(panel->view_pixwin,  &(panel->view_pixwin->pw_fixup));
   (void)pw_writebackground(panel->view_pixwin, 0,0,
       view_width(panel), view_height(panel), PIX_CLR);
   (void) panel_paint((Panel) panel, PANEL_NO_CLEAR);
   (void)pw_exposed(panel->view_pixwin);
}

static
partial_paint(panel, start, end, direction)
panel_handle   		panel;
int             	start, end;
Scrollbar_setting 	direction;
{
   register panel_item_handle ip;

   /* clear the region to be painted */
   if (direction == SCROLL_VERTICAL)
      (void)pw_writebackground(panel->view_pixwin, 0, start, 
			 BIG, end-start, PIX_CLR);
   else
      (void)pw_writebackground(panel->view_pixwin, start, 0, 
			 end-start, BIG, PIX_CLR);

   /* paint each hidden item in the region */
   for (ip = panel->items; ip; ip = ip->next)
      if (does_intersect(panel, &ip->rect, start, end, direction) && 
	  hidden(ip))
         (void)panel_paint_item(ip, PANEL_NO_CLEAR);
    
   /* paint each non-hidden item in the region */
   for (ip = panel->items; ip; ip = ip->next)
      if (does_intersect(panel, &ip->rect, start, end, direction) && 
	  !hidden(ip))
         (void)panel_paint_item(ip, PANEL_NO_CLEAR);
}

static
does_intersect(panel, r, start, end, direction)
panel_handle 		panel;
Rect           		*r;
int             	start, end;
Scrollbar_setting 	direction;
{
   register int adjusted_start, extent;

   if (direction == SCROLL_VERTICAL) {
      adjusted_start = r->r_top - panel->v_offset;
      extent         = r->r_height;
   } else {
      adjusted_start = r->r_left - panel->h_offset;
      extent         = r->r_width;
   }

   if (adjusted_start >= start && adjusted_start <= end) 
      return TRUE;
   else if (adjusted_start < start && adjusted_start+extent >= start) 
      return TRUE;
   else
      return FALSE;
}

/****************************************************************************/
/* panel_sb_modify                                                          */
/****************************************************************************/

panel_sb_modify(sb)
Scrollbar sb;
{
   register panel_handle         panel = (panel_handle)
			 LINT_CAST(scrollbar_get(sb, SCROLL_NOTIFY_CLIENT));
   register Scrollbar_setting    direction;
   register unsigned             normalize_mask;
   register int                  new_bar_width;
   register int       		*bar_width, *margin;
   Scrollbar_attribute           width_attr;

   direction = (Scrollbar_setting) scrollbar_get(sb, SCROLL_DIRECTION);
   if (direction == SCROLL_VERTICAL) {
      bar_width      = &panel->v_bar_width;
      margin         = &panel->v_margin;
      normalize_mask = SCROLL_V_NORMALIZE;
      width_attr     = SCROLL_WIDTH;
   } else {
      bar_width      = &panel->h_bar_width;
      margin         = &panel->h_margin;
      normalize_mask = SCROLL_H_NORMALIZE;
      width_attr     = SCROLL_HEIGHT;
   }

   new_bar_width = (int) scrollbar_get(sb, width_attr);
   if (new_bar_width != *bar_width) {
      *bar_width = new_bar_width;
      if (panel->h_scrollbar && panel->v_scrollbar) 
	 panel_adjust_scrollbar_rects(panel);
   }

   panel_resize_region(panel);

   *margin    = (unsigned)scrollbar_get(sb, SCROLL_GAP);
   if (scrollbar_get(sb, SCROLL_NORMALIZE))
      panel->status |= normalize_mask;
   else
      panel->status &= ~normalize_mask;
}

/*****************************************************************************/
/* panel_setup_scrollbar                                                     */
/* called from panel_public.c                                                */
/*****************************************************************************/

void
panel_setup_scrollbar(panel, direction, have_new_scrollbar, new_scrollbar)
register panel_handle panel;
Scrollbar_setting     direction;
int                   have_new_scrollbar;
Scrollbar             new_scrollbar;
{
   register Scrollbar *target_sb;
   register Scrollbar  old_sb;
   unsigned            normalize_mask;      
   int                *bar_width, *margin, *offset;
   int                 panel_length;
   struct inputmask    mask;
   int                 designee;

   if (!have_new_scrollbar)
      return;

   if (direction == SCROLL_VERTICAL) {
      target_sb       = &panel->v_scrollbar;
      bar_width       = &panel->v_bar_width;
      margin          = &panel->v_margin;
      offset          = &panel->v_offset;
      panel_length    = panel->v_end + 1;
      normalize_mask  = SCROLL_V_NORMALIZE;
   } else {
      target_sb       = &panel->h_scrollbar;
      bar_width       = &panel->h_bar_width;
      margin          = &panel->h_margin;
      offset          = &panel->h_offset;
      panel_length    = panel->h_end + 1;
      normalize_mask  = SCROLL_H_NORMALIZE;
   }

   old_sb     = *target_sb;
   *target_sb = new_scrollbar;
   if (new_scrollbar) {

      /* this call lets the scrollbar know about the panel,      */
      /* sets advanced mode, sets the scrollbar's direction, and */
      /* initializes the scrollbar's idea of the viewing window  */
      (void)scrollbar_set(new_scrollbar,
		    SCROLL_NOTIFY_CLIENT, panel,
		    SCROLL_OBJECT,        panel,
		    SCROLL_MODIFY_PROC,   panel_sb_modify,
		    SCROLL_ADVANCED_MODE, TRUE,
		    SCROLL_DIRECTION,     direction,
		    SCROLL_VIEW_START,    0,
		    SCROLL_OBJECT_LENGTH, panel_length,
		    0);

      /* let the panel know about the scrollbar */
      *bar_width = (int)      scrollbar_get(new_scrollbar, SCROLL_THICKNESS);
      *margin    = (unsigned) scrollbar_get(new_scrollbar, SCROLL_GAP);
      if (scrollbar_get(new_scrollbar, SCROLL_NORMALIZE))
	 panel->status |= normalize_mask;
      else
	 panel->status &= ~normalize_mask;

   } else {
      /* erase the old scrollbar's knowledge of the panel */
      if ((int(*)())LINT_CAST(scrollbar_get(old_sb,SCROLL_MODIFY_PROC)) == panel_sb_modify)
	 (void)scrollbar_set(old_sb, SCROLL_MODIFY_PROC, 0, 0);
      if ( (panel_handle) LINT_CAST(scrollbar_get(old_sb, SCROLL_NOTIFY_CLIENT)) == panel)
	 (void)scrollbar_set(old_sb, SCROLL_NOTIFY_CLIENT, 0, 0);
      if ( (panel_handle) LINT_CAST(scrollbar_get(old_sb, SCROLL_OBJECT)) == panel)
	 (void)scrollbar_set(old_sb, SCROLL_OBJECT, 0, 0);

      /* erase the panel's knowledge of the old scrollbar */
      *bar_width     = 0;
      *margin        = 0;
      *offset        = 0;
      panel->status &= ~normalize_mask;
   }

   panel_adjust_scrollbar_rects(panel);

   /* adjust regions to reflect presence or absence of new scrollbar */
   panel_resize_region(panel);

   /* paint the panel */
   /*
   panel_paint(panel, PANEL_CLEAR);
   */

   /* panel only wants LOC_MOV if it has a scrollbar */
   (void)win_getinputmask(panel->windowfd, &mask, &designee);
   if (has_scrollbar(panel) || has_background_proc(panel))
      win_setinputcodebit(&mask, LOC_MOVE);
   else
      win_unsetinputcodebit(&mask, LOC_MOVE);
   (void)win_setinputmask(panel->windowfd, &mask, (struct inputmask *)0, designee);
}

static 
panel_adjust_scrollbar_rects(panel)
panel_handle panel;
{
   register Scrollbar_setting v_gravity = (Scrollbar_setting)
      scrollbar_get(panel->v_scrollbar, SCROLL_PLACEMENT);
   register Scrollbar_setting h_gravity = (Scrollbar_setting)
      scrollbar_get(panel->h_scrollbar, SCROLL_PLACEMENT);

   if (panel->h_scrollbar && panel->v_scrollbar) {
      /* first set the height and width, which don't depend on the gravity */
      (void)scrollbar_set(panel->v_scrollbar, SCROLL_HEIGHT, view_height(panel), 0);
      (void)scrollbar_set(panel->h_scrollbar, SCROLL_WIDTH,  view_width(panel),  0);

      if (v_gravity == SCROLL_EAST && h_gravity == SCROLL_NORTH) {
	 (void)scrollbar_set(panel->v_scrollbar, 
		       SCROLL_TOP,  panel->h_bar_width /*- 1*/, 
	               SCROLL_LEFT, view_width(panel),
		       0);
	 (void)scrollbar_set(panel->h_scrollbar, 
		       SCROLL_TOP,  0, 
		       SCROLL_LEFT, 0, 
		       0);
	 (void)pw_writebackground(panel->pixwin,
	 		    view_width(panel),
	 		    0,
			    panel->v_bar_width,
			    panel->h_bar_width, 
			    PIX_CLR);
      } else if (v_gravity == SCROLL_EAST && h_gravity == SCROLL_SOUTH) {
	 (void)scrollbar_set(panel->v_scrollbar, 
		       SCROLL_TOP,  0, 
	               SCROLL_LEFT, view_width(panel),
		       0);
	 (void)scrollbar_set(panel->h_scrollbar, 
		       SCROLL_TOP,  view_height(panel), 
		       SCROLL_LEFT, 0, 
		       0);
	 (void)pw_writebackground(panel->pixwin, 
			    view_width(panel),
			    view_height(panel),
			    panel->v_bar_width,
			    panel->h_bar_width, 
			    PIX_CLR);
      } else if (v_gravity == SCROLL_WEST && h_gravity == SCROLL_NORTH) {
	 (void)scrollbar_set(panel->v_scrollbar, 
		       SCROLL_TOP,  panel->h_bar_width /*- 1*/, 
	               SCROLL_LEFT, 0,
		       0);
	 (void)scrollbar_set(panel->h_scrollbar, 
		       SCROLL_TOP,  0, 
		       SCROLL_LEFT, panel->v_bar_width /*- 1*/, 
		       0);
	 (void)pw_writebackground(panel->pixwin,
			    0,
			    0,
			    panel->v_bar_width,
			    panel->h_bar_width, 
			    PIX_CLR);
      } else if (v_gravity == SCROLL_WEST && h_gravity == SCROLL_SOUTH) {
	 (void)scrollbar_set(panel->v_scrollbar, 
		       SCROLL_TOP,  0, 
	               SCROLL_LEFT, 0,
		       0);
	 (void)scrollbar_set(panel->h_scrollbar, 
		       SCROLL_TOP,  view_height(panel), 
		       SCROLL_LEFT, panel->v_bar_width /*- 1*/, 
		       0);
	 (void)pw_writebackground(panel->pixwin,
	 		    0,
	 		    view_height(panel) /*+ 1*/,
			    panel->v_bar_width,
			    panel->h_bar_width, 
			    PIX_CLR);
      }
   } else if (panel->h_scrollbar) {
      (void)scrollbar_set(panel->h_scrollbar,
      		    SCROLL_LEFT,	0,
      		    SCROLL_WIDTH,	view_width(panel),
      		    0);
   } else if (panel->v_scrollbar) {
      (void)scrollbar_set(panel->v_scrollbar,
      		    SCROLL_TOP,		0,
      		    SCROLL_HEIGHT,	view_height(panel),
      		    0);
   }

   /* reset the view_length of both bars to make sure its right */
   (void)scrollbar_set(panel->v_scrollbar, SCROLL_VIEW_LENGTH, view_height(panel),0);
   (void)scrollbar_set(panel->h_scrollbar, SCROLL_VIEW_LENGTH, view_width(panel), 0);
}

void
panel_compute_region_rect(panel, rect)
panel_handle panel;
Rect         *rect;
{
   register Scrollbar_setting   v_gravity = (Scrollbar_setting)
      scrollbar_get(panel->v_scrollbar, SCROLL_PLACEMENT);
   register Scrollbar_setting   h_gravity = (Scrollbar_setting)
      scrollbar_get(panel->h_scrollbar, SCROLL_PLACEMENT);

   rect->r_left = v_gravity == SCROLL_WEST ? panel->v_bar_width : 0;
   rect->r_top = h_gravity == SCROLL_NORTH ? panel->h_bar_width : 0;
   rect->r_width = view_width(panel);
   rect->r_height = view_height(panel);
}

void
panel_resize_region(panel)
panel_handle panel;
{
   Rect                         rect;

   panel_compute_region_rect(panel, &rect);
   (void) pw_set_region_rect(panel->view_pixwin, &rect, TRUE);
}

/*****************************************************************************/
/* panel_update_scrollbars                                                   */
/* called from panel_paint.c                                                */
/*****************************************************************************/

/* ARGSUSED */
void
panel_update_scrollbars(panel, r)
panel_handle  panel;
Rect         *r;
{
   /* first place the scrollbars correctly within the panel */
   panel_adjust_scrollbar_rects(panel);

   /* now tell the scrollbars of the current size of the viewing window */
   (void)scrollbar_set(panel->v_scrollbar,
		 SCROLL_HEIGHT,         view_height(panel),
		 SCROLL_VIEW_START,     panel->v_offset,
		 SCROLL_VIEW_LENGTH,    view_height(panel),
		 SCROLL_OBJECT_LENGTH,  panel->v_end + 1,
		 0);
   (void)scrollbar_set(panel->h_scrollbar,
		 SCROLL_WIDTH,          view_width(panel),
		 SCROLL_VIEW_START,     panel->h_offset,
		 SCROLL_VIEW_LENGTH,    view_width(panel),
		 SCROLL_OBJECT_LENGTH,  panel->h_end + 1,
		 0);

   /* finally, paint the scrollbars */
   (void)scrollbar_paint_clear(panel->v_scrollbar);
   (void)scrollbar_paint_clear(panel->h_scrollbar);
}

/*****************************************************************************/
/* panel_update_scrolling_size                                               */
/*****************************************************************************/

panel_update_scrolling_size(client_panel)
Panel	client_panel;
{
   register panel_handle	panel = PANEL_CAST(client_panel);
   register panel_item_handle	item;

   panel->v_end = panel->h_end = 0;
   for (item = panel->items; item; item = item->next) {
      if (!hidden(item)) {
	 if (item->rect.r_top + item->rect.r_height > panel->v_end)
	    panel->v_end = item->rect.r_top + item->rect.r_height;
	 if (item->rect.r_left + item->rect.r_width > panel->h_end)
	    panel->h_end = item->rect.r_left + item->rect.r_width;
      }
   }
   (void)scrollbar_set(panel->v_scrollbar, SCROLL_OBJECT_LENGTH, panel->v_end + 1, 0);
   (void)scrollbar_set(panel->h_scrollbar, SCROLL_OBJECT_LENGTH, panel->h_end + 1, 0);
}

/*****************************************************************************/
/* panel_update_extent                                                       */
/* called from panel_attr.c                                                  */
/*****************************************************************************/

void
panel_update_extent(panel, rect)
panel_handle panel;
Rect         rect;
{
   if (rect.r_top + rect.r_height > panel->v_end) {
      panel->v_end = rect.r_top + rect.r_height;
      (void)scrollbar_set(panel->v_scrollbar,
		    SCROLL_OBJECT_LENGTH, panel->v_end+1, 0);
   }
   if (rect.r_left + rect.r_width > panel->h_end) {
      panel->h_end = rect.r_left + rect.r_width;
      (void)scrollbar_set(panel->h_scrollbar,
		    SCROLL_OBJECT_LENGTH, panel->h_end+1, 0);
   }
}
