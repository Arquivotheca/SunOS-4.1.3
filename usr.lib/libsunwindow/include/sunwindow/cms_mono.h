/*	@(#)cms_mono.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Definition of the colormap segment CMS_MONOCHROME,
 * a black and white color map segment.
 */

#define	CMS_MONOCHROME		"monochrome"
#define	CMS_MONOCHROMESIZE	2

#define	WHITE	0
#define	BLACK	1

#define	cms_monochromeload(r,g,b) \
	(r)[WHITE] = -1;(g)[WHITE] = -1;(b)[WHITE] = -1; \
	(r)[BLACK] = 0;(g)[BLACK] = 0;(b)[BLACK] = 0; 
