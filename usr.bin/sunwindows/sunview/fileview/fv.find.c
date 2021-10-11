#ifndef lint
static  char sccsid[] = "@(#)fv.find.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */

#include <stdio.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#ifdef SV1
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/text.h>
#include <suntool/canvas.h>
#include <suntool/scrollbar.h>
#else
#include <view2/view2.h>
#include <view2/panel.h>
#include <view2/text.h>
#include <view2/canvas.h>
#include <view2/scrollbar.h>
#endif

#include "fv.port.h"
#include "fv.h"
#include "fv.extern.h"

#define FINDPROG 	"/usr/bin/find"
#define MAXFIND		512
#define CANVAS_ROWS	8
#define EMPTY		(-1)

extern errno;
extern char *sys_errlist[];
extern time_t time();

static Frame Find_frame;
static Frame Find_panel;
static Canvas Find_canvas;
static PAINTWIN Pw;
static Scrollbar Find_canvassb;
static Panel_item Nameitem, Owneritem, Beforeitem, Afteritem, Patternitem;
static Panel_item Nametoggle, Ownertoggle, Casetoggle, Fromitem;
static Panel_item Find_button, Cancel_button, Open_button;

static FILE *Ptyfp;
static char *Line = "/dev/ptyXX";
#define LENDEVPTY	9
#define LENDEV		5

static int Pid;
static int Master, Slave;

static int Nfound;
static int Found_chosen = EMPTY;
static char *Found[MAXFIND];

static int Find_canvas_y;

static void find_button();
static void cancel_button();
static void open_button();
static void done();
static Notify_value find_input();

#define NITEMS 6
static char *Label[NITEMS] = {
	"Find files from folder:",
	"Name:",
	"Owner:",
	"Modified After:",
	"Modified Before:",
	"File contains text pattern:"
};
#ifdef SV1
# ifdef PROTO
#  define VALUE_COL	26
# else
#  define VALUE_COL	29
# endif
#else
# define VALUE_COL	17
#endif

fv_find_object()
{
	if (!Find_frame)
		make_popup();
	else
	{
		panel_set(Fromitem, PANEL_VALUE, Fv_path, 0);
#ifdef OPENLOOK
		panel_set(Open_button, PANEL_INACTIVE, TRUE, 0);
#endif
	}

	if ((int)window_get(Find_frame, WIN_SHOW) == FALSE)
		fv_winright(Find_frame);

	window_set(Find_frame, WIN_SHOW, TRUE, 0);
}


static
make_popup()
{
	extern Canvas fv_create_listbox();
	static void open_notify();
	Panel panel;

	/* XXX Small font problem in Sunview 1 */

	if ((Find_frame = window_create(Fv_frame, 
#ifdef SV1
		FRAME,
		FRAME_LABEL, "fileview: Find",
# ifdef PROTO
		FRAME_CLASS, FRAME_CLASS_COMMAND,
		FRAME_ADJUSTABLE, FALSE,
# endif
#else
		FRAME_CMD,
		FRAME_SHOW_FOOTER, TRUE,
		FRAME_CMD_PUSHPIN_IN, TRUE,
		FRAME_LABEL, " Find",
#endif
		FRAME_DONE_PROC, done,
		FRAME_SHOW_LABEL, TRUE,
		FRAME_NO_CONFIRM, TRUE,
		    0)) == NULL ||
#ifndef SV1
	    (panel = vu_get(Find_frame, FRAME_CMD_PANEL)) == NULL)
#else
	    (panel = window_create(Find_frame, PANEL, 
		0)) == NULL)
