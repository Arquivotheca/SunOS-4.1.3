#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)canvas_scroll.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <suntool/canvas_impl.h>

#define canvas_set_sb(canvas, direction, sb)	\
    canvas_sb(canvas, direction) = sb

static void	canvas_adjust_scrollbar_rects();
static void	canvas_region();
static void	scroll_pixwin();
void		pw_set_y_offset(),pw_set_x_offset();


void
canvas_setup_scrollbar(canvas, direction, sb)
register Canvas_info	*canvas;
Scrollbar_setting	direction;
Scrollbar             sb;
{
   register Pixwin	*pw	= (Pixwin *) (LINT_CAST(window_get(
   		(Window)(LINT_CAST(canvas)), WIN_PIXWIN)));
   register Scrollbar	old_sb;
   register int		is_vertical = direction == SCROLL_VERTICAL;

   old_sb = canvas_sb(canvas, direction);
  /* erase the old scrollbar's knowledge of the canvas */
  /*
  if ((int(*)())scrollbar_get(old_sb,SCROLL_MODIFY_PROC) == canvas_sb_modify)
     (void)scrollbar_set(old_sb, SCROLL_MODIFY_PROC, 0, 0);
  */
  if (scrollbar_get(old_sb, SCROLL_NOTIFY_CLIENT) == (caddr_t) canvas)
     (void)scrollbar_set(old_sb, SCROLL_NOTIFY_CLIENT, 0, 0);

   canvas_set_sb(canvas, direction, sb);

   if (sb) {

      /* this call lets the scrollbar know about the canvas,
       * sets the scrollbar's direction, and
       * initializes the scrollbar's idea of the viewing window
       */
      (void)scrollbar_set(sb,
		    SCROLL_DIRECTION, direction,
		    SCROLL_NOTIFY_CLIENT, canvas,
		    /*
		    SCROLL_MODIFY_PROC,   canvas_sb_modify,
		    */
		    SCROLL_VIEW_START,    is_vertical ? 
			canvas_y_offset(canvas) : canvas_x_offset(canvas),
		    SCROLL_OBJECT_LENGTH, is_vertical ?
			canvas_height(canvas) : canvas_width(canvas),
		    0);
   }

   canvas_adjust_scrollbar_rects(canvas, pw);

   /* adjust regions to reflect presence or absence of new scrollbar */
   (void)canvas_resize_pixwin(canvas, FALSE);
}

static void
canvas_adjust_scrollbar_rects(canvas, pw)
Canvas_info	*canvas;
Pixwin		*pw;
{
   register Scrollbar	v_sb	= canvas_sb(canvas, SCROLL_VERTICAL);
   register Scrollbar	h_sb	= canvas_sb(canvas, SCROLL_HORIZONTAL);

   register Scrollbar_setting	v_gravity = (Scrollbar_setting)
       scrollbar_get(v_sb, SCROLL_PLACEMENT);
   register int	v_width = (int) scrollbar_get(v_sb, SCROLL_THICKNESS);

   register Scrollbar_setting	h_gravity = (Scrollbar_setting)
       scrollbar_get(h_sb, SCROLL_PLACEMENT);
   register int	h_width = (int) scrollbar_get(h_sb, SCROLL_THICKNESS);

   register int		inner_width	= win_width(canvas) - v_width;
   register int		inner_height	= win_height(canvas) - h_width;

   if (v_sb && h_sb) {
      /* first set the height and width, which don't depend on the gravity */
      (void)scrollbar_set(v_sb, SCROLL_HEIGHT, inner_height, 0);
      (void)scrollbar_set(h_sb, SCROLL_WIDTH, inner_width, 0);

      if (v_gravity == SCROLL_EAST && h_gravity == SCROLL_NORTH) {
	 (void)scrollbar_set(v_sb, 
		       SCROLL_TOP,  h_width /*- 1*/, 
	               SCROLL_LEFT, inner_width,
		       0);
	 (void)scrollbar_set(h_sb, 
		       SCROLL_TOP,  0, 
		       SCROLL_LEFT, 0, 
		       0);
	 (void)pw_writebackground(pw, win_width(canvas), 0,
			    v_width, h_width, 
			    PIX_CLR);
      } else if (v_gravity == SCROLL_EAST && h_gravity == SCROLL_SOUTH) {
	 (void)scrollbar_set(v_sb, 
		       SCROLL_TOP,  0, 
	               SCROLL_LEFT, inner_width,
		       0);
	 (void)scrollbar_set(h_sb, 
		       SCROLL_TOP,  inner_height,
		       SCROLL_LEFT, 0, 
		       0);
	 (void)pw_writebackground(pw, 
			    win_width(canvas), win_height(canvas),
			    v_width, h_width, 
			    PIX_CLR);
      } else if (v_gravity == SCROLL_WEST && h_gravity == SCROLL_NORTH) {
	 (void)scrollbar_set(v_sb, 
		       SCROLL_TOP,  h_width /*- 1*/, 
	               SCROLL_LEFT, 0,
		       0);
	 (void)scrollbar_set(h_sb, 
		       SCROLL_TOP,  0, 
		       SCROLL_LEFT, v_width /*- 1*/, 
		       0);
	 (void)pw_writebackground(pw, 0, 0,
			    v_width, h_width, 
			    PIX_CLR);
      } else if (v_gravity == SCROLL_WEST && h_gravity == SCROLL_SOUTH) {
	 (void)scrollbar_set(v_sb, 
		       SCROLL_TOP,  0, 
	               SCROLL_LEFT, 0,
		       0);
	 (void)scrollbar_set(h_sb, 
		       SCROLL_TOP,  inner_height,
		       SCROLL_LEFT, v_width /*- 1*/, 
		       0);
	 (void)pw_writebackground(pw, 0, win_height(canvas) /*+ 1*/,
			    v_width, h_width, 
			    PIX_CLR);
      }
   } else if (h_sb)
      (void)scrollbar_set(h_sb, SCROLL_LEFT, 0, SCROLL_WIDTH, win_width(canvas), 0);
   else if (v_sb)
      (void)scrollbar_set(v_sb, SCROLL_TOP, 0, SCROLL_HEIGHT, win_height(canvas), 0);

   /* reset the win_length of both bars to make sure its right */
   (void)scrollbar_set(v_sb, SCROLL_VIEW_LENGTH, inner_height, 0);
   (void)scrollbar_set(h_sb, SCROLL_VIEW_LENGTH, inner_width, 0);
}


