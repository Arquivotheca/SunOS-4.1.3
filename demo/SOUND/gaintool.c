/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to Sun Microsystems, Inc.  The name of Sun Microsystems, Inc.
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission.  This software
 * is provided ``as is'' without express or implied warranty.
 */
#ident	"@(#)gaintool.c	1.1	92/07/30 SMI"

/*
 * gaintool - Audio Control Panel
 *
 * This program provides a GUI for basic audio configuration.
 * It is furnished as an example of audio programming.
 *
 * Gaintool illustrates the separation of audio control (eg, volume controls)
 * from actual audio i/o.  In general, programs should not need to provide
 * their own controls for output volume or output port settings, as these
 * can be controlled globally from the audio control panel.
 *
 * Gaintool also provides an audio status panel that is useful for debugging
 * audio programs.  The audio status panel is brought up by either pressing
 * the PROPS (L3) key or by selecting "Status..." on the pop-up menu.
 */

#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <stropts.h>

#include <multimedia/libaudio.h>
#include <multimedia/audio_device.h>

#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/seln.h>
#include "gaintool_ui.h"


/* Macro to translate strings */
#define T(str)	(dgettext("gaintool-labels", str))


#define	AUDIO_IODEV	"/dev/audio"

#define	MAX_GAIN	100

#define	FPRINTF (void) fprintf
#define	SPRINTF (void) sprintf
#define	STRCPY  (void) strcpy


/* Define names to extract GUI components from the DevGuide structures */
#define	Base_frame		MFrame->BaseFrame
#define	Base_panel		MFrame->GainPanel
#define	play_slider		MFrame->PlaySlider
#define	record_slider		MFrame->RecordSlider
#define	monitor_slider		MFrame->MonitorSlider
#define	speaker_item		MFrame->OutputSwitch
#define	pause_item		MFrame->Pause_button

#define	Status_frame		SFrame->StatusPanel
#define	play_open_item		SFrame->Popen_flag
#define	play_active_item	SFrame->Pactive_flag
#define	play_pause_item		SFrame->Ppaused_flag
#define	play_error_item		SFrame->Perror_flag
#define	play_waiting_item	SFrame->Pwaiting_flag
#define	play_eof_item		SFrame->Peof_value
#define	play_samples_item	SFrame->Psam_value
#define	play_sample_rate_item	SFrame->Prate_value
#define	play_channels_item	SFrame->Pchan_value
#define	play_precision_item	SFrame->Pprec_value
#define	play_encoding_item	SFrame->Pencode_value
#define	record_open_item	SFrame->Ropen_flag
#define	record_active_item	SFrame->Ractive_flag
#define	record_pause_item	SFrame->Rpaused_flag
#define	record_error_item	SFrame->Rerror_flag
#define	record_waiting_item	SFrame->Rwaiting_flag
#define	record_samples_item	SFrame->Rsam_value
#define	record_sample_rate_item	SFrame->Rrate_value
#define	record_channels_item	SFrame->Rchan_value
#define	record_precision_item	SFrame->Rprec_value
#define	record_encoding_item	SFrame->Rencode_value


int			Audio_pausefd = -1;		/* real device fd */
int			Audio_fd;			/* ctl device fd */
Audio_info		Info;				/* saved state */
char*			Audio_device = AUDIO_IODEV;	/* device name */
char*			Audio_ctldev;			/* control dev name */
int			Show_status = FALSE;		/* TRUE if visible */
int			Continuous = FALSE;		/* TRUE to set timer */
char*			prog;				/* program name */


Attr_attribute			INSTANCE;
gaintool_BaseFrame_objects*	MFrame = NULL;
gaintool_StatusPanel_objects*	SFrame = NULL;
Xv_opaque			Mainmenu = NULL;


/* Global variables */
extern int		optind;
extern char*		optarg;


void
usage()
{
	FPRINTF(stderr, "%s\n\t%s ",
	    T("Audio Control Panel -- usage:"), prog);
	FPRINTF(stderr, T("[-d device]\nwhere:\n"));
	FPRINTF(stderr, T("\t-d\tAudio device name (default:/dev/audio)\n"));
	exit(1);
}

