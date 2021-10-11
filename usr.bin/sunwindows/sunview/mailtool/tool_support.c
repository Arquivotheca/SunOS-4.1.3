#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)tool_support.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
#endif

/*
 * Mailtool - tool support 
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <sunwindow/window_hs.h>

#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <varargs.h>

#include <suntool/window.h>
#include <suntool/frame.h>
#include <suntool/panel.h>
#include <suntool/text.h>

#include <suntool/scrollbar.h>
#include <suntool/selection.h>
#include <suntool/selection_svc.h>
#include <suntool/selection_attributes.h>
#include <suntool/walkmenu.h>
#include <suntool/icon.h>
#include <suntool/alert.h>

#include "glob.h"
#include "tool.h"

time_t	mt_msg_time;		/* time msgfile was last written */

/*
 * Cursors
 */

static short    wait_image[] = {
#include <images/hglass.cursor>
};
DEFINE_CURSOR_FROM_IMAGE(hourglasscursor, 7, 7, PIX_SRC | PIX_DST, wait_image);

#ifdef notdef
short	hourglass_cursorimage[16] = {
	0x7FFE,0x4002,0x200C,0x1A38,0x0FF0,0x07E0,0x03C0,0x0180,
	0x0180,0x0240,0x0520,0x0810,0x1108,0x23C4,0x47E2,0x7FFE
};
mpr_static(hourglass_mpr, 16, 16, 1, hourglass_cursorimage);
struct	cursor hourglass_cursor = { 8, 8, PIX_SRC|PIX_DST, &hourglass_mpr};
#endif

#define NUMWINDOWS 64
static	Cursor	cursor[NUMWINDOWS];
static	Window	window[NUMWINDOWS];
int	numWindows;

static void	mt_alert_failed(), mt_replace_bytes();
static int	mt_position_headersw();
static Notify_value	/*mt_subwindow_event_proc(),*/ mt_scrollbar_event_proc();

/*
 * Load the headers into the header subwindow.
 */
void
mt_load_headers()
{
	FILE           *f;
	register int    i, pos = 0;
	register struct msg *mp;
	int		lower_context, upper_context;

	if (!(f = mt_fopen(mt_hdrfile, "w")))
		return;
	(void)chmod(mt_hdrfile, 0600);
	for (i = 1, mp = &mt_message[1]; i <= mt_maxmsg; i++, mp++) {
		mp->m_start = pos;
		mp->m_lineno = i;
		fputs(mp->m_header, f);
		pos += strlen(mp->m_header);
	}
	mt_message[mt_maxmsg+1].m_start = pos;
	(void) fclose(f);
	textsw_reset(mt_headersw, 0, 0);
	pos = mt_position_headersw();
	if (mt_debugging) {
		(void) printf("positioning headersw:\n\tmt_curmsg = %d,\n\tmt_maxmsg = %d,\n\tmt_del_count = %d,\n\tpos = %d\n",
		mt_curmsg, mt_maxmsg, mt_del_count, pos);
		(void) fflush(stdout);
	}
	lower_context = (int)window_get(mt_headersw, TEXTSW_LOWER_CONTEXT);
	upper_context = (int)window_get(mt_headersw, TEXTSW_UPPER_CONTEXT);
	(void) window_set(mt_headersw,
		TEXTSW_READ_ONLY, FALSE,
		TEXTSW_LOWER_CONTEXT, -1, /* WORKAROUND to avoid scrolling */
		TEXTSW_UPPER_CONTEXT, 0,	/* WORKAROUND to prevent
						 * textwindow from
						 * repositioning */
		TEXTSW_INSERT_FROM_FILE, mt_hdrfile,  
		TEXTSW_FIRST_LINE, pos - 1,
			/* TEXTSW_FIRST_LINE is zero based */
		TEXTSW_READ_ONLY, TRUE,
		0);
	(void) window_set(mt_headersw,
		TEXTSW_UPPER_CONTEXT, upper_context,
		TEXTSW_LOWER_CONTEXT, lower_context,
		0);
	(void)unlink(mt_hdrfile); 
}

/*
 * Add headers for messages with number >= n to headersw.
 */
