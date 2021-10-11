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
static char sccsid[] = "@(#)rasterfileio77.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

int file_to_raster();
int raster_to_file();

typedef struct {	/* RASTER, pixel coords */
	int width, height, depth;	/* width, height in pixels, bits/pixl */
	short *bits;
	} rast_type;

typedef struct {	/* colormap struct */
	int type, nbytes;
	char *data;
	} colormap_type;

int getfd_();

int filetoraster_(rasfid, raster, map)
int *rasfid;
rast_type *raster;
colormap_type *map;
	{
	return(file_to_raster(getfd_(rasfid), raster, map));
	}

#include "/usr/src/usr.lib/libI77/fio.h"

int rastertofile_(raster, map, rasfid, n)
rast_type *raster;
colormap_type *map;
int *rasfid, *n;
	{
	unit *u;

	u = mapunit(*rasfid);
	nowwriting(u);
	return(raster_to_file(raster, map, getfd_(rasfid), *n));
	}
