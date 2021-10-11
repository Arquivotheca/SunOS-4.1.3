#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)win_alarm.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * win_alarm.c
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sundev/kbd.h>
#include <sundev/kbio.h>
#include <pixrect/pixrect.h>
#include <sunwindow/defaults.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/notify.h>
#include <suntool/window.h>

extern void pw_batch();

/* Bell cached defaults */
static	Bool	win_do_audible_bell;
static	Bool	win_do_visible_bell;
static	int	win_bell_done_init;

static  struct  win_alarm_data {
		Win_alarm  data_alarm;
		struct  pixwin	  *pw;
		int	windowfd;
		Rect	r;
		struct  screen    screen;
		int	kbdfd;
} wa_data;

static  Notify_client	win_alarm_handle = (Notify_client) &wa_data;
static	Notify_value	win_alarm_start();
static  Notify_value    win_alarm_stop();
static  struct  	itimerval null_itv;


/*
 * Beep and flash multiple times.
 */
win_alarm(in_alarm, windowfd, pw)
	Win_alarm	*in_alarm;
	int 		windowfd;
	register struct	pixwin *pw;
{
	struct	itimerval	wa_itv;
	struct	screen		wa_screen;
	Rect 			wa_r;
	int 			wa_kbdfd, wa_cmd, wa_beeps, wa_flashes;
	Win_alarm		wa_alarm;


	if (!in_alarm) {
		win_get_alarm(&wa_alarm);
	}
	else	wa_alarm = *in_alarm; 

	null_itv.it_value.tv_sec = 0; 
	null_itv.it_value.tv_usec = 0; 
        null_itv.it_interval.tv_sec = 0;
        null_itv.it_interval.tv_usec = 0;
	
	wa_beeps = wa_alarm.beep_num;
	wa_flashes = wa_alarm.flash_num;
	wa_itv.it_value = wa_alarm.beep_duration;
	wa_itv.it_interval.tv_sec = 0;
	wa_itv.it_interval.tv_usec = 0;
	
	wa_data.data_alarm = wa_alarm;
	wa_data.pw    = pw;
	wa_data.windowfd = windowfd;

	if (wa_beeps<=0 && wa_flashes<=0)
		return;

	/* Get defaults for bell if first time */
	if (!win_bell_done_init) {
		win_do_audible_bell = defaults_get_boolean(
		    "/SunView/Audible_Bell", (Bool)TRUE, (int *)NULL);
		win_do_visible_bell = defaults_get_boolean(
		    "/SunView/Visible_Bell", (Bool)TRUE, (int *)NULL);
		win_bell_done_init = 1;
	}
	if (windowfd >= 0 && win_do_audible_bell) {
		/* Get screen keyboard name */
		win_screenget(windowfd, &wa_screen);
		wa_data.screen = wa_screen;
	}
	if (pw && windowfd >= 0 && win_do_visible_bell) {
		/* Get pw size */
		(void)win_getsize(windowfd, &wa_r);
		wa_data.r = wa_r;
	}
	if (wa_beeps>0 || wa_flashes>0) {
		if (wa_flashes>0) {
			/* Flash pw */
			if (pw && windowfd >= 0 && win_do_visible_bell) {
				(void)pw_writebackground(pw, 0, 0,
		    		wa_r.r_width, wa_r.r_height, PIX_NOT(PIX_DST));
				pw_show(pw);
			}
		}
		if (wa_beeps>0) {
			if (windowfd >= 0 && win_do_audible_bell) {
				/* Open keyboard */
				if ((wa_kbdfd = open(wa_screen.scr_kbdname, O_RDWR, 0)) < 0)
					return;
				wa_data.kbdfd = wa_kbdfd;
				/* Start bell */
				wa_cmd = KBD_CMD_BELL;
				(void) ioctl(wa_kbdfd, KIOCCMD, &wa_cmd);
			}
		}
		/* Wait */
		notify_set_itimer_func(win_alarm_handle, win_alarm_stop,
				       ITIMER_REAL, &wa_itv, &null_itv);
	}
}

static Notify_value
win_alarm_stop(win_alarm_handle)
	Notify_client win_alarm_handle;
{
	int	cmd;
	struct	itimerval	itv;

	itv.it_value = wa_data.data_alarm.beep_duration;
        itv.it_interval.tv_sec = 0;
        itv.it_interval.tv_usec = 0;

	if (wa_data.data_alarm.beep_num>0) {
			wa_data.data_alarm.beep_num--;
			if (wa_data.windowfd >= 0 && win_do_audible_bell) {
				/* Stop bell */
				cmd = KBD_CMD_NOBELL;
				(void) ioctl(wa_data.kbdfd, KIOCCMD, &cmd);
				/* Close keyboard */
				close(wa_data.kbdfd);
			}
		}
		/* Turn off flash */
		if (wa_data.data_alarm.flash_num>0) {
			wa_data.data_alarm.flash_num--;
			if (wa_data.pw && wa_data.windowfd >= 0 && win_do_visible_bell) {
				(void)pw_writebackground(wa_data.pw, 0, 0,
		    		wa_data.r.r_width, wa_data.r.r_height, PIX_NOT(PIX_DST));
				pw_show(wa_data.pw);
			}
		}
		if (wa_data.data_alarm.beep_num>0 || wa_data.data_alarm.flash_num>0)
			notify_set_itimer_func(win_alarm_handle,win_alarm_start,
				       ITIMER_REAL, &itv, &null_itv);
}

static Notify_value
win_alarm_start(win_alarm_handle)
	Notify_client	win_alarm_handle;
{
	int kbdfd, cmd;
	struct	itimerval	itv;

	itv.it_value = wa_data.data_alarm.beep_duration;
        itv.it_interval.tv_sec = 0;
        itv.it_interval.tv_usec = 0;
	if (wa_data.data_alarm.flash_num >0) {
		/* Flash pw */
		if (wa_data.pw && wa_data.windowfd >= 0 && win_do_visible_bell) {
			(void)pw_writebackground(wa_data.pw, 0, 0,
		    	wa_data.r.r_width, wa_data.r.r_height, PIX_NOT(PIX_DST));
			pw_show(wa_data.pw);
		}
	}
	if (wa_data.data_alarm.beep_num >0) {
		if (wa_data.windowfd >= 0 && win_do_audible_bell) {
			/* Open keyboard */
			if ((kbdfd = open(wa_data.screen.scr_kbdname, O_RDWR, 0)) < 0)
				return;
			wa_data.kbdfd = kbdfd;
			/* Start bell */
			cmd = KBD_CMD_BELL;
			(void) ioctl(wa_data.kbdfd, KIOCCMD, &cmd);
		}
	}
	/* Wait */
	notify_set_itimer_func(win_alarm_handle, win_alarm_stop,
			   ITIMER_REAL, &itv, &null_itv);
}