void
mt_append_headers(make_visible)
	int		make_visible;
{
	FILE           *f;
	register int    i, pos;
	register struct msg *mp;

	if (!(f = mt_fopen(mt_scratchfile, "w")))
		return;
	(void)chmod(mt_scratchfile, 0600);
	pos = mt_message[mt_curmsg].m_start;	/* initialized in previous
						 * call to either
						 * load_headers or
						 * append_headers */
	for (i = mt_curmsg, mp = &mt_message[mt_curmsg];
		i <= mt_maxmsg; i++, mp++) {
		mp->m_start = pos;
		mp->m_lineno = i - mt_del_count;
		fputs(mp->m_header, f);
		pos += strlen(mp->m_header);
	}
	mt_message[mt_maxmsg+1].m_start = pos;
	(void) fclose(f);
	(void) window_set(mt_headersw,
		TEXTSW_READ_ONLY, FALSE,
		TEXTSW_INSERTION_POINT, TEXTSW_INFINITY,
		TEXTSW_INSERT_FROM_FILE, mt_scratchfile,
		TEXTSW_READ_ONLY, TRUE,
		0);
	if (make_visible) {
		int		top, target, nrows;

		top = (int)window_get(mt_headersw, TEXTSW_FIRST_LINE) + 1;
		/* + 1 because TEXTSW_FIRST_LINE is zero based. */
		nrows = (int)window_get(mt_headersw, WIN_ROWS);
		target = mt_curmsg - mt_del_count;
		/* the line number for the first new message */
		if (top <= target && target < top + nrows) {
		 	/* already visible. don't do anything */
		 	if (mt_debugging) {
				(void) printf("headersw correctly positioned:\n\ttop = %d,\n\tmt_curmsg = %d,\n\tmt_del_count = %d,\n\tnrows = %d\n",
				top, mt_curmsg, mt_del_count, nrows);
				(void) fflush(stdout);
			}
		} else {
			int		upper_context;
			pos = mt_position_headersw();
			if (mt_debugging) {
				(void) printf("positioning headersw:\n\ttop = %d,\n\tmt_curmsg = %d,\n\tmt_del_count = %d,\n\tpos = %d\n",
				top, mt_curmsg, mt_del_count, pos);
				(void) fflush(stdout);
			}
			upper_context = (int)window_get(
				mt_headersw, TEXTSW_UPPER_CONTEXT);
			(void)window_set(mt_headersw,
				TEXTSW_UPPER_CONTEXT, 0,
				/*
				 * WORKAROUND to prevent textwindow from
				 * repositioning 
				 */
				TEXTSW_FIRST_LINE, pos - 1,
				0);
			(void) window_set(mt_headersw,
				TEXTSW_UPPER_CONTEXT, upper_context,
				0);
		}
	}
	(void)unlink(mt_scratchfile);
}

/*
 * used to make the current message be the top line in the window. fill out the
 * window from the top if too few remaining messages 
 */
static int
mt_position_headersw()
{
	int		height, start, nmsgs;

	/* height = textsw_screen_line_count(mt_headersw); */
	if (mt_curmsg == 1)	/* occurs when there is no new mail */
		return (1);
	height = (int)window_get(mt_headersw, WIN_ROWS);
	nmsgs = mt_maxmsg - mt_del_count; /* actual number of messages */
	if (nmsgs <= height)
		start = 1;
	else if (mt_maxmsg - mt_curmsg >= height - 1)
		/*
		 * there are more messages beyond mt_curmsg than would fit in
		 * the window with one blank line at the bottom 
		 */
		start = mt_curmsg - mt_del_count - 1;
		/* position window with mt_curmsg one line from top */
	else
		start = nmsgs - height + 2;
		/*
		 * position window so that mt_maxmsg is at the bottom,
		 * followed by one blank line 
		 */
	return (start); 
	/*
	 * Note: would like to use possibly_normalize so as to take into
	 * account user's settings for lower_context and upper_context.
	 * Unfortunately, in one case, want to normalize with respect to the
	 * top of the window. In the other case, it is the bottom. 
	 */
}

/*
 * Move the mark in the header subwindow to point to the new current message. 
 * This procedure is now called only from mt_update_msgsw
 */
