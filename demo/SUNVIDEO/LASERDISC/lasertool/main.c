#ifndef lint
static	char sccsid[] = "@(#)main.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <sunwindow/notify.h>
#include <suntool/alert.h>
#include <video.h>
#include "sony_laser.h"
#include "lasertool.h"

static short icon_image[] = {
#include "lasertool.icon"
};
mpr_static(icon_pixrect, 64, 64, 1, icon_image);

FILE *device;
Window base_frame, video_window;
Panel cycle_panel, button_panel, search_etc_panel;
Panel info_panel;
Icon laser_icon;

main(argc,argv)
int argc;
char **argv;
{
	extern void info_create(), button_create(), cycle_create();
	extern void search_etc_create(), alrm();
	extern int poll_fd, poll_state;
	extern int ch_go, disk_go, play_mode; 
	static Notify_value catch_intr(), poll();
	caddr_t frame_quit();
	void init_player();
	struct itimerval laser_timer;
	int no_video_window = 0;

	laser_icon = icon_create(ICON_IMAGE, &icon_pixrect, 0);

	base_frame = window_create( (Window)NULL, FRAME,
			WIN_Y, 0,
			WIN_X, 400,
			FRAME_LABEL, "LaserTool",
			FRAME_ARGC_PTR_ARGV, &argc, argv,
			FRAME_ICON, laser_icon,
			0);

	if(!base_frame) {
		fprintf(stderr,"lasertool : Must be in windows to run\n");
		exit(1);
	}

	argc--;
	argv++;
	while(argc && (**argv == '-')) {
		switch(*++*argv) { case 'n':
			no_video_window++;
			break;
		default :
			fprintf(stderr,"vid_dither : unknown flag\n");
			exit(1);
		}
		argc--;
		argv++;
	}

	if(argc == 1) {	
		if((device = (FILE *)sony_open(*argv)) == NULL) {
			fprintf(stderr,"Could not open serial line to laser disk\n");
			exit(1);
		}
	} else if(argc == 0) {
		if((device = (FILE *)sony_open((char *)NULL)) == NULL) {
			fprintf(stderr,"Could not open serial line to laser disk\n");
			exit(1);
		}
	} else {
		fprintf(stderr,"Usage : lasertool [-n] device\n");
		exit(1);
	} 
	

	/* 
	 * Initialise the disk player
	 */
	init_player();

	/*
	 * Create the video window, set i/p NTSC, sync NTSC,
	 * window size 640*480
	 */
	if(!no_video_window) {
		video_window = window_create(base_frame, VIDEO,
				WIN_WIDTH, 640,
				WIN_HEIGHT, 480,
				VIDEO_LIVE, TRUE,
				VIDEO_INPUT, VIDEO_NTSC,
				VIDEO_SYNC, VIDEO_NTSC,
				VIDEO_COMPRESSION, VIDEO_1X,
				VIDEO_DEMOD, VIDEO_DEMOD_AUTO,
				VIDEO_OUTPUT, VIDEO_ONSCREEN,
				0);
		if(!video_window) {
			fprintf(stderr,"Video windows not supported, ");
			fprintf(stderr,"using non-video window lasertool instead\n");
		}
	}

	cycle_panel = window_create(base_frame, PANEL, 
			WIN_WIDTH, 640, 
			WIN_HEIGHT, 180,
			0);
	if(video_window) {
		window_set(cycle_panel, 
			WIN_BELOW, video_window, 
			WIN_X, 0, 
			0);
	}

	info_panel = window_create(base_frame, PANEL,
			WIN_BELOW, cycle_panel,
			WIN_X, 0,
			WIN_WIDTH, 200,
			WIN_HEIGHT, 180,
			0);

	search_etc_panel = window_create(base_frame, PANEL, 
			WIN_WIDTH, 215,
			WIN_HEIGHT, 180,
			0);

	button_panel = window_create(base_frame, PANEL, 
			WIN_WIDTH, 215,
			WIN_HEIGHT, 180,
			0);

	play_mode = FWD;
	cycle_create();
	info_create();
	search_etc_create();
	button_create();
	window_fit(base_frame);

	/*
	 * Poll timer for checking completion status
	 * address, chapter and disk type
	 */
	laser_timer.it_interval.tv_usec = 4000;
	laser_timer.it_interval.tv_sec = 0;
	laser_timer.it_value.tv_usec = 4000;
	laser_timer.it_value.tv_sec = 0;
	poll_fd = fileno(device);
	poll_state = POLL_PRINT;
	ch_go = 0;
	disk_go = 0;
	notify_set_itimer_func((Notify_client)main, poll, ITIMER_REAL,
		&laser_timer, 0);

	/*
	 * Catch interrupts and clear laser disk using quit
	 */
	(void) notify_set_signal_func((Notify_client)main, catch_intr, 
		SIGINT, NOTIFY_ASYNC);

	/*
	 * Set the Frame menu quit function to use
	 * our own quit procedure
	 */
	menu_set( (Menu)menu_find( (Menu)window_get(base_frame, WIN_MENU),
				MENU_STRING, "Quit", 0),
					MENU_ACTION_PROC,
					 frame_quit, 0);

	window_main_loop(base_frame);
	exit(0);
} 

/*
 * Catch interrupts and clear laser disk 
 */
static Notify_value
catch_intr(me, signal, when)
Notify_client me;
int signal;
Notify_signal_mode when;
{
	sony_clear_all(device);
	sony_close(device);
	exit(0);
}

/*
 * Interpose frame quit action
 */
caddr_t
frame_quit(menu, menu_item)
Menu menu;
Menu_item menu_item;
{
	int result;

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

/*
 * set the initial disk player settings
 */
void
init_player()
{
	sony_clear_all(device);
	sony_motor_on(device);
	sony_frame_mode(device);
	sony_index_off(device);
	sony_audio_mute_on(device);
	sony_video_on(device);
	sony_index_off(device);
}
