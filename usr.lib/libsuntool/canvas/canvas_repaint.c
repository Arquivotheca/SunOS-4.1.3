#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)canvas_repaint.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <suntool/canvas_impl.h>

static void		compute_damage();
static void		clear_rectlist();

/* repaint the exposed areas of the canvas pixwin, including
 * the scrollbars and the view pixwin.
 */
void
canvas_repaint_window(canvas)
register Canvas_info	*canvas;
{
    Pixwin	*pw = (Pixwin *) (LINT_CAST(window_get(
    		(Window)(LINT_CAST(canvas)), WIN_PIXWIN)));
    Rectlist	canvas_damage, window_damage;
    int		repaint_v_sb, repaint_h_sb;

    /* compute the window & canvas damage */
    compute_damage(canvas, pw, &window_damage, &canvas_damage,
	&repaint_v_sb, &repaint_h_sb);
    
    /* clear the damaged window area.  The window area is that area of the
     * subwindow's pixwin not covered by the scrollbars or viewing pixwin.
     * Note that this has a side effect of altering the clipping list for pw.
     */
     clear_rectlist(pw, &window_damage);
     
    /* repaint the scrollbars with clearing */
    if (repaint_v_sb)
	(void)scrollbar_paint_clear(canvas_sb(canvas, SCROLL_VERTICAL));
    if (repaint_h_sb)
	(void)scrollbar_paint_clear(canvas_sb(canvas, SCROLL_HORIZONTAL));

    /* repaint the view pixwin */
    canvas_request_repaint(canvas, &canvas_damage, TRUE);

    status_reset(canvas, dirty);

    (void)rl_free(&window_damage);
}


/* compute the window and canvas damage for pw.
 * window damage is in window space, canvas damage
 * is in canvas space.
 */
static void
compute_damage(canvas, pw, window_damage, canvas_damage, 
	       repaint_v_sb, repaint_h_sb)
register Canvas_info	*canvas;
Pixwin			*pw;
register Rectlist	*window_damage, *canvas_damage;
int			*repaint_v_sb, *repaint_h_sb;
{
    Rect	*sb_rect;	/* scroll bar rect */
    Scrollbar	 sb;
    
    /* initialize both rectlists to
     * the pixwin's damaged rectlist.
     */
    *window_damage = rl_null;
    *canvas_damage = rl_null;
    (void)rl_copy(&pw->pw_clipdata->pwcd_clipping, canvas_damage);
    (void)rl_copy(canvas_damage, window_damage);

    /* compute the damaged canvas rectlist.
     * Translate the rect list to canvas coordinates.
     */
    (void)rl_rectintersection(view_rect(canvas), canvas_damage, canvas_damage);
    
    /* translate to viewing pixwin space */
    rl_passtochild(view_left(canvas), view_top(canvas), canvas_damage);
    
    /* translate to canvas space */
    rl_passtoparent(canvas_x_offset(canvas), canvas_y_offset(canvas),
		    canvas_damage);
		    
    (void)rl_normalize(canvas_damage);
    
    /* remove view pixwin damage from window damage */
    (void)rl_rectdifference(view_rect(canvas), window_damage, window_damage);

    /* remove scrollbar damage from the window damage */
    *repaint_v_sb = *repaint_h_sb = FALSE;
    if (sb = canvas_sb(canvas, SCROLL_VERTICAL)) {
        sb_rect = (Rect *) (LINT_CAST(scrollbar_get(sb, SCROLL_RECT)));
	if (rl_rectintersects(sb_rect, window_damage)) {
	    *repaint_v_sb = TRUE;
	    (void)rl_rectdifference(sb_rect, window_damage, window_damage);
	}
    }
    if (sb = canvas_sb(canvas, SCROLL_HORIZONTAL)) {
        sb_rect = (Rect *) (LINT_CAST(scrollbar_get(sb, SCROLL_RECT)));
	if (rl_rectintersects(sb_rect, window_damage)) {
	    *repaint_h_sb = TRUE;
	    (void)rl_rectdifference(sb_rect, window_damage, window_damage);
	}
    }
}


/* repaint the canvas from the retained pixrect,
 * or ask the client to repaint.  Clear the non canvas-backed
 * area of the viewing pixwin.  Repaint_area is assumed to be
 * in canvas space.  Note that repaint_area will be freed on
 * return.
 */
