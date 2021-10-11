#ifndef lint
static	char sccsid[] = "@(#)cycle_create.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <suntool/sunview.h>
#include <suntool/panel.h>
#include "lasertool.h"

static short fwd_button_data[] = {
#include "forward.icon"
};
static short fwd_on_button_data[] = {
#include "forward_on.icon"
};

static short rev_button_data[] = {
#include "reverse.icon"
};
static short rev_on_button_data[] = {
#include "reverse_on.icon"
};

static short audio_data[] = {
#include "audio.icon"
};

mpr_static(fwd_button_pixrect, 64, 64, 1, fwd_button_data);
mpr_static(fwd_on_button_pixrect, 64, 64, 1, fwd_on_button_data);
mpr_static(rev_button_pixrect, 64, 64, 1, rev_button_data);
mpr_static(rev_on_button_pixrect, 64, 64, 1, rev_on_button_data);
mpr_static(audio_pixrect, 64, 64, 1, audio_data);

Panel_item Forward, Reverse, cycle_speed, Audio_cycle, Eject_cycle;
Panel_item Frame_cycle, Index_cycle, Video_cycle, Psc_cycle;
Panel_item Motor_cycle, Mem_cycle, Disk_inq_cycle, error_return;
Panel_item trk_jmp, Rate;

extern int play_mode;

