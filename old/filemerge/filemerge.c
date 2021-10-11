#ifndef lint
static  char sccsid[] = "@(#)filemerge.c 1.1 92/08/05 Copyr 1986 Sun Micro";
#endif


/* 
 *		To Do:	tabs in getlen in sdiff
 */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/scrollbar.h>
#include <suntool/selection_svc.h>
#include <suntool/selection_attributes.h>
#include <sys/dir.h>   /* MAXNAMLEN */
#include <suntool/text.h>
#include "diff.h"
#include <suntool/entity_stream.h>

#define ABS(x) ((x) < 0 ? (-(x)) : (x))
#define updatebubble(x) window_set(x, TEXTSW_UPDATE_SCROLLBAR, 0);

static short icon_image[] ={
#include "filemerge.icon"
};
DEFINE_ICON_FROM_IMAGE(filemerge_icon, icon_image);

Notify_value	text_interpose();
Notify_value	quit_signal();
Es_handle	ts_create();
int		text_notify();
int	frame_event(), lock_toggle(), quit_button(), undo_button(),
	load_button(), bug_button(), left_button(), prev_button(),
	next_button(), popup_event(), maindone_button(), cnt_message();
int	debug_button(), merge_button();
char	*filename(), *filename3(), *expand();

char	*file1 = "";
char	*file2 = "";
char	*file3 = "";
static	char	*file1orig = "";
static	char	*file2orig = "";
static	char	*file3orig = "";
static	char	*commonfileorig = "";

static	char	*tmp1 = "/tmp/dt1.XXXXX";
static	char	*tmp2 = "/tmp/dt2.XXXXX";
static	char	*tmp3 = "/tmp/dt3.XXXXX";
static	char	*tmp4 = "/tmp/dt4.XXXXX";
static	FILE	*fp1, *fp2, *fp3;
static	int	client;
static	int	curgrpno;			/* what is currently bolded */
static	int	lock = 1;
static	Scrollbar sb1, sb2, sb3;
/*  is gap necessary? XXX */
static	int	gaps[MAXGAP];		/* XXX should be malloced */
static	int	gapind;

static	totlines;

#define LEFT 0
#define RIGHT 1
/*
 * state is indexed by groupno and keeps track of whether text3
 * is LEFT or RIGHT
 */
static char	state[MAXGROUP];	/* XXX should be malloced */

/*
 * history is record of the groupno's that were modified.
 * Sign keeps track of left vs right.  Hence it is actually groupno+1
 * since -0 = 0
 */
#define MAXHIST 200		/* must be even */
static int	history[MAXHIST];
static int	histind;

static Textsw_mark	*marks;
static	int		markind;

static	Panel_item	file1_item, file2_item, file3_item, cnt_item,
			left_item, right_item,
			done_item, load_item, quit_item, merge_item;

struct glyphlist glyphlist[MAXGLYPHS];	/* XXX should be malloced */
static	int groupind;			/* highest group no in glyphlist */

main(argc, argv)
	char **argv;
{
	int	wd, totwd, ht, totht, i;
	int	savepgrp;
	struct pixfont *pf_sys;
	
	/* filemerge uses a lot of fd's, so make sure that
	 * the per process limit isn't exceeded
	 */
	for (i = getdtablesize(); i >=3; i--)
		close(i);
	savepgrp = getpgrp(0);
	initglyphtable();
	pf_sys = pw_pfsysopen();
	charht = pf_sys->pf_defaultsize.y;
	charwd = pf_sys->pf_defaultsize.x;
	/* readrc must come before parseargs */
	readrc();
	parseargs(argc, argv);
	if (debug)
		malloc_debug(2);
	file1orig = file1;
	file2orig = file2;
	if (file3 && file3[0])
		file3orig = file3;
	else {
		file3 = file3orig = "filemerge.out";
	}
	commonfileorig = commonfile;
	/* totht doesn't include height of panel */
	computesizes(&wd, &totwd, &ht, &totht);
	
	frame = window_create(NULL, FRAME,
		WIN_CONSUME_KBD_EVENT, KEY_LEFT(3),
		WIN_CONSUME_PICK_EVENT, KEY_LEFT(3),
		PANEL_EVENT_PROC, popup_event,
		WIN_WIDTH, 1000,  /* so panel items won't wrap */
		WIN_HEIGHT, totht,
		ATTR_LIST, tool_attrs,
		FRAME_ICON, &filemerge_icon,
		WIN_EVENT_PROC, frame_event,
		0);
	if (frame == 0) {
		fprintf(stderr, "filemerge: Couldn't create window\n");
		exit(1);
	}
	tool_free_attribute_list(tool_attrs);
	panel = window_create(frame, PANEL,
		WIN_CONSUME_KBD_EVENT, KEY_LEFT(3),
		WIN_CONSUME_PICK_EVENT, KEY_LEFT(3),
		PANEL_EVENT_PROC, popup_event, 0);
	makepanelitems();
	window_set(frame, WIN_HEIGHT,
	    (int)window_get(frame, WIN_HEIGHT)
	    + (int)window_get(panel, WIN_HEIGHT),
		WIN_WIDTH, totwd, 0);
	
	sb1 = (Scrollbar)scrollbar_create(SCROLL_PLACEMENT, SCROLL_WEST, 0);
	text1 = window_create(frame, TEXTSW,
		TEXTSW_SCROLLBAR, sb1,
		TEXTSW_LINE_BREAK_ACTION, TEXTSW_CLIP,
		TEXTSW_RIGHT_MARGIN, TEXTMARGIN,
		TEXTSW_DISABLE_LOAD, 1,
		TEXTSW_DISABLE_CD, 1,
		TEXTSW_READ_ONLY, 1,
		WIN_HEIGHT, readonly ? -1 : ht,
		WIN_WIDTH, wd,
		0);

	sb2 = (Scrollbar)scrollbar_create(SCROLL_PLACEMENT, SCROLL_EAST, 0);
	text2 = window_create(frame, TEXTSW,
		TEXTSW_SCROLLBAR, sb2,
		TEXTSW_DISABLE_LOAD, 1,
		TEXTSW_DISABLE_CD, 1,
		TEXTSW_LINE_BREAK_ACTION, TEXTSW_CLIP,
		TEXTSW_LEFT_MARGIN, TEXTMARGIN,
		TEXTSW_READ_ONLY, 1,
		WIN_HEIGHT, readonly ? -1 : ht,
		WIN_RIGHT_OF, text1,
		0);

	if (!readonly) {
		text3 = window_create(frame, TEXTSW,
			TEXTSW_NOTIFY_LEVEL, TEXTSW_NOTIFY_EDIT,
			TEXTSW_LINE_BREAK_ACTION, TEXTSW_CLIP,
			TEXTSW_LEFT_MARGIN, TEXTMARGIN,
			TEXTSW_NOTIFY_PROC, text_notify,
			TEXTSW_ES_CREATE_PROC, ts_create,
			TEXTSW_DISABLE_LOAD, 1,
			TEXTSW_READ_ONLY, 0,
			WIN_X, 0,
			WIN_BELOW, text1,
			0);
		sb3 = (Scrollbar)window_get(text3, TEXTSW_SCROLLBAR);
		initdirectory();
	}
	adjustposition();
	
	mktemp(tmp1);
	mktemp(tmp2);
	if (commonfile) {
		mktemp(tmp3);
		mktemp(tmp4);
	}
	notify_interpose_event_func(text1, text_interpose, NOTIFY_SAFE);
	notify_interpose_event_func(text2, text_interpose, NOTIFY_SAFE);
	if (!readonly)
		notify_interpose_event_func(text3,text_interpose,NOTIFY_SAFE);

	notify_set_signal_func(&client, quit_signal,
	    SIGTERM, NOTIFY_ASYNC);
	notify_set_signal_func(&client, quit_signal,
	    SIGINT, NOTIFY_ASYNC);
	setpgrp(0, savepgrp);
	window_main_loop(frame);
	quit_signal(0, 0);
	/* NOTREACHED */
}

