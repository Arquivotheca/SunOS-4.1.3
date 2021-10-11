/*************************************************************
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
static char sccsid[] = "@(#)player.c 1.1 92/07/30 Copyr 1989, 1990 Sun Microsystem, Inc.";
#endif

#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/tty.h>
#include <suntool/textsw.h>
#include <suntool/canvas.h>
#include <suntool/alert.h>
#include <suntool/icon.h>

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

short player_image[256] = {
#include "player.icon"
};
DEFINE_ICON_FROM_IMAGE(cd_player_icon, player_image);

Cursor hglass_curs;

static short hglass_image[] = {
#include <images/hglass.cursor>
};
mpr_static(hglass_cursor, 16, 16, 1, hglass_image);

static	char *my_asctime();

struct tm 	trk_accum_tim;
struct tm	*trk_atim = &trk_accum_tim;
struct tm 	trk_total_tim;
struct tm	*trk_ttim = &trk_total_tim;
struct tm 	dsc_accum_tim;
struct tm	*dsc_atim = &dsc_accum_tim;
struct tm 	dsc_total_tim;
struct tm	*dsc_ttim = &dsc_total_tim;
struct tm	plus_one;
struct tm	minus_one;

struct icon	*player_icon = &cd_player_icon;

int	fd = -1;
int	frame_fd;
int open_flag = O_RDONLY|O_EXCL ; 

static	int	playing = 0;
static  int	paused = 0;
static	int	playing_track = -1;
static  int	starting_track = 1;
static	int repeating = 0;
static	int accum_disc = 0;
static	int accum_track= 0;
static	void	create_panel_items();
static	void	create_panel1();
static	void	create_panel2();
static	void	create_panel3();
static	void	pick_track_proc();
static void	stop_proc();
static void	eject_proc();
static  void	pause_proc();
static	void	play_proc();
static void		close_proc();
static	void	repeat_proc();
static	void	plus_track_proc();
static	void	quit_proc();
static	void	volume_proc();
static	void	read_toc_alert();
static	void	clear_buttons();
static	void	reset_button();
static	void	reset_all_buttons();
static	Notify_value clock_itimer_expired();
static	void	clock_init();
static	void	clock_update();
static	Toc	read_disc_toc();
static	void	reverse_button();
static	void	reverse_pause_button();
static	void	reverse_repeat_button();
static	void	reverse_play_button();
static	void	mode_select_page_eight();
void	init_player();
static	void	display_time();
static	int	end_audio_track();
static	void	generic_alert();
static 	void	track_time_proc();
static 	void	disc_time_proc();
static 	struct tm *add_tm2();
static 	void add_tm3();

static	Panel	panel;
static	Panel	panel1;
static	Panel	panel2;
static 	Panel	panel3;
static	Frame	frame;

static	int clock_client;
static  char time_str[128];
static	int		num_tracks;	
static	Panel_item	track_items[99];
static	Pixrect		*orig_images[99];
static 	Pixrect     *accum_button;
static 	Pixrect     *remain_button;

static	int		item_reversed[99];
static	Panel_item	pause_item;
static	Panel_item	play_item;
static	Panel_item	repeat_item;

static 	Panel_item	trk_item;
static 	Panel_item	dsc_item;
static 	Panel_item	track_time_item;
static 	Panel_item	disc_time_item;


static	Toc	toc;

static	int	volume = 225;
static	int	data_disc = 0;
char	*dev_name;
int	nx = 0;


/*
 *	these lines added for debugging by DGR
 */

#include <errno.h>
extern int errno;
static int debug_value = 0;
static int do_debug = 0;
int debug;
int ejected;
char *ProgName;
#define DB_ENTRY 			0x01
#define DB_UPDATE_FRAME 	0x02
#define DB_DUMP_FRAME 		0x03
#define DEBUG_FRAME_SIZE	4096
static char frame_buf[DEBUG_FRAME_SIZE][60];
static int fbi = 0;
static 	void	debug_proc();

/*
 *	end of added lines
 */

