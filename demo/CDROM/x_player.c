/***************************************************************
 *                                                           *
 *                     file: cdrom.c                         *
 *                                                           *
 *************************************************************/

/*
 * Copyright (C) 1988, 1989 Sun Microsystems, Inc.
 */

/*
 * This file contain the main module for the cd player demo program
 */

#ifndef lint
static char sccsid[] = "@(#)x_player.c 1.1 92/07/30 Copyr 1989, 1990 Sun Microsystem, Inc.";
#endif

#include <stdio.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/tty.h>
#include <xview/textsw.h>
#include <xview/canvas.h>
#include <xview/notice.h>
#include <xview/svrimage.h>
#include <xview/icon.h>
#include <xview/xv_xrect.h>
/*#include <gdd.h>*/


#include <sys/file.h>
#include <sys/types.h>
#include <sun/dkio.h>

#include <sys/time.h> 
#include <strings.h>

#ifdef sun4c
#include <scsi/scsi.h>
#include <scsi/targets/srdef.h>
#else
#include <sundev/srreg.h>
#endif

#undef min
#include "msf.h"
#include "toc.h"
#include "cdrom.h"
#include "player.h"
#include "prop_ui.h"

char	*Version = "1.0 Beta";

Attr_attribute	INSTANCE;
prop_prop_objects	*prop_objs;
int	prop_showing = 0;
int	time_remain_flag = 0;
int	last_guage_value = 0;
int	play_mode = 0;
int	idle_mode = 0;
int	idle_time = 180;
int	kbd_fd;
int	ms_fd;
int	mem_ntracks = 0;
int	mem_play[MAX_TRACKS];
int	mem_current_track;
struct	msf	mem_total_msf;
struct	msf	mem_cur_msf;

void	prop_init();

int debug = 0;
int repeating;
int cp_got_button_up = 1;


short player_image[] = {
#include "player.icon"
};

#define	CHOICE_STOP	0
#define CHOICE_PLAY	1
#define CHOICE_PAUSE	2
#define CHOICE_EJECT	3

#define EJECT_MESSAGE	"Ejected"

/* ifdef TRACK_CHOICE use PANEL_CHOICE items for tracks, else buttons */
#define TRACK_CHOICE	1

/* ifdef FUNCTION_GLYPHS use glyphs instead of text for function buttons */

#ifdef FUNCTION_GLYPHS
short stop_bits[] =  {
#include "stop.button"
};

short pause_bits[] =  {
#include "pause.button"
};

short eject_bits[] =  {
#include "eject.button"
};

short play_bits[] =  {
#include "play.button"
};

short rew_bits[] = {
#include "rew.button"
};

short ff_bits[] = {
#include "ff.button"
};

short cycle_bits[] = {
#include "cycle.button"
};
#endif

static	char *my_asctime();

struct tm 	player_tm;
struct tm	*tmp = &player_tm;
Msf	total_msf;
int	current_track;

Icon	player_icon;

int	fd;

/* state variables */
int	playing = 0;
int	paused = 0;
int	ejected = 0;
int	idled = 0;
int	rewinding = 0;
int	forwarding = 0;

static	int	playing_track = -1;
static  int	starting_track = -1;

static	void	create_panel_items();
static	void	create_panel1();
static	void	create_panel2();
static	void	create_panel3();

static	void	play_track_proc();
static	void	pick_track_proc();
static	void	stop_proc();
static	void	eject_proc();
void	pause_proc();
static	void	play_proc();
static	void	choice_notify();
static	void	rew_proc();
static	void	ff_proc();
static	void	cycle_proc();
static	void	close_proc();
static	void	volume_proc();
void	set_volume();

static	int	read_toc_alert();
static	void	clear_buttons();

static	void	reset_button();
static	void	reset_all_buttons();
static	Notify_value clock_itimer_expired();
static	void	clock_init_tmp();
int	playing_update_tmp();
static	Toc	read_disc_toc();
static	void	reverse_button();
static	void	reverse_pause_button();
static	void	reset_pause_button();
static	void	reverse_play_button();
static	void	reset_play_button();
static	void	mode_select_page_eight();
void	init_player();
static	void	display_track();
static	void	display_msf();
static	int	end_audio_track();
static	void	generic_alert();

static	void	fit_panels();
static	void	panel_events();
void	prop_notify();

Panel	panel;
static	Panel	panel1;
static	Panel	panel2;
static 	Panel	panel3;
static	Frame	frame;
static	Menu	prop_menu;

static	int clock_client;
static  char time_str[128];
int		num_tracks;	
Panel_item	track_items[MAX_TRACKS];
static	int		item_reversed[MAX_TRACKS];

Panel_item	msg_item;
static 	Panel_item	track_item;

static	Panel_item	play_choice;
static	Panel_item	rew_item;
static	Panel_item	ff_item;
static	Panel_item	cycle_item;

#ifdef TRACK_CHOICE
static	Panel_item	track_choice;
#endif

Toc	toc;

#define DEV_NAME	"/dev/rsr0"
char	*dev_name = DEV_NAME;

#define KBD_DEV_NAME	"/dev/kbd"
char	*kbd_dev = KBD_DEV_NAME;

#define MS_DEV_NAME	"/dev/mouse"
char	*ms_dev = MS_DEV_NAME;

static	int	volume = 9;
int	left_balance = 10;
int	right_balance = 10;

int	rew_delta = -1;
int	ff_delta = 1;

static	int	data_disc = 0;
int	nx = 0;
char	*ProgName;

#define	BUFFER_SIZE	0x400