#endif
	{
		fv_putmsg(TRUE, Fv_message[MEWIN], 0, 0);
		return;
	}


	Fromitem = panel_create_item(panel, PANEL_TEXT,
		PANEL_LABEL_STRING, Label[0],
		PANEL_VALUE_DISPLAY_LENGTH, 25,
		PANEL_LABEL_BOLD, TRUE,
		PANEL_VALUE, Fv_path,
		PANEL_ITEM_Y, ATTR_ROW(0),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);

	Nameitem = panel_create_item(panel, PANEL_TEXT,
		PANEL_LABEL_STRING, Label[1],
		PANEL_VALUE_DISPLAY_LENGTH, 15,
		PANEL_LABEL_BOLD, TRUE,
		PANEL_ITEM_Y, ATTR_ROW(1),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);
	Nametoggle = panel_create_item(panel, PANEL_CHOICE,
		PANEL_CHOICE_STRINGS, "Included", "Excluded", 0,
		PANEL_ITEM_Y, ATTR_ROW(1)-CHOICE_OFFSET,
		0);
	Owneritem = panel_create_item(panel, PANEL_TEXT,
		PANEL_LABEL_STRING, Label[2],
		PANEL_VALUE_DISPLAY_LENGTH, 15,
		PANEL_LABEL_BOLD, TRUE,
		PANEL_ITEM_Y, ATTR_ROW(2),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);
	Ownertoggle = panel_create_item(panel, PANEL_CHOICE,
		PANEL_CHOICE_STRINGS, "Included", "Excluded", 0,
		PANEL_ITEM_Y, ATTR_ROW(2)-CHOICE_OFFSET,
		0);
	Afteritem = panel_create_item(panel, PANEL_TEXT,
		PANEL_LABEL_STRING, Label[3],
		PANEL_LABEL_BOLD, TRUE,
		PANEL_VALUE_DISPLAY_LENGTH, 15,
		PANEL_VALUE_STORED_LENGTH, 16,
		PANEL_ITEM_Y, ATTR_ROW(3),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);
	panel_create_item(panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, "(mm/dd/yy)",
		PANEL_LABEL_BOLD, TRUE,
		0);
	Beforeitem = panel_create_item(panel, PANEL_TEXT,
		PANEL_LABEL_STRING, Label[4],
		PANEL_LABEL_BOLD, TRUE,
		PANEL_VALUE_DISPLAY_LENGTH, 15,
		PANEL_VALUE_STORED_LENGTH, 16,
		PANEL_ITEM_Y, ATTR_ROW(4),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);
	panel_create_item(panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, "(mm/dd/yy)",
		PANEL_LABEL_BOLD, TRUE,
		0);
	Patternitem = panel_create_item(panel, PANEL_TEXT,
		PANEL_LABEL_STRING, Label[5],
		PANEL_LABEL_BOLD, TRUE,
		PANEL_VALUE_DISPLAY_LENGTH, 15,
		PANEL_ITEM_Y, ATTR_ROW(5),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);
	Casetoggle = panel_create_item(panel, PANEL_TOGGLE,
		PANEL_CHOICE_STRINGS, "Ignore Case", 0,
		PANEL_ITEM_Y, ATTR_ROW(5)-CHOICE_OFFSET,
		0);
	Find_button = panel_create_item(panel, PANEL_BUTTON,
		MY_BUTTON_IMAGE(panel, "Find"),
		PANEL_NOTIFY_PROC, find_button,
		PANEL_ITEM_Y, ATTR_ROW(6),
		PANEL_ITEM_X, ATTR_COL(VALUE_COL-5),
		0);
	Cancel_button = panel_create_item(panel, PANEL_BUTTON,
		MY_BUTTON_IMAGE(panel, "Stop"),
#ifdef OPENLOOK
		PANEL_INACTIVE, TRUE,
#endif
		PANEL_NOTIFY_PROC, cancel_button, 0);
	Open_button = panel_create_item(panel, PANEL_BUTTON,
		MY_BUTTON_IMAGE(panel, "Open"),
#ifdef OPENLOOK
		PANEL_INACTIVE, TRUE,
#endif
		PANEL_NOTIFY_PROC, open_button, 0);

	Find_panel = panel;
	if ((Find_canvas = fv_create_listbox(Find_frame,
		512, &Found_chosen, &Nfound, Found, open_notify)) == NULL)
		return;

	window_fit(Find_panel);
	window_set(Find_canvas, WIN_BELOW, Find_panel,
		WIN_WIDTH, window_get(Find_panel, WIN_WIDTH),
		WIN_HEIGHT, ATTR_ROWS(7),
		0);

#ifdef SV1
	Pw = (PAINTWIN)canvas_pixwin(Find_canvas);
#else
	Pw = (PAINTWIN)vu_get(Find_canvas, CANVAS_NTH_PAINT_WINDOW, 0);
