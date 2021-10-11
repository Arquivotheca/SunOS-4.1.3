#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)wmgr_walkmenu.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Window mgr walkmenu handling.
 */

#include <stdio.h>
#include <ctype.h>
#include <suntool/tool_hs.h>
#include "suntool/tool_impl.h"
#include <sunwindow/defaults.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <suntool/walkmenu.h>
#include <suntool/help.h>

#define HELP_INFO(s) HELP_DATA,s,

#define TOOL_MENU_ITEM(label, proc, help) \
  MENU_ITEM, MENU_STRING, label, MENU_ACTION, proc, \
             MENU_CLIENT_DATA, tool, HELP_INFO(help) 0

void wmgr_top(), wmgr_open();
/*
 * Tool menu creation
 */

void		tool_menu_cstretch(), tool_menu_stretch(),
		tool_menu_zoom(), tool_menu_fullscreen(),
		tool_menu_expose(), tool_menu_hide(), tool_menu_open(),
		tool_menu_move(), tool_menu_cmove(), tool_menu_redisplay(),
		tool_menu_quit(), tool_menu_props();
Menu_item	tool_menu_zoom_state(), tool_menu_open_state(),
		tool_menu_fullscreen_state();
void		wmgr_fullscreen();

extern void 	wmgr_changerect();

     
Menu
tool_walkmenu(tool)
	register Tool *tool;
{   
    Menu	tool_menu;
    Menu	menu1;

    if (defaults_get_boolean("/Compatibility/New_Frame_Menu", TRUE, NULL))  {
	menu1 = menu_create(
		 TOOL_MENU_ITEM("Unconstrained", tool_menu_stretch,
				"sunview:toolucresize"),
		 TOOL_MENU_ITEM("Constrained", tool_menu_cstretch,
				"sunview:toolcresize"),
		 MENU_ITEM,
		   MENU_STRING, "Zoom",
		   MENU_ACTION, tool_menu_zoom,
		   MENU_GEN_PROC, tool_menu_zoom_state, 
		   MENU_CLIENT_DATA, tool,
		   HELP_INFO("sunview:toolzoom")
		   0, 
                 MENU_ITEM,
                   MENU_STRING, "FullScreen",
                   MENU_ACTION, tool_menu_fullscreen,
                   MENU_GEN_PROC, tool_menu_fullscreen_state,
                   MENU_CLIENT_DATA, tool,
                   HELP_INFO("sunview:toolfullscreen")
                   0,
	         HELP_INFO("sunview:toolresize")
		 0);
        tool_menu = menu_create(
	    MENU_ITEM,
	      MENU_STRING, "Open",
	      MENU_ACTION, tool_menu_open,
	      MENU_GEN_PROC, tool_menu_open_state,
	      MENU_CLIENT_DATA, tool,
	      HELP_INFO("sunview:toolopen")
	      0, 
	    MENU_ITEM,
	      MENU_STRING, "Move",
	      MENU_PULLRIGHT, menu_create(
		TOOL_MENU_ITEM("Unconstrained", tool_menu_move,
			       "sunview:toolucmove"), 
		TOOL_MENU_ITEM("Constrained", tool_menu_cmove,
			       "sunview:toolcmove"),
	        HELP_INFO("sunview:toolmove")
		0),
	      HELP_INFO("sunview:toolmove")
	      0,
	    MENU_ITEM,
	      MENU_STRING, "Resize",
	      MENU_PULLRIGHT, menu1,
	      HELP_INFO("sunview:toolresize")
	      0,
	    TOOL_MENU_ITEM("Front",tool_menu_expose, "sunview:toolfront"),
	    TOOL_MENU_ITEM("Back", tool_menu_hide, "sunview:toolback"),
  	    MENU_ITEM,
	      MENU_STRING, "Props",
	      MENU_ACTION, tool_menu_props, 
	      MENU_CLIENT_DATA, tool,
	      MENU_INACTIVE, TRUE,
	      HELP_INFO("sunview:toolprops")
	      0,
	    TOOL_MENU_ITEM("Redisplay", tool_menu_redisplay,
			   "sunview:toolredisplay"),
	    TOOL_MENU_ITEM("Quit", tool_menu_quit, "sunview:toolquit"),
 	    HELP_INFO("sunview:toolmenu")
	    0);
    }    else   {
        menu1 = menu_create(
                 TOOL_MENU_ITEM("Unconstrained", tool_menu_stretch,
                                "sunview:toolucresize"),
                 TOOL_MENU_ITEM("Constrained", tool_menu_cstretch,
                                "sunview:toolcresize"),
                 MENU_ITEM,
                   MENU_STRING, "Zoom",
                   MENU_ACTION, tool_menu_zoom,
                   MENU_GEN_PROC, tool_menu_zoom_state,
                   MENU_CLIENT_DATA, tool,
                   HELP_INFO("sunview:toolzoom")
                   0,
                 MENU_ITEM, 
                   MENU_STRING, "FullScreen", 
                   MENU_ACTION, tool_menu_fullscreen, 
                   MENU_GEN_PROC, tool_menu_fullscreen_state, 
                   MENU_CLIENT_DATA, tool, 
                   HELP_INFO("sunview:toolfullscreen") 
                   0, 
                 HELP_INFO("sunview:toolresize")
                 0);
        tool_menu = menu_create(
	    MENU_ITEM,
	      MENU_STRING, "Open",
	      MENU_ACTION, tool_menu_open,
	      MENU_GEN_PROC, tool_menu_open_state,
	      MENU_CLIENT_DATA, tool,
	      HELP_INFO("sunview:toolopen")
	      0, 
	    MENU_ITEM,
	      MENU_STRING, "Move",
	      MENU_PULLRIGHT, menu_create(
		TOOL_MENU_ITEM("Unconstrained", tool_menu_move,
			       "sunview:toolucmove"), 
		TOOL_MENU_ITEM("Constrained", tool_menu_cmove,
			       "sunview:toolcmove"),
	        HELP_INFO("sunview:toolmove")
		0),
	      HELP_INFO("sunview:toolmove")
	      0,
	    MENU_ITEM,
	      MENU_STRING, "Resize",
	      MENU_PULLRIGHT, menu1,
	      HELP_INFO("sunview:toolresize")
	      0,
	    TOOL_MENU_ITEM("Expose",tool_menu_expose, "sunview:toolfront"),
	    TOOL_MENU_ITEM("Hide", tool_menu_hide, "sunview:toolback"),
	    TOOL_MENU_ITEM("Redisplay", tool_menu_redisplay,
			   "sunview:toolredisplay"),
	    TOOL_MENU_ITEM("Quit", tool_menu_quit, "sunview:toolquit"),
 	    HELP_INFO("sunview:toolmenu")
	    0);
    }

    return tool_menu;
}

