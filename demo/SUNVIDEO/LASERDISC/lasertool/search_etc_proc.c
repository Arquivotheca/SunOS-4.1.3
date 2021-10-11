#ifndef lint
static	char sccsid[] = "@(#)search_etc_proc.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <sys/types.h>
#include <sys/time.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <video.h>
#include "sony_laser.h"
#include "lasertool.h"

extern Window video_window;
extern Panel_item ntimes, start_frame, stop_frame, speed, trk_jmp;
extern Panel_item error_return, address, search_etc_cycle;

extern Window base_frame;
extern FILE *device;
extern int poll_state;
extern char poll_ch;

/*
 * When the go button is hit do whatever was selected
 */
void
search_etc_proc(item, event)
Panel_item item;
Event *event;
{
	char string[80];
	unsigned char *sframe, *eframe, *loop, *rate, *trak, rc;
	register int value;

	value = (int)panel_get(search_etc_cycle, PANEL_VALUE);
	switch(value) {
	case 0 : /* Search */
		if(poll_state == POLL_PRINT) {
			poll_state = POLL_OFF;
			sframe = (unsigned char *)panel_get(start_frame,PANEL_VALUE); 
			if(*sframe == NULL) {
				sprintf(string,"Must specify start point for search\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			} else {
				if(video_window)
					window_set(video_window, VIDEO_LIVE, FALSE, 0);
				if((rc = sony_search(device, sframe)) != COMPLETION) { 
					sprintf(string,"Invalid search, 0x%x returned\n",rc);
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
				}
				if(video_window)		
					window_set(video_window, VIDEO_LIVE, TRUE, 0);
			}
			poll_state = POLL_PRINT;
		} else {
			sprintf(string,"Please clear_all first\n");
			panel_set(error_return, PANEL_LABEL_STRING, string, 0);
		}
		break;
	case 1 : /* Repeat from current frame to eframe, ntimes */
		sframe = (unsigned char *)panel_get(start_frame,PANEL_VALUE); 
		loop = (unsigned char *)panel_get(ntimes,PANEL_VALUE); 
		if(*sframe == NULL) {
			sprintf(string,"Must specify stop point for repeat\n");
			panel_set(error_return, PANEL_LABEL_STRING, string, 0);
		} else if(*loop == NULL) {
			sprintf(string,"Must specify a loop number for repeat\n");
			panel_set(error_return, PANEL_LABEL_STRING, string, 0);
		} else {
			if(poll_state == POLL_PRINT) {
				poll_state = POLL_OFF;
				if((rc = sony_repeat(device, sframe, loop, 0)) == ACK) {
					poll_state = POLL_SEARCH;
					poll_ch = COMPLETION;
				} else if(rc == CLV_DISK) {
					sprintf(string,"Invalid repeat for CLV disk\n");
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
					poll_state = POLL_PRINT;
					return;
				} else {
					sprintf(string,"Invalid repeat, 0x%x returned\n",rc);
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
					poll_state = POLL_PRINT;
					return;
				}		
			} else {
				sprintf(string,"Please clear_all first\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			}
		}
		break;
	case 2 : /* Replay frame sframe to eframe, ntimes */
		sframe = (unsigned char *)panel_get(start_frame,PANEL_VALUE); 
		eframe = (unsigned char *)panel_get(stop_frame,PANEL_VALUE); 
		loop = (unsigned char *)panel_get(ntimes,PANEL_VALUE); 
		if(*sframe == NULL) {
			sprintf(string,"Must specify start point for replay\n");
			panel_set(error_return, PANEL_LABEL_STRING, string, 0);
		}else if(*eframe == NULL) {
			sprintf(string,"Must specify end point for replay\n");
			panel_set(error_return, PANEL_LABEL_STRING, string, 0);
		} else if(*loop == NULL) {
			sprintf(string,"Must specify a loop number for replay\n");
			panel_set(error_return, PANEL_LABEL_STRING, string, 0);
		} else {
			if(poll_state == POLL_PRINT) {
				poll_state = POLL_OFF;
				if((rc = sony_replay_seq(device, sframe, eframe,loop,0))
						== ACK) {
					poll_state = POLL_SEARCH;
					poll_ch = COMPLETION;
				} else if(rc == CLV_DISK) {
					sprintf(string,"Invalid replay for CLV disk\n");
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
					poll_state = POLL_PRINT;
					return;
				} else {
					sprintf(string,"Invalid replay, 0x%x returned\n",rc);
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
					poll_state = POLL_PRINT;
					return;
				}
			} else {
				sprintf(string,"Please clear_all first\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			}
		}
		break;
	case 3 : /* Repeat from sframe to eframe, ntimes at speed rate */
		sframe = (unsigned char *)panel_get(start_frame,PANEL_VALUE); 
		eframe = (unsigned char *)panel_get(stop_frame,PANEL_VALUE); 
		loop = (unsigned char *)panel_get(ntimes,PANEL_VALUE); 
		rate = (unsigned char *)panel_get(speed,PANEL_VALUE); 
		if(*sframe == NULL) {
			sprintf(string,"Must specify start point for rep_vspeed\n");
			panel_set(error_return, PANEL_LABEL_STRING, string, 0);
		}else if(*eframe == NULL) {
			sprintf(string,"Must specify end point for rep_vspeed\n");
			panel_set(error_return, PANEL_LABEL_STRING, string, 0);
		} else if(*loop == NULL) {
			sprintf(string,"Must specify a loop number for rep_vspeed\n");
			panel_set(error_return, PANEL_LABEL_STRING, string, 0);
		} else if(*rate == NULL) {
			sprintf(string,"Must specify a rate for rep_vspeed\n");
			panel_set(error_return, PANEL_LABEL_STRING, string, 0);
		} else {
			if(poll_state == POLL_PRINT) {
				poll_state = POLL_OFF;
				rc = sony_replay_vspeed(device, sframe, eframe, loop, rate, 0);
				if(rc == CLV_DISK) {
					sprintf(string,"replay vspeed : Invalid command for CLV disks\n");
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
					return;
				} else if(rc == INVALID_SPEED) {
					sprintf(string,"Rate must be in the range 1 to 255\n");
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
				} else if(rc != ACK) {
					sprintf(string,"Invalid repvspeed, 0x%x returned\n",rc);
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
					return;
				}
				poll_state = POLL_SEARCH;
				poll_ch = COMPLETION;
			} else {
				sprintf(string,"Please clear_all first\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			}
		}
		break;
	case 4 : /* Set the mark frame to be sframe */
		if(poll_state == POLL_PRINT) {
			poll_state=POLL_OFF;	
			sframe = (unsigned char *)panel_get(start_frame,PANEL_VALUE); 
			if(*sframe == NULL) {
				sprintf(string,"Must specify start point for mark set\n");
				panel_set(error_return, PANEL_LABEL_STRING, string, 0);
			} else {
				if((rc = sony_mark_set(device, sframe)) != ACK) {
					sprintf(string,"Invalid mark set, 0x%x returned\n",rc);
					panel_set(error_return, PANEL_LABEL_STRING, string, 0);
				}
			}
			poll_state=POLL_PRINT;
		} else {
			sprintf(string,"Please clear_all first\n");
			panel_set(error_return, PANEL_LABEL_STRING, string, 0);
		}
		break;
	}
	return;
}
