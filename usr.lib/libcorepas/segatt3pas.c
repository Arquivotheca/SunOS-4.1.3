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
static char sccsid[] = "@(#)segatt3pas.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

int inquire_segment_image_transformation_3();
int inquire_segment_image_translate_3();
int set_segment_image_transformation_3();
int set_segment_image_translate_3();

int inqsegimgxform3(segname, sx, sy, sz, rx, ry, rz, tx, ty, tz)
int segname;
double *sx, *sy, *sz, *rx, *ry, *rz, *tx, *ty, *tz;
	{
	float x1,y1,z1,x2,y2,z2,x3,y3,z3;
	int f;
	f=inquire_segment_image_translate_3(segname, &x1, &y1, &z1, &x2, &y2, &z2, &x3, &y3, &z3);
	*sx=x1;
	*sy=y1;
	*sz=z1;
	*rx=x2;
	*ry=y2;
	*rz=z2;
	*tx=x3;
	*ty=y3;
	*tz=z3;
	return(f);
	}

int inqsegimgxlate3(segname, tx, ty, tz)
int segname;
double *tx, *ty, *tz;
	{
	float x1,y1,z1;
	int f;
	f=inquire_segment_image_translate_3(segname, &x1, &y1, &z1);
	*tx=x1;
	*ty=y1;
	*tz=z1;
	return(f);
	}

int setsegimgxform3(segname, sx, sy, sz, rx, ry, rz, tx, ty, tz)
int segname;
double sx, sy, sz, rx, ry, rz, tx, ty, tz;
	{
	return(set_segment_image_transformation_3(segname, sx, sy, sz,
						 rx, ry, rz, tx, ty, tz));
	}

int setsegimgxlate3(segname, dx, dy, dz)
int segname;
double dx, dy, dz;
	{
	return(set_segment_image_translate_3(segname, dx, dy, dz));
	}