/* 
 *  Menu item gen procs
 */
Menu_item
tool_menu_zoom_state(mi, op)
	Menu_item mi;
	Menu_generate op;
{
    Tool *tool;
    
    if (op == MENU_DESTROY) return mi;
    tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    if (tool->tl_flags&TOOL_FULL)
	(void)menu_set(mi, MENU_STRING, "UnZoom",
		       HELP_INFO("sunview:toolunzoom")
		       0);
    else
	(void)menu_set(mi, MENU_STRING, "Zoom",
		       HELP_INFO("sunview:toolzoom")
		       0);

    return mi;
}

Menu_item
tool_menu_open_state(mi, op)
	Menu_item mi;
	Menu_generate op;
{
    Tool *tool;
    
    if (op == MENU_DESTROY) return mi;
    tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    if (wmgr_iswindowopen(tool->tl_windowfd))
	(void)menu_set(mi, MENU_STRING, "Close",
		       HELP_INFO("sunview:toolclose")
		       0);
    else
	(void)menu_set(mi, MENU_STRING, "Open",
		       HELP_INFO("sunview:toolopen")
		       0);

    return mi;
}

Menu_item
tool_menu_fullscreen_state(mi, op)
        Menu_item mi;
        Menu_generate op;
{         
    Tool *tool;
    
    if (op == MENU_DESTROY) return mi;
    tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    if (tool->tl_flags&TOOL_LASTPRIV)
        (void)menu_set(mi,
                       MENU_INACTIVE, TRUE,
                       0);
    else
        (void)menu_set(mi,
                       MENU_INACTIVE, FALSE,
                       0);
    return mi;
}

/*
 *  Callout functions
 */

/*ARGSUSED*/
void    
tool_menu_open(menu, mi)
    Menu	menu;
    Menu_item	mi;
{   
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    int		rootfd = rootfd_for_toolfd(tool->tl_windowfd);
    void	wmgr_close(),wmgr_open();
    
    if (strcmp("Open", menu_get(mi, MENU_STRING)))
	wmgr_close(tool->tl_windowfd, rootfd);
    else
	wmgr_open(tool->tl_windowfd, rootfd);
    (void)close(rootfd);
}

/*ARGSUSED*/
void
tool_menu_move(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    struct	inputevent event;
    
    wmgr_changerect(tool->tl_windowfd, tool->tl_windowfd, &event, TRUE, -1);
}


