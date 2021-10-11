#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)scrollbar_paint.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*************************************************************************/
/*                         scrollbar_paint.c                             */
/*           Copyright (c) 1985 by Sun Microsystems, Inc.                */
/*************************************************************************/

#include <suntool/scrollbar_impl.h> 

static int paint_client, clear_client;

static void paint_bar(), paint_border(), paint_client_pw();
static void clear_bar(), paint_bubble();
static void lock_bar(), unlock_bar();
static void paint_v_arrow(), paint_h_arrow();

/**************************************************************************/
/* patterns                                                               */
/**************************************************************************/

static short scrollbar_bar_pattern[8] = {
        0x8080, 0x1010, 0x0202, 0x4040, 0x0808, 0x0101, 0x2020, 0x0404
};
mpr_static(scrollbar_mpr_for_sb_pattern, 16, sizeof(scrollbar_bar_pattern)/2, 1,
                scrollbar_bar_pattern);
 
static short scrollbar_grey_pattern[16] = {
        0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555,
        0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555
};
mpr_static(scrollbar_mpr_for_grey_pattern, 16, 16, 1, scrollbar_grey_pattern);

/**************************************************************************/
/* external procs                                                         */
/**************************************************************************/

scrollbar_paint(sb_client)
Scrollbar sb_client;
{
   register    bar_should_appear    = FALSE;
   register    bubble_should_appear = FALSE;
   register    scrollbar_handle	sb;

   sb = (scrollbar_handle)(LINT_CAST(sb_client));
   if ((!sb) || (!sb->pixwin))
      return 0;

   if (bar_always(sb) || ((sb == scrollbar_active_sb) && bar_active(sb)))
      bar_should_appear = TRUE;
   if (bubble_always(sb) || ((sb == scrollbar_active_sb) && bubble_active(sb)))
      bubble_should_appear = TRUE;

   lock_bar(sb);
   if (sb->bubble_modified || sb->attrs_modified) {
      if (bar_should_appear)
	 paint_bar(sb);
      else
	 clear_bar(sb);
      if (bubble_should_appear)
	 paint_bubble(sb);
   } else if (bar_should_appear) {
      if (!sb->bar_painted)
	 paint_bar(sb);
      if (bubble_should_appear) 
	 paint_bubble(sb);
   } else if (bubble_should_appear) {
      if (!sb->bar_painted && !sb->bubble_painted)
	 paint_bubble(sb);
      else {
	 clear_bar(sb);
	 paint_bubble(sb);
      }
   }
   unlock_bar(sb);

   sb->bubble_modified  = FALSE;
   sb->attrs_modified	= FALSE;
   return 1;
}

scrollbar_paint_clear(sb_client)
Scrollbar sb_client;
{
   scrollbar_handle sb;
   
   sb = (scrollbar_handle)(LINT_CAST(sb_client));
   if ((!sb) || (!sb->pixwin))
      return 0;

   lock_bar(sb);
   clear_bar(sb);
   if (bar_always(sb))
      paint_bar(sb);
   if (bubble_always(sb))
      paint_bubble(sb);
   unlock_bar(sb);

   sb->bubble_modified 	= FALSE;
   sb->attrs_modified 	= FALSE;
   return 1;
}


scrollbar_clear(sb_client)
Scrollbar sb_client;
{
   scrollbar_handle  sb;
   
   sb = (scrollbar_handle)(LINT_CAST(sb_client));
   lock_bar(sb);
   clear_bar(sb);
   unlock_bar(sb);
}

scrollbar_paint_bubble(sb_client)
Scrollbar sb_client;
{
   scrollbar_handle sb;
   
   sb = (scrollbar_handle)(LINT_CAST(sb_client));
   lock_bar(sb);
   paint_bubble(sb);
   unlock_bar(sb);
}

scrollbar_clear_bubble(sb_client)
Scrollbar sb_client;
{
   scrollbar_handle sb;
   
   sb = (scrollbar_handle)(LINT_CAST(sb_client));
   lock_bar(sb);
   if (bar_always(sb) || ((sb == scrollbar_active_sb) && bar_active(sb)))
      paint_bar(sb);
   else
      clear_bar(sb);
   unlock_bar(sb);
}

