#ifndef lint
static char     sccsid[] = "@(#)canfieldtool.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
/*
 * Copyright (c) 1985 by Sun Microsystems Inc. 
 */
#include "defs.h"
#include "icon.h"
#include "outline.h"

static short    gray17_data[] = {
#include <images/square_17.pr>
};
mpr_static(gray17_patch, 12, 12, 1, gray17_data);
static Panel    panel;
canfieldtool_main(argc, argv)
	int             argc;
	char          **argv;
{
	char          **tool_args = NULL;
	char           *toolname = "canfieldtool";
	struct inputmask mask;
	talon_num = 4;
	stock_num = 13;
	hand_num = 31;
	old_position = "  ";

	if (tool_parse_all(&argc, argv, &tool_args, toolname) < 0) {
		tool_usage(toolname);
		exit(1);
	}
	if (argc > 1)
		canfield = argv[1];
	else
		canfield = CANFIELD;

	/* start the tool */
	if ((tool = tool_make(WIN_NAME_STRIPE, 1,
			      WIN_LABEL, toolname,
			      WIN_ICON, &toolicon,
			      WIN_HEIGHT, TOOLHEIGHT,
			      WIN_WIDTH, TOOLWIDTH,
			      WIN_ATTR_LIST, tool_args,
			      0)) == NULL) {
		fputs("Can't start tool\n", stderr);
		exit(1);
	}
	tool_free_attribute_list(tool_args);

	/* start the panel subwindow */
	if ((panel_sw = panel_create(tool, 0)) == NULL) {
		fputs("Can't start panel\n", stderr);
		exit(1);
	}
	panel = panel_sw->ts_data;

	/* define the panel buttons */
	panel_create_item(panel, PANEL_BUTTON,
	   PANEL_LABEL_IMAGE, panel_button_image(panel, "Quit", 4, NULL),
			  PANEL_NOTIFY_PROC, quit_proc,
			  0);

	panel_create_item(panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE, panel_button_image(panel, "Restart", 7, NULL),
			  PANEL_NOTIFY_PROC, restart_proc,
			  0);

	msg_item = panel_create_item(panel, PANEL_MESSAGE,
				     PANEL_LABEL_STRING, "Use the mouse to move the cards, and click on the hand to deal",
				     0);

	panel_fit_height(panel);

	/* create the cards subwindow */
	if ((cards_sw = tool_createsubwindow(tool, "", TOOL_SWEXTENDTOEDGE,
					 TOOL_SWEXTENDTOEDGE)) == NULL) {
		fputs("can't create card subwindow\n", stderr);
		exit(1);
	}
	cards_sw->ts_io.tio_selected = mouse_reader;
	cards_sw->ts_io.tio_handlesigwinch = handle_sigwinch;
	cards_pixwin = pw_open(cards_sw->ts_windowfd);

	/* setting up the input mask */
	input_imnull(&mask);
	mask.im_flags |= IM_NEGEVENT;
	win_setinputcodebit(&mask, MS_LEFT);
	win_setinputcodebit(&mask, MS_MIDDLE);
	win_setinputcodebit(&mask, MS_RIGHT);
	win_setinputcodebit(&mask, LOC_MOVEWHILEBUTDOWN);
	win_setinputmask(cards_sw->ts_windowfd, &mask, (struct inputmask *) NULL, WIN_NULLLINK);

	setup_pixrects();

	signal(SIGWINCH, sigwinched);
	signal(SIGINT, SIG_IGN);
	tool_install(tool);

	old_selected = tool->tl_io.tio_selected;
	tool->tl_io.tio_selected = cf_reader;
	cf_fd = start_canfield();
	FD_SET(tool->tl_windowfd, &tool->tl_io.tio_inputmask);
	FD_SET(cf_fd, &tool->tl_io.tio_inputmask);

	tool_select(tool, 0);
	tool_destroy(tool);
}
setup_pixrects()
{
	f1_pixrect = f2_pixrect = f3_pixrect = f4_pixrect = NULL;
	stock_pixrect = talon_pixrect = NULL;
	outline_pr = &outline_mpr;
	background_pr = &gray17_patch;
	/* Since font only used as arg to pf_text, OK to be NULL */
	font = pf_open("/usr/lib/fonts/fixedwidthfonts/screen.b.14");
}
