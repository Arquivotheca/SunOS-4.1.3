#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)tool_input.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Sets up tool interactive window mgt functions.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sundev/kbd.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/defaults.h>
#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/cms_mono.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_input.h>
#include <suntool/alert.h>
#include <suntool/icon.h>
#include <suntool/frame.h>
#include <suntool/walkmenu.h>
#include <suntool/tool.h>
#include <suntool/tool_impl.h>
#include <suntool/wmgr.h>
#include <suntool/menu.h>

extern	struct pixfont *pf_sys;

/*ARGSUSED*/
tool_selectedstd(tool, ibits, obits, ebits, timer)
	struct	tool *tool;
	int	*ibits, *obits, *ebits;
	struct	timeval **timer;
{
	struct	inputevent event;

	*obits = *ebits = 0;
	if (!(*ibits & (1<<tool->tl_windowfd))) {
		*ibits = 0;
		return;
	}
	*ibits = 0;
	/*
	 * Read should not block.
	 */
	if (input_readevent(tool->tl_windowfd, &event)==-1) {
		perror("tool->tl_name");
		return;
	}
	(void) tool_input(tool, &event, (Notify_arg)0, NOTIFY_SAFE);
}

int
tool_is_exposed(toolfd)
	int     toolfd;
{
	int     sibling_fd, sibling_number, next_number;
	char    name[WIN_NAMESIZE];
	struct	rect	sibling_rect, tool_rect;

	sibling_number = win_getlink(toolfd, WL_COVERING);
/*
	if (sibling_number==WIN_NULLLINK)
		return (TRUE);
*/
	(void)win_getrect(toolfd, &tool_rect);
	while (sibling_number != WIN_NULLLINK) {
		(void)win_numbertoname(sibling_number, name);
		sibling_fd = open(name, O_RDONLY, 0);
		(void)win_getrect(sibling_fd, &sibling_rect);
		next_number = win_getlink(sibling_fd, WL_COVERING);
		(void)close(sibling_fd);
		
		if (rect_intersectsrect(&tool_rect, &sibling_rect))
			break;
		
		sibling_number = next_number;
	}
	
	return(sibling_number == WIN_NULLLINK);
}

int
rootfd_for_toolfd(toolfd)
      int     toolfd;
{
	int     rootnumber;
	char    name[WIN_NAMESIZE];

	rootnumber = win_getlink(toolfd, WL_PARENT);
	(void)win_numbertoname(rootnumber, name);
	return(open(name, O_RDONLY, 0));
}

