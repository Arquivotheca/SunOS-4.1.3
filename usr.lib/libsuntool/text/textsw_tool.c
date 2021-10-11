#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_tool.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Initialization and finalization of text subwindows.
 */

#include <suntool/primal.h>
#include <suntool/textsw_impl.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <suntool/wmgr.h>
#include <fcntl.h>
#include <varargs.h>

#include <sunwindow/win_struct.h>

pkg_private Textsw_view	textsw_init_internal();

pkg_private int
textsw_default_notify(abstract, attrs)
	Textsw		abstract;
	Attr_avlist	attrs;
{
	register Textsw_view	view = VIEW_ABS_TO_REP(abstract);
	register Textsw_folio	textsw = FOLIO_FOR_VIEW(view);

	for (; *attrs; attrs = attr_next(attrs)) {
	    switch ((Textsw_action)(*attrs)) {
	      case TEXTSW_ACTION_TOOL_CLOSE:
	      case TEXTSW_ACTION_TOOL_MGR:
	      case TEXTSW_ACTION_TOOL_DESTROY:
	      case TEXTSW_ACTION_TOOL_QUIT: {
		struct tool	*tool = (struct tool *)
					LINT_CAST(textsw->tool);
		int		 rootfd;
		if ((rootfd = rootfd_for_toolfd(tool->tl_windowfd)) < 0) {
		    return;
		}
		switch ((Textsw_action)(*attrs)) {
		  case TEXTSW_ACTION_TOOL_CLOSE:
		    if (!tool_is_iconic(tool))
			wmgr_close(tool->tl_windowfd, rootfd);
		    break;
		  case TEXTSW_ACTION_TOOL_MGR: {
		    extern Notify_value tool_input();

		    (void) tool_input(tool, (Event *)LINT_CAST(attrs[1]),
			       0, NOTIFY_SAFE);
		    break;
		    }
		  case TEXTSW_ACTION_TOOL_QUIT:
		    if (textsw_has_been_modified(abstract)) {
			int old_fcntl_flags =
			    fcntl(tool->tl_windowfd, F_GETFL, 0);

			Event	*event = (Event *)LINT_CAST(attrs[1]);
			(void) fcntl(tool->tl_windowfd, F_SETFL, FNDELAY);
			(void) alert_prompt(
			       (Frame)window_get(
				   WINDOW_FROM_VIEW(view), WIN_OWNER),
			        &event,
			        ALERT_MESSAGE_STRINGS,
			           "Unable to Quit.  The text has been edited.", 
			           "Please Save Current File or Store as New File",
				   "or Undo All Edits before quiting.",
				   0,
		    	        ALERT_BUTTON_YES,	"Continue",
				ALERT_TRIGGER,		ACTION_STOP,
		    	        0);
			(void) fcntl(tool->tl_windowfd, F_SETFL,
				     old_fcntl_flags);
			break;
		    }
		    tool_done(tool);
		    break;
		  case TEXTSW_ACTION_TOOL_DESTROY:
		    tool_done_with_no_confirm(tool);
		    break;
		}
		(void) close(rootfd);
		break;
	      }
	      default:
		break;
	    }
	}
}

/* VARARGS1 */
extern Textsw
textsw_build(tool, va_alist)
	struct tool		 *tool;
	va_dcl
{
	struct toolsw		 *toolsw;
	caddr_t			  attr_argv[ATTR_STANDARD_SIZE], *attrs;
	char			 *name = "textsw";
	int			  height = TOOL_SWEXTENDTOEDGE,
				  width = TOOL_SWEXTENDTOEDGE;
	Textsw_view		  view = NEW(struct textsw_view_object);
	Textsw_folio		  textsw;
	Textsw_status		  dummy_status;
	Textsw_status		 *status = &dummy_status;
	va_list			  args;

	va_start(args);
	(void) attr_make(attr_argv, ATTR_STANDARD_SIZE, args);
	va_end(args);
	for (attrs = attr_argv; *attrs; attrs = attr_next(attrs)) {
	    switch ((Textsw_attribute)(*attrs)) {
	      case WIN_NAME:
	      case TEXTSW_NAME:
		name = (char *)(attrs[1]);
		break;
	      case WIN_HEIGHT:
	      case TEXTSW_HEIGHT:
		height = (int)(attrs[1]);
		break;
	      case TEXTSW_STATUS:
		status = (Textsw_status *)LINT_CAST(attrs[1]);
		break;
	      case WIN_WIDTH:
	      case TEXTSW_WIDTH:
		width = (int)(attrs[1]);
		break;
	      default:
		break;
	    }
	}
	if (view) {
	    toolsw = tool_createsubwindow(tool, name, width, height);
	    if (toolsw == 0) {
		free((char *)view), view = 0;
	    } else {
		view->magic = TEXTSW_VIEW_MAGIC;
		view->window_fd = toolsw->ts_windowfd;
		view = textsw_init_internal(
			   view, status, textsw_default_notify, attr_argv);
		if (view == 0) {
		    textsw = (Textsw_folio)0;
		    tool_destroysubwindow(tool, toolsw);
		} else {
		    toolsw->ts_data = (caddr_t)view;
		    textsw = FOLIO_FOR_VIEW(view);
		    textsw->tool = (caddr_t)LINT_CAST(tool);
		}
	    }
	} else {
	    *status = TEXTSW_STATUS_CANNOT_ALLOCATE;
	}
	return(VIEW_REP_TO_ABS(view));
}

