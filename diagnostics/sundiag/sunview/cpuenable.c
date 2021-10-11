static  char sccsid[] = "@(#)cpuenable.c 1.1 92/07/30 Copyright Sun Micro";

/*
 * This file contains the routines that are
 * called by the schedule option in sundiag.
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include "sundiag.h"
#include "sundiag_msg.h"
#include "../../lib/include/libonline.h" /* sundiag standard include file */
#include "sdrtns.h"

static Frame cpu_enable_frame = NULL;
static Panel cpu_panel;

Panel_item cpus_item[30];

/*********** global variables ************/

int number_system_processors;
int system_processors_mask;
int system_processor_selected[30];

/*********** forward references ************/

static cpu_done_proc();
static cpu_default_proc();

/******************************************************************************
 * Notify procedure for the "Processors" button.			      *
 ******************************************************************************/
/* ARGSUSED */
cpu_proc()
{
    int which_row = 0, i;
    char processor_name[30];
    int memfd;

    if (running == GO) return;

    if (number_system_processors > 1)
    {
        /* close the system processors popup if it was opened */
        if (cpu_enable_frame != NULL)
	    frame_destroy_proc(cpu_enable_frame);

        cpu_enable_frame = window_create(sundiag_frame, FRAME,
	    FRAME_SHOW_LABEL, TRUE,
	    FRAME_LABEL,	"Processors Enable Menu",
	    WIN_X, (int)((STATUS_WIDTH+PERFMON_WIDTH)*frame_width)+15,
	    WIN_Y,	20,
	    FRAME_DONE_PROC, frame_destroy_proc, 0);

        cpu_panel = window_create(cpu_enable_frame, PANEL, 0);

	for (i = 0; i < number_system_processors; i++)
	{
	    sprintf(processor_name, "processor %d", i);
            cpus_item[i] = panel_create_item(
                cpu_panel, PANEL_CYCLE,
                PANEL_LABEL_STRING, processor_name, 
	        PANEL_CHOICE_STRINGS, "Enable ", "Disable", 0,
	        PANEL_VALUE, system_processor_selected[i],
                PANEL_ITEM_X, ATTR_COL(1),
                PANEL_ITEM_Y, ATTR_ROW(which_row++),
                0);
	} 

        ++which_row;

        (void)panel_create_item(cpu_panel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE, panel_button_image(cpu_panel,
			"Default", 7, (Pixfont *)NULL),
	    PANEL_ITEM_X, ATTR_COL(1),
	    PANEL_ITEM_Y, ATTR_ROW(which_row),
	    PANEL_NOTIFY_PROC, cpu_default_proc,
	    0);

        (void)panel_create_item(cpu_panel, PANEL_BUTTON,
    	    PANEL_LABEL_IMAGE, panel_button_image(cpu_panel,
			"Done", 7, (Pixfont *)NULL),
	    PANEL_NOTIFY_PROC, cpu_done_proc,
	    0);

        window_fit(cpu_panel);
        window_fit(cpu_enable_frame);
        (void)window_set(cpu_enable_frame, WIN_SHOW, TRUE, 0);
    }
    else
    {   
        (void) confirm("The Processors feature is not available for uniprocessor!", TRUE);
    }

}

/*
 * Come here if default button is pressed.
 */
static
cpu_default_proc()
{
    int i;

    /* set the panel item to default value, enable the processors */
    for (i = 0; i < number_system_processors; i++) 
    {
        (void)panel_set(cpus_item[i], PANEL_VALUE, 0, 0);
    }
    system_processors_mask = 0xf;

}

/*
 * Read in the entries from the scheduler menu.
 */
static
cpu_done_proc()
{
    int i;
  
    for (i = 0; i < number_system_processors; i++) 
    {
        system_processor_selected[i] = (int)panel_get_value(cpus_item[i]);
        if (system_processor_selected[i] == 1)  
        {
	    system_processors_mask &= ~(system_processor_selected[i] << i);
        }
    }

    (void)window_set(cpu_enable_frame, FRAME_NO_CONFIRM, TRUE, 0);
    (void)window_destroy(cpu_enable_frame);
    cpu_enable_frame = NULL;

}

