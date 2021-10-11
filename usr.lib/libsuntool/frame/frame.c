#ifndef lint
#ifdef sccs
static        char sccsid[] = "@(#)frame.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 *  frame.c: Creates a frame under window_create
 */


/* ------------------------------------------------------------------------- */

#include <stdio.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/file.h>

#include <sunwindow/sun.h>
#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>	/* For WMGR_ICONIC */
#include "sunwindow/sv_malloc.h"

#include <suntool/frame.h>
#include <suntool/tool_struct.h>
#include <suntool/wmgr.h>
#include <suntool/walkmenu.h>
#include <suntool/window.h>

#include "suntool/tool_impl.h"
#include "suntool/frame_impl.h"

char *calloc();
Bool defaults_get_boolean();
void pw_use_fast_monochrome();
Notify_value	tool_death();
/* ------------------------------------------------------------------------- */

/* 
 * Public
 */

caddr_t			frame_window_object();
Notify_value		frame_destroy();
void			frame_default_done_func();

extern struct cursor	 tool_cursor;
extern int		 win_getrect(), win_getsavedrect();
extern int		 win_setrect(), win_setsavedrect();
extern Menu		 tool_walkmenu();


/* 
 * Package private
 */

Pkg_private caddr_t	frame_get();
Pkg_private int		frame_set(), frame_layout();
Pkg_private void	frame_default_done_func();
Pkg_private Frame	shadow_frame();


/* 
 * Private
 */

Private			frame_menu_done();
Private Menu		frame_secondary_walkmenu();
Private Notify_value	frame_events();



/* ------------------------------------------------------------------------- */