/* ARGSUSED */
/* used from panel and frame */
popup_event(item, event)
	Panel_item	item;
	Event		*event;
{
	if (event_id(event) == KEY_LEFT(3))
		popitup();
	else
		panel_default_handle_event(item, event);
}

Notify_value
text_interpose(text, event, arg, type)
	Textsw	text;
	Event	*event;
	Notify_arg arg;
	Notify_event_type type;
{
	Notify_value	value;
	int	x, y, z, pos;
	int	i;
	Event	ev;
	static	first = 1;
	
	value = notify_next_event_func(text, event, arg, type);
	if (first && event_id(event) == WIN_REPAINT
	    && (text == text3 || (readonly && text == text2))) {
		first = 0;
		if (file2[0]) {
			if (list)
				event_set_shiftmask(&ev, SHIFTMASK);
			else
				event_set_shiftmask(&ev, 0);
			i = load_button(0, &ev);
		}
		if (file2[0] && i == 0)
			putnamestripe();
		else	
			window_set(frame, FRAME_LABEL, "filemerge", 0);
	}
	if (event_id(event) == KEY_LEFT(3)) {
		popitup();
		return (value);
	}
	if (event_id(event) == SCROLL_REQUEST && lock) {
		x = (int)window_get(text1, TEXTSW_FIRST_LINE);
		y = (int)window_get(text2, TEXTSW_FIRST_LINE);
		if (text == text1) {
			textsw_scroll_lines(text2, x - y);
			updatebubble(text2);
			if (!readonly) {
				pos = textsw_find_mark(text3,marks[x]);
				textsw_normalize_view(text3, pos);
				updatebubble(text3);
			}
		}
		else if (text == text2) {
			textsw_scroll_lines(text1, y - x);
			updatebubble(text1);
			if (!readonly) {
				pos = textsw_find_mark(text3,marks[y]);
				textsw_normalize_view(text3, pos);
				updatebubble(text3);
			}
		}
		else {
			pos = (int)window_get(text3, TEXTSW_FIRST);
			z = pos_to_mark(pos);
			textsw_scroll_lines(text1, z - x);
			updatebubble(text1);
			textsw_scroll_lines(text2, z - y);
			updatebubble(text2);
		}
	}
	return(value);
}

/* ARGSUSED */
debug_button(item, event)
	Panel_item item;
	struct inputevent *event;
{
	showmarks();
}

relayoutpanel()
{
	int		ln, x, wd;

	wd = (int)window_get(frame, WIN_COLUMN_WIDTH);
	ln = (int)window_get(frame, WIN_COLUMNS);
	if (commonfile)
		ln = (ln - 2*strlen(" File1: ") - strlen(" Ancestor: ") - 7)/3;
	else
		ln = (ln - 2*strlen(" File1: ") - 3*strlen(" Load ") - 7)/2;
	panel_set(file1_item,
		PANEL_VALUE_DISPLAY_LENGTH, ln, 0);
	panel_set(file2_item,
		PANEL_ITEM_X, ATTR_COL(8+ln),
		PANEL_VALUE_DISPLAY_LENGTH, ln, 0);
	x = ATTR_COL(16 + 2*ln);
	if (commonfile) {
		panel_set(file3_item,
			PANEL_ITEM_X, ATTR_COL(16 + 2*ln),
			PANEL_VALUE_DISPLAY_LENGTH, ln, 0);
		panel_set(load_item,
			PANEL_ITEM_Y, ATTR_ROW(1) + 5,
			PANEL_ITEM_X, x, 0);
		panel_set(done_item,
			PANEL_ITEM_Y, ATTR_ROW(1) + 5,
			PANEL_ITEM_X, x + 6*wd, 0);
		panel_set(quit_item,
			PANEL_ITEM_Y, ATTR_ROW(1) + 5,
			PANEL_ITEM_X, x + 12*wd, 0);
	} else {
		panel_set(load_item,
			PANEL_ITEM_X, x, 0);
		panel_set(done_item,
			PANEL_ITEM_X, x + 6*wd, 0);
		panel_set(quit_item,
			PANEL_ITEM_X, x + 12*wd, 0);
	}
	panel_paint(panel, PANEL_CLEAR);
}