/*ARGSUSED*/
void
tool_menu_cmove(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    struct	inputevent event;
    
    wmgr_changerect(tool->tl_windowfd, tool->tl_windowfd, &event, TRUE, -2);
}


/*ARGSUSED*/
void
tool_menu_stretch(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    struct	inputevent event;

    wmgr_changerect(tool->tl_windowfd, tool->tl_windowfd, &event, FALSE, -1);
}

/*ARGSUSED*/
void
tool_menu_cstretch(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    struct	inputevent event;

    wmgr_changerect(tool->tl_windowfd, tool->tl_windowfd, &event, FALSE, -2);
}


/*ARGSUSED*/
void
tool_menu_zoom(menu, mi)
    Menu	menu;
    Menu_item	mi;
{   
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    int		rootfd = rootfd_for_toolfd(tool->tl_windowfd);
    void wmgr_full();
             
    wmgr_full(tool, rootfd);
    (void)close(rootfd);
}


/*ARGSUSED*/
void
tool_menu_fullscreen(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    int		rootfd = rootfd_for_toolfd(tool->tl_windowfd);
             
    wmgr_fullscreen(tool, rootfd);
    (void)close(rootfd);
}


/*ARGSUSED*/
void
tool_menu_expose(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    int		rootfd = rootfd_for_toolfd(tool->tl_windowfd);
    
    wmgr_top(tool->tl_windowfd, rootfd);
    (void)close(rootfd);
}


/*ARGSUSED*/
void
tool_menu_hide(menu, mi)
    Menu	menu;
    Menu_item	mi;
{   
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    int		rootfd = rootfd_for_toolfd(tool->tl_windowfd);
    void	wmgr_bottom();
    
    wmgr_bottom(tool->tl_windowfd, rootfd);
    (void)close(rootfd);
}


/*ARGSUSED*/
void
tool_menu_props(menu, mi)
Menu	menu;
Menu_item mi;
{

	Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
	void    frame_handle_props();

	frame_handle_props(tool);
}


/*ARGSUSED*/
void
tool_menu_redisplay(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    void wmgr_refreshwindow();
    
    wmgr_refreshwindow(tool->tl_windowfd);
}


/*ARGSUSED*/
void
tool_menu_quit(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));

   /*
    * Handle "Quit" differently in notifier world.
    * Will confirm quit only if no other entity vetos or confirms.
    */
    if ((tool->tl_flags & TOOL_NOTIFIER)) {
	(void)tool_done(tool);
    } else {
	int returncode, result;
	Event alert_event;

	result = alert_prompt(
		(Frame)0,
		&alert_event,
		ALERT_MESSAGE_STRINGS,
		    "Do you really want to Quit?",
		    0,
		ALERT_BUTTON_YES,	"Confirm",
		ALERT_BUTTON_NO,	"Cancel",
		ALERT_NO_BEEPING,	1,
		0);
	switch (result) {
		case ALERT_YES: 
		    returncode = -1;
		    break;
		case ALERT_NO:
		    returncode = 0;
		    break;
		default: /* alert_failed */
		    returncode = 0;
		    break;
	}
	if (returncode) (void) tool_done_with_no_confirm(tool);
    }
}


void
wmgr_fullscreen(tool, rootfd)

	Tool 	*tool; 
	int	rootfd;
{
	Rect	oldopenrect, fullrect;
	int	iconic = tool_is_iconic(tool);
			
	if (iconic) (void)win_getsavedrect(tool->tl_windowfd, &oldopenrect);
	else (void)win_getrect(tool->tl_windowfd, &oldopenrect);
	(void)win_getrect(rootfd, &fullrect);
	if (rect_equal(&fullrect, &oldopenrect)) return; /* punt */
	(void)win_lockdata(tool->tl_windowfd);
	if (!(tool->tl_flags&TOOL_FULL)) tool->tl_openrect = oldopenrect;
	tool->tl_flags |= TOOL_FULL;		/* turn on zoom */
	tool->tl_flags |= TOOL_LASTPRIV;	/* turn on fullscreen */
	if (iconic) {
	    (void)win_setsavedrect(tool->tl_windowfd, &fullrect);
	    (void)tool_layoutsubwindows(tool);
	    wmgr_open(tool->tl_windowfd,rootfd);
	} else {
	    wmgr_top(tool->tl_windowfd,rootfd);		
	    (void)win_setrect(tool->tl_windowfd, &fullrect);
	    (void)tool_layoutsubwindows(tool);
	}		
	(void)win_unlockdata(tool->tl_windowfd);
}
