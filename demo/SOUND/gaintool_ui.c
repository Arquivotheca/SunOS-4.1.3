/*
 * gaintool_ui.c - User interface object initialization functions.
 */
#ifndef lint
static  char sccsid[] = "@(#)gaintool_ui.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif
  
/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/scrollbar.h>
#include <xview/svrimage.h>
#include <xview/termsw.h>
#include <xview/text.h>
#include <xview/tty.h>
#include <xview/xv_xrect.h>
#include "gaintool_ui.h"

/*
 * Create object `props_menu' in the specified instance.
 */
Xv_opaque
gaintool_props_menu_create(ip, owner)
	caddr_t		ip;
	Xv_opaque	owner;
{
	extern Menu_item	menu_status();
	Xv_opaque	obj;
	
	obj = xv_create(XV_NULL, MENU_COMMAND_MENU,
		XV_KEY_DATA, INSTANCE, ip,
		MENU_TITLE_ITEM, "Audio Control",
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, "Status...",
			MENU_GEN_PROC, menu_status,
			NULL,
		MENU_DEFAULT, 2,
		NULL);
	return obj;
}

/*
 * Initialize an instance of object `BaseFrame'.
 */
gaintool_BaseFrame_objects *
gaintool_BaseFrame_objects_initialize(ip, owner)
	gaintool_BaseFrame_objects	*ip;
	Xv_opaque	owner;
{
	if (!ip && !(ip = (gaintool_BaseFrame_objects *) calloc(1, sizeof (gaintool_BaseFrame_objects))))
		return (gaintool_BaseFrame_objects *) NULL;
	if (!ip->BaseFrame)
		ip->BaseFrame = gaintool_BaseFrame_BaseFrame_create(ip, owner);
	if (!ip->GainPanel)
		ip->GainPanel = gaintool_BaseFrame_GainPanel_create(ip, ip->BaseFrame);
	if (!ip->PlaySlider)
		ip->PlaySlider = gaintool_BaseFrame_PlaySlider_create(ip, ip->GainPanel);
	if (!ip->RecordSlider)
		ip->RecordSlider = gaintool_BaseFrame_RecordSlider_create(ip, ip->GainPanel);
	if (!ip->MonitorSlider)
		ip->MonitorSlider = gaintool_BaseFrame_MonitorSlider_create(ip, ip->GainPanel);
	if (!ip->OutputSwitch)
		ip->OutputSwitch = gaintool_BaseFrame_OutputSwitch_create(ip, ip->GainPanel);
	if (!ip->Pause_button)
		ip->Pause_button = gaintool_BaseFrame_Pause_button_create(ip, ip->GainPanel);
	return ip;
}

/*
 * Create object `BaseFrame' in the specified instance.
 */
Xv_opaque
gaintool_BaseFrame_BaseFrame_create(ip, owner)
	gaintool_BaseFrame_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	Xv_opaque		BaseFrame_image;
	static unsigned short	BaseFrame_bits[] = {
#include "gaintool.icon"
	};
	Xv_opaque		BaseFrame_image_mask;
	static unsigned short	BaseFrame_mask_bits[] = {
#include "gaintool.mask.icon"
	};
	
	BaseFrame_image = xv_create(XV_NULL, SERVER_IMAGE,
		SERVER_IMAGE_DEPTH, 1,
		SERVER_IMAGE_BITS, BaseFrame_bits,
		XV_WIDTH, 64,
		XV_HEIGHT, 64,
		NULL);
	BaseFrame_image_mask = xv_create(XV_NULL, SERVER_IMAGE,
		SERVER_IMAGE_DEPTH, 1,
		SERVER_IMAGE_BITS, BaseFrame_mask_bits,
		XV_WIDTH, 64,
		XV_HEIGHT, 64,
		NULL);
	obj = xv_create(owner, FRAME,
		XV_KEY_DATA, INSTANCE, ip,
		XV_WIDTH, 290,
		XV_HEIGHT, 94,
		XV_LABEL, "Audio Control Panel",
		FRAME_SHOW_FOOTER, FALSE,
		FRAME_SHOW_RESIZE_CORNER, FALSE,
		FRAME_ICON, xv_create(XV_NULL, ICON,
			ICON_IMAGE, BaseFrame_image,
			ICON_MASK_IMAGE, BaseFrame_image_mask,
			NULL),
		NULL);
	return obj;
}

