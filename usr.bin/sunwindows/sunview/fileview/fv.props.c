#ifndef lint
static  char sccsid[] = "@(#)fv.props.c 1.1 92/07/30 Copyr 1988 Sun Micro";
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

#include <sys/types.h>
#include <sys/stat.h>

#ifdef SV1
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <suntool/scrollbar.h>
#else
#include <view2/view2.h>
#include <view2/panel.h>
#include <view2/canvas.h>
#include <view2/scrollbar.h>
#endif

#include "fv.port.h"
#include "fv.h"
#include "fv.extern.h"

static Frame Props_frame;
static Panel Props_panel;

static Panel_item Slider_item;
static Panel_item Slider_feedback_item;
static Panel_item Editor_item;
static Panel_item Other_editor_item;
static Panel_item Wantdu_item;
static Panel_item Confirm_delete;
static Panel_item Sort_item;
static Panel_item Style_item;
static Panel_item Icon_item;
static Panel_item Dot_item;
static Panel_item Filter_item;
static Panel_item Print_item;
static Panel_item Width_item;
 
#ifdef SV1
# ifdef PROTO
#  define VALUE_COL	22
# else
#  define VALUE_COL	24
# endif
#else
# define VALUE_COL	14
#endif

#define NITEMS	12
static char *Label[NITEMS] = 
{
	"Folder Display:", 
	"List Options:", 
	"Sort By:",
	"Show Folder Size By:",
	"Hidden Files:",
	"Deletions:",
	"Document Editor_item:",
	"Check for Changes:",
	"See Filter Pattern:",
	"Default Print Script:",
	"Longest Filename:"
};
static void done_proc();
static void props_proc();

fv_preference()
{
	int row;		/* Current panel item row position */
	char buf[4];		/* buffer */
	static void show_other();
	static void show_slider_value();
#ifdef OPENLOOK
	static int activate_list();
#endif
#ifndef SV1
	static void apply(), reset();
#endif

	if (Props_frame)
	{
		/* Only one property sheet; ensure it's visible */

		window_set(Props_frame, WIN_SHOW, TRUE, 0);
		return;
	}


	if ((Props_frame = window_create(Fv_frame, 
#ifdef SV1
		FRAME,
# ifdef PROTO
		FRAME_CLASS, FRAME_CLASS_COMMAND,
		FRAME_ADJUSTABLE, FALSE,
		FRAME_SHOW_FOOTER, FALSE,
# endif
		FRAME_LABEL, "fileview: Properties",
#else
		FRAME_PROPS,
		FRAME_PROPS_APPLY_PROC, apply,
		FRAME_PROPS_RESET_PROC, reset,
		FRAME_SHOW_FOOTER, FALSE,
		FRAME_LABEL, " Properties",
#endif
		FRAME_SHOW_LABEL, TRUE,
		FRAME_NO_CONFIRM, TRUE, 
		FRAME_DONE_PROC, done_proc,
		0)) == NULL ||
#ifdef SV1
	   (Props_panel = window_create(Props_frame, PANEL, 0)) == NULL)
#else
	   (Props_panel = vu_get(Props_frame, FRAME_PROPS_PANEL)) == NULL)
#endif
	{
		fv_putmsg(TRUE, Fv_message[MEWIN], 0, 0);
		return;
	}

#ifdef SV1
	make_ok_button();
#endif

	row = 0;
	Icon_item = panel_create_item(Props_panel, PANEL_CHOICE,
		PANEL_LABEL_STRING, Label[0],
		PANEL_LABEL_BOLD, TRUE,
		PANEL_CHOICE_STRINGS, "Icon", "List", 0,
		PANEL_VALUE, !(Fv_style & FV_DICON),
#ifdef OPENLOOK
		PANEL_NOTIFY_PROC, activate_list,
#endif
		PANEL_ITEM_Y, ATTR_ROW(row++),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);
#ifndef PROTO
	panel_create_item(Props_panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, Label[1],
		PANEL_LABEL_BOLD, TRUE,
		PANEL_ITEM_Y, ATTR_ROW(row),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);
