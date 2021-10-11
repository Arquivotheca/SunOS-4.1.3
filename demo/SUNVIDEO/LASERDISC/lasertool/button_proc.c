#ifndef lint
static	char sccsid[] = "@(#)button_proc.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/alert.h>
#include "sony_laser.h"
#include "lasertool.h"

extern Panel_item Clear_entry, Speed;
extern Panel_item Clear_all, Cont;
extern Panel_item Forward, Reverse;
extern Panel_item Memory, Laser_menu, Non_cf_play;
extern Panel_item Still, Stop, Quit;

extern FILE *device;
extern int poll_state;
extern Window base_frame;
extern Panel_item error_return;

int play_mode;

void
button_proc(item, event)
Panel_item item;
Event *event;
{
	static char string[80];
	unsigned char rc;
	int result;

	if(item == Clear_entry) 
		sony_clear_entry(device);
	else if(item == Clear_all) {
		sony_clear_all(device);
		poll_state = POLL_PRINT;
		panel_set(error_return, PANEL_LABEL_STRING, "", 0);
	} else if(item == Cont) 
		sony_cont(device);
	else if(item == Laser_menu)
		sony_menu(device);
	else if(item == Non_cf_play)
		sony_non_cf_play(device);
	else if(item == Still) {
		if((rc = sony_still(device)) == CLV_DISK) {
			sprintf(string,"still : Invalid command for CLV disks\n");
			panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			return;
		} else if(rc == NAK) {
			sprintf(string,"still : NAK returned\n");
			panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			return;
		}
	} else if(item == Stop)
		sony_stop(device);
	else if(item == Quit) {
		result = alert_prompt((Frame)base_frame, (Event *) NULL,
			ALERT_MESSAGE_STRINGS, "Confirm quit", 0,
			ALERT_BUTTON_YES, "Confirm",
			ALERT_BUTTON_NO, "Cancel",
			ALERT_POSITION, ALERT_CLIENT_CENTERED,
			0);
		if((result == ALERT_NO) || (result == ALERT_FAILED))
			return;
		sony_clear_all(device);
		sony_close(device);
		exit(0);
	}
	return;
}