#define	BUFFER_SIZE	0x400

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	i;

	dev_name = "/dev/rsr0";
	if (argc < 1) {
		fprintf(stderr, "usage: %s [-d device] [-n]\n", argv[0]);
		exit(1);
	}
	for (i=1; i<argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'd': 
				i++;
				dev_name = argv[i];
				break;
			case 'D':
				do_debug = 1;
				break;
			case 'W':
				break;
			case 'n':
				open_flag = O_RDONLY;
				break;
			default:
				fprintf(stderr, 
					"%c: unkown switch\n", argv[i][1]);
				fprintf(stderr, "usage: %s [-d device] [-n]\n",
					argv[0]);
				exit(1);
			}
		}
	}

	if ((fd = open(dev_name, open_flag)) < 0) {
		fprintf(stderr, "%s: failed to open device %s: ",
			argv[0], dev_name);
		perror("");
		exit(1);
	}

	/* stop any existing audio play */

	cdrom_stop(fd);

	clock_init();
	
	frame = window_create(NULL, FRAME, FRAME_LABEL, "CD PLAYER",
		FRAME_CLOSED, TRUE,
		FRAME_ICON, player_icon, 
		FRAME_ARGS, argc, argv,
		0);
	
	frame_fd = window_fd(frame);

	panel = window_create(frame, PANEL,
		WIN_ROW_HEIGHT, 16,
		WIN_COLUMN_WIDTH, 16,
		WIN_COLUMN_GAP, 17,
		WIN_ROW_GAP, 12,
		WIN_ROWS, 5,
		WIN_COLUMNS, 5,
		0);
	
	hglass_curs = cursor_create(
		CURSOR_IMAGE, hglass_cursor,
		CURSOR_OP, PIX_SRC | PIX_DST,
		0);

	if ((toc = read_disc_toc()) == (Toc)NULL) {
		perror();
		exit(1);
	}
	
	dsc_ttim -> tm_min = toc -> total_msf -> min;
	dsc_ttim -> tm_sec = toc -> total_msf -> sec;

	num_tracks = toc->size;
	create_panel_items(panel);
	window_fit_height(panel);

	create_panel1(frame);

	create_panel2(frame);

	create_panel3(frame);

	window_fit(frame);

	init_player(fd);

	window_main_loop(frame);

	exit(0); 
	/* NOTREACHED */
}

static void
create_panel1(frame)
	Frame	frame;
{
	panel1 = window_create(frame, PANEL,
		WIN_X, 0,
		WIN_BELOW, panel,
		WIN_ROW_HEIGHT, 16,
		WIN_COLUMN_WIDTH, 36,
		WIN_COLUMN_GAP, 19,
		WIN_ROW_GAP, 14,
		WIN_ROWS, 3,
		WIN_COLUMNS, 3,
		0);
	
	play_item = panel_create_item(panel1, PANEL_BUTTON,
		PANEL_ITEM_Y, ATTR_ROW(0),
		PANEL_ITEM_X, ATTR_COL(1),
		PANEL_LABEL_IMAGE,
		panel_button_image(panel1, "Play", 5, 0),
		PANEL_NOTIFY_PROC, play_proc,
		0);

	repeat_item = panel_create_item(panel1, PANEL_BUTTON,
		PANEL_ITEM_Y, ATTR_ROW(0),
		PANEL_ITEM_X, ATTR_COL(0),
		PANEL_LABEL_IMAGE,
		panel_button_image(panel1, "Again", 5, 0),
		PANEL_NOTIFY_PROC, repeat_proc,
		0);

	pause_item = panel_create_item(panel1, PANEL_BUTTON,
		PANEL_ITEM_Y, ATTR_ROW(0),
		PANEL_ITEM_X, ATTR_COL(2),
		PANEL_LABEL_IMAGE,
		panel_button_image(panel1, "Pause", 5, 0),
		PANEL_NOTIFY_PROC, pause_proc,
		0);

	panel_create_item(panel1, PANEL_BUTTON,
		PANEL_ITEM_Y, ATTR_ROW(1),
		PANEL_ITEM_X, ATTR_COL(0),
		PANEL_LABEL_IMAGE, 
		panel_button_image(panel1, "Stop", 5, 0),
		PANEL_NOTIFY_PROC, stop_proc,
		0);

	panel_create_item(panel1, PANEL_BUTTON,
		PANEL_ITEM_Y, ATTR_ROW(1),
		PANEL_ITEM_X, ATTR_COL(2),
		PANEL_LABEL_IMAGE,
		panel_button_image(panel1, "Eject", 5, 0),
		PANEL_NOTIFY_PROC, eject_proc,
		0);

	if(do_debug) {

		panel_create_item(panel1, PANEL_CYCLE,
			PANEL_ITEM_Y, ATTR_ROW(3),
			PANEL_ITEM_X, ATTR_COL(0),
			PANEL_LABEL_STRING, "Debug:",
			PANEL_CHOICE_STRINGS,
				"none", "entry", "frame", "dump", "four", 
				0,
			PANEL_NOTIFY_PROC, debug_proc,
			0);
	}

	window_fit_height(panel1);
}