/* compute and open the canvas viewing pixwin based
 * on the canvas width, height, margin, and scrollbars.
 */
void
canvas_open_region(canvas, pw)
Canvas_info	*canvas;
Pixwin		*pw;
{
    register int 	x, y, width, height;
    Scrollbar		v_sb	= canvas_sb(canvas, SCROLL_VERTICAL);
    Scrollbar		h_sb	= canvas_sb(canvas, SCROLL_HORIZONTAL);
    register int	v_width = (int) scrollbar_get(v_sb, SCROLL_THICKNESS);
    register int	h_width = (int) scrollbar_get(h_sb, SCROLL_THICKNESS);

   register Scrollbar_setting v_gravity = (Scrollbar_setting)
      scrollbar_get(v_sb, SCROLL_PLACEMENT);

   register Scrollbar_setting h_gravity = (Scrollbar_setting)
      scrollbar_get(h_sb, SCROLL_PLACEMENT);

   x = v_gravity == SCROLL_WEST ? v_width : 0;
   y = h_gravity == SCROLL_NORTH ? h_width : 0;

    /* account for a margin on the left, right, top, bottom */
    x += canvas->margin;
    y += canvas->margin;

    width = win_width(canvas) - v_width - 2 * canvas->margin;
    height = win_height(canvas) - h_width - 2 * canvas->margin;

    /* don't let the margin mess up the
     * view pixwin.
     * Probably should complain here.
     */
    if (width < 0)
	width = 0;

    if (height < 0)
	height = 0;

   canvas_region(canvas, pw, x, y, width, height);

}

void
canvas_update_scrollbars(canvas, repaint)
register Canvas_info	*canvas;
short	repaint;
{
   register Pixwin	*pw	= (Pixwin *) (LINT_CAST(window_get(
   			(Window)(LINT_CAST(canvas)), WIN_PIXWIN)));
   Scrollbar		v_sb	= canvas_sb(canvas, SCROLL_VERTICAL);
   Scrollbar		h_sb	= canvas_sb(canvas, SCROLL_HORIZONTAL);

   /* first place the scrollbars correctly within the panel */
   canvas_adjust_scrollbar_rects(canvas, pw);

   /* now tell the scrollbars of the current size of the viewing window */
   (void)scrollbar_set(v_sb,
		 SCROLL_VIEW_START,     canvas_y_offset(canvas),
		 SCROLL_OBJECT_LENGTH,  canvas_height(canvas),
		 0);
   (void)scrollbar_set(h_sb,
		 SCROLL_VIEW_START,     canvas_x_offset(canvas),
		 SCROLL_OBJECT_LENGTH,  canvas_width(canvas),
		 0);

   /* finally, paint the scrollbars */
   if (repaint) {
       (void)scrollbar_paint_clear(v_sb);
       (void)scrollbar_paint_clear(h_sb);
    }
}


/* scroll the canvas according to LAST_VIEW_START and 
 * VIEW_START.
 */