makepanelitems()
{
	Panel_item	item;
	int	ht, ln;
	
	ln = (int)window_get(frame, WIN_COLUMNS);
	if (commonfile)
		ln = (ln - 2*strlen(" File1: ") - strlen(" Ancestor: ") - 7)/3;
	else
		ln = (ln - 3*strlen(" Load ") - 7)/2;
	file1_item = panel_create_item(panel, PANEL_TEXT,
		PANEL_ITEM_X, ATTR_COL(0),
		PANEL_LABEL_STRING, "File1:",
		PANEL_VALUE, file1,
		PANEL_VALUE_DISPLAY_LENGTH, ln, 0);

	file2_item = panel_create_item(panel, PANEL_TEXT,
		PANEL_ITEM_X, ATTR_COL(8+ln),
		PANEL_LABEL_STRING, "File2:",
		PANEL_VALUE, file2,
		PANEL_VALUE_DISPLAY_LENGTH, ln, 0);

	if (commonfile) {
		file3_item = panel_create_item(panel, PANEL_TEXT,
			PANEL_ITEM_X, ATTR_COL(16 + 2*ln),
			PANEL_LABEL_STRING, "Ancestor:",
			PANEL_VALUE, commonfile,
			PANEL_VALUE_DISPLAY_LENGTH, ln, 0);
		ht = ATTR_ROW(1) + 5;
		load_item = item = panel_create_item(panel, PANEL_BUTTON,
			PANEL_ITEM_X, ATTR_COL(16 + 2*ln),
			PANEL_ITEM_Y, ht,
			PANEL_LABEL_IMAGE,
			    panel_button_image(panel, "Load", 0, 0),
			PANEL_NOTIFY_PROC, load_button, 0);
	} else {
		panel_create_item(panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING, "     ", 0);
		ht = ATTR_ROW(0);
		load_item = item = panel_create_item(panel, PANEL_BUTTON,
			PANEL_ITEM_X, ATTR_COL(16 + 2*ln),
			PANEL_LABEL_IMAGE,
			    panel_button_image(panel, "Load", 0, 0),
			PANEL_NOTIFY_PROC, load_button, 0);
	}
	if (list)
		setupmenu(item, "Load", "Next [Shift]", 1, "", 0, cmdpanel_event);
	
	done_item = item = panel_create_item(panel, PANEL_BUTTON,
		PANEL_ITEM_Y, ht,
		PANEL_LABEL_IMAGE, panel_button_image(panel, "Done", 0, 0),
		PANEL_NOTIFY_PROC, maindone_button, 0);
	setupmenu(item, "Done", "Save [Shift]", 1, "", 0, cmdpanel_event);

	quit_item = panel_create_item(panel, PANEL_BUTTON,
		PANEL_ITEM_Y, ht,
		PANEL_LABEL_IMAGE, panel_button_image(panel, "Quit", 0, 0),
		PANEL_NOTIFY_PROC, quit_button, 0);

	if (debug)
	panel_create_item(panel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, panel_button_image(panel, "Debug", 0, 0),
		PANEL_NOTIFY_PROC, debug_button, 0);

	if (commonfile)
		ht = ATTR_ROW(2) + 10;
	else
		ht = ATTR_ROW(1) + 5;
	item = panel_create_item(panel, PANEL_BUTTON,
		PANEL_ITEM_X, ATTR_COL(0),
		PANEL_ITEM_Y, ht,
		PANEL_LABEL_IMAGE, panel_button_image(panel, "Next", 0, 0),
		PANEL_NOTIFY_PROC, next_button, 0);
	setupmenu(item, "Next", "First [Shift]", 1, "", 0, cmdpanel_event);

	item = panel_create_item(panel, PANEL_BUTTON,
		PANEL_ITEM_Y, ht,
		PANEL_LABEL_IMAGE, panel_button_image(panel, "Prev", 0, 0),
		PANEL_NOTIFY_PROC, prev_button, 0);
	setupmenu(item, "Prev", "Last [Shift]", 1, "", 0, cmdpanel_event);

	panel_create_item(panel, PANEL_MESSAGE,
		PANEL_ITEM_Y, ht,
		PANEL_LABEL_STRING, " ", 0);

	if (!readonly) {
		if (commonfile) {
			merge_item = item = panel_create_item(panel,
			    PANEL_BUTTON, PANEL_ITEM_Y, ATTR_ROW(1) + 5,
			    PANEL_LABEL_IMAGE, panel_button_image(panel,
			    "Merge", 0,0), PANEL_NOTIFY_PROC, merge_button, 0);
			setupmenu(merge_item, "Merge", autoadvance ?
					"Merge & Not Next [Shift]" :
					"Merge & Next     [Shift]", 1,
					"Remainder Merge  [Ctrl]", 2,
					cmdpanel_event);
		}
		left_item = item = panel_create_item(panel, PANEL_BUTTON,
			PANEL_ITEM_Y, ht,
			PANEL_LABEL_IMAGE, panel_button_image(panel,
			    "Left", 0, 0),
			PANEL_NOTIFY_PROC, left_button, 0);
		setupmenu(item, "Left", autoadvance ?
				"Left & Not Next [Shift]" :
				"Left & Next     [Shift]", 1,
				"Remainder Left  [Ctrl]", 2,
				cmdpanel_event);

		right_item = item = panel_create_item(panel, PANEL_BUTTON,
			PANEL_ITEM_Y, ht,
			PANEL_LABEL_IMAGE, panel_button_image(panel, "Right",
			    0, 0),
			PANEL_NOTIFY_PROC, left_button, 0);
		setupmenu(item, "Right", autoadvance ?
				"Right & Not Next [Shift]" :
				"Right & Next     [Shift]", 1,
				"Remainder Right  [Ctrl]", 2,
				cmdpanel_event);

		item = panel_create_item(panel, PANEL_BUTTON,
			PANEL_ITEM_Y, ht,
			PANEL_LABEL_IMAGE, panel_button_image(panel, "Undo",
			    0, 0),
			PANEL_NOTIFY_PROC, undo_button, 0);
		setupmenu(item, "Undo", "Reset [Shift]", 1, "", 0, cmdpanel_event);
	}

	panel_create_item(panel, PANEL_MESSAGE,
		PANEL_ITEM_Y, ht,
		PANEL_LABEL_STRING, " ", 0);

	panel_create_item(panel, PANEL_TOGGLE,
		PANEL_ITEM_Y, ht-3,
		PANEL_CHOICE_STRINGS, "Scroll-Lock", 0,
		PANEL_VALUE, 1,
		PANEL_NOTIFY_PROC, lock_toggle, 0);

	panel_create_item(panel, PANEL_MESSAGE,
		PANEL_ITEM_Y, ht,
		PANEL_LABEL_STRING, " ", 0);


	panel_create_item(panel, PANEL_MESSAGE,
		PANEL_ITEM_Y, ht,
		PANEL_LABEL_STRING, " ", 0);

	cnt_item = panel_create_item(panel, PANEL_MESSAGE,
		PANEL_ITEM_Y, ht,
		PANEL_NOTIFY_PROC, cnt_message, 0);
	setupmenu(cnt_item, "Go To Selected Line", "Go To n", 1, "", 0, cmdpanel_event);

	window_fit_height(panel);
}

