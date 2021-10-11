/*	@(#)attr.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#ifndef sunwindow_attr_DEFINED
#define	sunwindow_attr_DEFINED

#include <sys/types.h> 

typedef caddr_t *	Attr_avlist;
typedef unsigned	Attr_attribute;		/* 32 bit quantity */
/* Attr_avlist is not an array of Attr_attributes, because it is an array
 * of intermixed attributes and values.
 */

/* This macro is machine dependent in that it assumes
 * the cardinality will be in the lower 5 bits of the type-cardinality
 * pair.
 */
#define	ATTR_TYPE(base_type, cardinality) \
    (((((unsigned)(base_type)) & 0x07f) << 5) |	\
     (((unsigned)(cardinality)) & 0x0f))

/* Base types in the range ATTR_BASE_UNUSED_FIRST
 * to ATTR_BASE_UNUSED_LAST are available for
 * client use.
 */
#define	ATTR_BASE_UNUSED_FIRST		0
#define	ATTR_BASE_UNUSED_LAST		31
/* Base types 32 through 63 are
 * reserved for future use.
 */
#define	ATTR_BASE_FIRST			64

typedef enum {
    ATTR_BASE_INT		= ATTR_BASE_FIRST,
    ATTR_BASE_INT_PAIR		= ATTR_BASE_FIRST + 1,
    ATTR_BASE_X			= ATTR_BASE_FIRST + 2,
    ATTR_BASE_INDEX_X		= ATTR_BASE_FIRST + 3,
    ATTR_BASE_Y			= ATTR_BASE_FIRST + 4,
    ATTR_BASE_INDEX_Y		= ATTR_BASE_FIRST + 5,
    ATTR_BASE_XY		= ATTR_BASE_FIRST + 6,
    ATTR_BASE_INDEX_XY		= ATTR_BASE_FIRST + 7,
    ATTR_BASE_BOOLEAN		= ATTR_BASE_FIRST + 8,
    ATTR_BASE_ENUM		= ATTR_BASE_FIRST + 9,
    ATTR_BASE_CHAR		= ATTR_BASE_FIRST + 10,
    ATTR_BASE_STRING		= ATTR_BASE_FIRST + 11,
    ATTR_BASE_PIXRECT_PTR	= ATTR_BASE_FIRST + 12,
    ATTR_BASE_PIXFONT_PTR	= ATTR_BASE_FIRST + 13,
    ATTR_BASE_PIXWIN_PTR	= ATTR_BASE_FIRST + 14,
    ATTR_BASE_RECT_PTR		= ATTR_BASE_FIRST + 15,
    ATTR_BASE_OPAQUE		= ATTR_BASE_FIRST + 16,
    ATTR_BASE_NO_VALUE		= ATTR_BASE_FIRST + 17,
    ATTR_BASE_AV		= ATTR_BASE_FIRST + 18,
    ATTR_BASE_FUNCTION_PTR	= ATTR_BASE_FIRST + 19,
    ATTR_BASE_ICON_PTR		= ATTR_BASE_FIRST + 20,
    ATTR_BASE_SINGLE_COLOR_PTR	= ATTR_BASE_FIRST + 21,
    ATTR_BASE_CURSOR_PTR	= ATTR_BASE_FIRST + 22,
    ATTR_BASE_INT_TRIPLE	= ATTR_BASE_FIRST + 23,
    ATTR_BASE_LONG		= ATTR_BASE_FIRST + 24,
    ATTR_BASE_SHORT		= ATTR_BASE_FIRST + 25
} Attr_base_type;

/* Clients of the attribute value package should use
 * Attr_base_cardinality elements to define the base type
 * and cardinality of their attributes.
 */  
