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

/*	@(#)cgpixwindd.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sunwindow/cms.h>

/*
 * Instance variables for view surfaces
 */
 struct cgpwddvars {
	ipt_type ddcp;			/* device coords current point */
	short fillop, fillrastop;	/* rop for region fill, raster */
	short cursorrop;		/* rop for cursor raster */
	short lineop;			/* rop for lines */
	short rtextop;			/* rop for raster text */
	short vtextop;			/* rop for vector text */
	short RAS8func;			/* rop for depth 8 raster */
	int cmapsize;
	char cmsname[CMS_NAMESIZE];
	u_char red[256], green[256], blue[256];	/* colormap */
	short lineindex, textindex, fillindex;	/* color attributes */
	int linewidth, linestyle, polyintstyle;	/* line, poly attrib */
	int ddfont;			/* text attributes */
	int openfont;
	ipt_type ddup, ddpath, ddspace;
	PIXFONT *pf;
	short openzbuffer;		/* zbuffer control */
        int _core_hidden;		/* True if doing hidden surfaces */
	struct pixrect *zbuffer, *cutaway;
	float *xarr, *zarr;		/* NDC cutaway data */
	int cutarraysize;		/* no. of cutaway points */
	int maxz;
	struct windowstruct wind;	/* info about window */
	int toolpid;
};