#endif
	Style_item = panel_create_item(Props_panel, PANEL_TOGGLE,
#ifdef PROTO
		PANEL_LABEL_STRING, Label[1],
		PANEL_LABEL_BOLD, TRUE,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_NCOLS, 2,
#else
		PANEL_LAYOUT, PANEL_VERTICAL,
#endif
#ifdef OPENLOOK
		PANEL_INACTIVE, Fv_style & FV_DICON,
#endif
		PANEL_CHOICE_STRINGS,
			"Permissions", "Links", "Owner", "Group",
			"Size", "Date", 0,
		PANEL_VALUE, Fv_style>>1,
		PANEL_ITEM_Y, ATTR_ROW(row++),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);

#ifdef PROTO
	row += 2;
#else
# ifdef SV1
	row += 6;
# else
	row += 3;
# endif
#endif

#ifndef PROTO
	panel_create_item(Props_panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, Label[2],
		PANEL_LABEL_BOLD, TRUE,
		PANEL_ITEM_Y, ATTR_ROW(row),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);
#endif
	Sort_item = panel_create_item(Props_panel, PANEL_CHOICE,
#ifdef PROTO
		PANEL_LABEL_STRING, Label[2],
		PANEL_LABEL_BOLD, TRUE,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_NCOLS, 2,
#else
		PANEL_LAYOUT, PANEL_VERTICAL,
#endif
		PANEL_CHOICE_STRINGS, 
			"Name", "Date", "Size", "Type by Name", 
			"Color by Name", "None", 0,
		PANEL_VALUE, Fv_sortby,
#ifdef OPENLOOK
		PANEL_ITEM_Y, ATTR_ROW(row)-10,
#endif
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);

#ifdef PROTO
	row += 2;
#else
# ifdef SV1
	row += 7;
# else
	row += 4;
# endif
#endif

	/* Do a 'du -s' on folders instead of file size */

	Wantdu_item = panel_create_item(Props_panel, PANEL_CHOICE,
		PANEL_LABEL_STRING, Label[3],
		PANEL_LABEL_BOLD, TRUE,
		PANEL_CHOICE_STRINGS,
			"Container",
			"Contents", 0,
		PANEL_VALUE, Fv_wantdu,
		PANEL_ITEM_Y, ATTR_ROW(row++),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);

	Dot_item = panel_create_item(Props_panel, PANEL_CHOICE,
		PANEL_CHOICE_STRINGS,
			"Invisible",
			"Visible", 0,
		PANEL_LABEL_STRING, Label[4],
		PANEL_LABEL_BOLD, TRUE,
		PANEL_VALUE, Fv_see_dot,
		PANEL_ITEM_Y, ATTR_ROW(row++),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);

	/* Confirm file/folder deletions */

	Confirm_delete = panel_create_item(Props_panel, PANEL_CHOICE,
		PANEL_CHOICE_STRINGS,
			"Confirm",
			"Don't Confirm", 0,
		PANEL_LABEL_STRING, Label[5],
		PANEL_LABEL_BOLD, TRUE,
		PANEL_VALUE, !Fv_confirm_delete,
		PANEL_ITEM_Y, ATTR_ROW(row++),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);

	/* Allow user to switch editors on generic documents; ie
	 * documents without an action bound to them.
	 */
	Editor_item = panel_create_item(Props_panel, PANEL_CHOICE,
		PANEL_LABEL_STRING, Label[6],
		PANEL_LABEL_BOLD, TRUE,
		PANEL_CHOICE_STRINGS, "textedit", "vi", "Other:", 0,
		PANEL_NOTIFY_PROC, show_other,
		PANEL_VALUE, Fv_editor,
		PANEL_ITEM_Y, ATTR_ROW(row++),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);

	Other_editor_item = panel_create_item(Props_panel, PANEL_TEXT,
		PANEL_VALUE_DISPLAY_LENGTH, 10,
		PANEL_VALUE, Fv_other_editor,
		PANEL_SHOW_ITEM, Fv_editor==FV_OTHER_EDITOR,
		0);

	/* Automatically update changes to folder every n seconds */

	Slider_item = panel_create_item(Props_panel, PANEL_SLIDER,
		PANEL_LABEL_STRING, Label[7],
		PANEL_LABEL_BOLD, TRUE,
		PANEL_VALUE, Fv_update_interval,
		PANEL_MIN_VALUE, 0,
		PANEL_MAX_VALUE, 99,
		PANEL_SLIDER_WIDTH, 200,
		PANEL_SHOW_RANGE, FALSE,
		PANEL_SHOW_VALUE, FALSE,
		PANEL_NOTIFY_PROC, show_slider_value,
		PANEL_ITEM_Y, ATTR_ROW(row++),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);
	Slider_feedback_item = panel_create_item(Props_panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, buf, 0);
	show_slider_value();

	Filter_item = panel_create_item(Props_panel, PANEL_TEXT,
		PANEL_LABEL_STRING, Label[8],
		PANEL_LABEL_BOLD, TRUE,
		PANEL_VALUE_DISPLAY_LENGTH, 30,
		PANEL_VALUE, Fv_filter,
		PANEL_ITEM_Y, ATTR_ROW(row++),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);

	Print_item = panel_create_item(Props_panel, PANEL_TEXT,
		PANEL_LABEL_STRING, Label[9],
		PANEL_LABEL_BOLD, TRUE,
		PANEL_VALUE_DISPLAY_LENGTH, 30,
		PANEL_VALUE, Fv_print_script,
		PANEL_ITEM_Y, ATTR_ROW(row++),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);

	sprintf(buf, "%d", Fv_maxwidth/Fv_fontsize.x);
	Width_item = panel_create_item(Props_panel, PANEL_TEXT,
		PANEL_LABEL_STRING, Label[10],
		PANEL_LABEL_BOLD, TRUE,
		PANEL_VALUE_DISPLAY_LENGTH, 3,
		PANEL_VALUE_STORED_LENGTH, 4,
		PANEL_VALUE, buf,
		PANEL_ITEM_Y, ATTR_ROW(row++),
		PANEL_VALUE_X, ATTR_COL(VALUE_COL),
		0);