void
canvas_scroll(canvas, sb)
register Canvas_info	*canvas;
Scrollbar	sb;
{
    int		offset	   = (int) scrollbar_get(sb, SCROLL_VIEW_START);
    int		old_offset = (int) scrollbar_get(sb, SCROLL_LAST_VIEW_START);
    int		is_vertical = 
	(Scrollbar_setting) scrollbar_get(sb, SCROLL_DIRECTION) == SCROLL_VERTICAL;
    /*
     * NOTE: max_offset is 0 if CANVAS_AUTO_SHRINK is set to 0
     *       which is the default.  In this case offset will likely
     *	     be set to 0, which will cause the canvas not to scroll
     *	     at all. This is likely to produce some counter-intuitive
     *	     behavior in the user program.  Should CANVAS_AUTO_SHRINK 
     *	     be defaulted to FALSE?
     */
    int		max_offset = is_vertical ? 
		    (canvas_height(canvas) - view_height(canvas)) :
		    (canvas_width(canvas) - view_width(canvas));
	    
    /* fill the clipping list with the
     * exposed regions of the canvas view pixwin.
     */
    (void)pw_exposed(canvas->pw);

    if (max_offset < 0)
	max_offset = 0;

    if (offset > max_offset) {
	offset = max_offset;
	(void)scrollbar_set(sb, SCROLL_VIEW_START, offset, 0);
    }

    if (offset == old_offset)
	return;

    if (!status(canvas, retained))
	scroll_pixwin(canvas, old_offset, offset, is_vertical);
    else {
	if (is_vertical)
	    pw_set_y_offset(canvas->pw, offset);
	else
	    pw_set_x_offset(canvas->pw, offset);
	canvas_restrict_clipping(canvas);
    }

    (void)scrollbar_paint(sb);
}



/* scroll the bits in the canvas pixwin
 * from old_offset to new_offset.  Ask the
 * client to repaint the new bits.
 */
static void
scroll_pixwin(canvas, old_offset, new_offset, is_vertical)
register Canvas_info	*canvas;
register int		old_offset, new_offset;
register int		is_vertical;
{
    register int     	delta;
    Rect		new_rect;
    Rectlist  		rl;
    int			old_x_offset	= pw_get_x_offset(canvas->pw);
    int			old_y_offset	= pw_get_y_offset(canvas->pw);

    delta = new_offset > old_offset ? 
	    new_offset - old_offset : old_offset - new_offset;

    rl = rl_null;
    /* copy the old bits */
    if (delta <= (is_vertical ? view_height(canvas) : view_width(canvas))) {
	/* for simplicity, reset the offsets of the
	 * viewing pixwin so we can draw in view-pixwin space.
	 */
	pw_set_x_offset(canvas->pw, 0);
	pw_set_y_offset(canvas->pw, 0);

	if (new_offset > old_offset) {
	    if (is_vertical) {
		(void)pw_copy(canvas->pw,
			0, 0,
			view_width(canvas), view_height(canvas) - delta, 
			PIX_SRC, canvas->pw, 0, delta);
		rect_construct(&new_rect, 0, view_height(canvas) - delta, 
			       view_width(canvas), delta);
	    } else {
		(void)pw_copy(canvas->pw,
			0, 0,
			view_width(canvas) - delta, view_height(canvas), 
			PIX_SRC, canvas->pw, delta, 0);
		rect_construct(&new_rect, view_width(canvas) - delta, 0, 
			       delta, view_height(canvas));
	    }
	} else if (new_offset < old_offset) {
	    if (is_vertical) {
		(void)pw_copy(canvas->pw,
			0, delta,
			view_width(canvas), view_height(canvas) - delta,
			PIX_SRC, canvas->pw, 0, 0);
		rect_construct(&new_rect, 0, 0, view_width(canvas), delta);
	    } else {
		(void)pw_copy(canvas->pw,
			delta, 0,
			view_width(canvas) - delta, view_height(canvas),
			PIX_SRC, canvas->pw, 0, 0);
		rect_construct(&new_rect, 0, 0, delta, view_height(canvas));
	    }
	}
	/* include the bits not copied by
	 * pw_copy() to be cleint-repaired.
	 */
	(void)rl_union(&rl, &(canvas->pw->pw_fixup), &rl);

	/* restore the old translation */
	pw_set_x_offset(canvas->pw, old_x_offset);
	pw_set_y_offset(canvas->pw, old_y_offset);
    } else {
	/* paint the whole view pixwin */
	rect_construct(&new_rect, 0, 0, view_width(canvas), 
		       view_height(canvas));
    }
	
    (void)rl_rectunion(&new_rect, &rl, &rl);

    if (is_vertical)
	pw_set_y_offset(canvas->pw, new_offset);
    else
	pw_set_x_offset(canvas->pw, new_offset);

    /* translate to canvas space */
    rl_passtoparent(canvas_x_offset(canvas), canvas_y_offset(canvas), &rl);
    (void)rl_normalize(&rl);

    canvas_request_repaint(canvas, &rl, TRUE);
}


/* ARGSUSED */
static void
canvas_region(canvas, pw, x, y, width, height)
Canvas_info	*canvas;
Pixwin		*pw;
int		x, y, width, height;
{
    Rect	rect;

    rect_construct(&rect, x, y, width, height);

    (void)pw_set_region_rect(canvas->pw, &rect, TRUE);
}
