#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tool_bdry.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Does tool interactive boundary mgt.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <sundev/kbd.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/cms_mono.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_input.h>
#include <suntool/wmgr.h>
#include <suntool/icon.h>
#include <suntool/tool.h>

#define	MIN_SW_WIDTH	16
#define	MIN_SW_HEIGHT	16

typedef enum border_side { 
	BORDERS_NONE, 
	BORDERS_LEFT,
	BORDERS_RIGHT,
	BORDERS_TOP,
	BORDERS_BOTTOM
} Border;

static Border	rect_border();
static Toolsw	*nearest_sw(), *resolve_xy_to_sw();
static adjust_position();

extern void	tool_expand_neighbors(),
		tool_compute_constraint();

static void	constrain_sw(),
		compute_obstacles(), 
		compute_shadows(),
		constrain_from_tool(),
		constrain_neighbor(),
		change_sw(),
		sw_set_rect(),
		get_tool_info(),
		compute_contortion();

int
tool_moveboundary(tool, event)
	register Tool	*tool;
	register Event	*event;
{
	Toolsw		*target_sw;
	register int	x = event_x(event), y = event_y(event);
	int		is_move = !event_ctrl_is_down(event);
	int		wants_fill = !event_shift_is_down(event);
	int		swsp = tool_subwindowspacing(tool);
	int		top_border = tool_stripeheight(tool) + 2;
	Rect		old_rect, new_rect;
	Rect		rconstrain;
	WM_Direction	grasp;
	Rectlist 	obstacles;
	int		x_used, y_used;
	extern WM_Direction wmgr_compute_grasp();
	
	if (~tool->tl_flags&TOOL_BOUNDARYMGR ||
	    (tool->tl_rectcache.r_width - swsp <= x) ||
	    (tool->tl_rectcache.r_height - swsp <= y) ||
	    (tool->tl_rectcache.r_left + swsp >= x) ||
	    (tool->tl_rectcache.r_top + top_border >= y))
		return(-1);

	target_sw = nearest_sw(tool, x, y, (int *)LINT_CAST(&x_used), 
				(int *)LINT_CAST(&y_used));
	if (!target_sw)
		return (-1);
	
	event_set_x(event, x_used);
	event_set_y(event, y_used);
			
	/*
	 * Determine constrainning rect
	 */
	(void)win_getrect(target_sw->ts_windowfd, &old_rect);
	new_rect = old_rect;
	
	grasp = wmgr_compute_grasp(&new_rect, event, is_move, FALSE, TRUE);
	constrain_sw(tool, target_sw, grasp, is_move, &rconstrain);
	
	/* Compute the obstacles posed 
	 * by the other subwindows
	 */
	compute_obstacles(tool, target_sw, &obstacles);
	
	/*
	 * Display feedback that the user drags around the screen.
	 */
	wmgr_providefeedback(tool->tl_windowfd, &new_rect, event, is_move, 
				 FALSE, grasp, &rconstrain,
				 (int (*)())adjust_position, (int)(swsp / 2), 
				 &obstacles);
	(void)rl_free(&obstacles);

	/* Nothing to do if rect
	 * size/position did not change.
	 */
	if (rect_equal(&new_rect, &old_rect))
		return(0);
		
	/* Apply the new subwindow position/size */	
	change_sw(tool, target_sw, &new_rect, wants_fill);
	
	/* expand the subwindow's former neighbors
	 * to fill any new holes.
	 */
	if (wants_fill)
		tool_expand_neighbors(tool, target_sw, &old_rect);
		
	/* call the client's subwindow layout routine
	 * so the client knows the subwindows have changed.
	 * Note that the default routine will not alter the
	 * subwindows, since the above code will have adjusted
	 * the subwindows already.
	 */
	(void)tool_layoutsubwindows(tool);

        /*
         * Need to redisplay the tool because the sw boundaries need to be
         * fixed up.
         */
        (void)_tool_display(tool, FALSE);
	return(0);
}


