#if !defined(lint) && !defined(NOID)
static  char sccsid[] = "@(#)soundtool.c	1.1	92/07/30 Copyr 1991 Sun Micro";
#endif

/*
 * Copyright 1991, Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/types.h>
#include <sys/param.h>
#include <unistd.h>
#ifdef SYSV
#include <dirent.h>
#include <sys/audioio.h>
#else
#include <sys/dir.h>
#include <sun/audioio.h>
#endif
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/ioctl.h>

#include <xview/xview.h>
#include <xview/notice.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/pixwin.h>
#include <xview/scrollbar.h>
#include <xview/tty.h>
#include <xview/seln.h>
#include <xview/cursor.h>
#include <xview/screen.h>
#include <xview/server.h>
#include <xview/cms.h>

#include <multimedia/libaudio.h>
#include <multimedia/audio_device.h>
#include <multimedia/ulaw2linear.h>

Server_image soundtool_icon_image;
Server_image waveform_cursor_image;

static unsigned short soundtool_image_bits[] = {
#include "soundtool.icon"
};

static unsigned short waveform_cursor_bits[] = {
#include <images/ibeam.cursor>
};

#define	FPRINTF	(void) fprintf
#define	SPRINTF	(void) sprintf
#define	STRCPY	(void) strcpy


#ifdef DEBUG
#define	EVENTP(P, E)	(void) printf("%s: event %d\n", (P), event_id(E))
#define	DEBUGF(args)	(void) printf args
#define	PERROR(msg)	FPRINTF(stderr, "%s(%d): \"%s\" (%s)\n",	\
			    prog, __LINE__, msg, sys_errlist[errno]);
#else
#define	EVENTP(P, E)
#define	DEBUGF(args)
#define	PERROR(msg)	FPRINTF(stderr, "%s: \"%s\" (%s)\n",	\
			    prog, msg, sys_errlist[errno]);
#endif	/*!DEBUG*/


#define	REFRESH_RELOAD	(20)		/* only refresh every 20th time */

#define	BUTTON_WIDTH	(8)
#define	SCROLLBAR_WIDTH	(18)

#define	WINDOW_WIDTH	(800)
#define	SCOPE_HEIGHT	(256 + SCROLLBAR_WIDTH + 4)

#define	SCOPE_WIDTH	(256)
#define	SCOPE_MASK	(SCOPE_WIDTH - 1)
#define	WAVEFORM_WIDTH	(1600)
#define	WAVEFORM_HEIGHT	(SCOPE_HEIGHT)

#define	INFO_SIZE	(80)		/* max length of info string */
#define	MIN_BUFSIZE	(5)		/* min bufsize for looping */
#define	MAX_RECORD_TIME	(5 * 60)	/* max record time in seconds */
#define	MAX_GAIN	(100)		/* gain range */

#define	NOTICE_FILE	"alert.au"
#define	AUDIO_DEV	"/dev/audio"
#define	AUDIO_CTLDEV	"/dev/audioctl"

typedef struct Graph {
    Xv_Window       PW;
    Display        *dsp;
    XID             xid;
    GC              gc;
}               GraphType;

char		*prog;

Frame		Base_frame;
Panel		Main_panel;
Panel_item	Main_play_item;
Panel_item	Main_record_item;
Panel_item	Main_pause_item;
Panel_item	Main_playgain_item;
Panel_item	Main_recordgain_item;
Panel_item	Main_playport_item;
Panel_item	Loop_item;

Frame		Describe_frame;
Panel		Describe_panel;
Panel_item	Describe_delta_item;
Panel_item	Describe_length_item;
Panel_item	Describe_channel_item;
Panel_item	Describe_sample_item;
Panel_item	Describe_bits_item;
Panel_item	Describe_encoding_item;
Panel_item	Describe_info_item;

Panel		File_panel;
Panel_item	File_name_item;
Panel_item	File_directory_item;

Panel		Scope_canvas;

Canvas		Waveform_canvas;
void		waveform_repaint_proc();
Panel_item	Waveform_zoom_item;
Panel		Waveform_panel;

Xv_Cursor	Waveform_cursor;

Scrollbar position_sb;

short		dash_tex[] = {8, 8, 0};
short		dot_tex[] = {3, 3, 0};
short		solid_tex[] = {1, 0};
Pr_texture	solid = { solid_tex, 0};
Pr_texture	dashed = { dash_tex, 0};
Pr_texture	dotted = { dot_tex, 0};

Display *wc_display;
Drawable wc_drawable;
GC 		 wc_gc;
XGCValues wc_gcval;
Xv_Screen wc_screen;
int	wc_screen_no;

Display *sc_display;
Drawable sc_drawable;
GC 		 sc_gc;
XGCValues sc_gcval;
Xv_Screen sc_screen;
int	sc_screen_no;

Xv_Server soundtool_server;

unsigned	Zoom = 2;		/* waveform display scaling factor */
int		Waveform_update_inhibit = FALSE;
int		Show_describe;		/* TRUE if describe panel is up */
int		Refresh_ctr;		/* slows waveform refresh down a bit */
int		Loop_flag;		/* TRUE if looping */
int		Wait_flag;		/* OR of PLAY and RECORD bits */
int		Active_flag;		/* OR of PLAY and RECORD bits */
#define	PLAY	1
#define	RECORD	2

int		Audioctl_fd = -1;	/* fd for audio control device */
int		Audio_fd = -1;		/* fd for audio i/o device */
int		Alert_fd = -1;		/* fd for alert i/o */
int		Sync_sched = FALSE;	/* sigpoll_sync_handler scheduled */
Audio_info	Audio_state;		/* audio device state structure */
Audio_hdr	Device_phdr;		/* Play configuration info */
Audio_hdr	Device_rhdr;		/* Record configuration info */

struct sound_buffer {
	Audio_hdr		hdr;		/* encoding info */
	char			info[INFO_SIZE + 1]; /* info string */

	char			directory[MAXPATHLEN];
	char			filename[MAXPATHLEN];

	unsigned char		*data;		/* ptr to data buffer */
	unsigned		alloc_size;	/* size of data buffer */
	int			draining;
	int			paused;

	struct {
		char		directory[MAXPATHLEN];
		time_t		modified;
#ifdef SYSV
		struct dirent	**files;
#else
		struct direct	**files;
#endif
		Menu		menu;
	}			menu;		/* cached menu */

	struct {
		unsigned	start;		/* selected region to play */
		unsigned	end;
		unsigned	io_position;
	}			play;
	struct {
		int	cursor_pos;		/* cursor position in buffer */
		int	start;			/* marks selected region */
		int	end;
		int	position;
		int	last;
	}			display;
} Buffer;


/* Local functions */
Notify_value	timer_handler();
Notify_value	sigpoll_async_handler();
Notify_value	sigpoll_sync_handler();
extern int scope_repaint_proc();

/* External functions */
long		lseek();
#ifndef SYSV
int		alphasort();
#endif

extern char	*sys_errlist[];

/* Main entry point for soundtool demo */
main(argc, argv, envp)
	int	argc;
	char	**argv;
	char	**envp;
{
	/* Save the invocation command name for error messages */
	prog = argv[0];

	soundtool_server = xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, 0);

	soundtool_icon_image = xv_create(NULL, SERVER_IMAGE,
	    XV_WIDTH, 64,
	    XV_HEIGHT, 64, 
	    SERVER_IMAGE_DEPTH, 1, 
	    SERVER_IMAGE_BITS, soundtool_image_bits, 
	    0);

	waveform_cursor_image = xv_create(NULL, SERVER_IMAGE,
	    XV_WIDTH, 16,
	    XV_HEIGHT, 16, 
	    SERVER_IMAGE_DEPTH, 1, 
	    SERVER_IMAGE_BITS, waveform_cursor_bits, 
	    0);

	/* Create base window */
	Base_frame = xv_create((Xv_Window) 0, FRAME,
	    WIN_ROWS, 35,
	    XV_WIDTH, WINDOW_WIDTH,
	    XV_LABEL, "soundtool",
	    FRAME_ICON,
		xv_create(XV_NULL, ICON,
		    ICON_IMAGE, soundtool_icon_image,
		    ICON_TRANSPARENT, TRUE,
		    NULL),
	    0);

	if (Base_frame == (Frame) 0) {
		PERROR("Can't get base frame");
		exit(1);
	}

	/* Set up to catch SIGPOLL asynchronously */
	(void) notify_set_signal_func(Base_frame,
	    (Notify_func)sigpoll_async_handler, SIGPOLL, NOTIFY_ASYNC);

	/* Set a synchronous event handler for SIGPOLL to schedule */
	(void) notify_set_event_func(SIGPOLL,
	    (Notify_func)sigpoll_sync_handler, NOTIFY_SAFE);

	/* Open audio control device for volume controls */
	audio_control_init();
	init_buffer();

	/* Create main panels */
	main_create_panel();
	file_create_panel();
	waveform_create_panel();
	scope_create_canvas();
	waveform_create_canvas();

	/* Create the pop-up panels */
	describe_create_panel();
	window_fit_height(Base_frame);
	xv_set(Base_frame, WIN_SHOW, TRUE, 0);
    
	window_main_loop(Base_frame);

	/* Flush audio output and close the audio device, if it is open */
	closedown();
/*NOTREACHED*/
}

/* Stop everything and exit */
closedown()
{
	if ((Active_flag & PLAY) || (Wait_flag & PLAY))
		stop_play();
	if ((Active_flag & RECORD) || (Wait_flag & RECORD))
		stop_record();
	exit(0);
/*NOTREACHED*/
}

/* Routines that implement the main control panel */

/* Play/Stop button pressed */
/*ARGSUSED*/
main_play_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	unsigned int     i;

	EVENTP("main_play_proc", event);

	/* Play stops in-progress Record */
	if ((Active_flag & RECORD) || (Wait_flag & RECORD))
		stop_record();

	if (Wait_flag & PLAY) {
		stop_play();			/* never got the device */
		return;
	} else if (Active_flag & PLAY) {	/* Stop button pressed */
		stop_play();
		if (Audio_state.play.error) {
			message_display("Underflow detected during Play.");
		}
		return;
	}

	/* Turn off looping if very small selection */
	if (selectcheck() == 0)
		return;			/* no data in buffer */

	switch (audio_open(PLAY)) {
	case 0:				/* open succeeded */
		break;

	case 1:				/* open returned EBUSY */
		Wait_flag |= PLAY;	/* SIGPOLL is sent on close() */
		xv_set(Main_play_item, PANEL_LABEL_STRING, "Waiting", 0);

        /*
         * Set the 'waiting' flag, which would have been set if we
         * had done a blocking open(), in the off chance that the
         * holder of the audio device is benevolent.
         */
        i = TRUE;
        (void) audio_set_play_waiting(Audioctl_fd, &i);
		return;

	case -1:			/* open error */
	default:
		message_display("Error opening audio device.");
		return;
	}

	start_play();
}