/* ARGSUSED */
maindone_button(item, event)
	Panel_item item;
	struct inputevent *event;
{
	char winname[WIN_NAMESIZE];
	int	toolfd, rootfd, shift;
	
	shift = event_shift_is_down(event);
	toolfd = (int)window_get(frame, WIN_FD);
	we_getparentwindow(winname);
	if ((rootfd = open(winname, 0)) < 0) {
		fprintf(stderr, "filemerge: can't open root window %s\n",
		    winname);
		exit(1);
	}
	if (readonly) {
		if (!shift)
			wmgr_close(toolfd, rootfd);
	}
	else if (file2[0] && textsw_save(text3, 0, 0) == 0 && !shift)
		wmgr_close(toolfd, rootfd);
	close(rootfd);
}

/* ARGSUSED */
quit_button(item, event)
	Panel_item item;
	struct inputevent *event;
{
	if (window_get(text3, TEXTSW_MODIFIED))
		fullscreen_prompt("\
You have UNSAVED edits!  Do you\n\
really want to quit? Press left\n\
mouse button to confirm Quit and\n\
lose your changes.   To cancel,\n\
press the right mouse button.", "Really Quit", "Cancel Quit", event,
	    window_get(frame, WIN_FD));
	else
		fullscreen_prompt("\
Press the left mouse button\n\
to confirm Quit.  To cancel,\n\
press the right mouse button.", "Really Quit", "Cancel Quit", event,
	    window_get(frame, WIN_FD));
	if (event_id(event) == MS_LEFT)
		quit_signal();
}


/* ARGSUSED */
cnt_message(item, event)
	Panel_item item;
	struct inputevent *event;
{
	int	i, j;
	char	buf[100];
	Seln_holder	seln_holder;
	Seln_request    *seln_buffer;
	
	if (event_shift_is_down(event)) {
		i = fullscreen_prompt_for_input("Number of difference to jump to:",
		    buf, window_get(frame, WIN_FD));
		if (i == 0 && buf[0])
			i = atoi(buf) - 1;
		else
			return;
	} else {
		seln_holder = seln_inquire(SELN_PRIMARY);
		/* does the headersw have the primary selection? */
		if (seln_holder.state == SELN_NONE ||
		    (!seln_holder_same_client(&seln_holder, text1)
		     && !seln_holder_same_client(&seln_holder, text2))) {
			fullscreen_prompt("No selection!",
			    "Continue", "", event, window_get(frame, WIN_FD));
                        return;
		}
		seln_buffer = seln_ask((Seln_request *)&seln_holder,
		    SELN_REQ_FAKE_LEVEL, SELN_LEVEL_LINE,
		    SELN_REQ_FIRST_UNIT, 0,
		    0);
		if (seln_buffer->status == SELN_FAILED){
			fullscreen_prompt("Can't get selection",
			    "Continue", "", event, window_get(frame, WIN_FD));
                        return;
		}
		i = linetogroup(seln_to_lineno(seln_buffer));
	}
	if (i < 0 || i >= groupind) {
		fullscreen_prompt("Invalid difference number",
		    "Continue", "", event, window_get(frame, WIN_FD));
		return;
	}
	j = grouptoind(curgrpno);
	unboldglyph(j);
	curgrpno = i;
	do_next_button(curgrpno);
}

/* ARGSUSED */
prev_button(item, event)
	Panel_item item;
	struct inputevent *event;
{
	int	i;
	Event	ev;
	
	if (glyphind == 0) {
		fullscreen_prompt("No differences.",
		    "Continue", "", &ev, window_get(frame, WIN_FD));
		return;
	}
	i = grouptoind(curgrpno);
	unboldglyph(i);
	if (event_shift_is_down(event)) /* last */
		curgrpno = groupind-1;
	else {
		curgrpno--;
		if (curgrpno < 0)
			curgrpno += groupind;
	}
	do_next_button(curgrpno);
}

/* ARGSUSED */
next_button(item, event)
	Panel_item item;
	struct inputevent *event;
{
	int	i;
	Event	ev;
	
	if (glyphind == 0) {
		fullscreen_prompt("No differences.",
		    "Continue", "", &ev, window_get(frame, WIN_FD));
		return;
	}
	i = grouptoind(curgrpno);
	unboldglyph(i);
	if (event_shift_is_down(event)) { /* first */
		curgrpno = 0;
	} else {
		curgrpno++;
		if (curgrpno >= groupind)
			curgrpno -= groupind;
	}
	do_next_button(curgrpno);
}