static Toolsw *
nearest_sw(tool, x, y, x_used, y_used)
	Tool		 *tool;
	register int	 x, y;
	register int	*x_used, *y_used;
{
	register Toolsw		*result = resolve_xy_to_sw(tool, x, y);
	register Toolsw		*sw;
	int			 min_dist_sq = 0x7FFFFFFF;
	register int		 dist_sq;
	int			 x_int, y_int;
	Rect			 rect;
	
	/* might be in a subwindow.
	 * In that case return 0.
	 */
	if (result) {
	    (void)win_getrect(result->ts_windowfd, &rect);
	    if (rect_includespoint(&rect, x, y))
	        return (Toolsw *) 0;
	        
	    if (x_used) *x_used = x;
	    if (y_used) *y_used = y;
	    return result;
	}
	
	/* not in a subwindow or on any border.
	 * look for the closest subwindow.
	 */
	for (sw = tool->tl_sw; sw; sw = sw->ts_next) {
	    (void)win_getrect(sw->ts_windowfd, &rect);
	    dist_sq = rect_distance(&rect, x, y, &x_int, &y_int);
	    if (dist_sq < min_dist_sq) {
	        min_dist_sq = dist_sq;
	        result = sw;
	        if (x_used) *x_used = x_int;
	        if (y_used) *y_used = y_int;
	    }
	}
	return(result);
}


static Toolsw *
resolve_xy_to_sw(tool, x, y)
	register Tool	*tool;
	register int	 x, y;
{
	register Toolsw		*sw;
	Rect			rect, rtest;
	Toolsw			*target_sw = 0;
	int			swsp = tool_subwindowspacing(tool);
	
	/*
	 * Find sw window which is above or to left of position
	 */
	for (sw = tool->tl_sw; sw; sw = sw->ts_next) {
		(void)win_getrect(sw->ts_windowfd, &rect);
		
		if (rect_includespoint(&rect, x, y))
			return sw;
			
		/* First check the bottom edge */
		rect_construct(&rtest, rect.r_left, rect_bottom(&rect),
		    rect.r_width, swsp+1);
		if (rect_includespoint(&rtest, x, y))
			return sw;
		
		/* Now check the right edge */
		rect_construct(&rtest, rect_right(&rect), rect.r_top,
		    swsp, rect.r_height+1);
		if (rect_includespoint(&rtest, x, y))
			return sw;
		
		/* Now check the top edge */
		rect_construct(&rtest, rect.r_left, rect.r_top - swsp,
		    rect.r_width, swsp+1);
		if (rect_includespoint(&rtest,  x, y)) {
			target_sw = sw;
			continue;
		} 
		
		/* Now check the left edge */
		rect_construct(&rtest, rect.r_left - swsp, rect.r_top,
		    swsp, rect.r_height+1);
		if (rect_includespoint(&rtest, x, y)) {
			target_sw = sw;
			continue;
		}
	}
	return target_sw;
}


/* Constrain target_sw from moving or stretching
 * outside the tool.
 */
static void	
constrain_sw(tool, target_sw, grasp, is_move, rconstrain)
Tool	*tool;
Toolsw	*target_sw;
WM_Direction grasp;
int	is_move;
Rect	*rconstrain;
{
	Rect		target_rect;
	extern Rect	wmgr_set_bounds();
	
	
	constrain_from_tool(tool, rconstrain);
	(void)win_getrect(target_sw->ts_windowfd, &target_rect);
	*rconstrain = 
	    wmgr_set_bounds(&target_rect, rconstrain,grasp, is_move, 
	    	(struct pr_pos *)0, MIN_SW_WIDTH, MIN_SW_HEIGHT);
	
}

/* Compute the constraining rect imposed
 * by the tool.
 */
