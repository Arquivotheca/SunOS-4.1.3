/*	@(#)win_cursor.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
#ifndef win_cursor_DEFINED
#define	win_cursor_DEFINED

#ifndef KERNEL
#include <sunwindow/attr.h>
#endif not KERNEL

/*
 * Mouse cursor shape interface.
 *
 * The kernel tracks the mouse cursor and displays a different shape
 * depending on which window the cursor is directly over.
 *
 * The hot spot on the cursor is the offset of the mouse position from
 * the upper left hand corner of the cur_shape.
 *
 * The function is the operation used to put the cursor on the screen.
 * It is the value of a boolean expression of PIX_SRC and PIX_DEST
 * (see pixrect.h).
 *
 * The image is always a memory pixrect in order to facilitate exchange
 * of image data between the kernel and user processes.
 */

/*
 * In practice, for now, the kernel only supports a limited size
 * cur_shape.pr_data->md_image.  The following constants are the maximun number
 * of bytes and words that cur_shape.pr_data->md_image can have.
 */

#ifdef ecd.cursor
#define CURSOR_MAX_SIZE                 32      /* pixels (length, width) */
#define CURSOR_MIN_SIZE                 16      /* pixels (length, width) */
 
                                        /* max. # of image bytes */
#define CURSOR_MAX_IMAGE_BYTES          (CURSOR_MAX_SIZE*CURSOR_MAX_SIZE/8)
                                        /* max. # of image words */
#define CURSOR_MAX_IMAGE_WORDS          (CURSOR_MAX_IMAGE_BYTES/2)
#else
#define CURSOR_MAX_IMAGE_BYTES          32      /* max. # of image bytes */
#define CURSOR_MAX_IMAGE_WORDS          16
#endif ecd.cursor

/* these are for compatability with previous releases */
#define CUR_MAXIMAGEBYTES		CURSOR_MAX_IMAGE_BYTES
#define CUR_MAXIMAGEWORDS		CURSOR_MAX_IMAGE_WORDS

#define	CURSOR_MAX_HAIR_THICKNESS	5	/* max. crosshair thickness */

#define	CURSOR_TO_EDGE			-1

#ifndef KERNEL
#define	CURSOR_ATTR(type, ordinal)	ATTR(ATTR_PKG_CURSOR, type, ordinal)

typedef enum {

    /* boolean attributes */
    CURSOR_SHOW_CURSOR 			=  CURSOR_ATTR(ATTR_BOOLEAN, 1),
    CURSOR_SHOW_CROSSHAIRS 		=  CURSOR_ATTR(ATTR_BOOLEAN, 2),
    CURSOR_SHOW_HORIZ_HAIR 		=  CURSOR_ATTR(ATTR_BOOLEAN, 3),
    CURSOR_SHOW_VERT_HAIR 		=  CURSOR_ATTR(ATTR_BOOLEAN, 4),
    CURSOR_FULLSCREEN 			=  CURSOR_ATTR(ATTR_BOOLEAN, 5),
    CURSOR_CROSSHAIR_BORDER_GRAVITY 	=  CURSOR_ATTR(ATTR_BOOLEAN, 6),
    CURSOR_HORIZ_HAIR_BORDER_GRAVITY 	=  CURSOR_ATTR(ATTR_BOOLEAN, 7),
    CURSOR_VERT_HAIR_BORDER_GRAVITY 	=  CURSOR_ATTR(ATTR_BOOLEAN, 8),

    /* integer attributes */
    CURSOR_XHOT				= CURSOR_ATTR(ATTR_INT, 9),
    CURSOR_YHOT				= CURSOR_ATTR(ATTR_INT, 10),
    CURSOR_OP				= CURSOR_ATTR(ATTR_INT, 11),
    CURSOR_CROSSHAIR_COLOR		= CURSOR_ATTR(ATTR_INT, 12),
    CURSOR_CROSSHAIR_OP			= CURSOR_ATTR(ATTR_INT, 13),
    CURSOR_CROSSHAIR_THICKNESS		= CURSOR_ATTR(ATTR_INT, 14),
    CURSOR_CROSSHAIR_LENGTH		= CURSOR_ATTR(ATTR_INT, 15),
    CURSOR_CROSSHAIR_GAP		= CURSOR_ATTR(ATTR_INT, 16),
    CURSOR_HORIZ_HAIR_COLOR		= CURSOR_ATTR(ATTR_INT, 17),
    CURSOR_HORIZ_HAIR_OP		= CURSOR_ATTR(ATTR_INT, 18),
    CURSOR_HORIZ_HAIR_THICKNESS		= CURSOR_ATTR(ATTR_INT, 19),
    CURSOR_HORIZ_HAIR_LENGTH		= CURSOR_ATTR(ATTR_INT, 20),
    CURSOR_HORIZ_HAIR_GAP		= CURSOR_ATTR(ATTR_INT, 21),
    CURSOR_VERT_HAIR_COLOR		= CURSOR_ATTR(ATTR_INT, 22),
    CURSOR_VERT_HAIR_OP			= CURSOR_ATTR(ATTR_INT, 23),
    CURSOR_VERT_HAIR_THICKNESS		= CURSOR_ATTR(ATTR_INT, 24),
    CURSOR_VERT_HAIR_LENGTH		= CURSOR_ATTR(ATTR_INT, 25),
    CURSOR_VERT_HAIR_GAP		= CURSOR_ATTR(ATTR_INT, 26),

    /* pointer attributes */
    CURSOR_IMAGE			= CURSOR_ATTR(ATTR_PIXRECT_PTR, 27)
} Cursor_attribute;

