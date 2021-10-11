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
static char sccsid[] = "@(#)inqsegatt77.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int inquire_image_transformation_type();
int inquire_segment_detectability();
int inquire_segment_highlighting();
int inquire_segment_image_transformation_2();
int inquire_segment_image_transformation_type();
int inquire_segment_image_translate_2();
int inquire_segment_visibility();

int inqimgxformtype_(segtype)
int *segtype;
	{
	return(inquire_image_transformation_type(segtype));
	}

int inqsegdetectable_(segname, detectbl)
int *segname, *detectbl;
	{
	return(inquire_segment_detectability(*segname, detectbl));
	}

int inqsegdetectable(segname, detectbl)
int *segname, *detectbl;
	{
	return(inquire_segment_detectability(*segname, detectbl));
	}

int inqseghighlight_(segname, highlght)
int *segname, *highlght;
	{
	return(inquire_segment_highlighting(*segname, highlght));
	}

int inqsegimgxform2_(segname, sx, sy, a, tx, ty)
int *segname;
float *sx, *sy, *a, *tx, *ty;
	{
	return(inquire_segment_image_transformation_2(*segname,sx,sy,a,tx,ty));
	}

int inqsegimgxfrmtyp_(segname, segtype)
int *segname, *segtype;
	{
	return(inquire_segment_image_transformation_type(*segname, segtype));
	}

int inqsegimgxfrmtyp(segname, segtype)
int *segname, *segtype;
	{
	return(inquire_segment_image_transformation_type(*segname, segtype));
	}

int inqsegimgxlate2_(segname, tx, ty)
int *segname;
float *tx, *ty;
	{
	return(inquire_segment_image_translate_2(*segname, tx, ty));
	}

int inqsegvisibility_(segname, visbilty)
int *segname, *visbilty;
	{
	return(inquire_segment_visibility(*segname, visbilty));
	}

int inqsegvisibility(segname, visbilty)
int *segname, *visbilty;
	{
	return(inquire_segment_visibility(*segname, visbilty));
	}
