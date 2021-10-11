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
static char sccsid[] = "@(#)outprim3pas.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

#include "pasarray.h"

int inquire_current_position_3();
int line_abs_3();
int line_rel_3();
int move_abs_3();
int move_rel_3();
int polyline_abs_3();
int polyline_rel_3();

int inqcurrpos3(x, y, z)
double *x, *y, *z;
	{
	float tx, ty, tz;
	int f;
	f=inquire_current_position_3(&tx, &ty, &tz);
	*x=tx;
	*y=ty;
	*z=tz;
	return(f);
	}

int lineabs3(x, y, z)
double x, y, z;
	{
	return(line_abs_3(x, y, z));
	}

int linerel3(dx, dy, dz)
double dx, dy, dz;
	{
	return(line_rel_3(dx, dy, dz));
	}

int moveabs3(x, y, z)
double x, y, z;
	{
	return(move_abs_3(x, y, z));
	}

int moverel3(dx, dy, dz)
double dx, dy, dz;
	{
	return(move_rel_3(dx, dy, dz));
	}

int polylineabs3(xcoord, ycoord, zcoord, n)
double xcoord[], ycoord[], zcoord[];
int n;
	{
		int i;
		for (i = 0; i < n; ++i) {
			xcoort[i] = xcoord[i];
			ycoort[i] = ycoord[i];
			zcoort[i] = zcoord[i];
		}
		return(polyline_abs_3(xcoort, ycoort, zcoort, n));
	}

int polylinerel3(xcoord, ycoord, zcoord, n)
double xcoord[], ycoord[], zcoord[];
int n;
	{
		int i;
		for (i = 0; i < n; ++i) {
			xcoort[i] = xcoord[i];
			ycoort[i] = ycoord[i];
			zcoort[i] = zcoord[i];
		}
		return(polyline_rel_3(xcoort, ycoort, zcoort, n));
	}