/* Convert encoding enum to a string */
void
encode_string(enc, bp)
	unsigned int	enc;
	char		*bp;
{
	switch (enc) {
	case AUDIO_ENCODING_LINEAR:
		STRCPY(bp, "linear"); break;
	case AUDIO_ENCODING_ULAW:
		STRCPY(bp, "u-law"); break;
	case AUDIO_ENCODING_ALAW:
		STRCPY(bp, "A-law"); break;
	default:
		SPRINTF(bp, "[%d]", enc); break;
	}
}

/* Open the audio control device */
void
audio_open()
{
	/* Construct the control device name by appending "ctl" */
	Audio_ctldev = (char*) malloc(strlen(Audio_device) + 4);
	SPRINTF(Audio_ctldev, "%s%s", Audio_device, "ctl");

	if ((Audio_fd = open(Audio_ctldev, O_RDWR)) < 0) {
		FPRINTF(stderr, "%s: %s ", prog, T("Cannot open"));
		perror(Audio_ctldev);
		exit(1);
	}

	/*
	 * Set the notify flag so that this program (and all others
	 * with this stream open) will be sent a SIGPOLL if changes
	 * are made to the parameters of audio device.
	 */
	if (ioctl(Audio_fd, I_SETSIG, S_MSG) < 0) {
		FPRINTF(stderr, "%s: %s ", prog, T("Cannot initialize"));
		perror(Audio_ctldev);
		exit(1);
	}

	/* Init the state structure so update_panels() will set everything */
	AUDIO_INITINFO(&Info);
}

/* Convert local gain into device parameters */
unsigned
scale_gain(g)
	unsigned	g;
{
	return (AUDIO_MIN_GAIN + (unsigned)
	    irint(((double) (AUDIO_MAX_GAIN - AUDIO_MIN_GAIN)) *
	    ((double)g / (double)MAX_GAIN)));
}

/* Convert device gain into the local scaling factor */
unsigned
unscale_gain(g)
	unsigned	g;
{
	return ((unsigned) irint((double)MAX_GAIN *
	    (((double)(g - AUDIO_MIN_GAIN)) /
	    (double)(AUDIO_MAX_GAIN - AUDIO_MIN_GAIN))));
}


/* Get current audio state */
void
getinfo(ap)
	Audio_info	*ap;
{
	if (ioctl(Audio_fd, AUDIO_GETINFO, ap) < 0) {
		FPRINTF(stderr, "%s: %s ", prog, T("Cannot access"));
		perror(Audio_ctldev);
		exit(1);
	}

	/* Set the output port to a value we understand */
	ap->play.port = (ap->play.port == AUDIO_SPEAKER ? 0 : 1);

	ap->play.gain = unscale_gain(ap->play.gain);
	ap->record.gain = unscale_gain(ap->record.gain);
	ap->monitor_gain = unscale_gain(ap->monitor_gain);
}

/*
 * Get the latest device state and update the panels appropriately.
 */
void
update_panels()
{
	Audio_info	newinfo;
	char		buf[128];

	/* Only change settings that changed, to avoid flashing */
#define	NEWVAL(X, Y)	{						\
			if (newinfo.X != Info.X)			\
			    xv_set(Y, PANEL_VALUE, newinfo.X, NULL);	\
			}

#define	Set_Statuslight(item, on)	xv_set((item),			\
					    PANEL_VALUE, ((on) ? 1 : 0), NULL);

#define	NEWSTATE(X, Y)	{						\
			if (newinfo.X != Info.X)			\
				Set_Statuslight(Y, newinfo.X);		\
			}

