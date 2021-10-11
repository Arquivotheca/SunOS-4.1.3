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
static char sccsid[] = "@(#)rasterprim77.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "../libcore/coretypes.h"

int allocate_raster();
int free_raster();
int get_raster();
int put_raster();
int size_raster();

#define DEVNAMESIZE 20

int allocateraster_(rptr)
rast_type *rptr;
	{
	return(allocate_raster(rptr));
	}

int freeraster_(rptr)
rast_type *rptr;
	{
	return(free_raster(rptr));
	}

int getraster_(surfname, xmin, xmax, ymin, ymax, xd, yd, raster)
struct vwsurf *surfname;
float *xmin, *xmax, *ymin, *ymax;
int *xd, *yd;
rast_type *raster;
	{
	return(get_raster(surfname, *xmin, *xmax, *ymin, *ymax,
					*xd, *yd, raster));
	}

int putraster_(srast)
rast_type *srast;
	{
	return(put_raster(srast));
	}

int sizeraster_(surfname, xmin, xmax, ymin, ymax, raster)
struct vwsurf *surfname;
float *xmin, *xmax, *ymin, *ymax;
rast_type *raster;
	{
	return(size_raster(surfname, *xmin, *xmax, *ymin, *ymax, raster));
	}