typedef enum {
    ATTR_INT			= ATTR_TYPE(ATTR_BASE_INT, 1),
    ATTR_INT_PAIR		= ATTR_TYPE(ATTR_BASE_INT_PAIR, 2),
    ATTR_INT_TRIPLE		= ATTR_TYPE(ATTR_BASE_INT_TRIPLE, 3),
    ATTR_X			= ATTR_TYPE(ATTR_BASE_X, 1),
    ATTR_INDEX_X		= ATTR_TYPE(ATTR_BASE_INDEX_X, 2),
    ATTR_Y			= ATTR_TYPE(ATTR_BASE_Y, 1),
    ATTR_INDEX_Y		= ATTR_TYPE(ATTR_BASE_INDEX_Y, 2),
    ATTR_XY			= ATTR_TYPE(ATTR_BASE_XY, 2),
    ATTR_INDEX_XY		= ATTR_TYPE(ATTR_BASE_INDEX_XY, 3),
    ATTR_BOOLEAN		= ATTR_TYPE(ATTR_BASE_BOOLEAN, 1),
    ATTR_ENUM			= ATTR_TYPE(ATTR_BASE_ENUM, 1),
    ATTR_CHAR			= ATTR_TYPE(ATTR_BASE_CHAR, 1),
    ATTR_STRING			= ATTR_TYPE(ATTR_BASE_STRING, 1),
    ATTR_PIXRECT_PTR		= ATTR_TYPE(ATTR_BASE_PIXRECT_PTR, 1),
    ATTR_PIXFONT_PTR		= ATTR_TYPE(ATTR_BASE_PIXFONT_PTR, 1),
    ATTR_PIXWIN_PTR		= ATTR_TYPE(ATTR_BASE_PIXWIN_PTR, 1),
    ATTR_RECT_PTR		= ATTR_TYPE(ATTR_BASE_RECT_PTR, 1),
    ATTR_OPAQUE			= ATTR_TYPE(ATTR_BASE_OPAQUE, 1),
    ATTR_NO_VALUE		= ATTR_TYPE(ATTR_BASE_NO_VALUE, 0),
    ATTR_AV			= ATTR_TYPE(ATTR_BASE_AV, 1),
    ATTR_FUNCTION_PTR		= ATTR_TYPE(ATTR_BASE_FUNCTION_PTR, 1),
    ATTR_ICON_PTR		= ATTR_TYPE(ATTR_BASE_ICON_PTR, 1),
    ATTR_SINGLE_COLOR_PTR	= ATTR_TYPE(ATTR_BASE_SINGLE_COLOR_PTR, 1),
    ATTR_CURSOR_PTR		= ATTR_TYPE(ATTR_BASE_CURSOR_PTR, 1),
    ATTR_LONG			= ATTR_TYPE(ATTR_BASE_LONG, 1),
    ATTR_SHORT			= ATTR_TYPE(ATTR_BASE_SHORT, 1)
} Attr_base_cardinality;

/* Note that this macro is machine dependent in that it assumes the
 * base_type-cardinality pair will be in the lower 13 bits of the 
 * list_type-base_cardinality pair.
 */
#define	ATTR_LIST_OF(list_type, list_ptr_type, base_cardinality) \
    (((((unsigned)(list_type)) & 0x3) << 14) | \
     (((unsigned)(list_ptr_type) & 0x1) << 13) | \
     (((unsigned)(base_cardinality)) & 0x3fff))

typedef enum {
    ATTR_LIST_IS_INLINE	= 0,
    ATTR_LIST_IS_PTR	= 1
} Attr_list_ptr_type;

#define	ATTR_LIST_INLINE(list_type, base_cardinality)	\
    ATTR_LIST_OF(list_type, ATTR_LIST_IS_INLINE, base_cardinality)
    
#define	ATTR_LIST_PTR(list_type, base_cardinality)	\
    ATTR_LIST_OF(list_type, ATTR_LIST_IS_PTR, base_cardinality)
    
typedef enum {
    /* Note that ATTR_NONE must have a value of zero,
     * since a no-list type is assumed for each of the
     * types in Attr_base_cardinality.
     */
    ATTR_NONE		= 0,
    ATTR_RECURSIVE	= 1,
    ATTR_NULL		= 2,
    ATTR_COUNTED	= 3
} Attr_list_type;

/*
 * Note that 0 is NEVER a valid package id.
 *
 * The range from ATTR_PKG_UNUSED_FIRST to
 * ATTR_PKG_UNUSED_LAST is reserved for non-Sun packages.
 */
#define	ATTR_PKG_UNUSED_FIRST	1
#define ATTR_PKG_UNUSED_LAST	31
/* 32 through 63 are reserved for 
 * future use.
 */
#define	ATTR_PKG_FIRST		64
/* #define	ATTR_PKG_LAST		127 */
/* 128 through 255 are spare */
    