#define	NEWSTRING(X, Y) {						\
			if (newinfo.X != Info.X) {			\
				SPRINTF(buf, "%u", newinfo.X);		\
				xv_set(Y, PANEL_LABEL_STRING, buf, NULL); \
			} }

	/* Get the current device state */
	getinfo(&newinfo);

	/* Update main panel items */
	NEWVAL(play.gain, play_slider);
	NEWVAL(record.gain, record_slider);
	NEWVAL(monitor_gain, monitor_slider);
	NEWVAL(play.port, speaker_item);

	/*
	 * Pause is tricky, since we may be holding the device open.
	 * If 'resume', then release the device, if held.
	 */
	if (!newinfo.play.pause && (Audio_pausefd >= 0)) {
		(void) close(Audio_pausefd);
		Audio_pausefd = -1;
	}

	if (newinfo.play.pause != Info.play.pause) {
		if (newinfo.play.pause) {
			xv_set(pause_item,
			    PANEL_LABEL_STRING, T("Resume Play"), NULL);
		} else {
			xv_set(pause_item,
			    PANEL_LABEL_STRING, T("Pause Play"), NULL);
		}
	}

	/* Update status panel, if it is visible */
	if (Show_status) {
		NEWSTATE(play.open, play_open_item);
		NEWSTATE(play.pause, play_pause_item);
		NEWSTATE(play.active, play_active_item);
		NEWSTATE(play.error, play_error_item);
		NEWSTATE(play.waiting, play_waiting_item);
		NEWSTRING(play.eof, play_eof_item);
		NEWSTRING(play.sample_rate, play_sample_rate_item);
		NEWSTRING(play.channels, play_channels_item);
		NEWSTRING(play.precision, play_precision_item);

		if (newinfo.play.samples != Info.play.samples) {
			SPRINTF(buf, "%-10u", newinfo.play.samples);
			xv_set(play_samples_item,
			    PANEL_LABEL_STRING, buf, NULL);
		}
		if (newinfo.play.encoding != Info.play.encoding) {
			encode_string(newinfo.play.encoding, buf);
			xv_set(play_encoding_item,
			    PANEL_LABEL_STRING, buf, NULL);
		}

		NEWSTATE(record.open, record_open_item);
		NEWSTATE(record.active, record_active_item);
		NEWSTATE(record.pause, record_pause_item);
		NEWSTATE(record.error, record_error_item);
		NEWSTATE(record.waiting, record_waiting_item);
		NEWSTRING(record.sample_rate, record_sample_rate_item);
		NEWSTRING(record.channels, record_channels_item);
		NEWSTRING(record.precision, record_precision_item);

		if (newinfo.record.samples != Info.record.samples) {
			SPRINTF(buf, "%-10u", newinfo.record.samples);
			xv_set(record_samples_item,
			    PANEL_LABEL_STRING, buf, NULL);
		}
		if (newinfo.record.encoding != Info.record.encoding) {
			encode_string(newinfo.record.encoding, buf);
			xv_set(record_encoding_item,
			    PANEL_LABEL_STRING, buf, NULL);
		}
	}
	/* Save latest values */
	Info = newinfo;
}

/*
 * Entered any time there is a change of state of the audio device.
 * However, the Notifier schedules this routine synchronously, so
 * we don't have to worry about stomping on global data structures.
 */
/*ARGSUSED*/
Notify_value
sigpoll_handler(client, sig, when)
	Notify_client		client;
	int			sig;
	Notify_signal_mode	when;
{
	update_panels();
	return (NOTIFY_DONE);
}

/* Timer handler...entered periodically when continuous update is selected */
/*ARGSUSED*/
Notify_value
timer_handler(client, which)
	Notify_client	client;
	int		which;
{
	update_panels();
	return (NOTIFY_DONE);
}

/* Enable/Disable timer */
void
set_timer(on)
	int	on;
{
	struct itimerval	timer;

	if (on) {			/* Update Mode: Continuous */
		/* Set up to catch interval timer */
		timer.it_value.tv_usec = 1000000 / 4;		/* 1/4 second */
		timer.it_value.tv_sec = 0;
		timer.it_interval.tv_usec = 1000000 / 4;	/* 1/4 second */
		timer.it_interval.tv_sec = 0;

		(void) notify_set_itimer_func(Base_frame,
		    timer_handler, ITIMER_REAL, &timer, (struct itimerval *)0);

	} else {			/* Update Mode: Status Change */
		/* Cancel timer */
		(void) notify_set_itimer_func(Base_frame,
		    timer_handler, ITIMER_REAL,
		    (struct itimerval *)0, (struct itimerval *)0);
	}
}

