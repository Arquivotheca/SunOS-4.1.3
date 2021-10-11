#ifndef lint
static  char sccsid[] = "@(#)popup.c 1.1 92/08/05 Copyr 1986 Sun Micro";
#endif

#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/text.h>
#include <suntool/scrollbar.h>
#include <pwd.h>
#include <ctype.h>
#include "diff.h"

#define RCFILE ".filemergerc"

#define BOTTOMLINES_DEFAULT 14
#define TOPLINES_DEFAULT 15
#define COLUMNS_DEFAULT 34

#define TOPLINES 0
#define BOTTOMLINES 1
#define COLUMNS 2

#define NITEMS 3
static	int	newsizes[NITEMS]; /* = {-1, -1, ...} */
static	Panel_item	size_item[NITEMS];
int	sizes[NITEMS] =
	{TOPLINES_DEFAULT, BOTTOMLINES_DEFAULT, COLUMNS_DEFAULT};

int	autoadvance_toggle(), moveundo_toggle();

static	Frame	popup;
static	Panel	popuppanel;
static	int	textht, resizeit;
static	topmargin, leftmargin;
static	applying = 1;

static	int	newautoadvance = -1;
static	int	newmoveundo = -1;

Panel_item	autoadvance_item;
Panel_item	moveundo_item;
Panel_setting	size_text();

/*
 * these variables are here to handle the case when the size
 * is forced with -W flags
 */
static	int	bottomlines = BOTTOMLINES_DEFAULT;
static	int	toplines = TOPLINES_DEFAULT;
static	int	columns = COLUMNS_DEFAULT;

#define ROWPAD 10		/* extra space between rows in panel */

popitup()
{
	
	if (popuppanel == NULL) {
		popup = window_create(frame, FRAME,
		    WIN_SHOW, 0,
		    FRAME_SHOW_LABEL, 1,
		    FRAME_LABEL, "filemerge Properties",
		    FRAME_DONE_PROC, done_button, 0);
		popuppanel = window_create(popup, PANEL, 0);
		makepopuppanel();
	}
	window_set(popup, WIN_SHOW, TRUE, 0);
}

/* ARGSUSED */
/* VARARGS */
/* also called from apply_button, reset_button */
done_button(item, event)
	Panel_item item;
	struct inputevent *event;
{
	if (confirm_reset())
		window_set(popup, WIN_SHOW, 0, 0);
}

/* ARGSUSED */
apply_button(item, event)
	Panel_item item;
	struct inputevent *event;
{
	int	change = 0, sizechange = 0;
	int	j, wd, totwd, ht, totht, pht, oldht, oldwd;
	
	if (newautoadvance != -1) {
		change++;
		autoadvance = newautoadvance;
		changemenu(autoadvance);
		newautoadvance = -1;
	}
	if (newmoveundo != -1) {
		change++;
		moveundo = newmoveundo;
		newmoveundo = -1;
	}
	for (j = 0; j < NITEMS; j++) {
		if (newsizes[j] != -1) {
			newsizes[j] = -1;
			sizechange++;
			sizes[j] = atoi(panel_get_value(size_item[j]));
		}
	}
	if (sizechange) {
		oldht = (int)window_get(frame, WIN_HEIGHT);
		oldwd = (int)window_get(frame, WIN_WIDTH);
		pht = (int)window_get(panel, WIN_HEIGHT);
		bottomlines = sizes[BOTTOMLINES];
		toplines = sizes[TOPLINES];
		columns = sizes[COLUMNS];
		computesizes(&wd, &totwd, &ht, &totht);
		resizeit = 1;
		applying = 1;
		window_set(frame,
			WIN_WIDTH, totwd,
			WIN_HEIGHT, totht + pht, 0);
		/* check to see if only relatize hts changed */
		adjustposition();
		if (oldht == totht + pht && oldwd == totwd) {
			event_set_id(event, WIN_REPAINT);
			frame_event(0, event);
		}
	}
	if (event_ctrl_is_down(event))
		done_button();
	if (change || sizechange)
		writerc();
}

