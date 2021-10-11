#ifndef lint
static	char sccsid[] = "@(#)info_create.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <suntool/sunview.h>
#include <suntool/panel.h>

/*
 * Info for display
 */
Panel_item adrezz, chpter, dsk_type;

info_create()
{
	extern Panel info_panel;
	Panel_item adrezzs, chpters, dsk_types;

	adrezzs = panel_create_item(info_panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING, "Track     : ",
			PANEL_ITEM_X, ATTR_COL(1),
			PANEL_ITEM_Y, 25,
			0);
	chpters = panel_create_item(info_panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING, "Chapter   : ",
			PANEL_ITEM_X, ATTR_COL(1),
			PANEL_ITEM_Y, 75,
			0);
	dsk_types = panel_create_item(info_panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING, "Disk Type : ",
			PANEL_ITEM_X, ATTR_COL(1),
			PANEL_ITEM_Y, 125,
			0);

	adrezz = panel_create_item(info_panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING, "",
			PANEL_ITEM_X, ATTR_COL(13),
			PANEL_ITEM_Y, 25,
			0);
	chpter = panel_create_item(info_panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING, "",
			PANEL_ITEM_X, ATTR_COL(13),
			PANEL_ITEM_Y, 75, 
			0); 
	dsk_type = panel_create_item(info_panel, PANEL_MESSAGE, 
			PANEL_LABEL_STRING, "",
			PANEL_ITEM_X, ATTR_COL(13),
			PANEL_ITEM_Y, 125,
			0);
}