#ifdef PROTO
	frame_set_font_size(Props_frame, 
		(int)window_get(Fv_frame, WIN_FONT_SIZE));
#endif
	window_fit(Props_panel);
	window_fit(Props_frame);
	fv_winright(Props_frame);

	window_set(Props_frame, WIN_SHOW, TRUE, 0);
}

#ifdef OPENLOOK
static
activate_list()
{
	panel_set(Style_item, PANEL_INACTIVE, 
		!panel_get(Icon_item, PANEL_VALUE), 0);
}
#endif

static void
show_other()		/* Display Other Editor_item text line when chosen */
{
	panel_set(Other_editor_item, PANEL_SHOW_ITEM, 
		(int)panel_get_value(Editor_item) == FV_OTHER_EDITOR, 0);
	
}


static void
show_slider_value()
{
	char buf[20];
	int secs = (int)panel_get_value(Slider_item);
	if (secs)
		sprintf(buf, "(%d secs)", secs);
	else
		strcpy(buf, "Disabled");
	panel_set(Slider_feedback_item, PANEL_LABEL_STRING, buf, 0);
}


static void
apply()
{
	props_proc(FRAME_PROPS_SET);
}

static void
reset()
{
	props_proc(FRAME_PROPS_RESET);
}


static void
props_proc(op)
	short op;
{
	int see_dot, width;
	char *filter;
	char buf[4];
	BOOLEAN change = FALSE;		/* When folder has to be reread */


	if (op == FRAME_PROPS_SET)
	{
		Fv_style = (int)panel_get_value(Style_item);
		Fv_style <<= 1;
		if (!panel_get_value(Icon_item))
			Fv_style |= FV_DICON;
		fv_file_scroll_height();
		Fv_sortby = (short)panel_get_value(Sort_item);
		see_dot = (BOOLEAN)panel_get_value(Dot_item);
		if (Fv_see_dot != see_dot)
			Fv_see_dot = see_dot, change++;
		Fv_wantdu = (BOOLEAN)panel_get_value(Wantdu_item);
		Fv_confirm_delete = !(BOOLEAN)panel_get_value(Confirm_delete);

		Fv_editor = (short)panel_get_value(Editor_item);
		strcpy(Fv_other_editor, (char*)panel_get_value(Other_editor_item));
		filter = (char *)panel_get_value(Filter_item);
		change += strcmp(Fv_filter, filter);
		(void)strcpy(Fv_filter, filter);
		(void)strcpy(Fv_print_script, (char*)panel_get_value(Print_item));
		width = atoi(panel_get_value(Width_item)) * Fv_fontsize.x;
		if (width != Fv_maxwidth)
			Fv_maxwidth = width;

		/* Reread directory if change */

		fv_display_folder(change ? FV_BUILD_FOLDER : FV_STYLE_FOLDER);
		fv_namestripe();

		fv_auto_update((int)panel_get_value(Slider_item));

#ifdef OPENLOOK
		/* Take popup down if not pinned */
		if (!window_get(Props_frame, FRAME_PROPS_PUSHPIN_IN))
#endif
			done_proc();

	}
	else
	{
		panel_set(Editor_item, PANEL_VALUE, Fv_editor, 0);
		panel_set(Wantdu_item, PANEL_VALUE, Fv_wantdu, 0);
		panel_set(Confirm_delete, PANEL_VALUE, !Fv_confirm_delete, 0);
		panel_set(Slider_item, PANEL_VALUE, Fv_update_interval, 0);
		panel_set(Icon_item, PANEL_VALUE, !(Fv_style & FV_DICON), 0);
		panel_set(Style_item, PANEL_VALUE, Fv_style>>1, 0);
		panel_set(Sort_item, PANEL_VALUE, Fv_sortby, 0);
		panel_set(Dot_item, PANEL_VALUE, Fv_see_dot, 0);
		panel_set(Filter_item, PANEL_VALUE, Fv_filter, 0);
		panel_set(Print_item, PANEL_VALUE, Fv_print_script, 0);
		panel_set(Other_editor_item, PANEL_VALUE, Fv_other_editor, 0);
		(void)sprintf(buf, "%d", Fv_maxwidth/Fv_fontsize.x);
		panel_set(Width_item, PANEL_VALUE, buf, 0);
	}
}