main(argc, argv, envp)
	int	argc;
	char	**argv;
	char *envp[];
{
	int	i, err = 0;
	Server_image	icon_image;

	ProgName = *argv;

	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, 0);

	INSTANCE = xv_unique_key();

	for (i = 1; i<argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'd': 
				i++;
				dev_name = argv[i];
				break;
			case 'q':
				debug = 0;
				break;
			case 'x':
				i++;
				sscanf(argv[i],"%d",&debug);
				break;
			case 'v':
				i++;
				sscanf(argv[i],"%d",&volume);
				break;
			case 'n':
				nx = 1;
				break;
			      default:
				fprintf(stderr, 
					"%c: unknown switch\n", argv[i][1]);
				err++;
			}
		}
	}

	if (err) {
	    fprintf(stderr,"\nUsage:\n");
	    fprintf(stderr,"%s: [-d dev] [-v volume] [-x debug | -q] [-n]\n",
		ProgName);
	    fprintf(stderr,"\n-d dev\t\tdevice file name\n");
	    fprintf(stderr,"-v volume\tPreset volume\n");
	    fprintf(stderr,"-x debug\tDebug Level, -q = quiet\n");
	    fprintf(stderr,"-n\t\tUse non-exclusive device access\n");
	    fprintf(stderr,"\ncdplayer, Version %s, Robert C. Terzi\n\n",
		Version);

	    exit(-1);
	}

	cdrom_open();

	clock_init_tmp();
	
	icon_image = xv_create(NULL, SERVER_IMAGE,
		XV_WIDTH, 64,
		XV_HEIGHT, 38, 
		SERVER_IMAGE_DEPTH, 1, 
		SERVER_IMAGE_BITS, player_image, 
		0);

	player_icon = xv_create(NULL, ICON,
		XV_WIDTH,	 64,
		XV_HEIGHT, 38,
		ICON_IMAGE, icon_image,
		0);

	frame = xv_create(NULL, FRAME, XV_LABEL, "CD Player",
		FRAME_SHOW_RESIZE_CORNER, FALSE,
		FRAME_ICON, player_icon,
		0);

	panel = xv_create(frame, PANEL,
		WIN_ROW_HEIGHT, 16,
		WIN_COLUMN_WIDTH, 17,
		WIN_COLUMN_GAP, 19,
		WIN_ROW_GAP, 4,
		WIN_ROWS, 5,
		WIN_COLUMNS, 5,
		0);

	xv_set(canvas_paint_window(panel),
		WIN_EVENT_PROC,	panel_events,
		WIN_CONSUME_EVENTS,
			WIN_MOUSE_BUTTONS,
			0,
		0);

	prop_menu = (Menu) xv_create(NULL, MENU,
		MENU_TITLE_ITEM, "CD Player",
		MENU_STRINGS, "Properties...", NULL,
		MENU_NOTIFY_PROC, prop_notify,
		0);

	prop_objs = prop_prop_objects_initialize(NULL, frame);

	prop_init();

	if ((kbd_fd = open(kbd_dev, O_RDONLY)) < 0) {
		fprintf(stderr,"%s: idle disabled - open of keyboard %s failed",
			ProgName, kbd_dev);
		perror("");
		idle_mode = 0;

		xv_set(prop_objs->idle_choice, 
			PANEL_INACTIVE, TRUE,
			PANEL_VALUE, idle_mode,
			0);

		xv_set(prop_objs->idle_item,
			PANEL_INACTIVE, TRUE,
			0);
	}

	if ((ms_fd = open(ms_dev, O_RDONLY)) < 0) {
		fprintf(stderr,"%s: idle disabled - open of mouse %s failed",
			ProgName, ms_dev);
		perror("");
		idle_mode = 0;

		xv_set(prop_objs->idle_choice,
			PANEL_INACTIVE, TRUE,
			PANEL_VALUE, idle_mode, 
			0);
		
		xv_set(prop_objs->idle_item, 
			PANEL_INACTIVE, TRUE,
			0);
	}

	if (!ejected) {  /* no disc in drive, open failed */

		if ((toc = read_disc_toc()) == (Toc)NULL) {
			perror();
			exit(1);
		}

		num_tracks = toc->size;
		create_panel_items(panel);
			/* window_fit_height(panel); */
	}
	else { 	
		/*
		 * do proper init away.  Other panels depend upon
		 * the width and height of this panel, just create
		 * some fake buttons 
		 */

		num_tracks = 15;
		create_panel_items(panel); 
		window_fit_height(panel);  
		xv_set(prop_objs->mode_choice, 
			PANEL_INACTIVE, TRUE, 
			0);
		
		xv_set(prop_objs->mem_list,
			PANEL_READ_ONLY, TRUE,
			0);
	}

	create_panel1(frame);

	create_panel3(frame);

	create_panel2(frame);

	fit_panels();

	if (ejected) {
		/* It seems to be more reliable to delete the buttons
		 * after the frame has been sized
		 */
		xv_set(play_choice, PANEL_VALUE, CHOICE_EJECT, 0);
		xv_set(msg_item, PANEL_LABEL_STRING, EJECT_MESSAGE, 0);
		clear_buttons();
	}
	else {
		/*
		 * check to see whether a disc is already playing
		 * if not ejected.  
		 */
		if (cdrom_playing(fd, &current_track)) {
			DPRINTF(2)("%s: already playing track %d\n", ProgName,
			current_track);
			playing_track = current_track;
			playing = 1;
			reverse_play_button();
			/* if (cdrom_paused(fd, &current_track))
				pause_proc(NULL, NULL);
			*/
			clock_init_tmp();
			clock_itimer_expired(&clock_client, ITIMER_REAL);
		}
		else {
			display_msf(total_msf);
			display_track(0);
		}
	}

	window_main_loop(frame);

	exit(0); 
	/* NOTREACHED */
}

static void
create_panel1(frame)
	Frame	frame;
{
#ifdef FUNCTION_GLYPHS
	Server_image	stop_image, play_image, pause_image, eject_image;
	Server_image	rew_image, ff_image, cycle_image;
#endif

	panel1 = xv_create(frame, PANEL,
		WIN_X, 0,
		WIN_BELOW, panel,
		XV_WIDTH, xv_get(panel, XV_WIDTH),
		0);

	xv_set(canvas_paint_window(panel1),
		WIN_EVENT_PROC,	panel_events,
		WIN_CONSUME_EVENTS, 
			WIN_MOUSE_BUTTONS, 
			LOC_WINEXIT,
			0,
		0);

	DPRINTF(5)("create_panel1: width of panel %d\n", xv_get(panel,
	XV_WIDTH));

#ifndef FUNCTION_GLYPHS
	play_choice = xv_create(panel1, PANEL_CHOICE,
		PANEL_LABEL_WIDTH, 34,
		PANEL_CHOICE_STRINGS,
			"Stop",
			"Play", 
			"Pause", 
			"Eject", 
			NULL,
		PANEL_NOTIFY_PROC, choice_notify,
		0);
#else
    stop_image = (Server_image) xv_create(NULL, SERVER_IMAGE,
		XV_WIDTH, 32,
		XV_HEIGHT, 16,
		SERVER_IMAGE_DEPTH, 1,
		SERVER_IMAGE_BITS, stop_bits,
		NULL);

    eject_image = (Server_image) xv_create(NULL, SERVER_IMAGE,
		XV_WIDTH, 32,
		XV_HEIGHT, 16,
		SERVER_IMAGE_DEPTH, 1,
		SERVER_IMAGE_BITS, eject_bits,
		NULL);

    pause_image = (Server_image) xv_create(NULL, SERVER_IMAGE,
		XV_WIDTH, 32,
		XV_HEIGHT, 16,
		SERVER_IMAGE_DEPTH, 1,
		SERVER_IMAGE_BITS, pause_bits,
		NULL);

    play_image = (Server_image) xv_create(NULL, SERVER_IMAGE,
		XV_WIDTH, 32,
		XV_HEIGHT, 16,
		SERVER_IMAGE_DEPTH, 1,
		SERVER_IMAGE_BITS, play_bits,
		NULL);

    rew_image = (Server_image) xv_create(NULL, SERVER_IMAGE,
		XV_WIDTH, 32,
		XV_HEIGHT, 16,
		SERVER_IMAGE_DEPTH, 1,
		SERVER_IMAGE_BITS, rew_bits,
		NULL);

    ff_image = (Server_image) xv_create(NULL, SERVER_IMAGE,
		XV_WIDTH, 32,
		XV_HEIGHT, 16,
		SERVER_IMAGE_DEPTH, 1,
		SERVER_IMAGE_BITS, ff_bits,
		NULL);

    cycle_image = (Server_image) xv_create(NULL, SERVER_IMAGE,
		XV_WIDTH, 32,
		XV_HEIGHT, 16,
		SERVER_IMAGE_DEPTH, 1,
		SERVER_IMAGE_BITS, cycle_bits,
		NULL);

	play_choice = xv_create(panel1, PANEL_CHOICE,
		XV_X, 10,
		XV_Y, xv_row(panel1, 0),
		PANEL_CHOICE_IMAGES,
			stop_image, 
			play_image, 
			pause_image, 
			eject_image, 
			NULL,
		PANEL_NOTIFY_PROC, choice_notify,
		PANEL_LABEL_WIDTH, 34,
		0);

#endif

	rew_item = xv_create(panel1, PANEL_TOGGLE,
		XV_X, xv_cols(panel1, 3),
		XV_Y, xv_rows(panel1, 1),
#ifndef FUNCTION_GLYPHS
		PANEL_CHOICE_STRINGS, "rew", 0,
#else
		PANEL_CHOICE_IMAGES, rew_image, NULL,
#endif
		PANEL_EVENT_PROC, rew_proc,
		PANEL_LABEL_WIDTH, 36,
		0);

	cycle_item = xv_create(panel1, PANEL_TOGGLE,
		XV_Y, xv_rows(panel1, 1),
#ifndef FUNCTION_GLYPHS
		XV_X, xv_cols(panel1, 12),
		PANEL_CHOICE_STRINGS, "rpt ", 0,
#else
		XV_X, xv_cols(panel1, 9),
		PANEL_CHOICE_IMAGES, cycle_image, NULL,
#endif
		PANEL_EVENT_PROC, cycle_proc,
		XV_WIDTH, 36,
		0);

	ff_item = xv_create(panel1, PANEL_TOGGLE,
		XV_Y, xv_rows(panel1, 1),
#ifndef FUNCTION_GLYPHS
		XV_X, xv_cols(panel1, 12),
		PANEL_CHOICE_STRINGS, "f.f.", 0,
#else
		XV_X, xv_cols(panel1, 15),
		PANEL_CHOICE_IMAGES, ff_image, NULL,
#endif
		PANEL_EVENT_PROC, ff_proc,
		XV_WIDTH, 36,
		0);
}