caddr_t
frame_window_object(win, avlist)
	Window win;
	Frame_attribute avlist[];
{   
    int	iconic = 0, parent_fd, win_fd,  parentlink, link;
    char parentname[WIN_NAMESIZE];
    struct inputmask im;
    struct rect	 rectnormal, recticon;
    Tool *tool;
    caddr_t owner;
    Menu menu;
    Frame_attribute *attrs;
    extern Notify_value tool_event(), frame_destroy();
    extern int tool_notify_count;	/* Number of notify based tools */
    int		frame_has_shadow;
    int		i;
    int		is_sub_frame, user_flags;
    static char  *pgrp_str;

   /*
    * Get tool structs & open its fd
    */
    tool = (Tool *)(LINT_CAST(calloc(1, sizeof(struct toolplus))));
    if (tool == NULL) return NULL;

    win_fd = window_fd(win);
    tool->tl_windowfd = win_fd;
   /* set default tool flags */
    tool->tl_flags |= TOOL_NOTIFIER | TOOL_BOUNDARYMGR | TOOL_NAMESTRIPE;

    owner = window_get(win, WIN_OWNER);
    frame_has_shadow = (owner != NULL);   /* Default all subframe to TRUE */
    
    
    is_sub_frame = (owner != NULL);
    
    /* Create subframe or frame menu */	
    menu = is_sub_frame ? (Menu)frame_secondary_walkmenu(tool)
	         : (Menu)tool_walkmenu(tool);

    tool->tl_menu = menu;
    if (defaults_get_boolean("/SunView/Embolden_labels",(Bool)FALSE,(int *)NULL))
	tool->tl_flags |= TOOL_EMBOLDEN_LABEL;

    ((struct toolplus *)tool)->default_done_proc = frame_default_done_func;
    ((struct toolplus *)tool)->frame_list.client = (caddr_t)win;
	
   /*
    * Determine parent
    */
    if (we_getparentwindow(parentname)) {
	(void)fprintf(stderr,
		"Frame can't access parent.  Probably not running under suntools\n");
	goto Failure;
    }

   /*
    * Open parent window
    */
    parent_fd = open(parentname, O_RDONLY, 0);
    if (parent_fd < 0) {
	(void)fprintf(stderr, "%s (parent window) would not open\n", parentname);
	goto Failure;
    }

   /*
    * Setup links
    */
    parentlink = win_nametonumber(parentname);
    (void)win_setlink(win_fd, WL_PARENT, parentlink);
    link = win_getlink(parent_fd, WL_TOPCHILD);
    (void)win_setlink(win_fd, WL_OLDERSIB, link);
       

   /*
    * Give frame own cursor
    */
    (void)win_setcursor(win_fd, &tool_cursor);
    
    if (is_sub_frame)  { /* Mark this is window is subframe */
        user_flags = (win_getuserflags(win_fd) | WMGR_SUBFRAME);
        win_setuserflags(win_fd, user_flags);        
    }

   /*
    * Open pixwin
    */
    tool->tl_pixwin=(struct pixwin *)(LINT_CAST(window_get(win, WIN_PIXWIN)));
    if (tool->tl_pixwin == (struct pixwin *)0) goto Failure;
    /* Use fast monochrome plane group (if available) */
    pw_use_fast_monochrome(tool->tl_pixwin);

   /*
    * Set up input mask so can do menu/window management stuff
    */
    (void)input_imnull(&im);
    im.im_flags |= IM_NEGEVENT;
    im.im_flags |= IM_ASCII;

   /*
    * Make sure input mask allows tracking cursor motion and buttons
    * for tool boundary moving.
    * Don't want to always track cursor else would drag in tool's code
    * and data when simply moved over it.
    */
    win_setinputcodebit(&im, LOC_MOVEWHILEBUTDOWN);
    win_setinputcodebit(&im, MS_LEFT);
    win_setinputcodebit(&im, MS_MIDDLE);
    win_setinputcodebit(&im, MS_RIGHT);
    /*
     *	Get all the function keys
     */
    win_keymap_set_smask(win_fd, ACTION_OPEN);
    win_keymap_set_smask(win_fd, ACTION_CLOSE);
    win_keymap_set_smask(win_fd, ACTION_FRONT);
    win_keymap_set_smask(win_fd, ACTION_BACK);
    win_keymap_set_smask(win_fd, ACTION_HELP);
    win_keymap_set_smask(win_fd, DELETE_KEY);
    win_keymap_set_imask_from_std_bind(&im, ACTION_OPEN);
    win_keymap_set_imask_from_std_bind(&im, ACTION_FRONT);
    win_keymap_set_imask_from_std_bind(&im, ACTION_HELP);
    win_keymap_set_imask_from_std_bind(&im, DELETE_KEY);

    for (i = 1; i <= 16; i++) {
    	win_setinputcodebit(&im, KEY_LEFT(i));
    	win_setinputcodebit(&im, KEY_RIGHT(i));
    	win_setinputcodebit(&im, KEY_TOP(i));
    }
    win_setinputcodebit(&im, KBD_REQUEST);
    win_setinputcodebit(&im, LOC_WINENTER);
    win_setinputcodebit(&im, LOC_WINEXIT);
    (void)win_setinputmask(win_fd, &im, (struct inputmask *)0, WIN_NULLLINK);

    if (pgrp_str == 0) {
        pgrp_str = sv_malloc(50);
        sprintf (pgrp_str, "PGRP_PID= %ld", getpgrp (getpid()));
        if (putenv (pgrp_str) != 0)
       		printf ("Unable to add PGRP_PID to env.\n");
       	}

   /*	Get any position / size / state info from environment.
    *	Iconic state could be passed in environment or in command-line;
    *	this ordering gives command-line precedence
    */
    rect_construct(&rectnormal, WMGR_SETPOS, WMGR_SETPOS,
		   WMGR_SETPOS, WMGR_SETPOS);
    recticon = rectnormal;
    (void)win_setrect(win_fd, &rectnormal);
    (void)win_setsavedrect(win_fd, &recticon);

    if (we_getinitdata(&rectnormal, &recticon, &iconic)) {
	struct screen screen;		/*   no initializers	*/
	int rootnumber;

	(void)win_screenget(parent_fd, &screen);
	rootnumber = win_nametonumber(screen.scr_rootname);
	if (parentlink != rootnumber) {  /* ?? tool within a tool -- punt? */
	    rect_construct(&rectnormal, 0, 0, 200, 400);
	    recticon = rectnormal;
	}
    } else {		/*  Got some environment parameters	*/
	(void)we_clearinitdata();
    }

   /*	Iconic state and name stripe have to be determined before any
    *	size parameters are interpreted, so the attribute list is mashed
    *	and explicitly interrogated for them here.
    */
    for (attrs = avlist; *attrs; attrs = frame_attr_next(attrs)) {
	switch (*attrs) {

	  case FRAME_CLOSED:
	    if (attrs[1]) {
	        tool->tl_flags |= TOOL_ICONIC;
	    }   else   {
	    	tool->tl_flags &= ~TOOL_ICONIC;
	    }
	    break;

	  case FRAME_SHOW_LABEL:
	    if (!attrs[1]) tool->tl_flags &= ~TOOL_NAMESTRIPE;
	    break;

	  case FRAME_SHOW_SHADOW:
   	    frame_has_shadow = (int)attrs[1];
	    break;

	  default:
	    break;
	}
    }    

    (void)win_lockdata(parent_fd);

    if (recticon.r_width == WMGR_SETPOS) {
	recticon.r_width = TOOL_ICONWIDTH;
    }
    if (recticon.r_height == WMGR_SETPOS) {
	recticon.r_height = TOOL_ICONHEIGHT;
    }
    (void)wmgr_set_tool_rect(parent_fd, win_fd, &rectnormal);
    (void)wmgr_set_icon_rect(parent_fd, win_fd, &recticon);
    if (tool->tl_flags & TOOL_ICONIC)  {
	int	flags;

	tool->tl_rectcache = recticon;
	(void)win_setrect(win_fd, &recticon);
	(void)win_setsavedrect(win_fd, &rectnormal);
	flags = win_getuserflags(win_fd);
	flags |= WMGR_ICONIC;
	(void)win_setuserflags(win_fd, flags);
    } else {
	tool->tl_rectcache = rectnormal;
    }
    (void)win_unlockdata(parent_fd);
    (void)close(parent_fd);

   /*
    * Cached rect is self relative.
    */
    tool->tl_rectcache.r_left = 0;
    tool->tl_rectcache.r_top = 0;

   /* Register with window manager and set ops vector */
    (void)window_set(win, WIN_SHOW, FALSE, WIN_OBJECT, tool, WIN_TYPE, FRAME_TYPE, 
	       WIN_GET_PROC, frame_get, WIN_SET_PROC, frame_set,
	       WIN_LAYOUT_PROC, frame_layout, 
	       WIN_NOTIFY_EVENT_PROC, tool_event,
	       WIN_NOTIFY_DESTROY_PROC, frame_destroy,
	       WIN_MENU, menu, 
	       WIN_TOP_MARGIN,
	         tool_headerheight(tool->tl_flags & TOOL_NAMESTRIPE),
	       WIN_BOTTOM_MARGIN, 0,
	       WIN_LEFT_MARGIN, TOOL_BORDERWIDTH,
	       WIN_RIGHT_MARGIN, TOOL_BORDERWIDTH,
	       0);

    win = (Window)tool;  	/* HACK: tool is now window handle */

    (void)notify_interpose_event_func((Notify_client)tool,frame_events, NOTIFY_SAFE);
    
    if (owner) {		/* Must be a subframe */
	int y = (int)window_get(owner, WIN_Y);
	
	y = y > 10 ? -10 : -y;
	(void)window_set(win, FRAME_SHOW_LABEL, FALSE, WIN_X, 10, WIN_Y, y, 0);
        

    }
    
    if  (frame_has_shadow) { 
	(Frame) shadow_frame((Frame) win);
    } else {
        ((struct toolplus *)(LINT_CAST(win)))->shadow = NULL;
        ((struct toolplus *)(LINT_CAST(win)))->has_active_shadow = FALSE;
    }

   /* 
    * Set cached version of rect in case changed size from the default
    * before called win_register.
    */
    (void)win_getsize(tool->tl_windowfd, &tool->tl_rectcache);
    tool_notify_count++;

    return (caddr_t)tool;

  Failure:
    free((char *)(LINT_CAST(tool)));
    return NULL;
}