static void
done_proc()
{
	window_destroy(Props_frame);
	Props_frame = NULL;
}


fv_auto_update(timer)
	int timer;
{
	struct itimerval period;	/* Itimer interval */
	static void freq_itimer();

	period.it_interval.tv_sec = timer;
	period.it_interval.tv_usec = 0;
	period.it_value.tv_sec = timer;
	period.it_value.tv_usec = 0;
	notify_set_itimer_func(Fv_frame, freq_itimer, ITIMER_REAL, &period, 0);
	Fv_update_interval = timer;
}


static void
freq_itimer()				/* ARGS IGNORED */
{
	struct stat fstatus;

	/* Ignore update when iconic */

	if (window_get(Fv_frame, FRAME_CLOSED))
		return;

	if (stat(Fv_openfolder_path, &fstatus) == 0)
		if (fstatus.st_mtime > Fv_current->mtime)
		{
			fv_display_folder(FV_BUILD_FOLDER);
			Fv_current->mtime = fstatus.st_mtime;
		}
}


#ifdef SV1
static Menu Ok_menu;

#ifndef PROTO
static void
ok_button(item, event)
	Panel_item item;
	Event *event;
{
	if (event_is_down(event) && event_id(event) == MS_RIGHT)
		menu_show(Ok_menu, Props_panel, event, 0);
	else if (event_is_up(event) && event_id(event) == MS_LEFT)
		apply();
	else
		panel_default_handle_event(item, event);
}
#endif


static
make_ok_button()
{
	Ok_menu = menu_create(
		MENU_ITEM,
			MENU_STRING, "Apply",
			MENU_NOTIFY_PROC, apply, 0,
		MENU_ITEM,
			MENU_STRING, "Reset",
			MENU_NOTIFY_PROC, reset, 0,
		0);
	panel_create_item(Props_panel, MY_MENU(Ok_menu),
		MY_BUTTON_IMAGE(Props_panel, "OK"),
#ifndef PROTO
		PANEL_EVENT_PROC, ok_button,
#endif
		0);
}
#endif
