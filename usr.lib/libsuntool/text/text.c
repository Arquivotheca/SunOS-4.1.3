#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)text.c 1.1 92/07/30 Copyright 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*-
	WINDOW wrapper

	text.c, Mon Sep  2 15:42:54 1985

		Craig Taylor,
		Sun Microsystems
 */

#include <suntool/primal.h>
#include <suntool/textsw_impl.h>
#include <sunwindow/win_struct.h>
#include <suntool/window.h>
#include <suntool/text.h>

#define window_attr_next(attr) (Window_attribute *)attr_next((caddr_t *)attr)

		    
caddr_t textsw_get();
static Textsw_status textsw_wrapped_set();
Window win_from_client();
caddr_t win_set_client();

caddr_t
textsw_window_object(win, avlist)
	Window win;
	Attr_avlist avlist;
{   
    Textsw		  textsw;
    Textsw_view_object	  *view;

    textsw = textsw_build((struct tool *)(LINT_CAST(
    		window_get(win, WIN_OWNER))), ATTR_LIST, avlist, 0);
    if (!textsw) return NULL;
    view = (Textsw_view_object *)(LINT_CAST(textsw));  /* Cast to implementation */
    (void)window_set(win,
	       WIN_SHOW, TRUE, /* Set the default to be exposed */
	       WIN_OBJECT, view, 
	       WIN_COMPATIBILITY_INFO,
	         WIN_FD_FOR_VIEW(view), PIXWIN_FOR_VIEW(view),
	       WIN_TYPE, TEXT_TYPE,
	       WIN_SET_PROC, textsw_wrapped_set, WIN_GET_PROC, textsw_get,
	       WIN_MENU, textsw_get((Textsw)(LINT_CAST(view)), TEXTSW_MENU),
	       0);
    return (caddr_t)view;
}


caddr_t
textsw_view_window_object(win, avlist)
	Window win;
	Window_attribute *avlist;
{   
    Textsw_view_object	  *view;
    Window_attribute	  *attrs;

    for (attrs = avlist; *attrs; attrs = window_attr_next(attrs))
	switch (attrs[0]) {

	  case WIN_OBJECT:
	    view = (Textsw_view_object *)attrs[1];
	    break;
	    
	  case WIN_SHOW:  /* Because this skips the standard initialization */
	    break;
	}	    
    (void)window_set(win,
	       WIN_SHOW, TRUE, /* Set the default to be exposed */
	       WIN_OBJECT, view,
	       WIN_COMPATIBILITY_INFO,
	         WIN_FD_FOR_VIEW(view), PIXWIN_FOR_VIEW(view),
	       WIN_TYPE, TEXT_TYPE,
	       WIN_SET_PROC, textsw_wrapped_set, WIN_GET_PROC, textsw_get,
	       WIN_MENU, textsw_get((Textsw)(LINT_CAST(view)), TEXTSW_MENU),
	       0);
    return (caddr_t)view;
}


void /* Package private */
textsw_swap_wins(view1, view2)
    Textsw_view_object	  *view1;
    Textsw_view_object	  *view2;
{
    register Window w1, w2;
    
    w1 = win_from_client((Window)(LINT_CAST(view1)));
    w2 = win_from_client((Window)(LINT_CAST(view2)));
    (void)win_set_client((struct window *)(LINT_CAST(w1)), (caddr_t)(LINT_CAST(view2)), FALSE);
    (void)win_set_client((struct window *)(LINT_CAST(w2)), (caddr_t)(LINT_CAST(view1)), FALSE);
}


void /* Package private */
textsw_clear_win(view)
    Textsw_view_object	  *view;
{
    (void)win_set_client((struct window *)(LINT_CAST(
    	win_from_client((Window)(LINT_CAST(view))))),(caddr_t)NULL, FALSE);
}


static Textsw_status
textsw_wrapped_set(abstract, attr_argv)
	Textsw		abstract;
	caddr_t		attr_argv[];
{   
    return(textsw_set_internal((Textsw_view)(LINT_CAST(
    	VIEW_ABS_TO_REP(abstract))), attr_argv));
}