/* Display the status panel */
/*ARGSUSED*/
void
show_status()
{
	/* If already up, flash or bring to front */
	if (Show_status) {
		xv_set(Status_frame, XV_SHOW, FALSE, NULL);
	} else {
		Rect*	r;
		Rect	mr, sr;
		int	sb, fp,	fhw, phw;
		int	x, y;

		/* Position status panel */
		r = (Rect *) xv_get(Base_frame, WIN_SCREEN_RECT);
		frame_get_rect(Base_frame, &mr);
		frame_get_rect(Status_frame, &sr);

		/* Center status under tool, unless off-screen */
		fp = mr.r_left;
		fhw = mr.r_width;
		phw = sr.r_width;
		x = (fhw - phw) / 2;
		if ((fp + x) < 0)
			x = -fp;
		else if ((fp + x + phw) > r->r_width)
			x = r->r_width - fp - phw;
		sr.r_left = x + fp;

		/* Put status under tool, unless tool is near screen bottom */
		sb = r->r_height;
		fp = mr.r_top;
		fhw = mr.r_height;
		phw = sr.r_height;
		if ((fp + fhw + phw) > sb)
			y = -phw;		/* move props above tool */
		else
			y = fhw;
		sr.r_top = y + fp;

		frame_set_rect(Status_frame, &sr);

		/* Start the timer, if continuous mode */
		set_timer(Continuous);
	}

	/* Init info so that update_panels() resets everything */
	AUDIO_INITINFO(&Info);
	Show_status = TRUE;
	update_panels();
	xv_set(Status_frame,
	    XV_SHOW, TRUE,
	    FRAME_CMD_PUSHPIN_IN, TRUE,
	    0);
}

/* Done callback function for `StatusPanel' */
void
StatusPanel_done_proc(frame)
	Frame		frame;
{
	/* Take down the status panel */
	xv_set(frame, XV_SHOW, FALSE, NULL);
	Show_status = FALSE;

	/* Cancel timer */
	set_timer(FALSE);
}

/* Callback for "Status..." menu item */
Menu_item
menu_status(item, op)
	Menu_item		item;
	Menu_generate		op;
{
	switch (op) {
	case MENU_NOTIFY:
		show_status();		/* bring up the status panel */
		break;
	default:
		break;
	}
	return (item);
}

Notify_value
main_event_proc(frame, event, arg, type)
	Frame			frame;
	Event			*event;
	Notify_arg		arg;
	Notify_event_type	type;
{
	if ((event_action(event) == ACTION_PROPS) && event_is_up(event)) {
		show_status();
		return (NOTIFY_DONE);
	}
	if ((event_action(event) == ACTION_MENU) && event_is_down(event)) {
		menu_show(Mainmenu, frame, event, 0);
		return (NOTIFY_DONE);
	}
	return (notify_next_event_func(frame, (Notify_event)event, arg, type));
}

/*
 * The following notify procedures only have to set the appropriate state.
 * Once set, a signal will be generated and the SIGPOLL handler will take
 * care of updating the panels.
 */

/* Notify callback function for `PlaySlider' */
/* ARGSUSED */
void
play_volume_proc(item, value, event)
	Panel_item 	item;
	int		value;
	Event		*event;
{
	Audio_info	aif;

	AUDIO_INITINFO(&aif);
	aif.play.gain = scale_gain(value);
	(void) ioctl(Audio_fd, AUDIO_SETINFO, &aif);

	/* Since the return value is quantized, always update the display */
	Info.play.gain = ~0;
}

/* Notify callback function for `RecordSlider' */
/* ARGSUSED */
void
record_volume_proc(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	Audio_info	aif;

	AUDIO_INITINFO(&aif);
	aif.record.gain = scale_gain(value);
	(void) ioctl(Audio_fd, AUDIO_SETINFO, &aif);

	/* Since the return value is quantized, always update the display */
	Info.record.gain = ~0;
}

