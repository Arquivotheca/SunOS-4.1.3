/*	@(#)bitmap.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/****
 *
 * BITMAP.H - Bitmap object info
 *
 ****/
#ifndef _BITMAP
#include "obj.h"

/*
 * Bitmap types
 */
#define BMAP_TYPE_PIXRECT	0
#define BMAP_TYPE_PACKED	1
#define BMAP_TYPE_UNPACKED	2

#define	BMAP_TYPE	0
#define	BMAP_WIDTH	1
#define	BMAP_HEIGHT	2
#define	BMAP_DEPTH	3
#define	BMAP_DATA	4
#define	BMAP_PIXRECT	5
#define	BMAP_SIZE	6

typedef	OBJID	BITMAP;

extern	BITMAP	Create_Bitmap();

#define	_BITMAP	1
#endif