scrollbar_repaint(sb_client, status)  /* called by scrollbar_event.c */
Scrollbar	 sb_client;
int              status;
{
   scrollbar_handle	sb;
   
   sb = (scrollbar_handle)(LINT_CAST(sb_client));
   if ((!sb) || (!sb->pixwin))
      return 0;

   if (!(bar_active(sb) || bubble_active(sb)))
      return 0;

   lock_bar(sb);
   if (status == SCROLLBAR_OUTSIDE) {
      if (sb->bar_display_level == SCROLL_ALWAYS)
	 paint_bar(sb);
      else {
	 clear_bar(sb);
	 if (bubble_always(sb))
	    paint_bubble(sb);
      }
   } else {
      if (bar_active(sb))
	 paint_bar(sb);
      if (bubble_active(sb) || (bubble_always(sb) && sb->bar_painted))
	 paint_bubble(sb);
   }
   unlock_bar(sb);
   return 0;
}

/**************************************************************************/
/* lock/unlock & cover for external interface                             */
/**************************************************************************/

static void
lock_bar(sb)
scrollbar_handle sb;
{
   struct rect rect;
   
   (void)pw_get_region_rect(sb->pixwin, &rect);
   rect.r_left = rect.r_top = 0;
   (void)pw_lock(sb->pixwin, &rect);
   
   paint_client = clear_client = FALSE;
}

static void
unlock_bar(sb)
scrollbar_handle sb;
{
   (void)pw_unlock(sb->pixwin);
   if (paint_client || clear_client) 
      paint_client_pw(sb, paint_client);
}

static void
paint_bar(sb)
scrollbar_handle sb;
{
   register int x, y, w, h;

   if (sb->buttons && (!sb->buttons_painted || sb->attrs_modified))
      sb->paint_buttons_proc(sb);

   if (sb->horizontal) {
      x = sb->button_length;
      y = 0;
      w = sb->rect.r_width - (2*sb->button_length);
      h = sb->rect.r_height;
      if (sb->border) {
	 h--; 
	 if (sb->gravity == SCROLL_MAX) 
	    y++;
      }
   } else {
      x = 0;
      y = sb->button_length;
      w = sb->rect.r_width;
      h = sb->rect.r_height - (2*sb->button_length);
      if (sb->border) {
	 w--; 
	 if (sb->gravity == SCROLL_MAX) 
	    x++;
      }
   }

   if (!sb->one_button_case)
      if (sb->bar_grey)
         (void)pw_replrop(sb->pixwin, x, y, w, h, 
                    PIX_SRC, &scrollbar_mpr_for_sb_pattern, 0, 0);
      else
         (void)pw_writebackground(sb->pixwin, x, y, w, h, PIX_CLR);
   
   if (sb->border && (!sb->border_painted || sb->attrs_modified)) 
      paint_border(sb);

   sb->bar_painted    = TRUE;
   sb->bubble_painted = FALSE;
}

static void
paint_border(sb)
scrollbar_handle sb;
{
   register int x, y, w, h;

   if (sb->horizontal) {
      x = sb->button_length;
      y = 0;
      w = sb->rect.r_width - (2*sb->button_length);
      h = 1;
      if (sb->gravity == SCROLL_MIN)
         y = sb->rect.r_height - 1;
   } else {
      x = 0;
      y = sb->button_length;
      w = 1;
      h = sb->rect.r_height - (2*sb->button_length);
      if (sb->gravity == SCROLL_MIN)
         x = sb->rect.r_width - 1;
   }
   
   if (!sb->one_button_case)
      (void)pw_vector(sb->pixwin, x, y, x+w-1, y+h-1, PIX_SET, 1);

   sb->border_painted  = TRUE;
   paint_client = TRUE;
}

static void
paint_client_pw(sb, paint_flag)
scrollbar_handle	sb;
int			paint_flag;
{
   struct rect		rect;
   struct pixwin	*client_pw = NULL;
   int			x, y, h, w;

   /* now paint or clear the border on the end of the button if necessary */
   (void)win_getsize(sb->client_windowfd, &rect);
   x = sb->rect.r_left;
   y = sb->rect.r_top;
   w = sb->rect.r_width;
   h = sb->rect.r_height;
   if (sb->horizontal) {
      if (sb->rect.r_left) { 				/* paint left edge */
         client_pw++;
         x--;
      } else if (sb->rect.r_width != rect.r_width) { 	/* paint right edge */
         client_pw++;
         x += w;
      }
      w = 1;
   } else {
      if (sb->rect.r_top) {				/* paint top edge */
         client_pw++;
         y--;
      } else if (sb->rect.r_height != rect.r_height) {	/* paint bottom edge */
         client_pw++;
         y += h;
      }
      h = 1;
   }

   if (client_pw) {
      client_pw  = win_get_pixwin(sb->notify_client);
      if (paint_flag)
	 (void)pw_vector(client_pw, x, y, x+w-1, y+h-1, PIX_SET, 1);
      else
         (void)pw_writebackground(client_pw, x, y, w, h, PIX_CLR);
   }
}

