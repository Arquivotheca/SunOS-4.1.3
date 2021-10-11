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
/*	@(#)bw1dd.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Instance variables for view surfaces
 */
 struct bw1ddvars {
	int ddcp[2];			/* device coords current point */
	short fillop, fillrastop;	/* rop for region fill, raster */
	short cursorrop;		/* rop for cursor raster */
	short lineop;			/* rop for lines */
	short rtextop;			/* rop for raster text */
	short vtextop;			/* rop for vector text */
	int texture; 			/* set the region fill texture */
	struct pixrect prmask;		/* 16x16 texture bit mask */
	struct mpr_data prmaskdata;
	short msklist[16];
	u_char red[256], green[256], blue[256];
	int linewidth, linestyle, polyintstyle;	/* line attributes */
	int ddfont;			/* text attributes */
	int openfont;
	ipt_type ddup, ddpath, ddspace;
	PIXFONT *pf;
	int maxz;
	int fullx, fully, xoff, yoff, scale;
	struct pixrect *screen;
	struct windowstruct wind;
	int curstrk;
	int lockcount;
};
