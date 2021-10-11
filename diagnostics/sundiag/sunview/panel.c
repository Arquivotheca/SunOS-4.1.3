#ifndef lint
static	char sccsid[] = "@(#)panel.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include "sundiag.h"
#include <sys/stat.h>
#include "procs.h"

#define BUTTON_X		0	/* button block's origin */
#define BUTTON_Y		0

#define BUTTON_ROWS		1	/* gap between button rows */
#define BUTTON_COLS		14	/* gap between button columns */

/*
 * control panel object declarations
 */
Panel_item		test_item;
Panel_item		reset_item;
Panel_item		log_files_item;
Panel_item		option_files_item;
Panel_item		options_item;
Panel_item		quit_item;
Panel_item		pscrn_item;
Panel_item		ttime_item;
Panel_item		eeprom_item;
Panel_item              cpu_item;


Panel_item		mode_item;
Panel_item		select_item;

Pixrect			*start_button, *stop_button, *reset_button,
			*resume_button, *suspend_button;
Pixrect			*ttime_button, *ttime_button_org;
Pixrect			*options_button, *options_button_org;
Pixrect			*eeprom_button, *eeprom_button_org;
Pixrect			*opf_button, *opf_button_org;
Pixrect                 *cpu_button, *cpu_button_org;

Menu			log_files_menu;

/******************************************************************************
 * Initialize the objects in the control subwindow.			      *
 ******************************************************************************/
