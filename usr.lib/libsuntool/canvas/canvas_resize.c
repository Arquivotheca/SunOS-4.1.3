#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)canvas_resize.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <suntool/canvas_impl.h>

static void		compute_new_area();


/* resize the canvas view pixwin.  Resize the canvas if allowed.
*/
int
canvas_resize_pixwin(canvas, repaint)
register Canvas_info	*canvas;
short			repaint;
{

    register Pixwin	*pw = (Pixwin *) (LINT_CAST(window_get(
    			(Window)(LINT_CAST(canvas)), WIN_PIXWIN)));
    register int	width, height;
    int			old_top = view_top(canvas);
    int			old_left = view_left(canvas);

    canvas_open_region(canvas, pw);

    /* if the pixwin region moved, the canvas is dirty */
    if (view_top(canvas) != old_top || view_left(canvas) != old_left)
	status_set(canvas, dirty);

    width = view_width(canvas);
    height = view_height(canvas);

    /* now change the size of the canvas
     * pixrect if it's not fixed and the window
     * became wider or longer.
     */
    if (width > canvas_width(canvas)) {
	if (!status(canvas, auto_expand))
	    width = 0;
    } else if (!status(canvas, auto_shrink))
	width = 0;

    if (height > canvas_height(canvas)) {
	if (!status(canvas, auto_expand))
	    height = 0;
    } else if (!status(canvas, auto_shrink))
	height = 0;

    if (height > 0 || width > 0)
	return canvas_resize_canvas(canvas, width, height, repaint);
    else {
	canvas_restrict_clipping(canvas);
	return TRUE;
    }
}


/* resize the canvas.  realloc the retained pixrect if used.
*/
int
canvas_resize_canvas(canvas, width, height, repaint)
register Canvas_info	*canvas;
register int		width, height;
short			repaint;
{

    register int	 old_width	= canvas_width(canvas);
    register int	 old_height	= canvas_height(canvas);
    struct pixrect	*old_pr		= canvas_pr(canvas);
    struct pixrect	*new_pr;
    Rectlist		new_area;

    if (width < 0 || height < 0)
	return FALSE;

    /* use the old width if not
     * given a new one.
     */
    if (width == 0)
	width = old_width;

    /* use the old height if not
     * given a new one.
     */
    if (height == 0)
	height = old_height;

    /* don't do anything if the width & height
     * have not changed.
     */
    if (width == old_width && height == old_height)
	return TRUE;

    if (status(canvas, retained)) {
        new_pr = mem_create(width, height, canvas->depth);
	if (!new_pr)
	    return FALSE;

	canvas_set_pr(canvas, new_pr);
	if (old_pr) {
	    /* note that we do not use pw_rop() here,
	     * the client must call canvas_paint() or we'll get
	     * it with the REPAINT event.
	     */
	    (void)pr_rop(new_pr, 0, 0, old_pr->pr_width, old_pr->pr_height, 
		   PIX_SRC, old_pr, 0, 0);
	    (void)pr_destroy(old_pr);
	}
    }

    /* compute the difference in the old canvas
     * area and the new canvas area.
     */
    compute_new_area(canvas, width, height, &new_area);

    /* update the area */
    canvas_set_width(canvas, width);
    canvas_set_height(canvas, height);

    /* let the client know the size has
     * changed.
     */
    canvas_inform_resize(canvas);

    /* repaint the area difference */
    canvas_request_repaint(canvas, &new_area, repaint);

    return TRUE;
}


/* inform the client about the new canvas size */

void
canvas_inform_resize(canvas)
register Canvas_info	*canvas;
{
    /* don't call the resize proc if none or window is
     * not installed.
     */
    if (!canvas->resize_proc || !status(canvas, first_resize))
	return;

    (*canvas->resize_proc)(canvas, canvas_width(canvas), canvas_height(canvas));
}


/* compute the new area of the canvas create by
 * changing the canvas width and height to width and height.
 */
static void
compute_new_area(canvas, width, height, new_area)
register Canvas_info	*canvas;
register int		width, height;
register Rectlist	*new_area;
{

    Rect			rect;

    /* compute the newly exposed area, 
     * if any.
     */
    *new_area = rl_null;

    rect_construct(&rect, canvas_left(canvas), canvas_top(canvas),
	width, height);

    if (rect_includesrect(&rect, canvas_rect(canvas))) {
	(void)rl_initwithrect(&rect, new_area);
	(void)rl_rectdifference(canvas_rect(canvas), new_area, new_area);
    } else {
	(void)rl_initwithrect(canvas_rect(canvas), new_area);
	(void)rl_rectdifference(&rect, new_area, new_area);
    }

    /* if the canvas is not a fixed image, then
     * include the entire new canvas rect to be
     * repainted.
     */
    if (!status(canvas, fixed_image))
	(void)rl_rectunion(&rect, new_area, new_area);
}