static void	
constrain_from_tool(tool, rconstrain)
Tool		*tool;
register Rect	*rconstrain;
{
	short		border_width = tool_borderwidth(tool);
	
	*rconstrain 		= tool->tl_rectcache;
	rconstrain->r_left 	+= border_width;
	rconstrain->r_width 	-= 2 * border_width;
	rconstrain->r_top 	+= tool_stripeheight(tool) + 2;
	rconstrain->r_height	-= tool_stripeheight(tool) + 2 + border_width;
}
	

/* Compute the obstacle rectangles posed by the 
 * other subwindows in tool.
 */
static void
compute_obstacles(tool, target_sw, obstacles)
Tool	*tool;
Toolsw	*target_sw;
Rectlist *obstacles;
{
	register Toolsw	*sw;
	register short	swsp = tool_subwindowspacing(tool) / 2;
	Rect		rect;
	
	*obstacles = rl_null;
		
	for (sw = tool->tl_sw; sw; sw = sw->ts_next) {
		if (sw == target_sw)
			continue;
			
		(void)win_getrect(sw->ts_windowfd, &rect);
		rect_marginadjust(&rect, swsp);
		(void)rl_rectunion(&rect, obstacles, obstacles);
	}
}


/* If new_rect collides with an obstacle and the obstacle can't
 * be contorted, use old_rect instead.  This is called by
 * wmgr_providefeedback() in wmgr_rect.c.
 */
static
adjust_position(obstacles, swsp, old_rect, new_rect)
Rectlist	*obstacles;
short		swsp;
Rect		*old_rect, *new_rect;
{
	register Rectnode	*rnode;
	register Rect		*r;
	short			width_left, width_right, width;
	short			height_above, height_below, height;
	
	rect_marginadjust(new_rect, swsp + 1);
	for (rnode = obstacles->rl_head; rnode; rnode = rnode->rn_next) {
		r = &rnode->rn_rect;
		if (rect_intersectsrect(r, new_rect)) {
			compute_contortion(r, new_rect, 
				&width_left, &width_right, &width,
				&height_above, &height_below, &height);
			if ((width < MIN_SW_WIDTH) && (height < MIN_SW_HEIGHT)) {
			    *new_rect = *old_rect;
			    return;
			}
		}
	}
	rect_marginadjust(new_rect, -(swsp + 1));
}
			
/* Expand neighboring subwindows to fill in the
 * gap caused by changing target_sw from old_rect.
 */		     
extern void
tool_expand_neighbors(tool, target_sw, old_rect)
Tool		*tool;
Toolsw		*target_sw;
Rect		*old_rect;
{
	register Toolsw	*sw;
	Rect		rect, rconstrain;
	register Rect	*r = &rect;
	register short	swsp = tool_subwindowspacing(tool) / 2;
	register Border	borders_on;
	register int	hole;
	int		tool_left, tool_top, tool_right, tool_bottom;
	
	rect_marginadjust(old_rect, swsp + 1);
	
	(void)win_lockdata(tool->tl_windowfd);
	
	get_tool_info(tool, &tool_left, &tool_top,
		      &tool_right, &tool_bottom);
	
	/* For each subwindow bordering old_rect */
	for (sw = tool->tl_sw; sw; sw = sw->ts_next) {
		if (sw == target_sw)
			continue;
			
		(void)win_getrect(sw->ts_windowfd, r);
		rect_marginadjust(r, swsp);

		borders_on = rect_border(r, old_rect);
		
		if (borders_on == BORDERS_NONE)
			continue;
		
		tool_compute_constraint(tool, sw, &rconstrain);
		
		switch (borders_on) {
		case BORDERS_LEFT:
			hole = rect_right(&rconstrain) - rect_right(r);
			if (hole > 0)
				r->r_width += hole;
			break;
			
		case BORDERS_RIGHT:
			hole = r->r_left - rconstrain.r_left;
			if (hole > 0) {
				r->r_width += hole;
				r->r_left = rconstrain.r_left;
			}
			break;
			
		case BORDERS_TOP:
			hole = rect_bottom(&rconstrain) - rect_bottom(r);
			if (hole > 0)
				r->r_height += hole;
			break;
			
		case BORDERS_BOTTOM:
			hole = r->r_top - rconstrain.r_top;
			if (hole > 0) {
				r->r_height += hole;
				r->r_top = rconstrain.r_top;
			}
			break;
		}
			
		rect_marginadjust(r, -swsp);
		sw_set_rect(sw, r, tool_left, tool_top, tool_right,
		            tool_bottom);
	}
	
	rect_marginadjust(old_rect, -(swsp + 1));
	(void)win_unlockdata(tool->tl_windowfd);
}


