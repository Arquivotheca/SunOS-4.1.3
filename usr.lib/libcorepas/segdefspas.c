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
static char sccsid[] = "@(#)segdefspas.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

int inquire_detectability();
int inquire_highlighting();
int inquire_image_transformation_2();
int inquire_image_transformation_3();
int inquire_image_translate_2();
int inquire_image_translate_3();
int inquire_visibility();
int set_detectability();
int set_highlighting();
int set_image_transformation_2();
int set_image_transformation_3();
int set_image_translate_2();
int set_image_translate_3();
int set_visibility();

int inqdetectability(detectability)
int *detectability;
	{
	return(inquire_detectability(detectability));
	}

int inqhighlighting(highlighting)
int *highlighting;
	{
	return(inquire_highlighting(highlighting));
	}

int inqimgtransform2(sx, sy, a, tx, ty)
double *sx, *sy, *a, *tx, *ty;
	{
	float x1,y1,ta,x2,y2;
	int f;
	f=inquire_image_transformation_2(&x1, &y1, &ta, &x2, &y2);
	*sx=x1;
	*sy=y1;
	*a=ta;
	*tx=x2;
	*ty=y2;
	return(f);
	}

int inqimgtransform3(sx, sy, sz, ax, ay, az, tx, ty, tz)
double *sx, *sy, *sz, *ax, *ay, *az, *tx, *ty, *tz;
	{
	float x1,y1,z1,x2,y2,z2,x3,y3,z3;
	int f;
	f=inquire_image_transformation_3(&x1, &y1, &z1, &x2, &y2, &z2, &x3, &y3, &z3);
	*sx=x1;
	*sy=y1;
	*sz=z1;
	*ax=x2;
	*ay=y2;
	*az=z2;
	*tx=x3;
	*ty=y3;
	*tz=z3;
	return(f);
	}

int inqimgtranslate2(tx, ty)
double *tx, *ty;
	{
	float x1,y1;
	int f;
	f=inquire_image_translate_2(&x1, &y1);
	*tx=x1;
	*ty=y1;
	return(f);
	}

int inqimgtranslate3(tx, ty, tz)
double *tx, *ty, *tz;
	{
	float x1,y1,z1;
	int f;
	f=inquire_image_translate_3(&x1, &y1, &z1);
	*tx=x1;
	*ty=y1;
	*tz=z1;
	return(f);
	}

int inqvisibility(visibility)
int *visibility;
	{
	return(inquire_visibility(visibility));
	}

int setdetectability(detectability)
int detectability;
	{
	return(set_detectability(detectability));
	}

int sethighlighting(highlighting)
int highlighting;
	{
	return(set_highlighting(highlighting));
	}

int setimgtransform2(sx, sy, a, tx, ty)
double sx, sy, a, tx, ty;
	{
	return(set_image_transformation_2(sx, sy, a, tx, ty));
	}

int setimgtransform3(sx, sy, sz, ax, ay, az, tx, ty, tz)
double sx, sy, sz, ax, ay, az, tx, ty, tz;
	{
	return(set_image_transformation_3(sx, sy, sz, ax, ay,
						az, tx, ty, tz));
	}

int setimgtranslate2(tx, ty)
double tx, ty;
	{
	return(set_image_translate_2(tx, ty));
	}

int setimgtranslate3(tx, ty, tz)
double tx, ty, tz;
	{
	return(set_image_translate_3(tx, ty, tz));
	}

int setvisibility(visibility)
int visibility;
	{
	return(set_visibility(visibility));
	}