Notify_value
frame_destroy(tool, status)
	struct toolplus *tool;
	Destroy_status status;
{   
    register struct list_node *node = tool->frame_list.next;

    if ((status == DESTROY_CHECKING) || (status == DESTROY_PROCESS_DEATH))
	 tool->tool.tl_flags |= TOOL_DESTROY;
    else if (!(tool->tool.tl_flags & TOOL_DESTROY))
	return NOTIFY_DONE;  /* destroy has been vetoed */

   /* 
    * Destroy the subframes
    */
    for (; node; node = node->next) {
	Window_type win_type = (Window_type)window_get(
		(Window)(LINT_CAST(node->client)), WIN_TYPE);
	Tool *child_tool = (Tool *)(LINT_CAST(node->client));
	
	if (win_type == FRAME_TYPE) {
	    if ((status == DESTROY_CHECKING) ||
		(status == DESTROY_PROCESS_DEATH))
		child_tool->tl_flags |= (TOOL_DESTROY | TOOL_NO_CONFIRM);

	    (void)notify_post_destroy((Notify_client)(LINT_CAST(child_tool)),
	    		status, NOTIFY_IMMEDIATE);

	    if (!(child_tool->tl_flags & TOOL_DESTROY)) {
		(void)notify_veto_destroy((Notify_client)(LINT_CAST(child_tool)));
		return(NOTIFY_IGNORED);
	    }
	} else if (node->tsw && status == DESTROY_CHECKING &&
		   NOTIFY_DESTROY_VETOED ==
		   notify_post_destroy(node->tsw->ts_data, status,
		   	       NOTIFY_IMMEDIATE)) {
	    tool->tool.tl_flags &= ~TOOL_DESTROY;
	    return NOTIFY_DONE;
	} else if (node->tsw && status != DESTROY_CHECKING
		&& status != DESTROY_PROCESS_DEATH) {
	    /* Prepare for tool_death below */
	    /* Note added condition since frame_layout may call win_insert()
	     * which shouldn't be done on a DESTROY_PROCESS_DEATH since the
	     * desktop is being cleaned up and inhibits tree manipulations
	     * (see /usr/src/sys/sunwindowdev/winioctl.c)
	     */
	    (void)frame_layout((Window)(LINT_CAST(tool)), node->client, WIN_INSERT);
	}
    }

   /* 
    * Destroy the subwindows
    */
    (void)tool_death((Tool *)(LINT_CAST(tool)), status);
    
    if (status != DESTROY_CHECKING)  /* tool_death has already cleaned up */
	(void)window_set((Window)(LINT_CAST(tool)), WIN_COMPATIBILITY_INFO, -1, NULL, 0);

    return NOTIFY_DONE;
}