/* Compute the border relationship of
 * neighbor_rect to target_rect.
 */
static Border
rect_border(neighbor_rect, target_rect)
register Rect	*neighbor_rect;
register Rect	*target_rect;
{

	if (rect_right(neighbor_rect) == (target_rect->r_left - 1))
		return BORDERS_LEFT;
		
	if (neighbor_rect->r_left == (rect_right(target_rect) + 1))
		return BORDERS_RIGHT;
		
	if (rect_bottom(neighbor_rect) == (target_rect->r_top - 1))
		return BORDERS_TOP;
		
	if (neighbor_rect->r_top == (rect_bottom(target_rect) + 1))
		return BORDERS_BOTTOM;
	
	return BORDERS_NONE;
}		

/* Constrain target_sw according to
 * its shadows.  rconstrain is computed as the tallest, lowest, and
 * widest that target_sw can be.  Note that if tool is iconic,
 * rconstrain will be offset by the usual TOOL_ICONIC_OFFSET.
 */
void		
tool_compute_constraint(tool, target_sw, rconstrain)
Tool		*tool;
Toolsw		*target_sw;
register Rect	*rconstrain;
{
	Toolsw		*sw;
	Rect		rect;
	register int	constraint;
	register short	swsp = tool_subwindowspacing(tool) / 2;
	Rect		left_rect, right_rect;
	Rect		top_rect, bottom_rect;
	register Rect	*left_shadow = &left_rect, *right_shadow = &right_rect;
	register Rect	*top_shadow = &top_rect, *bottom_shadow = &bottom_rect;
	
	constrain_from_tool(tool, rconstrain);
	/* compute constraint for the outer borders */
	rect_marginadjust(rconstrain, swsp);
	
	compute_shadows(tool, target_sw, left_shadow, right_shadow, 
			top_shadow, bottom_shadow);	
			
	/* For each subwindow in the shadow */
	for (sw = tool->tl_sw; sw; sw = sw->ts_next) {
		if (sw == target_sw)
			continue;
			
		(void)win_getrect(sw->ts_windowfd, &rect);
		rect_marginadjust(&rect, swsp + 1);
		
		if (rect_intersectsrect(&rect, left_shadow)) {
			constraint = rect_right(&rect) + 1;
			if (constraint > rconstrain->r_left) {
				rconstrain->r_width -=
					 constraint - rconstrain->r_left;
				rconstrain->r_left = constraint;
			}
		} else if (rect_intersectsrect(&rect, right_shadow)) {
			constraint = rect.r_left - 1;
			if (constraint < rect_right(rconstrain))
				rconstrain->r_width -= 
				    rect_right(rconstrain) - constraint;
		}
		if (rect_intersectsrect(&rect, top_shadow)) {
			constraint = rect_bottom(&rect) + 1;
			if (constraint > rconstrain->r_top) {
				rconstrain->r_height -=
					 constraint - rconstrain->r_top;
				rconstrain->r_top = constraint;
			}
		} else if (rect_intersectsrect(&rect, bottom_shadow)) {
			constraint = rect.r_top - 1;
			if (constraint < rect_bottom(rconstrain))
				rconstrain->r_height -= 
				    rect_bottom(rconstrain) - constraint;
		}

	}
}

/* Compute the shadow rects
 * for target_sw.
 */
static void
compute_shadows(tool, target_sw, left_shadow, right_shadow,
	        top_shadow, bottom_shadow)