cycle_create()
{
	extern Panel cycle_panel;
	extern void sbyte_bproc(), dir_mode_proc();
	extern void audio_proc(), frame_proc(), video_proc();
	extern void index_proc(), psc_proc(), eject_proc();
	extern void motor_proc(), mem_proc(), disk_inq_proc();
	void dir_mode_notify(), cycle_speed_notify();

	Reverse = panel_create_item(cycle_panel, PANEL_BUTTON,
			PANEL_LABEL_IMAGE, &rev_button_pixrect,
			PANEL_NOTIFY_PROC, dir_mode_notify,
			PANEL_ITEM_Y, ATTR_ROW(0),
			0);
	Forward = panel_create_item(cycle_panel, PANEL_BUTTON,
			PANEL_LABEL_IMAGE, &fwd_on_button_pixrect,
			PANEL_NOTIFY_PROC, dir_mode_notify,
			PANEL_ITEM_Y, ATTR_ROW(0),
			0);
	cycle_speed = panel_create_item(cycle_panel, PANEL_CYCLE,
			PANEL_CHOICE_STRINGS,
			"play", "scan", "fast", "slow", "step & still",
			"var speed", "Track Jump", 0,
			PANEL_ITEM_Y, 55,
			PANEL_NOTIFY_PROC, cycle_speed_notify,
			0);

	Rate = panel_create_item(cycle_panel, PANEL_TEXT,
			PANEL_LABEL_STRING, "Rate : ",
			PANEL_VALUE_DISPLAY_LENGTH, 5,
			PANEL_ITEM_X, 275,
			PANEL_ITEM_Y, 35,
			0);

	trk_jmp = panel_create_item(cycle_panel, PANEL_TEXT,
			PANEL_LABEL_STRING, "# Tracks : ",
			PANEL_VALUE_DISPLAY_LENGTH, 5,
			PANEL_ITEM_X, 275,
			PANEL_ITEM_Y, 55,
			0);

	Audio_cycle = panel_create_item(cycle_panel, PANEL_CYCLE,
			PANEL_LABEL_IMAGE, &audio_pixrect,
			PANEL_CHOICE_STRINGS,
			"Audio mute on", "Audio mute off", "Ch1 on", "Ch1 off",
			"Ch2 on", "Ch2 off", "CX on", "CX off", 0,
			PANEL_NOTIFY_PROC, audio_proc,
			PANEL_ITEM_X, 425,
			PANEL_ITEM_Y, ATTR_ROW(0),
			0);

	Frame_cycle = panel_create_item(cycle_panel, PANEL_CYCLE,
			PANEL_LABEL_STRING, "Frame Mode",
			PANEL_CHOICE_STRINGS,
			"Frame", "Chapter", 0,
			PANEL_NOTIFY_PROC, frame_proc,
			PANEL_ITEM_X, ATTR_ROW(0),
			PANEL_ITEM_Y, 75,
			0);

	Eject_cycle = panel_create_item(cycle_panel, PANEL_CYCLE,
			PANEL_LABEL_STRING, "Eject",
			PANEL_CHOICE_STRINGS, 
			"enable", "disable", "eject", 0,
			PANEL_NOTIFY_PROC, eject_proc,
			PANEL_ITEM_X, 200,
			PANEL_ITEM_Y, 75,
			0);

	Index_cycle = panel_create_item(cycle_panel, PANEL_CYCLE,
			PANEL_LABEL_STRING, "Frame Display",
			PANEL_CHOICE_STRINGS, "Off", "On", 0,
			PANEL_NOTIFY_PROC, index_proc,
			PANEL_ITEM_X, 400,
			PANEL_ITEM_Y, 75,
			0);

	Video_cycle = panel_create_item(cycle_panel, PANEL_CYCLE,
			PANEL_LABEL_STRING, "Video",
			PANEL_CHOICE_STRINGS, "On", "Off", 0,
			PANEL_NOTIFY_PROC, video_proc,
			PANEL_ITEM_X, ATTR_ROW(0),
			PANEL_ITEM_Y, 100,
			0);

	Psc_cycle = panel_create_item(cycle_panel, PANEL_CYCLE,
			PANEL_LABEL_STRING, "Psc",
			PANEL_CHOICE_STRINGS, 
			"enable", "disable", 0,
			PANEL_NOTIFY_PROC, psc_proc,
			PANEL_ITEM_X, 200,
			PANEL_ITEM_Y, 100,
			0);


	Motor_cycle = panel_create_item(cycle_panel, PANEL_CYCLE,
			PANEL_LABEL_STRING, "Motor",
			PANEL_CHOICE_STRINGS, "On", "Off", 0,
			PANEL_NOTIFY_PROC, motor_proc,
			PANEL_ITEM_X, 400,
			PANEL_ITEM_Y, 100,
			0);

	Disk_inq_cycle = panel_create_item(cycle_panel, PANEL_CYCLE,
			PANEL_LABEL_STRING, "Disk Inq",
			PANEL_CHOICE_STRINGS,
			"Status", "User Code", "Rom Version", 0,
			PANEL_NOTIFY_PROC, disk_inq_proc,
			PANEL_ITEM_X, ATTR_ROW(0),
			PANEL_ITEM_Y, 125,
			0);

	Mem_cycle = panel_create_item(cycle_panel, PANEL_CYCLE,
			PANEL_LABEL_STRING, "Memory",
			PANEL_CHOICE_STRINGS, "Mark", "Search", 0,
			PANEL_NOTIFY_PROC, mem_proc,
			PANEL_ITEM_X, 200,
			PANEL_ITEM_Y, 125,
			0);

	error_return = panel_create_item(cycle_panel, PANEL_MESSAGE,
			PANEL_LABEL_BOLD, TRUE,
			PANEL_ITEM_X, ATTR_COL(11),
			PANEL_ITEM_Y, 155,
			0);
}

/*
 * Change of mode of play, keep direction the same
 * and reset the play mode
 */
void
cycle_speed_notify(item, value, event)
Panel_item item;
int value;
Event *event;
{
	extern void dir_mode_proc();

	dir_mode_proc(play_mode, value);
	return;
}

/*
 * Change of direction, reset the direction keep
 * the play mode the same
 */
void
dir_mode_notify(item, event)
Panel_item item;
Event *event;
{
	if(item == Forward)
		dir_mode_proc(FWD, (int)panel_get(cycle_speed,PANEL_VALUE) );
	else if(item == Reverse)
		dir_mode_proc(REV, (int)panel_get(cycle_speed,PANEL_VALUE) );
	return;
}
