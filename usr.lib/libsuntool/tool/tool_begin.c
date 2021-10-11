#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tool_begin.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 *  tool_begin.c - Make tool using attribute list and notifier.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <stdio.h>
#include <signal.h>
#include <varargs.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/defaults.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/win_notify.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <suntool/icon.h>
#include <suntool/walkmenu.h>
#include <suntool/tool.h>
#include <suntool/tool_impl.h>
#include <suntool/wmgr.h>

extern void	tool_remove_tool_from_count();

int	tool_notify_count;	/* Number of notify based tools */

/* VARARGS */
struct	tool *
tool_begin(va_alist)
	va_dcl
{
	va_list valist;
	caddr_t avlist[ATTR_STANDARD_SIZE];
	struct tool *tool;
	Notify_value tool_event(), tool_death();

	va_start(valist);
	(void) attr_make(avlist, ATTR_STANDARD_SIZE, valist);
	va_end(valist);

	if ((tool = tool_make(WIN_ATTR_LIST, avlist, 0)) == NULL)
	    return(tool);

	/* Get out of toolio world */
	tool->tl_flags |= TOOL_NOTIFIER;
	tool->tl_io.tio_handlesigwinch = (int (*)())0;
	tool->tl_io.tio_selected = (int (*)())0;
	/* Register with window manager */
	if (win_register((Notify_client)(LINT_CAST(tool)), tool->tl_pixwin, 
			tool_event, tool_death, 0))
		return(TOOL_NULL);
	/*
	 * Set cached version of rect in case changed size from the default
	 * before called win_register.
	 */
	(void)win_getsize(tool->tl_windowfd, &tool->tl_rectcache);
	if (tool->tl_flags & TOOL_NOTIFIER)
		++tool_notify_count;
	return(tool);
}

Notify_value
tool_event(tool, event, arg, type)
	Tool *tool;
	Event *event;
	Notify_arg arg;
	Notify_event_type type;
{
	switch (event_id(event)) {
	case WIN_REPAINT:
		/*
		 * Turnoff notifier "display all" flag set when rearrange
		 * subwindows.
		 */
		(void)win_set_flags((char *)tool, 
			(unsigned)(win_get_flags((char *)tool) & (~PW_REPAINT_ALL)));
		/*
		 * Tool iconic state transition will only be caught here
		 * if the normal and iconic sizes are the same.
		 */
		(void) tool_check_state(tool);
		/*
		 * _tool_display (vs tool_display) doesn't defeat
		 * TOOL_REPAINT_LOCK.
		 */
		(void)_tool_display(tool, 1);
		return(NOTIFY_DONE);
	case WIN_RESIZE:
		/* Tool iconic state transition is caught here */
		(void) tool_check_state(tool);
		(void)tool_resize(tool);
		return(NOTIFY_DONE);
	default:
		return(tool_input(tool, event, arg, type));
	}
}

tool_resize(tool)
	Tool *tool;
{
	(void)win_getsize(tool->tl_windowfd, &tool->tl_rectcache);
	(void)tool_layoutsubwindows(tool);
}

int
tool_check_state(tool)
	Tool *tool;
{
	int	flags = win_getuserflags(tool->tl_windowfd);

	if ((flags&WMGR_ICONIC) && (~tool->tl_flags&TOOL_ICONIC)) {
		/*
		 * Tool has just gone iconic, so, add y offset to sws
		 * to move them out of the picture.
		 */
		(void)_tool_addyoffsettosws(tool, 2048);
		tool->tl_flags |= TOOL_ICONIC;
		return 1;
	} else if ((~flags&WMGR_ICONIC) && (tool->tl_flags&TOOL_ICONIC)) {
		/*
		 * Tool has just gone from iconic to normal, so, subtract
		 * y offset from sws to move them into the picture again.
		 */
		tool->tl_flags &= ~TOOL_ICONIC;
		(void)_tool_addyoffsettosws(tool, -2048);
		return 1;
	}
	return 0;
}