/* ARGSUSED */
reset_button(item, event)
	Panel_item item;
	struct inputevent *event;
{
	int	j;
	char	buf[20];
	
	if (newautoadvance != -1) {
		panel_set_value(autoadvance_item, autoadvance);
		newautoadvance = -1;
	}
	if (newmoveundo != -1) {
		panel_set_value(moveundo_item, moveundo);
		newmoveundo = -1;
	}
	for (j = 0; j < NITEMS; j++) {
		if (newsizes[j] != -1) {
			sprintf(buf, "%d", sizes[j]);
			panel_set_value(size_item[j], buf);
			newsizes[j] = -1;
		}
	}
	if (event_ctrl_is_down(event))
		done_button();
}

makepopuppanel()
{
	Panel_item	item;

	item = panel_create_item(popuppanel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, panel_button_image(popuppanel,
		    "Apply", 0, 0),
		PANEL_NOTIFY_PROC, apply_button, 0);
	setupmenu(item, "Apply", "Apply & Done [Ctrl]", 2, "", 0, cmdpanel_event);
	item = panel_create_item(popuppanel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, panel_button_image(popuppanel,
		    "Reset", 0, 0),
		PANEL_NOTIFY_PROC, reset_button, 0);
	setupmenu(item, "Reset", "Reset & Done [Ctrl]", 2, "", 0, cmdpanel_event);
	panel_create_item(popuppanel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, panel_button_image(popuppanel,
		    "Done", 0, 0),
		PANEL_NOTIFY_PROC, done_button, 0);

	autoadvance_item = panel_create_item(popuppanel, PANEL_TOGGLE,
		/* check marks are 5 pixels too high */
		PANEL_ITEM_X, ATTR_COL(0),
		PANEL_ITEM_Y, ATTR_ROW(1) + 1*ROWPAD - 5,
		PANEL_VALUE, autoadvance,
		PANEL_CHOICE_STRINGS, "autoadvance", 0,
		PANEL_NOTIFY_PROC, autoadvance_toggle, 0);

	moveundo_item = panel_create_item(popuppanel, PANEL_TOGGLE,
		/* check marks are 5 pixels too high */
		PANEL_ITEM_Y, ATTR_ROW(1) + 1*ROWPAD - 5,
		PANEL_VALUE, moveundo,
		PANEL_CHOICE_STRINGS, "move on undo", 0,
		PANEL_NOTIFY_PROC, moveundo_toggle, 0);

	makeitem(TOPLINES, "Toplines:", toplines);
	makeitem(BOTTOMLINES, "Bottomlines", bottomlines);
	makeitem(COLUMNS, "Columns:", columns);

	window_fit(popuppanel);
	window_fit(popup);
}

makeitem(i, str, val)
	char	*str;
{
	char		buf[5];
	static		first = 1;

	sprintf(buf, "%d", val);
	if (first) {
		first = 0;
		size_item[i] = panel_create_item(popuppanel, PANEL_TEXT,
			PANEL_ITEM_X, ATTR_COL(0),
			PANEL_ITEM_Y, ATTR_ROW(2) + 2*ROWPAD,
			PANEL_LABEL_STRING, str,
			PANEL_VALUE, buf,
			PANEL_CLIENT_DATA, i,
			PANEL_NOTIFY_LEVEL, PANEL_ALL,
			PANEL_VALUE_DISPLAY_LENGTH, 4,
			PANEL_NOTIFY_PROC, size_text, 0);
	}
	else
		size_item[i] = panel_create_item(popuppanel, PANEL_TEXT,
			PANEL_ITEM_Y, ATTR_ROW(2) + 2*ROWPAD,
			PANEL_LABEL_STRING, str,
			PANEL_VALUE, buf,
			PANEL_CLIENT_DATA, i,
			PANEL_NOTIFY_LEVEL, PANEL_ALL,
			PANEL_VALUE_DISPLAY_LENGTH, 4,
			PANEL_NOTIFY_PROC, size_text, 0);
}