do_next_button(ind)
{
	int	x, y, pos, i, j;
	char	buf[80];
	int	group, line1;
	
	sprintf(buf, "%3d of %3d", ind+1, groupind);
	panel_set(cnt_item, PANEL_LABEL_STRING, buf, 0);
	i = grouptoind(ind);
	boldglyph(i, 0);
	group = glyphlist[i].g_groupno;
	for (j = i; j < glyphind; j++) {
		if (glyphlist[j].g_groupno != group)
			break;
	}
	line1 = glyphlist[j-1].g_lineno;
	my_textsw_possibly_normalize(text1, glyphlist[i].g_lineno, line1, 3);
	updatebubble(text1);
	/* scroll other windows */
	x = (int)window_get(text1, TEXTSW_FIRST_LINE);
	y = (int)window_get(text2, TEXTSW_FIRST_LINE);
	textsw_scroll_lines(text2, x - y);
	updatebubble(text2);
	if (!readonly) {
		pos = textsw_find_mark(text3,marks[x]);
		textsw_normalize_view(text3, pos);
		updatebubble(text3);
		/* textsw_set_selection(text3, start, stop, 1); */
	}
}

/* ARGSUSED */
merge_button(item, event)
	Panel_item item;
	struct inputevent *event;
{
	Event	ev;
	int	i, save;
	
	if (glyphind == 0) {
		fullscreen_prompt("No differences.",
		    "Continue", "", &ev, window_get(frame, WIN_FD));
		return;
	}
	if (event_ctrl_is_down(event)) {
		event_set_shiftmask(event, 0);
		save = autoadvance;
		autoadvance = 0;
		while (1) {
			merge_button(item, event);
			if (curgrpno >= groupind - 1) {
				autoadvance = save;
				return;
			}
			next_button(item, event);
		}
	}
	i = grouptoind(curgrpno);
	if (leftmerge(i, curgrpno))
		left_button(left_item, event);
	else if (rightmerge(i, curgrpno))
		left_button(right_item, event);
	else {
		fullscreen_prompt("Can't merge this",
		    "Continue", "", &ev, window_get(frame, WIN_FD));
	}
}

/* ARGSUSED */
left_button(item, event)
	Panel_item item;
	struct inputevent *event;
{
	int	shift, save;
	Event	ev;

	if (glyphind == 0) {
		fullscreen_prompt("No differences.",
		    "Continue", "", &ev, window_get(frame, WIN_FD));
		return;
	}
	if (event_ctrl_is_down(event)) {
		event_set_shiftmask(event, 0);
		save = autoadvance;
		autoadvance = 0;
		while (1) {
			left_button(item, event);
			if (curgrpno >= groupind - 1) {
				autoadvance = save;
				return;
			}
			next_button(item, event);
		}
	}
	if (item == left_item) {
		if (state[curgrpno] != LEFT) {
			if (histind >= MAXHIST)
				shifthistory();
			history[histind++] = -(curgrpno + 1);
			substitute(text1, curgrpno);
		}
	} else {
		if (state[curgrpno] != RIGHT) {
			if (histind >= MAXHIST)
				shifthistory();
			history[histind++] = curgrpno + 1;
			substitute(text2, curgrpno);
		}
	}
	shift = event_shift_is_down(event);
	if ((autoadvance && !shift) || (!autoadvance && shift)) {
		event_set_shiftmask(event, 0);
		next_button(item, event);
	}
}

/* ARGSUSED */
undo_button(item, event)
	Panel_item item;
	struct inputevent *event;
{
	int	i;
	Event	ev;
	
	if (event_shift_is_down(event)) {
		while (histind > 0) {
			histind--;
			if (history[histind] < 0)
				substitute(text2, -history[histind] - 1);
			else
				substitute(text1, history[histind] - 1);
		}
	}
	else if (histind <= 0) {
		fullscreen_prompt("Nothing more to undo",
		    "Continue", "", &ev, window_get(frame, WIN_FD));
		return;
	} else {
		histind--;
		if (history[histind] < 0)
			substitute(text2, -history[histind] - 1);
		else
			substitute(text1, history[histind] - 1);
	}
	if (moveundo) {
		i = grouptoind(curgrpno);
		unboldglyph(i);
		curgrpno = ABS(history[histind]) - 1;
		do_next_button(curgrpno);
	}
}

