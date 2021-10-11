#ifndef lint
static	char sccsid[] = "@(#)cycle_proc.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <video.h>
#include "sony_laser.h"
#include "lasertool.h"

extern Window video_window;
extern Panel_item Forward, Reverse, error_return, cycle_speed;
extern Panel_item speed, trk_jmp, Rate;
extern FILE *device;
extern int play_mode, poll_state;

void
dir_mode_proc(direction, value)
int direction, value;
{
	static char string[80];
	unsigned char rc, *rate, *trak;
	extern Pixrect fwd_on_button_pixrect, fwd_button_pixrect;
	extern Pixrect rev_on_button_pixrect, rev_button_pixrect;

	if(direction == FWD) {
		play_mode = FWD;
		panel_set(Forward, PANEL_LABEL_IMAGE, 
			&fwd_on_button_pixrect, 0);
		panel_set(Reverse, PANEL_LABEL_IMAGE, 
			&rev_button_pixrect, 0);
		switch(value) {
		case 0 :
			if((rc = sony_f_play(device)) == NAK) {
				sprintf(string,"play fwd : Command not acknowledged by LDP\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			}
			break;
		case 1 :
			if((rc = sony_f_scan(device)) == NAK) {
				sprintf(string,"scan fwd : Command not acknowledged by LDP\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			}
			break;
		case 2 :
			if((rc = sony_f_fast(device)) == CLV_DISK) {
				sprintf(string,"fast fwd : Invalid command for CLV disks\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			} else if (rc == NAK) {
				sprintf(string,"fast fwd : Command not acknowledged by LDP\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			}
			break;
		case 3 :
			if((rc = sony_f_slow(device)) == CLV_DISK) {
				sprintf(string,"slow fwd : Invalid command for CLV disks\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			} else if (rc == NAK) {
				sprintf(string,"slow fwd : Command not acknowledged by LDP\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			}
			break;
		case 4 :
			if((rc = sony_f_step_still(device)) == CLV_DISK) {
				sprintf(string,"step still fwd : Invalid command for CLV disks\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			} else if (rc == NAK) {
				sprintf(string,"step still fwd : Command not acknowledged by LDP\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			}
			break;
		case 5 :
			rate = (unsigned char *)panel_get(Rate,PANEL_VALUE); 
			if(*rate == NULL) {
				sprintf(string,"Must specify a rate for fwd step\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			} else {
				if((rc=sony_f_step(device, rate)) == INVALID_SPEED) {
					sprintf(string,"Rate must be in the range 1 to 255\n");
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
				} else if (rc == CLV_DISK) {
					sprintf(string,"step fwd : Invalid command for CLV disks\n");
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
				} else if (rc == NAK) {
					sprintf(string,"step fwd : Command not acknowledged by LDP\n");
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
				}
			}
			break;
		case 6 :
			trak = (unsigned char *)panel_get(trk_jmp,PANEL_VALUE); 
			if(*trak == NULL) {
				sprintf(string,"Must specify a track for fwd track jump\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			} else {
				if((rc = sony_f_track_jump(device, trak)) == NAK) {
					sprintf(string,"Tracks must be in range 1 to 255\n");
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
				} else if (rc == CLV_DISK) {
					sprintf(string,"trk jump fwd : Invalid command for CLV disks\n");
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
				} else if (rc == NAK) {
					sprintf(string,"trk fwd jump : Command not acknowledged by LDP\n");
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
				}
			}
			break;
		}
	}
	if(direction == REV) {
		play_mode = REV;
		panel_set(Reverse, PANEL_LABEL_IMAGE, 
			&rev_on_button_pixrect, 0);
		panel_set(Forward, PANEL_LABEL_IMAGE, 
			&fwd_button_pixrect, 0);
		switch(value) {
		case 0 :
			if((rc = sony_r_play(device)) == CLV_DISK) {
				sprintf(string,"rev play : Invalid command for CLV disks\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			} else if (rc == NAK) {
				sprintf(string,"play rev : Command not acknowledged by LDP\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			}
			break;
		case 1 :
			if ((rc = sony_r_scan(device)) == NAK) {
				sprintf(string,"scan rev: Command not acknowledged by LDP\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			}
			break;
		case 2 :
			if((rc = sony_r_fast(device)) == CLV_DISK) {
				sprintf(string,"fast rev : Invalid command for CLV disks\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			} else if (rc == NAK) {
				sprintf(string,"fast rev : Command not acknowledged by LDP\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			}
			break;
		case 3 :
			if((rc = sony_r_slow(device)) == CLV_DISK) {
				sprintf(string,"slow rev : Invalid command for CLV disks\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			} else if (rc == NAK) {
				sprintf(string,"slow rev : Command not acknowledged by LDP\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			}
			break;
		case 4 :
			if((rc = sony_r_step_still(device)) == CLV_DISK) {
				sprintf(string,"step still rev : Invalid command for CLV disks\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			} else if (rc == NAK) {
				sprintf(string,"step still rev : Command not acknowledged by LDP\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			}
			break;
		case 5 :
			rate = (unsigned char *)panel_get(Rate,PANEL_VALUE); 
			if(*rate == NULL) {
				sprintf(string,"Must specify a rate for rev step\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			} else {
				if((rc=sony_r_step(device, rate)) == INVALID_SPEED) {
					sprintf(string,"Rate must be in the range 1 to 255\n");
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
				} else if (rc == CLV_DISK) {
					sprintf(string,"step rev : Invalid command for CLV disks\n");
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
				} else if (rc == NAK) {
					sprintf(string,"step rev : Command not acknowledged by LDP\n");
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
				}
			}
			break;
		case 6 :
			trak = (unsigned char *)panel_get(trk_jmp,PANEL_VALUE); 
			if(*trak == NULL) {
				sprintf(string,"Must specify a track for rev track jump\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			} else {
				if((rc = sony_r_track_jump(device, trak)) == NAK) {
					sprintf(string,"Tracks must be in range 1 to 255\n");
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
				} else if (rc == CLV_DISK) {
					sprintf(string,"trk jump rev : Invalid command for CLV disks\n");
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
				} else if (rc == NAK) {
					sprintf(string,"trk jump rev : Command not acknowledged by LDP\n");
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
				}
			}
			break;
		}
	}
	return;
}

audio_proc(item, value, event)
Panel_item item;
int value;
Event *event;
{
	switch(value) {
	case 0 :
		sony_audio_mute_on(device);	
		break;
	case 1 :
		sony_audio_mute_off(device);
		break;
	case 2 :
		sony_ch1_on(device);
		break;
	case 3 :
		sony_ch1_off(device);
		break;
	case 4 :
		sony_ch2_on(device);
		break;
	case 5 :
		sony_ch2_off(device);
		break;
	case 6 :
		sony_cx_on(device);
		break;
	case 7 :
		sony_cx_off(device);
		break;
	}
	return;
}

void
frame_proc(item, value, event)
Panel_item item;
int value;
Event *event;
{
	switch(value) {
	case 0 :
		sony_frame_mode(device);
		break;
	case 1 :
		sony_chapter_mode(device);
		break;
	}
	return;
}

void
index_proc(item, value, event)
Panel_item item;
int value;
Event *event;
{
	switch(value) {
	case 0 :
		sony_index_off(device);
		break;
	case 1 :
		sony_index_on(device);
		break;
	}
	return;
}

void
video_proc(item, value, event)
Panel_item item;
int value;
Event *event;
{
	switch(value) {
	case 0 :
		sony_video_on(device);
		break;
	case 1 :
		sony_video_off(device);
		break;
	}
	return;
}

void
eject_proc(item, value, event)
Panel_item item;
int value;
Event *event;
{
	switch(value) {
	case 0 :
		sony_eject_enable(device);
		break;
	case 1 :
		sony_eject_disable(device);
		break;
	case 2 :
		/* 
		 * Turn off the information display while 
		 * the disk is ejected
	 	 */
		poll_state = POLL_OFF;
		sony_eject(device);
		break;
	}
	return;
}

void
psc_proc(item, value, event)
Panel_item item;
int value;
Event *event;
{
	switch(value) {
	case 0 :
		sony_psc_enable(device);
		break;
	case 1 :
		sony_psc_disable(device);
		break;
	}
	return;
}

void
motor_proc(item, value, event)
Panel_item item;
int value;
Event *event;
{
	switch(value) {
	case 0 :
		sony_motor_on(device);
		/* Turn on the display again */
		poll_state = POLL_PRINT;
		break;
	case 1 :
		/* 
		 * Turn off the information display while 
		 * the motor is off 
	 	 */
		poll_state = POLL_OFF;
		sony_motor_off(device);
		break;
	}
	return;
}

void
disk_inq_proc(item, value, event)
Panel_item item;
int value;
Event *event;
{
	static char string[80];
	unsigned char player_stat[5], user_code[4], romv;

	switch(value) {
	case 0:
		sony_status_inq(device, player_stat);
		sprintf(string,"Player Status bytes 0=0x%x, 1=0x%x, 2=0x%x, 3=0x%x, 4=0x%x\n",
			player_stat[0],player_stat[1],player_stat[2],player_stat[3],
			player_stat[4]);
		panel_set(error_return, PANEL_LABEL_STRING, string,
			0);
		break;
	case 1:
		sony_ucode_inq(device, user_code);
		if(user_code[0] == NAK)
			sprintf(string,"Rom version not high enough\n");
		else
			sprintf(string,"Current user_code 0=0x%x, 1=0x%x, 2=0x%x, 3=0x%x\n",
				user_code[0],user_code[1],user_code[2],user_code[3]);
		panel_set(error_return, PANEL_LABEL_STRING, string,
			0);
		break;
	case 2: 
		sony_rom_version(device, &romv);
		sprintf(string,"Rom Version = 0x%x\n",romv);
		panel_set(error_return, PANEL_LABEL_STRING, string,
			0);
		break;
	}
	return;
}

void
mem_proc(item, value, event)
Panel_item item;
int value;
Event *event;
{
	switch(value) {
	case 0 :
		sony_memory(device);
		break;
	case 1 :
		window_set(video_window, VIDEO_LIVE, FALSE, 0);
		sony_m_search(device);
		window_set(video_window, VIDEO_LIVE, TRUE, 0);
		break;
	}
	return;
}