/*ARGSUSED*/
Notify_value
tool_input(tool, event, arg, type)
	Tool *tool;
	Event *event;
	Notify_arg arg;
	Notify_event_type type;
{
	int	rootfd;
	struct	menuitem *mi = (struct menuitem *) 0;
	extern	struct menu *wmgr_toolmenu;
	extern	struct menuitem *menu_display();
	extern void wmgr_changerect();
	extern void frame_handle_props();
	unsigned int action = event_action(event);

	/* Tell the selection service about GET, PUT, FIND, DELETE */

	switch (action) {
		case ACTION_COPY:
		case ACTION_PASTE:
		case ACTION_CUT:
		case ACTION_FIND_FORWARD:
		case ACTION_FIND_BACKWARD:
			seln_report_event(tool, event);
			break;
		default:
			break;
	}
	/*
	 * Only want positive events (except for function keys)
	 */
	if (event_is_down(event)) {
		switch (action) {
		case OPEN_KEY:	/* Fall through */
		case UN_OPEN_KEY:
		case TOP_KEY:
		case UN_TOP_KEY:
		case DELETE_KEY:
		case PROPS_KEY:
			return(NOTIFY_IGNORED);
		default:
			break;
		}
	} else {
		switch (action) {
		case OPEN_KEY:	/* Fall through */
		case UN_OPEN_KEY:
		case TOP_KEY:
		case UN_TOP_KEY:
		case PROPS_KEY:
#ifdef DELETE_ACCEL
		case DELETE_KEY:
#endif
			break;
		default:
			return(NOTIFY_IGNORED);
		}
	}
	/*
	 * Get root window handle
	 */
	if ((rootfd = rootfd_for_toolfd(tool->tl_windowfd)) < 0) {
		(void)printf("Can't find root window\n");
		perror("tool->tl_name");
		return(NOTIFY_UNEXPECTED);
	}
	switch (action) {
	  case MENU_BUT: /* Do menus */
	    if (tool->tl_menu) {
		(void)menu_show_using_fd(tool->tl_menu, tool->tl_windowfd, event);
	    }
	    break;
	case SELECT_BUT:
		if (event_ctrl_is_down(event))  	/* Full */	
			wmgr_full(tool, rootfd);
		else if (event_shift_is_down(event)) 	/* Hide */
			wmgr_bottom(tool->tl_windowfd, rootfd);
		else if (tool_is_iconic(tool)) 			/* Open */
			wmgr_open(tool->tl_windowfd, rootfd);
		else					/* Expose */
			wmgr_top(tool->tl_windowfd, rootfd);
		break;
	case MS_MIDDLE:
		if (!tool_is_iconic(tool) && 
		    (tool_moveboundary(tool, event) != -1)) {
			/*
			 * Moved boundary while in boundary stripe
			 */
		 } else {
			/*
			 * Do move/stretch operation without prompt
			 * If the ctrl key is down, do an accelerated
			 * stretch.
			 */
			(void)wmgr_changerect(tool->tl_windowfd, tool->tl_windowfd,
			    event, !event_ctrl_is_down(event), TRUE);
			goto Done;
		}
		break;
	case KBD_REQUEST:
		/*
		 * Tool window always refuses kbd focus request
		 * unless shift down
		 */
		if (!event_shift_is_down(event)) {
			(void)win_refuse_kbd_focus(tool->tl_windowfd);
		}
		break;

#ifdef	KEYACCELS
	case 'c': /* Close */
	case 'C':
	case 'i': /* Iconic */
	case 'I':
		if (!tool_is_iconic(tool)) /* Close */
			wmgr_close(tool->tl_windowfd, rootfd);
		break;
	case 'o': /* Open */
	case 'O':
	case 'n': /* Normal */
	case 'N':
		if (tool_is_iconic(tool)) /* Open */
			wmgr_open(tool->tl_windowfd, rootfd);
		break;
	case 'm': /* Move */
	case 'M':
		wmgr_move(tool->tl_windowfd);
		break;
	case 's': /* Stretch */
	case 'S':
		if (!tool_is_iconic(tool)) /* Stretch */
			wmgr_stretch(tool->tl_windowfd);
		break;
	case 'e': /* Expose */
	case 'E':
	case 't': /* Top */
	case 'T':
		wmgr_top(tool->tl_windowfd, rootfd);
		break;
	case 'h': /* Hide */
	case 'H':
	case 'b': /* Bottom */
	case 'B':
		wmgr_bottom(tool->tl_windowfd, rootfd);
		break;
	case 'r': /* Redisplay */
	case 'R':
	case 'd': /* Display */
	case 'D':
	case '\014': /* Cntl L */
		wmgr_refreshwindow(tool->tl_windowfd);
		break;
	case '\004':	/*  Quit on ^D	*/
		(void)tool_done(tool);
		break;
#endif	KEYACCELS
	case '?': { /*  HELP	*/
		Event	event;

		/*
		 * Display a prompt
		 */
		(void) alert_prompt(
			(Frame)0,
			&event,
			ALERT_MESSAGE_STRINGS,
			    "  Clicking the left mouse button on the frame exposes tool,",
			    "the middle mouse button moves the tool or adjusts",
			    "a boundary and the right mouse button displays a menu.",
			    "  When the tool is closed, clicking the left mouse",
			    "button on its icon opens it.",
			    0,
			ALERT_BUTTON_YES,	"Continue",
			ALERT_TRIGGER,		ACTION_STOP,
			0);
		break;
		}

	/* Note semantics: OPEN_KEY is a toggle, and UN_OPEN_KEY forces closed */
	case OPEN_KEY:
		if (event_ctrl_is_down(event)) {
		    /* Handle zoom feature */
		    wmgr_full(tool, rootfd);
		    break;
		}
		if (tool->tl_menu) {
		    /* Use menu to handle both frame and subframe cases */
		    Menu_item m_item; caddr_t action;
		    m_item = menu_find(tool->tl_menu, MENU_STRING, "Close", 0);
		    if (!m_item)
			m_item = menu_find(tool->tl_menu, MENU_STRING, "Done", 0);
		    if (m_item && (action = menu_get(m_item, MENU_ACTION))) {   
			((void (*)())(LINT_CAST(action)))(tool->tl_menu, m_item);
			break;
		    }
		}
		/* Do it the old way */
		if (!tool_is_iconic(tool)) {
		    wmgr_close(tool->tl_windowfd, rootfd);
		} else {
		    wmgr_open(tool->tl_windowfd, rootfd);
		}
		break;
	case UN_OPEN_KEY:
		if (tool->tl_menu) {
		    /* Use menu to handle both frame and subframe cases */
		    Menu_item m_item; caddr_t action;
		    m_item = menu_find(tool->tl_menu, MENU_STRING, "Close", 0);
		    if (!m_item)
			m_item = menu_find(tool->tl_menu, MENU_STRING, "Done", 0);
		    if (m_item && (action = menu_get(m_item, MENU_ACTION))) {   
			((void (*)())(LINT_CAST(action)))(tool->tl_menu, m_item);
			break;
		    }
		}
		/* Do it the old way */
		if (!tool_is_iconic(tool))
		    wmgr_close(tool->tl_windowfd, rootfd);

		break;

	/* Note semantics: TOP_KEY is a toggle and UN_TOP_KEY forces bottom */
	case TOP_KEY:
		if (tool_is_exposed(tool->tl_windowfd))
			wmgr_bottom(tool->tl_windowfd, rootfd);
		else
			wmgr_top(tool->tl_windowfd, rootfd);
		break;
	case UN_TOP_KEY:
		wmgr_bottom(tool->tl_windowfd, rootfd);
		
		break;
	case PROPS_KEY:
#ifdef GPROF
		if (event_ctrl_is_down(event)) {
			moncontrol(0);  /* stop collecting  */
		} else {
			moncontrol(1);  /* start collecting */
		}
#endif
		frame_handle_props(tool);
		break;
#ifdef DELETE_ACCEL
	case DELETE_KEY:
		/* ctrl-delete is an accelerator for
		 * quit.
		 */
		if (event_ctrl_is_down(event))
			(void)tool_done_with_no_confirm(tool);
		else
			(void)tool_done(tool);
		break;
#endif

	default: {}
	}
Done:
	/*
	 * Done with rootfd
	 */
	(void)close(rootfd);
	return(NOTIFY_DONE);
}

