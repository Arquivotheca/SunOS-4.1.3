/*	@(#)frame_impl.h 1.1 92/07/30 SMI	*/

/***********************************************************************/
/*	                      frame_impl.h		               */
/*          Copyright (c) 1985 by Sun Microsystems, Inc.               */
/***********************************************************************/

#ifndef frame_impl_DEFINED
#define frame_impl_DEFINED

#include <sunwindow/attr.h>
#include <suntool/help.h>
#include <suntool/frame.h>

#define frame_attr_next(attr) (Frame_attribute *)attr_next((caddr_t *)attr)

#define Pkg_private
#define Pkg_extern extern
#define Private static


struct list_node {
    struct list_node *next;
    caddr_t client;
    Toolsw *tsw;
};

struct toolplus {
    Tool tool;
    void (*done_proc)();
    void (*default_done_proc)();
    struct list_node frame_list;
    Frame shadow;
    int	  has_active_shadow;
    caddr_t help_data;
};

#define FRAME_REPAINT_LOCK	FRAME_ATTR(ATTR_BOOLEAN, 9)
#define FRAME_COLUMNS		TFRAME_ATTR(ATTR_X, 1)
#define FRAME_LINES		TFRAME_ATTR(ATTR_Y, 2)
#define FRAME_WIDTH		TFRAME_ATTR(ATTR_X, 3)
#define FRAME_HEIGHT		TFRAME_ATTR(ATTR_Y, 4)
#define FRAME_LEFT		TFRAME_ATTR(ATTR_X, 5)
#define FRAME_TOP		TFRAME_ATTR(ATTR_Y, 6)


#ifdef old_doc

/*
 * Compatibility Issues:  The new frame code attempts to map the tool
 *			  attrs as makes sense.  The key to the mapping is:
 *
 * NEW:			New name and possibly new functionality.
 * SAME:		Same functionality and name.
 * COMPATIBLE:		Scheduled to be removed but should work.
 * COMPATIBLE-:		Should work for set but get allocates storage
 * 			that needs to be explicitly freed w/tool_free_attribute.
 * GET CHANGED:		Same name, functionality superseded for get.
 * IGNORED:		Just a place holder.  Functionality is superseded
 *			by new window layer.
 */

/*
 * Old attrs here for documentation purposes
 */

    FRAME_ICONIC	= FRAME_ATTR(ATTR_BOOLEAN, 7),	/* COMPATIBLE */
    					/* window is iconic */
					/* 0 | 1 */
    FRAME_REPAINT_LOCK	= FRAME_ATTR(ATTR_BOOLEAN, 9), 	/* Same */
    					/* supress repainting */
					/* 0 | 1 */
    FRAME_LAYOUT_LOCK	= FRAME_ATTR(ATTR_BOOLEAN, 10),	/* COMPATIBLE */
    					/* surpress subwindow layout */
					/* 0 | 1 */
    FRAME_NAME_STRIPE	= FRAME_ATTR(ATTR_BOOLEAN, 11),	/* COMPATIBLE */
    					/* display name stripe */
					/* 0 | 1 */
    FRAME_BOUNDARY_MGR	= FRAME_ATTR(ATTR_BOOLEAN, 12),	/* COMPATIBLE */
    					/* enable subwindow boundary mover */
					/* 0 | 1 */
    FRAME_ICON_LEFT	= FRAME_ATTR(ATTR_X, 17),	/* COMPATIBLE */
    					/* icon''s x position on desktop */
					/* int */
    FRAME_ICON_TOP	= FRAME_ATTR(ATTR_Y, 18),	/* COMPATIBLE */
    					/* icon''s y position on desktop */
					/* int */
    FRAME_ICON_LABEL	= FRAME_ATTR(ATTR_STRING, 19),	/* COMPATIBLE- */
    					/* icon''s label */
					/* char * */
    FRAME_ICON_IMAGE	= FRAME_ATTR(ATTR_PIXRECT_PTR, 20), /* COMPATIBLE- */
    					/* icon''s graphic image */
					/* struct pixrect * (pixrect.h) */
    FRAME_ICON_FONT	= FRAME_ATTR(ATTR_PIXFONT_PTR, 21), /* COMPATIBLE */ 
    					/* icon label''s font */
					/* struct pixfont * (pixfont.h) */

#endif
#endif ~frame_impl_DEFINED
