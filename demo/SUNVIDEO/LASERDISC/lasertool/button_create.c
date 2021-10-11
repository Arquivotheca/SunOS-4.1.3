#ifndef lint
static	char sccsid[] = "@(#)button_create.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <suntool/sunview.h>
#include <suntool/panel.h>

Panel_item Clear_all, Cont, Clear_entry, Memory, Laser_menu, Non_cf_play;
Panel_item Still, Stop, Quit;

button_create()
{
	extern void button_proc();
	extern Panel button_panel;

	Laser_menu = panel_create_item(button_panel, PANEL_BUTTON,
			PANEL_LABEL_IMAGE, panel_button_image(button_panel,"menu",0,0),
			PANEL_NOTIFY_PROC, button_proc,
			PANEL_ITEM_X, ATTR_COL(0),
			PANEL_ITEM_Y, ATTR_ROW(0),
			0);

	Non_cf_play = panel_create_item(button_panel, PANEL_BUTTON,
			PANEL_LABEL_IMAGE, 
				panel_button_image(button_panel,"non cf play",0,0),
			PANEL_NOTIFY_PROC, button_proc,
			PANEL_ITEM_X, ATTR_COL(11),
			PANEL_ITEM_Y, ATTR_ROW(0),
			0);

	Still = panel_create_item(button_panel, PANEL_BUTTON,
			PANEL_LABEL_IMAGE, panel_button_image(button_panel,"still",0,0),
			PANEL_NOTIFY_PROC, button_proc,
			PANEL_ITEM_X, ATTR_COL(0),
			PANEL_ITEM_Y, ATTR_ROW(2),
			0);

	Stop = panel_create_item(button_panel, PANEL_BUTTON,
			PANEL_LABEL_IMAGE, panel_button_image(button_panel,"stop",0,0),
			PANEL_NOTIFY_PROC, button_proc,
			PANEL_ITEM_X, ATTR_COL(11),
			PANEL_ITEM_Y, ATTR_ROW(2),
			0);

	Cont = panel_create_item(button_panel, PANEL_BUTTON,
			PANEL_LABEL_IMAGE, 
				panel_button_image(button_panel,"continue",0,0),
			PANEL_NOTIFY_PROC, button_proc,
			PANEL_ITEM_X, ATTR_COL(0),
			PANEL_ITEM_Y, ATTR_ROW(4),
			0);
	Clear_entry = panel_create_item(button_panel, PANEL_BUTTON,
			PANEL_LABEL_IMAGE, 
				panel_button_image(button_panel,"clear entry",0,0),
			PANEL_NOTIFY_PROC, button_proc,
			PANEL_ITEM_X, ATTR_COL(11),
			PANEL_ITEM_Y, ATTR_ROW(4),
			0);

	Clear_all = panel_create_item(button_panel, PANEL_BUTTON,
			PANEL_LABEL_IMAGE, panel_button_image(button_panel,"clear all",0,0),
			PANEL_NOTIFY_PROC, button_proc,
			PANEL_ITEM_X, ATTR_COL(0),
			PANEL_ITEM_Y, ATTR_ROW(6),
			0);

	Quit = panel_create_item(button_panel, PANEL_BUTTON,
			PANEL_LABEL_IMAGE, panel_button_image(button_panel,"quit",0,0),
			PANEL_NOTIFY_PROC, button_proc,
			PANEL_ITEM_X, ATTR_COL(11),
			PANEL_ITEM_Y, ATTR_ROW(6),
			0);
}