/* Notify callback function for `MonitorSlider' */
/* ARGSUSED */
void
monitor_volume_proc(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	Audio_info	aif;

	AUDIO_INITINFO(&aif);
	aif.monitor_gain = scale_gain(value);
	(void) ioctl(Audio_fd, AUDIO_SETINFO, &aif);

	/* Since the return value is quantized, always update the display */
	Info.monitor_gain = ~0;
}

/* Notify callback function for `OutputSwitch' */
/* ARGSUSED */
void
control_output_proc(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	Audio_info	aif;

	AUDIO_INITINFO(&aif);
	aif.play.port = (value == 0 ? AUDIO_SPEAKER : AUDIO_HEADPHONE);
	(void) ioctl(Audio_fd, AUDIO_SETINFO, &aif);
}

/*
 * The Pause button only affects output.
 * When pushed, it changes to a Resume button.
 *
 * Setting the pause bit on a device that is not open has no effect.
 * If the button is pressed when the play stream of /dev/audio is closed,
 * we open the device here in order to make sure no output occurs.
 *
 * If the resume button is pressed or any other process sets 'play.resume',
 * we let go of the device in update_panels().
 *
 * Since device open/close signals a state change, update_panels()
 * will take care of switching the button image.
 */
/* ARGSUSED */
void
pause_proc(item, event)
	Panel_item	item;
	Event		*event;
{
	Audio_info	aif;

	/* If trying to pause, try to open /dev/audio first */
	if (!Info.play.pause && (Audio_pausefd < 0)) {
		Audio_pausefd = open(Audio_device, O_WRONLY | O_NDELAY);
	}

	/* Set up to set the pause bit */
	AUDIO_INITINFO(&aif);
	aif.play.pause = !Info.play.pause;
	(void) ioctl(Audio_fd, AUDIO_SETINFO, &aif);
}


/* Notify callback function for `Update_switch' */
/* ARGSUSED */
void
StatusPanel_update_proc(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	Continuous = (value != 0);
	set_timer(Continuous);
}

/* Null event callback for read-only status items */
/* ARGSUSED */
void
StatusPanel_null_event_proc(item, event)
	Panel_item	item;
	Event		*event;
{
}

/* Create the main panel */
void
GainPanel_Init()
{
	MFrame = gaintool_BaseFrame_objects_initialize(NULL, NULL);
	Mainmenu = gaintool_props_menu_create((caddr_t)MFrame, Base_frame);
	if (Mainmenu == NULL)
		MFrame = NULL;

	if (MFrame == NULL)
		return;

	/* Set the sliders very active */
	xv_set(play_slider, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);
	xv_set(record_slider, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);
	xv_set(monitor_slider, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);

	/* Set up a notify interposer */
	notify_interpose_event_func(Base_panel, main_event_proc,
	    NOTIFY_SAFE);
}

/* Create the status pin-up panel */
void
StatusPanel_Init()
{
	SFrame = gaintool_StatusPanel_objects_initialize(NULL, Base_frame);
	if (SFrame == NULL)
		return;
}
	
/* gaintool entry point */
main(argc, argv)
	int	argc;
	char	**argv;
{
	int			i;

	/* Get the program name */
	prog = strrchr(argv[0], '/');
	if (prog == NULL)
		prog = argv[0];
	else
		prog++;

	/* Initialize I18N and XView */
	INSTANCE = xv_unique_key();
	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv,
	    XV_USE_LOCALE, TRUE,
	    0);

	/* Parse start-up options */
	while ((i = getopt(argc, argv, "d:")) != EOF) switch (i) {
	case 'd':			/* device flag */
		Audio_device = optarg;
		break;
	default:
		usage();
	}

	GainPanel_Init();
	StatusPanel_Init();
	if ((MFrame == NULL) || (SFrame == NULL)) {
		FPRINTF(stderr, "%s: %s\n", prog, T("Cannot initialize XView"));
		exit(1);
	}

	/* Set up to catch SIGPOLL */
	(void) notify_set_signal_func(Base_frame, sigpoll_handler,
	    SIGPOLL, NOTIFY_SYNC);

	audio_open();
	update_panels();

	xv_main_loop(Base_frame);

	exit(0);
}
