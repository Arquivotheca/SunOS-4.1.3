#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)panel_util.c 1.1 92/07/30";
#endif
#endif

/*****************************************************************************/
/*                          panel_util.c                                     */
/*           Copyright (c) 1985 by Sun Microsystems, Inc.                    */
/*****************************************************************************/

#include <suntool/panel_impl.h>
#include <sunwindow/sun.h>

static short empty_image[] = { 0 };
static mpr_static(panel_empty_pr, 0, 0, 1, empty_image);

static unsigned short	gray12_data[16] = {	/* 12 % grey pattern */
			0x8080, 0x1010, 0x0202, 0x4040,
			0x0808, 0x0101, 0x2020, 0x0404,
			0x8080, 0x1010, 0x0202, 0x4040,
			0x0808, 0x0101, 0x2020, 0x0404
		};
mpr_static(panel_shade_pr, 16, 16, 1, gray12_data);

static u_short  gray17_data[16] = { /*  really 16-2/3   */
        0x8208, 0x2082, 0x0410, 0x1041,
        0x4104, 0x0820, 0x8208, 0x2082,
        0x0410, 0x1041, 0x4104, 0x0820,
        0x8208, 0x2082, 0x0410, 0x1041
};
mpr_static(panel_gray17_pr, 12, 12, 1, gray17_data);

/* caret handling functions */
extern	void	(*panel_caret_on_proc)();
extern	void	(*panel_caret_invert_proc)();

/* selection service functions */
extern	void	(*panel_seln_inform_proc)();
extern	void	(*panel_seln_destroy_proc)();


/*****************************************************************************/
/* panel_enclosing_rect                                                      */
/*****************************************************************************/

Rect
panel_enclosing_rect(r1, r2)
register Rect *r1, *r2;
{
   /* if r2 is undefined then return r1 */
   if (r2->r_left == -1)
      return(*r1);

   return rect_bounding(r1, r2);
}

/*****************************************************************************/
/* font char/pixel conversion routines                                       */
/*****************************************************************************/

panel_col_to_x(font, col)
Pixfont *font;
int 		col;
{
   struct pr_size size;

   size = pf_textwidth(1,font,"n");
   return (col * size.x);
}


panel_row_to_y(font, line)
Pixfont *font;
int 		line;
{
   return (line * font->pf_defaultsize.y);
}


panel_x_to_col(font, x)
Pixfont *font;
int 		x;
{
   struct pr_size size;

   size = pf_textwidth(1,font,"n");
   return(x / font->pf_defaultsize.x); 
}


panel_y_to_row(font, y)
Pixfont *font;
int y;
{
   return(y / font->pf_defaultsize.y); 
}


/*****************************************************************************/
/* panel_make_image                                                           */
/* if value is NULL, use "" or the empty pixrect instead.
/*****************************************************************************/

struct pr_size
panel_make_image(font, dest, type_code, value, bold_desired, shaded_desired)
Pixfont 		*font;
panel_image_handle 	 dest; 
int 			 type_code;
caddr_t 		 value;
int 			 bold_desired;
int 			 shaded_desired;
{
   struct pr_size size;
   char           *str;

   size.x = size.y = 0;
   dest->im_type = type_code;
   image_set_shaded(dest, shaded_desired);
   switch (type_code) {
      case IM_STRING:
	 if (!value)
	    value = (caddr_t) "";
	 if (!(str = panel_strsave(value)))
	    return(size);
	 image_set_string(dest, str);
	 image_set_font(dest, font);
	 image_set_bold(dest, bold_desired);
	 size = pf_textwidth(strlen(str), font, str);
	 if (bold_desired)
	    size.x += 1;
	 break;

       case IM_PIXRECT:	
	 if (!value)
	    value = (caddr_t) &panel_empty_pr;
	 image_set_pixrect(dest, (Pixrect *) LINT_CAST(value));
	 size = ((Pixrect *) LINT_CAST(value))->pr_size;
	 break;
   }
   return size;
}