setupmenu(item, str1, str2, v2, str3, v3, func)
	Panel_item	item;
	char	*str1, *str2, *str3;
	int	(*func)();
{
	Menu menu;
	static int menuval[4] = {0,SHIFTMASK,CTRLMASK,SHIFTMASK|CTRLMASK};

	menu = menu_create(MENU_CLIENT_DATA, item,
	    MENU_NOTIFY_PROC, menu_return_item, 
	    /* MENU_LEFT_MARGIN, 10, */
	    0);
	menu_set(menu, MENU_STRING_ITEM, str1, menuval[0], 0);
	if (str2[0])
		menu_set(menu, MENU_STRING_ITEM, str2, menuval[v2], 0);
	if (str3[0])
		menu_set(menu, MENU_STRING_ITEM, str3, menuval[v3], 0);
	panel_set(item,
	    PANEL_EVENT_PROC, func,
	    PANEL_CLIENT_DATA, menu,
	    0);
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
		    (Menu)panel_get(item, PANEL_CLIENT_DATA), popuppanel,
		  	panel_window_event(popuppanel, ie) , 0);
		if (mi != NULL) {
			event_set_shiftmask(ie, (int)menu_get(mi, MENU_VALUE));
			proc = (func)panel_get(item, PANEL_NOTIFY_PROC);
			(*proc)(item, ie);
		}
	} else
		panel_default_handle_event(item, ie);
}

/* ARGSUSED */
autoadvance_toggle(item, value, event)
	Panel_item item;
	struct inputevent *event;
{
	newautoadvance = value;
}

/* ARGSUSED */
moveundo_toggle(item, value, event)
	Panel_item item;
	struct inputevent *event;
{
	newmoveundo = value;
}

/* 
 * return true if really want to reset
 */
confirm_reset()
{
	Event	event;
	Panel_item dummyitem;
	int	i, allmaps;
	
	allmaps = 1;
	for (i = 0; i < NITEMS; i++)
		allmaps = allmaps && (newsizes[i] == -1);
	if (newautoadvance == -1 && newmoveundo == -1 && allmaps)
		return (1);
	event_set_shiftmask(&event, 0);
	if (expert) {
		reset_button(dummyitem, &event);
		return (1);
	}
	fullscreen_prompt("\
You have made changes that haven't been\n\
applied. They will be lost if you close\n\
the property sheet.  To confirm closing\n\
the property sheet press the left mouse\n\
button. To cancel press the right mouse\n\
button now.", "Really Close", "Don't Close", &event, window_get(frame,WIN_FD));
	if (event_id(&event) == MS_LEFT) {
		reset_button(dummyitem, &event);
		return (1);
	} else
		return (0);
}

writerc()
{
	struct	passwd *pw;
	char	buf[100];
	FILE	*fp;

	pw = getpwuid(getuid());
	if (pw == NULL) {
		fprintf(stderr,"filemerge: can't find home directory\n");
		exit(1);
	}
	strcpy(buf, pw->pw_dir);
	strcat(buf, "/");
	strcat(buf, RCFILE);
	fp = fopen(buf, "w");
	if (fp == NULL) {
		fprintf(stderr, "filemerge: can't create %s file\n", RCFILE);
		return;
	}
	if (autoadvance)
		fprintf(fp, "set %sautoadvance\n", autoadvance ? "" : "no");
	if (moveundo)
		fprintf(fp, "set %smoveundo\n", moveundo ? "" : "no");
	if (sizes[BOTTOMLINES] != BOTTOMLINES_DEFAULT)
		fprintf(fp, "set bottomlines=%d\n", sizes[BOTTOMLINES]);
	if (sizes[TOPLINES] != TOPLINES_DEFAULT)
		fprintf(fp, "set toplines=%d\n", sizes[TOPLINES]);
	if (sizes[COLUMNS] != COLUMNS_DEFAULT)
		fprintf(fp, "set columns=%d\n", sizes[COLUMNS]);
	fclose(fp);
}