/* Record/Stop button pressed */
/*ARGSUSED*/
main_record_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	EVENTP("main_record_proc", event);

	/* Record stops in-progress Play */
	if ((Active_flag & PLAY) || (Wait_flag & PLAY))
		stop_play();

	if (Wait_flag & RECORD) {
		stop_record();			/* never got the device */
		return;
	} else if (Active_flag & RECORD) {	/* Stop button pressed */
		stop_record();
		if (Audio_state.record.error) {
			message_display("Overflow detected during Record.");
		}
		return;
	}

	switch (audio_open(RECORD)) {
	case 0:				/* open succeeded */
		break;

	case 1:				/* open returned EBUSY */
		message_display("Audio device currently in use.");
		return;

	case -1:			/* open error */
	default:
		message_display("Error opening audio device.");
		return;
	}

	start_record();
}

/* Pause/Resume button pressed */
/*ARGSUSED*/
main_pause_proc(item, event)
     Panel_item item;
     Event *event;
{
	EVENTP("main_pause_proc", event);

	if (!Active_flag)
		return;

	/*
	 * Set the audio device state to the converse of the current state
	 * and let the SIGPOLL handler update the buttons.
	 */
	if (Active_flag & PLAY) {
		if (Buffer.paused)
			(void) audio_resume_play(Audioctl_fd);
		else
			(void) audio_pause_play(Audioctl_fd);
	}
	if (Active_flag & RECORD) {
		if (Buffer.paused)
			(void) audio_resume_record(Audioctl_fd);
		else
			(void) audio_pause_record(Audioctl_fd);
	}
}

/* Describe button pressed */
/*ARGSUSED*/
main_describe_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	int		x;

	EVENTP("main_describe_proc", event);

	if (Show_describe) {
		Show_describe = FALSE;
		(void) xv_set(Describe_frame, WIN_SHOW, FALSE, 0);
		return;
	}

	Show_describe = TRUE;
	describe_update_panel(TRUE);

	/* Place describe panel in the upper right-hand corner of the frame */
	x = (int) xv_get(Base_frame, XV_WIDTH) -
	    (int) xv_get(Describe_frame, XV_WIDTH);
	(void) xv_set(Describe_frame,
	    WIN_X,		x,
	    WIN_Y,		0,
	    WIN_SHOW,		TRUE,
	    0);
}

/* Convert local gain into device parameters */
double
scale_gain(g)
	int	g;
{
	return ((double)g / (double)MAX_GAIN);
}

/* Convert device gain into the local scaling factor */
int
unscale_gain(g)
	double	g;
{
	return (irint((double)MAX_GAIN * g));
}


/* Play volume slider moved */
/*ARGSUSED*/
main_play_volume_proc(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	double		gain;

	EVENTP("main_play_volume_proc", event);

	/* Let SIGPOLL handler adjust displayed value */
	gain = scale_gain(value);
	Audio_state.play.gain = ~0;
	(void) audio_set_play_gain(Audioctl_fd, &gain);
}

/* Record volume slider moved */
/*ARGSUSED*/
main_record_volume_proc(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	double		gain;

	EVENTP("main_record_volume_proc", event);

	/* Let SIGPOLL handler adjust displayed value */
	gain = scale_gain(value);
	Audio_state.record.gain = ~0;
	(void) audio_set_record_gain(Audioctl_fd, &gain);
}

/* 'Output to:' changed */
/*ARGSUSED*/
control_output_proc(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	unsigned	port;

	EVENTP("control_output_proc", event);

	/* Change the output port */
	Audio_state.play.port = (unsigned) value;
	port = (value == 0 ? AUDIO_SPEAKER : AUDIO_HEADPHONE);
	(void) audio_set_play_port(Audioctl_fd, &port);
}

/* 'Looping:' changed */
/*ARGSUSED*/
control_loop_proc(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	EVENTP("control_loop_proc", event);

	Loop_flag = (value != 0);
	if (Loop_flag) {
		/* Don't set Looping if tiny buffer */
		(void) selectcheck();

		/* Keep going if already draining */
		if (Active_flag & PLAY)
			Buffer.draining = FALSE;
	}
}

/*
 * Update the variable audio controls on the main panel from current state.
 * Called when any parameter changes may have occurred.
 */
main_update_panel(init)
	int		init;
{
	Audio_info	Audio_new;

	/* Only set the values if they have changed (prevents flashing) */
#define	NEWVAL(X, Y)	{						\
			if (Audio_new.X != Audio_state.X)		\
				(void) xv_set(Y, PANEL_VALUE, Audio_new.X, 0); \
			}

	if (!audio_readstate(&Audio_new) && !init)
		return;

	NEWVAL(play.gain, Main_playgain_item);
	NEWVAL(record.gain, Main_recordgain_item);
	NEWVAL(play.port, Main_playport_item);

	/* Make sure that the Pause/Resume button is in sync with reality */
	/*
	 * XXX - Pause should save state and close the audio device,
	 * in order to let other audio applications access /dev/audio.
	 * Note that, if it did close /dev/audio, 'gaintool' could no
	 * longer resume this program.
	 */
	if (Active_flag & PLAY)
		Buffer.paused = Audio_new.play.pause;
	if (Active_flag & RECORD)
		Buffer.paused = Audio_new.record.pause;

	/* If not active, assume pause state only if both flags set */
	if (!Active_flag) {
		Buffer.paused = Audio_new.play.pause && Audio_new.record.pause;
	} else if (Active_flag & PLAY) {
		/* turn off timer when paused; restart on resume */
		if (Buffer.paused)
			cancel_timer();
		else
			set_timer((double)SCOPE_WIDTH /
			    (double)Buffer.hdr.sample_rate);
	}

	/* Use the play.pause field to indicate the current button state */
	Audio_new.play.pause = Buffer.paused;

	if (Audio_new.play.pause != Audio_state.play.pause) {
		(void) xv_set(Main_pause_item, 
		    PANEL_LABEL_STRING,
		    (Buffer.paused ? "Resume" : "Pause"), 0);
	}

	/* Copy the current display state for the next time */
	Audio_state = Audio_new;
}

/* Create the main control panel */
main_create_panel()
{
	Main_panel = xv_create(Base_frame, PANEL,
	    XV_LEFT_MARGIN,	10,
	    XV_RIGHT_MARGIN,	10,
	    WIN_ROWS,		6,
	    0);

	/* Init buttons that flip values when pressed */

	Main_play_item = xv_create(Main_panel, PANEL_BUTTON,
	    XV_X, xv_col(Main_panel, 0),
	    XV_Y, xv_row(Main_panel, 0),
	    PANEL_LABEL_STRING, "Play",
	    PANEL_NOTIFY_PROC, main_play_proc,
	    0);

	Main_record_item = xv_create(Main_panel, PANEL_BUTTON,
	    XV_X, xv_col(Main_panel, 0),
	    XV_Y, xv_row(Main_panel, 1),
	    PANEL_LABEL_STRING, "Record",
	    PANEL_NOTIFY_PROC, main_record_proc,
	    0);

	Main_pause_item = xv_create(Main_panel, PANEL_BUTTON,
	    XV_X, xv_col(Main_panel, 0),
	    XV_Y, xv_row(Main_panel, 2),
	    PANEL_LABEL_STRING, "Pause",
	    PANEL_NOTIFY_PROC, main_pause_proc,
	    0);

	(void) xv_create(Main_panel, PANEL_BUTTON,
	    XV_X, xv_col(Main_panel, 0),
	    XV_Y, xv_row(Main_panel, 3),
	    PANEL_LABEL_STRING, "Describe",
	    PANEL_NOTIFY_PROC, main_describe_proc,
	    0);

	Main_playgain_item = xv_create(Main_panel, PANEL_SLIDER,
	    XV_X, xv_col(Main_panel, 14),
	    XV_Y, xv_row(Main_panel, 0),
	    PANEL_LABEL_STRING, "Play volume  ",
	    PANEL_MIN_VALUE, 0,
	    PANEL_MAX_VALUE, MAX_GAIN,
	    PANEL_SHOW_RANGE,	FALSE,
	    PANEL_SLIDER_WIDTH, MAX_GAIN + 1,
	    PANEL_NOTIFY_PROC, main_play_volume_proc,
	    PANEL_NOTIFY_LEVEL, PANEL_ALL,
	    PANEL_SHOW_RANGE,   FALSE,
	    PANEL_SHOW_VALUE,   TRUE,
	    0);

	Main_recordgain_item = xv_create(Main_panel, PANEL_SLIDER,
	    XV_X, xv_col(Main_panel, 14),
	    XV_Y, xv_row(Main_panel, 1),
	    PANEL_LABEL_STRING, "Record volume",
	    PANEL_MIN_VALUE, 0,
	    PANEL_MAX_VALUE, MAX_GAIN,
	    PANEL_SHOW_RANGE,	FALSE,
	    PANEL_SLIDER_WIDTH, MAX_GAIN + 1,
	    PANEL_NOTIFY_PROC, main_record_volume_proc,
	    PANEL_NOTIFY_LEVEL, PANEL_ALL,
	    PANEL_SHOW_RANGE,   FALSE,
	    PANEL_SHOW_VALUE,   TRUE,
	    0);

	Main_playport_item = xv_create(Main_panel, PANEL_CHOICE,
	    XV_X, xv_col(Main_panel, 14),
	    XV_Y, xv_row(Main_panel, 2),
	    PANEL_LABEL_STRING,	"Output to:",
	    PANEL_CHOICE_STRINGS, "Speaker", "Jack", 0,
	    PANEL_NOTIFY_PROC, control_output_proc,
	    0);

	Loop_item = xv_create(Main_panel, PANEL_CHOICE,
	    XV_X, xv_col(Main_panel, 14),
	    XV_Y, xv_row(Main_panel, 3),
	    PANEL_LABEL_STRING, "Looping:  ",
	    PANEL_CHOICE_STRINGS, "Off    ", "On", 0,
	    PANEL_NOTIFY_PROC, control_loop_proc,
	    0);

	/*
	 * Set initial values for volume and output from Audio_state,
	 * which should already have been initialized from the device state.
	 */
	main_update_panel(TRUE);
	window_fit_width(Main_panel);
}

/* Routines that implement the file control panel */

/* Load button pressed */
/*ARGSUSED*/
file_load_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	EVENTP("file_load_proc", event);
	if (Active_flag & RECORD)
		return;			/* no-op during record */
	soundfile_read();		/* read in the new file */
	waveform_repaint();
}

/* Store button pressed */
/*ARGSUSED*/
file_store_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	EVENTP("file_store_proc", event);

	if (Buffer.hdr.data_size == 0) {
		message_display("No sound data to store.");
		return;
	}
	soundfile_write(FALSE);
}

/* Append button pressed */
/*ARGSUSED*/
file_append_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	EVENTP("file_append_proc", event);

	if (Buffer.hdr.data_size == 0) {
		message_display("No sound data to append.");
		return;
	}
	soundfile_write(TRUE);
}

/* Event handler for 'File:' item */
Panel_setting 
file_name_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	EVENTP("file_name_proc", event);

	if (event_action(event) == ACTION_MENU && event_is_down(event)) {
		file_name_menu(event); 
	} else {
		(void) panel_default_handle_event(item, event);
	}
	return (panel_text_notify(item, event));
}

file_wait_cursor()
{
	xv_set(Base_frame, FRAME_BUSY, TRUE, 0);
}