/*****************************************************************************/
/* panel_successor -- returns the next unhidden item after ip.                */
/*****************************************************************************/

panel_item_handle
panel_successor(ip)
register panel_item_handle  ip;
{
   if (!ip)
      return NULL;

   for (ip = ip->next; ip && hidden(ip); ip = ip->next);

   return ip;
}

/*****************************************************************************/
/* panel_append                                                              */
/*****************************************************************************/

panel_item_handle
panel_append(ip)
register panel_item_handle ip;
{
   panel_handle			panel = ip->panel;
   register panel_item_handle	ip_cursor;
   Rect				deltas;

   if (!panel->items)
      panel->items =  ip;
   else {
      for (ip_cursor = panel->items;
	   ip_cursor->next != NULL;
	   ip_cursor = ip_cursor->next);
      ip_cursor->next = ip;
   }
   ip->next = NULL;

   /* item rect encloses the label & value */
   ip->rect = panel_enclosing_rect(&ip->label_rect, &ip->value_rect);

   /* move the item if not fixed and past the right edge */
   if (!(item_fixed(ip) || label_fixed(ip) || value_fixed(ip))) {
      /* only move it down if its advantageous, i.e. the item is */
      /* not in column 0 */
      if ((ip->rect.r_left > 0) &&
	  (rect_right(&ip->rect) + panel->v_bar_width > 
	   rect_right(&panel->rect))) {
	 deltas.r_left = PANEL_ITEM_X_START - ip->rect.r_left;
	 deltas.r_top = panel->max_item_y + panel->item_y_offset;
	 /* tell the item to move */
	 (void)panel_layout(ip, &deltas);
      }
   }

   /* for scrolling computations, note new extent of panel, tell scrollbars */
   panel_update_extent(ip->panel, ip->rect);

   /* determine the next default position */
   (void)panel_find_default_xy(panel);

   return (ip) ;
}


/*****************************************************************************/
/* panel_find_default_xy                                                     */
/* computes panel->item_x, panel->item_y, and panel->max_item_y based on the */
/* geometry of the current items in the panel.                               */
/* First the lowest "row" is found, then the default position is on that     */
/* row to the right of any items which intersect that row.                   */
/* The max_item_y is set to the height of the lowest item rectangle on the   */
/* lowest row.                                                               */
/*****************************************************************************/

panel_find_default_xy(panel)
panel_handle	panel;
{
   register panel_item_handle	ip;
   register int			lowest_top      = PANEL_ITEM_Y_START;
   register int			rightmost_right = PANEL_ITEM_X_START;
   register int			lowest_bottom   = PANEL_ITEM_Y_START;

   if (!panel->items) {
      panel->max_item_y = 0;
      panel->item_x = PANEL_ITEM_X_START;
      panel->item_y = PANEL_ITEM_Y_START;
      return;
   }

   /* find the lowest row */
   for (ip = panel->items; ip; ip = ip->next) {
      lowest_top = max(lowest_top, ip->rect.r_top);
      lowest_bottom = max(lowest_bottom, rect_bottom(&ip->rect));
   }

   /* find the rightmost position on the row */
   for (ip = panel->items; ip; ip = ip->next)
      if (rect_bottom(&ip->rect) >= lowest_top)
	 rightmost_right = max(rightmost_right, rect_right(&ip->rect));

   /* update the panel info */
   panel->max_item_y = lowest_bottom - lowest_top; /* offset to next row */
   panel->item_x = rightmost_right + panel->item_x_offset;
   panel->item_y = lowest_top;

   /* advance to the next row if past right edge */
   if (panel->item_x > panel->rect.r_width) {
      panel->item_x = PANEL_ITEM_X_START;
      panel->item_y += panel->max_item_y + panel->item_y_offset;
      panel->max_item_y = 0;
   }
}


/*****************************************************************************/
/* panel_layout                                                               */
/* lays out the generic item, label & value rects in ip and calls the item's */
/* layout proc.                                                              */
/*****************************************************************************/