static void
create_panel2(frame)
	Frame	frame;
{
	panel2 = xv_create(frame, PANEL,
		WIN_X, 0,
		/* WIN_BELOW, panel1, */
		WIN_COLUMN_WIDTH, 16,
		WIN_COLUMN_GAP, 15,
		/* WIN_COLUMNS, 5, */
		XV_WIDTH, xv_get(panel, XV_WIDTH),
		XV_HEIGHT, 15,
		0);

	xv_set(canvas_paint_window(panel2),
		WIN_EVENT_PROC,	panel_events,
		WIN_CONSUME_EVENTS, 
			WIN_MOUSE_BUTTONS, 
			0,
		0);

	msg_item = xv_create(panel2, PANEL_MESSAGE,
		XV_X, xv_cols(panel2, 0) + 2,
		XV_Y, xv_rows(panel2, 0),
		PANEL_LABEL_STRING, "00:00",
		PANEL_PAINT, PANEL_NO_CLEAR,
		0);

	track_item = xv_create(panel2, PANEL_MESSAGE,
		XV_X, xv_cols(panel2, 4),
		XV_Y, xv_rows(panel2, 0),
		PANEL_LABEL_STRING,
		"Track: --",
		PANEL_PAINT, PANEL_NO_CLEAR,
		0);
}	

static void
create_panel3(frame)
	Frame	frame;
{
	panel3 = xv_create(frame, PANEL,
		WIN_X, 0,
		/* WIN_BELOW, panel2, */
		/* WIN_COLUMN_WIDTH, 16,
		WIN_COLUMN_GAP, 17,
		WIN_COLUMNS, 1, */
		WIN_ROWS, 1,
		WIN_ROW_GAP, 0,
		XV_WIDTH, xv_get(panel, XV_WIDTH),
		0);


	xv_set(canvas_paint_window(panel3),
		WIN_EVENT_PROC,	panel_events,
		WIN_CONSUME_EVENTS, 
			WIN_MOUSE_BUTTONS, 
			0,
		0);

	xv_create(panel3, PANEL_SLIDER,
		XV_X, xv_cols(panel3,0) + 5,
		XV_Y, xv_rows(panel3,0),
		PANEL_VALUE, volume,
		PANEL_MIN_VALUE, 0,
		PANEL_MAX_VALUE, 11,
		PANEL_SLIDER_WIDTH, 70,
		PANEL_LABEL_STRING, "Vol",
		PANEL_SHOW_VALUE, FALSE,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_NOTIFY_PROC, volume_proc,
		PANEL_TICKS, 10,
		0);

}

static void
fit_panels()
{
    window_fit_height(panel);
    xv_set(panel1, WIN_BELOW, panel, 0);
    window_fit_height(panel1);
    xv_set(panel3, WIN_BELOW, panel1, 0);
    window_fit_height(panel3);
    xv_set(panel2, WIN_BELOW, panel3, 0);
    window_fit_height(panel2);

    window_fit(frame);
}


static void	
create_panel_items(panel)
	Panel	panel;
{
	int	i, index, x, y;
	char	str[4];
	char	**attr;

/*
 * XXX - should check for audio tracks
 * also should walk toc here - is it possible to have tracks not start at 1?
 */

#ifndef TRACK_CHOICE
	for (i = 1; i <= num_tracks; i++) {
		x = (i-1) % 5;
		y = (i-1) / 5;
		sprintf(str, "%02d", i);
		track_items[i] = xv_create(panel, PANEL_BUTTON,
			XV_X, xv_cols(panel, x) + 3,
			XV_Y, xv_rows(panel, y) + 5,
			PANEL_LABEL_STRING, str,
			PANEL_CLIENT_DATA, (caddr_t)i,
			PANEL_NOTIFY_PROC, pick_track_proc,
			0);

		item_reversed[i] = 0;
	}
#else
	for (i = 1; i <= num_tracks; i++) {
		x = (i-1) % 5;
		y = (i-1) / 5;
		sprintf(str, "%02d", i);
		track_items[i] = xv_create(panel, PANEL_TOGGLE,
			XV_X, xv_cols(panel, x) + 4,
			XV_Y, xv_rows(panel, y) + 5,
			PANEL_CHOICE_STRINGS, str, XV_NULL,
			PANEL_CLIENT_DATA, (caddr_t)i,
			PANEL_NOTIFY_PROC, pick_track_proc,
			0);

		item_reversed[i] = 0;

	}
#endif

/* attempted to make track_choice, one choice item
 * gave up on this because of %&$#@ flash when doing panel_toggle
 * now implemented as single choice toggle items
 */
#ifdef NOTDEF 
	if ((attr = (char **) malloc(sizeof(char *) * (num_tracks + 2)))
	== NULL) {
		perror("Malloc of track attributes failed");
		exit(-1);
	}

	track_choice = xv_create(panel, PANEL_TOGGLE, 
			XV_SHOW, FALSE,
			PANEL_NOTIFY_PROC, pick_track_proc,
			0);

	attr[0] = (char *) PANEL_CHOICE_STRINGS;
	for (i = 1, index = 1; i < num_tracks; i++) {
		sprintf(str, "%02d", i);
		attr[index++] = str;
	}
	attr[index++] = XV_NULL;
	attr[index] = XV_NULL;
	(void) xv_set(track_choice, ATTR_LIST, attr, 0);
	
	attr[0] = (char *) PANEL_CHOICE_XS;
	for (i = 1, index = 1; i < num_tracks; i++) {
		attr[index++] = (char *) (xv_cols(panel, ((i-1) % 5)) + 2);
	}
	attr[index++] = XV_NULL;
	attr[index] = XV_NULL;
	(void) xv_set(track_choice, ATTR_LIST, attr, 0);
	
	attr[0] = (char *) PANEL_CHOICE_YS;
	for (i = 1, index = 1; i < num_tracks; i++) {
		attr[index++] = (char *) (xv_rows(panel, ((i-1) / 5)) + 4);
	}
	attr[index++] = XV_NULL;
	attr[index] = XV_NULL;
	(void) xv_set(track_choice, ATTR_LIST, attr, 0);
	
	xv_set(track_choice, XV_SHOW, TRUE, 0);
	free(attr);
#endif
}				  
				  