file_restore_cursor()
{
	xv_set(Base_frame, FRAME_BUSY, FALSE, 0);
}

/* Error handler, just like it says */
file_error_handler(error_str)
	char		*error_str;
{
	generic_notice(error_str);
	file_restore_cursor();
}

file_menu_proc(menu, menu_item)
	Menu		menu;
	Menu_item	menu_item;
{
	xv_set(File_name_item, PANEL_VALUE, xv_get(menu_item, MENU_STRING), 0);
}

/*
 * File selector routine called by scandir().
 * Return TRUE if filename is an audio file  or filename ends in
 * ".au" or ".snd".
 */
int
#ifdef SYSV
file_select(entry)
	struct dirent *entry
#else
file_select(entry)
	struct direct *entry;
#endif
{
	char		*ptr;
	char		tmp[MAXPATHLEN];

	if ((strcmp(entry->d_name, ".") == 0) ||
	    (strcmp(entry->d_name, "..") == 0))
		return (FALSE);

	/* Check for .au or .snd filename extensions */
	ptr = strrchr(entry->d_name, '.');
	if ((ptr != NULL) &&
	    ((strcmp(ptr, ".au") == 0) || (strcmp(ptr, ".snd") == 0)))
		return (TRUE);

	/* Check for audio file header */
	if (Buffer.directory[0] == '\0')
		STRCPY(Buffer.directory, ".");
	SPRINTF(tmp, "%s/%s", Buffer.directory, entry->d_name);
	return (audio_isaudiofile(tmp));
}

#ifdef SYSV
/*
 * scandir() isn't included in SYSV - copy of 4.1.1 version
 */
scandir(dirname, namelist, select, dcomp)
	char		*dirname;
	struct dirent	*(*namelist[]);
	int		(*select)();
	int		(*dcomp)();
{
	register struct dirent *d, *p, **names;
	register int nitems;
	register char *cp1, *cp2;
	struct stat stb;
	long arraysz;
	DIR *dirp;

	if ((dirp = opendir(dirname)) == NULL)
		return (-1);
	if (fstat(dirp->dd_fd, &stb) < 0)
		return (-1);

	/*
	 * estimate the array size by taking the size of the directory file
	 * and dividing it by a multiple of the minimum size entry. 
	 */
	arraysz = (stb.st_size / 24);
	names = (struct dirent **)malloc(arraysz * sizeof (struct dirent *));
	if (names == NULL)
		return (-1);

	nitems = 0;
	while ((d = readdir(dirp)) != NULL) {
		if (select != NULL && !(*select)(d))
			continue;	/* just selected names */
		/*
		 * Make a minimum size copy of the data
		 */
		p = (struct dirent *)malloc(d->d_reclen);
		if (p == NULL)
			return (-1);
		p->d_ino = d->d_ino;
		p->d_reclen = d->d_reclen;
		p->d_off = d->d_off;
		for (cp1 = p->d_name, cp2 = d->d_name; *cp1++ = *cp2++; );
		/*
		 * Check to make sure the array has space left and
		 * realloc the maximum size.
		 */
		if (++nitems >= arraysz) {
			if (fstat(dirp->dd_fd, &stb) < 0)
				return (-1);	/* just might have grown */
			arraysz = stb.st_size / 12;
			names = (struct dirent **)realloc((char *)names,
			    arraysz * sizeof (struct dirent *));
			if (names == NULL)
				return (-1);
		}
		names[nitems-1] = p;
	}
	closedir(dirp);
	if (nitems && dcomp != NULL)
		qsort(names, nitems, sizeof (struct dirent *), dcomp);
	*namelist = names;
	return (nitems);
}

/*
 * Alphabetic order comparison routine for those who want it.
 */
alphasort(d1, d2)
	struct dirent **d1, **d2;
{
	return (strcmp((*d1)->d_name, (*d2)->d_name));
}
#endif /* SYSV */

/* Build and display a menu of audio files in the current directory */
file_name_menu(event)
	Event		*event;
{
	int		i;
	int		selection;
	int		count;
	int		columns;
	struct stat	st;
	Menu		menu;
	Menu_item mi;

	EVENTP("file_name_menu", event);

	/* Get the directory name in case it changed. */
	STRCPY(Buffer.directory,
	    (char *) xv_get(File_directory_item, PANEL_VALUE));

	if (Buffer.directory[0] == '\0')
		STRCPY(Buffer.directory, ".");

	/* If invalid directory, don't bother screwing around */
	if (stat(Buffer.directory, &st) < 0 || !(S_ISDIR(st.st_mode))) {
		file_error_handler("Bad directory entry");
		return;
	}

	/* If there's already a cached menu, check that it's still good */
	if (Buffer.menu.menu != NULL) {
		if ((strcmp(Buffer.directory, Buffer.menu.directory) != 0) ||
		    (st.st_mtime != Buffer.menu.modified)) {
			/* Directory changed...throw away old menu */
			if (Buffer.menu.files != NULL) {
				(void) free((char *)Buffer.menu.files);
				Buffer.menu.files = NULL;
			}
			xv_destroy(Buffer.menu.menu);
			Buffer.menu.menu = NULL;
		}
	}

	if (Buffer.menu.menu == NULL) {
		/* Change the cursor to an hourglass and build a new menu */
		file_wait_cursor();
		Buffer.menu.modified = st.st_mtime;
		STRCPY(Buffer.menu.directory, Buffer.directory);

		Buffer.menu.menu = xv_create(NULL, MENU,
		    MENU_TITLE_ITEM, "Audio files:",
		    MENU_NOTIFY_PROC, file_menu_proc,
		    0);

		count = scandir(Buffer.menu.directory, &Buffer.menu.files,
		    file_select, alphasort);
		if (count < 0)
			Buffer.menu.files = NULL;

		/* If no files found, make a non-selectable menu item */
		if (count <= 0) {
			/* create empty item else menu layout dumps core */
			mi = (Menu_item) xv_create(XV_NULL, MENUITEM,
			    MENU_STRING, "",
			    MENU_RELEASE,
			    MENU_RELEASE_IMAGE,
			    MENU_FEEDBACK, FALSE,
			    0, 0);
			(void) xv_set(Buffer.menu.menu,
			    MENU_TITLE_ITEM, "No audio files in this directory",
			    MENU_APPEND_ITEM, mi, 0);
		}
			
		/* Add a menu item for each audio file found */
		for (i = 0; i < count; i++) {
			mi = (Menu_item) xv_create(XV_NULL, MENUITEM,
			    MENU_STRING, Buffer.menu.files[i]->d_name,
			    MENU_RELEASE,
			    MENU_RELEASE_IMAGE,
			    MENU_NOTIFY_PROC, file_menu_proc,
			    0, 0);
			(void) xv_set(Buffer.menu.menu,
			    MENU_APPEND_ITEM, mi, 0);
		}

		/*
		 * Calculate the number of columns in the menu.  We want a menu
		 * that is approximately square.  We assume that the typical
		 * filename is 12 characters long and that the intercharacter
		 * distance is twice the interline distance.
		 */
		columns = sqrt((double)i / 6);
		xv_set(Buffer.menu.menu, MENU_NCOLS, columns, 0);

		/* Restore the cursor */
		file_restore_cursor();
	}

	/* Display menu, get selection (if any), and keep the menu cached */
	menu_show(Buffer.menu.menu, File_panel, event, 0); 
}

file_create_panel()
{
	extern char **environ;

	File_panel = xv_create(Base_frame, PANEL,
	    WIN_ROWS,		6,
	    WIN_RIGHT_OF,	Main_panel,
	    0);

	(void) xv_create(File_panel, PANEL_BUTTON,
	    XV_X,	xv_col(File_panel, 0),
	    XV_Y,	xv_row(File_panel, 0),
	    PANEL_LABEL_STRING, "Load",
	    PANEL_NOTIFY_PROC, file_load_proc,
	    0);

	(void) xv_create(File_panel, PANEL_BUTTON,
	    XV_X,	xv_col(File_panel, 0),
	    XV_Y,	xv_row(File_panel, 1),
	    PANEL_LABEL_STRING, "Store",
	    PANEL_NOTIFY_PROC, file_store_proc,
	    0);

	(void) xv_create(File_panel, PANEL_BUTTON,
	    XV_X,	xv_col(File_panel, 0),
	    XV_Y,	xv_row(File_panel, 2),
	    PANEL_LABEL_STRING, "Append",
	    PANEL_NOTIFY_PROC, file_append_proc,
	    0);

	File_directory_item = xv_create(File_panel, PANEL_TEXT,
	    XV_X,	xv_col(File_panel, 0),
	    XV_Y,	xv_row(File_panel, 3),
	    PANEL_VALUE, getenv("PWD="),
	    PANEL_VALUE_STORED_LENGTH, 80,
	    PANEL_VALUE_DISPLAY_LENGTH, 30,
	    PANEL_LABEL_STRING, "Directory: ",
	    0);

	File_name_item = xv_create(File_panel, PANEL_TEXT,
	    XV_X,	xv_col(File_panel, 0),
	    XV_Y,	xv_row(File_panel, 4),
	    PANEL_VALUE_STORED_LENGTH, 80,
	    PANEL_VALUE_DISPLAY_LENGTH, 30,
	    PANEL_LABEL_STRING, "File: ",
	    PANEL_EVENT_PROC,	file_name_proc,
	    0);
}

/* Allocate a buffer to hold data */
alloc_buffer(size)
	unsigned int	size;
{
	if (Buffer.data != NULL)
		(void) free((char *)Buffer.data);

	/* allocate a buffer, shrinking if request is too big */
	do {
		Buffer.data = (unsigned char *)malloc(size);
	} while ((Buffer.data == NULL) && (size = size - (size / 8)));

	Buffer.alloc_size = size;
}

/* Initialize the buffer */
init_buffer()
{
	if (Buffer.data != NULL)
		(void) free((char *)Buffer.data);
	Buffer.data = NULL;
	Buffer.alloc_size = 0;
	Buffer.hdr = Device_phdr;
	Buffer.hdr.data_size = 0;
}

/*
 * Convert the file panel directory and filename values to a path.
 * Return TRUE if no filename is specified.
 */
int
soundfile_path(str)
	char		*str;
{
	STRCPY(Buffer.directory,
	    (char *) xv_get(File_directory_item, PANEL_VALUE));
	STRCPY(Buffer.filename, (char *) xv_get(File_name_item, PANEL_VALUE));

	if (Buffer.filename[0] == '\0')
		return (TRUE);

	/* Need to do this in case user cleared the directory string */
	if (Buffer.directory[0] == '\0')
		STRCPY(Buffer.directory, ".");

	SPRINTF(str, "%s/%s", Buffer.directory, Buffer.filename);
	return (FALSE);
}