register Tool	*tool;
Toolsw		*target_sw;
register Rect	*left_shadow, *right_shadow;
register Rect	*top_shadow, *bottom_shadow;
{
	Rect	target_rect;

	/* Compute the min & max shadow rects */
	(void)win_getrect(target_sw->ts_windowfd, &target_rect);

	rect_construct(left_shadow,
			0, target_rect.r_top, 
			target_rect.r_left, target_rect.r_height);
	rect_construct(right_shadow,
			rect_right(&target_rect), target_rect.r_top, 
			tool->tl_rectcache.r_width - rect_right(&target_rect), 
			target_rect.r_height);
		      
	rect_construct(top_shadow,
			target_rect.r_left, 0, 
			target_rect.r_width, target_rect.r_top);
	rect_construct(bottom_shadow, target_rect.r_left, 
			rect_bottom(&target_rect), 
			target_rect.r_width, 
			tool->tl_rectcache.r_height - target_rect.r_top);
}
		


/* Change target_sw to new_rect and contort any
 * obstacles.
 */		     

static void
change_sw(tool, target_sw, new_rect, wants_fill)
Tool	*tool;
Toolsw	*target_sw;
Rect	*new_rect;
int	 wants_fill;
{
	register Toolsw	*sw;
	Rect		rect, rectoriginal;
	register Rect	*r = &rect;
	register short	swsp = tool_subwindowspacing(tool) / 2;
	short		width_left, width_right, width;
	int		width_area;
	short		height_above, height_below, height;
	int		height_area;
	int		tool_left, tool_top, tool_right, tool_bottom;

	get_tool_info(tool, &tool_left, &tool_top,
		      &tool_right, &tool_bottom);
	
	/* First set the rect for the target */
	sw_set_rect(target_sw, new_rect, tool_left, tool_top, tool_right, 
	            tool_bottom);
	
	rect_marginadjust(new_rect, swsp + 1);
	(void)win_lockdata(tool->tl_windowfd);
	
	/* For each subwindow in the way */
	for (sw = tool->tl_sw; sw; sw = sw->ts_next) {
		if (sw == target_sw)
			continue;
			
		(void)win_getrect(sw->ts_windowfd, r);
		rectoriginal = *r;
		rect_marginadjust(r, swsp);

		if (!rect_intersectsrect(r, new_rect))
			continue;
			
		compute_contortion(r, new_rect, 
			&width_left, &width_right, &width,
			&height_above, &height_below, &height);
		width_area = (width < MIN_SW_WIDTH) ? 0 : (width * r->r_height);
		height_area = (height < MIN_SW_HEIGHT) ? 0 : (height * r->r_width);
			
		if (width_area >= height_area) {
		    if (width == width_right)
		           r->r_left = rect_right(new_rect) + 1;
		    r->r_width = width;
		} else {
		    if (height == height_below)
		        r->r_top = rect_bottom(new_rect) + 1;
		    r->r_height = height;
		}
		rect_marginadjust(r, -swsp);

		sw_set_rect(sw, r, tool_left, tool_top, tool_right, 
	                    tool_bottom);
			
		if (wants_fill)
			tool_expand_neighbors(tool, sw, &rectoriginal);
	}
	rect_marginadjust(new_rect, -(swsp + 1));

	(void)win_unlockdata(tool->tl_windowfd);
}

/* Compute the possible contortions of a victim rectangle caused by
 * overlapping an agressor rectangle.
 */
static void
compute_contortion(victim, aggressor, width_left, width_right, width,
		   height_above, height_below, height)
register Rect	*victim, *aggressor;
short		*width_left, *width_right, *width;
short		*height_above, *height_below, *height;
{
	/* allow one pixel between rectangles.
	 * if we want to move next to position x, we skip
	 * x + 1 and use x + 2. Note that width = right - left + 1
	 * and height = bottom - top + 1.
	 */
	*width_left = (aggressor->r_left - 1) - victim->r_left + 1;
	*width_right = rect_right(victim) - (rect_right(aggressor) + 1) + 1;
	*width = max(*width_left, *width_right);
			
	*height_above = (aggressor->r_top - 1) - victim->r_top + 1;
	*height_below = rect_bottom(victim) - (rect_bottom(aggressor) + 1) + 1;
	*height = max(*height_above, *height_below);
}


