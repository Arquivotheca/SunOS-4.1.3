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
static char sccsid[] = "@(#)rasterprimpas.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

int allocate_raster();
int free_raster();
int get_raster();
int put_raster();
int size_raster();

typedef struct {	/* RASTER, pixel coords */
	int width, height, depth;	/* width, height in pixels, bits/pixl */
	short *bits;
	} rast_type;

#define DEVNAMESIZE 20

typedef struct  {
    		char screenname[DEVNAMESIZE];
    		char windowname[DEVNAMESIZE];
		int fd;
		int (*dd)();
		int instance;
		int cmapsize;
    		char cmapname[DEVNAMESIZE];
		int flags;
		char *ptr;
		} vwsurf;

int allocateraster(rptr)
rast_type *rptr;
	{
	return(allocate_raster(rptr));
	}

int freeraster(rptr)
rast_type *rptr;
	{
	return(free_raster(rptr));
	}

int getraster(surfname, xmin, xmax, ymin, ymax, xd, yd, raster)
vwsurf *surfname;
double xmin, xmax, ymin, ymax;
int xd, yd;
rast_type *raster;
	{
	return(get_raster(surfname, xmin, xmax, ymin, ymax,
					xd, yd, raster));
	}

int putraster(srast)
rast_type *srast;
	{
	return(put_raster(srast));
	}

int sizeraster(surfname, xmin, xmax, ymin, ymax, raster)
vwsurf *surfname;
double xmin, xmax, ymin, ymax;
rast_type *raster;
	{
	return(size_raster(surfname, xmin, xmax, ymin, ymax, raster));
	}