/*
 * Create object `GainPanel' in the specified instance.
 */
Xv_opaque
gaintool_BaseFrame_GainPanel_create(ip, owner)
	gaintool_BaseFrame_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 0,
		XV_Y, 0,
		XV_WIDTH, WIN_EXTEND_TO_EDGE,
		XV_HEIGHT, WIN_EXTEND_TO_EDGE,
		WIN_BORDER, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `PlaySlider' in the specified instance.
 */
Xv_opaque
gaintool_BaseFrame_PlaySlider_create(ip, owner)
	gaintool_BaseFrame_objects	*ip;
	Xv_opaque	owner;
{
	extern void		play_volume_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_SLIDER,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 30,
		XV_Y, 6,
		PANEL_SLIDER_WIDTH, 100,
		PANEL_TICKS, 0,
		PANEL_LABEL_STRING, "Play Volume:",
		PANEL_DIRECTION, PANEL_HORIZONTAL,
		PANEL_SLIDER_END_BOXES, FALSE,
		PANEL_SHOW_RANGE, FALSE,
		PANEL_SHOW_VALUE, TRUE,
		PANEL_MIN_VALUE, 0,
		PANEL_MAX_VALUE, 100,
		PANEL_VALUE, 0,
		PANEL_NOTIFY_PROC, play_volume_proc,
		NULL);
	return obj;
}

/*
 * Create object `RecordSlider' in the specified instance.
 */
Xv_opaque
gaintool_BaseFrame_RecordSlider_create(ip, owner)
	gaintool_BaseFrame_objects	*ip;
	Xv_opaque	owner;
{
	extern void		record_volume_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_SLIDER,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 13,
		XV_Y, 24,
		PANEL_SLIDER_WIDTH, 100,
		PANEL_TICKS, 0,
		PANEL_LABEL_STRING, "Record Volume:",
		PANEL_DIRECTION, PANEL_HORIZONTAL,
		PANEL_SLIDER_END_BOXES, FALSE,
		PANEL_SHOW_RANGE, FALSE,
		PANEL_SHOW_VALUE, TRUE,
		PANEL_MIN_VALUE, 0,
		PANEL_MAX_VALUE, 100,
		PANEL_VALUE, 0,
		PANEL_NOTIFY_PROC, record_volume_proc,
		NULL);
	return obj;
}

/*
 * Create object `MonitorSlider' in the specified instance.
 */
Xv_opaque
gaintool_BaseFrame_MonitorSlider_create(ip, owner)
	gaintool_BaseFrame_objects	*ip;
	Xv_opaque	owner;
{
	extern void		monitor_volume_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_SLIDER,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 6,
		XV_Y, 42,
		PANEL_SLIDER_WIDTH, 100,
		PANEL_TICKS, 0,
		PANEL_LABEL_STRING, "Monitor Volume:",
		PANEL_DIRECTION, PANEL_HORIZONTAL,
		PANEL_SLIDER_END_BOXES, FALSE,
		PANEL_SHOW_RANGE, FALSE,
		PANEL_SHOW_VALUE, TRUE,
		PANEL_MIN_VALUE, 0,
		PANEL_MAX_VALUE, 100,
		PANEL_VALUE, 0,
		PANEL_NOTIFY_PROC, monitor_volume_proc,
		NULL);
	return obj;
}

/*
 * Create object `OutputSwitch' in the specified instance.
 */
Xv_opaque
gaintool_BaseFrame_OutputSwitch_create(ip, owner)
	gaintool_BaseFrame_objects	*ip;
	Xv_opaque	owner;
{
	extern void		control_output_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_CHOICE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 14,
		XV_Y, 67,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_CHOOSE_NONE, FALSE,
		PANEL_LABEL_STRING, "Output:",
		PANEL_NOTIFY_PROC, control_output_proc,
		PANEL_CHOICE_STRINGS,
			"Spkr",
			"Jack",
			NULL,
		NULL);
	return obj;
}

/*
 * Create object `Pause_button' in the specified instance.
 */
Xv_opaque
gaintool_BaseFrame_Pause_button_create(ip, owner)
	gaintool_BaseFrame_objects	*ip;
	Xv_opaque	owner;
{
	extern void		pause_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 180,
		XV_Y, 69,
		PANEL_LABEL_WIDTH, 78,
		PANEL_LABEL_STRING, "Resume Play",
		PANEL_NOTIFY_PROC, pause_proc,
		NULL);
	return obj;
}