#endif
	Find_canvassb = (Scrollbar)window_get(Find_canvas, WIN_VERTICAL_SCROLLBAR);

#ifdef PROTO
	frame_set_font_size(Find_frame, (int)window_get(Fv_frame, WIN_FONT_SIZE));
#endif
	window_fit(Find_frame);
}


static void
find_button()
{
	int i, pid, after_days, before_days, today;
	char *from, *name, *owner, *after, *before, *pattern;
	char *av[30], buf[10], buf1[10];
	union wait status;

	/* Get and check name, owner, modify time, and pattern parameters */

	from = (char *)panel_get(Fromitem, PANEL_VALUE);
	name = (char *)panel_get(Nameitem, PANEL_VALUE);
	owner = (char *)panel_get(Owneritem, PANEL_VALUE);
	after = (char *)panel_get(Afteritem, PANEL_VALUE);
	before = (char *)panel_get(Beforeitem, PANEL_VALUE);
	pattern = (char *)panel_get(Patternitem, PANEL_VALUE);

	if (*from == 0)
	{
		error(FALSE, Fv_message[MSEARCH], Fv_path);
		from = Fv_path;
	}
	if (*name == 0 && *owner == 0 && *after == 0 && *before == 0 && 
		*pattern == 0)
	{
		/* Supply default parameter */
		*name = '*';
		*(name+1) = 0;
	}

	if (*after && (after_days = calc_days(after)) == -1)
	{
		error(TRUE, Fv_message[MEDATE], "After");
		return;
	}
	if (*before && (before_days = calc_days(before)) == -1)
	{
		error(TRUE, Fv_message[MEDATE], "Before");
		return;
	}
	today = today_in_days();

	if (*owner && getunum(owner) == -1)
	{
		error(TRUE, Fv_message[MEOWNER], owner);
		return;
	}

	/* Get canvas ready for find's output... */

	for (i=0; i < Nfound; i++)
		free(Found[i]);
	Nfound = 0;
	Find_canvas_y = 0;
	Found_chosen = EMPTY;
	pw_writebackground(Pw, 0, 0, 
		(int)window_get(Find_canvas, CANVAS_WIDTH),
		(int)window_get(Find_canvas, CANVAS_HEIGHT), PIX_CLR);
	if (scrollbar_get(Find_canvassb, SCROLL_VIEW_START))
		scrollbar_scroll_to(Find_canvassb, 0);
	window_set(Find_canvas, CANVAS_HEIGHT, MAXFIND*Fv_fontsize.y, 0);

	/* Fork off the find... */

	Master = getMaster();
	if (Master < 0) {
		error(TRUE, Fv_message[MEPTY], 0);
		return;
	}

	/* Don't block if no process has status to report */

 	while (wait3(&status, WNOHANG, (struct rusage *)0) > 0)
		continue;
	if ((pid = fork()) < 0) {
		error(TRUE, sys_errlist[errno], 0);
		return;
	}
	if (pid == 0) {		/* child */
		i = open("/dev/tty", O_RDWR);
		if (i >= 0) {
			(void) ioctl(i, TIOCNOTTY, (char *)0);
			(void) close(i);
		}
		Slave = getSlave();
		(void) close(Master);
		(void) dup2(Slave, 0);
		(void) dup2(Slave, 1);
		(void) dup2(Slave, 2);
		for (i = getdtablesize() - 1; i >= 3; i--)
			(void)close(i);
		i = 0;
		av[i++] = "find";
		av[i++] = from;
		if (name[0]) {
			if (panel_get_value(Nametoggle))
				av[i++] = "!";
			av[i++] = "-name";
			av[i++] = name;
		}
		if (owner[0]) {
			if (panel_get_value(Ownertoggle))
				av[i++] = "!";
			av[i++] = "-user";
			av[i++] = owner;
		}
		if (after[0]) {
			av[i++] = "-mtime";
			(void)sprintf(buf, "-%d", today-after_days-1);
			av[i++] = buf;
		}
		if (before[0]) {
			av[i++] = "!";
			av[i++] = "-mtime";
			(void)sprintf(buf1, "-%d", today-before_days);
			av[i++] = buf1;
		}
		if (pattern[0]) {
			av[i++] = "-exec";
			av[i++] = "egrep";
			av[i++] = "-l";
			if (panel_get_value(Casetoggle))
				av[i++] = "-i";
			av[i++] = pattern;
			av[i++] = "{}";
			av[i++] = ";";
		}
		else
			av[i++] = "-print";

		av[i++] = NULL;

		execve(FINDPROG, av, (char **)0);
		(void)fprintf(stderr, "exec failed\n");
		exit(127);
	}
	if ((Ptyfp = fdopen(Master, "r")) == NULL)
		error(TRUE, Fv_message[MEREAD], 0);
	else
	{
		error(FALSE, "Searching...", 0);
#ifdef OPENLOOK
		panel_set(Cancel_button, PANEL_INACTIVE, FALSE, 0);
		panel_set(Open_button, PANEL_INACTIVE, TRUE, 0);
#endif
		Pid = pid;
		notify_set_input_func(&Pid, find_input, Master);
	}
}