substitute(text, grpno)
	int	grpno;
	Textsw	text;
{
	int	i, j;
	int	line1, line2;
	int	char1, char2, savech, savei, savestate;
	int	first, lastp;
	char	*z;

	savestate = state[grpno];
	if (text == text1)
		state[grpno] = LEFT;
	else
		state[grpno] = RIGHT;
	i = j = grouptoind(grpno);
	savei = i;
	for (; j < glyphind; j++) {
		if (glyphlist[j].g_groupno != grpno)
			break;
	}
	j--;
	line1 = glyphlist[i].g_lineno;
	line2 = glyphlist[j].g_lineno;
	char1 = textsw_find_mark(text3,marks[line1]);
	savech = char1;
	char2 = textsw_find_mark(text3,marks[line2+1]);
	if (debug)
		printf("before delete: %d %d\n",
		    textsw_find_mark(text3,marks[line1]),
		    textsw_find_mark(text3,marks[line2+1]));
	/* 
	 * For '>', nothing to delete, only add stuff
	 * For '<', nothing to add, only delete
	 */
	/* XXX lchar vs rchar ???? */
	if (!commonfile) {
		if ((savestate == LEFT && glyphlist[savei].g_rchar != '>')
		    || (savestate == RIGHT && glyphlist[savei].g_lchar != '<'))
			textsw_delete(text3, char1, char2);
	} else {
		if ((savestate == LEFT && glyphlist[savei].g_lchar != '-'
		    && glyphlist[savei].g_rchar != '+') || 
		    (savestate == RIGHT && glyphlist[savei].g_rchar != '-'
		    && glyphlist[savei].g_lchar != '+'))
			textsw_delete(text3, char1, char2);
	}
	char1 = textsw_index_for_file_line(text, line1);
	char2 = textsw_index_for_file_line(text, line2+1);
	if (char2 < 0)
		char2 = (int)window_get(text, TEXTSW_LENGTH);
	i = char2 - char1;
	z = (char *)malloc(i+1);
	window_get(text, TEXTSW_CONTENTS, char1, z, i);
	window_set(text3, TEXTSW_INSERTION_POINT, savech, 0);

	if ((!commonfile && ((savestate == LEFT
	    && glyphlist[savei].g_lchar != '<')
	    || (savestate == RIGHT && glyphlist[savei].g_rchar != '>')))

	|| (commonfile && ((savestate == LEFT && glyphlist[savei].g_rchar!= '-'
	    && glyphlist[savei].g_lchar != '+') || (savestate == RIGHT
	    && glyphlist[savei].g_lchar != '-'
	    && glyphlist[savei].g_rchar != '+')))) {

		textsw_insert(text3, z, i);
		/* textsw_set_selection(text3, savech, savech+i, 1); */
		grpno = first = savech;
		for (i = line1; i <= line2; i++) {
			textsw_remove_mark(text3, marks[i]);
			marks[i] = textsw_add_mark(text3, grpno,
			    TEXTSW_MARK_MOVE_AT_INSERT);
			grpno = textsw_find_bytes(text3, &first, &lastp, "\n",
			    1, 0) +1;
			first = lastp;
		}
	} else {
		/* marks pile up on next available line on '<' */
		first = savech;
		grpno = textsw_find_bytes(text3, &first, &lastp, "\n", 1, 0)+1;
		/* textsw_set_selection(text3, savech, lastp, 1); */
		for (i = line1; i <= line2; i++) {
			textsw_remove_mark(text3, marks[i]);
			marks[i] = textsw_add_mark(text3, savech,
			    TEXTSW_MARK_MOVE_AT_INSERT);
		}
	}

	/* make sure bottom textwindow is in sync */
	if (!readonly) {
		i = (int)window_get(text1, TEXTSW_FIRST_LINE);
		j = textsw_find_mark(text3,marks[i]);
		textsw_normalize_view(text3, j);
		updatebubble(text3);
	}
	i = grouptoind(curgrpno);
	boldglyph(i, 1);
	free(z);
}

/* ARGSUSED */
lock_toggle(item, value, event)
	Panel_item item;
	struct inputevent *event;
{
	int	x, y, pos;
	
	lock = value;
	if (lock) {
		x = (int)window_get(text1, TEXTSW_FIRST_LINE);
		y = (int)window_get(text2, TEXTSW_FIRST_LINE);
		textsw_scroll_lines(text2, x - y);
		if (!readonly) {
			pos = textsw_find_mark(text3,marks[x]);
			textsw_normalize_view(text3, pos);
			updatebubble(text3);
		}
		updatebubble(text2);
	}
}

/* returns 0 if OK */
/* ARGSUSED */
load_button(item, event)
	Panel_item item;
	struct inputevent *event;
{
	char	listfilename[200];
	int	notok = 1, x, res;
	Event	ev;

	if (window_get(text3, TEXTSW_MODIFIED)) {
		fullscreen_prompt("\
You have UNSAVED edits!  Do you\n\
really want to load? Press left\n\
mouse button to confirm Load and\n\
lose your changes.   To cancel,\n\
press the right mouse button.", "Really Load", "Cancel Load", &ev,
		    window_get(frame, WIN_FD));
		if (event_id(&ev) != MS_LEFT)
			return(1);
	}
	if (event_shift_is_down(event)) {
		x = fscanf(listfp, "%s", listfilename);
		if (x != 1) {
			error("No more files", 0);
			listfilename[0] = 0;
			return(1);
		}
	} else
		listfilename[0] = 0;
	changecursor();
	removeglyphs();
	removemarks();
	bzero(gaps, sizeof(gaps));
	resetextra();
	bzero(state, sizeof(state));
	bzero(glyphlist, sizeof(glyphlist));
	histind = 0;
	curgrpno = 0;
	/* XXX is gap necessary? */
	gaps[0] = -1;
	gapind = 0;