/*
 * Initialize an instance of object `StatusPanel'.
 */
gaintool_StatusPanel_objects *
gaintool_StatusPanel_objects_initialize(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	if (!ip && !(ip = (gaintool_StatusPanel_objects *) calloc(1, sizeof (gaintool_StatusPanel_objects))))
		return (gaintool_StatusPanel_objects *) NULL;
	if (!ip->StatusPanel)
		ip->StatusPanel = gaintool_StatusPanel_StatusPanel_create(ip, owner);
	if (!ip->PlayStatusPanel)
		ip->PlayStatusPanel = gaintool_StatusPanel_PlayStatusPanel_create(ip, ip->StatusPanel);
	if (!ip->Ppanel_label)
		ip->Ppanel_label = gaintool_StatusPanel_Ppanel_label_create(ip, ip->PlayStatusPanel);
	if (!ip->Popen_flag)
		ip->Popen_flag = gaintool_StatusPanel_Popen_flag_create(ip, ip->PlayStatusPanel);
	if (!ip->Ppaused_flag)
		ip->Ppaused_flag = gaintool_StatusPanel_Ppaused_flag_create(ip, ip->PlayStatusPanel);
	if (!ip->Pactive_flag)
		ip->Pactive_flag = gaintool_StatusPanel_Pactive_flag_create(ip, ip->PlayStatusPanel);
	if (!ip->Perror_flag)
		ip->Perror_flag = gaintool_StatusPanel_Perror_flag_create(ip, ip->PlayStatusPanel);
	if (!ip->Pwaiting_flag)
		ip->Pwaiting_flag = gaintool_StatusPanel_Pwaiting_flag_create(ip, ip->PlayStatusPanel);
	if (!ip->Peof_label)
		ip->Peof_label = gaintool_StatusPanel_Peof_label_create(ip, ip->PlayStatusPanel);
	if (!ip->Peof_value)
		ip->Peof_value = gaintool_StatusPanel_Peof_value_create(ip, ip->PlayStatusPanel);
	if (!ip->Psam_label)
		ip->Psam_label = gaintool_StatusPanel_Psam_label_create(ip, ip->PlayStatusPanel);
	if (!ip->Psam_value)
		ip->Psam_value = gaintool_StatusPanel_Psam_value_create(ip, ip->PlayStatusPanel);
	if (!ip->Prate_label)
		ip->Prate_label = gaintool_StatusPanel_Prate_label_create(ip, ip->PlayStatusPanel);
	if (!ip->Prate_value)
		ip->Prate_value = gaintool_StatusPanel_Prate_value_create(ip, ip->PlayStatusPanel);
	if (!ip->Pchan_label)
		ip->Pchan_label = gaintool_StatusPanel_Pchan_label_create(ip, ip->PlayStatusPanel);
	if (!ip->Pchan_value)
		ip->Pchan_value = gaintool_StatusPanel_Pchan_value_create(ip, ip->PlayStatusPanel);
	if (!ip->Pprec_label)
		ip->Pprec_label = gaintool_StatusPanel_Pprec_label_create(ip, ip->PlayStatusPanel);
	if (!ip->Pprec_value)
		ip->Pprec_value = gaintool_StatusPanel_Pprec_value_create(ip, ip->PlayStatusPanel);
	if (!ip->Pencode_label)
		ip->Pencode_label = gaintool_StatusPanel_Pencode_label_create(ip, ip->PlayStatusPanel);
	if (!ip->Pencode_value)
		ip->Pencode_value = gaintool_StatusPanel_Pencode_value_create(ip, ip->PlayStatusPanel);
	if (!ip->RecStatusPanel)
		ip->RecStatusPanel = gaintool_StatusPanel_RecStatusPanel_create(ip, ip->StatusPanel);
	if (!ip->Rpanel_label)
		ip->Rpanel_label = gaintool_StatusPanel_Rpanel_label_create(ip, ip->RecStatusPanel);
	if (!ip->Ropen_flag)
		ip->Ropen_flag = gaintool_StatusPanel_Ropen_flag_create(ip, ip->RecStatusPanel);
	if (!ip->Rpaused_flag)
		ip->Rpaused_flag = gaintool_StatusPanel_Rpaused_flag_create(ip, ip->RecStatusPanel);
	if (!ip->Ractive_flag)
		ip->Ractive_flag = gaintool_StatusPanel_Ractive_flag_create(ip, ip->RecStatusPanel);
	if (!ip->Rerror_flag)
		ip->Rerror_flag = gaintool_StatusPanel_Rerror_flag_create(ip, ip->RecStatusPanel);
	if (!ip->Rwaiting_flag)
		ip->Rwaiting_flag = gaintool_StatusPanel_Rwaiting_flag_create(ip, ip->RecStatusPanel);
	if (!ip->Rsam_label)
		ip->Rsam_label = gaintool_StatusPanel_Rsam_label_create(ip, ip->RecStatusPanel);
	if (!ip->Rsam_value)
		ip->Rsam_value = gaintool_StatusPanel_Rsam_value_create(ip, ip->RecStatusPanel);
	if (!ip->Rrate_label)
		ip->Rrate_label = gaintool_StatusPanel_Rrate_label_create(ip, ip->RecStatusPanel);
	if (!ip->Rrate_value)
		ip->Rrate_value = gaintool_StatusPanel_Rrate_value_create(ip, ip->RecStatusPanel);
	if (!ip->Rchan_label)
		ip->Rchan_label = gaintool_StatusPanel_Rchan_label_create(ip, ip->RecStatusPanel);
	if (!ip->Rchan_value)
		ip->Rchan_value = gaintool_StatusPanel_Rchan_value_create(ip, ip->RecStatusPanel);
	if (!ip->Rprec_label)
		ip->Rprec_label = gaintool_StatusPanel_Rprec_label_create(ip, ip->RecStatusPanel);
	if (!ip->Rprec_value)
		ip->Rprec_value = gaintool_StatusPanel_Rprec_value_create(ip, ip->RecStatusPanel);
	if (!ip->Rencode_label)
		ip->Rencode_label = gaintool_StatusPanel_Rencode_label_create(ip, ip->RecStatusPanel);
	if (!ip->Rencode_value)
		ip->Rencode_value = gaintool_StatusPanel_Rencode_value_create(ip, ip->RecStatusPanel);
	if (!ip->StatusControls)
		ip->StatusControls = gaintool_StatusPanel_StatusControls_create(ip, ip->StatusPanel);
	if (!ip->Update_switch)
		ip->Update_switch = gaintool_StatusPanel_Update_switch_create(ip, ip->StatusControls);
	return ip;
}

