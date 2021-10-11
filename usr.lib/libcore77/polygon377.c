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
static char sccsid[] = "@(#)polygon377.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "../libcore/coretypes.h"

int set_zbuffer_cut();
int polygon_abs_3();
int polygon_rel_3();
int set_light_direction();
int set_shading_parameters();
int set_vertex_indices();
int set_vertex_normals();

#define DEVNAMESIZE 20

int setzbuffercut_(surfname, xlist, zlist, n)
float xlist[], zlist[];
int *n;
struct vwsurf *surfname;
	{
	return(set_zbuffer_cut(surfname, xlist, zlist, *n));
	}

int polygonabs3_(xlist, ylist, zlist, n)
float *xlist, *ylist, *zlist;
int *n;
	{
	return(polygon_abs_3(xlist, ylist, zlist, *n));
	}

int polygonrel3_(xlist, ylist, zlist, n)
float *xlist, *ylist, *zlist;
int *n;
	{
	return(polygon_rel_3(xlist, ylist, zlist, *n));
	}

int setlightdirect_(dx, dy, dz)
float *dx, *dy, *dz;
	{
	return(set_light_direction(*dx, *dy, *dz));
	}

int setshadingparams_(amb, dif, spec, flood, bump, hue, style)
float *amb, *dif, *spec, *flood, *bump;
int *hue, *style;
	{
	return(set_shading_parameters(*amb, *dif, *spec, *flood,
					*bump, *hue, *style));
	}

int setshadingparams(amb, dif, spec, flood, bump, hue, style)
float *amb, *dif, *spec, *flood, *bump;
int *hue, *style;
	{
	return(set_shading_parameters(*amb, *dif, *spec, *flood,
					*bump, *hue, *style));
	}

int setvertexindices_(indxlist, n)
int *indxlist, *n;
	{
	return(set_vertex_indices(indxlist, *n));
	}

int setvertexindices(indxlist, n)
int *indxlist, *n;
	{
	return(set_vertex_indices(indxlist, *n));
	}

int setvertexnormals_(dxlist, dylist, dzlist, n)
int *n;
float *dxlist, *dylist, *dzlist;
	{
	return(set_vertex_normals(dxlist, dylist, dzlist, *n));
	}

int setvertexnormals(dxlist, dylist, dzlist, n)
int *n;
float *dxlist, *dylist, *dzlist;
	{
	return(set_vertex_normals(dxlist, dylist, dzlist, *n));
	}