static void
create_panel2(frame)
	Frame	frame;
{

	panel2 = window_create(frame, PANEL,
		WIN_X, 0,
		WIN_BELOW, panel1,
		WIN_COLUMN_WIDTH, 16,
		WIN_COLUMN_GAP, 17,
		WIN_COLUMNS, 5,
		0);
    
	accum_button = panel_button_image(panel2, "Cumulative", 10, (Pixfont *) 0);
    remain_button = panel_button_image(panel2, "Remaining", 10, (Pixfont *) 0);

	trk_item = panel_create_item(panel2, PANEL_MESSAGE,
		PANEL_LABEL_STRING, my_asctime(accum_track ? trk_atim : trk_ttim),
		PANEL_PAINT, PANEL_NO_CLEAR,
		PANEL_ITEM_Y, ATTR_ROW(0),
		PANEL_ITEM_X, ATTR_COL(3),
		0);

	track_time_item = panel_create_item(panel2, PANEL_BUTTON,
		PANEL_ITEM_Y, ATTR_ROW(0),
		PANEL_ITEM_X, ATTR_COL(0),
		PANEL_LABEL_IMAGE, remain_button,
		PANEL_NOTIFY_PROC, track_time_proc,
		0);

	dsc_item = panel_create_item(panel2, PANEL_MESSAGE,
		PANEL_LABEL_STRING, my_asctime(accum_disc ? dsc_atim : dsc_ttim),
		PANEL_PAINT, PANEL_NO_CLEAR,
		PANEL_ITEM_Y, ATTR_ROW(1),
		PANEL_ITEM_X, ATTR_COL(3),
		0);

	disc_time_item = panel_create_item(panel2, PANEL_BUTTON,
		PANEL_ITEM_Y, ATTR_ROW(1),
		PANEL_ITEM_X, ATTR_COL(0),
		PANEL_LABEL_IMAGE, remain_button,
		PANEL_NOTIFY_PROC, disc_time_proc,
		0);

	display_time();
	window_fit_height(panel2);
}	

static void
create_panel3(frame)
	Frame	frame;
{
	panel3 = window_create(frame, PANEL,
		WIN_X, 0,
		WIN_BELOW, panel2,
		WIN_COLUMN_WIDTH, 16,
		WIN_COLUMN_GAP, 17,
		WIN_COLUMNS, 5,
		0);

	panel_create_item(panel3, PANEL_SLIDER,
		PANEL_ITEM_X, ATTR_COL(0),
		PANEL_ITEM_Y, ATTR_ROW(0),
		PANEL_VALUE, 5,
		PANEL_MIN_VALUE, 0,
		PANEL_MAX_VALUE, 10,
		PANEL_SLIDER_WIDTH, 30,
		PANEL_LABEL_STRING, "Vol",
		PANEL_NOTIFY_PROC, volume_proc,
		0);

	panel_create_item(panel3, PANEL_BUTTON,
		PANEL_ITEM_X, ATTR_COL(0),
		PANEL_ITEM_Y, ATTR_ROW(1),
		PANEL_LABEL_IMAGE,
		panel_button_image(panel3, "Close", 5, 0),
		PANEL_NOTIFY_PROC, close_proc,
		0);

	panel_create_item(panel3, PANEL_BUTTON,
		PANEL_ITEM_X, ATTR_COL(3),
		PANEL_ITEM_Y, ATTR_ROW(1),
		PANEL_LABEL_IMAGE,
		panel_button_image(panel3, "Quit", 4, 0),
		PANEL_NOTIFY_PROC, quit_proc,
		0);

	window_fit_height(panel3);
}

static void	
create_panel_items(panel)
	Panel	panel;
{
	int	i;
	int	x;
	int	y;
	char	str[4];

	for (i=1; i<=num_tracks; i++) {
		x = (i-1) % 5;
		y = (i-1) / 5;
		sprintf(str, "%d", i);
		track_items[i] = panel_create_item(panel, PANEL_BUTTON,
			PANEL_ITEM_X, ATTR_COL(x),
			PANEL_ITEM_Y, ATTR_ROW(y),
			PANEL_LABEL_IMAGE,
			panel_button_image(panel, str, 2, 0),
			PANEL_CLIENT_DATA, (caddr_t)i,
			PANEL_NOTIFY_PROC, pick_track_proc,
			0);
		orig_images[i] = (Pixrect *)panel_button_image(panel, str, 
							       2, 0);
		item_reversed[i] = 0;

	}
}				  