readrc()
{
	struct	passwd *pw;
	char	buf[100];
	char	line[200];		/* one line */
	char	*p, *q;
	int	n, no, k;
	FILE	*fp;

	/* weird place to put this, but ... */
	for (n = 0; n < NITEMS; n++)
		newsizes[n] = -1;
	pw = getpwuid(getuid());
	if (pw == NULL) {
		fprintf(stderr,"filemerge: can't find home directory\n");
		exit(1);
	}
	strcpy(buf, pw->pw_dir);
	strcat(buf, "/");
	strcat(buf, RCFILE);
	fp = fopen(buf, "r");
	if (fp == NULL)
		return;
	while (fgets(line, sizeof(line), fp)) {
		if (line[0] == '#' || line[0] == '\n')
			continue;
		if (strncmp(line, "set", 3) != 0) {
			fprintf(stderr, "filemerge: Can't parse\n");
			fprintf(stderr, "\t%s", line);
			continue;
		}
		p = line+3;
		while (isspace(*p))
			p++;
		q = p;
		while(isalpha(*p))
			p++;
		n = p - q;
		if (strncmp(q, "no", 2) == 0) {
			q +=2;
			n -=2;
			no = 1;
		} else
			no = 0;
		if (strncmp(q, "autoadvance", n) == 0)
			autoadvance = !no;
		else if (strncmp(q, "moveundo", n) == 0)
			moveundo = !no;
		else {
			if ((p = (char *)index(q, '=')) == NULL || no) {
				fprintf(stderr, "filemerge: Can't parse\n");
				fprintf(stderr, "\t%s", line);
				continue;
			}
			k = atoi(p+1);
			if (strncmp(q, "toplines", n) == 0)
				sizes[TOPLINES] = k;
			else if (strncmp(q, "bottomlines", n) == 0)
				sizes[BOTTOMLINES] = k;
			else if (strncmp(q, "columns", n) == 0)
				sizes[COLUMNS] = k;
			else {
				fprintf(stderr, "filemerge: Can't parse\n");
				fprintf(stderr, "\t%s", line);
				continue;
			}
		}
	}
	fclose(fp);
	toplines = sizes[TOPLINES];
	bottomlines = sizes[BOTTOMLINES];
	columns = sizes[COLUMNS];
}

/* ARGSUSED */
Panel_setting
size_text(item, event)
	Panel_item item;
	Event	*event;
{
	int	i;
	
	i = (int)panel_get(item, PANEL_CLIENT_DATA);
	newsizes[i] = 1;
	if (event_id(event) == '\r')
		return(PANEL_NEXT);
	else if (isascii(event_id(event)) && isprint(event_id(event)))
		return (PANEL_INSERT);
	else
		return (PANEL_NONE);
}

computesizes(wdp, totwdp, htp, tothtp)
	int	*wdp;
	int	*totwdp;
	int	*htp;
	int	*tothtp;
{
	/* 4 is TEXTSW_LEFT_MARGIN */	*wdp = columns*charwd + TEXTMARGIN + 4
		+ (int)scrollbar_get(SCROLLBAR, SCROLL_THICKNESS);
	*htp = toplines*charht;
	*totwdp = *wdp * 2 + leftmargin;
	*tothtp = *htp + (readonly ? 0 : bottomlines*charht)
	   + topmargin;
	if (readonly)
		textht = -1;
	else
		textht = *htp;
}

/* ARGSUSED */
frame_event(fr, event)
	Frame	fr;
	Event	*event;
{
	static	oldwd, oldht;
	int	wd, ht;
	
	if (event_id(event) == WIN_REPAINT) {
		wd = (int)window_get(frame, WIN_WIDTH) - 15;
		ht = (int)window_get(frame, WIN_HEIGHT);
		if (wd != oldwd)
			relayoutpanel();
		if (wd != oldwd || ht != oldht || resizeit) {
			if (!applying && oldht != ht)
				textht =
				   ((int)window_get(frame, WIN_HEIGHT)
				    - topmargin
				    - (int)window_get(panel, WIN_HEIGHT))/2;
			window_set(text1, WIN_WIDTH, wd/2,
			    WIN_HEIGHT, readonly ? -1 : textht, 0);
			window_set(text2, WIN_RIGHT_OF, text1,
			    WIN_HEIGHT, readonly ? -1 : textht, 0);
			if (text3)
				window_set(text3, WIN_BELOW, text1,
				    WIN_X, 0, 0);
			oldwd = wd;
			oldht = ht;
			resizeit = 0;
			applying = 0;
		}
	}
}