static void
clear_bar(sb)
scrollbar_handle sb;
{
   (void)pw_writebackground(sb->pixwin, 0, 0,
		      sb->rect.r_width, sb->rect.r_height, 
		      PIX_CLR);

   sb->bar_painted     = FALSE;
   sb->bubble_painted  = FALSE;
   sb->buttons_painted = FALSE;
   sb->border_painted  = FALSE;
   
   clear_client = TRUE;
}

static void
paint_bubble(sb)
scrollbar_handle sb;
{
   struct rect bubble;

   if (!sb->one_button_case) {
      (void)scrollbar_compute_bubble_rect(sb, &bubble);
      if (sb->bubble_grey)
         (void)pw_replrop(sb->pixwin,
		    bubble.r_left, bubble.r_top,
		    bubble.r_width, bubble.r_height, PIX_SRC,
		    &scrollbar_mpr_for_grey_pattern, 0, 0);
      else
         (void)pw_writebackground(sb->pixwin, 
			    bubble.r_left, bubble.r_top,
			    bubble.r_width, bubble.r_height, PIX_SET);
   }
   
   sb->bubble_painted = TRUE;
}

/**************************************************************************/
/* scrollbar_paint_buttons                                                */
/**************************************************************************/

scrollbar_paint_buttons(sb)
scrollbar_handle sb;
{
   /*register int tb_margin, lr_margin, t, b, l, r;*/

   if (sb->horizontal) {
      /* right button */
      (void)pw_writebackground(sb->pixwin,
                         (int)(sb->rect.r_width-sb->button_length),
			 0, 
			 (int)(sb->button_length) /*+1*/,
			 sb->rect.r_height,
			 PIX_CLR); 
      if (!sb->one_button_case)
      (void)pw_vector(sb->pixwin, 
                (int)(sb->rect.r_width - sb->button_length) /*+ 1*/, 
                0, 
                (int)(sb->rect.r_width - sb->button_length) /*+ 1*/, 
                sb->rect.r_height - 1, 
       		PIX_SET, 1);
      if (sb->gravity == SCROLL_MIN)
	 (void)pw_vector(sb->pixwin, 
		   (int)(sb->rect.r_width  - sb->button_length) /*+1*/, 
		   sb->rect.r_height - 1, 
		   (int)(sb->rect.r_width - 1) /**/, 
		   sb->rect.r_height - 1, 
		   PIX_SET, 1);
      else if (sb->gravity == SCROLL_MAX)
	 (void)pw_vector(sb->pixwin, 
		   (int)(sb->rect.r_width - sb->button_length) /*+1*/, 
		   0, 
		   (int)(sb->rect.r_width - 1) /**/, 
		   0, 
		   PIX_SET, 1);
      if (sb->rect.r_height >= 12)
	 paint_h_arrow(sb, 
		       (short)(sb->rect.r_width-sb->button_length+1),
	               sb->rect.r_height);

      /* left button */
      if (!sb->one_button_case) {
      (void)pw_writebackground(sb->pixwin,
			 0,
      			 0, 
			 (int)sb->button_length,
			 sb->rect.r_height,
			 PIX_CLR); 
      (void)pw_vector(sb->pixwin, 
                (int)(sb->button_length - 1), 
                0, 
                (int)(sb->button_length - 1), 
                sb->rect.r_height - 1, 
                PIX_SET, 1);
      if (sb->gravity == SCROLL_MIN)
	 (void)pw_vector(sb->pixwin, 
		   0, 
		   sb->rect.r_height - 1, 
		   (int)(sb->button_length - 1) /**/, 
		   sb->rect.r_height - 1, 
		   PIX_SET, 1);
      else if (sb->gravity == SCROLL_MAX)
	 (void)pw_vector(sb->pixwin,
	 	   0,
	 	   0, 
		   (int)(sb->button_length - 1) /**/,
	 	   0,
	 	   PIX_SET, 1);
      if (sb->rect.r_height >= 12)
	 paint_h_arrow(sb, 0, sb->rect.r_height);
      }
   } else {
      /* bottom button */
      (void)pw_writebackground(sb->pixwin,
			 0, 
			 (int)(sb->rect.r_height-sb->button_length), 
			 sb->rect.r_width,
			 (int)sb->button_length /*+1*/,
			 PIX_CLR); 
      /* horizontal line on top of button */
      if (!sb->one_button_case)
      (void)pw_vector(sb->pixwin, 
		0,
		(int)(sb->rect.r_height - sb->button_length) /*+ 1*/, 
		sb->rect.r_width  - 1,
		(int)(sb->rect.r_height - sb->button_length) /*+ 1*/, 
		PIX_SET, 1);
      if (sb->gravity == SCROLL_MIN)
	 /* vertical line on right of button */
	 (void)pw_vector(sb->pixwin, 
		   sb->rect.r_width - 1, 
		   (int)(sb->rect.r_height - sb->button_length)  /*+ 1*/, 
		   sb->rect.r_width - 1, 
		   sb->rect.r_height - 1 /**/, 
		   PIX_SET, 1);
      else if (sb->gravity == SCROLL_MAX)
	 (void)pw_vector(sb->pixwin, 
		   0, 
		   (int)(sb->rect.r_height - sb->button_length) /*+ 1*/, 
		   0, 
		   sb->rect.r_height - 1 /**/, 
		   PIX_SET, 1);
      if (sb->rect.r_width >= 12)
	 paint_v_arrow(sb, 0, (short)(sb->rect.r_height - sb->button_length + 1));

      if (!sb->one_button_case) {
      /* top button */
      (void)pw_writebackground(sb->pixwin,
      			 0,
      			 0, 
			 sb->rect.r_width,
			 (int)sb->button_length,
			 PIX_CLR); 
      /* horizontal line on bottom of button */
      (void)pw_vector(sb->pixwin, 
                0,
                (int)(sb->button_length - 1), 
                sb->rect.r_width  - 1, 
                (int)(sb->button_length - 1), 
                PIX_SET, 1);
       if (sb->gravity == SCROLL_MIN)
	 /* vertical line on right of button */
	 (void)pw_vector(sb->pixwin, 
		   sb->rect.r_width  - 1, 
		   0, 
		   sb->rect.r_width  - 1, 
		   (int)(sb->button_length - 1), 
		   PIX_SET, 1);
       else if (sb->gravity == SCROLL_MAX)
	 /* vertical line on left of button */
	 (void)pw_vector(sb->pixwin,
	 	   0,
	 	   0,
	 	   0,
	 	   (int)(sb->button_length - 1),
	 	   PIX_SET, 1);
      if (sb->rect.r_width >= 12)
         paint_v_arrow(sb, 0, 0);
      }
   }

   sb->buttons_painted = TRUE;
}