static void
track_time_proc()
{
	accum_track = (accum_track) ? 0 : 1;

	panel_set(track_time_item, 
		PANEL_LABEL_IMAGE, (accum_track ? accum_button : remain_button), 
		0);
	
	panel_set(trk_item, 
		PANEL_LABEL_STRING, my_asctime(accum_track ? trk_atim : trk_ttim),
		0);
}
				  
static void
disc_time_proc()
{
	accum_disc = (accum_disc) ? 0 : 1;

	panel_set(disc_time_item, 
		PANEL_LABEL_IMAGE, (accum_disc ? accum_button : remain_button), 
		0);
	
	panel_set(dsc_item, 
		PANEL_LABEL_STRING, my_asctime(accum_disc ? dsc_atim : dsc_ttim),
		0);
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
	track_number = (int) panel_get(item, PANEL_CLIENT_DATA);
	reverse_button(track_number);
	starting_track = track_number;
	msf = get_track_duration(toc, track_number);
	trk_ttim -> tm_min = msf -> min;
	trk_ttim -> tm_sec = msf -> sec;
	get_disc_duration(dsc_atim, 1, track_number - 1);
	get_disc_duration(dsc_ttim, track_number, toc -> end_track);
	display_time();
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
	int	current_track;
	Msf msf;
	
	panel_set(trk_item, 
		PANEL_LABEL_STRING, my_asctime(accum_track ? trk_atim : trk_ttim),
		0);
	
	panel_set(dsc_item, 
		PANEL_LABEL_STRING, my_asctime(accum_disc ? dsc_atim : dsc_ttim),
		0);

	if (cdrom_playing(fd, &current_track)) {
		if (current_track != playing_track) {
			reset_button(playing_track);
			playing_track = current_track;
			msf = get_track_duration(toc, playing_track);
			trk_ttim -> tm_min = msf -> min;
			trk_ttim -> tm_sec = msf -> sec;
		}
		clock_update();
		reverse_button(playing_track);
	}
	else if (paused) {
		reverse_button(playing_track);
	}
	/*
		added to support repeating disc
	*/
	else if (repeating) {
		reverse_button(playing_track);
		play_proc();
	}
	else {
		clock_init();
		display_time();
		if (playing) {
			playing = 0;
			reverse_play_button();
		}
		reset_all_buttons();
		return (NOTIFY_DONE);
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
clock_init()
{

	trk_atim -> tm_sec = 0;
	trk_atim -> tm_min = 0;
	plus_one.tm_sec = 1;
	plus_one.tm_min = 0;
	minus_one.tm_sec = -1;
	minus_one.tm_min = 0;
}

static void
clock_update()
{
	static last_sec = 0;
	struct cdrom_subchnl sc;

	if(debug_value == DB_UPDATE_FRAME) {
		cdrom_get_subchnl(fd, &sc);
		sprintf(frame_buf[fbi++ & (DEBUG_FRAME_SIZE - 1)],
		"a: %d c: %d t: %02d i: %02d at: %02d.%02d f: %02d rt: %02d.%02d f: %02d",
			sc.cdsc_adr, sc.cdsc_ctrl, sc.cdsc_trk, sc.cdsc_ind,
			sc.cdsc_absaddr.msf.minute, 
			sc.cdsc_absaddr.msf.second, 
			sc.cdsc_absaddr.msf.frame,
			sc.cdsc_reladdr.msf.minute, 
			sc.cdsc_reladdr.msf.second, 
			sc.cdsc_reladdr.msf.frame);
	}
	cdrom_get_relmsf(fd, trk_atim, &playing_track);
	if(last_sec != trk_atim -> tm_sec) {
		last_sec = trk_atim -> tm_sec;
		add_tm3(dsc_atim, plus_one, dsc_atim);
		add_tm3(dsc_ttim, minus_one, dsc_ttim);
		add_tm3(trk_ttim, minus_one, trk_ttim);
	}
}

static char *
my_asctime(tm)
	struct	tm *tm;
{
	char	*sec_str[3];
	char	*min_str[3];
	register int	sec;
	register int	minutes;

	sec = tm -> tm_sec;
	minutes = tm -> tm_min;
	
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
	if (value == 0) {
		volume = 0;
	}	
	else {
		volume = 205 + (value * 5);	
	}
	cdrom_volume(fd, volume, volume);
}

/*ARGSUSED*/
static void
debug_proc(item, val, event)
Panel_item	item;
	int			val;
	Event		*event;
{
	register int i;

	debug_value = val;
	if(debug_value == DB_DUMP_FRAME) {
		for(i = 0; i < DEBUG_FRAME_SIZE; i++)
			fprintf(stdout, "%s\n", frame_buf[i]);
	}
}

/*ARGSUSED*/
static void
stop_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	if(repeating) {
		repeating = 0;
		reverse_repeat_button();
	}
	cdrom_stop(fd);
	clock_init();
	if (playing) {
		playing = 0;
		reverse_play_button();
		display_time();
	} 
	if (paused) {
		paused = 0;
		reverse_pause_button();
	}
	playing_track = -1;
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
		reverse_pause_button();
	}

	if ((data_disc == 0) || (nx == 0)) { 
		cdrom_eject(fd);
		clear_buttons();
		read_toc_alert();
	}
	else {
		/* disc has data track, see if it is mounted */
		if (mounted(dev_name) == UNMOUNTED) {
			cdrom_eject(fd);
			clear_buttons();
			read_toc_alert();
		}
		else {
			generic_alert("CD-ROM Disc is mounted.",
				      "Disc not ejected.");
		}
	}
}