void
mt_update_curmsg()
{
	int  	  spos;

	mt_update_info("");	/* if current message changes, remove info */
	(void) window_set(mt_headersw, TEXTSW_READ_ONLY, FALSE, 0);
	if (mt_prevmsg > 0
	    && mt_prevmsg <= mt_maxmsg
	    && !mt_message[mt_prevmsg].m_deleted) {
		spos = mt_message[mt_prevmsg].m_start;
		mt_replace_bytes(spos, spos+2, "  ");
	}
	spos = mt_message[mt_curmsg].m_start;
	mt_replace_bytes(spos, spos+2, "> ");
	(void) window_set(mt_headersw, TEXTSW_READ_ONLY, TRUE, 0);
	mt_last_sel_msg = mt_curmsg;	/* This message is now effectively
					 * selected. Previously, we called
					 * mt_set_curselmsg, which set
					 * mt_last_sel_msg, and also moved
					 * the selection to be the message
					 * number. */
	mt_prevmsg = mt_curmsg;
}

/*
 * Make sure the indicated message is visible. Scroll if it is not. 
 */
void
mt_update_headersw(msg)
	int	msg;
{

	int	top, height, target;

	top = (int)window_get(mt_headersw, TEXTSW_FIRST_LINE) + 1;
		/* TEXTSW_FIRST_LINE is zero based */
	height = (int)window_get(mt_headersw, WIN_ROWS);
	target = mt_message[msg].m_lineno;
	/*
	 * gives "true" line no, taking into account deleted messages
	 * that occur earlier in headersw 
	 */
	if (target < top || target > top + height - 1) {
		int	n;

		n = target - 1 - height / 2;	/* position target in
						 * middle of window */
		(void) window_set(mt_headersw,
			TEXTSW_FIRST_LINE, (n >= 0 ? n : 0),
			TEXTSW_UPDATE_SCROLLBAR,
			0);
	}
}

/*
 * If the message displayed in the message subwindow
 * has been modified, write it back out to the file
 * and tell mail to reload the message from the file.
 * If the user stores the message into a different
 * file then the filename for the textsw will change
 * so we have to check for that too.
 */
void
mt_save_curmsg()
{
	struct stat     statb;
	char            curmsgfile[1024], *s;
	register struct msg *mp;

	if (mt_maxmsg == 0)
		return;	/* no messages */
	curmsgfile[0] = '\0';
	textsw_append_file_name(mt_msgsw, curmsgfile);
	if (window_get(mt_msgsw, TEXTSW_MODIFIED)) {
		/*
		 * with use of TEXTSW_FILE_CONTENTS, TEXTSW_MODIFIED now
		 * returns the right thing even for memory files, i.e., can
		 * distinguish user edits from the "edits" that mailtool made 
		 */
		if (mt_value("editmessagewindow") &&
			/*
			 * if !editmessagewindow, then confirmation has
			 * already taken place 
			 */
			!mt_confirm(mt_msgsw, TRUE, FALSE,
				"Save changes",
				"Discard changes",
				"Message has been modified.",
				"Please confirm changes.",
				0))
                {       /* user wants to discard changes */
                        int x, y;
                        x = (int) window_get(mt_msgsw, WIN_X);
                        y = (int) window_get(mt_msgsw, WIN_Y);
                        /*
                         * (Bug# 1011209 & Bug# 1012499 fix)
                         * this is a low-level windows call which invokes
                         * an UNDO_ALL_EDITS in the subsw.  Its faster
than
                         * loading the same (unedited) message again.
                         */
                        textsw_reset_2(mt_msgsw,x+15,y+15,TRUE,TRUE);

			mt_msg_time = 0;
			return;
		} 
		if (curmsgfile[0] != '\0') {
			if (textsw_save(mt_msgsw, 0, 0))
				goto failed;
		} else { 
			if (textsw_store_file(mt_msgsw, mt_msgfile, 0, 0))
				goto failed;
			(void) strcpy(curmsgfile, mt_msgfile);
		}
		mt_load_msg(mt_curmsg, curmsgfile);
	} else if (curmsgfile[0] != '\0' && stat(curmsgfile, &statb) >= 0
			&& statb.st_mtime > mt_msg_time) {
		/*
		 * Note: this check is to cover the case where the user made
		 * some changes, did a store, and then did a next or some
		 * other operation. It notes that the file has been recently
		 * written. IT DOES NOT WORK IF /Text/Store_changes_file IS
		 * FALSE 
		 */
		mt_msg_time = statb.st_mtime;
		mt_load_msg(mt_curmsg, curmsgfile);
	} else
		return;
	s = mt_get_header(mt_curmsg);
	mp = &mt_message[mt_curmsg];
	if (strcmp(mp->m_header, s) != 0) {
		int             from, to;
		register int    i, delta;

		/* header changed. update */
		free(mp->m_header);
		mp->m_header = mt_savestr(s);	/* make a copy of the string */
		from = mp->m_start;
		to = mt_message[mt_curmsg + 1].m_start;
		strncpy(s, "> ", 2);
		mt_replace_header(from,	to, s);
		delta = strlen(s) - (to - from);
		for (i = mt_curmsg+1, mp = &mt_message[mt_curmsg+1];
			i <= mt_maxmsg + 1; i++, mp++)  
			mp->m_start = mp->m_start + delta;
	}
	return;
   failed:
	mt_warn(mt_frame, "Can't save changes.", 0);
	unlink(mt_msgfile);
	mt_msg_time = 0;
	return;
}