	truncate(tmp1, 0);
	fp1 = fopen(tmp1, "w");
	if (fp1 == NULL) {
		fprintf(stderr, "filemerge: can't open %s\n", tmp1);
		exit(1);
	}
	/* check to see if either name is a directory */
	file1 = expand(panel_get_value(file1_item));
	file2 = expand(panel_get_value(file2_item));
	commonfile = expand(panel_get_value(file3_item));
	if (listfilename[0] == 0) {
		if (isdir(file1) && isdir(file2)) {
			error("Can't have both input files be directories", 0);
			goto done;
		}
		if (commonfile)
			commonfile = filename3(commonfile, file1, file2);
		file1 = filename(file1, file2);
		file2 = filename(file2, file1);
	} else {
		file1 = filename(file1orig, listfilename);
		file2 = filename(file2orig, listfilename);
		if (commonfile)
			commonfile = filename(commonfileorig, listfilename);
		panel_set(file1_item, PANEL_VALUE, file1, 0);
		panel_set(file2_item, PANEL_VALUE, file2, 0);
	}
	if (!readonly) {
		if (listfilename[0] == 0) {
			if (file3 == 0 || file3[0] == 0)
				file3 = "filemerge.out";
			else	
				file3 = filename3(file3, file1);
		}
		else {
			if (isdir(file3orig)) {
				if (!isdir(file1))
					file3 = filename(file3orig, file1);
				else if (!isdir(file2))
					file3 = filename(file3orig, file2);
				else
					fprintf(stderr,
	   "filemerge: at least one file argument must be a non-directory\n");
			}
		}
		if (samecheck(file1, file3)) {
			file3 = "";
			goto done;
		}
		if (samecheck(file2, file3)) {
			file3 = "";
			goto done;
		}
		/* XXX should check for overwriting */
		fp3 = fopen(file3, "w");
		if (fp3 == NULL) {
			error("Cannot open: %s",file3);
			goto done;
		}
	}
	window_set(frame, FRAME_LABEL, "performing diff....", 0);
	dontrepaint = 1;
	if (commonfile) {
		firstpass = 1;
		res = sdiff(file1, commonfile, blanks);
	} else {
		truncate(tmp2, 0);
		fp2 = fopen(tmp2, "w");
		if (fp2 == NULL) {
			fprintf(stderr, "filemerge: can't open %s\n", tmp2);
			exit(1);
		}
		res = sdiff(file1, file2, blanks);
	}
	if (res) {
		if (commonfile) {
			truncate(tmp2, 0);
			fp2 = fopen(tmp2, "w");
			if (fp2 == NULL) {
				fprintf(stderr, "filemerge: can't open %s\n",
				    tmp3);
				exit(1);
			}
			firstpass = 0;
			res = sdiff(commonfile, file2, blanks);
			if (!res)
				goto done;
			fclose(fp1);
			fclose(fp2);
			fp1 = fopen(tmp1, "r");
			if (fp1 == NULL) {
				fprintf(stderr, "filemerge: can't open %s\n",
				    tmp1);
				exit(1);
			}
			fp2 = fopen(tmp2, "r");
			if (fp2 == NULL) {
				fprintf(stderr, "filemerge: can't open %s\n",
				    tmp1);
				exit(1);
			}
			totlines = combine(fp1, fp2, tmp3, tmp4);
		}
		panel_set(cnt_item, PANEL_LABEL_STRING, "", 0);
		invalidate(0);
		fflush(fp1);
		window_set(text1, TEXTSW_FILE, commonfile ? tmp3 : tmp1,
			TEXTSW_FIRST, 0,
			TEXTSW_READ_ONLY, 1, 0);
		window_set(text1, TEXTSW_READ_ONLY, 1, 0);
		fflush(fp2);
		window_set(text2, TEXTSW_FILE, commonfile ? tmp4 : tmp2,
			TEXTSW_FIRST, 0,
			TEXTSW_READ_ONLY, 1, 0);
		window_set(text2, TEXTSW_READ_ONLY, 1, 0);
		if (!readonly) {
			fflush(fp3);
			window_set(text3, TEXTSW_FILE, file3,
				TEXTSW_FIRST, 0,
				TEXTSW_READ_ONLY, 0, 0);
			initnamestripe();
			putmarks(text3);
		}
		groupind = paintglyph();
		notok = 0;
		/* if there are any differences, bold the first one */
		if (glyphind > 0)
			do_next_button(0);
		else
			panel_set(cnt_item, PANEL_LABEL_STRING,
			    "No differences", 0);
	}
  done:
	if (file2[0] && !readonly && !notok)
		putnamestripe();
	else
		window_set(frame, FRAME_LABEL, "filemerge", 0);
	dontrepaint = 0;
	fclose(fp1);
	fclose(fp2);
	fclose(fp3);
	if (notok) {
		textsw_reset(text1, 0, 0);
		textsw_reset(text2, 0, 0);
		if (!readonly)
			textsw_reset(text3, 0, 0);
	}
	restorecursor();
	return(notok);
}

removemarks()
{
	int	i;
	
	window_set(frame, FRAME_LABEL, "deleting marks....", 0);
	if (marks) {
		/* for (i = 0; i < markind; i++) */
		for (i = markind-1; i >= 0; i--)
			textsw_remove_mark(text3, marks[i]);
		free(marks);
	}
	marks = NULL;
}

putmarks(text)
	Textsw	text;
{
	int	i, ind, g, lines;
	int	first, last;

	if (commonfile)
		lines = totlines;
	else
		lines = linecnt;
	window_set(frame, FRAME_LABEL, "adding marks....", 0);
	marks = (Textsw_mark *)malloc((lines+1)*sizeof(Textsw_mark));
	first = 0;
	ind = 0;
	g = 0;
	for (i = 0; i < lines; i++) {
		marks[i] = textsw_add_mark(text, ind,
		    TEXTSW_MARK_MOVE_AT_INSERT);
		if (i == gaps[g])
			g++;
		else {
			textsw_find_bytes(text, &first, &last, "\n", 1, 0);
			ind = first = last;
		}
	}
	marks[lines] = textsw_add_mark(text, ind,TEXTSW_MARK_MOVE_AT_INSERT);
	markind = lines+1;
}

/* from bsearch() */
pos_to_mark(pos)
{
	Textsw_mark	*base, *last = marks + (markind-1);
	register Textsw_mark	*p;
	register	res;
	
	base = marks;
	while (last >= base) {
		p = base + (last - base)/2;
		res = pos - textsw_find_mark(text3, *p);
		if (res == 0)
			break;
		else if (res < 0)
			last = p - 1;
		else
			base = p + 1;
	}
	last = marks + (markind-1);
	for ( ; p < last; p++)
		if (textsw_find_mark(text3, *p) > pos)
			return (p - marks - 1);
	return(markind-1);
}

/* XXX Grossly ineffecient */
shiftgap(line)
{
	int	i, old, tmp, first;
	
	first = 1;
	line++;
	for (i = 0; i < gapind; i++) {
		if (gaps[i] < line && first)
			continue;
		else if (first) {
			first = 0;
			gapind++;
			old = gaps[i];
			gaps[i] = line;
		}
		else {
			tmp = gaps[i];
			gaps[i] = old+1;
			old = tmp;
		}
	}
	if (first) {
		gaps[gapind] = line;
		gapind++;
	}
}