/*
 * Create object `StatusPanel' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_StatusPanel_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	extern void	StatusPanel_done_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, FRAME_CMD,
		XV_KEY_DATA, INSTANCE, ip,
		XV_WIDTH, 379,
		XV_HEIGHT, 308,
		XV_LABEL, "Audio Device Status",
		XV_SHOW, FALSE,
		FRAME_SHOW_FOOTER, FALSE,
		FRAME_SHOW_RESIZE_CORNER, FALSE,
		FRAME_CMD_PUSHPIN_IN, TRUE,
		FRAME_DONE_PROC, StatusPanel_done_proc,
		NULL);
	xv_set(xv_get(obj, FRAME_CMD_PANEL), WIN_SHOW, FALSE, NULL);
	return obj;
}

/*
 * Create object `PlayStatusPanel' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_PlayStatusPanel_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "gaintool:PlayStatusPanel",
		XV_X, 0,
		XV_Y, 0,
		XV_WIDTH, 190,
		XV_HEIGHT, 274,
		WIN_BORDER, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `Ppanel_label' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Ppanel_label_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 57,
		XV_Y, 10,
		PANEL_LABEL_STRING, "Play Status",
		PANEL_LABEL_BOLD, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `Popen_flag' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Popen_flag_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	extern void		StatusPanel_null_event_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TOGGLE, PANEL_FEEDBACK, PANEL_MARKED,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 10,
		XV_Y, 33,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_EVENT_PROC, StatusPanel_null_event_proc,
		PANEL_CHOICE_STRING, 0, "Open",
		PANEL_VALUE, 0,
		NULL);
	return obj;
}

/*
 * Create object `Ppaused_flag' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Ppaused_flag_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	extern void		StatusPanel_null_event_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TOGGLE, PANEL_FEEDBACK, PANEL_MARKED,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 10,
		XV_Y, 52,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_EVENT_PROC, StatusPanel_null_event_proc,
		PANEL_CHOICE_STRING, 0, "Paused",
		PANEL_VALUE, 0,
		NULL);
	return obj;
}

/*
 * Create object `Pactive_flag' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Pactive_flag_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	extern void		StatusPanel_null_event_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TOGGLE, PANEL_FEEDBACK, PANEL_MARKED,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 10,
		XV_Y, 81,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_EVENT_PROC, StatusPanel_null_event_proc,
		PANEL_CHOICE_STRING, 0, "Active",
		PANEL_VALUE, 0,
		NULL);
	return obj;
}

/*
 * Create object `Perror_flag' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Perror_flag_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	extern void		StatusPanel_null_event_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TOGGLE, PANEL_FEEDBACK, PANEL_MARKED,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 10,
		XV_Y, 100,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_EVENT_PROC, StatusPanel_null_event_proc,
		PANEL_CHOICE_STRING, 0, "Underflow",
		PANEL_VALUE, 0,
		NULL);
	return obj;
}

/*
 * Create object `Pwaiting_flag' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Pwaiting_flag_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	extern void		StatusPanel_null_event_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TOGGLE, PANEL_FEEDBACK, PANEL_MARKED,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 10,
		XV_Y, 119,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_EVENT_PROC, StatusPanel_null_event_proc,
		PANEL_CHOICE_STRING, 0, "Open-Waiting",
		PANEL_VALUE, 0,
		NULL);
	return obj;
}

/*
 * Create object `Peof_label' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Peof_label_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 21,
		XV_Y, 152,
		PANEL_LABEL_STRING, "EOF Count:",
		PANEL_LABEL_BOLD, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `Peof_value' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Peof_value_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 98,
		XV_Y, 152,
		PANEL_LABEL_STRING, " ",
		PANEL_LABEL_BOLD, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `Psam_label' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Psam_label_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 35,
		XV_Y, 169,
		PANEL_LABEL_STRING, "Samples:",
		PANEL_LABEL_BOLD, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `Psam_value' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Psam_value_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 98,
		XV_Y, 169,
		PANEL_LABEL_STRING, " 0000000000",
		PANEL_LABEL_BOLD, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `Prate_label' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Prate_label_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 10,
		XV_Y, 198,
		PANEL_LABEL_STRING, "Sample Rate:",
		PANEL_LABEL_BOLD, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `Prate_value' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Prate_value_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 98,
		XV_Y, 198,
		PANEL_LABEL_STRING, " ",
		PANEL_LABEL_BOLD, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `Pchan_label' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Pchan_label_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 29,
		XV_Y, 215,
		PANEL_LABEL_STRING, "Channels:",
		PANEL_LABEL_BOLD, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `Pchan_value' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Pchan_value_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 98,
		XV_Y, 215,
		PANEL_LABEL_STRING, " ",
		PANEL_LABEL_BOLD, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `Pprec_label' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Pprec_label_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 31,
		XV_Y, 232,
		PANEL_LABEL_STRING, "Precision:",
		PANEL_LABEL_BOLD, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `Pprec_value' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Pprec_value_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 98,
		XV_Y, 232,
		PANEL_LABEL_STRING, " ",
		PANEL_LABEL_BOLD, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `Pencode_label' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Pencode_label_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 31,
		XV_Y, 249,
		PANEL_LABEL_STRING, "Encoding:",
		PANEL_LABEL_BOLD, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `Pencode_value' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Pencode_value_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 98,
		XV_Y, 249,
		PANEL_LABEL_STRING, " ",
		PANEL_LABEL_BOLD, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `RecStatusPanel' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_RecStatusPanel_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "gaintool:RecStatusPanel",
		XV_X, 189,
		XV_Y, 0,
		XV_WIDTH, WIN_EXTEND_TO_EDGE,
		XV_HEIGHT, 274,
		WIN_BORDER, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `Rpanel_label' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Rpanel_label_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 45,
		XV_Y, 10,
		PANEL_LABEL_STRING, "Record Status",
		PANEL_LABEL_BOLD, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `Ropen_flag' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Ropen_flag_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	extern void		StatusPanel_null_event_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TOGGLE, PANEL_FEEDBACK, PANEL_MARKED,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 10,
		XV_Y, 33,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_EVENT_PROC, StatusPanel_null_event_proc,
		PANEL_CHOICE_STRING, 0, "Open",
		PANEL_VALUE, 0,
		NULL);
	return obj;
}

/*
 * Create object `Rpaused_flag' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Rpaused_flag_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	extern void		StatusPanel_null_event_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TOGGLE, PANEL_FEEDBACK, PANEL_MARKED,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 10,
		XV_Y, 52,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_EVENT_PROC, StatusPanel_null_event_proc,
		PANEL_CHOICE_STRING, 0, "Paused",
		PANEL_VALUE, 0,
		NULL);
	return obj;
}

/*
 * Create object `Ractive_flag' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Ractive_flag_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	extern void		StatusPanel_null_event_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TOGGLE, PANEL_FEEDBACK, PANEL_MARKED,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 10,
		XV_Y, 81,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_EVENT_PROC, StatusPanel_null_event_proc,
		PANEL_CHOICE_STRING, 0, "Active",
		PANEL_VALUE, 0,
		NULL);
	return obj;
}

/*
 * Create object `Rerror_flag' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Rerror_flag_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	extern void		StatusPanel_null_event_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TOGGLE, PANEL_FEEDBACK, PANEL_MARKED,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 10,
		XV_Y, 100,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_EVENT_PROC, StatusPanel_null_event_proc,
		PANEL_CHOICE_STRING, 0, "Overflow",
		PANEL_VALUE, 0,
		NULL);
	return obj;
}

/*
 * Create object `Rwaiting_flag' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Rwaiting_flag_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	extern void		StatusPanel_null_event_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TOGGLE, PANEL_FEEDBACK, PANEL_MARKED,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 10,
		XV_Y, 119,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_EVENT_PROC, StatusPanel_null_event_proc,
		PANEL_CHOICE_STRING, 0, "Open-Waiting",
		PANEL_VALUE, 0,
		NULL);
	return obj;
}

/*
 * Create object `Rsam_label' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Rsam_label_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 35,
		XV_Y, 169,
		PANEL_LABEL_STRING, "Samples:",
		PANEL_LABEL_BOLD, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `Rsam_value' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Rsam_value_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 98,
		XV_Y, 169,
		PANEL_LABEL_STRING, " 0000000000",
		PANEL_LABEL_BOLD, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `Rrate_label' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Rrate_label_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 10,
		XV_Y, 198,
		PANEL_LABEL_STRING, "Sample Rate:",
		PANEL_LABEL_BOLD, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `Rrate_value' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Rrate_value_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 98,
		XV_Y, 198,
		PANEL_LABEL_STRING, " ",
		PANEL_LABEL_BOLD, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `Rchan_label' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Rchan_label_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 29,
		XV_Y, 215,
		PANEL_LABEL_STRING, "Channels:",
		PANEL_LABEL_BOLD, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `Rchan_value' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Rchan_value_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 98,
		XV_Y, 215,
		PANEL_LABEL_STRING, " ",
		PANEL_LABEL_BOLD, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `Rprec_label' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Rprec_label_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 31,
		XV_Y, 232,
		PANEL_LABEL_STRING, "Precision:",
		PANEL_LABEL_BOLD, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `Rprec_value' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Rprec_value_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 98,
		XV_Y, 232,
		PANEL_LABEL_STRING, " ",
		PANEL_LABEL_BOLD, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `Rencode_label' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Rencode_label_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 31,
		XV_Y, 249,
		PANEL_LABEL_STRING, "Encoding:",
		PANEL_LABEL_BOLD, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `Rencode_value' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Rencode_value_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 98,
		XV_Y, 249,
		PANEL_LABEL_STRING, " ",
		PANEL_LABEL_BOLD, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `StatusControls' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_StatusControls_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "gaintool:StatusControls",
		XV_X, 0,
		XV_Y, 273,
		XV_WIDTH, WIN_EXTEND_TO_EDGE,
		XV_HEIGHT, WIN_EXTEND_TO_EDGE,
		WIN_BORDER, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `Update_switch' in the specified instance.
 */
Xv_opaque
gaintool_StatusPanel_Update_switch_create(ip, owner)
	gaintool_StatusPanel_objects	*ip;
	Xv_opaque	owner;
{
	extern void		StatusPanel_update_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_CHOICE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 45,
		XV_Y, 6,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_CHOOSE_NONE, FALSE,
		PANEL_LABEL_STRING, "Update Mode:",
		PANEL_NOTIFY_PROC, StatusPanel_update_proc,
		PANEL_CHOICE_STRINGS,
			"Status Change",
			"Continuous",
			NULL,
		NULL);
	return obj;
}

