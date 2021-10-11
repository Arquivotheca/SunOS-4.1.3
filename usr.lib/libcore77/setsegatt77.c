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
static char sccsid[] = "@(#)setsegatt77.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int set_segment_detectability();
int set_segment_highlighting();
int set_segment_image_transformation_2();
int set_segment_image_translate_2();
int set_segment_visibility();

int setsegdetectable_(segname, detectbl)
int *segname, *detectbl;
	{
	return(set_segment_detectability(*segname, *detectbl));
	}

int setsegdetectable(segname, detectbl)
int *segname, *detectbl;
	{
	return(set_segment_detectability(*segname, *detectbl));
	}

int setseghighlight_(segname, highlght)
int *segname, *highlght;
	{
	return(set_segment_highlighting(*segname, *highlght));
	}

int setsegimgxform2_(segname, sx, sy, a, tx, ty)
int *segname;
float *sx, *sy, *a, *tx, *ty;
	{
	return(set_segment_image_transformation_2(*segname,*sx,*sy,*a,*tx,*ty));
	}

int setsegimgxlate2_(segname, tx, ty)
int *segname;
float *tx, *ty;
	{
	return(set_segment_image_translate_2(*segname, *tx, *ty));
	}

int setsegvisibility_(segname, visbilty)
int *segname, *visbilty;
	{
	return(set_segment_visibility(*segname, *visbilty));
	}

int setsegvisibility(segname, visbilty)
int *segname, *visbilty;
	{
	return(set_segment_visibility(*segname, *visbilty));
	}