puttext1(str, ln)
	char *str;
{
	if (firstpass || !commonfile) {
		if (fwrite(str, sizeof(char), ln, fp1) != ln) {
			fprintf(stderr, "filemerge: fwrite failed on %s\n",
			    tmp1);
			exit(1);
		}
		if (!readonly)
			if (fwrite(str, sizeof(char), ln, fp3) != ln) {
				fprintf(stderr,
				    "filemerge: fwrite failed on %s\n",
				    file3);
				exit(1);
			}
	}
}

/*
 * If a blank was inserted into file1 solely to match up extra lines
 * in file2, then don't put those lines into file3
 */
puttext1leftpad(str, ln)
	char *str;
{
	static	shown;
	
	if (firstpass || !commonfile) {
		if (fwrite(str, sizeof(char), ln, fp1) != ln) {
			fprintf(stderr, "filemerge: fwrite failed on %s\n",
			    tmp1);
			exit(1);
		}
		if (gapind >= MAXGAP) {
			if (!shown)
				fprintf(stderr, "filemerge: too many gaps\n");
			shown = 1;
			return;
		}
		shown = 0;
		gaps[gapind++] = linecnt-1;
	}
}

puttext1rightpad(str, ln)
	char *str;
{
	if (firstpass) {
		if (fwrite(str, sizeof(char), ln, fp1) != ln) {
			fprintf(stderr, "filemerge: fwrite failed on %s\n",
			    tmp1);
			exit(1);
		}
		if (!readonly)
			if (fwrite(str, sizeof(char), ln, fp3) != ln) {
				fprintf(stderr,
				    "filemerge: fwrite failed on %s\n",
				    file3);
				exit(1);
			}
	}
}

puttext2(str, ln)
	char *str;
{
	if (!firstpass || !commonfile) {
		if (fwrite(str, sizeof(char), ln, fp2) != ln) {
			fprintf(stderr, "filemerge: fwrite failed on %s\n",
			    tmp2);
			exit(1);
		}
	}
}

/* ARGSUSED */
/* VARARGS */
Notify_value
quit_signal(clnt, sig)
	Notify_client clnt;
	int sig;
{
	if (debug)
		fprintf(stderr, "filemerge: signal is %d\n", sig);
	unlink(tmp1);
	unlink(tmp2);
	unlink(tmp3);
	unlink(tmp4);
	exit(0);
}

/*
 * Handle input events when over panel items.
 * Menu request gives menu of possible options.
 */
/* ARGSUSED */
static
cmdpanel_event(item, ie)
	Panel_item item;
	Event *ie;
{
	Menu_item mi;
	typedef int (*func)();
	func proc;

	if (event_id(ie) == MENU_BUT && event_is_down(ie)) {
		mi = (Menu_item)menu_show(
		    (Menu)panel_get(item, PANEL_CLIENT_DATA), panel,
		  	panel_window_event(panel, ie) , 0);
		if (mi != NULL) {
			event_set_shiftmask(ie, (int)menu_get(mi, MENU_VALUE));
			proc = (func)panel_get(item, PANEL_NOTIFY_PROC);
			(*proc)(item, ie);
		}
	} else
		panel_default_handle_event(item, ie);
}

error(s1, s2)
	char	*s1, *s2;
{
	Event	event;
	char buf[100];

	sprintf(buf, s1, s2);
	strcat(buf, "\nClick left mouse button to continue");
	fullscreen_prompt(buf, "Continue", "", &event,
	    window_get(frame, WIN_FD));
}

/* for debugging */
showmarks()
{
	int	i, pos, line;

	for (i = 0; i < gapind; i++)
		printf("%d ", gaps[i]);
	printf("\n");
	for (i = 0; i < markind; i++) {
		pos = textsw_find_mark(text3, marks[i]);
		line = index_to_line(text3, pos);
		printf("%d: %d %d\n", i, pos, line);
	}
	printf("\n");
	for (i = 0; i < glyphind; i++) {
		pos = glyphlist[i].g_lineno;
		printf("%d: (%c %c) %d\n", i, glyphlist[i].g_lchar,
		    glyphlist[i].g_rchar, pos);
	}
}

Textsw_mark
getmark(j)
{
	return(marks[glyphlist[j].g_lineno]);
}

getcurgrpno()
{
	return(curgrpno);
}

shifthistory()
{
	int	x;
	
	x = MAXHIST/2;
	bcopy(history+x, history, x);
	histind = x-1;
}

changemenu(autoadv)
{
	Menu	menu;
	
	menu = (Menu)panel_get(left_item, PANEL_CLIENT_DATA);
	menu_set(menu_get(menu, MENU_NTH_ITEM, 2),  MENU_STRING_ITEM,
	    autoadv ?
	    "Left & Not Next [Shift]" :
	    "Left & Next     [Shift]", SHIFTMASK, 0);
	menu = (Menu)panel_get(right_item, PANEL_CLIENT_DATA);
	menu_set(menu_get(menu, MENU_NTH_ITEM, 2),  MENU_STRING_ITEM,
	    autoadv ?
	    "Right & Not Next [Shift]" :
	    "Right & Next     [Shift]", SHIFTMASK, 0);
	menu = (Menu)panel_get(merge_item, PANEL_CLIENT_DATA);
	menu_set(menu_get(menu, MENU_NTH_ITEM, 2),  MENU_STRING_ITEM,
	    autoadv ?
	    "Merge & Not Next [Shift]" :
	    "Merge & Next     [Shift]", SHIFTMASK, 0);
}

/*
 * From mailtool: Get the line number within the header
 * file of the current selection.
 */
int
seln_to_lineno(buffer)
	Seln_request *buffer;
{
	unsigned *srcp;

	srcp = (unsigned *)(LINT_CAST(buffer->data));
	if (*srcp++ != (unsigned)SELN_REQ_FAKE_LEVEL)
		return (-1);
	srcp++;
	if (*srcp++ != (unsigned)SELN_REQ_FIRST_UNIT)
		return (-1);
	return ((int)*srcp);
}

usage()
{
	fprintf(stderr,
 "Usage: filemerge [-b] [-r] [-a file] [-l file] [file1 [file2 [outfile]]]\n");
	exit(1);
}