/*ARGSUSED*/
static void
pick_track_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	int	track_number;
	Msf	msf;

	reset_all_buttons();
	track_number = (int) xv_get(item, PANEL_CLIENT_DATA);

	if (!playing) {
		display_track(track_number);
		if (play_mode != PLAY_MODE_NORMAL) {
			xv_set(item, PANEL_TOGGLE_VALUE, 0, FALSE, 0);
			add_memory_track(track_number, TRUE);
		}
		else {
		    reverse_button(track_number);
		    starting_track = track_number;
		}
	}
	else {
		if (play_mode == PLAY_MODE_NORMAL) {
		    starting_track = track_number;
		    reverse_button(track_number);
		}
		else {
		    starting_track = search_memlist(track_number);
		    if (starting_track == -1) {
		        add_memory_track(track_number, TRUE);
				xv_set(item, PANEL_TOGGLE_VALUE, 0, FALSE, 0);
				starting_track = track_number;
		    }
			else {
				starting_track = mem_play[starting_track];
		    }
		}
	}
	msf = get_track_duration(toc, track_number);
	display_msf(msf);
}

/*ARGSUSED*/
static
Notify_value
clock_itimer_expired(clock, which)
	Notify_client	clock;
	int		which;
{
	struct	itimerval itimer;
	struct	timeval c_tv;

	/* possible old itimers, since ejected, etc.*/
	if (!playing) return(NOTIFY_DONE);

        /* now checks whether cdrom_playing */
	if (playing_update_tmp(&current_track) > 0) {
		if (!paused) xv_set(msg_item, PANEL_LABEL_STRING,
			my_asctime(tmp), 0);
		if (prop_showing) {
			set_remain_guage();
		}

		if (current_track != playing_track) {
		    /* still playing but on a different track */
		    DPRINTF(2)("itimer: Now playing track %d was %d\n",
			current_track, playing_track);
		    if (play_mode == PLAY_MODE_NORMAL) {
				reset_button(playing_track);
				playing_track = current_track;
				display_track(playing_track);
		    } /* else
			if (mem_play_next_track() < 1) return(NOTIFY_DONE);
			*/
		}
		else reverse_button(playing_track);
	}
	else if (paused) {
		reverse_button(playing_track);
	}
	else if (repeating) {
		reverse_button(playing_track);
		play_proc();
	}
	else {
	/* not playing and not paused 
	 * check here if more memory tracks remain
	 * if so go to play
	 */
	    if (!ejected && playing && play_mode != PLAY_MODE_NORMAL &&
		    mem_current_track < mem_ntracks - 1) {
			mem_play_next_track();
	    }
		else {
			DPRINTF(2)("itimer: End of disc\n");
			stop_proc(NULL, NULL);
			return (NOTIFY_DONE);
	    }
	}

	if (idle_mode && playing && !(paused && !idled)) check_idle();

	/* check if rew or f.f. is still down - continue motion */

	if (rewinding) {
	    rewinding++;
	    rew_proc(NULL, NULL);
	} 
	else if (forwarding) {
	    forwarding++;
	    ff_proc(NULL, NULL);
	}

	/* Reset itimer everytime */

	/* Compute next timeout from current time */
	itimer.it_value.tv_usec = 500000;
	itimer.it_value.tv_sec = 0; 

	/* Don't utilize subsequent interval */
	itimer.it_interval.tv_usec = 0;
	itimer.it_interval.tv_sec = 0;
	/* Utilize notifier hack to avoid stopped clock bug */
	c_tv = itimer.it_value;
	c_tv.tv_sec *= 2;
	notify_set_signal_check(c_tv);
	/* Set itimer event handler */
	(void) notify_set_itimer_func((Notify_client)(&clock_client), 
		clock_itimer_expired, ITIMER_REAL, &itimer,(struct itimerval *)0);
	return(NOTIFY_DONE);
}

static void
clock_init_tmp()
{

	tmp->tm_sec = 0;
	tmp->tm_min = 0;
}

int
playing_update_tmp(current_track)
	int *current_track;
{
	return(cdrom_get_relmsf(fd, tmp, current_track));
}

static char *
my_asctime(tm)
	struct	tm *tm;
{
	char	*sec_str[3];
	char	*min_str[3];
	register int	sec;
	register int	minutes;

	sec = tm->tm_sec;
	minutes = tm->tm_min;
	
	if (sec < 10) {
		sprintf(sec_str, "0%d", sec);
	}
	else {
		sprintf(sec_str, "%d", sec);
	}
	if (minutes < 10) {
		sprintf(min_str, "0%d", minutes);
	}
	else {
		sprintf(min_str, "%d", minutes);
	}
	
	sprintf(time_str, "%s:%s", min_str, sec_str);
	return time_str;
}

/*ARGSUSED*/
static void
volume_proc(item, value, event)
Panel_item	item;
	int		value;
	Event		*event;
{
	DPRINTF(4)("volume_proc: %d\n",volume);

	volume = value;
	set_volume();
}


static int
vol_conv(vol, balance)
	int vol, balance;
{
/* If the player is ever not underrated or changes so that more
 * of a range of volume control is necessary - these are the
 * values that will need to be changed.
 *
 * Yes, this probably could have been done in many better ways
 * If you come up with a great one -  send it to me  - rct
 */
	if (vol == 0 || balance == 0) return(0);
	else return((200 + (vol * 5)) - (10 - balance) * 5);
}


void
set_volume()
{
	int	left, right; 

	left = vol_conv(volume, left_balance);

	right = vol_conv(volume, right_balance);

	DPRINTF(2)("set_volume: left %d, right %d\n", left, right);
	if (fd > 0 && !ejected) cdrom_volume(fd, left, right);
}


int
mem_play_next_track()
{
	if (mem_current_track == mem_ntracks - 1) {
	    DPRINTF(2)("Mem play: end of memory play list\n");
	    stop_proc(NULL, NULL);
	    return(0);
	}

	reset_button(mem_play[mem_current_track++]);
	playing_track = mem_play[mem_current_track];
	clock_init_tmp();
	display_track(playing_track);
	acc_msf(&mem_cur_msf, get_track_duration(toc, playing_track));
	/* should msf and remain guage be updated here? 
	 * will be updated by next itimer, how long lag?
	 */
	DPRINTF(2)("Mem playing: No. %d - Track %d\n",mem_current_track,
	    playing_track);
	xv_set(prop_objs->mem_list, PANEL_LIST_SELECT,
	    mem_current_track, TRUE, 0);
	return(cdrom_play_track(fd, playing_track, playing_track));
}


/*ARGSUSED*/
static void
stop_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	cdrom_stop(fd);
	clock_init_tmp();
	if(repeating) reset_cycle_button();
	if (playing) {
		playing = 0;
		if (!ejected) {  /* just in case called by an old itimer
				  * event which has occured since ejected
				  */
		    display_msf(total_msf);
		    reset_play_button();
		}
		display_track(-1);
	} 
	if (paused) {
		paused = 0;
		reset_pause_button();
	}

	if (!ejected) reset_all_buttons();
	/* I wouldn't think this would be necessary, but I have seen
	 * cases where the player finishes (itimer notices) and then
	 * find the player later still spinning...
	 */

	if (!ejected) cdrom_stop(fd);

	playing_track = -1;
	starting_track = -1;
	mem_current_track = -1;
	mem_cur_msf.min = 0;
	mem_cur_msf.sec = 0;

	if (prop_showing) {
	    set_remain_guage();
	}
}