/* Open the named sound file and read it into memory.  Return TRUE if error. */
int
soundfile_read()
{
	int		fd;
	unsigned	size;
	int		valid;
	char		msg[256];
	char		path[MAXPATHLEN];
	struct stat	st;

	/* Get the soundfile path and make sure there's a filename */
	if (soundfile_path(path)) {
		message_display("No filename specified.");
		return;
	}

	DEBUGF(("Load file >%s<\n", path));

	if (((fd = open(path, O_RDONLY)) < 0) || (fstat(fd, &st) < 0)) {
		SPRINTF(msg, "Can't read '%s' (%s).", path, sys_errlist[errno]);
		message_display(msg);
		return;
	}

	/*
	 * If the soundfile has a header, read it in and decode it.
	 */
	valid = (AUDIO_SUCCESS == audio_read_filehdr(fd, &Buffer.hdr,
	    Buffer.info, sizeof (Buffer.info)));
	if (valid) {
		if (Buffer.hdr.data_size == AUDIO_UNKNOWN_SIZE) {
			/* Calculate the data size, if not already known */
			Buffer.hdr.data_size =
			    st.st_size - lseek(fd, 0L, SEEK_CUR);
		}
	} else {
		/* If no header, read the file raw and assume compatibility */
		Buffer.hdr  = Device_phdr;	/* use device configuration */
		(void) lseek(fd, 0L, L_SET);	/* rewind file */
		Buffer.hdr.data_size = st.st_size - lseek(fd, 0L, SEEK_CUR);
		Buffer.info[0] = '\0';
	}

	/* Set info string in display */
	
	xv_set(Describe_info_item, PANEL_VALUE, Buffer.info, 0);

	/*
	 * If active output, set draining flag so that play_service() won't
	 * try to access the obsolete buffer.  file_update() will turn
	 * off the draining flag if output is still active.
	 */
	if (Active_flag & PLAY)
		Buffer.draining = TRUE;

	/* Release the old buffer and allocate a new one to hold the data */
	size = Buffer.hdr.data_size;
	alloc_buffer(size);

	/* Read in as much data as possible and close the file. */
	Buffer.hdr.data_size = read(fd,
	    (char *)Buffer.data, (int)Buffer.alloc_size);
	(void) close(fd);

	Buffer.display.start = Buffer.play.start = 0;
	Buffer.display.end = Buffer.play.end = Buffer.hdr.data_size;
	file_update();			/* display new file */

	/* If we could not allocate or load the whole file, show msg */
	if (size != Buffer.hdr.data_size) {
		SPRINTF(msg, "%.2f seconds of data truncated from '%s'.",
		    audio_bytes_to_secs(&Buffer.hdr,
		    (size - Buffer.hdr.data_size)), path);
		message_display(msg);
	}

	/* If file is not an audio file, display a warning */
	if (!valid) {
		SPRINTF(msg, "'%s' is not a valid audio file.  %s\n", path,
		    "STORE will convert it to audio file format.");
		message_display(msg);
	} else if ((Buffer.hdr.encoding != AUDIO_ENCODING_ULAW) ||
	    (Buffer.hdr.bytes_per_unit != 1) ||
	    (Buffer.hdr.samples_per_unit != 1) ||
	    (Buffer.hdr.channels != 1)) {
		SPRINTF(msg,
	    "'%s' audio encoding cannot be played or displayed properly.\n",
		    path);
		message_display(msg);
	}
}

/* Write or append to the named audio file */
soundfile_write(append)
	int		append;	/* TRUE if append to existing file */
{
	int		fd;
	int		bytes;
	int		exists;
	int		err;
	char		*info;
	struct stat	st;
	char		msg[256];
	char		path[MAXPATHLEN];
	Audio_hdr	tmphdr;

	bytes = buffer_selected_size();
	if (bytes <= 0)
		return;

	/* Get the soundfile path and make sure there's a filename */
	if (soundfile_path(path)) {
		message_display("No filename specified.");
		return;
	}
	exists = (stat(path, &st) == 0);
	if (!exists)
		append = FALSE;

	DEBUGF(("%s (%d bytes) to file >%s<\n",
	    (append ? "Appending" : "Writing"), bytes, Buffer.filename));

	if (append) {
		/* Append to existing file */
		if ((fd = open(path, O_RDWR)) < 0) {
			SPRINTF(msg, "Can't open '%s' (%s).",
			    Buffer.filename, sys_errlist[errno]);
			message_display(msg);
			return;
		}

		/* Make sure this is already an audio file */
		if (!S_ISREG(st.st_mode) ||
		    (audio_read_filehdr(fd, &tmphdr, (char *)NULL, 0) !=
		    AUDIO_SUCCESS)) {
			SPRINTF(msg, "'%s' is not a valid audio file.",
			    Buffer.filename, sys_errlist[errno]);
			message_display(msg);
			goto closerr;
		}

		if ((int)lseek(fd, st.st_size, SEEK_SET) < 0)
			goto writerr;

	} else {
		/* Create new file */
		if (exists) {
			if (!message_confirm(
			    "Existing file will be overwritten."))
				goto closerr;
		}

		/* Get current info string */
		info = (char *) xv_get(Describe_info_item, PANEL_VALUE);

		fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);

		/* write an audio file header first */
		tmphdr = Buffer.hdr;
		tmphdr.data_size = bytes;
		if ((fd < 0) || (audio_write_filehdr(fd, &tmphdr,
		    info, ((unsigned)strlen(info) + 1)) != AUDIO_SUCCESS))
			goto writerr;
	}

	err = (write(fd, (char *)&Buffer.data[Buffer.play.start], bytes) !=
	    bytes);
	if (append && !err) {
		if ((tmphdr.data_size != AUDIO_UNKNOWN_SIZE) &&
		    (audio_rewrite_filesize(fd, (tmphdr.data_size + bytes)) !=
		    AUDIO_SUCCESS))
			err++;
	}

	if (err) {
writerr:
		SPRINTF(msg, "Can't %s to '%s' (%s).",
		    (append ? "append" : "write"), Buffer.filename,
		    sys_errlist[errno]);
		message_display(msg);
	}
closerr:
	(void) close(fd);
}

/* Routines that implement the file description pop-up panel */

/* Called when the describe pop-up is destroyed */
/*ARGSUSED*/
Notify_value
describe_destroy_proc(frame)
	Frame		*frame;
{
	(void) xv_set(Describe_frame, WIN_SHOW, FALSE, 0);
	Show_describe = FALSE;
	return (NOTIFY_DONE);
}

/* Null event proc to make items readonly */
/*ARGSUSED*/
null_panel_event(item, event)
	Panel_item	item;
	Event		*event;
{
}

/* Initialize the pop-up panel */
describe_create_panel()
{
#define	DWIDTH	25

	Describe_frame = xv_create(Base_frame, FRAME,
	    XV_LABEL,		"           Audio Buffer Status",
	    FRAME_SHOW_LABEL,	TRUE,
	    FRAME_DONE_PROC,	describe_destroy_proc,
	    XV_LEFT_MARGIN,	10,
	    XV_WIDTH,		xv_col(Base_frame, 15 + DWIDTH),
	    0);

	Describe_panel = xv_create(Describe_frame, PANEL,
	    0);

	Describe_sample_item = xv_create(Describe_panel, PANEL_TEXT,
	    XV_X,		xv_col(Describe_panel, 0),
	    XV_Y,		xv_row(Describe_panel, 0),
	    PANEL_VALUE_X,	xv_col(Describe_panel, 15),
	    PANEL_VALUE_DISPLAY_LENGTH,	DWIDTH,
	    PANEL_LABEL_STRING,	"Sample Rate:  ",
	    PANEL_EVENT_PROC,	null_panel_event,
	    0);

	Describe_channel_item = xv_create(Describe_panel, PANEL_TEXT,
	    XV_X,		xv_col(Describe_panel, 0),
	    XV_Y,		xv_row(Describe_panel, 1),
	    PANEL_VALUE_X,	xv_col(Describe_panel, 15),
	    PANEL_VALUE_DISPLAY_LENGTH,	DWIDTH,
	    PANEL_LABEL_STRING,	"Channels:     ",
	    PANEL_EVENT_PROC,	null_panel_event,
	    0);

	Describe_bits_item = xv_create(Describe_panel, PANEL_TEXT,
	    XV_X,		xv_col(Describe_panel, 0),
	    XV_Y,		xv_row(Describe_panel, 2),
	    PANEL_VALUE_X,	xv_col(Describe_panel, 15),
	    PANEL_VALUE_DISPLAY_LENGTH,	DWIDTH,
	    PANEL_LABEL_STRING,	"Precision:    ",
	    PANEL_EVENT_PROC,	null_panel_event,
	    0);

	Describe_encoding_item = xv_create(Describe_panel, PANEL_TEXT,
	    XV_X,		xv_col(Describe_panel, 0),
	    XV_Y,		xv_row(Describe_panel, 3),
	    PANEL_VALUE_X,	xv_col(Describe_panel, 15),
	    PANEL_VALUE_DISPLAY_LENGTH,	DWIDTH,
	    PANEL_LABEL_STRING,	"Encoding:     ",
	    PANEL_EVENT_PROC,	null_panel_event,
	    0);

	Describe_length_item = xv_create(Describe_panel, PANEL_TEXT,
	    XV_X,		xv_col(Describe_panel, 0),
	    XV_Y,		xv_row(Describe_panel, 4),
	    PANEL_VALUE_X,	xv_col(Describe_panel, 15),
	    PANEL_VALUE_DISPLAY_LENGTH,	DWIDTH,
	    PANEL_LABEL_STRING,	"Total Length: ",
	    PANEL_EVENT_PROC,	null_panel_event,
	    0);

	Describe_delta_item = xv_create(Describe_panel, PANEL_TEXT,
	    XV_X,		xv_col(Describe_panel, 0),
	    XV_Y,		xv_row(Describe_panel, 5),
	    PANEL_VALUE_X,	xv_col(Describe_panel, 15),
	    PANEL_VALUE_DISPLAY_LENGTH,	DWIDTH,
	    PANEL_LABEL_STRING,	"Selection:    ",
	    PANEL_EVENT_PROC,	null_panel_event,
	    0);

	Describe_info_item = xv_create(Describe_panel, PANEL_TEXT,
	    XV_X,		xv_col(Describe_panel, 0),
	    XV_Y,		xv_row(Describe_panel, 6),
	    PANEL_VALUE_X,	xv_col(Describe_panel, 15),
	    PANEL_VALUE_STORED_LENGTH,	(INFO_SIZE - 1),
	    PANEL_VALUE_DISPLAY_LENGTH,	DWIDTH,
	    PANEL_LABEL_STRING,	"Info string:  ",
	    0);

	/* Put the caret on the info string */
	(void) panel_backup_caret(Describe_panel);

	window_fit(Describe_panel);
	window_fit(Describe_frame);
}

/*
 * Update the description panel with the description of the file we have
 * in memory.
 * If 'init' is TRUE, set all values.  Otherwise, just set length.
 */
