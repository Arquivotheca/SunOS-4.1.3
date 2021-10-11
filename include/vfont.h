/*	@(#)vfont.h 1.1 92/07/30 SMI; from UCB 4.1 83/05/03	*/

/*
 * The structures header and dispatch define the format of a font file.
 *
 * A font file contains one struct 'header', an array of NUM_DISPATCH struct
 * 'dispatch'es, then an array of bytes containing bit maps.
 *
 * See vfont(5) for more details.
 */

#ifndef _vfont_h
#define _vfont_h

struct header {
	short		magic;		/* Magic number VFONT_MAGIC */
	unsigned short	size;		/* Total # bytes of bitmaps */
	short		maxx;		/* Maximum horizontal glyph size */
	short		maxy;		/* Maximum vertical   glyph size */
	short		xtend;		/* (unused?) */
}; 
#define	VFONT_MAGIC	0436

struct dispatch {
	unsigned short	addr;		/* &(glyph) - &(start of bitmaps) */
	short		nbytes;		/* # bytes of glyphs (0 if no glyph) */
	char		up, down, left, right;	/* Widths from baseline point */
	short		width;		/* Logical width, used by troff */
};
#define	NUM_DISPATCH	256

#endif /*!_vfont_h*/