void
canvas_request_repaint(canvas, repaint_area, repaint)
register Canvas_info	*canvas;
register Rectlist	*repaint_area;
register int		repaint;
{

    Rectlist		canvas_area_rectlist;
    register Rectlist	*canvas_area = &canvas_area_rectlist;
    Rect		*r, visible_rect;
    struct pixrect	*retained_pr;
    void		pw_batch();

    /* compute the visible repaint area */
    rect_construct(&visible_rect,
	canvas_x_offset(canvas), canvas_y_offset(canvas), 
	view_width(canvas), view_height(canvas));

    /* restrict the repaint area to the visible repaint area */
    (void)rl_rectintersection(&visible_rect,  repaint_area,  repaint_area);

    /* compute the canvas-backed pixwin area */
    *canvas_area = rl_null;
    (void)rl_rectintersection(canvas_rect(canvas), repaint_area, canvas_area);

    /* restrict repaint_area to be the non canvas-backed pixwin area */
    (void)rl_difference(repaint_area, canvas_area, repaint_area);

    (void)pw_exposed(canvas->pw);

    /* clear the non canvas-backed pixwin area */
    if (repaint_area->rl_bound.r_width && repaint_area->rl_bound.r_height) {
	if (repaint)
	    clear_rectlist(canvas->pw, repaint_area);
	else
	    status_set(canvas, dirty);
    }

    /* only ask for repaint if there is an area to repaint.
     * Also, if we are not allowed to repaint now, set the
     * dirty bit.
     */
    if (canvas_area->rl_bound.r_width && canvas_area->rl_bound.r_height) {
	if (!repaint)
	    status_set(canvas, dirty);
    } else
	/* nothing to repaint */
	repaint = FALSE;

    if (repaint) {
	/* restrict the view pixwin's clipping area 
	 * the the client's repaint area.
	 */
	(void)rl_copy(canvas_area, repaint_area);
	rl_passtochild(canvas_x_offset(canvas), canvas_y_offset(canvas), 
		       repaint_area);
	(void)rl_normalize(repaint_area);
	(void)pw_restrict_clipping(canvas->pw,  repaint_area);

	/* only call repaint proc if there is one and window is installed */
	if (canvas->repaint_proc) {
	    if (status(canvas, first_resize)) {
		/* ask the client to repaint */
		if (status(canvas, auto_clear))
		    clear_rectlist(canvas->pw, canvas_area);

		(*canvas->repaint_proc)(canvas, canvas->pw, canvas_area);
	    }
	} else if (status(canvas, retained)) {
	    Pw_batch_type orig_type = canvas->pw->pw_clipdata->pwcd_batch_type;

	    /* repaint from the retained pixrect */
	    retained_pr = canvas_pr(canvas);

	    /* this will show whatever is batched up */
	    pw_batch(canvas->pw, PW_NONE);
	    canvas_set_pr(canvas, 0);
	    r = &(canvas_area->rl_bound);
	    (void)pw_rop(canvas->pw, r->r_left, r->r_top, r->r_width,
	    	   r->r_height, PIX_SRC, retained_pr, r->r_left, r->r_top);
	    canvas_set_pr(canvas, retained_pr);
	    /* restore the batching */
	    pw_batch(canvas->pw, orig_type);

	} else
	    /* no repaint proc & not retained, so
	     * clear the area to be repainted.
	     */
	    clear_rectlist(canvas->pw, canvas_area);
    }

    (void)rl_free(repaint_area);
    (void)rl_free(canvas_area);

    canvas_restrict_clipping(canvas);
}


/* restrict the clipping of the canvas viewing
 * pixwin to the canvas-backed area.
 * This prevents the client from drawing on the
 * non canvas-backed area.
 */
void
canvas_restrict_clipping(canvas)
Canvas_info	*canvas;
{
    Rectlist	clip_area;

    clip_area = rl_null;
    (void)rl_initwithrect(canvas_rect(canvas), &clip_area);
    rl_passtochild(canvas_x_offset(canvas), canvas_y_offset(canvas), 
		   &clip_area);
    (void)pw_exposed(canvas->pw);
    (void)pw_restrict_clipping(canvas->pw, &clip_area);
    (void)rl_free(&clip_area);
}


static void
clear_rectlist(pw, rl)
register Pixwin	*pw;
Rectlist	*rl;
{
    register Rectnode	*rn;
    
    (void)pw_lock(pw, &rl->rl_bound);
    for (rn = rl->rl_head; rn; rn = rn->rn_next)
        (void)pw_writebackground(pw, rn->rn_rect.r_left, rn->rn_rect.r_top,
		           rn->rn_rect.r_width, rn->rn_rect.r_height,
                           PIX_CLR);
    (void)pw_unlock(pw);
}