static Notify_value		/* Stick find's output into text subwindow... */
find_input()			/* ARGS IGNORED */
{
	char buf[255];
	unsigned len;
	static int threshold = CANVAS_ROWS;
	
	if (fgets(buf, sizeof(buf), Ptyfp))
	{
		/* Find has output... */

		/* Errors typically have a colon before the error
		 * message.  Touch luck if the filename has an embedded
		 * colon!
		 * Put errors from command on message line.
		 * Note: Find quits without informing us on sparc m/cs.
		 */

		if (strchr(buf, ':'))
			error(FALSE, buf, 0);
		else if (Nfound > MAXFIND)
		{
			cancel_button();
			error(FALSE, Fv_message[ME2MANYFILES], MAXFIND);
		}
		else
		{
			/* Strip off newline at end */

			len = (unsigned)strlen(buf);
			len--;
			buf[len] = NULL;

			if ((Found[Nfound]=fv_malloc(len+1))==0)
				goto cleanup;

			(void)strcpy(Found[Nfound], buf);
			Find_canvas_y += Fv_fontsize.y;
			pw_text(Pw, 5, Find_canvas_y, PIX_SRC, Fv_font, buf);
			Nfound++;
			if (Nfound>=threshold)
			{
				scrollbar_scroll_to(Find_canvassb, 
					(Nfound-1)*Fv_fontsize.y);
				threshold += CANVAS_ROWS;
			}

		}
	}
	else {
cleanup:
		(void)close(Master);
		notify_set_input_func(&Pid, NOTIFY_FUNC_NULL, Master);

		if (Nfound)
			error(TRUE, "%d file(s) found", Nfound);
		else
			error(TRUE, "Nothing found", 0);
		window_set(Find_canvas, CANVAS_HEIGHT, (Nfound+1)*Fv_fontsize.y, 0);
#ifdef OPENLOOK
		panel_set(Cancel_button, PANEL_INACTIVE, TRUE, 0);
#endif
		threshold = CANVAS_ROWS;	/* Reset */
	}
	return(NOTIFY_DONE);
}


static void
cancel_button()				/* ARGS IGNORED */
{
	/* XXX Must find a way of flushing Find's buffer */
	/* Kill find in progress... */

	if (Pid > 0) {
		if (kill(Pid, SIGTERM) == -1)
			error(TRUE, "Can't signal find", 0);
		else
			error(TRUE, "Find stopped", 0);
		Pid = 0;
		(void)close(Master);
		notify_set_input_func(&Pid, NOTIFY_FUNC_NULL, Master);
#ifdef OPENLOOK
		panel_set(Cancel_button, PANEL_INACTIVE, TRUE, 0);
#endif
	}
}


static void
open_notify()
{
#ifdef OPENLOOK
	panel_set(Open_button, PANEL_INACTIVE, FALSE, 0);
#endif
}

static void
done()
{
	register int i;

	(void)window_set(Find_frame, WIN_SHOW, FALSE, 0);
	/*fv_lb_delete(Find_canvas);*/		/* If you do delete... */
	
	for (i=0; i < Nfound; i++)
		free(Found[i]);
	Nfound = 0;
}