/*ARGSUSED*/
static void
eject_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	if (playing) {
		stop_proc();
	}
	if (paused) {
		paused = 0;
		reset_pause_button();
	}
	if(repeating) reset_cycle_button();

	/* yes, this is messy, when stop proc is called, it sets the
	 * function buttons back to stop.  Since it has come from
	 * eject, it eject button should still be depressed.
	 */

	xv_set(play_choice, PANEL_VALUE, CHOICE_EJECT, 0);
	xv_set(prop_objs->mode_choice, PANEL_VALUE, PLAY_MODE_NORMAL,
		PANEL_INACTIVE, TRUE, 0);
	play_mode = PLAY_MODE_NORMAL;

	xv_set(prop_objs->mem_list, PANEL_READ_ONLY, TRUE, 0);

	if (ejected || fd < 0) { /* disc never mounted, device not opened */
	    DPRINTF(2)("no cd currently mounted, attempting to mount\n");
	    read_toc_alert();
	}
	else if ((data_disc == 0) || (nx == 0)) { 
		DPRINTF(4)("Ejecting mounted cd\n");
		ejected++;
		cdrom_eject(fd);
		clear_buttons();
		xv_set(msg_item, PANEL_LABEL_STRING, EJECT_MESSAGE, 0);
		xv_set(prop_objs->guage1, PANEL_VALUE, 0, 0);
		read_toc_alert();
	}
	else {
		/* disc has data track, see if it is mounted */
		if (mounted(dev_name) == UNMOUNTED) {
		    DPRINTF(4)("Ejected mounted cd with data tracks\n");
		    ejected++;
		    cdrom_eject(fd);
		    clear_buttons();
		    xv_set(msg_item, PANEL_LABEL_STRING, EJECT_MESSAGE, 0);
		    xv_set(prop_objs->guage1, PANEL_VALUE, 0, 0);
		    read_toc_alert();
		}
		else {
		    DPRINTF(2)("Data tracks mounted, cd not ejected\n");
		    generic_alert("CD-ROM Disc is mounted.",
				  "Disc not ejected.");
		    reset_play_button();
		}
	}
}

static void
clear_buttons()
{
	int	i;
	
	for (i = 1; i <= num_tracks; i++) {
		xv_destroy(track_items[i]);
	}
	/* wmgr_refreshwindow(frame); */

}

static Toc
read_disc_toc()
{
	struct	cdrom_tochdr	hdr;
	struct	cdrom_tocentry	entry;
	int	first_track;
	int	last_track;
	int	i;
	Toc	toc;
	Msf	msf, tmp_msf;
	int	control;
	Track_entry tl;
	char	buf[20];

	if (cdrom_read_tochdr(fd, &hdr) == 0) {
		return (Toc)NULL;
	}

	first_track = hdr.cdth_trk0;
	last_track = hdr.cdth_trk1;

	entry.cdte_format = CDROM_MSF;
	entry.cdte_track = CDROM_LEADOUT;
	cdrom_read_tocentry(fd, &entry);
	msf = init_msf();
	msf->min = entry.cdte_addr.msf.minute;
	msf->sec = entry.cdte_addr.msf.second;
	msf->frame = entry.cdte_addr.msf.frame;

	total_msf = msf;
	toc = init_toc(first_track, last_track, msf);


	tmp_msf = init_msf();
	entry.cdte_format = CDROM_MSF;
	data_disc = 0;
	for (i = first_track; i <= last_track; i++) {
		entry.cdte_track = i;
		cdrom_read_tocentry(fd, &entry);
		msf = init_msf();
		msf->min = entry.cdte_addr.msf.minute;
		msf->sec = entry.cdte_addr.msf.second;
		msf->frame = entry.cdte_addr.msf.frame;
		control = entry.cdte_ctrl;
		if (control & CDROM_DATA_TRACK) {
			data_disc = 1;
		}

		entry.cdte_track = i + 1;
		if (entry.cdte_track > last_track) {
			entry.cdte_track = CDROM_LEADOUT;
		}
		entry.cdte_format = CDROM_MSF;
		cdrom_read_tocentry(fd, &entry);

		tmp_msf->min = entry.cdte_addr.msf.minute; 
		tmp_msf->sec = entry.cdte_addr.msf.second; 
		tmp_msf->frame = entry.cdte_addr.msf.frame;

		add_track_entry(toc, i, control,
				msf, tmp_msf);
	}

	/* initialize track_list for memory play selections */
	/* this perhaps doesn't belong here */

	mem_ntracks = 0;	
	mem_total_msf.min = 0;
	mem_total_msf.sec = 0;
	mem_total_msf.frame = 0;
	for (i = 0; i < toc->size; i++) {
	sprintf(buf, "%02d - %02d:%02d", toc->toc_list[i]->track_number,
	    toc->toc_list[i]->duration->min, 
	    toc->toc_list[i]->duration->sec);
	xv_set(prop_objs->track_list, PANEL_LIST_INSERT, i,
	    PANEL_LIST_STRING, i, buf,
	    PANEL_LIST_CLIENT_DATA, i, toc->toc_list[i]->track_number,
	    PANEL_PAINT, PANEL_NONE,
	    0);
	}
	xv_set(prop_objs->track_list, XV_SHOW, TRUE, 0);
	return (toc);
}

static void
generic_alert(msg1, msg2)
	char	*msg1;
	char	*msg2;
{
	int status;

	status = notice_prompt(frame, (Event *)NULL,
		     NOTICE_MESSAGE_STRINGS,
		     msg1, msg2, 0,
		     NOTICE_BUTTON_YES, "Continue",
		     NOTICE_BUTTON_NO, "Quit", 0);
	if (status == NOTICE_NO) {
		exit(0);
	}
}

static int
read_toc_alert()
{
	int	status;
	int	i;

	/* shouldn't this be in eject? */
	if (toc != NULL) {
		for (i = num_tracks - 1; i >= 0; i--)
			xv_set(prop_objs->track_list,
			    PANEL_LIST_DELETE, i,
			    PANEL_PAINT, PANEL_NONE,
			0);
		xv_set(prop_objs->track_list, XV_SHOW, TRUE, 0);
		if (mem_ntracks) mem_cleanup();
		destroy_toc(toc);
	}

	if (fd < 0) {
	    DPRINTF(2)("read_toc_alert: device never opened\n");
	    cdrom_open();
	}

	if (fd < 0 || (toc = read_disc_toc()) == (Toc)NULL) {

		xv_set(panel1, WIN_MOUSE_XY, 
		    xv_get(play_choice, PANEL_CHOICE_X, CHOICE_EJECT),
		    xv_get(play_choice, PANEL_CHOICE_Y, CHOICE_EJECT), 0);

/* this seemed to be ignored */
		xv_set(panel1, NOTICE_FOCUS_XY, 
		    xv_get(play_choice, PANEL_CHOICE_X, CHOICE_EJECT),
		    xv_get(play_choice, PANEL_CHOICE_Y, CHOICE_EJECT), 0);

		status = notice_prompt(frame, (Event *)NULL,
			      NOTICE_MESSAGE_STRINGS,
			      "Insert Another Audio CD and Click Continue",
			      0,
			      NOTICE_BUTTON_YES, "Continue", 
			      NOTICE_BUTTON_NO, "Quit",
			      0);
		if (status == NOTICE_NO) {
			exit(0);
		}
	}

	if (fd < 0 || (toc = read_disc_toc()) == (Toc) NULL) {
	    DPRINTF(2)("read_toc_alert: failed to mount cd\n");
	    return(FALSE);
	}

	num_tracks = toc->size;
	create_panel_items(panel);
	display_msf(total_msf);
	reset_play_button(NULL, NULL);
	ejected = FALSE;
	xv_set(prop_objs->mode_choice, PANEL_INACTIVE, FALSE, 0);
	xv_set(prop_objs->mem_list, PANEL_READ_ONLY, FALSE, 0);
	fit_panels();

	return(TRUE);
}