describe_update_panel(init)
	int	init;
{
	char	sample_string[80];
	char	bit_string[80];
	char	channel_string[80];
	char	encoding_string[80];
	char	length_string[80];

	/* If not visible, don't bother */
	if (!Show_describe)
		return;

	if (init) {
		SPRINTF(sample_string, "%d", Buffer.hdr.sample_rate);
		SPRINTF(bit_string, "%d", ((8 * Buffer.hdr.bytes_per_unit) /
		    Buffer.hdr.samples_per_unit));
		SPRINTF(channel_string, "%d", Buffer.hdr.channels);

		switch (Buffer.hdr.encoding) {
		case AUDIO_ENCODING_ULAW:
			STRCPY(encoding_string, "u-law");
			break;
		case AUDIO_ENCODING_ALAW:
			STRCPY(encoding_string, "A-law");
			break;
		default:
			SPRINTF(encoding_string, "unknown (%d)",
			    Buffer.hdr.encoding);
			break;
		}

		xv_set(Describe_sample_item,
			PANEL_VALUE, sample_string,
			0);
				
		xv_set(Describe_bits_item,
			PANEL_VALUE, bit_string,
			0);
				
		xv_set(Describe_channel_item,
			PANEL_VALUE, channel_string,
			0);
				
		xv_set(Describe_encoding_item,
			PANEL_VALUE, encoding_string,
			0);
	}
	if (Buffer.hdr.data_size == 0) {
		STRCPY(length_string, "(buffer empty)");
	} else {
		char	tmp[AUDIO_MAX_TIMEVAL];

		SPRINTF(length_string, "%s",
		    audio_secs_to_str(audio_bytes_to_secs(&Buffer.hdr,
		    Buffer.hdr.data_size), tmp, 2));
	}
	
	xv_set(Describe_length_item, PANEL_VALUE, length_string, 0);
}

/* Routines that implement the waveform display control panel */

/* Zoom slider value changed */
/*ARGSUSED*/
waveform_zoom_proc(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	EVENTP("waveform_zoom_proc", event);

	if (value != Zoom) {
		waveform_set_zoom((unsigned)value);
	}
}

/* Initalize the waveform control panel */
waveform_create_panel()
{
	Waveform_panel = xv_create(Base_frame, PANEL,
	    WIN_BELOW,	Main_panel,
	    WIN_X,	0,
	    0);

	Waveform_zoom_item = xv_create(Waveform_panel, PANEL_SLIDER,
	    PANEL_LABEL_X,	xv_col(Waveform_panel, 25),
	    PANEL_LABEL_STRING, "Zoom",
	    PANEL_MIN_VALUE,	1,
	    PANEL_MAX_VALUE,	2,
	    PANEL_VALUE,	2,
	    PANEL_SHOW_VALUE,	TRUE,
	    PANEL_SHOW_RANGE,	FALSE,
	    PANEL_SLIDER_WIDTH,	WINDOW_WIDTH - SCOPE_WIDTH - 100,
	    PANEL_NOTIFY_PROC,	waveform_zoom_proc,
	    PANEL_NOTIFY_LEVEL, PANEL_ALL,
	    0);

	window_fit_height(Waveform_panel);
}

/* Routines that implement the oscilloscope display canvas */

/* Called to repair damage to the scope canvas */
/*ARGSUSED*/
scope_repaint_proc(canvas, pw, repaint_area)
	Canvas		canvas;
	Pixwin		*pw;
	Rectlist	*repaint_area;
{
	DEBUGF(("Repaint scope from %d, %d to %d, %d\n",
	    repaint_area->rl_bound.r_left,
	    repaint_area->rl_bound.r_top,
	    repaint_area->rl_bound.r_width,
	    repaint_area->rl_bound.r_height));

	/* We could speed this up by just redrawing the damaged area. */
	update_scope();
}

/* Initialize the oscilloscope canvas */
scope_create_canvas()
{
	Scope_canvas = xv_create(Base_frame, CANVAS,
	    CANVAS_WIDTH,		SCOPE_WIDTH,
	    CANVAS_HEIGHT,		SCOPE_HEIGHT,
	    XV_WIDTH,			SCOPE_WIDTH,
	    XV_HEIGHT,			SCOPE_HEIGHT,
	    CANVAS_REPAINT_PROC,	scope_repaint_proc,
	    WIN_BELOW,			Waveform_panel,
	    0);

	sc_display = (Display *) xv_get(Base_frame, XV_DISPLAY);
	sc_drawable = (Drawable) xv_get(Base_frame, XV_XID);
	sc_screen = (Xv_Screen) xv_get(Base_frame, XV_SCREEN);
	sc_screen_no = xv_get(sc_screen, SCREEN_NUMBER);
	sc_gcval.foreground = BlackPixel(sc_display, sc_screen_no);
	sc_gcval.background = WhitePixel(sc_display, sc_screen_no);
	sc_gc = XCreateGC(sc_display, sc_drawable, GCForeground|GCBackground,
	    &sc_gcval);
}

/* Clear the scope canvas */
scope_clear()
{
	(void) pw_writebackground((Xv_Window)canvas_pixwin(Scope_canvas),
	    0, 0, SCOPE_WIDTH, SCOPE_HEIGHT, PIX_SRC);

	/* reset counter to initiate waveform refresh on record */
	Refresh_ctr = 1;
}

/*
 * Redraw the scope canvas, getting data at the specified buffer offset.
 * If the cursor is active over the waveform panel, show it's position.
 */
scope_display(off)
	int		off;
{
	int		from;
	int		to;
	int		i;
	Xv_Window	pw;
	Rect		r;
	Pr_brush	brush;

	r.r_left = 0;
	r.r_top = 0;
	r.r_width = SCOPE_WIDTH;
	r.r_height = SCOPE_HEIGHT;

	pw = (Xv_Window) canvas_pixwin(Scope_canvas);

	(void) pw_writebackground(pw, 0, 0, SCOPE_WIDTH, SCOPE_HEIGHT, PIX_SRC);

	/* If the cursor is being displayed, show it in the scope, too */
	if (Buffer.display.cursor_pos > 0) {
		brush.width = 1;
		i = Buffer.display.cursor_pos - off;
		(void) pw_line((struct pixwin *)pw, i, 0, i, SCOPE_HEIGHT,
		    &brush, &solid, PIX_SET);
	}

	/* If the start or end markers are in this region, show them */
	i = Buffer.display.start - off;
	if ((i > 0) && (i < SCOPE_WIDTH)) {
		brush.width = 3;
		(void) pw_line((struct pixwin *)pw, i, 0, i, SCOPE_HEIGHT,
		    &brush, &dashed, PIX_SET);
	}
	i = Buffer.display.end - off;
	if ((i > 0) && (i < SCOPE_WIDTH)) {
		brush.width = 3;
		(void) pw_line((struct pixwin *)pw, i, 0, i, SCOPE_HEIGHT,
		    &brush, &dotted, PIX_SET);
	}

	/* If the start point is before the data start, adjust the start */
	i = 0;
	if (off < 0) {
		i = -off;
		off = 0;
	}
	from = 127 - audio_u2c(Buffer.data[off++]);
	for (; i < SCOPE_WIDTH; i++) {
		if (off >= Buffer.hdr.data_size) {
			break;
		}
		to = 127 - audio_u2c(Buffer.data[off++]);
		(void) pw_vector(pw, i, from, (i + 1), to, PIX_SET, 1);
		from = to;
	}

}

/* Calculate a new range to display on the scope and repaint it */
update_scope()
{
	int		display_position;
	unsigned	s;
	int 		i;
	short		try;
	short		last;

	if (Buffer.hdr.data_size == 0)
		return;

	if (Active_flag & RECORD) {			/* Recording */
		/*
		 * Display the next piece of data.  We only want to display
		 * full panels of data.
		 */
		display_position =
		    (Buffer.hdr.data_size - SCOPE_WIDTH) & ~SCOPE_MASK;
		if (display_position < 0) {
			display_position = 0;
		}
	} else if (Active_flag & PLAY) {		/* Playing */
		/*
		 * Now display the scope waveform.  We want to display
		 * starting at a multiple of 256 (the scope window size)
		 * to keep repetitive waveforms from jittering.  If we are
		 * not to the next such boundary, then return without
		 * doing anything.
		 */
		s = 0;
		(void) audio_get_play_samples(Audio_fd, &s);
		display_position = (Buffer.play.start +
		    (s % (Buffer.play.end - Buffer.play.start)))
		    & ~SCOPE_MASK;
	} else {
		display_position = Buffer.display.last;		/* Inactive */
		scope_display(display_position);	/* repair damage */
		return;
	}

	/* Look for a rising edge at a zero-crossing */
	last = audio_u2s(Buffer.data[display_position]);
	for (i = display_position - 1;
	    (i >= 0) && (i > (display_position - (SCOPE_WIDTH / 2)));
	    i--) {
		try = audio_u2s(Buffer.data[i]);
		if ((try < last) && (last >= 0) && (try <= 0)) {
			if (last < ((try < 0) ? -try : try))
				i++;
			display_position = i;
			break;
		}
		last = try;
	}

	if (display_position != Buffer.display.last) {
		scope_display(display_position);
		Buffer.display.last = display_position;
		Refresh_ctr--;
	}
}

/* Routines that implement the waveform display canvas */

/* Called for all events concerning the waveform canvas */
/*ARGSUSED*/
void
waveform_cursor_proc(canvas, event, arg)
	Canvas		canvas;
	Event		*event;
	caddr_t		arg;
{
	int		position;
	int		cursor_loc = -1;
	static int	dragwave = -1;
	static int	dragstart = -1;
	static int	dragend = -1;
	static int	dragwhich = -1;

	EVENTP("waveform_cursor_proc", event);

	if (vuid_in_range(VUID_SCROLL, event_id(event))) {
		waveform_scrollbar_proc(position_sb, event);
		return;
	}

	if (Buffer.hdr.data_size == 0) return;

	EVENTP("waveform_cursor_proc", event);

	/* Get new mouse position */
	cursor_loc = event_x(event);
	position = event_x(event) * Zoom;
	if (position >= Buffer.hdr.data_size)
		position = Buffer.hdr.data_size - 1;
	if (position < 0)
		position = 0;

	switch (event_id(event)) {
	case KBD_DONE:
		/* If adjusting markers, finish up */
		if (dragend >= 0)
			goto dragend_done;

		Buffer.display.cursor_pos = -1;

		if ((Active_flag == 0) || Buffer.paused)
			scope_display(Buffer.display.last);
		
		break;

	/* Ordinary cursor motion in the panel updates the scope */
	case LOC_MOVE:
		dragwave = -1;
locmove:
		/* Play/Record scope takes priority */
		if ((Active_flag == 0) || Buffer.paused) {
			int	half = SCOPE_WIDTH / 2;

			/* Set a new cursor position */
			Buffer.display.cursor_pos = (cursor_loc * Zoom);

			/* Figure out what to display in the scope */
			Buffer.display.last = Buffer.display.cursor_pos - half;

			if ((Buffer.display.last + half) > Buffer.hdr.data_size)
				Buffer.display.last =
				    Buffer.hdr.data_size - half;

			scope_display(Buffer.display.last);
		}
		break;

	case LOC_DRAG:
		/* If dragging a selection, update the scope */
		if (dragend >= 0) {
			dragend = position;
			goto locmove;
		}

		if (dragwave >= 0) {
			/*
			 * move dragwave so that it is under the cursor.
			 */
			int	newstart,
				view_length,
				object_size,
				view_start;

			view_length = (int)xv_get(Waveform_canvas, XV_WIDTH);
			object_size = Buffer.hdr.data_size / Zoom;
			view_start = Buffer.display.position / Zoom;

			newstart = dragwave - event_x(event) + view_start;

			if (newstart < 0) newstart = 0;

			if ((newstart + view_length) <= object_size)
				scrollbar_scroll_to(position_sb, newstart);
			break;
		}
		break;

	case MS_MIDDLE:
		if (dragend >= 0)
			break;
		if (event_is_down(event))
			dragwave = cursor_loc;
		else
			dragwave = -1;
		break;

	case MS_LEFT:
	case MS_RIGHT:
		if (dragwave >= 0)
			break;

		/* If down event, set the start point and drag the end point */
		if (event_is_down(event)) {
			if (event_id(event) == MS_LEFT) {
				Buffer.display.end = -1;
				Buffer.display.start = position;
				dragwhich = MS_LEFT;
			} else {
				Buffer.display.start = -1;
				Buffer.display.end = position;
				dragwhich = MS_RIGHT;
			}
			dragstart = cursor_loc;
			dragend = position;
			waveform_repaint();
			break;
		}

		/* If up event, do nothing if there was no down */
		if (dragend < 0)
			break;

		/* Up event (or exiting the window) ends selection */

dragend_done:

		/* Very short selections are probably sloppy mouse clicks */
		position = abs(cursor_loc - dragstart);

		if (dragwhich == MS_LEFT) {
			if (position > 3)
				Buffer.display.end = dragend;
			else
				Buffer.display.end = Buffer.play.end;
		} else {
			if (position > 3)
				Buffer.display.start = dragend;
			else
				Buffer.display.start = Buffer.play.start;
		}

		dragstart = -1;
		dragend = -1;

		/* Swap cursors in case of crossover */
		if (Buffer.display.start > Buffer.display.end) {
			position = Buffer.display.start;
			Buffer.display.start = Buffer.display.end;
			Buffer.display.end = position;
		}

		Buffer.play.start = Buffer.display.start;
		Buffer.play.end = Buffer.display.end;

		/* We could speed this up by just redrawing the cursors */
		waveform_repaint();

		if ((Active_flag == 0) || Buffer.paused)
			scope_display(Buffer.display.last);

		/* Turn off looping if very small selection */
		(void) selectcheck();

		/* Give Play a chance to sync back up */
		play_update_cursor();
		describe_delta_update();
	default:
		break;
	}
}