static void
sw_set_rect(target_sw, new_rect, tool_left, tool_top, tool_right, tool_bottom)
Toolsw	*target_sw;
Rect	*new_rect;
int	tool_left, tool_top, tool_right, tool_bottom;
{
	Rect	old_rect;
	extern void wmgr_completechangerect();
	
	/* don't clobber the width or height
	 * unless it changed.
	 */
	(void)win_getrect(target_sw->ts_windowfd, &old_rect);
	if (old_rect.r_width != new_rect->r_width)
		target_sw->ts_width = new_rect->r_width;
	if (old_rect.r_height != new_rect->r_height)
		target_sw->ts_height = new_rect->r_height;
		
	/* Only allow subwindows which actually border
	 * the tool to be extend-to-edge.
	 * Also if the sw now borders the tool,
	 * make it extend-to-edge.
	 */
	if (rect_right(new_rect) >= tool_right)
		target_sw->ts_width = TOOL_SWEXTENDTOEDGE;
	else if (target_sw->ts_width == TOOL_SWEXTENDTOEDGE)
		target_sw->ts_width = new_rect->r_width;

	if (rect_bottom(new_rect) >= tool_bottom)
		target_sw->ts_height = TOOL_SWEXTENDTOEDGE;
	else if (target_sw->ts_height == TOOL_SWEXTENDTOEDGE)
		target_sw->ts_height = new_rect->r_height;
		
	wmgr_completechangerect(target_sw->ts_windowfd, new_rect, &old_rect,
		tool_left, tool_top);
}

/* Compute the left, top of the tool in screen coordinates,
 * and compute the rightmost and bottommost point in tool-relative
 * coordinates.
 */
static void
get_tool_info(tool, left, top, right, bottom)
Tool	*tool;
int	*left, *top, *right, *bottom;
{
	Rect		 rect;
	register Rect	*r = &rect;
	register int	 border_width = tool_borderwidth(tool);
	
	(void)win_getrect(tool->tl_windowfd, r);
	*left = r->r_left;
	*top = r->r_top;
	*right = r->r_width - border_width - 1;
	*bottom = r->r_height - border_width - 1;
}


/* This is no longer needed since we use wmgr_providefeedback()
 * now.
 */
#ifdef notdef

#define	xor_box(r) \
	_tool_draw_box(fsglobal->fs_pixwin, PIX_NOT(PIX_DST), (r), 1, BLACK);

wmgr_dragboundary(feedbackfd, r, rconstrain, event, changewidth)
	int	feedbackfd;
	struct	rect  *r, *rconstrain;
	struct	inputevent *event;
	int	changewidth;
{
	struct	rect  rold;
	struct	fullscreen *fs;

	/*
	 * Get full screen access
	 */
	fs = fullscreen_init(feedbackfd);
	/*
	 * put the outline up to start
	 */
	xor_box(r);
	
	do {
		if (input_readevent(feedbackfd, event)==-1)
			break;
		rold = *r;
		if (rect_includespoint(rconstrain,
		    event->ie_locx, event->ie_locy)) {
			if (changewidth)
				r->r_width = event->ie_locx-r->r_left;
			else
				r->r_height = event->ie_locy-r->r_top;
			if (!(rect_equal(r, &rold))) {
				xor_box(&rold); 
				xor_box(r);
			}
		}
	} while (!((event->ie_code==SELECT_BUT || event->ie_code==MS_MIDDLE) &&
	    win_inputnegevent(event)));
	/*
	 * Erase box
	 */
	xor_box(r);
	fullscreen_destroy(fs);
	return;
}

#endif notdef
