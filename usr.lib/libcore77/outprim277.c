/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
#ifndef lint
static char sccsid[] = "@(#)outprim277.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int inquire_current_position_2();
int line_abs_2();
int line_rel_2();
int move_abs_2();
int move_rel_2();
polyline_abs_2();
polyline_rel_2();

int inqcurrpos2_(x, y)
float *x, *y;
	{
	return(inquire_current_position_2(x, y));
	}

int lineabs2_(x, y)
float *x, *y;
	{
	return(line_abs_2(*x, *y));
	}

int linerel2_(dx, dy)
float *dx, *dy;
	{
	return(line_rel_2(*dx, *dy));
	}

int moveabs2_(x, y)
float *x, *y;
	{
	return(move_abs_2(*x, *y));
	}

int moverel2_(dx, dy)
float *dx, *dy;
	{
	return(move_rel_2(*dx, *dy));
	}

int polylineabs2_(xcoord, ycoord, n)
float xcoord[], ycoord[];
int *n;
	{
	return(polyline_abs_2(xcoord, ycoord, *n));
	}

int polylinerel2_(xcoord, ycoord, n)
float xcoord[], ycoord[];
int *n;
	{
	return(polyline_rel_2(xcoord, ycoord, *n));
	}