static void
clear_buttons()
{
	int	i;
	
	for (i=1; i<=num_tracks; i++) {
		panel_destroy_item(track_items[i]);
	}
	wmgr_refreshwindow(frame_fd);
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

	if (cdrom_read_tochdr(fd, &hdr) == 0) {
		return (Toc)NULL;
	}

	first_track = hdr.cdth_trk0;
	last_track = hdr.cdth_trk1;

	entry.cdte_format = CDROM_MSF;
	entry.cdte_track = CDROM_LEADOUT;
	cdrom_read_tocentry(fd, &entry);
	if(debug_value == DB_ENTRY)
		printf("0t: %03d a: %d c: %d f: %03d t: %02d.%02d f: %03d d: %03d\n",
			entry.cdte_track, entry.cdte_adr, entry.cdte_ctrl, 
			entry.cdte_format,
			entry.cdte_addr.msf.minute, entry.cdte_addr.msf.second,
			entry.cdte_addr.msf.frame, entry.cdte_datamode);

	msf = init_msf();						/* malloc an msf */
	msf->min = entry.cdte_addr.msf.minute;
	msf->sec = entry.cdte_addr.msf.second;
	msf->frame = entry.cdte_addr.msf.frame;

	toc = init_toc(first_track, last_track, msf);	/* malloc a toc */

	tmp_msf = init_msf();
	entry.cdte_format = CDROM_MSF;
	data_disc = 0;
	for (i=first_track; i<=last_track; i++) {
		entry.cdte_track = i;
		cdrom_read_tocentry(fd, &entry);
		if(debug_value == DB_ENTRY)
			printf("1t: %03d a: %d c: %d f: %03d t: %02d.%02d f: %03d d: %03d\n",
				entry.cdte_track, entry.cdte_adr, entry.cdte_ctrl, 
				entry.cdte_format,
				entry.cdte_addr.msf.minute, entry.cdte_addr.msf.second,
				entry.cdte_addr.msf.frame, entry.cdte_datamode);
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
		if(debug_value == DB_ENTRY)
			printf("2t: %03d a: %d c: %d f: %03d t: %02d.%02d f: %03d d: %03d\n",
			entry.cdte_track, entry.cdte_adr, entry.cdte_ctrl, 
			entry.cdte_format,
			entry.cdte_addr.msf.minute, entry.cdte_addr.msf.second,
			entry.cdte_addr.msf.frame, entry.cdte_datamode);

		tmp_msf->min = entry.cdte_addr.msf.minute; 
		tmp_msf->sec = entry.cdte_addr.msf.second; 
		tmp_msf->frame = entry.cdte_addr.msf.frame;

		add_track_entry(toc, i, control, msf, tmp_msf);
	}
	
	return (toc);
}

