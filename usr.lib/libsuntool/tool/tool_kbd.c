#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tool_kbd.c 1.1 92/07/30 SMI";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Tool_create: Tool/sw creation code.
 */
#include <stdio.h>
#include <sys/file.h>
#include <suntool/tool_hs.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/cms_mono.h> 
#include <suntool/wmgr.h>
#include <suntool/tool_impl.h>

/*
 *  find textsw corresponding to client.
 */
extern	struct	toolsw *
tool_find_sw_with_client(tool, client)
	struct tool	*tool;
	caddr_t		client;
{
	struct toolsw	*sw;

	for (sw = tool->tl_sw;sw;sw = sw->ts_next) {
		if (sw->ts_data == client) {
			break;
		}
	}
	return(sw);
}

/*
 *  highlight subwindow border for client.
 */
tool_kbd_use(tool, client)
	struct	tool *tool;
	caddr_t	client;
{
	struct	toolsw	*sw;
	Toolsw_priv	*sw_priv;
	
	sw = tool_find_sw_with_client(tool, client);
	if (!sw)
		return;
	sw_priv = (Toolsw_priv *)(LINT_CAST(sw->ts_priv));
	sw_priv->have_kbd_focus = TRUE;
	(void)_tool_displayswborders(tool, sw);
}

/*
 *  unhighlight subwindow border for client.
 */
tool_kbd_done(tool, client)
	struct	tool *tool;
	caddr_t	client;
{
	struct	toolsw	*sw;
	Toolsw_priv	*sw_priv;
	
	sw = tool_find_sw_with_client(tool, client);
	if (!sw)
		return;
	sw_priv = (Toolsw_priv *)(LINT_CAST(sw->ts_priv));
	sw_priv->have_kbd_focus = FALSE;
	(void)_tool_displayswborders(tool, sw);
	
	for (sw = tool->tl_sw;sw;sw = sw->ts_next) {
		sw_priv = (Toolsw_priv *)(LINT_CAST(sw->ts_priv));
		if (sw_priv->have_kbd_focus == TRUE) {
			(void)_tool_displayswborders(tool, sw);
			break;
		}
	}
}