/*ARGSUSED*/
void
pause_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	if (paused) {
		cdrom_resume(fd);
		paused = 0;
		reset_pause_button();
	}
	else if (playing) {
		cdrom_pause(fd);
		paused = 1;
		reverse_pause_button();
	}
}	

/*ARGSUSED*/
static void
play_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	int	track_number;
	int	ending_track;

	if (paused) {
		paused = 0;
		reset_pause_button();
		if(starting_track == playing_track) {
			cdrom_resume(fd);
			return;
		}
	}

	if (play_mode == PLAY_MODE_NORMAL) {
	    if (starting_track == -1) { /* no currently picked track */
			track_number = 1;
	    }
		else track_number = starting_track;
	    starting_track = -1; /* reset starting track for next play */
	    ending_track = end_audio_track(track_number);
	}
	else if (!playing && play_mode != PLAY_MODE_NORMAL) {
		/*
		 * Memory or shuffle play mode:
		 * read memory play list at this point
		 */
	    if (mem_ntracks == 0) { /* no memory tracks picked - nop */
			reset_play_button();
			return;
	    }
	    mem_play_init();
	    track_number = mem_play[mem_current_track];
	    starting_track = -1;
	    ending_track = mem_play[mem_current_track];
	}
	else { 
	    /* playing && memory play mode */
	    if (starting_track == -1) {
			/* no currently picked track - restart current track */
			track_number = current_track;
			ending_track = track_number;
			starting_track = -1;
	    }
		else {
		/* search for picked track in memory play list & position
		 * play there.  Else just start playing from beginning of
		 * current track
		 */
		 if ((starting_track = search_memlist(starting_track)) != -1) {
		     mem_current_track = starting_track;
		     track_number = mem_play[mem_current_track];
		     ending_track = track_number;
		     starting_track = -1;
		 }
		 else {
		     fprintf(stderr,"play: picked track not found in mem list\n");
		     track_number = current_track;
		     ending_track = current_track;
		 }
	    } 
	}

	if (check_data_track(track_number)) {
	    char	str[32];

	    sprintf(str, "Track %d Is Not An Audio Track",
		track_number);
	    notice_prompt(frame, (Event *)NULL,
		 NOTICE_MESSAGE_STRINGS, str, 0,
		 NOTICE_BUTTON_YES, "Continue",
		 0);
	    reset_play_button();

	    return;
	}


	if (play_mode != PLAY_MODE_NORMAL) xv_set(prop_objs->mem_list,
	    PANEL_LIST_SELECT, mem_current_track, TRUE, 0);

	mem_cur_msf.min = 0;
	mem_cur_msf.sec = 0;
	display_track(track_number);
	set_volume();
	cdrom_play_track(fd, track_number, ending_track);
	clock_init_tmp();
	if (!playing) {
	    playing = 1;
	    reverse_play_button();
	}
	if (playing_track != -1) {
	    reset_all_buttons();
	}
	playing_track = track_number;
	clock_itimer_expired(&clock_client, ITIMER_REAL);
}


static void
choice_notify(item, value, event)
Panel_item	item;
	int		value;
	Event		*event;
{
	DPRINTF(2)("function button hit %d\n", value);

	/* if a cartridge is not already loaded
	 * send one alert, try to get it open
	 * if not return to ejected state.
	 */

	if (ejected || fd < 0) {
	    if (!read_toc_alert()) {
		xv_set(play_choice, PANEL_VALUE, CHOICE_EJECT, 0);
		return;
	    }
		else {
	    	/* cd just inserted, set button correctly, (reset by read_toc)
		 * don't finish eject if just mounted
		 */
		if (value == CHOICE_EJECT)
		    return;
		else
		    xv_set(play_choice, PANEL_VALUE, value, 0);
	    }
	}

	switch(value) {
	case CHOICE_STOP:
	    stop_proc(NULL, NULL);
	    break;
	case CHOICE_PLAY:
	    play_proc(NULL, NULL);
	    break;
	case CHOICE_PAUSE:
	    pause_proc(NULL, NULL);
	    break;
	case CHOICE_EJECT:
	    eject_proc(NULL, NULL);
	    break;
	default:
		fprintf(stderr,"choice notify: invalid value %d returned\n",
			value);
		break;
	}
}


static int
end_audio_track(track_number)
	int	track_number;
{
	int	i;

	for (i = track_number+1; i <= num_tracks; i++) {
		if (get_track_control(toc, i) & CDROM_DATA_TRACK) {
			return (i-1);
		}
	}
	return (num_tracks);
}

static void
reverse_button(track)
	int	track;
{

	DPRINTF(8)("reverse_button: track %d currently %d\n",
	track,item_reversed[track]);

	if ((track < 1) || (track > num_tracks)) {
		fprintf(stderr,"reverse_button: bad track %d\n",track);
		return;
	}


	if (item_reversed[track] == 0) {
		item_reversed[track] = 1;
#ifndef TRACK_CHOICE
		xv_set(track_items[track], PANEL_ITEM_COLOR, 3, 0);
#else
	    xv_set(track_items[track], PANEL_TOGGLE_VALUE, 0, TRUE, 0);
#endif
	}
	else {
		item_reversed[track] = 0;
		/* xv_set(track_items[track], PANEL_ITEM_COLOR, 0, 0); */
#ifndef TRACK_CHOICE
		xv_set(track_items[track], PANEL_ITEM_COLOR, 4, 0);
#else
	    xv_set(track_items[track], PANEL_TOGGLE_VALUE, 0, FALSE, 0);
#endif
	}
}

static void
reset_button(track)
	int	track;
{
#ifdef TRACK_CHOICE
	DPRINTF(5)("reset button track %d reversed %d toggle %d\n",track,
		item_reversed[track], xv_get(track_items[track],
			PANEL_TOGGLE_VALUE, 0));

	if (item_reversed[track] || xv_get(track_items[track],
		PANEL_TOGGLE_VALUE, 0)) {
	    xv_set(track_items[track], PANEL_TOGGLE_VALUE, 0, FALSE, 0);
	    item_reversed[track] = 0;
	}
#else
	DPRINTF(5)("reset button track %d reversed %d color %d\n",track,
		item_reversed[track], xv_get(track_items[track],
			PANEL_ITEM_COLOR));
	if (item_reversed[track]) { /* avoid unnecessary painting */
	    xv_set(track_items[track], PANEL_ITEM_COLOR, 2, 0);
	    item_reversed[track] = 0;
	}
#endif
}

static void
reset_all_buttons()
{
	int	i;

	for (i = 1; i <= num_tracks; i++) {
		reset_button(i);
	}

	starting_track = -1;
}

static void
reverse_pause_button()
{
	DPRINTF(4)("reverse pause button\n");
	xv_set(play_choice, PANEL_VALUE, CHOICE_PAUSE, 0);
}

static void
reset_pause_button()
{
	DPRINTF(4)("reset pause button\n");
	if (playing) {
	    if (xv_get(play_choice, PANEL_VALUE) != CHOICE_PLAY)
		xv_set(play_choice, PANEL_VALUE, CHOICE_PLAY, 0);
	}
	else {
	    if (xv_get(play_choice, PANEL_VALUE) != CHOICE_STOP)
	    xv_set(play_choice, PANEL_VALUE, CHOICE_STOP, 0);
	}
}