static
getunum(s)
	char	*s;
{
	register i;
	struct	passwd	*getpwnam(), *pw;

	i = -1;
	if (((pw = getpwnam(s)) != NULL) && pw != (struct passwd *)EOF)
		i = pw->pw_uid;
	return(i);
}


static
getMaster()
{
	char *pty, *bank, *cp;
	struct stat stb;
	int	i;
	
	pty = &Line[LENDEVPTY];
	for (bank = "pqrs"; *bank; bank++) {
		Line[LENDEVPTY-1] = *bank;
		*pty = '0';
		if (stat(Line, &stb) < 0)
			break;
		for (cp = "0123456789abcdef"; *cp; cp++) {
			*pty = *cp;
			i = open(Line, O_RDWR);
			if (i >= 0) {
				char *tp = &Line[LENDEV];
				int ok;

				/* verify Slave side is usable */
				*tp = 't';
				ok = access(Line, R_OK|W_OK) == 0;
				*tp = 'p';
				if (ok)
					return(i);
				(void) close(i);
			}
		}
	}
	return(-1);
}

static	struct	sgttyb	b = {13, 13, CTRL(?), CTRL(U), 06310};

static
getSlave()
{
	int	i;
	
	Line[LENDEV] = 't';
	i = open(Line, O_RDWR);
	if (i < 0)
		return(-1);

	(void) ioctl(i, TIOCSETP, (char *)&b);
	return(i);
}


static void
open_button()				/* ARGS IGNORED */
{
	if (Found_chosen >=0 && Found_chosen < Nfound)
		fv_openfile(Found[Found_chosen],
			(char *)panel_get(Patternitem, PANEL_VALUE), FALSE);
	else
		error(TRUE, Fv_message[MESELFIRST], 0);
}

static short Month_days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

/* Returns number of days since start of century; no error checking */

static
get_days(month, day, year)
	register int month;
	int day;
	register int year;
{
	register int days = 0;
	short leap;

	year += 1900;
	leap = year%4 == 0 && year%100 != 0 || year%400 == 0;
	year--;
	while (year >= 1900)
	{
		days += 365 + (year%4 == 0 && year%100 || year%400 == 0);
		year--;
	}

	if (leap && month > 2)
		days++;
	month -= 2;
	while (month >= 0)
		days += Month_days[month--];
	days += day;

	return(days);
}


static
calc_days(mdy)			/* Validate and return # of days from string */
	char *mdy;		/* Month, day, year */
{
	int scanned;
	int month, day, year;
	short leap;

	scanned = sscanf(mdy, "%d/%d/%d", &month, &day, &year);
	if (scanned==2)
	{
		/* Complete the year for the user */
		struct tm *today;

		year = time((time_t *)0);
		today = localtime((long *)&year);
		year = today->tm_year;
	}
	else if (scanned!=3)
		return(-1);

	/* Do we have a valid date? */

	year += 1900;
	leap = year%4 == 0 && year%100 != 0 || year%400 == 0;
	year -= 1900;
	if (month<1 || month>12 || day < 1 || year < 0 || year > 99)
		return(-1);
	if (leap && month==2 && day>29)
		return(-1);
	if (day > Month_days[month-1])
		return(-1);

	/* We do indeed have a valid date, now calculate the number
	 * of days from today...
	 */

	return(get_days(month, day, year));
}

/* Return the number of days from century start 'til today */

static
today_in_days()
{
	time_t time_p;
	struct tm *ltime;

	time_p = time((time_t *)0);
	ltime = localtime(&time_p);

	return(get_days(ltime->tm_mon+1, ltime->tm_mday, ltime->tm_year));
}


static
error(bell, msg, arg)
	char bell;
	char *msg;
	char *arg;
{
	char buffer[256];

	if (arg)
	{
		(void)sprintf(buffer, msg, arg);
		msg = buffer;
	}

#ifdef SV1
# ifdef PROTO
	window_set(Find_frame, FRAME_MESSAGE, msg, 0);
	if (bell)
		window_bell(Find_frame);
# else
	fv_putmsg(bell, msg, (int)arg, 0);
# endif
#else
	window_set(Find_frame, FRAME_MESSAGE, msg, 0);
	if (bell)
		window_bell(Find_frame);
#endif
}