/*
 * Load the message into the message subwindow.
 */
void
mt_update_msgsw(msg, ign, ask_save, make_visible, set_seln)
	int             msg, ign, ask_save, make_visible, set_seln;
{
	struct stat     statb;
	int		lower_context;
/*	Textsw_status	status; */ /* not used - hala */

	if (ask_save)
		mt_save_curmsg();
	if (msg == 0)
		return;
	unlink(mt_msgfile);	/* just in case there's one left over */
	mt_print_msg(msg, mt_msgfile, ign);
	(void) stat(mt_msgfile, &statb);
	mt_msg_time = statb.st_mtime;
	mt_curmsg = msg;
	mt_update_curmsg();
	if (make_visible)
		mt_update_headersw(msg);
	if (set_seln)
		mt_set_curselmsg(msg);
	lower_context = (int)window_get(mt_headersw, TEXTSW_LOWER_CONTEXT);
	(void) window_set(mt_msgsw,
		TEXTSW_DISABLE_LOAD, FALSE,
		TEXTSW_LOWER_CONTEXT, -1,	/* work around to
						 * suppress scrolling as
						 * you insert */
		TEXTSW_FILE_CONTENTS, mt_msgfile, 
		TEXTSW_FIRST, 0,
		TEXTSW_LOWER_CONTEXT, lower_context, 
		TEXTSW_INSERTION_POINT, 0,
		TEXTSW_DISABLE_LOAD, TRUE,
		0);
}

void
mt_update_info(s)
	char           *s;
{
	char           *p;

	if (*s == '\0')
		return;	/* FOR NOW */
	else if (p = index(s, '\n'))
		*p = '\0';
	if (mt_info_item != NULL) 
		(void) panel_set_value(mt_info_item, s);
}

/*
 * Makes the message number in the indicated message be the primary selection 
 */
void
mt_set_curselmsg(msg)
	int             msg;
{
	int             spos, epos;
	extern int      mt_last_sel_msg;

	epos = mt_message[msg].m_start + 5;
	if (msg < 10)
		spos = epos - 1;
	else if (msg < 100)
		spos = epos - 2;
	else
		spos = epos - 3;
	(void) textsw_set_selection(mt_headersw, spos, epos, SEL_PRIMARY);
	mt_last_sel_msg = msg;
}

/*
 * Delete characters in the header subwindow.
 */
void
mt_del_header(from, to)
	int             from, to;
{
	mt_replace_header(from, to, "");
}

/*
 * Insert characters in the header subwindow.
 */ 
void
mt_ins_header(start, s)
	int             start;
	char           *s;
{
	mt_replace_header(start, start, s);
}

/*
 * replace characters in the header subwindow.
 */
void
mt_replace_header(from, to, s)
	int             from, to;
	char           *s;
{
	(void) window_set(mt_headersw, TEXTSW_READ_ONLY, FALSE, 0);
	mt_replace_bytes(from, to, s);
	(void) window_set(mt_headersw,
		TEXTSW_READ_ONLY, TRUE,
		TEXTSW_UPDATE_SCROLLBAR,
		0);
}

static void
mt_replace_bytes(from, to, s)
	int             from, to;
	char           *s;
{
	if ((textsw_replace_bytes(mt_headersw, from, to, s, strlen(s)) == 0)
		&& (from != 0))
		/* replace failed */
		return;
	return;
}