static void
generic_alert(msg1, msg2)
	char	*msg1;
	char	*msg2;
{
	int status;

	status = alert_prompt(frame, (Event *)NULL,
		     ALERT_MESSAGE_STRINGS,
		     msg1, msg2, 0,
		     ALERT_BUTTON_YES, "Continue",
		     ALERT_BUTTON_NO, "Quit", 0);

	if (status == ALERT_NO) {
		exit(0);
	}
}

static void
device_open_alert()
{
}

static void
device_error_alert()
{
}

static void
read_toc_alert()
{
	int	status;
	Cursor curs;

	destroy_toc(toc);

	/* change the cursor to the hourglass */

	curs = window_get(panel1, WIN_CURSOR);
	(void) window_set(panel1, WIN_CURSOR, hglass_curs, 0);

	do {
		status = alert_prompt(frame, (Event *)NULL,
			ALERT_MESSAGE_STRINGS,
			"Insert Another Audio CD and Click Continue",
			0,
			ALERT_BUTTON_YES, "Continue", 
			ALERT_BUTTON_NO, "Quit",
			0);

		if (status == ALERT_NO) {
			exit(0);
		}

	} while ((toc = read_disc_toc()) == (Toc)NULL);
	
	/* change the cursor back */

	(void) window_set(panel1, WIN_CURSOR, curs, 0);

	/* get the total and display time from the toc for the msf structure */

	num_tracks= toc->size;
	create_panel_items(panel);
	wmgr_refreshwindow(frame_fd);
	window_set(panel, 
		WIN_COLUMN_WIDTH, 16,
		WIN_COLUMN_GAP, 17, 
		WIN_COLUMNS, 5,
		0);

	window_fit_height(panel);

	window_set(panel1, WIN_BELOW, panel, 0);
	window_fit_height(panel1);

	window_set(panel2, WIN_BELOW, panel1, 0);
	window_fit_height(panel2);

	window_set(panel3, WIN_BELOW, panel2, 0);
	window_fit_height(panel3);

	window_fit(frame);
}
	
/*ARGSUSED*/
static void
repeat_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	if(repeating) {
		repeating = 0;
	}
	else {
		repeating = 1;
	}
	reverse_repeat_button();
}

/*ARGSUSED*/
static void
pause_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	if (paused) {
		cdrom_resume(fd);
		paused = 0;
		reverse_pause_button();
	} else if (playing) {
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
	Msf msf;

	if (paused) {
		cdrom_resume(fd);
		paused = 0;
		reverse_pause_button();
	}
	else {
		track_number = starting_track;
		if (get_track_control(toc, track_number) & CDROM_DATA_TRACK) {
			char	str[32];

			sprintf(str, "Track %d Is Not An Audio Track", track_number);
			alert_prompt(frame, (Event *)NULL,
				ALERT_MESSAGE_STRINGS, str,
				0,
				ALERT_BUTTON_YES, "Continue",
				0);
		}
		else {
			starting_track = 1;
			ending_track = end_audio_track(track_number);
			cdrom_play_track(fd, track_number, ending_track);
			cdrom_volume(fd, volume, volume);
			clock_init();
			get_disc_duration(dsc_ttim, track_number, ending_track);
			if (!playing) {
				playing = 1;
				reverse_play_button();
			}
			if (playing_track != -1) {
				reset_all_buttons();
			}
			playing_track = track_number;
			
			/*	get length of track for display and total msf */
			msf = get_track_duration(toc, track_number);
			trk_ttim ->  tm_min = msf -> min;
			trk_ttim ->  tm_sec = msf -> sec;

			clock_itimer_expired(&clock_client, ITIMER_REAL);
		}
	}
}

