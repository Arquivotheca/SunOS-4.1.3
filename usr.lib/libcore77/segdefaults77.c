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
static char sccsid[] = "@(#)segdefaults77.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
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

int inqdetectability_(detectability)
int *detectability;
	{
	return(inquire_detectability(detectability));
	}

int inqdetectability(detectability)
int *detectability;
	{
	return(inquire_detectability(detectability));
	}

int inqhighlighting_(highlighting)
int *highlighting;
	{
	return(inquire_highlighting(highlighting));
	}

int inqimgtransform2_(sx, sy, a, tx, ty)
float *sx, *sy, *a, *tx, *ty;
	{
	return(inquire_image_transformation_2(sx, sy, a, tx, ty));
	}

int inqimgtransform2(sx, sy, a, tx, ty)
float *sx, *sy, *a, *tx, *ty;
	{
	return(inquire_image_transformation_2(sx, sy, a, tx, ty));
	}

int inqimgtransform3_(sx, sy, sz, ax, ay, az, tx, ty, tz)
float *sx, *sy, *sz, *ax, *ay, *az, *tx, *ty, *tz;
	{
	return(inquire_image_transformation_3(sx, sy, sz, ax, ay, az,
						tx, ty, tz));
	}

int inqimgtransform3(sx, sy, sz, ax, ay, az, tx, ty, tz)
float *sx, *sy, *sz, *ax, *ay, *az, *tx, *ty, *tz;
	{
	return(inquire_image_transformation_3(sx, sy, sz, ax, ay, az,
						tx, ty, tz));
	}

int inqimgtranslate2_(tx, ty)
float *tx, *ty;
	{
	return(inquire_image_translate_2(tx, ty));
	}

int inqimgtranslate2(tx, ty)
float *tx, *ty;
	{
	return(inquire_image_translate_2(tx, ty));
	}

int inqimgtranslate3_(tx, ty, tz)
float *tx, *ty, *tz;
	{
	return(inquire_image_translate_3(tx, ty, tz));
	}

int inqimgtranslate3(tx, ty, tz)
float *tx, *ty, *tz;
	{
	return(inquire_image_translate_3(tx, ty, tz));
	}

int inqvisibility_(visibility)
int *visibility;
	{
	return(inquire_visibility(visibility));
	}

int setdetectability_(detectability)
int *detectability;
	{
	return(set_detectability(*detectability));
	}

int setdetectability(detectability)
int *detectability;
	{
	return(set_detectability(*detectability));
	}

int sethighlighting_(highlighting)
int *highlighting;
	{
	return(set_highlighting(*highlighting));
	}

int setimgtransform2_(sx, sy, a, tx, ty)
float *sx, *sy, *a, *tx, *ty;
	{
	return(set_image_transformation_2(*sx, *sy, *a, *tx, *ty));
	}

int setimgtransform2(sx, sy, a, tx, ty)
float *sx, *sy, *a, *tx, *ty;
	{
	return(set_image_transformation_2(*sx, *sy, *a, *tx, *ty));
	}

int setimgtransform3_(sx, sy, sz, ax, ay, az, tx, ty, tz)
float *sx, *sy, *sz, *ax, *ay, *az, *tx, *ty, *tz;
	{
	return(set_image_transformation_3(*sx, *sy, *sz, *ax, *ay,
						*az, *tx, *ty, *tz));
	}

int setimgtransform3(sx, sy, sz, ax, ay, az, tx, ty, tz)
float *sx, *sy, *sz, *ax, *ay, *az, *tx, *ty, *tz;
	{
	return(set_image_transformation_3(*sx, *sy, *sz, *ax, *ay,
						*az, *tx, *ty, *tz));
	}

int setimgtranslate2_(tx, ty)
float *tx, *ty;
	{
	return(set_image_translate_2(*tx, *ty));
	}

int setimgtranslate2(tx, ty)
float *tx, *ty;
	{
	return(set_image_translate_2(*tx, *ty));
	}

int setimgtranslate3_(tx, ty, tz)
float *tx, *ty, *tz;
	{
	return(set_image_translate_3(*tx, *ty, *tz));
	}

int setimgtranslate3(tx, ty, tz)
float *tx, *ty, *tz;
	{
	return(set_image_translate_3(*tx, *ty, *tz));
	}

int setvisibility_(visibility)
int *visibility;
	{
	return(set_visibility(*visibility));
	}