panel_layout(ip, deltas)
register panel_item_handle	ip;
register Rect			*deltas;
{
   /* item rect */
   ip->rect.r_left += deltas->r_left;
   ip->rect.r_top += deltas->r_top;

   /* label rect */
   ip->label_rect.r_left += deltas->r_left;
   ip->label_rect.r_top += deltas->r_top;

   /* value rect */
   ip->value_rect.r_left += deltas->r_left;
   ip->value_rect.r_top += deltas->r_top;

   /* item */
   (*ip->ops->layout)(ip, deltas);
} /* panel_layout */

/*****************************************************************************/
/* panel_paint_image                                                         */
/* paints image in pw in rect.                                               */
/*****************************************************************************/

panel_paint_image(panel, image, rect, op)
panel_handle		panel;
panel_image_handle	image;
Rect 			*rect;
int			op;
{
   int top;

   switch (image->im_type) {
      case IM_STRING:
	 /* baseline offset */
	 top = rect->r_top + panel_fonthome(image_font(image)); 
	 (void)panel_pw_text(panel, rect->r_left, top, PIX_SRC|op, 
  		       image_font(image), image_string(image));
	 if (image_bold(image))
	    (void)panel_pw_text(panel, rect->r_left+1, top, 
			  PIX_SRC|PIX_DST|op, 
		          image_font(image), image_string(image));
	 break;

      case IM_PIXRECT:
         (void)panel_pw_write(panel, rect->r_left, rect->r_top,
		        image_pixrect(image)->pr_width, 
		        image_pixrect(image)->pr_height,
	                PIX_SRC|op, image_pixrect(image), 0, 0);
	 break;
   }
   if (image_shaded(image))
      (void)panel_shade(panel, rect);
}

/*****************************************************************************/
/* panel_paint_pixrect                                                       */
/*****************************************************************************/

panel_paint_pixrect(panel, pr, rect, op)
panel_handle	panel;
Pixrect 	*pr;
Rect    	*rect;
int		op;

{
   (void)panel_pw_write(panel, rect->r_left, rect->r_top,
	    pr->pr_width, pr->pr_height,
	    PIX_SRC|op, pr, 0, 0);
}


/*****************************************************************************/
/* panel versions of writing and locking routines, with scrolling offsets    */
/*****************************************************************************/

static 
scroll_adjust_rect(panel, rect, adjusted_rect)
panel_handle panel;
Rect *rect, *adjusted_rect;
{
   *adjusted_rect         = *rect;
   adjusted_rect->r_top  -= panel->v_offset;
   adjusted_rect->r_left -= panel->h_offset;
}

panel_pw_lock(panel, rect)
panel_handle	panel;
Rect		*rect;
{
   Rect adjusted_rect;

   scroll_adjust_rect(panel, rect, &adjusted_rect);
   (void)pw_lock(panel->view_pixwin, &adjusted_rect);
}

panel_draw_box(panel, op, r, x, y)
panel_handle   panel;
int            op;
Rect           *r;
int            x, y;
{
   Rect adjusted_rect;

   scroll_adjust_rect(panel, r, &adjusted_rect);
   (void)_tool_draw_box(panel->view_pixwin, op, &adjusted_rect, x, y);
}

panel_pw_write(panel, xd, yd, width, height, op, pr, xs, ys)
register panel_handle   panel;
int            		xd, yd, width, height, op;
Pixrect        		*pr;
int            		xs, ys;
{
   (void)pw_write(panel->view_pixwin, 
	    xd - panel->h_offset, yd - panel->v_offset, 
	    width, height, op, pr, xs, ys);
}

panel_pw_writebackground(panel, xd, yd, width, height, op)
register panel_handle   panel;
int            		xd, yd, width, height, op;
{
   (void)pw_writebackground(panel->view_pixwin, 
	              xd - panel->h_offset, yd - panel->v_offset, 
                      width, height, op);
}

