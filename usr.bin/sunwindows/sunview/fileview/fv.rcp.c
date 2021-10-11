#ifndef lint
static  char sccsid[] = "@(#)fv.rcp.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */

#include <stdio.h>
#include <errno.h>

#ifdef SV1
# include <suntool/sunview.h>
# include <suntool/panel.h>
# include <suntool/canvas.h>
# include <suntool/scrollbar.h>
#else
# include <view2/view2.h>
# include <view2/panel.h>
# include <view2/canvas.h>
# include <view2/scrollbar.h>
#endif

#include "fv.port.h"
#include "fv.h"
#include "fv.extern.h"

static Frame Popup_frame;		/* Command popup window */
static Panel Popup_panel;		/* It's panel */
static Panel_item Source;		/* Text field */
static Panel_item Destination;		/* Destination text field */
static Panel_item Copy_button;
#ifndef SV1
# define VALUE_COL	8
#else
# define VALUE_COL	14
#endif

static void
proceed_button()
{
	char buffer[BUFSIZ];
	char *source;

	if (source = (char *)panel_get_value(Source))
	{
		(void)sprintf(buffer, "/bin/rcp %s %s 2>/tmp/.err", source,
			(char *)panel_get_value(Destination));
		if (system(buffer))
			fv_showerr();
		else
		{
			/* Take popup down if not pinned */
#ifdef OPENLOOK
			if (!window_get(Popup_frame, FRAME_CMD_PUSHPIN_IN))
#endif
				done_copy_popup();
		}
	}
}


void
fv_remote_transfer()
{
	static void panel_refont();
	static done_copy_popup();
	char buffer[MAXPATH];
	register FV_FILE **f_p, **l_p;	/* File pointers */

	if (Popup_frame)
	{
		window_set(Popup_frame, WIN_SHOW, TRUE, 0);
		return;
	}

	if ((Popup_frame = window_create(Fv_frame, 
#ifdef SV1
		FRAME,
		FRAME_LABEL, "fileview: Remote Transfer",
# ifdef PROTO
		FRAME_SHOW_FOOTER, FALSE,
		FRAME_CLASS, FRAME_CLASS_COMMAND,
		FRAME_ADJUSTABLE, FALSE,
# endif
#else
		FRAME_CMD,
		FRAME_LABEL, " Remote Transfer",
#endif
		FRAME_SHOW_LABEL, TRUE,
		FRAME_DONE_PROC, done_copy_popup,
		FRAME_NO_CONFIRM, TRUE, 0)) == NULL ||
#ifdef SV1
	    (Popup_panel = window_create(Popup_frame, PANEL, 
#else
	    (Popup_panel = vu_get(Popup_frame, FRAME_CMD_PANEL,
#endif
			0)) == NULL)
	{
		fv_putmsg(TRUE, Fv_message[MEWIN], 0, 0);
		return;
	}


	buffer[0] = NULL;
	fv_collect_selected(buffer, FALSE);

	Source = panel_create_item(Popup_panel, PANEL_TEXT,
		PANEL_LABEL_STRING, "Source:",
		PANEL_LABEL_BOLD, TRUE,
		PANEL_VALUE, buffer,
		PANEL_VALUE_DISPLAY_LENGTH, 13,
		PANEL_VALUE_STORED_LENGTH, 255,
		PANEL_ITEM_Y, ATTR_ROW(0),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);
	Destination = panel_create_item(Popup_panel, PANEL_TEXT,
		PANEL_LABEL_STRING, "Destination:",
		PANEL_LABEL_BOLD, TRUE,
		PANEL_VALUE_DISPLAY_LENGTH, 13,
		PANEL_ITEM_Y, ATTR_ROW(1),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);
	Copy_button = panel_create_item(Popup_panel, PANEL_BUTTON,
		MY_BUTTON_IMAGE(Popup_panel, "Transfer"),
		PANEL_NOTIFY_PROC, proceed_button,
		PANEL_ITEM_Y, ATTR_ROW(2),
		PANEL_ITEM_X, ATTR_COL(VALUE_COL-3),
		0);

	window_fit(Popup_panel);
	window_fit(Popup_frame);
#ifdef PROTO
	frame_set_font_size(Popup_frame, 
		(int)window_get(Fv_frame, WIN_FONT_SIZE));
#endif
	fv_winright(Popup_frame);
	window_set(Popup_frame, WIN_SHOW, TRUE, 0);
}


static
done_copy_popup()
{
	window_destroy(Popup_frame),
	Popup_frame = NULL;
}