static int
end_audio_track(track_number)
	int	track_number;
{
	int	i;

	for (i = track_number+1; i<= num_tracks; i++) {
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
	Pixrect	*button_image;

	if ((track < 1) || (track > num_tracks)) {
		return;
	}

	button_image = (Pixrect *)panel_get(track_items[track],
					    PANEL_LABEL_IMAGE);
	pr_reversevideo(button_image, 0, 1);
	panel_set(track_items[track], PANEL_LABEL_IMAGE, button_image, 0);
	if (item_reversed[track] == 0) {
		item_reversed[track] = 1;
	}
	else {
		item_reversed[track] = 0;
	}
}

static void
reset_button(track)
	int	track;
{
/*	panel_set(track_items[track], PANEL_LABEL_IMAGE,
		  orig_images[track], 0);*/
	if (item_reversed[track]) {
		reverse_button(track);
		item_reversed[track] = 0;
	}
}

static void
reset_all_buttons()
{
	int	i;

	for (i = 1; i<= num_tracks; i++) {
		reset_button(i);
	}
}

static void
reverse_pause_button()
{
	Pixrect	*button_image;

	button_image = (Pixrect *)panel_get(pause_item,
					    PANEL_LABEL_IMAGE);
	pr_reversevideo(button_image, 0, 1);
	panel_set(pause_item, PANEL_LABEL_IMAGE, button_image, 0);
}

static void
reverse_repeat_button()
{
	Pixrect	*button_image;

	button_image = (Pixrect *)panel_get(repeat_item,
					    PANEL_LABEL_IMAGE);
	pr_reversevideo(button_image, 0, 1);
	panel_set(repeat_item, PANEL_LABEL_IMAGE, button_image, 0);
}

static void
reverse_play_button()
{
	Pixrect	*button_image;

	button_image = (Pixrect *)panel_get(play_item, PANEL_LABEL_IMAGE);
	pr_reversevideo(button_image, 0, 1);
	panel_set(play_item,
		PANEL_LABEL_IMAGE, button_image,
		0);
}

/*ARGSUSED*/
static void
quit_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	exit(0);
}

/*ARGSUSED*/
static void
close_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	window_set(frame, FRAME_CLOSED, TRUE, 0);
}

/*ARGSUSED*/
void
init_player(fd)
	int	fd;
{
	volume = 225;
	cdrom_volume(fd, volume, volume);
}

static void
display_time()
{
	panel_set(trk_item,
		PANEL_LABEL_STRING, my_asctime(accum_track ? trk_atim : trk_ttim),
		0);

	panel_set(dsc_item,
		PANEL_LABEL_STRING, my_asctime(accum_disc ? dsc_atim : dsc_ttim),
		0);
}

/*
	add two tm structures, putting results in third
*/

static struct tm *
add_tm2(a, b)
	struct tm *a, *b;
{
	int carry = 0;
	static struct tm wrk, *c = &wrk;

	/*
		add the seconds
	*/
	c -> tm_sec = a -> tm_sec + b -> tm_sec;
	/*
		if there is roll over, add a minute
	*/
	if(c -> tm_sec > 59) {
		c -> tm_sec -= 60;
		carry = 1;
	}
	
	/*
		add the minutes
	*/
	c -> tm_min = a -> tm_min + b -> tm_min + carry;
	/*
		if the goal was subtraction, need to adjust the result
	*/
	if(c -> tm_sec < 0) {
		if(c -> tm_min <= 0) {
			c -> tm_sec = 0;
			c -> tm_min = 0;
		}
		else {
			c -> tm_min--;
			c -> tm_sec += 60;
		}
	}
	if(c -> tm_min < 0) c -> tm_min = 0;
	return(c);
}

static void
add_tm3(a, b, c)
	struct tm *a, *b, *c;
{
	int carry = 0;

	/*
		add the seconds
	*/
	c -> tm_sec = a -> tm_sec + b -> tm_sec;
	/*
		if there is roll over, add a minute
	*/
	if(c -> tm_sec > 59) {
		c -> tm_sec -= 60;
		carry = 1;
	}
	
	/*
		add the minutes
	*/
	c -> tm_min = a -> tm_min + b -> tm_min + carry;
	/*
		if the goal was subtraction, need to adjust the result
	*/
	if(c -> tm_sec < 0) {
		if(c -> tm_min <= 0) {
			c -> tm_sec = 0;
			c -> tm_min = 0;
		}
		else {
			c -> tm_min--;
			c -> tm_sec += 60;
		}
	}
	if(c -> tm_min < 0) c -> tm_min = 0;
}

/*
	get the total time from start_track to end_track
*/
get_disc_duration(ms, start_track, end_track)
	struct tm *ms;
	int start_track, end_track;
{
	register int i;
	Msf msfp;
	struct tm tmp_tm;

	ms -> tm_sec = 0;
	ms -> tm_min = 0;

	for(i = start_track; i <= end_track; i++) {
		msfp = get_track_duration(toc, i);
		tmp_tm.tm_min = msfp -> min;
		tmp_tm.tm_sec = msfp -> sec;
		add_tm3(ms, &tmp_tm, ms);
	}
}