typedef enum {
    ATTR_PKG_ZERO	= 0,
    ATTR_PKG_GENERIC	= ATTR_PKG_FIRST,
    ATTR_PKG_PANEL	= ATTR_PKG_FIRST + 1,
    ATTR_PKG_SCROLLBAR	= ATTR_PKG_FIRST + 2,
    ATTR_PKG_CURSOR	= ATTR_PKG_FIRST + 3,
    ATTR_PKG_MENU	= ATTR_PKG_FIRST + 4,
    ATTR_PKG_TEXTSW	= ATTR_PKG_FIRST + 5,
    ATTR_PKG_TOOL	= ATTR_PKG_FIRST + 6,
    ATTR_PKG_SELN_BASE	= ATTR_PKG_FIRST + 7,
    ATTR_PKG_ENTITY	= ATTR_PKG_FIRST + 8, 
    ATTR_PKG_IMAGE	= ATTR_PKG_FIRST + 9, 
    ATTR_PKG_WIN	= ATTR_PKG_FIRST + 10, 
    ATTR_PKG_FRAME	= ATTR_PKG_FIRST + 11, 
    ATTR_PKG_CANVAS	= ATTR_PKG_FIRST + 12, 
    ATTR_PKG_TTY	= ATTR_PKG_FIRST + 13, 
    ATTR_PKG_ICON	= ATTR_PKG_FIRST + 14, 
    ATTR_PKG_NOP	= ATTR_PKG_FIRST + 15, 
    ATTR_PKG_PIXWIN	= ATTR_PKG_FIRST + 16,
    ATTR_PKG_ALERT	= ATTR_PKG_FIRST + 17, 
    ATTR_PKG_HELP	= ATTR_PKG_FIRST + 18,
#ifndef PRE_FLAMINGO
    ATTR_PKG_VIDEO	= ATTR_PKG_FIRST + 19,
    ATTR_PKG_LAST	= ATTR_PKG_FIRST + 19,
#else
    ATTR_PKG_LAST	= ATTR_PKG_FIRST + 18
#endif
    /* Change ATTR_PKG_LAST to be EQUAL to the last legal pkg id. */
    /* The procedure counter(), called by attr_make, aborts if */
    /* PKG_ID > ATTR_PKG_LAST */
    /* PKG name should also be added to attr_names[] in attr.c */
} Attr_pkg;

/* An attribute is composed of 
 * pkg     ( 8 bits): id of package that uses the attribute
 * ordinal ( 8 bits): ordinal number of the attribute
 * type    (16 bits): list type, list pointer type, base type, and cardinality
 */
#define	ATTR(pkg, type, ordinal)	\
    ( ((((unsigned)(pkg))	& 0x7f) << 24) | \
      ((((unsigned)(ordinal))	& 0xff) << 16) | \
       (((unsigned)(type))	& 0xefef)	)

typedef union {
    struct {
	Attr_pkg		pkg		: 8;
	unsigned		ordinal		: 8;
	Attr_list_type		list_type	: 2;
	Attr_list_ptr_type	list_ptr_type	: 1;
	unsigned		spare1		: 1;	/* unused */
	Attr_base_type		base_type	: 7;
	unsigned		spare2		: 1;	/* unused */
	unsigned		cardinality	: 4;
    } 			info;
    Attr_attribute	code;
} Attr_union;

/* Generic attributes
*/
typedef enum {
    ATTR_LIST = ATTR(ATTR_PKG_GENERIC, ATTR_LIST_PTR(ATTR_RECURSIVE, ATTR_NO_VALUE), 0)
} Attr_generic;

/* ATTR_STANDARD_SIZE is large enough to allow for 
 * most attribute-value lists.
 */
#define	ATTR_STANDARD_SIZE	250

#ifdef notdef
/* Note that in these macros, attr must not be 
 * in a register or be a constant.
 * Since this is deemed too restrictive, we use
 * shifting & masking instead.  
 */
#define ATTR_UNION(attr)	((Attr_union *) &((Attr_attribute) (attr)))
#define	ATTR_INFO(attr)		(ATTR_UNION(attr)->info)
#define	ATTR_CODE(attr)		(ATTR_UNION(attr)->code)
#define	ATTR_LIST_TYPE(attr)	(ATTR_INFO(attr).list_type)
#define	ATTR_LIST_PTR_TYPE(attr)	(ATTR_INFO(attr).list_ptr_type)
#define	ATTR_BASE_TYPE(attr)	(ATTR_INFO(attr).base_type)
#define	ATTR_CARDINALITY(attr)	(ATTR_INFO(attr).cardinality)
#define	ATTR_PKG(attr)		(ATTR_INFO(attr).pkg)
#define	ATTR_ORDINAL(attr)	(ATTR_INFO(attr).ordinal)
#endif notdef

#define	ATTR_CODE(attr)		((unsigned) (attr))

#define	ATTR_PKG(attr)	\
	((Attr_pkg) ((ATTR_CODE(attr) >> 24) & 0xFF))

#define ATTR_VALID_PKG_ID(attr)		\
	(((int)ATTR_PKG(attr)) > ((int) ATTR_PKG_ZERO) && \
	 ((int)ATTR_PKG(attr)) <= ((int)ATTR_PKG_LAST))

#define	ATTR_ORDINAL(attr)	\
	((unsigned) ((ATTR_CODE(attr) >> 16) & 0xFF))

