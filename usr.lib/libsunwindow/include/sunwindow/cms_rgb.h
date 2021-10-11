/*	@(#)cms_rgb.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Definition of the colormap segment CMS_RGB,
 * the collection of rgb (red-green-blue) binary combinations.
 */

#define	CMS_RGB			"rgb"
#define	CMS_RGBSIZE		8

#define	BLACK	0
#define	RED	1
#define	YELLOW	2
#define	GREEN	3
#define	CYAN	4
#define	BLUE	5
#define	MAGENTA	6
#define	WHITE	7

#define	cms_rgbsetup(r,g,b) \
	(r)[BLACK] = 0;		(g)[BLACK] = 0;		(b)[BLACK] = 0; \
	(r)[RED] = 255;		(g)[RED] = 0;		(b)[RED] = 0; \
	(r)[YELLOW] = 128;	(g)[YELLOW] = 128;	(b)[YELLOW] = 0; \
	(r)[GREEN] = 0;		(g)[GREEN] = 255;	(b)[GREEN] = 0; \
	(r)[CYAN] = 0;		(g)[CYAN] = 128;	(b)[CYAN] = 128; \
	(r)[BLUE] = 0;		(g)[BLUE] = 0;		(b)[BLUE] = 255; \
	(r)[MAGENTA] = 128;	(g)[MAGENTA] = 0;	(b)[MAGENTA] = 128; \
	(r)[WHITE] = 255;	(g)[WHITE] = 255;	(b)[WHITE] = 255;