panel_pw_replrop(panel, xd, yd, width, height, op, pr, xs, ys)
panel_handle   panel;
int            xd, yd, width, height, op;
Pixrect	       *pr;
int            xs, ys;
{
   (void)pw_replrop(panel->view_pixwin,
	      xd - panel->h_offset, yd - panel->v_offset, 
	      width, height, op, pr, xs, ys);
}

panel_pw_vector(panel, x0, y0, x1, y1, op, value)
register panel_handle   panel;
int            		x0, y0, x1, y1, op, value;
{
   (void)pw_vector(panel->view_pixwin, x0 - panel->h_offset, y0 - panel->v_offset,
	     x1 - panel->h_offset, y1 - panel->v_offset, op, value);
}

panel_pw_text(panel, x, y, op, font, s)
panel_handle   panel;
int            x, y, op;
Pixfont       *font;
char          *s;
{
   (void)pw_text(panel->view_pixwin, 
	   x - panel->h_offset, y - panel->v_offset, op, font, s);
}

panel_pw_ttext(panel, x, y, op, font, s)
panel_handle   panel;
int            x, y, op;
Pixfont       *font;
char          *s;
{
   (void)pw_ttext(panel->view_pixwin, 
	   x - panel->h_offset, y - panel->v_offset, op, font, s);
}

/*****************************************************************************/
/* panel_invert                                                              */
/* inverts the rect r using panel's pixwin.                                  */
/*****************************************************************************/

panel_invert(panel, r)
panel_handle	panel;
register Rect	*r;
{
   (void)panel_pw_writebackground(panel, r->r_left, r->r_top, 
		            r->r_width, r->r_height, PIX_NOT(PIX_DST));
}

panel_shade(panel, r)
panel_handle	panel;
register Rect	*r;
{
   (void)panel_pw_replrop(panel, r->r_left, r->r_top, r->r_width, r->r_height,
	            PIX_SRC | PIX_DST, &panel_shade_pr, 0, 0);
}

panel_gray(panel, r)
panel_handle    panel;
register Rect   *r;
{
   (void)panel_pw_replrop(panel, r->r_left, r->r_top, r->r_width,
        r->r_height, PIX_SRC | PIX_DST, &panel_gray17_pr, 0, 0);
   (void)panel_pw_writebackground(panel, r->r_left, r->r_top,
        r->r_width, r->r_height, PIX_NOT(PIX_DST));
}

panel_clear(panel, r)
panel_handle	panel;
register Rect	*r;
{
   (void)panel_pw_writebackground(panel, r->r_left, r->r_top, 
		            r->r_width, r->r_height, PIX_CLR);
}

/*****************************************************************************/
/* panel_strsave                                                             */
/*****************************************************************************/

char *
panel_strsave(source)
char *source;
{
   char *dest;
   extern char *strcpy();

   dest = (char *) malloc((u_int) (strlen(source) + 1));
   if (!dest)
      return NULL;

   (void) strcpy(dest, source);
   return dest;
}

/*****************************************************************************/
/* miscellaneous utilities                                                   */
/*****************************************************************************/

panel_fonthome(font) Pixfont *font; {
   return -(font->pf_char['n'].pc_home.y);
}

panel_nullproc() { 
   return 0; 
}

void
panel_caret_on(panel, on)
panel_handle	panel;
int		on;
{
   (*panel_caret_on_proc)(panel, on);
}

void
panel_caret_invert(panel)
panel_handle	panel;
{
   (*panel_caret_invert_proc)(panel);
}


void
panel_seln_inform(panel, event)
panel_handle	panel;
Event		*event;
{
   (*panel_seln_inform_proc)(panel, event);
}


void
panel_seln_destroy(panel)
panel_handle	panel;
{
   (*panel_seln_destroy_proc)(panel);
}


panel_free_choices(choices, first, last)
panel_image_handle choices;
int		   first, last;
{
   register int i;		/* counter */

   if (!choices || last < 0)
      return;

   for (i = first; i <= last; i++)	/* free the choice strings */
      if (is_string(&choices[i]))
	 free(image_string(&choices[i]));

   free((char *) choices);
} /* panel_free_choices */
