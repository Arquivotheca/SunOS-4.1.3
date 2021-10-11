#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)rect_util.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Various rectangle utilities
 */
 
#include <sunwindow/rect.h>

/* Compute the distance from rect to (x, y).
 * If (x, y) is in rect, zero is returned.
 * If x_used or y_used are non-zero, the projection
 * point is returned.
 */
int
rect_distance(rect, x, y, x_used, y_used)
	register Rect	*rect;
	register int	 x, y;
	register int	*x_used, *y_used;
{
	int			 near_x, near_y;
	register int		 dist_sq, temp;
	
	near_x = rect_nearest_edge(rect->r_left, rect->r_width, x);
	near_y = rect_nearest_edge(rect->r_top, rect->r_height, y);
	temp = (near_x - x);
	dist_sq = temp*temp;
	temp = (near_y - y);
	dist_sq += temp*temp;
	if (x_used) *x_used = near_x;
	if (y_used) *y_used = near_y;
	return dist_sq;
}


static int
rect_nearest_edge(minimum, delta, val)
	int	minimum, delta, val;
{
	return((val <= minimum) ? minimum
	       : ((val > (minimum+delta)) ? (minimum+delta) : val));
}

