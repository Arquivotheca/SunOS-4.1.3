#ifndef lint
#ifdef sccs
static        char sccsid[] = "@(#)frame_set.c 1.1 92/07/30 Copyright 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 *  frame_set: Sets frame attributes
 */

/* ------------------------------------------------------------------------- */

#include <stdio.h>

#include <sys/time.h>
#include <sys/types.h>

#include <pixrect/pixrect.h>

#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>

#include <suntool/tool_struct.h>
#include <suntool/window.h>
#include <suntool/wmgr.h>
#include <suntool/walkmenu.h>

#include "suntool/tool_impl.h"
#include "suntool/frame_impl.h"

/* ------------------------------------------------------------------------- */

/* 
 * Public
 */

extern int			win_getrect(), win_getsavedrect();
extern int			win_setrect(), win_setsavedrect();
void				frame_cmdline_help();


/* 
 * Package private
 */

Pkg_private int/*bool*/		frame_set();

Pkg_extern void			frame_default_done_func();


/* 
 * Private
 */

Private void			free_argc_argv_lists();


/* ------------------------------------------------------------------------- */



int/*bool*/
frame_set(win, avlist)
	Window win;
	Frame_attribute avlist[];
{   
    Frame_attribute	*attrs;
    Tool		*tool = (Tool *)(LINT_CAST(win));
    int			x;
    int			iconic = -1, tool_set = FALSE;
    void		(*help_proc)() = frame_cmdline_help;

    for (attrs = avlist; *attrs; attrs = frame_attr_next(attrs)) {
	switch (attrs[0]) {
	    
	  case WIN_MENU:
	    tool->tl_menu = (Menu)attrs[1];
	    break;

	  case WIN_SHOW_UPDATES:
	    x = FRAME_REPAINT_LOCK; /* Compiler work around */
	    attrs[0] = (Frame_attribute)x;
	    attrs[1] = (Frame_attribute)!(int)attrs[1];
	    break;

	  case FRAME_ARGC_PTR_ARGV:
	  case_frame_argc_ptr_argv: {
	    Frame_attribute *parsed_args = 0;
	    char *name = ((char **)attrs[2])[0];

	    if (-1 == tool_parse_all((int *)(LINT_CAST(attrs[1])), 
	    			     (char **)(LINT_CAST(attrs[2])), 
				     (char ***)(LINT_CAST(&parsed_args)), name)) {
		(help_proc)(name);
		exit(1);
	    } else {
		Attr_generic fa = ATTR_LIST;   /* Compiler work around */
		attrs[0] = (Frame_attribute)fa;
	    }
	    attrs[1] = (Frame_attribute)parsed_args;
	    attrs[2] = FRAME_NULL_ATTR;
	    tool_set = TRUE;
	    if (parsed_args) {
		Frame_attribute *attributes = parsed_args;
		
		for (;*attributes; attributes = frame_attr_next(attributes))
		    switch (*attributes) {
		      
		      case FRAME_CLOSED:
			if (!window_get(win, WIN_OWNER))
			    if (window_get(win, WIN_CREATED))
				break;
			    else
				iconic = (int)attributes[1];
			attributes[0] = attributes[1] = FRAME_NULL_ATTR;
			break;
			
		      case FRAME_COLUMNS:
			(Window_attribute)*attributes = WIN_COLUMNS;
			break;
		      
		      case FRAME_LINES:
			(Window_attribute)*attributes = WIN_ROWS;
			break;

		      case FRAME_WIDTH:
			(Window_attribute)*attributes = WIN_WIDTH;
			break;

		      case FRAME_HEIGHT:
			(Window_attribute)*attributes = WIN_HEIGHT;
			break;

		      case FRAME_LEFT:
			(Window_attribute)*attributes = WIN_X;
			break;

		      case FRAME_TOP:
			(Window_attribute)*attributes = WIN_Y;
			break;

		    }
		(void)window_set(win, WIN_POSTSET_PROC, free_argc_argv_lists, 0);
	    }
	    break;
	  }

	  case FRAME_ARGS:
	    x = (int)attrs[1];
	    attrs[1] = (Frame_attribute)&x;
	    goto case_frame_argc_ptr_argv;
	  
	  case FRAME_CLOSED:			/* WIN_ICONIC */
	    if (!window_get(win, WIN_OWNER))
		if (window_get(win, WIN_CREATED))
		    break;
		else
		    iconic = (int)attrs[1];
	    attrs[0] = attrs[1] = FRAME_NULL_ATTR;
	    break;

	  case FRAME_CLOSED_RECT:
	    if (window_get(win, WIN_OWNER)) break;	/* Is a subframe? */
	    
	    if (tool_is_iconic(tool))
		(void)win_setrect(tool->tl_windowfd, (struct rect *)(LINT_CAST(
			attrs[1])));
	    else
		(void)win_setsavedrect(tool->tl_windowfd, (struct rect *)(LINT_CAST(
			attrs[1])));
	    break;

	  case FRAME_CURRENT_RECT:
	    if (window_get(win, WIN_OWNER)) {	       /* Is a subframe? */
		(void)window_set(win, WIN_RECT, attrs[1],0);
	    }
	    
	    (void)win_setrect(tool->tl_windowfd, (struct rect *)(LINT_CAST(attrs[1])));
	    break;

	  case FRAME_DEFAULT_DONE_PROC:
	    ((struct toolplus *)(LINT_CAST(win)))->default_done_proc = 
	    	(void (*)())attrs[1];
	    if (!((struct toolplus *)(LINT_CAST(win)))->default_done_proc)
		((struct toolplus *)(LINT_CAST(win)))->default_done_proc =
		    frame_default_done_func;
	    break;

	  case FRAME_DONE_PROC:
	    ((struct toolplus *)(LINT_CAST(win)))->done_proc = 
	    	(void (*)())attrs[1];
	    break;
	  
	  case FRAME_PROPS_ACTION_PROC:
	   {
	    Menu_item props_item;
	    Menu     tool_menu = tool->tl_menu;

	    if (tool_menu) {
		/* only walking menu has "props" item */
	        tool->props_proc = 
	    	    (void (*)())attrs[1];
	    }		
	    break;
	  }
	  
	  case FRAME_PROPS_ACTIVE:  {
	      Menu_item  props_item;
	      Menu       tool_menu = ((struct tool *)LINT_CAST(win))->tl_menu;
	      
	      if ((int)attrs[1] != tool->props_active) {
	          tool->props_active = (int)attrs[1];
		  menu_set(menu_find(tool_menu, MENU_STRING, "Props", 0),
		  	MENU_INACTIVE, ((int)attrs[1] ? FALSE : TRUE), 0);
	      }
	      break;
	  }
	  	       
	  case FRAME_EMBOLDEN_LABEL:
	    if (attrs[1])
		tool->tl_flags |= TOOL_EMBOLDEN_LABEL;
	    else
		tool->tl_flags &= ~TOOL_EMBOLDEN_LABEL;
	    (void)tool_displaynamestripe(tool);
	    break;

	  case FRAME_CMDLINE_HELP_PROC:
	    help_proc = (void (*)())attrs[1];
	    break;	    
	    
	  case FRAME_NO_CONFIRM:
	    if (attrs[1])
		tool->tl_flags |= TOOL_NO_CONFIRM;
	    else
		tool->tl_flags &= ~TOOL_NO_CONFIRM;
	    break;

	  case FRAME_OPEN_RECT:
	    if (window_get(win, WIN_OWNER)) {	       /* Is a subframe? */
		(void)window_set(win, WIN_RECT, attrs[1], 0);
	    }

	    if (tool_is_iconic(tool))
		(void)win_setsavedrect(tool->tl_windowfd, (struct rect *)(LINT_CAST(
			attrs[1])));
	    else
		(void)win_setrect(tool->tl_windowfd, (struct rect *)(LINT_CAST(
			attrs[1])));
	    break;

	  case FRAME_SHOW_LABEL: /* WIN_NAME_STRIPE: */
	    {   
		struct list_node *node;
		Rect swrect;
		int n;
		
	 /*  TOP_MARGIN SET BELOW */	
	/*(void)window_set(win, WIN_TOP_MARGIN, tool_headerheight((int)attrs[1]), 0); */

		if (!(tool->tl_flags & TOOL_NAMESTRIPE) == !attrs[1]) break;
		/* Relocate hidden sw''s (tool code doesn''t do this) */
		n = tool_headerheight(TRUE) - TOOL_BORDERWIDTH;
		if (!attrs[1]) n = -n;
		node = ((struct toolplus *)tool)->frame_list.next;
		for (; node; node = node->next) {
		    if (node->tsw) {
			(void)win_getrect(node->tsw->ts_windowfd, &swrect);
			swrect.r_top += n;
			(void)win_setrect(node->tsw->ts_windowfd, &swrect);
		    }
		}
	    }
	    break;
	  case FRAME_SHOW_SHADOW:
	    if (attrs[1]) {
	        if (!((struct toolplus *)
	              (LINT_CAST(win)))->has_active_shadow) {
	              
	           if (((struct toolplus *)
	                    (LINT_CAST(win)))->shadow == NULL) {
	               shadow_frame(win);
	           } else
	               ((struct toolplus *)
	                     (LINT_CAST(win)))->has_active_shadow  = TRUE;
	                 
	           window_set(((struct toolplus *)
	                       (LINT_CAST(win)))->shadow, WIN_SHOW, TRUE, 0);
	        }
	     } else {
	        if (((struct toolplus *)
	              (LINT_CAST(win)))->has_active_shadow) {
	            
	            ((struct toolplus *)
	                   (LINT_CAST(win)))->has_active_shadow = FALSE;
	            window_set(((struct toolplus *)
	                       (LINT_CAST(win)))->shadow, WIN_SHOW, FALSE, 0);
	         }
	    }
	    break;  
	  case FRAME_NULL_ATTR:
	    break;
	  case HELP_DATA:
	    ((struct toolplus *)win)->help_data = (caddr_t)attrs[1];
  	    break;
 
	  default:
	    if (ATTR_PKG_FRAME == ATTR_PKG(attrs[0]))
		(void)fprintf(stderr,
			"frame_set: Frame attribute not allowed.\n%s\n",
			attr_sprint((char *)NULL, (unsigned)attrs[0]));
	    break;

	}

/* 	  case FRAME_CLOSED_X:			WIN_ICON_LEFT */
/* 	  case FRAME_CLOSED_Y:			WIN_ICON_TOP */
/* 	  case FRAME_BACKGROUND_COLOR:		WIN_BACKGROUND */
/* 	  case FRAME_FOREGROUND_COLOR:		WIN_FOREGROUND */
/* 	  case FRAME_ICON:			WIN_ICON */
/* 	  case FRAME_INHERIT_COLORS:		WIN_DEFAULT_CMS */
/* 	  case FRAME_LABEL:			WIN_LABEL */
/* 	  case FRAME_SUBWINDOWS_ADJUSTABLE:	WIN_BOUNDARY_MGR */
	  
	if (ATTR_PKG_TOOL ==  ATTR_PKG(attrs[0])) tool_set = TRUE;
    }

    if (tool_set) {
	(void)tool_set_attributes(tool, ATTR_LIST, avlist, 0);
	(void)window_set(win, 
   	     WIN_TOP_MARGIN, tool_headerheight((int) (tool->tl_flags & TOOL_NAMESTRIPE)),
     0);
	if (tool_check_state(tool)) {
	    if (!window_get(win, WIN_CREATED)) (void)win_remove(tool->tl_windowfd);
	    (void)win_getsize(tool->tl_windowfd, &tool->tl_rectcache);
	}
    }
    
    if (iconic == TRUE && !tool_is_iconic(tool)) {
	struct	rect rect, savedrect;
	int fd = tool->tl_windowfd;
	
	(void)win_getrect(fd, &savedrect);
	(void)win_getsavedrect(fd, &rect);
	(void)win_setsavedrect(fd, &savedrect);
	(void)win_setrect(fd, &rect);
	tool->tl_flags |= TOOL_ICONIC;
	(void)win_setuserflags(fd, win_getuserflags(fd) | WMGR_ICONIC);
    }

    return TRUE;
}


void
frame_cmdline_help(name)
char *name;
{   
    (void)tool_usage(name);
}


Private void
free_argc_argv_lists(win, avlist)
	Window win;
	Frame_attribute avlist[];
{   
    Frame_attribute *attrs;
		
    for (attrs = avlist; *attrs; attrs = frame_attr_next(attrs))
	if ((Attr_generic)attrs[0] == ATTR_LIST && attrs[2] == FRAME_NULL_ATTR)
	    (void)tool_free_attribute_list((char **)(LINT_CAST(attrs[1])));
    (void)window_set(win, WIN_POSTSET_PROC, NULL, 0);
}