/* Recalculate waveform display parameters when Zoom changes */
waveform_set_zoom(new_zoom)
	unsigned int	new_zoom;
{
	long		obj_length,
			win_width,
			view_size,
			view_start,
			view_end,
			cursor_start,
			cursor_end,
			disp_center;
	int		start_visible,
			end_visible;

	cursor_start	= Buffer.display.start;
	cursor_end	= Buffer.display.end;
	view_start	= Buffer.display.position;
	win_width	= (int)xv_get(Waveform_canvas, XV_WIDTH);
	view_size	= win_width * Zoom;
	view_end	= view_start + view_size;
	obj_length	= Buffer.hdr.data_size;
	disp_center	= view_start + (view_size / 2);

	start_visible = ((cursor_start >= view_start) &&
	    (cursor_start < view_end));

	end_visible = ((cursor_end >= view_start) && (cursor_end < view_end));

	/* Recalculate the displayed data descriptors */
	Zoom = new_zoom;
	view_size = win_width * Zoom;
	view_start = disp_center - (view_size / 2);
	view_end = view_start + view_size;

	/*
	 * If one of the cursors is on the display, then try to keep
	 * it on the display after zoom changes. The start cursor
	 * has priority over the end cursor.
	 */
	if (end_visible && (cursor_end > view_end)) {
		view_start = cursor_end - view_size + 1;
		view_end = view_start + view_size;
		disp_center = view_start + (view_size / 2);
	}

	if (start_visible && (cursor_start < view_start)) {
		view_start = cursor_start;
		view_end = view_start + view_size;
		disp_center = view_start + (view_size / 2);
	}

	/*
	 * If trying to display beyond start or end of data, shift the view.
	 */
	if (view_end > obj_length) {
		view_end = obj_length;
		view_start = view_end - view_size;
		disp_center = view_start + (view_size / 2);
	}
	if (view_start < 0) {
		view_start = 0;
		view_end = view_size;
		disp_center = view_start + (view_size / 2);
	}
	Buffer.display.position = view_start;

	/* Update the scrollbar and canvas offsets */
	(void) xv_set(position_sb,
	    SCROLLBAR_OBJECT_LENGTH, obj_length / Zoom,
	    SCROLLBAR_VIEW_START, view_start / Zoom,
	    0);

	pw_set_x_offset(canvas_pixwin(Waveform_canvas), view_start / Zoom);

	/* This may trigger a repaint of the canvas.  Inhibit it. */

	Waveform_update_inhibit = TRUE;
	xv_set(Waveform_canvas, CANVAS_WIDTH, obj_length / Zoom, 0);

	Waveform_update_inhibit = FALSE;

	/* Now force a repaint */
	waveform_repaint();
}

/*ARGSUSED*/
waveform_scrollbar_proc(sb, event)
	Scrollbar	sb;
	Event		*event;
{
	int		position;

	EVENTP("waveform_scrollbar_proc", event);

	position = (int)xv_get(sb, SCROLLBAR_VIEW_START);

	if (Buffer.display.position != position * Zoom) {
		Buffer.display.position = Zoom * position;
	}
	/* Canvas repaint routine is automatically called by scrollbar code */
}

waveform_repaint()
{
	Rectlist repaint;
	Rect *temp;
	Xv_Window pw;

	/* Get the pixwin and rectlist for the entire canvas */
	
	pw = canvas_paint_window(Waveform_canvas);

	temp = (Rect*) xv_get(Waveform_canvas, CANVAS_VIEWABLE_RECT, pw);
	repaint.rl_bound.r_left = temp -> r_left;
	repaint.rl_bound.r_top = temp -> r_top;
	repaint.rl_bound.r_width = temp -> r_width;
	repaint.rl_bound.r_height = temp -> r_height;

	waveform_repaint_proc(Waveform_canvas, pw, &repaint);
}

/*
 * This routine is called to repair damage to the waveform canvas.
 * Redraw the waveform according the current scaling factor.
 */
/*ARGSUSED*/
void
waveform_repaint_proc(canvas, pw, repaint_area)
	Canvas			canvas;
	Xv_Window		pw;
	Rectlist		*repaint_area;
{
	int			i;
	Rect			*rp;
	Pr_brush		brush;
	int			pt1,
				pt2,
				from,
				to;

	if ((Buffer.data == NULL) || Waveform_update_inhibit)
		return;

	rp = &repaint_area->rl_bound;

	DEBUGF(("Repaint waveform from %d to %d\n",
	    rp->r_left, rp->r_left+rp->r_width));

	brush.width = 3;

	/* Clear the damaged region */
	(void) pw_writebackground(pw, rp->r_left, rp->r_top,
	    rp->r_left + rp->r_width, rp->r_top + rp->r_height, PIX_SRC);

	pt1 = (rp->r_left * Zoom);

	/* Draw the start and end markers */
	if (Buffer.display.start >= 0) {
		i = Buffer.display.start / Zoom;
		(void) pw_line((struct pixwin *)pw, i, rp->r_top, i,
		    rp->r_top + rp->r_height, &brush, &dashed, PIX_SET);
	}
	if (Buffer.display.end >= 0) {
		i = Buffer.display.end / Zoom;
		(void) pw_line((struct pixwin *)pw, i, rp->r_top, i,
		    rp->r_top + rp->r_height, &brush, &dotted, PIX_SET);
	}

	for (i = 0; i < rp->r_width; i++, pt1 = pt2) {
        if (i >= Buffer.display.end)
            break;

        pt2 = pt1 + Zoom;
		from = 127 - audio_u2c(Buffer.data[pt1]);
		to = 127 - audio_u2c(Buffer.data[pt2]);

		(void) pw_vector(pw,
		    rp->r_left+i, from, rp->r_left+i+1, to, PIX_SET, 1);
	}

}


/* Create the canvas in which is displayed the entire waveform */
waveform_create_canvas()
{
	Xv_Window win;
	Xv_singlecolor fg, bg, *cmsp;

	Waveform_canvas = xv_create(Base_frame, CANVAS,
	    XV_WIDTH,			WAVEFORM_WIDTH,
	    XV_HEIGHT,			WAVEFORM_HEIGHT,
	    WIN_BELOW,			Waveform_panel,
	    WIN_RIGHT_OF,		Scope_canvas,
	    OPENWIN_AUTO_CLEAR,		FALSE,
	    CANVAS_FIXED_IMAGE,		FALSE,
		CANVAS_RETAINED,		FALSE,
	    CANVAS_REPAINT_PROC,	waveform_repaint_proc,
	    CANVAS_AUTO_SHRINK,		FALSE,
	    0);
	
	position_sb = xv_create(Waveform_canvas, SCROLLBAR,
	    SCROLLBAR_DIRECTION, SCROLLBAR_HORIZONTAL,
	    0);

	wc_display = (Display *) xv_get(Base_frame, XV_DISPLAY);
	wc_drawable = (Drawable) xv_get(Base_frame, XV_XID);
	wc_screen = (Xv_Screen) xv_get(Base_frame, XV_SCREEN);
	wc_screen_no = xv_get(wc_screen, SCREEN_NUMBER);
	wc_gcval.foreground = BlackPixel(wc_display, wc_screen_no);
	wc_gcval.background = WhitePixel(wc_display, wc_screen_no);
	wc_gc = XCreateGC(wc_display, wc_drawable, GCForeground|GCBackground,
	    &wc_gcval);

	cmsp = (Xv_singlecolor*) xv_get(Base_frame, FRAME_FOREGROUND_COLOR);
	fg.red = cmsp -> red;
	fg.green = cmsp -> green;
	fg.blue = cmsp -> blue;

	cmsp = (Xv_singlecolor*) xv_get(Base_frame, FRAME_BACKGROUND_COLOR);
	bg.red = cmsp -> red;
	bg.green = cmsp -> green;
	bg.blue = cmsp -> blue;

	Waveform_cursor = xv_create(Waveform_panel, CURSOR,
	    CURSOR_IMAGE, waveform_cursor_image,
	    CURSOR_FOREGROUND_COLOR, &fg,
	    CURSOR_BACKGROUND_COLOR, &bg,
	    0);
	
	win = xv_get(Waveform_canvas, CANVAS_NTH_PAINT_WINDOW, 0);

	xv_set(win,
	    WIN_CURSOR,         Waveform_cursor,
	    WIN_EVENT_PROC,	waveform_cursor_proc,
	    WIN_CONSUME_EVENTS,	
		WIN_MOUSE_BUTTONS,
		LOC_DRAG,
		LOC_MOVE,
		WIN_IN_TRANSIT_EVENTS,
		0,
	    0);
}

/* Routines that implement Play and Record */

/* Start playing data (audio device is open) */
start_play()
{
	/* Verify buffer */
	if (selectcheck() <= 0) {
		stop_play();
		return;
	}
	xv_set(Main_play_item, PANEL_LABEL_STRING, "Stop", 0);

	Buffer.play.io_position = Buffer.play.start;
	Buffer.display.last = -1;
	Buffer.draining = FALSE;

	scope_clear();
	Active_flag |= PLAY;

	/* Set a timer to go off around once per scope width */
	set_timer((double)SCOPE_WIDTH / (double)Buffer.hdr.sample_rate);

	/* SIGPOLL kicks off play_service() */
	(void) kill(getpid(), SIGPOLL);
}

