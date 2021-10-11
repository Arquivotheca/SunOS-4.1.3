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
static char sccsid[] = "@(#)outprim377.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int inquire_current_position_3();
int line_abs_3();
int line_rel_3();
int move_abs_3();
int move_rel_3();
int polyline_abs_3();
int polyline_rel_3();

int inqcurrpos3_(x, y, z)
float *x, *y, *z;
	{
	return(inquire_current_position_3(x, y, z));
	}

int lineabs3_(x, y, z)
float *x, *y, *z;
	{
	return(line_abs_3(*x, *y, *z));
	}

int linerel3_(dx, dy, dz)
float *dx, *dy, *dz;
	{
	return(line_rel_3(*dx, *dy, *dz));
	}

int moveabs3_(x, y, z)
float *x, *y, *z;
	{
	return(move_abs_3(*x, *y, *z));
	}

int moverel3_(dx, dy, dz)
float *dx, *dy, *dz;
	{
	return(move_rel_3(*dx, *dy, *dz));
	}

int polylineabs3_(xcoord, ycoord, zcoord, n)
float xcoord[], ycoord[], zcoord[];
int *n;
	{
	return(polyline_abs_3(xcoord, ycoord, zcoord, *n));
	}

int polylinerel3_(xcoord, ycoord, zcoord, n)
float xcoord[], ycoord[], zcoord[];
int *n;
	{
	return(polyline_rel_3(xcoord, ycoord, zcoord, *n));
	}
