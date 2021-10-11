#ifndef lint
static	char sccsid[] = "@(#)search_etc_create.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <suntool/sunview.h>
#include <suntool/panel.h>

Panel_item search_etc_cycle;
Panel_item ntimes, start_frame, stop_frame, speed, go_button;
extern Panel search_etc_panel;

void
search_etc_create()
{
	extern void search_etc_proc();

	start_frame = panel_create_item(search_etc_panel, PANEL_TEXT,
			PANEL_LABEL_STRING, "Start : ",
			0);

	stop_frame = panel_create_item(search_etc_panel, PANEL_TEXT,
			PANEL_LABEL_STRING, "Stop : ",
			0);

	ntimes = panel_create_item(search_etc_panel, PANEL_TEXT,
			PANEL_LABEL_STRING, "# of times : ",
			0);

	speed = panel_create_item(search_etc_panel, PANEL_TEXT,
			PANEL_LABEL_STRING, "Step speed : ",
			0);


	search_etc_cycle = panel_create_item(search_etc_panel, PANEL_CYCLE,
			PANEL_LABEL_STRING, "Options",
			PANEL_CHOICE_STRINGS, 
			"Search", "Repeat", "Replay", "Var Speed",
			"Set Mark Frame", 0,
			PANEL_ITEM_Y, ATTR_ROW(5),
			0);

	go_button = panel_create_item(search_etc_panel, PANEL_BUTTON,
			PANEL_LABEL_IMAGE, 
				panel_button_image(search_etc_panel,"Go",0,0),
			PANEL_NOTIFY_PROC, search_etc_proc,
			PANEL_ITEM_X, ATTR_COL(10),
			PANEL_ITEM_Y, ATTR_ROW(7),
			0);
}
