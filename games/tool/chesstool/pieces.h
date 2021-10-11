/* @(#)pieces.h 1.1 92/07/30 Copyr 1984 Sun Micro */

/*
 * Copyright (c) 1984 by Sun Microsystems Inc.
 */

#ifndef PATCHES_DEFINED
#define PATCHES_DEFINED
/*	@(#)patches.h 1.1 84/01/11 SMI	*/

/*
 *	patches:	Sun Microsystems 1984
 *
 *		64 X 64 memory pixrects with useful patterns: white,
 *		simple 25% gray, root-window 25% gray, 50% gray, 75% gray,
 *		and black.
 */

#include <pixrect/pixrect_hs.h>

extern struct pixrect	light_square;
extern struct pixrect	dark_square;
extern struct pixrect	piecepawn;
extern struct pixrect	pieceknight;
extern struct pixrect	piecebishop;
extern struct pixrect	piecerook;
extern struct pixrect	piecequeen;
extern struct pixrect	pieceking;
extern struct pixrect	outlinepawn;
extern struct pixrect	outlineknight;
extern struct pixrect	outlinebishop;
extern struct pixrect	outlinerook;
extern struct pixrect	outlinequeen;
extern struct pixrect	outlineking;

#endif
