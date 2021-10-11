#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tool_install.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Tool_install: Handles 2.0 vs 3.0 notifier vs toolio stuff compatibility.
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

int
tool_install(tool)
	Tool *tool;
{
	struct toolsw *sw;
	Notify_value tool_sw_input();

	for (sw = tool->tl_sw;sw;sw = sw->ts_next) {
		/*
		 * For compatibility with old toolio stuff, setup a condition
		 * with the notifier that will let a old style notification
		 * selected function know that input is pending.  This only
		 * supports subwindows that are interested in input and
		 * nothing else.
		 */
		if (sw->ts_io.tio_selected && tool->tl_flags & TOOL_NOTIFIER) {
			if (notify_get_input_func((Notify_client)(LINT_CAST(sw)),
				sw->ts_windowfd) ==  NOTIFY_FUNC_NULL)
				(void) notify_set_input_func((Notify_client)sw, 
				    tool_sw_input, sw->ts_windowfd);
		}
	}
	(void)tool_layoutsubwindows(tool);
	(void)win_insert(tool->tl_windowfd);
	/* Note: should return success or failure */
	return(0);
}

int
tool_remove(tool)
	Tool *tool;
{
	(void)win_remove(tool->tl_windowfd);
	return (0);
}

Notify_value
tool_sw_input(sw, fd)
	struct toolsw *sw;
	int fd;
{
	struct toolio *tio = &sw->ts_io;

	FD_ZERO(&tio->tio_inputmask);
	FD_SET(fd, &tio->tio_inputmask);
	tio->tio_selected(sw->ts_data, &tio->tio_inputmask,
	    &tio->tio_outputmask, &tio->tio_exceptmask, &tio->tio_timer);
	return(NOTIFY_DONE);
}


struct	toolsw *
tool_sw_from_client(tool, client)
	Tool *tool;
	Notify_client client;
{
	struct toolsw *sw;

	for (sw = tool->tl_sw;sw;sw = sw->ts_next)
		if (sw->ts_data == client)
			return(sw);
	return(TOOLSW_NULL);
}

/*
 * Set flag indicating that should return from tool_select (non-notify) or
 * instigate safe tool destroy (notify).
 */
tool_done_with_no_confirm(tool)
	struct	tool *tool;
{
	tool->tl_flags |= TOOL_NO_CONFIRM;
	(void)tool_done(tool);
}

tool_done(tool)
	struct	tool *tool;
{
	if (tool->tl_flags & TOOL_NOTIFIER) {
		tool->tl_flags |= TOOL_DESTROY;
		/*
		 * Posting DESTROY_CLEANUP after DESTROY_CHECKING when
		 * NOTIFY_SAFE will abort if DESTROY_CLEANUP if vetoed.
		 */
		(void)notify_post_destroy((Notify_client)(LINT_CAST(tool)),
			DESTROY_CHECKING, NOTIFY_SAFE);
		(void)notify_post_destroy((Notify_client)(LINT_CAST(tool)),
			DESTROY_CLEANUP, NOTIFY_SAFE);
	} else
		tool->tl_flags |= TOOL_DONE;
}

void
tool_veto_destroy(tool)
	Tool *tool;
{
	tool->tl_flags &= ~TOOL_DESTROY;
	(void)notify_veto_destroy((Notify_client)(LINT_CAST(tool)));
}

void
tool_confirm_destroy(tool)
	Tool *tool;
{
	/* Subwindow did confirmation so tool shouldn't */
	tool->tl_flags |= TOOL_NO_CONFIRM;
}