parseargs(argc, argv)
	char **argv;
{
	int	i, wd, ht;
	
	leftmargin = 15;
	topmargin = 15 + charht + 4;	/* 4 is fudge factor */
	argv++;
	argc--;
	while (argc > 0) {
		if (argv[0][0] != '-') {
			if (file1[0] == 0)
				file1 = argv[0];
			else if (file2[0] == 0)
				file2 = argv[0];
			else if (file3[0] == 0)
				file3 = argv[0];
			else
				usage();
		} else
		switch(argv[0][1]) {
		case 'a':	/* ancestor */
			if (argc > 1)
				commonfile = argv[1];
			else
				commonfile = "";
			argv++;
			argc--;
			break;
		case 'd':
			debug++;
			break;
		case 'r':
			readonly++;
			break;
		case 'b':
			blanks = 1;
			break;
		case 'l':
			if (argc < 2)
				usage();
			list = argv[1];
			if (list[0] == '-')
				listfp = stdin;
			else
				listfp = fopen(list, "r");
			if (listfp == NULL) {
				fprintf(stderr, "filemerge: can't open %s\n",
				    list);
			}
			argv++;
			argc--;
			break;
		case 'W':
			if (argv[0][2] == 's') {
				if (argc < 3)
					usage();
				/* 
				 * totwd = 2*(cols*charwd + TEXTMARGIN
				 *          + 4 + SCROLLTHICKNESS) + leftmargin
				 */

				/* 
				 * totht = toplines*charht + topmargin
				 *         +  (readonly ? bottom*charht)
				 */
				wd = atoi(argv[1]);
				ht = atoi(argv[2]);
				columns =
				    ((wd - leftmargin)/2 - 4 - TEXTMARGIN -
				    (int)scrollbar_get(SCROLLBAR, SCROLL_THICKNESS))
				    / charwd;
				/* -3 to allow for panel */
				i = (ht - topmargin)/charht - 3;
				if (readonly)
					toplines = i;
				else {
					if (toplines > i - 2)
						toplines = i - 2;
					else
						toplines = toplines;
					bottomlines = i - toplines;
				}
				argv += 2;
				argc -= 2;
				break;
			} else if (argv[0][2] == 'h') {
				if (argc < 2)
					usage();
				ht = atoi(argv[1]);
				/* -3 to allow for panel */
				i = (ht*charht + charht + 4 - topmargin)/charht - 3;
				if (readonly)
					toplines = i;
				else {
					if (toplines > i - 2)
						toplines = i - 2;
					bottomlines = i
						    - toplines;
				}
				argv++;
				argc--;
				break;
			}
			else if (argv[0][2] == 'w') {
				if (argc < 2)
					usage();
				wd = atoi(argv[1]);
				columns =
				    ((wd*charwd + 10 - leftmargin)/2 - 4 - TEXTMARGIN -
				    (int)scrollbar_get(SCROLLBAR, SCROLL_THICKNESS))
				    / charwd;
				argv++;
				argc--;
				break;			
			}
			/* fall thru */
		default:
			if ((i = tool_parse_one(argc, argv,
			    &tool_attrs, "filemerge")) == -1) {
				tool_usage("filemerge");
				exit(1);
			} else if (i == 0)
				usage();
			argv += i;
			argc -= i;
			continue;
		}
		argc--;
		argv++;
	}
	if (list && (!isdir(file1) || !isdir(file2))) {
		fprintf(stderr,
 "filemerge: can't use -l flag unless both arguments are directories\n");
		exit(1);
	}
	newsizes[TOPLINES] = -1 + (sizes[TOPLINES] - toplines);
	newsizes[BOTTOMLINES] = -1 + (sizes[BOTTOMLINES] - bottomlines);
	newsizes[COLUMNS] = -1 + (sizes[COLUMNS] - columns);
}