init_control(sundiag_control)
Panel	sundiag_control;
{

    start_button = panel_button_image(sundiag_control, "Start",
							10, (Pixfont *)NULL);
    stop_button = panel_button_image(sundiag_control, "Stop",
							10, (Pixfont *)NULL);
    reset_button = panel_button_image(sundiag_control, "Reset",
							10, (Pixfont *)NULL);
    suspend_button = panel_button_image(sundiag_control, "Suspend",
							10, (Pixfont *)NULL);
    resume_button = panel_button_image(sundiag_control, "Resume",
							10, (Pixfont *)NULL);

    /* These versions of buttons will be grayed when running tests */
    ttime_button = panel_button_image(sundiag_control, "Schedule",
							10, (Pixfont *)NULL);
    options_button = panel_button_image(sundiag_control, "Options",
							10, (Pixfont *)NULL);
    opf_button = panel_button_image(sundiag_control, "Optfiles",
							10, (Pixfont *)NULL);
    if (strncmp(cpuname, "Sun-4m/", 7))
    {
        eeprom_button = panel_button_image(sundiag_control, "EEPROM",
							10, (Pixfont *)NULL);
    }
    cpu_button = panel_button_image(sundiag_control, "Processors",
							10, (Pixfont *)NULL);

    /* keep an extra copy so that the "grayed" version can be restored from */
    ttime_button_org = panel_button_image(sundiag_control, "Schedule",
							10, (Pixfont *)NULL);
    options_button_org = panel_button_image(sundiag_control, "Options",
							10, (Pixfont *)NULL);
    opf_button_org = panel_button_image(sundiag_control, "Optfiles",
							10, (Pixfont *)NULL);
    if (strncmp(cpuname, "Sun-4m/", 7))
    {
        eeprom_button_org = panel_button_image(sundiag_control, "EEPROM",
							10, (Pixfont *)NULL);
    }
    cpu_button_org = panel_button_image(sundiag_control, "Processors",
                                                        10, (Pixfont *)NULL);


/*---- 1st row ----*/

	/***** START(STOP) TESTS *****/
	test_item = panel_create_item(sundiag_control, PANEL_BUTTON,
      	    PANEL_LABEL_IMAGE,	start_button,
      	    PANEL_NOTIFY_PROC,  start_proc,
	    PANEL_ITEM_X,	ATTR_COL(BUTTON_Y),
	    PANEL_ITEM_Y,	ATTR_ROW(BUTTON_X),
      	    0);

	/***** RESET TESTS *****/
	reset_item = panel_create_item(sundiag_control, PANEL_BUTTON,
      	    PANEL_LABEL_IMAGE,  reset_button,
      	    PANEL_NOTIFY_PROC,  reset_proc,
	    PANEL_ITEM_X,	ATTR_COL(BUTTON_Y + BUTTON_COLS),
	    PANEL_ITEM_Y,	ATTR_ROW(BUTTON_X),
      	    0);

	/***** QUIT *****/
	quit_item = panel_create_item(sundiag_control, PANEL_BUTTON,
      	    PANEL_LABEL_IMAGE,	panel_button_image(sundiag_control,
				    "Quit", 10, (Pixfont *)NULL),
      	    PANEL_NOTIFY_PROC,  quit_proc,
	    PANEL_ITEM_X,	ATTR_COL(BUTTON_Y + 2*BUTTON_COLS),
	    PANEL_ITEM_Y,	ATTR_ROW(BUTTON_X),
      	    0);

/*---- 2nd row ----*/

	/***** Print Screen *****/
	pscrn_item = panel_create_item(sundiag_control, PANEL_BUTTON,
      	    PANEL_LABEL_IMAGE,	panel_button_image(sundiag_control,
				    "Print", 10, (Pixfont *)NULL),
      	    PANEL_NOTIFY_PROC,  pscrn_proc,
	    PANEL_ITEM_X,	ATTR_COL(BUTTON_Y),
	    PANEL_ITEM_Y,	ATTR_ROW(BUTTON_X + BUTTON_ROWS),
      	    0);

	/***** LOG FILES *****/
	log_files_menu = menu_create(
	    MENU_NCOLS, 3,
	    MENU_STRINGS,
	    "Display error logs", "Remove error logs", "Print error logs",
	    "Display info. logs", "Remove info. logs", "Print info. logs",
	    "Display UNIX logs", "Remove UNIX logs", "Print UNIX logs", 0,
	    0);

	log_files_item = panel_create_item(sundiag_control, PANEL_BUTTON,
      	    PANEL_LABEL_IMAGE,	panel_button_image(sundiag_control,
				    "Logfiles", 10, (Pixfont *)NULL),
      	    PANEL_EVENT_PROC,   log_files_menu_proc,
	    PANEL_ITEM_X,	ATTR_COL(BUTTON_Y + BUTTON_COLS),
	    PANEL_ITEM_Y,	ATTR_ROW(BUTTON_X + BUTTON_ROWS),
      	    0);

        cpu_item = panel_create_item(sundiag_control, PANEL_BUTTON,
            PANEL_LABEL_IMAGE,  cpu_button,
            PANEL_NOTIFY_PROC,  cpu_proc,
            PANEL_ITEM_X,       ATTR_COL(BUTTON_Y + 2*BUTTON_COLS),
            PANEL_ITEM_Y,       ATTR_ROW(BUTTON_X + BUTTON_ROWS),
            0);

/*---- 3rd row ----*/

	/***** TEST TIMES *****/
	ttime_item = panel_create_item(sundiag_control, PANEL_BUTTON,
      	    PANEL_LABEL_IMAGE,	ttime_button,
      	    PANEL_NOTIFY_PROC,  ttime_proc,
	    PANEL_ITEM_X,	ATTR_COL(BUTTON_Y),
	    PANEL_ITEM_Y,	ATTR_ROW(BUTTON_X + 2*BUTTON_ROWS),
      	    0);

	/***** OPTIONS *****/
	options_item = panel_create_item(sundiag_control, PANEL_BUTTON,
      	    PANEL_LABEL_IMAGE,	options_button,
      	    PANEL_NOTIFY_PROC,  options_proc,
	    PANEL_ITEM_X,	ATTR_COL(BUTTON_Y + BUTTON_COLS),
	    PANEL_ITEM_Y,	ATTR_ROW(BUTTON_X + 2*BUTTON_ROWS),
      	    0);

	/***** OPTION FILES *****/
	option_files_item = panel_create_item(sundiag_control, PANEL_BUTTON,
      	    PANEL_LABEL_IMAGE,	opf_button,
      	    PANEL_NOTIFY_PROC,  option_files_proc,
	    PANEL_ITEM_X,	ATTR_COL(BUTTON_Y + 2*BUTTON_COLS),
	    PANEL_ITEM_Y,	ATTR_ROW(BUTTON_X + 2*BUTTON_ROWS),
      	    0);

/*---- 4th row ----*/

    if (strncmp(cpuname, "Sun-4m/", 7))
    {
	/***** CHECK EEPROM*****/
	eeprom_item = panel_create_item(sundiag_control, PANEL_BUTTON,
      	    PANEL_LABEL_IMAGE,	eeprom_button,
      	    PANEL_NOTIFY_PROC,  eeprom_proc,
	    PANEL_ITEM_X,	ATTR_COL(BUTTON_Y),
	    PANEL_ITEM_Y,	ATTR_ROW(BUTTON_X + 3*BUTTON_ROWS),
      	    0);
    }

/*---- the remains ----*/

	/***** TEST MODE *****/
	mode_item = panel_create_item(sundiag_control, PANEL_CYCLE,
            PANEL_LABEL_X,              ATTR_COL(BUTTON_Y),
            PANEL_LABEL_Y,              ATTR_ROW(BUTTON_X+(5*BUTTON_ROWS)),
            PANEL_LABEL_STRING,         "Intervention:",
	    PANEL_CHOICE_STRINGS,	"Disable", "Enable", 0,
	    PANEL_VALUE,		intervention,
	    PANEL_NOTIFY_PROC,		interven_proc,
            0);

	/***** TEST SELECTIONS *****/
	select_item = panel_create_item(sundiag_control, PANEL_CHOICE,
            PANEL_LABEL_X,              ATTR_COL(BUTTON_Y),
            PANEL_LABEL_Y,              ATTR_ROW(BUTTON_X+(6*BUTTON_ROWS)),
            PANEL_LABEL_STRING,         "Test selections:",
	    PANEL_CHOICE_STRINGS,	" Default ", " None ", " All ", 0,
	    PANEL_FEEDBACK,		PANEL_INVERTED,
	    PANEL_VALUE,		select_value,
	    PANEL_NOTIFY_PROC,		select_proc,
            0);
}		