#define	ATTR_LIST_TYPE(attr)	\
	((Attr_list_type) ((ATTR_CODE(attr) >> 14) & 0x3))

#define	ATTR_LIST_PTR_TYPE(attr)	\
	((Attr_list_ptr_type) ((ATTR_CODE(attr) >> 13) & 0x1))

#define	ATTR_BASE_TYPE(attr)	\
	((Attr_base_type) ((ATTR_CODE(attr) >> 5) & 0x7F))

#define	ATTR_CARDINALITY(attr)	\
	((unsigned) (ATTR_CODE(attr) & 0xF))


#define	attr_skip(attr, argv)	\
    ((ATTR_LIST_TYPE((attr)) == ATTR_NONE) \
	? (argv) + ATTR_CARDINALITY((attr)) \
	: ((Attr_avlist)attr_skip_value((Attr_attribute)(attr), (argv))))

#define	attr_next(attrs)	attr_skip((*(attrs)), ((attrs)+1))

/* Character unit support */

#ifdef	lint
/* The following #ifndef fixes kernel lint warnings, but is pretty strange */
#ifndef lint
extern	void	attr_replace_cu();
extern	int	attr_cu_to_x();
extern	int	attr_cu_to_y();
#endif
#else	lint
#define attr_replace_cu(avlist, font, lmargin, tmargin, rgap) \
    attr_rc_units_to_pixels(avlist, font->pf_defaultsize.x, \
	font->pf_defaultsize.y, lmargin, tmargin, 0, rgap)
    
#define attr_cu_to_x(encoded_value, font, left_margin) \
    attr_rc_unit_to_x(encoded_value, font->pf_defaultsize.x, left_margin, 0)

#define attr_cu_to_y(encoded_value, font, top_margin, row_gap) \
    attr_rc_unit_to_y(encoded_value, font->pf_defaultsize.y, top_margin, row_gap)
#endif	lint

typedef enum {
    ATTR_CU_POSITION	= 0x0,		/* bit 29 is off */
    ATTR_CU_LENGTH	= 0x20000000	/* bit 29 is on */

} Attr_cu_type;

#define	ATTR_CU_TAG		0x80000000
#define ATTR_PIXEL_OFFSET	0x00008000

#define	ATTR_CU(unit, n)	\
   (((unsigned)(unit)) | (((n) & 0x1FFF) << 16) |	\
    ATTR_CU_TAG | ATTR_PIXEL_OFFSET)
    
#define	ATTR_CU_MASK		0xC0000000
#define	ATTR_CU_TYPE(n)		\
    ((Attr_cu_type) ((n) & (unsigned) (ATTR_CU_LENGTH)))

/* attr_is_cu(n) returns non-zero if n has 
 * been encoded using ATTR_CU()
 */
#define	attr_is_cu(n)		(((n) & ATTR_CU_MASK) == ATTR_CU_TAG)

/* Macros for position including
 * margins.
 */
#define	ATTR_COL(n)		ATTR_CU(ATTR_CU_POSITION, n)
#define	ATTR_ROW(n)		ATTR_CU(ATTR_CU_POSITION, n)

/* Macros for length excluding
 * margins.
 */
#define	ATTR_COLS(n)		ATTR_CU(ATTR_CU_LENGTH, n)
#define	ATTR_ROWS(n)		ATTR_CU(ATTR_CU_LENGTH, n)
#define	ATTR_CHARS(n)		ATTR_CU(ATTR_CU_LENGTH, n)
#define	ATTR_LINES(n)		ATTR_CU(ATTR_CU_LENGTH, n)
   

/* attr_make() is not interested in the
 * count from attr_make_count().
 */
#define	attr_make(array, max_size, argv)	\
    attr_make_count(array, max_size, argv, (int *)0)

/* Following are useful for multi-pass avlist processing. */
#define	ATTR_NOP(attr)					\
	( (((unsigned)ATTR_PKG_NOP) << 24) | (ATTR_CODE(attr) & 0xffffff) )
#define	ATTR_CONSUME(attr)	(attr) = ((caddr_t)ATTR_NOP(attr))

#ifndef lint
/* Available functions
 */
extern Attr_avlist	attr_create_list();
extern Attr_avlist	attr_create();
extern Attr_avlist	attr_make_count();
extern int		attr_copy();
extern int		attr_count();
extern char 	       *attr_sprint();
extern Attr_avlist	attr_skip_value();
extern void		attr_free();
extern int		attr_rc_unit_to_x();
extern int		attr_rc_unit_to_y();
extern void		attr_rc_units_to_pixels();
#endif lint
#endif


