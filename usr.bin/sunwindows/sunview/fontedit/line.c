#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)line.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */


#include  <stdio.h>

#define	ABS(x) ( (x) < 0 ? -(x) : (x) )

/******************************************************************************

     This function is the preprocessor to the Bresenham line drawing routine.
If the slope of the line  is < 1, then x is passed as the independent variable;
otherwise y is the independent variable.

     The coordinates of the points are read in and no error checking is
done; four integers must be in the input buffer.

****************************************************************************/
static 	int	color;
static  int	org_x, org_y;

fted_draw_line_of_cells(x, y, x1, y1, x2, y2, draw_color)
int x,y;	/* orgin of pad in which cells are drawn	*/
int x1,y1,	/* coordinates of the first cell		*/
    x2,y2;	/* coordinates of the 2nd cell			*/
int draw_color; /* color of line				*/

{
    register int
                    is_x,	/* = 1 if x is the independent variable 	*/
                    dx,		/* abs of the difference in  x coordinates	*/
                    dy;		/* abs of the difference in y coordinates 	*/


    org_x = x; org_y = y;
    color = draw_color;

    dx = ABS(x2 - x1);
    dy = ABS(y2 - y1);

    if (dy < dx) {		/* slope is < 1 */
	is_x = 1;
	bresenham (x1, y1, x2, y2, dx, dy, is_x);
    }
    else {			/* slope is >= 1 */
	is_x = 0;
	bresenham (y1, x1, y2, x2, dy, dx, is_x);
    }
}				/* end draw_line */

/*********************************************************************

   This function draws a line in the bitmap by using a generalized
Bresenham's method.  This method determines for each point between the
independent endpoints a dependent value.  Thus for slopes >=1 y is
the independent variable and x values are calculated; for slopes < 1
x is the indepent variable. ( NOTE: the determination of the independent
and dependent variable is done by fted_draw_line_of_cells. )

**********************************************************************/

static
bresenham (ind1, dep1, ind2, dep2, d_ind, d_dep,is_x)
int ind1,dep1,		  /* the 1st pt   */
    ind2,dep2,		  /* the 2nd pt   */
    d_ind,		  /* difference between the indpendent variable */
    d_dep,		  /* difference between the dependent variable */
    is_x;		  /* = 1 if x is the indpendent variable */
{
	register int
		incr1,		/* increment if d < 0   */
		incr2,		/* increment if d >= 0  */
		incr_dep,	/* increment the dependent variable;
				   either 1 (positive slope) or -1 (neg.) */
		d,		/* determines if the dependent variable
				   should be increment */
		ind,		/* the current value of the independent
				   variable  */
		dep,		/* the current value of the dependent
				   variable	 */
		ind_end,	/* the stopping point of the independent
				   variable  */
		y,		/* the y value of the pixel */
		x;		/* the x value of the pixel */

	d = 2 * d_dep - d_ind;	/* calulate the initial d value */
	incr1 = 2 * d_dep;
	incr2 = 2 * (d_dep - d_ind);
	/*
	 * Find which point has the lowest independent variable and use it
	 *   as the starting point.
	 */
	if (ind1 > ind2) {
	    ind = ind2;
	    dep = dep2;
	    ind_end = ind1;
	    incr_dep = (dep2 > dep1) ? -1 : 1;
	} else {
	    ind = ind1;
	    dep = dep1;
	    ind_end = ind2;
	    incr_dep = (dep1 > dep2) ? -1 : 1;
	}
	do {
	    /*
	     * x and y are assigned depending on whether x is the dependent or
	     *   independent variable.
	     */
	    if (is_x) {
		x = ind;
		y = dep;
	    } else {
		x = dep;
		y = ind;
	    }
	    fted_paint_cell(org_x, org_y, x, y, color);
	    if (d < 0)
		d += incr1;
	    else {
		dep += incr_dep;
		d += incr2;
	    }
	} while (ind++ < ind_end);
}