typedef caddr_t	Cursor;

extern Cursor	cursor_create();
extern void	cursor_destroy();
extern Cursor	cursor_copy();
extern caddr_t	cursor_get();
extern int	cursor_set();
#endif not KERNEL

/* The following cursor struct definition is provided for compatibility
 * with previous releases.  New users of the cursor package should 
 * not rely on this definition; the routines cursor_create(), cursor_get(),
 * and cursor_set() should be used instead.
 */

struct cursor {
	short	cur_xhot, cur_yhot;	/* offset of mouse position from shape*/
	int	cur_function;		/* relationship of shape to screen */
	struct	pixrect *cur_shape;	/* memory image to use */
	int	flags;			/* various options */

	short	horiz_hair_thickness;	/* horizontal crosshair height */
	int	horiz_hair_op;		/*            drawing op       */
	int	horiz_hair_color;	/*            color            */ 
	short	horiz_hair_length;	/*            width           */ 
	short	horiz_hair_gap;		/*            gap             */ 

	short	vert_hair_thickness;	/* vertical crosshair width  */
	int	vert_hair_op;		/*          drawing op       */
	int	vert_hair_color;	/*          color            */ 
	short	vert_hair_length;	/*          height           */ 
	short	vert_hair_gap;   	/*          gap              */ 
};

/* This cursor struct allows previous releases to run on the current
 * kernel.  It is for internal use only and should not be used by new code.
 */
struct old_cursor {
	short	cur_xhot, cur_yhot;	/* offset of mouse position from shape*/
	int	cur_function;		/* relationship of shape to screen */
	struct	pixrect *cur_shape;	/* memory image to use */
};


/*
 * The following two macros are intended to make it easy for novice users to
 * make their own cursors without having to understand how to use mpr_static.
 */
#ifdef i386
#define DEFINE_CURSOR_FROM_IMAGE(name, hot_x, hot_y, func, image)	\
	static struct mpr_data CAT(name,_data) =			\
	    {mpr_linebytes(16,1), (short *)(image), {0, 0}, 0, MP_STATIC};	\
	extern struct pixrectops mem_ops;				\
	static struct pixrect CAT(name,_mpr) =				\
	    {&mem_ops, 16, 16, 1, (caddr_t)&CAT(name,_data)};		\
	static struct cursor name = {(hot_x), (hot_y), (func), &CAT(name,_mpr)};
#else
#define DEFINE_CURSOR_FROM_IMAGE(name, hot_x, hot_y, func, image)	\
	static struct mpr_data CAT(name,_data) =			\
	    {mpr_linebytes(16,1), (short *)(image), {0, 0}, 0, 0};	\
	extern struct pixrectops mem_ops;				\
	static struct pixrect CAT(name,_mpr) =				\
	    {&mem_ops, 16, 16, 1, (caddr_t)&CAT(name,_data)};		\
	static struct cursor name = {(hot_x), (hot_y), (func), &CAT(name,_mpr)};
#endif

#define	DEFINE_CURSOR(name, hot_x, hot_y, func, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11, i12, i13, i14, i15, i16)					\
	static short CAT(name,_image)[16] = {				\
	    i1, i2, i3, i4, i5, i6, i7, i8,				\
	    i9, i10, i11, i12, i13, i14, i15, i16};			\
	DEFINE_CURSOR_FROM_IMAGE(name, (hot_x), (hot_y), func, CAT(name,_image))


#endif