/**************************************************************************/
/* arrow painting routines                                                */
/**************************************************************************/

static void
paint_h_arrow(sb, left, bottom)
scrollbar_handle sb;
short left, bottom;
{
   register int /*tb_margin, lr_margin, t, */ b, l, r;

   l = left + 2;
   r = left + sb->button_length - 3;
   /*tb_margin = (sb->rect.r_height - 7) / 2; */
   b = bottom - ((sb->rect.r_height - 7) / 2) - 2;

   /* left triangle */
   (void)pw_put(sb->pixwin, l, b-3, 1);
   (void)pw_vector(sb->pixwin, l+1, b-2, l+1, b-4, PIX_SET, 1);
   (void)pw_vector(sb->pixwin, l+2, b-1, l+2, b-5, PIX_SET, 1);
   (void)pw_vector(sb->pixwin, l+3, b,   l+3, b-6, PIX_SET, 1);

   /* right triangle */
   (void)pw_put(sb->pixwin, r, b-3, 1);
   (void)pw_vector(sb->pixwin, r-1, b-2, r-1, b-4, PIX_SET, 1);
   (void)pw_vector(sb->pixwin, r-2, b-1, r-2, b-5, PIX_SET, 1);
   (void)pw_vector(sb->pixwin, r-3, b,   r-3, b-6, PIX_SET, 1);
}