void
frame_default_done_func(frame)
Window frame;
{   
    (void)window_set(frame, WIN_SHOW, FALSE, 0);
}


/*ARGSUSED*/
Private
frame_menu_done(menu, mi)
    Menu	menu;
    Menu_item	mi;
{   
    Frame frame = menu_get(mi, MENU_CLIENT_DATA);
    
    frame_done_proc(frame);
}


Private Menu
frame_secondary_walkmenu(tool)
	Tool *tool;
{
    Menu	tool_menu;
    Menu_item	mi;

    tool_menu = tool_walkmenu(tool);

    /* ?? FIXME:  Use lookup routine when it is ready */

    /* Open */
    mi = (Menu_item)menu_get(tool_menu, MENU_NTH_ITEM, 1);
    (void)menu_set(tool_menu, MENU_REMOVE_ITEM, mi, 0);
    menu_destroy(mi);

    /* Quit */
    mi = (Menu_item)menu_get(tool_menu, MENU_NTH_ITEM,
			     menu_get(tool_menu, MENU_NITEMS));
    (void)menu_set(tool_menu, MENU_REMOVE_ITEM, mi, 0);
    menu_destroy(mi);

    (void)menu_set(tool_menu, MENU_INSERT, 0,
	     menu_create_item(MENU_STRING, "Done",
			      MENU_ACTION, frame_menu_done,
			      MENU_CLIENT_DATA, tool,
			      MENU_RELEASE,
			      0),
	     0);

    return tool_menu;
}


Private Notify_value
frame_events(tool, event, arg, type)
	Tool *tool;
	struct inputevent *event;
	Notify_arg arg;
	Notify_event_type type;
{
    Notify_value v;
    
    if (event_action(event) == ACTION_HELP &&
        ((struct toolplus *)tool)->help_data != NULL) {
	if (event_is_down(event))
	    help_request((Window)(LINT_CAST(tool)),
			 ((struct toolplus *)tool)->help_data,
			   event);
	return NOTIFY_DONE;
    }
    v = notify_next_event_func((Notify_client)(LINT_CAST(tool)), 
      				(Notify_event)(LINT_CAST(event)), arg, type);
    window_event_proc(((Window)(LINT_CAST(tool))), event, arg);
    return v;
}

Pkg_private void
frame_handle_props(tool)
Tool *tool;
{
    if (tool && tool->props_proc && tool->props_active)   {
	tool->props_proc(tool);
    }
}