static void
reverse_play_button()
{
	DPRINTF(4)("reverse play button\n");
	if (xv_get(play_choice, PANEL_VALUE) != CHOICE_PLAY)
	    xv_set(play_choice, PANEL_VALUE, CHOICE_PLAY, 0);

	xv_set(prop_objs->mode_choice, PANEL_INACTIVE, TRUE, 0);
	xv_set(prop_objs->mem_list, PANEL_READ_ONLY, TRUE, 0);
}

static void
reset_play_button()
{	
	DPRINTF(4)("reset play button\n");
	if (xv_get(play_choice, PANEL_VALUE) != CHOICE_STOP)
	    xv_set(play_choice, PANEL_VALUE, CHOICE_STOP, 0);
	xv_set(prop_objs->mode_choice, PANEL_INACTIVE, FALSE, 0);
	xv_set(prop_objs->mem_list, PANEL_READ_ONLY, FALSE, 0);
}


static void
rew_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	struct	cdrom_msf	msf;
	struct	msf	rew_delta_msf;
	Msf		cur_msf;

	if (!playing || paused) {
	    rewinding = 0;
	    xv_set(rew_item, PANEL_TOGGLE_VALUE, 0, FALSE, 0);
	    return;
	}

	if (event != NULL)
	    switch(event_action(event)) {
	    case ACTION_SELECT:
	    case ACTION_ADJUST:
	    case PANEL_EVENT_CANCEL:
		if (event_is_down(event) &&
			event_action(event) != PANEL_EVENT_CANCEL) {
		    rewinding = 1;
		    xv_set(rew_item, PANEL_TOGGLE_VALUE, 0, TRUE, 0);
		    forwarding = 0;
		}
		else {
		    rewinding = 0;
		    xv_set(rew_item, PANEL_TOGGLE_VALUE, 0, FALSE, 0);
		}
		break;

	    default:
		DPRINTF(4)("rew: event action %d\n",event_action(event));
		break;
	    }

	if (!rewinding) return;

	rew_delta_msf.min = 0;
	rew_delta_msf.sec = rew_delta * rewinding;
	rew_delta_msf.frame = 0;

	if (!cdrom_get_msf(fd, &msf)) {
	    perror("rewind: get msf failed");
	    return;
	}

	DPRINTF(4)("rew: MSF before rewind %d %d %d\n", msf.cdmsf_min0,
	    msf.cdmsf_sec0, msf.cdmsf_frame0);

	/* figure out where to position to */

	/* these msf gyrations are necessary because the msf
	 * routines use the msf.h structure def, and the cdrom
	 * routines use the cdromio(4) notion of msf
	 */

	cur_msf = init_msf();
	cur_msf->min = msf.cdmsf_min0;
	cur_msf->sec = msf.cdmsf_sec0;
	cur_msf->frame = msf.cdmsf_frame0;

	acc_msf(cur_msf, rew_delta_msf);

	msf.cdmsf_min0 = cur_msf->min;
	msf.cdmsf_sec0 = cur_msf->sec;
	msf.cdmsf_frame0 = cur_msf->frame;

	/*
	 * at this point we could be at another track, if we are in
	 * normal (linear) mode everything is fine since this is
	 * just an absolute msf on disc.  If we are in memory mode
	 * we want to set the position to be in the order of the
	 * memory play list
	 *
	 * We also need to figure out how long to play.
	 * If in normal (linear) play mode, play til end of disc.
	 * If in memory play mode, need to play only to the end of
	 * the current track.  Clock_itimer will detect when the playing
	 * of the current track is complete and play the next track in
	 * the list.
	 */

	if (play_mode == PLAY_MODE_NORMAL) {
	    /* did we run off 1st track? - start at beginning */
	    if (cmp_msf(cur_msf, toc->toc_list[0]->msf) < 0) {
		DPRINTF(4)("rew: rewound past 1st track\n");
		msf.cdmsf_min0 = toc->toc_list[0]->msf->min; 
		msf.cdmsf_sec0 = toc->toc_list[0]->msf->sec; 
		msf.cdmsf_frame0 = toc->toc_list[0]->msf->frame; 
	    }

	    /* play til end of disc */
	    msf.cdmsf_sec1 = total_msf->sec;
	    msf.cdmsf_min1 = total_msf->min;
	    msf.cdmsf_frame1 = total_msf->frame - 1;

	}
	else { /* memory play mode */
	    if (cmp_msf(cur_msf, toc->toc_list[current_track-1]->msf) < 0) {
		/* rewound past beginning of current track */
			if (mem_current_track == 0) {
			/* rewound past beginning of first memory track
			 * so just start at the beginning of first memory track
			 */
			   DPRINTF(4)("rew: rewound past beginning of 1st mem track\n");
			   msf.cdmsf_min0 = toc->toc_list[current_track-1]->msf->min; 
			   msf.cdmsf_sec0 = toc->toc_list[current_track-1]->msf->sec; 
			   msf.cdmsf_frame0 = toc->toc_list[current_track-1]->msf->frame; 
			}
			else {
			/* rewound past current track && ! first memory track
			 * so position to end of  previous memory track - rew_delta
			 */
			   reset_button(current_track);
			   current_track = mem_play[--mem_current_track];
			   xv_set(prop_objs->mem_list, PANEL_LIST_SELECT,
				   mem_current_track, TRUE, 0);
			   playing_track = current_track;

			   DPRINTF(4)("rew: rewound into mem %d track %d\n",
		       mem_current_track, current_track);

			   cur_msf->min = toc->toc_list[current_track-1]->msf->min; 
			   cur_msf->sec = toc->toc_list[current_track-1]->msf->sec; 
			   cur_msf->frame = toc->toc_list[current_track-1]->msf->frame; 

			   acc_msf(cur_msf, toc->toc_list[current_track-1]->duration);
			   acc_msf(cur_msf, rew_delta_msf);
			   
			   msf.cdmsf_min0 = cur_msf->min;
			   msf.cdmsf_sec0 = cur_msf->sec;
			   msf.cdmsf_frame0 = cur_msf->frame;
			}
	    }

	    /* figure how long to play - memory mode - end of current track */
	    if (current_track < num_tracks) {
			msf.cdmsf_sec1 = toc->toc_list[current_track]->msf->sec;
			msf.cdmsf_min1 = toc->toc_list[current_track]->msf->min;
			msf.cdmsf_frame1 = toc->toc_list[current_track]->msf->frame;
	    }
		else { /* last track - play til end of disc */
			msf.cdmsf_sec1 = total_msf->sec;
			msf.cdmsf_min1 = total_msf->min;
			msf.cdmsf_frame1 = total_msf->frame - 1;
	    }
	}

	free(cur_msf);

	DPRINTF(4)("rew: MSF after rewind %d %d %d - %d %d %d\n",
	    msf.cdmsf_min0, msf.cdmsf_sec0, msf.cdmsf_frame0,
	    msf.cdmsf_min1, msf.cdmsf_sec1, msf.cdmsf_frame1);

	if (!cdrom_play_msf(fd, &msf)) {
	    perror("rewind: play msf failed after rewind");
	}

	playing_update_tmp(&current_track);
	xv_set(msg_item, PANEL_LABEL_STRING, my_asctime(tmp), 0);
}

reverse_cycle_button()
{
	if(repeating) {
#ifndef TRACK_CHOICE
		xv_set(cycle_item, PANEL_ITEM_COLOR, 3, 0);
#else
		xv_set(cycle_item, PANEL_TOGGLE_VALUE, 0, TRUE, 0);
#endif
	}
	else {
#ifndef TRACK_CHOICE
		xv_set(cycle_item, PANEL_ITEM_COLOR, 4, 0);
#else
		xv_set(cycle_item, PANEL_TOGGLE_VALUE, 0, FALSE, 0);
#endif
	}
}