/* Asynchronous SIGPOLL handler.  Can't call hardly any SunView stuff. */
/*ARGSUSED*/
Notify_value
sigpoll_async_handler(client, sig, when)
	Notify_client		client;
	int			sig;
	Notify_signal_mode	when;
{
	int			save_errno;

	DEBUGF(("sigpoll_handler: %s\n",
	    (Wait_flag ? "Waiting for open()" : "")));

	save_errno = errno;	/* XXX - workaround notifier bug */

	if (!Wait_flag) {
		/* Device is open.  Attempt to keep queues filled. */
		if (Active_flag & PLAY)
			play_service();
		if (Active_flag & RECORD)
			record_service();
	}

	/*
	 * SIGPOLL is also sent if the state of the device changed.
	 * Schedule the synchronous handler, unless it already was scheduled.
	 */
	if (!Sync_sched) {
		Sync_sched = TRUE;
		(void) notify_post_event(SIGPOLL, NULL, NOTIFY_SAFE);
	}

	errno = save_errno;	/* XXX - workaround notifier bug */
	return (NOTIFY_DONE);
}

/*
 * This is the routine that is called from the main loop that writes
 * sound to the device.
 *
 * It needs the following state data:
 *	- cursor start and end (calculate in proc)
 *	- current position in the buffer
 */
play_service()
{
	int	start;
	int	end;
	int	outcnt;
	int	rtn;

	if (Buffer.draining)
		return;
again:
	start = Buffer.play.io_position;
	end = Buffer.play.end;

	outcnt = end - start;

	while (outcnt > 0) {
		/* write as much data as possible */
		rtn = write(Audio_fd, (char *)&Buffer.data[start], outcnt);

		if (rtn > 0) {
			outcnt -= rtn;
			start += rtn;
			Buffer.play.io_position = start;
		} else {
			break;
		}
	}

	/* Check for end of sound condition. */
	if (outcnt == 0) {
		if (Loop_flag && (Buffer.play.start < Buffer.play.end)) {
			Buffer.play.io_position = Buffer.play.start;
			goto again;
		} else if (!Buffer.draining) {
			Buffer.draining = TRUE;	/* drain if not looping */
			/* Write EOF marker */
			(void) write(Audio_fd, (char *)&Buffer.data[0], 0);
		}
	}
}

/* This routine is called when the buffer markers are changed */
play_update_cursor()
{
	Audio_info	tmpinfo;

	/* No need to take action if not actually playing */
	if (!(Active_flag & PLAY))
		return;

	/* If start and end points are the same, catch it here */
	if (Buffer.play.end <= Buffer.play.start) {
		stop_play();
		return;
	}

	/* Turn off the SIGPOLL handler for now */
	Active_flag &= ~PLAY;

	/* Flush queue and start over */
	(void) audio_flush_play(Audio_fd);
	Buffer.draining = FALSE;
	Buffer.play.io_position = Buffer.play.start;
	Active_flag |= PLAY;

	/* Reset sample, error, and eof counts.  SIGPOLL will kick play. */
	AUDIO_INITINFO(&tmpinfo);
	tmpinfo.play.eof = 0;
	tmpinfo.play.error = 0;
	tmpinfo.play.samples = 0;
	(void) audio_setinfo(Audio_fd, &tmpinfo);
}

/*  This routine is used when we stop playing sound, for whatever reason. */
stop_play()
{
	unsigned	u;

	Active_flag &= ~PLAY;

	/* If waiting for device open, quit waiting now. */
	if (Wait_flag & PLAY) {
		Wait_flag &= ~PLAY;
		xv_set(Main_play_item, PANEL_LABEL_STRING, "Play", 0);
		return;
	}

	Buffer.draining = FALSE;

	/* Get latest error count */
	(void) audio_get_play_error(Audio_fd, &u);
	Audio_state.play.error = u;
	if (!Active_flag) {
		audio_flushclose();
		cancel_timer();
	}
	xv_set(Main_play_item, PANEL_LABEL_STRING, "Play", 0);
}

/* Initiate a record operation (/dev/audio is already open) */
start_record()
{
	Audio_info	tmpinfo;

	/* Pause the input and flush the read queue */
	(void) audio_pause_record(Audio_fd);
	(void) audio_flush_record(Audio_fd);

	xv_set(Main_record_item, PANEL_LABEL_STRING, "Stop", 0);

	Buffer.hdr = Device_rhdr;
	Buffer.display.position = 0;
	Buffer.display.last = -1;
	Buffer.draining = FALSE;
	Buffer.hdr.data_size = 0;
	alloc_buffer(audio_secs_to_bytes(&Buffer.hdr, (double)MAX_RECORD_TIME));

	Buffer.display.start = -1;
	Buffer.play.start = 0;
	Buffer.display.end = -1;
	Buffer.play.end = 0;
	
	xv_set(Describe_delta_item, PANEL_VALUE, " ", 0);

	/* Clear the scope/wave displays */
	
	xv_set(Waveform_zoom_item,
	    PANEL_MIN_VALUE,	1,
	    PANEL_MAX_VALUE,	2,
	    PANEL_VALUE,	2,
	    0);

	scope_clear();

	/* Restart input and reset counters.  SIGPOLL will start reading. */
	Active_flag |= RECORD;
	AUDIO_INITINFO(&tmpinfo);
	tmpinfo.record.pause = 0;
	tmpinfo.record.samples = 0;
	tmpinfo.record.error = 0;
	(void) audio_setinfo(Audio_fd, &tmpinfo);
}

/*
 * This is the routine that is called from the main loop that reads
 * sound from the device.
 */
record_service()
{
	int	read_size;
	int	rtn;

	/* This prevents an infinite recursion */
	if (Buffer.hdr.data_size >= Buffer.alloc_size)
		return;

	if (ioctl(Audio_fd, FIONREAD, &read_size) < 0) {
		PERROR("FIONREAD");
		return;
	}

	/*
	 * Don't bother if there is nothing to read,
	 * or if not much (and we're not draining input).
	 */
	if ((read_size == 0) || ((read_size < SCOPE_WIDTH) && !Buffer.draining))
		return;

	for (;;) {
		if ((Buffer.hdr.data_size + read_size) > Buffer.alloc_size) {
			read_size = Buffer.alloc_size - Buffer.hdr.data_size;
		}

		if (read_size == 0)
			break;

		rtn = read(Audio_fd,
		    (char *)&Buffer.data[Buffer.hdr.data_size], read_size);

		if (rtn < 0) {
			if (errno != EWOULDBLOCK)
				PERROR("read");
			return;
		}
		Buffer.hdr.data_size += rtn;

		if (ioctl(Audio_fd, FIONREAD, &read_size) < 0)
			return;
	}

	if (Buffer.hdr.data_size >= Buffer.alloc_size) {
		stop_record();
		message_display("Maximum record time exceeded.");
		return;
	}
}

stop_record()
{
	unsigned	u;

	Active_flag &= ~RECORD;

	/* If waiting for device open, quit waiting now. */
	if (Wait_flag & RECORD) {
		Wait_flag &= ~RECORD;
		xv_set(Main_record_item, PANEL_LABEL_STRING, "Record", 0);
		return;
	}

	/* Pause the device so that we can drain the input */
	(void) audio_pause_record(Audio_fd);

	/* Read residual input, if any */
	Buffer.draining = TRUE;
	record_service();
	Buffer.draining = FALSE;

	(void) audio_get_record_error(Audio_fd, &u);
	Audio_state.record.error = u;
	if (!Active_flag) {
		audio_flushclose();
		cancel_timer();
	}

	xv_set(Main_record_item, PANEL_LABEL_STRING, "Record", 0);

	/* Set new endpoints (if not selected during record) & redisplay */
	if (Buffer.play.end == 0) {
		Buffer.display.end = Buffer.play.end = Buffer.hdr.data_size - 1;
	}
	Buffer.display.start = Buffer.play.start;
	file_update();
	wave_update();
}

/*
 * Synchronous SIGPOLL handler...entered when the asynchronous SIGPOLL
 * handler detects something to do synchronously, like updating displays.
 */
/*ARGSUSED*/
Notify_value
sigpoll_sync_handler(client, event, arg, when)
	Notify_client		client;
	Notify_event		event;
	Notify_arg		arg;
	Notify_event_type	when;
{
	Sync_sched = FALSE;	/* flag to async handler */

	/* Waiting for the device open to succeed */
	if (Wait_flag) {
		if (!audio_open(Wait_flag)) {
			/* device is open...start transfers */
			if (Wait_flag & PLAY) {
				Wait_flag &= ~PLAY;
				start_play();
			}
			if (Wait_flag & RECORD) {
				Wait_flag &= ~RECORD;
				start_record();
			}
		}
	} else if (Active_flag & RECORD) {
		update_scope();
		describe_update_panel(FALSE);

		/* Only update the waveform once in a while. */
		if (Refresh_ctr < 0) {
			Refresh_ctr = REFRESH_RELOAD;
			/* If user adjusted zoom, stop updating */
			if ((int) xv_get(Waveform_zoom_item, PANEL_VALUE) ==
			    (int) xv_get(Waveform_zoom_item, PANEL_MAX_VALUE))
				wave_update();
		}
	}

	/* Get current audio status and update display accordingly */
	main_update_panel(FALSE);

	/*
	 * Detect whether output is complete.  The play.eof flag is
	 * incremented when a zero-length write has been processed.
	 */
	if ((Active_flag & PLAY) && Audio_state.play.eof && Buffer.draining) {
		/* Output draining is complete. */
		stop_play();
#ifdef notdef
		if (Audio_state.play.error) {
			message_display("Underflow detected during Play.");
		}
#endif
	}
	return (NOTIFY_DONE);
}

/* Timer handler...entered whenever it's time to do something */
/*ARGSUSED*/
Notify_value
timer_handler(client, which)
	Notify_client		client;
	int			which;
{
	if (Active_flag & PLAY)
		update_scope();

	return (NOTIFY_DONE);
}

/* Set a periodic timer to poll for device open availability */
set_timer(time)
	double			time;
{
	struct itimerval	timer;
	int			secs;
	int			usecs;

	DEBUGF(("init timer (%.2f seconds)\n", time));

	secs = (int)time;
	usecs = (int)((time - (double)secs) * 1000000.);

	timer.it_value.tv_usec = usecs;
	timer.it_value.tv_sec = secs;
	timer.it_interval.tv_usec = usecs;
	timer.it_interval.tv_sec = secs;
	(void) notify_set_itimer_func(Base_frame, (Notify_func)timer_handler,
	    ITIMER_REAL, &timer, ((struct itimerval *)0));
}

/* Cancel any outstanding periodic timer */
cancel_timer()
{
	(void) notify_set_itimer_func(Base_frame, (Notify_func)timer_handler,
	    ITIMER_REAL, ((struct itimerval *)0), ((struct itimerval *)0));

	DEBUGF(("cancel timer\n"));
}