/*
 * Temporarily set the namestripe while tool is busy doing something.
 * Does not affect the value of the buffers mt_namestripe_left or
 * mt_namestripe_right
 */
void
mt_set_namestripe_temp(s)
	char           *s;
{

	if (!mt_frame) {}
	else 
		(void) window_set(mt_frame, FRAME_LABEL, s, 0);
}

/*
 * Set the left side of the namestripe to message of the form %s - folder: %s %s
 */
void
mt_set_namestripe_left(folder, status, update)
	char           *folder, *status;
	int		update;
{

	if (!mt_frame) {}
	else {
		*mt_namestripe_left = '\0';
		(void) sprintf(mt_namestripe_left, "%s - %s%s %s",
			mt_cmdname,
			(mt_panel_style != mt_Old && mt_system_mail_box) ? "" : "folder: ",
			/* to save some space in namestripe */
			folder, status);
		if (update)
			mt_set_namestripe_both();
	} 
}

/*
 * Set the "right side" of the namestripe
 */
void
mt_set_namestripe_right(s, update)
	char           *s;
{

	if (!mt_frame) {}
	else {
		*mt_namestripe_right = '\0';
		if (mt_panel_style != mt_Old)
			(void) strcpy(mt_namestripe_right, s);
		if (update)
			mt_set_namestripe_both();
	}
}

/*
 * set the name stripe with all of the left side and as much of the right
 * side as will fit. Take right most characters of right side if not all will
 * fit. 
 */
void
mt_set_namestripe_both()
{
	register int	i, j, lenl, lenr;
	int		dots = 0;

	lenl = strlen(mt_namestripe_left);
	lenr = strlen(mt_namestripe_right);
	*mt_namestripe = '\0';
	j = lenl + lenr + 3 - mt_namestripe_width;
		/* take right most characters */
	if (j <= 0)
		/* all will fit */
		j = 0;
	else
		dots = 3;

	for (i = 0; i < mt_namestripe_width; i++ ) {
		if (i < lenl)
			mt_namestripe[i] = mt_namestripe_left[i];
		else if (i < lenl + 3)
			mt_namestripe[i] = ' ';	/* the -3 to make sure at
						 * least three spaces between
						 * two sides */
		else if (i < mt_namestripe_width - lenr)
			mt_namestripe[i] = ' ';
		else if (dots > 0) {
			mt_namestripe[i] = '.';
			dots--;
			j++;
		} else
			mt_namestripe[i] = mt_namestripe_right[j++];
	}
	mt_namestripe[i] = '\0';
	if (strcmp(window_get(mt_frame, FRAME_LABEL), mt_namestripe) != 0)
		/*
		 * to avoid multiple repainting of name stripe. Should be
		 * handled in SunView libraries 
		 */
		(void) window_set(mt_frame, FRAME_LABEL, mt_namestripe, 0);
}

/*
 * restores name stripe to previous value.
 */
void
mt_restore_namestripe()
{
	(void) window_set(mt_frame, FRAME_LABEL, mt_namestripe, 0);

}

/*
 * Set the icon.
 */
void
mt_set_icon(ic)
	struct icon    *ic;
{
	(void) window_set(mt_frame, FRAME_ICON, ic, 0);
}

/*
 * Announce the arrival of new mail.
 */
void
mt_announce_mail()
{
	static struct timeval tv = {0, 300000};
	int             bells, flashes, fd; 
	Pixwin		*pw;

	bells = mt_bells;
	flashes = mt_flashes;
	if (bells <= 0 && flashes <= 0)
		return;
	fd = (int)window_get(mt_frame, WIN_FD);
	pw = (Pixwin *)window_get(mt_frame, WIN_PIXWIN);
	for (;;) {
		win_bell(bells-- > 0 ? fd : -1, tv, flashes-- > 0 ? pw : 0);
		if (bells > 0 || flashes > 0)
			/* pause between bells */
			select(0, 0, 0, 0, &tv);
		else
			break;
	}
}

/*
 * Request confirmation from the user. Use alerts.  
 */
