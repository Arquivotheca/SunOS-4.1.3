#ifndef PATCHES_DEFINED
#define PATCHES_DEFINED
/*	@(#)patches.h 1.1 92/07/30 SMI	*/
/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */


/*
 *	patches:	Sun Microsystems 1984
 *
 *		16 X 16 memory pixrects with useful patterns: white,
 *		black, and grays ranging from 1/6 to 5/6.
 */

#include <pixrect/pixrect_hs.h>

extern struct pixrect	white_patch;
extern struct pixrect	fted_gray17_patch;
extern struct pixrect	fted_gray20_patch;
extern struct pixrect	fted_gray25_patch;
extern struct pixrect	fted_gray50_patch;
extern struct pixrect	fted_gray75_patch;
extern struct pixrect	fted_gray80_patch;
extern struct pixrect	fted_gray83_patch;
extern struct pixrect	fted_root_gray_patch;
extern struct pixrect	fted_black_patch;

#endif