/* Update a display item's value only if it is changing (prevents flashing) */
/*VARARGS1*/
set_item_val(item, val)
	Panel_item	item;
	int		val;
{
	if (val != (int) xv_get(item, PANEL_VALUE))
		xv_set(item, PANEL_VALUE, val, 0);
}

/* Update the Describe panel selection length string */
describe_delta_update()
{
	char	delta_string[(2 * AUDIO_MAX_TIMEVAL) + 8];
	char	tmp1[AUDIO_MAX_TIMEVAL];
	char	tmp2[AUDIO_MAX_TIMEVAL];

	SPRINTF(delta_string, "%s - %s",
	    audio_secs_to_str(audio_bytes_to_secs(
	    &Buffer.hdr, (unsigned) Buffer.display.start), tmp1, 2),
	    audio_secs_to_str(audio_bytes_to_secs(
	    &Buffer.hdr, (unsigned) Buffer.display.end), tmp2, 2));

	xv_set(Describe_delta_item, PANEL_VALUE, delta_string, 0);
}

/* Set up a new sound buffer and display it. */
file_update()
{
	Buffer.display.position = 0;
	Buffer.play.io_position = Buffer.play.start;

	play_update_cursor();

	/* Update the selection length in the Describe panel */
	describe_delta_update();
	describe_update_panel(TRUE);
	update_scope();
	wave_update();
}

/* wave_update - a new file has been loaded or additional data
 *	has been recorded. Redisplay the waveform panel and canvas.
 */
wave_update()
{
	int	window_width;
	int	max_zoom;
	int	min_zoom;

	window_width = ((int)xv_get(Waveform_canvas, XV_WIDTH));
	max_zoom = Buffer.hdr.data_size / window_width;
	if (max_zoom < 1)
		max_zoom = 2;

	/*
	 * XXX - Use 0x7fff, max value of positive short,
	 * because canvas coord's are signed shorts
	 */
	min_zoom = (Buffer.hdr.data_size + 0x7fff) / 0x7fff;
	if (min_zoom < 1)
		min_zoom = 1;

	xv_set(Waveform_zoom_item,
	    PANEL_MIN_VALUE,	min_zoom,
	    PANEL_MAX_VALUE,	max_zoom,
	    PANEL_VALUE,	max_zoom,
	    0);

	waveform_set_zoom((unsigned)max_zoom);
}


/* Verify the selected region of the waveform buffer and return its size */
int
buffer_selected_size()
{
	int	cnt;
	char	msg[256];

	if (Buffer.hdr.data_size == 0) {
		message_display("No data in buffer.");
		return (0);
	}

	cnt = Buffer.play.end - Buffer.play.start;

	if (cnt == 0) {
		message_display("No data in selected region.");
		return (0);
	}

	/* XXX - sanity checks for now */
	if (Buffer.play.start >= Buffer.hdr.data_size) {
		SPRINTF(msg, "Start position (%d) beyond EOF (%d).",
		    Buffer.play.start, Buffer.hdr.data_size);
		message_display(msg);
		return (-1);
	}

	if (cnt < 0) {
		SPRINTF(msg, "Start position (%d) beyond End position (%d).",
		    Buffer.play.start, Buffer.play.end);
		message_display(msg);
		return (-1);
	}
	return (cnt);
}

/*
 * If you try to loop on a very small buffer, the window system will hang.
 * This routine filters such requests.  It returns the number of samples
 * in the buffer, or -1 if the buffer is too small.
 */
selectcheck()
{
	int	samples;

	samples = buffer_selected_size();
	if (samples <= 0)
		return (samples);

	if ((samples < MIN_BUFSIZE) && Loop_flag) {
		Loop_flag = 0;
		set_item_val(Loop_item, 0);
		message_display("Looping disabled: buffer size too small.");
		return (-1);
	}
	return (samples);
}

/* Display an alert (with audio alert, if any). */
message_display(msg)
	char	*msg;
{
	int	playing;

	playing = audio_play_alert();

	notice_prompt(Base_frame, (Event *) 0,
	    NOTICE_MESSAGE_STRINGS, msg, 0,
	    NOTICE_BUTTON_YES, "Ok",
	    NOTICE_NO_BEEPING, playing,
	    0);

	if (playing)
		alert_close();
}

/*
 * Display a confirmation box (with audio alert, if any).
 * Return TRUE if Confirm, FALSE if Cancel.
 */
message_confirm(msg)
	char	*msg;
{
	int	playing;
	int	result;

	playing = audio_play_alert();

	result = notice_prompt(Base_frame, (Event *) 0,
	    NOTICE_MESSAGE_STRINGS, msg, 0,
	    NOTICE_BUTTON_YES, "Confirm",
	    NOTICE_BUTTON_NO,  "Cancel",
	    NOTICE_NO_BEEPING, playing,
	    0);

	if (playing)
		alert_close();

	return (result == NOTICE_YES);
}

/* Audio functions. */

/*
 * Open the audio device, using the Active_flag to derive open modes.
 * Returns:
 *	0	Successful open
 *	1	Audio device is busy (try again later)
 *	-1	Error during open
 */
int
audio_open(flag)
	int	flag;
{
	if (Audio_fd >= 0) {	/* already open */
		FPRINTF(stderr, "%s already open\n", AUDIO_DEV);
		return (-1);
	}

	/*
	 * Read-only access if Record.
	 * Write-only access if Play or Alert.
	 */
	flag = ((flag & RECORD) ? O_RDONLY : O_WRONLY) | O_NDELAY;

	if ((Audio_fd = open(AUDIO_DEV, flag)) < 0) {
		if ((errno == EINTR) || (errno == EBUSY))
			return (1);
		PERROR(AUDIO_DEV);
		return (-1);
	}

	flag = fcntl(Audio_fd, F_GETFL, 0) | FNDELAY;
	if (fcntl(Audio_fd, F_SETFL, flag) < 0)
		PERROR("F_SETFL fcntl");
	if (ioctl(Audio_fd, I_SETSIG, S_INPUT|S_OUTPUT|S_MSG) < 0)
		PERROR("I_SETSIG ioctl");
	return (0);
}

/* Flush queued output and close the audio device */
audio_flushclose()
{
	if (Audio_fd < 0)
		return;
	(void) audio_flush(Audio_fd);
	(void) close(Audio_fd);
	Audio_fd = -1;
}

/* Close audio device (queued output will drain) */
audio_close()
{
	if (Audio_fd < 0)
		return;			/* Already closed */
	(void) close(Audio_fd);
	Audio_fd = -1;
}

/*
 * Open audio control device (/dev/audioctl) and read its state.
 * This may be used for state get/set (eg, volume levels) without holding
 * the main audio device (/dev/audio) open.
 */
audio_control_init()
{
	/* open audio control device */
	if ((Audioctl_fd = open(AUDIO_CTLDEV, O_RDWR)) < 0) {
		PERROR(AUDIO_CTLDEV);
		Device_phdr.sample_rate = 8000;
		Device_phdr.channels = 1;
		Device_phdr.bytes_per_unit = 1;
		Device_phdr.samples_per_unit = 1;
		Device_phdr.encoding = AUDIO_ENCODING_ULAW;
		Device_rhdr = Device_phdr;
		Buffer.hdr = Device_phdr;
	} else {

		/* tell the driver to send SIGPOLL on device state changes */
		if (ioctl(Audioctl_fd, I_SETSIG, S_MSG) < 0)
			PERROR("Could not issue I_SETSIG ioctl");

		/* Get the device play & record configuration */
		if ((audio_get_play_config(Audioctl_fd, &Device_phdr) !=
		    AUDIO_SUCCESS) ||
		    (audio_get_record_config(Audioctl_fd, &Device_rhdr) !=
		    AUDIO_SUCCESS)) {
			PERROR("Could not get encoding configuration");
		}
	}
	AUDIO_INITINFO(&Audio_state);
}

/* Read the audio device state and translate fields into values we understand */
int
audio_readstate(ip)
	Audio_info	*ip;
{
	if ((Audioctl_fd > 0) &&
	    (audio_getinfo(Audioctl_fd, ip) != AUDIO_SUCCESS)) {
		/* If error, quit trying to access control device */
		Audioctl_fd = -1;
		PERROR(AUDIO_CTLDEV);
	}
	if (Audioctl_fd < 0) {
		/* Set dummy values */
		ip->play.gain = 35;
		ip->record.gain = 60;
		ip->play.port = 0;
		ip->play.eof = 0;
		ip->play.error = FALSE;
		ip->play.pause = FALSE;
		ip->record.error = FALSE;
		ip->record.pause = FALSE;
		return (FALSE);
	}

	/* Convert to values that we understand */
	ip->play.gain = unscale_gain((double)(ip->play.gain - AUDIO_MIN_GAIN) /
	    (double)AUDIO_MAX_GAIN);
	ip->record.gain = unscale_gain(
	    (double)(ip->record.gain - AUDIO_MIN_GAIN) /
	    (double)AUDIO_MAX_GAIN);
	ip->play.port = (unsigned) ((ip->play.port == AUDIO_SPEAKER) ? 0 : 1);
	return (TRUE);
}


/*
 * Queue an entire sound file (maximum 2 seconds of data) to /dev/audio.
 * Leave /dev/audio open...do not wait for output to drain.
 * Return TRUE if audio output started; else return FALSE.
 *
 * XXX - buffer should be allocated dynamically to accomodate other formats.
 */
audio_play_alert()
{
	int		fd;
	int		rtn;
	Audio_hdr	hdr;
	char		buffer[16000];

	/*
	 * If play active...don't interrupt it.
	 * If record active, we might be able to play this alert.
	 */
	if (Active_flag & PLAY)
		return (FALSE);

	/* Open, read, close the sound file */
	if ((fd = open(NOTICE_FILE, O_RDONLY)) < 0)
		return (FALSE);
	rtn = (AUDIO_SUCCESS == audio_read_filehdr(fd, &hdr, (char *)NULL, 0));
	if (rtn) {
		rtn = read(fd, buffer, sizeof (buffer));
	}
	(void) close(fd);
	if (rtn <= 0)
		return (FALSE);

	DEBUGF(("playing alert file >%s<\n", NOTICE_FILE));

	if ((Alert_fd = open(AUDIO_DEV, O_WRONLY)) < 0) {
		if ((errno != EINTR) && (errno != EBUSY))
			PERROR(AUDIO_DEV);
		return (FALSE);
	}

	if (write(Alert_fd, buffer, rtn) < 0) {
		PERROR("audio alert write");
		alert_close();
		return (FALSE);
	}
	return (TRUE);
}

/* Flush queued output and close the audio alert device */
alert_close()
{
	if (Alert_fd < 0)
		return;
	if (ioctl(Alert_fd, I_FLUSH, FLUSHW) < 0)
		PERROR("alert I_FLUSH ioctl");
	(void) close(Alert_fd);
	Alert_fd = -1;
}

generic_notice(msg1)
	char	*msg1;
{
	notice_prompt(Base_frame, (Event *)NULL,
		NOTICE_MESSAGE_STRINGS, msg1, 0,
		NOTICE_BUTTON_YES, "Continue",
		0);
}