/*ARGSUSED*/
static void
paint_v_arrow(sb, left, top)
scrollbar_handle sb;
short left, top;
{
   register int /*tb_margin, r, */ t, b, l;

   t = top + 2;
   b = top + sb->button_length - 3;
   l = (sb->rect.r_width - 7) / 2;

   /* up triangle */
   (void)pw_put(sb->pixwin, l+3, t, 1);
   (void)pw_vector(sb->pixwin, l+2, t+1, l+4, t+1, PIX_SET, 1);
   (void)pw_vector(sb->pixwin, l+1, t+2, l+5, t+2, PIX_SET, 1);
   (void)pw_vector(sb->pixwin, l,   t+3, l+6, t+3, PIX_SET, 1);

   /* down triangle */
   (void)pw_put(sb->pixwin, l+3, b, 1);
   (void)pw_vector(sb->pixwin, l+2, b-1, l+4, b-1, PIX_SET, 1);
   (void)pw_vector(sb->pixwin, l+1, b-2, l+5, b-2, PIX_SET, 1);
   (void)pw_vector(sb->pixwin, l,   b-3, l+6, b-3, PIX_SET, 1);
}

/**************************************************************************/
/* scrollbar_paint_all_clear                                              */
/**************************************************************************/

int
scrollbar_paint_all_clear(pixwin)
struct pixwin	*pixwin;
{
   register scrollbar_handle	sb;

   SCROLLBAR_FOR_ALL(sb) {
      if (pixwin == sb->pixwin) (void)scrollbar_paint_clear((Scrollbar)(LINT_CAST(sb)));
   }
}

int
scrollbar_paint_all(pixwin)
struct pixwin	*pixwin;
{
   register scrollbar_handle	sb;

   SCROLLBAR_FOR_ALL(sb) {
      if (pixwin == sb->pixwin) (void)scrollbar_paint((Scrollbar)(LINT_CAST(sb)));
   }
}

/**************************************************************************/
/* scrollbar_compute_bubble_rect                                          */
/* the bubble rect is relative to the scrollbar's region, NOT the         */
/* scrollbar's rect (which is relative to the underlying pixwin).         */
/**************************************************************************/

scrollbar_compute_bubble_rect(sb, result)
register scrollbar_handle	 sb;
register struct rect		*result;
{
   register int  bubble_length; 
   register int  bubble_offset; 
   register int  major_axis_length; 
   register int  minor_axis_length; 
   register int  min_bubble_length = MIN_BUBBLE_LENGTH; 

   if (sb->horizontal) {
      major_axis_length = sb->rect.r_width;
      minor_axis_length = sb->rect.r_height;
   } else {
      major_axis_length = sb->rect.r_height;
      minor_axis_length = sb->rect.r_width;
   }
   major_axis_length -= 2*sb->button_length;
   if (major_axis_length < MIN_BUBBLE_LENGTH)
      min_bubble_length = major_axis_length;

/* round bubble_offset down & bubble_length up */
   bubble_offset = (double)sb->view_start  * major_axis_length / 
                   (double)sb->object_length;
   bubble_length = sb->view_length * major_axis_length;
   bubble_length =(bubble_length + (double)sb->object_length - 1)/ 
                  (double)sb->object_length;
   
   major_axis_length += sb->button_length;
   bubble_offset     += sb->button_length;

   if (bubble_offset < sb->button_length) 
      bubble_offset = sb->button_length;
   if (bubble_offset + min_bubble_length > major_axis_length) 
      bubble_offset = major_axis_length - min_bubble_length;

   if (bubble_length < min_bubble_length) 
      bubble_length = min_bubble_length;
   if (bubble_offset + bubble_length > major_axis_length) 
      bubble_length = major_axis_length - bubble_offset;
   
   /* Leave at least one pixel, if view_start is not at file start */   
   if ((sb->view_start != 0) && (bubble_offset == sb->button_length))
   	bubble_offset++;
   	
   if (sb->horizontal) {
      result->r_left   = bubble_offset;
      result->r_top    = sb->bubble_margin;
      result->r_width  = bubble_length;
      result->r_height = minor_axis_length - 2*sb->bubble_margin;

      if (sb->border) 
	 result->r_height--;
      if (sb->gravity == SCROLL_MAX) 
         result->r_top++;
      if (result->r_height < 0)
      	 result->r_height = 0;
   } else {
      result->r_top    = bubble_offset;
      result->r_left   = sb->bubble_margin;
      result->r_height = bubble_length;
      result->r_width  = minor_axis_length - 2*sb->bubble_margin;

      if (sb->border) 
	 result->r_width--;
      if (sb->gravity == SCROLL_MAX) 
         result->r_left++;
      if (result->r_width < 0)
         result->r_width = 0;
   }
}
