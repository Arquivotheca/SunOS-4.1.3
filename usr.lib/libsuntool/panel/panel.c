#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)panel.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*****************************************************************************/
/*                               panel.c                                     */
/*               Copyright (c) 1985 by Sun Microsystems, Inc.                */
/*****************************************************************************/

#include <sunwindow/rect.h>
#include <suntool/panel_impl.h>
#include <suntool/window.h>
#include <suntool/frame.h>

#ifdef KEYMAP_DEBUG
#include "../../libsunwindow/win/win_keymap.h"
#else
#include <sunwindow/win_keymap.h>
#endif
extern panel_default_event();

static panel_handle	init_panel();
static void		layout();

/*****************************************************************************/
/* panel_window_object                                                       */
/* called from win_create().  Calls init_panel(), which calls                */
/* panel_create_panel_struct() to actually malloc and initialize the panel   */
/* struct.                                                                   */
/*****************************************************************************/

caddr_t
panel_window_object(win)
Window win;
{   
    register panel_handle	panel;

   if (!(panel = init_panel(win))) return NULL;

    (void)window_set(win, 
	       WIN_NOTIFY_INFO,  	PW_FIXED_IMAGE | PW_INPUT_DEFAULT,
	       WIN_OBJECT,       	panel, 
	       WIN_SET_PROC,     	panel->ops->set_attr,
	       WIN_GET_PROC,     	panel->ops->get_attr,
	       WIN_NOTIFY_EVENT_PROC,   panel_notify_event,
	       WIN_EVENT_PROC,		panel_default_event,
	       WIN_DEFAULT_EVENT_PROC,  panel_default_event,
	       WIN_NOTIFY_DESTROY_PROC, panel_destroy,
	       WIN_LAYOUT_PROC,		layout,
	       WIN_TYPE,		PANEL_TYPE,
	       WIN_TOP_MARGIN,		PANEL_ITEM_Y_START,
	       WIN_LEFT_MARGIN,		PANEL_ITEM_X_START,
	       WIN_ROW_GAP,		ITEM_Y_GAP,
	       0);

    return (caddr_t)panel;
}

static panel_handle
init_panel(win)
Window	win;
{
   register panel_handle	panel;
   void pw_use_fast_monochrome();
   int	windowfd = 
   		(int)    window_get(win, WIN_FD);
   Frame frame = 
   	(Window) window_get(win, WIN_OWNER);
	int ownerfd = (int)window_get(frame, WIN_FD);

   if (!(panel = panel_create_panel_struct((caddr_t)win, windowfd))) 
      return NULL;

   panel->status |= USING_NOTIFIER;
   panel->status |= USING_WRAPPER;
   
   (void)panel_set_inputmask(panel, windowfd);

   panel->tool		= (Tool *)(LINT_CAST(frame));
   panel->pixwin	= (Pixwin *) (LINT_CAST(window_get(win, WIN_PIXWIN)));
   /* Use fast monochrome plane group (if available) */
   pw_use_fast_monochrome(panel->pixwin);

   panel->view_pixwin = pw_region(panel->pixwin, 0, 0,
                                  panel->rect.r_width,
                                  panel->rect.r_height);

   return panel;
}


/* update the panel when the window is
 * resized from window_set().
 */
/* ARGSUSED */
static void
layout(owner, panel, op)
Window			owner;
register panel_handle	panel;
Window_layout_op	op;
{
    register Rect	*new_win_rect;

    if (op != WIN_ADJUSTED)
	return;

    /* Note: maybe this should post a resize event
     * to the panel (or call panel_resize() directly).
     */
    new_win_rect = (Rect *) (LINT_CAST(window_get((Window)(LINT_CAST(panel)), 
    			WIN_RECT)));
    panel->rect  = *new_win_rect;
}