/* VARARGS1 */
int
mt_confirm(frame, beep, optional, button1, button2, va_alist)
	Frame		frame;
	int		beep, optional;
	char		*button1, *button2;
	va_dcl
{
	va_list         args;
	Event           ie;
	char		*p;
	int		status, i;
	char		*array[100];


	if (mt_value("expert"))
		return(TRUE);

	i = 0;
	va_start(args);
	while ((p = va_arg(args, char *)) != (char *) 0) 
		array[i++] = p;
	va_end(args);
	array[i] = (char *) 0;
	status = alert_prompt(frame,
		&ie,
		ALERT_MESSAGE_STRINGS_ARRAY_PTR, array,
		ALERT_BUTTON_YES, button1,
		ALERT_BUTTON_NO, button2,
		ALERT_OPTIONAL, optional,
		ALERT_NO_BEEPING, (mt_bells == 0 || beep == 0),
		0);
	switch (status) {
		case ALERT_YES: return (TRUE);
		case ALERT_NO: return(FALSE);
		case ALERT_FAILED: mt_alert_failed("Could not put up alert to request confirmation of the following operation:\n",
			array,
			"\nProceeding by assuming that this operation was NOT confirmed\n");
			return(FALSE);
	}
}

/*
 * Warn the user about an error.
 */
/* VARARGS1 */
void
mt_warn(frame, va_alist)
	Frame		frame;
	va_dcl
{
	va_list         args;
	Event           ie;
	char		*p;
	int		status, i;
	char		*array[100];

	i = 0;
	va_start(args);
	while ((p = va_arg(args, char *)) != (char *) 0) 
		array[i++] = p;
	va_end(args);
	array[i] = (char *) 0;
	status = alert_prompt(frame,
		&ie,
		ALERT_MESSAGE_STRINGS_ARRAY_PTR, array,
		ALERT_BUTTON_YES, "Continue",
		ALERT_NO_BEEPING, (mt_bells == 0),
		ALERT_TRIGGER, ACTION_STOP, 
		0);
	switch (status) {
		case ALERT_YES: return;
		case ALERT_TRIGGERED: return; 
		case ALERT_FAILED: mt_alert_failed("Could not put up alert to warn user of the following condition:\n",
		array, "");
			return;
	}
}

static void
mt_alert_failed(str1, array, str2)
	char	*str1, *str2;
	char	*array[];
{
	int	i = 0;
	(void) fprintf(stderr, str1);
	for (i = 0; array[i] != (char *) 0; i++)
		(void) fprintf(stderr, array[i]);
	(void) fprintf(stderr, str2);
}

/* 
 * Change cursor to hourglass
 */
void
mt_waitcursor()
{
	int             fd, i;
	static          index;

	while (index < numWindows)
		cursor[index++] = cursor_create((char *)0);
	for (i = 0; i < numWindows; i++) {
		fd = (int) window_get(window[i], WIN_FD);
		(void) win_getcursor(fd, cursor[i]);
		(void) win_setcursor(fd, &hourglasscursor);
	}
}

/* 
 * Restore cursor
 */
void
mt_restorecursor()
{
	int             i, fd;

	for (i = 0; i < numWindows; i++) {
		fd = (int) window_get(window[i], WIN_FD);
		(void) win_setcursor(fd, cursor[i]);
	}
}

/*
 * stores w in window[], an array of all subwindows. window[] is used for
 * setting cursor in mt_waitcursor and restoring it in mt_restore_cursor 
 */
void
mt_add_window(w, type)
	Window          w;
	enum mt_Window_Type type;
{
	window[numWindows++] = w;
}


/* takes care of removing w from the window array */
void
mt_remove_window(w)
	Window          w;
{
	int             i;

	for (i = 1; i < numWindows; i++)
		if (window[i] == w)
			for (; i < numWindows; i++)
				window[i] = window[i+1];
	numWindows--;
}


/* installed on all subwindows in the tool */
/* not used - hala
static Notify_value 
mt_subwindow_event_proc(client, event, arg, when)
	Notify_client   client;
	Event          *event;
	Notify_arg      arg;
	Notify_event_type when;
{
	mt_log_event(event);
	return (notify_next_event_func(client, event, arg, when));
}
*/

FILE           *
mt_fopen(filename, type)
	char           *filename, *type;
{
	FILE           *f;

	if (filename == NULL)
		return(NULL);

	if (!(f = fopen(filename, type))) {
		char            s[128];

		(void) strcpy(s, "couldn't open ");
		(void) strcat(s, filename);
		mt_warn(mt_frame, s, 0);
	}
	return(f);
}
