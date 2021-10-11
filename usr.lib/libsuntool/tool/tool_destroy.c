#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tool_destroy.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Tool_destroy: Handles tool/sw destruction functions of win_tool.h interface.
 * Separate from tool_create.c because if exiting a process then don't need
 * to do any cleanup.  Thus, can save a little code space in tool by not
 * calling these routines.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <pixrect/pixrect.h>
#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_struct.h>
#include <suntool/tool.h>
#include <suntool/tool_impl.h>

/*
 * Cleanup routines
 */
tool_destroy(tool)
	struct	tool *tool;
{
	Notify_value tool_death();

	tool->tl_flags |= (TOOL_DONE | TOOL_DESTROY);
	(void)tool_death(tool, DESTROY_CLEANUP);
}

tool_destroysubwindow(tool, toolsw)
	struct	tool *tool;
	struct	toolsw *toolsw;
{
    (void)tool_destroysubwindow_inserted(tool, toolsw, 1);
}

tool_destroysubwindow_inserted(tool, toolsw, inserted)
	struct	tool *tool;
	struct	toolsw *toolsw;
{
	int	windowfd;

	if ((tool==0) || (toolsw==0))
		return;
	/*
	 * Remove tool window tree from display clipping tree.
	 */
	if (inserted) {
	    (void)win_remove(toolsw->ts_windowfd);
	    (void)tool_repaint_all_later(tool);
	}	    
	/*
	 * Cleanup subwindow specific stuff
	 */
	if (toolsw->ts_destroy)
		toolsw->ts_destroy(toolsw->ts_data);
	else {
		/* Perhaps client using notifier destroy */
		if (notify_get_destroy_func(toolsw->ts_data) !=
		    NOTIFY_FUNC_NULL)
			(void) notify_post_destroy(
			    (Notify_client)(LINT_CAST(toolsw->ts_data)),
			    DESTROY_CLEANUP, NOTIFY_IMMEDIATE);
	}
	/*
	 * Remove from tool sw list.
	 */
	windowfd = _tool_removesubwindow(tool, toolsw);
	/*
	 * Close window
	 */
	(void)close(windowfd);
	/*
	 * Note: Should  rearrange subwindows to tile window at this point.
	 * Punt that for now.
	 */
	return;
}

/*
 * Utilities
 */
int
_tool_removesubwindow(tool, toolsw)
	struct	tool *tool;
	struct	toolsw *toolsw;
{
	int	windowfd;
	struct	toolsw *sw, **swptr;

	swptr = &(tool->tl_sw);
	/*
	 * Look for toolsw in tool
	 */
	for (sw=tool->tl_sw;sw;sw = sw->ts_next) {
		if (sw==toolsw) {
			/*
			 * Fixup links
			 */
			*swptr = sw->ts_next;
			break;
		}
		swptr = &(sw->ts_next);
	}
	sw = toolsw;
	if (sw) {
		Toolsw_priv *ts_priv = (Toolsw_priv *) (LINT_CAST(sw->ts_priv));

		windowfd = sw->ts_windowfd;
		/* Free notifier entanglements. */
		if (sw->ts_io.tio_selected &&
		    tool->tl_flags & TOOL_NOTIFIER)
			(void) notify_set_input_func((Notify_client)sw,
			    NOTIFY_FUNC_NULL, sw->ts_windowfd);
		/* Free toolsw_priv structure */
		free((char *)(LINT_CAST(ts_priv)));
		/*
		 * Free sw data structure
		 */
		free((char *)(LINT_CAST(sw)));
		return(windowfd);
	}
	return(-1);
}