reset_cycle_button()
{
#ifndef TRACK_CHOICE
	xv_set(cycle_item, PANEL_ITEM_COLOR, 4, 0);
#else
	xv_set(cycle_item, PANEL_TOGGLE_VALUE, 0, FALSE, 0);
#endif
	repeating = 0;
	cp_got_button_up = 1;
}

static void
cycle_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	if (event != NULL) {
		if(event_left_is_down(event) && cp_got_button_up) {
			cp_got_button_up = 0;
			if(!repeating) repeating = 1;
			else repeating = 0;
			reverse_cycle_button();
		}
		else if(event_is_up(event) || event_id(event) == LOC_WINEXIT) 
			cp_got_button_up = 1;
	}
}

static void
ff_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	struct	cdrom_msf	msf;
	struct	msf		ff_delta_msf;
	Msf	cur_msf;

	if (!playing || paused) {
	    forwarding = 0;
	    xv_set(ff_item, PANEL_TOGGLE_VALUE, 0, FALSE, 0);
	    return;
	}

	if (event != NULL) {
	    switch(event_action(event)) {
	    case ACTION_SELECT:
	    case ACTION_ADJUST:
	    case PANEL_EVENT_CANCEL:
			if (event_is_down(event) &&
				event_action(event) != PANEL_EVENT_CANCEL) {
				forwarding = 1;
				xv_set(ff_item, PANEL_TOGGLE_VALUE, 0, TRUE, 0);
				rewinding = 0;
			}
			else {
				forwarding = 0;
				xv_set(ff_item, PANEL_TOGGLE_VALUE, 0, FALSE, 0);
			}
			break;

	    default:
			DPRINTF(4)("f.f.: event action %d\n",event_action(event));
		}
	}

	if (!forwarding) return;

	ff_delta_msf.min = 0;
	ff_delta_msf.sec = forwarding * ff_delta;
	ff_delta_msf.frame = 0;

	if (!cdrom_get_msf(fd, &msf)) {
	    perror("fast forward: get msf failed");
	    return;
	}

	DPRINTF(4)("ff: MSF before fast forward %d %d %d\n", msf.cdmsf_min0,
		msf.cdmsf_sec0, msf.cdmsf_frame0);

	/* figure out where to position to */

	cur_msf = init_msf();
	cur_msf->min = msf.cdmsf_min0;
	cur_msf->sec = msf.cdmsf_sec0;
	cur_msf->frame = msf.cdmsf_frame0;

	acc_msf(cur_msf, ff_delta_msf);

	msf.cdmsf_min0 = cur_msf->min;
	msf.cdmsf_sec0 = cur_msf->sec;
	msf.cdmsf_frame0 = cur_msf->frame;

	/*
	 * at this point we could be at another track, if we are in
	 * normal (linear) mode everything is fine since this is
	 * just an absolute msf on disc.  If we are in memory mode
	 * we want to set the position to be in the order of the
	 * memory play list
	 * 
 	 * we also need to figure out how long to play. Normal - til end of
	 * disc.  Memory - til end of current track
	 */

	if (play_mode == PLAY_MODE_NORMAL) {
	    /* did we run off last track - don't do anything */
	    if (cmp_msf(cur_msf, total_msf) > 0) {
		DPRINTF(4)("fast forward: attempt to f.f. past end of disc\n");
		free(cur_msf);
		return;
	    }

	    /* play til end of disc */
	    msf.cdmsf_sec1 = total_msf->sec;
	    msf.cdmsf_min1 = total_msf->min;
	    msf.cdmsf_frame1 = total_msf->frame - 1;
	}
	else {
	    if (cmp_msf(cur_msf, current_track == num_tracks ? total_msf :
		    toc->toc_list[current_track]->msf) > 0) {
		/* f.f'd past end of current track */
		if (mem_current_track+1 == mem_ntracks) {
		   /* f.f'd past end of last memory track - nop */
		   DPRINTF(4)("f.f.: attempt to f.f. past end of last memory track\n");
		   free(cur_msf);
		   return;
		}

		/* f.f'd past current track && ! last memory track
		 * so position to start of next memory track
		 */

		reset_button(current_track);
		current_track = mem_play[++mem_current_track];
		xv_set(prop_objs->mem_list, PANEL_LIST_SELECT,
		    mem_current_track, TRUE, 0);
		playing_track = current_track;

		DPRINTF(4)("ff: now playing mem %d track %d\n",
		    mem_current_track, current_track);

		msf.cdmsf_min0 = toc->toc_list[current_track-1]->msf->min;
		msf.cdmsf_sec0 = toc->toc_list[current_track-1]->msf->sec;
		msf.cdmsf_frame0 = toc->toc_list[current_track-1]->msf->frame;
	    }
	    
	    /* memory mode play til end of track */
	    if (current_track < num_tracks) {
		msf.cdmsf_sec1 = toc->toc_list[current_track]->msf->sec;
		msf.cdmsf_min1 = toc->toc_list[current_track]->msf->min;
		msf.cdmsf_frame1 = toc->toc_list[current_track]->msf->frame;
	    }
		else { /* last track - play til end of disc */
		msf.cdmsf_sec1 = total_msf->sec;
		msf.cdmsf_min1 = total_msf->min;
		msf.cdmsf_frame1 = total_msf->frame - 1;
	    }
	}

	DPRINTF(4)("ff: MSF after fast forward %d %d %d - %d %d %d\n",
	    msf.cdmsf_min0, msf.cdmsf_sec0, msf.cdmsf_frame0,
	    msf.cdmsf_min1, msf.cdmsf_sec1, msf.cdmsf_frame1);

	if (!cdrom_play_msf(fd, &msf)) {
	    perror("fast forward: play msf failed after f.f.");
	}

	playing_update_tmp(&current_track);
	xv_set(msg_item, PANEL_LABEL_STRING, my_asctime(tmp), 0);
}


int
check_data_track(track_number)
	int	track_number;
{
	if (get_track_control(toc, track_number) & CDROM_DATA_TRACK) {
	    return(TRUE);
	}

	return(FALSE);
}


/*ARGSUSED*/
static void
close_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	xv_set(frame, FRAME_CLOSED, TRUE, 0);
}

/*ARGSUSED*/
void
init_player(fd)
	int	fd;
{
	/* cdrom_volume(fd, volume, volume); */
}

static void
display_track(number)
	int	number;
{
	char	str[20];

	if (number <= 0) {
		sprintf(str, "Track: --"); 
	}
	else sprintf(str, "Track: %02d", number);

	xv_set(track_item,PANEL_LABEL_STRING, str, 0);

}

static void
display_msf(msf)
	Msf	msf;
{
	char	*sec_str[3];
	char	*min_str[3];
	register int	sec;
	register int	minutes;

	sec = msf->sec;
	minutes = msf->min;
	
	if (sec < 10) {
		sprintf(sec_str, "0%d", sec);
	}
	else {
		sprintf(sec_str, "%d", sec);
	}
	if (minutes < 10) {
		sprintf(min_str, "0%d", minutes);
	}
	else {
		sprintf(min_str, "%d", minutes);
	}
	
	sprintf(time_str, "%s:%s", min_str, sec_str);
	xv_set(msg_item, PANEL_LABEL_STRING, time_str, 0);
}


static void
panel_events(win, event)
	Xv_Window	win;
	Event		*event;
{
	if (event_action(event) == ACTION_MENU) {
		menu_show(prop_menu, win, event, NULL);
	}
}
