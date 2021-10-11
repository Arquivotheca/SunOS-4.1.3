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
/*	@(#)gp1pixwindd.h 1.1 92/07/30 Copyr 1985 Sun Micro */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef CMS_H
#define CMS_H
#include <sunwindow/cms.h>
#endif

/*
 * Instance variables for view surfaces
 */
 struct gp1pwddvars {
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
	short opcolorset;
	int linewidth, linestyle, polyintstyle;	/* line, poly attrib */
	int ddfont, openfont;			/* text attributes */
	PIXFONT *pf;
	ipt_type ddup, ddpath, ddspace;
	short openzbuffer;		/* zbuffer control:
					   NOZB => no z-buffer
					   SWZB => software z-buffer
					   HWZB => hardware z-buffer (GB)
					*/
	struct pixrect *zbuffer, *cutaway;	/* SWZB only */
	float *xarr, *zarr;		/* NDC cutaway data -- SWZB only */
	int cutarraysize;		/* no. of cutaway points -- SWZB only */

	int maxz;
	struct windowstruct wind;	/* info about window and gp */
	porttype port;			/* NDC viewport info */
	int toolpid;
        struct gp1_attr *gp1attr;	/* ptr to gp1_attr struct */
};


#define NOTRAWDEV 0


#ifndef NOZB

/* z-buffer constants */
#define NOZB 0
#define SWZB 1
#define HWZB 2


/* opcolorset constants */
#define LINE_OPCOLOR 0
#define REGION_OPCOLOR 1
#define RASTER_OPCOLOR 2
#define RASTER8_OPCOLOR 3
#define RASTERTEXT_OPCOLOR 4
#define VECTORTEXT_OPCOLOR 5
#define CURSOR_OPCOLOR 6

#endif