Notify_value
tool_death(tool, status)
	Tool *tool;
	Destroy_status status;
{
	void	tool_veto_destroy();
	int	result, wmgr_result, quit_confirmed;
	Event	event;
	
    /* Do checking */
     if (status == DESTROY_CHECKING) {
	struct	toolsw *sw;

	/* Check with sw''s */
	for (sw = tool->tl_sw;sw;sw = sw->ts_next) {
        /*
	 * If checking was vetoed then don''t check further.
	 * Could be vetoed by interposing in front of tool_death
	 */
	    if (!(tool->tl_flags & TOOL_DESTROY)) break;
	    if (notify_get_destroy_func(sw->ts_data) != NOTIFY_FUNC_NULL &&
		NOTIFY_DESTROY_VETOED == 
		notify_post_destroy(sw->ts_data, status, NOTIFY_IMMEDIATE))
	    {   
		tool->tl_flags &= ~TOOL_DESTROY;
	    }
	}
	/*
	 * Do standard confirmation if still planning on destruction,
	 * and no one else did any kind of confirm.
	 */
	 if ((tool->tl_flags & TOOL_DESTROY) &&
	     !(tool->tl_flags & TOOL_NO_CONFIRM)) {
		result = alert_prompt(
		    (Frame)0,
		    &event,
		    ALERT_MESSAGE_STRINGS,
			"Are you sure you want to Quit?",
			0,
		    ALERT_BUTTON_YES,	"Confirm",
		    ALERT_BUTTON_NO,	"Cancel",
		    ALERT_NO_BEEPING,	1,
		    ALERT_OPTIONAL,	1,
		    ALERT_POSITION,	ALERT_SCREEN_CENTERED,
		    0);
		if (result == ALERT_FAILED) {
		    quit_confirmed = 0;
		} else if (result == ALERT_YES) {
		    quit_confirmed = 1;
		} else {
		    quit_confirmed = 0;
		}
		if (!quit_confirmed) tool_veto_destroy(tool);
	 }
	 /* Reset no confirmation flag for next time */
	 tool->tl_flags &= ~TOOL_NO_CONFIRM;
	 return(NOTIFY_DONE);
     }
     /*
      * Do process death stuff.  Notifier will tell subwindows about
      * process death.
      */
     /* Window will be removed from screen by kernel */
     if (status == DESTROY_PROCESS_DEATH) return(NOTIFY_DONE);
     /* See if checking was vetoed */
     if (!(tool->tl_flags & TOOL_DESTROY)) return(NOTIFY_IGNORED);
     /*
      * Remove tool window tree from display clipping tree.
      */
     (void)win_remove(tool->tl_windowfd);
     /*
      * Destroy all subwindows
      */
     while  (tool->tl_sw) (void)tool_destroysubwindow(tool, tool->tl_sw);
     /*
      * Close pf_sys
      */
     (void)pw_pfsysclose();
     /*
      * Close tool pixwin
      */
     (void)pw_close(tool->tl_pixwin);
     /*
      * Free other dynamic storage
      */
     if (tool->tl_flags & TOOL_DYNAMIC_STORAGE) {
	 (void)tool_free_attr((int)(LINT_CAST(WIN_LABEL)), tool->tl_name);
	 (void)tool_free_attr((int)(LINT_CAST(WIN_ICON)), 
	 		(char *)(LINT_CAST(tool->tl_icon)));
     }
     /* Remove from lower level control */
     if (tool->tl_flags & TOOL_NOTIFIER) 
     	(void)win_unregister((Notify_client)(LINT_CAST(tool)));

     /*
      * Close tool window
      */
     (void)close(tool->tl_windowfd);

     if (tool->tl_menu)
         menu_destroy(tool->tl_menu);
         
     /*
      * Conditionally stop the notifier.  This is a special case so that
      * single tool clients don''t have to interpose in from of tool_death
      * in order to notify_remove(...other clients...) so that notify_start 
      * will return.
      */
     if (tool->tl_flags & TOOL_NOTIFIER) {
	  if (--tool_notify_count == 0)
	      (void) notify_stop();
     }
     /*
      * Free tool window data
      */
     free((char *)(LINT_CAST(tool)));
     return(NOTIFY_DONE);
}

/* The following routine (tool_remove_tool_from_count) will bump the
 * active tool count in order to allow the process to finish if the
 * tool wasn't going to go away otherwise.  This 'hack' was needed
 * by the alert library which was caching a base frame and wan't able
 * to make it go away a process destruction time.
 */

extern void
tool_remove_tool_from_count()
{
    if (tool_notify_count > 0) tool_notify_count--;
}

