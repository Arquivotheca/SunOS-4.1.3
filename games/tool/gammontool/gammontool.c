#ifndef lint
static	char sccsid[] = "@(#)gammontool.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <suntool/tool_hs.h>
#include <suntool/wmgr.h>
#include <suntool/panel.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_ioctl.h>
#include <sys/stat.h>
#include <ctype.h>
#include "defs.h"
static short icon_data[256]={
#include <images/gammon.icon>
};
#include "cursors.h"

#define MINBRDHT	(22 * msgfont->pf_defaultsize.y)
#define OPTHT		(2 * (msgfont->pf_defaultsize.y + 10))
#define MSGHT		(2 * msgfont->pf_defaultsize.y)

mpr_static(gammon_pr, 64, 64, 1, icon_data);
struct icon gammon_icon = {
	TOOL_ICONWIDTH, TOOL_ICONHEIGHT, 0,
	{0, 0, TOOL_ICONWIDTH, TOOL_ICONHEIGHT},
	&gammon_pr, {0, 0, 0, 0}, 0, 0, 0
};

static struct tool	*tool;
struct toolsw	*optsw, *msgsw, *brdsw;
struct pixwin	*msgwin, *brdwin;
struct pixfont	*msgfont;
struct rect	msgrect, brdrect;

static	tool_sigwinchhandler(), quit(), gammonpipeready(), msg_sigwinch(),
	(*cached_toolselected)(), brd_sigwinch(), brd_selected();

static int double_proc(), accept_proc(), refuse_proc(), showmove_proc(),
	   redo_proc(), redoall_proc(), forfeit_proc(), quit_proc(),
	   newgame_proc(), newcolor_proc();

Panel_item panelitems[10];

char *cached_msg[2] = {"", ""};

#define NUMPOINTS	4

int humanpieces[NUMPOINTS][2] = { {6, 5}, {8, 3}, {13, 5}, {24, 2} };
int computerpieces[NUMPOINTS][2] = { {1, 2}, {12, 5}, {17, 3}, {19, 5} };

int minwidth, minheight;

static char tool_name[] = "gammontool";

gammontool_main(argc, argv)
int argc;
char **argv;
{
	char **tool_attrs = NULL;

	if (tool_parse_all(&argc, argv, &tool_attrs, tool_name) == -1) {
		tool_usage(tool_name);
		exit(1);
	}

	execbkg = 0;
	while (--argc) {
		++argv;
		if (execbkg)
			fprintf(stderr, "Only one binary allowed!\n");
		else {
			gammonbin = *argv;
			execbkg++;
		}
	}

	msgfont = pw_pfsysopen();

	tool = tool_make(WIN_NAME_STRIPE, 1,
			WIN_LABEL, tool_name,
			WIN_ATTR_LIST,	tool_attrs,
			WIN_ICON, &gammon_icon,
			0, 0);
	if (tool == NULL) {
		fprintf(stderr, "Couldn't create tool\n");
		exit(1);
	}
	tool_free_attribute_list(tool_attrs);

	optsw = panel_create(tool, PANEL_HEIGHT, OPTHT, 0);
	initopt();

	msgsw = tool_createsubwindow(tool, "messages", -1, MSGHT);
	initmsg();
	message(MSG, "To begin the game, roll the dice by clicking the left or middle buttons.");
	message(ERR, "(The mouse cursor may be anywhere over the board when rolling the dice.)");

	brdsw = tool_createsubwindow(tool, "board", -1, -1);
	initbrd();

	initmatch();

	signal(SIGWINCH, tool_sigwinchhandler);
	signal(SIGINT, quit);
	signal(SIGQUIT, quit);
	signal(SIGTERM, quit);	

	tool_install(tool);

	cached_toolselected = tool->tl_io.tio_selected;
	tool->tl_io.tio_selected = gammonpipeready;
	FD_SET(tool->tl_windowfd, &tool->tl_io.tio_inputmask);
	startgammon();
	FD_SET(gammonfd, &tool->tl_io.tio_inputmask);

	tool_select(tool, 0);

	signal(SIGWINCH, SIG_IGN);
	signal(SIGINT, SIG_IGN);

	tool_destroy(tool);

	if (logfp != NULL)
		fprintf(logfp, "Match over.\n");

	killpg(getpgrp(), SIGINT);

	exit(0);
}

initopt()
{
	Panel psw = (Panel)optsw->ts_data;
	int n = 0, x, y;

	x = y = 4;
	panelitems[n++] = panel_create_item(psw, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE, panel_button_image(psw, "Double", 6, NULL),
	    PANEL_ITEM_X, x,
	    PANEL_ITEM_Y, y,
	    PANEL_NOTIFY_PROC, double_proc, 0);
	x += 6 * msgfont->pf_defaultsize.x + 21;
	panelitems[n++] = panel_create_item(psw, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE, panel_button_image(psw, "Accept Double", 13, NULL),
	    PANEL_ITEM_X, x,
	    PANEL_ITEM_Y, y,
	    PANEL_NOTIFY_PROC, accept_proc, 0);
	x += 13 * msgfont->pf_defaultsize.x + 21;
	panelitems[n++] = panel_create_item(psw, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE, panel_button_image(psw, "Refuse Double", 13, NULL),
	    PANEL_ITEM_X, x,
	    PANEL_ITEM_Y, y,
	    PANEL_NOTIFY_PROC, refuse_proc, 0);
	x += 13 * msgfont->pf_defaultsize.x + 21;
	panelitems[n++] = panel_create_item(psw, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE, panel_button_image(psw, "Show Last Move", 14, NULL),
	    PANEL_ITEM_X, x,
	    PANEL_ITEM_Y, y,
	    PANEL_NOTIFY_PROC, showmove_proc, 0);
	x += 14 * msgfont->pf_defaultsize.x + 21;
	panelitems[n++] = panel_create_item(psw, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE, panel_button_image(psw, "Redo Move", 9, NULL),
	    PANEL_ITEM_X, x,
	    PANEL_ITEM_Y, y,
	    PANEL_NOTIFY_PROC, redo_proc, 0);
	x = 4;
	y += msgfont->pf_defaultsize.y + 8;
	panelitems[n++] = panel_create_item(psw, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE, panel_button_image(psw, "Redo Entire Move", 16, NULL),
	    PANEL_ITEM_X, x,
	    PANEL_ITEM_Y, y,
	    PANEL_NOTIFY_PROC, redoall_proc, 0);
	x += 16 * msgfont->pf_defaultsize.x + 21;
	panelitems[n++] = panel_create_item(psw, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE, panel_button_image(psw, "Forfeit", 7, NULL),
	    PANEL_ITEM_X, x,
	    PANEL_ITEM_Y, y,
	    PANEL_NOTIFY_PROC, forfeit_proc, 0);
	x += 7 * msgfont->pf_defaultsize.x + 21;
	panelitems[n++] = panel_create_item(psw, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE, panel_button_image(psw, "Quit", 4, NULL),
	    PANEL_ITEM_X, x,
	    PANEL_ITEM_Y, y,
	    PANEL_NOTIFY_PROC, quit_proc, 0);
	x += 4 * msgfont->pf_defaultsize.x + 21;
	panelitems[n++] = panel_create_item(psw, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE, panel_button_image(psw, "New Game", 8, NULL),
	    PANEL_ITEM_X, x,
	    PANEL_ITEM_Y, y,
	    PANEL_NOTIFY_PROC, newgame_proc, 0);
	x += 8 * msgfont->pf_defaultsize.x + 21;
	panelitems[n++] = panel_create_item(psw, PANEL_CHOICE,
	    PANEL_LABEL_STRING, "Color:",
	    PANEL_ITEM_X, x,
	    PANEL_ITEM_Y, y,
	    PANEL_CHOICE_STRINGS, "White", "Black", 0,
	    PANEL_NOTIFY_PROC, newcolor_proc, 0);
}

initmsg()
{
	msgwin = pw_open(msgsw->ts_windowfd);
	msgsw->ts_io.tio_handlesigwinch = msg_sigwinch;
}

initbrd()
{
	struct inputmask im;

	brdwin = pw_open(brdsw->ts_windowfd);
	brdsw->ts_io.tio_handlesigwinch = brd_sigwinch;
	brdsw->ts_io.tio_selected = brd_selected;
	input_imnull(&im);
	win_setinputcodebit(&im, MS_LEFT);
	win_setinputcodebit(&im, MS_MIDDLE);
	win_setinputcodebit(&im, LOC_MOVEWHILEBUTDOWN);
	win_setinputcodebit(&im, LOC_WINEXIT);
	im.im_flags = IM_NEGEVENT;
	win_setinputmask(brdsw->ts_windowfd, &im, 0, WIN_NULLLINK);
}

static
initmatch()
{
	struct stat buf;
	char logfile[512];

	srandom(time(0));
	getscore(humanname, &humanscore, &computerscore);
	win_getcursor(brdsw->ts_windowfd, &cursors[ORIGINAL_CUR]);
	humancolor = NONE;
	state = STARTGAME;
	humancolor = WHITE;	/* for drawing purposes these must be set */
	initpieces();		/* they will be updated when game starts */
	numhdice = numcdice = 0;
	lastroll[0][0] = lastroll[1][0] = 0;
	lastdoubled = NOBODY;
	lastmoved = NOBODY;
	alreadyrolled = 0;
	minwidth = 2 * tool_borderwidth(tool) + 72 * msgfont->pf_defaultsize.x;
	minheight = OPTHT + MSGHT + MINBRDHT + 2 * tool_subwindowspacing(tool);

	/* open the log file only if it is already there */

	sprintf(logfile, "%s/gammonlog", getenv("HOME"));
	if (stat(logfile, &buf) == 0) {
		if ((logfp = fopen(logfile, "a")) != NULL) {
			setbuf(logfp, NULL);
			if (buf.st_size > 0)
				putc('\n', logfp);
			fprintf(logfp, "New match.\n");
		}
	}
}

static
tool_sigwinchhandler()
{
	int toosmall = 0;
	struct rect r;

	win_getrect(tool->tl_windowfd, &r);
	if (r.r_width < minwidth) {
		r.r_width = minwidth;
		toosmall++;
	}
	if (r.r_height < minheight) {
		r.r_height = minheight;
		toosmall++;
	}
	if (toosmall && (tool->tl_flags & TOOL_ICONIC) == 0 &&
		(win_getuserflags(tool->tl_windowfd) & WMGR_ICONIC) == 0)
			win_setrect(tool->tl_windowfd, &r);
	else
		tool_sigwinch(tool);
}

static
msg_sigwinch()
{
	win_getrect(msgsw->ts_windowfd, &msgrect);
	pw_damaged(msgwin);
	redisplay_message();
	pw_donedamaged(msgwin);
}

static
brd_sigwinch()
{
	struct rect r;

	win_getsize(brdsw->ts_windowfd, &r);
	if (!rect_equal(&r, &brdrect)) {

		/*
		 * the following call to pw_damaged is necessary on color
		 * monitors (to initialize bit planes); the pw_donedamaged
		 * call discards the clip list so the whole board is redrawn
		 */

		pw_damaged(brdwin);
		pw_donedamaged(brdwin);
		brdrect.r_left = r.r_left;
		brdrect.r_top = r.r_top;
		brdrect.r_width = r.r_width;
		brdrect.r_height = r.r_height;
		changeboardsize();
		redisplay();
	} else {
		pw_damaged(brdwin);
		redisplay();
		pw_donedamaged(brdwin);
	}
}

static
quit()
{
	tool_done(tool);
}

static
gammonpipeready(data, ibits, obits, ebits, timer)
caddr_t data;
int *ibits, *obits, *ebits;
struct timeval **timer;
{
	if (*ibits & (1 << gammonfd)) {
		readfromgammon();
		*ibits &= ~(1 << gammonfd);
	}
	cached_toolselected(data, ibits, obits, ebits, timer);
	*ibits |= (1 << tool->tl_windowfd);
	*ibits |= (1 << gammonfd);
}

static
brd_selected(data, ibits, obits, ebits, timer)
caddr_t data;
int *ibits, *obits, *ebits;
struct timeval **timer;
{
	struct inputevent ie;

	input_readevent(brdsw->ts_windowfd, &ie);
	gammon_reader(&ie);
	*ibits = *obits = *ebits = 0;
}

static
double_proc(item, event)
	Panel_item item;
	struct intputevent *event;
{
	message(ERR, "");
	switch (state) {
	case STARTGAME:
		message(ERR, "The game hasn't started yet!");
		break;
	case COMPUTERDOUBLING:
		message(ERR, "You must accept or refuse the computer's double first!");
		break;
	case HUMANDOUBLING:
		message(ERR, "Hold on, the computer is thinking about whether to accept your double!");
		break;
	case ROLL:
		if (lastdoubled == HUMAN) {
			message(ERR, "You don't have the cube!");
		} else if (gamevalue >= 64) {
			message(ERR, "You can't double any higher!");
		} else {
			sendtogammon(DOUBLEREQ);
			state = HUMANDOUBLING;
			diddouble = 1;
			if (logfp != NULL)
				fprintf(logfp, "Human doubled.\n");
		}
		break;
	case MOVE:
		message(ERR, "You can only double before you roll the dice!");
		break;
	case THINKING:
		message(ERR, "It's not your turn!");
		break;
	case GAMEOVER:
		message(ERR, "The game is over!");
		break;
	}
}

static
accept_proc(item, event)
	Panel_item item;
	struct intputevent *event;
{
	message(ERR, "");
	switch (state) {
	case STARTGAME:
		message(ERR, "The game hasn't started yet!");
		break;
	case COMPUTERDOUBLING:
		gamevalue *= 2;
		lastdoubled = COMPUTER;
		drawcube();
		sendtogammon(ACCEPT_DBLE);
		message(MSG, "Thinking...");
		startcomputerturn();
		break;
	case HUMANDOUBLING:
		message(ERR, "Hold on, the computer is thinking about whether to accept your double!");
		break;
	case ROLL:
		message(ERR, "The computer hasn't doubled you!");
		break;
	case MOVE:
		message(ERR, "The computer hasn't doubled you!");
		break;
	case THINKING:
		message(ERR, "It's not your turn!");
		break;
	case GAMEOVER:
		message(ERR, "The game is over!");
		break;
	}
}

static
refuse_proc(item, event)
	Panel_item item;
	struct intputevent *event;
{
	message(ERR, "");
	switch (state) {
	case STARTGAME:
		message(ERR, "The game hasn't started yet!");
		break;
	case COMPUTERDOUBLING:
		win(COMPUTER, REFUSEDDBLE);
		break;
	case HUMANDOUBLING:
		message(ERR, "Hold on, the computer is thinking about whether to accept your double!");
		break;
	case ROLL:
		message(ERR, "The computer hasn't doubled you!");
		break;
	case MOVE:
		message(ERR, "The computer hasn't doubled you!");
		break;
	case THINKING:
		message(ERR, "It's not your turn!");
		break;
	case GAMEOVER:
		message(ERR, "The game is over!");
		break;
	}
}

static
showmove_proc(item, event)
	Panel_item item;
	struct intputevent *event;
{
	message(ERR, "");
	if (state == STARTGAME) {
		message(ERR, "The game hasn't started yet!");
	} else {
		if (lastmoved == HUMAN && movesmade > 0)
			showlasthumanmove();
		else
			showlastcomputermove();
	}
}

static
redo_proc(item, event)
	Panel_item item;
	struct intputevent *event;
{
	message(ERR, "");
	switch (state) {
	case STARTGAME:
		message(ERR, "The game hasn't started yet!");
		break;
	case COMPUTERDOUBLING:
		message(ERR, "You must accept or refuse the computer's double!");
		break;
	case HUMANDOUBLING:
		message(ERR, "Hold on, the computer is thinking about whether to accept your double!");
		break;
	case ROLL:
		message(ERR, "You haven't moved yet!");
		break;
	case MOVE:
		if (movesmade <= 0)
			message(ERR, "You haven't moved yet!");
		else
			undoonemove();
		break;
	case THINKING:
		message(ERR, "It's not your turn!");
		break;
	case GAMEOVER:
		message(ERR, "The game is over!");
		break;
	}
}

static
redoall_proc(item, event)
	Panel_item item;
	struct intputevent *event;
{
	message(ERR, "");
	switch (state) {
	case STARTGAME:
		message(ERR, "The game hasn't started yet!");
		break;
	case COMPUTERDOUBLING:
		message(ERR, "You must accept or refuse the computer's double!");
		break;
	case HUMANDOUBLING:
		message(ERR, "Hold on, the computer is thinking about whether to accept your double!");
		break;
	case ROLL:
		message(ERR, "You haven't moved yet!");
		break;
	case MOVE:
		if (movesmade <= 0)
			message(ERR, "You haven't moved yet!");
		else
			undowholemove();
		break;
	case THINKING:
		message(ERR, "It's not your turn!");
		break;
	case GAMEOVER:
		message(ERR, "The game is over!");
		break;
	}
}

static
forfeit_proc(item, event)
	Panel_item item;
	struct intputevent *event;
{
	message(ERR, "");
	if (state == STARTGAME) {
		message(ERR, "The game hasn't started yet!");
	} else if (state == GAMEOVER) {
		message(ERR, "The game is over!");
	} else if (state == COMPUTERDOUBLING) {
		win(COMPUTER, REFUSEDDBLE);
	} else {
		message(ERR, "Please confirm that you want to forfeit this game with the left button.");
		if (!cursor_confirm(brdsw->ts_windowfd)) {
			message(ERR, "");
			return;
		}
		message(ERR, "");
		win(COMPUTER, RESIGNED);
	}
}

static
quit_proc(item, event)
	Panel_item item;
	struct intputevent *event;
{
	message(ERR, "");
	if (state != GAMEOVER && state != STARTGAME) {
		message(ERR, "If you want to quit the game you must forfeit first.");
		return;
	}
	quit();
}

static
newgame_proc(item, event)
	Panel_item item;
	struct intputevent *event;
{
	message(ERR, "");
	if (state == STARTGAME) {	
		message(MSG, "To begin the game, roll the dice by clicking the left or middle buttons.");
		message(ERR, "(The mouse cursor may be anywhere over the board when rolling the dice.)");
	} else if (state != GAMEOVER) {
		message(ERR, "If you want to restart the game, you must forfeit this one first.");
	} else {
		startgammon();
		replacepieces();
		initpieces();
		numhdice = numcdice = 0;
		drawdice(BOTH, NOFLASH);
		if (gamevalue != 1) {
			gamevalue = 1;
			clearcube();
		}
		lastdoubled = NOBODY;
		drawcube();
		state = STARTGAME;
		message(MSG, "To begin the game, roll the dice by clicking the left or middle buttons.");
		message(ERR, "(The mouse cursor may be anywhere over the board when rolling the dice.)");
	}
}

static
newcolor_proc(item, value, event)
	Panel_item item;
	struct inputevent *event;
{
	int newcolor;

	message(ERR, "");
	if (panel_get_value(item) == 0)
		newcolor = WHITE;
	else
		newcolor = BLACK;
	if (newcolor != humancolor) {
		humancolor = newcolor;
		drawpieces();
	}
}

message(kind, str)
int kind;
char *str;
{
	int height = kind * msgfont->pf_defaultsize.y;

	pw_writebackground(msgwin, 0, height, msgrect.r_width,
					msgfont->pf_defaultsize.y, PIX_SRC);
	pw_text(msgwin, 0, height - msgfont->pf_char['n'].pc_home.y,
						PIX_SRC, msgfont, str);
	cached_msg[kind] = str;
}

redisplay_message()
{
	int c, height;

	pw_writebackground(msgwin, 0, 0, msgrect.r_width, msgrect.r_height,
								PIX_SRC);
	for (c=0, height=0; c < 2; c++, height += msgfont->pf_defaultsize.y)
		pw_text(msgwin, 0, height - msgfont->pf_char['n'].pc_home.y,
					PIX_SRC, msgfont, cached_msg[c]);
}

setcursor(which)
int which;
{
	win_setcursor(brdsw->ts_windowfd, &cursors[which]);
}

initgame()
{
	if (logfp != NULL)
		fprintf(logfp, "New game.\n");
	initpieces();
	gamevalue = 1;
	lastdoubled = NOBODY;
	diddouble = 0;
	rolldice(BOTH);		/* initial roll; doubles not allowed */
	while (humandieroll[0] == computerdieroll[0])
		rolldice(BOTH);
	if (humandieroll[0] > computerdieroll[0]) {
		sendtogammon(HUMANFIRST);
		message(MSG, "You go first.");
		dicedisplayed = HUMAN;
		starthumanturn();
	} else {
		sendtogammon(COMPUTERFIRST);
		alreadyrolled = 1;
		message(MSG, "The computer goes first.  Thinking...");
		state = THINKING;
		dicedisplayed = COMPUTER;
		setcursor(THINKING_CUR);
	}
}

initpieces()
{
	int index;

	for (index = 0; index < 26; index++)
		humanboard[index] = computerboard[index] = 0;
	for (index = 0; index < NUMPOINTS; index++) {
		humanboard[humanpieces[index][0]] = humanpieces[index][1];
		computerboard[computerpieces[index][0]] = computerpieces[index][1];
	}
}

rolldice(who)
int who;
{
	int d0, d1;

	/* save dice currently displayed for show-last-move */

	lastroll[0][0] = numhdice;
	lastroll[0][1] = humandieroll[0];
	lastroll[0][2] = humandieroll[1];
	lastroll[1][0] = numcdice;
	lastroll[1][1] = computerdieroll[0];
	lastroll[1][2] = computerdieroll[1];

	/* generate numbers for new dice */

	d0 = random() % 6 + 1;
	d1 = random() % 6 + 1;

	/* assign numbers depending on whose turn it is */

	switch (who) {
	case HUMAN:
		humandieroll[0] = d0;
		humandieroll[1] = d1;
		numhdice = 2;
		numcdice = 0;
		dicedisplayed = HUMAN;
		drawdice(COMPUTER, NOFLASH);	/* erase computer's dice */
		break;
	case COMPUTER:
		computerdieroll[0] = d0;
		computerdieroll[1] = d1;
		numcdice = 2;
		numhdice = 0;
		dicedisplayed = COMPUTER;
		drawdice(HUMAN, NOFLASH);	/* erase human's dice */
		break;
	case BOTH:			/* special case for first roll */
		humandieroll[0] = d0;
		humandieroll[1] = d1;
		computerdieroll[0] = d1;
		computerdieroll[1] = d0;
		numhdice = numcdice = 1;
		break;
	}
	drawdice(who, FLASH);
}

win(who, how)
int who, how;
{
	int points = gamevalue;

	if (who == HUMAN) {
		switch (how) {
		case STRAIGHT:
			message(MSG, "You beat the computer!");
			if (logfp != NULL)
				fprintf(logfp, "Human won.\n");
			break;
		case GAMMON:
			message(MSG, "You gammoned the computer!");
			points *= 2;
			if (logfp != NULL)
				fprintf(logfp, "Human gammoned computer.\n");
			break;
		case BACKGAMMON:
			message(MSG, "You backgammoned the computer!!!");
			points *= 3;
			if (logfp != NULL)
				fprintf(logfp, "Human backgammoned computer.\n");
			break;
		case REFUSEDDBLE:
			message(MSG, "The computer refused your double!");
			if (logfp != NULL)
				fprintf(logfp, "Computer refused double.\n");
			break;
		case RESIGNED:
			message(MSG, "The computer resigned!");
			points *= 3;
			if (logfp != NULL)
				fprintf(logfp, "Computer resigned\n");
			break;
		}
		humanscore += points;
		drawscore(HUMAN);
	} else {
		switch (how) {
		case STRAIGHT:
			message(MSG, "The computer beat you!");
			if (logfp != NULL)
				fprintf(logfp, "Computer won.\n");
			break;
		case GAMMON:
			message(MSG, "The computer gammoned you!");
			points *= 2;
			if (logfp != NULL)
				fprintf(logfp, "Computer gammoned human.\n");
			break;
		case BACKGAMMON:
			message(MSG, "The computer backgammoned you!!!");
			points *= 3;
			if (logfp != NULL)
				fprintf(logfp, "Computer backgammoned human.\n");
			break;
		case REFUSEDDBLE:
			message(MSG, "The computer wins!");
			if (logfp != NULL)
				fprintf(logfp, "Human refused double.\n");
			break;
		case RESIGNED:
			message(MSG, "The computer takes triple the stakes!");
			points *= 3;
			if (logfp != NULL)
				fprintf(logfp, "Human resigned\n");
			break;
		}
		computerscore += points;
		drawscore(COMPUTER);
	}
	state = GAMEOVER;
	setcursor(ORIGINAL_CUR);
	savescore(humanscore, computerscore);
	if (logfp != NULL)
		fprintf(logfp, "Score: %d (human), %d (computer)\n", humanscore, computerscore);
}

movepiece(from, to, who)
int from, to, who;
{
	int color, fromnum, tonum;

	if (who == HUMAN) {
		fromnum = humanboard[from]--;
		tonum = humanboard[to]++;
		color = humancolor;
	} else {
		fromnum = computerboard[from]--;
		tonum = computerboard[to]++;
		color = computercolor;
	}
	erasepiece(from, color, fromnum);
	drawpiece(to, color, tonum);
}

/*
 * novel type of redisplay when restarting game: moves each piece back
 * to where it goes rather than erasing and redisplaying the entire board
 */

replacepieces()
{
	int c, p, pmax, pc, moves[30][5], movcntr = 0, pnt;

	for (c = 0; c < 26; c++) {
		pc = pointcount(c, humancolor);
		if (humanboard[c] > 0 && humanboard[c] > pc) {
			pmax = humanboard[c] - pc;
			for (p = 0; p < pmax; p++) {
				pnt = findapoint(humancolor);
				moves[movcntr][0] = c;
				moves[movcntr][1] = pnt;
				moves[movcntr][2] = humanboard[c]--;
				moves[movcntr][3] = humanboard[pnt]++;
				moves[movcntr][4] = humancolor;
				movcntr++;
			}
		}
		pc = pointcount(c, computercolor);
		if (computerboard[c] > 0 && computerboard[c] > pc) {
			pmax = computerboard[c] - pc;
			for (p = 0; p < pmax; p++) {
				pnt = findapoint(computercolor);
				moves[movcntr][0] = c;
				moves[movcntr][1] = pnt;
				moves[movcntr][2] = computerboard[c]--;
				moves[movcntr][3] = computerboard[pnt]++;
				moves[movcntr][4] = computercolor;
				movcntr++;
			}
		}
	}

	/*
	 * we can't use movepiece() because if a white piece on point 1
	 * must trade with a black piece on point 24, it can't be done
	 * unless both pieces are removed first
	 */

	for (c = 0; c < movcntr; c++)
		erasepiece(moves[c][0], moves[c][4], moves[c][2]);
	for (c = 0; c < movcntr; c++)
		drawpiece(moves[c][1], moves[c][4], moves[c][3]);
}

pointcount(point, color)
int point, color;
{
	int index;

	for (index = 0; index < NUMPOINTS; index++) {
		if (color == humancolor) {
			if (humanpieces[index][0] == point)
				return(humanpieces[index][1]);
		} else {
			if (computerpieces[index][0] == point)
				return(computerpieces[index][1]);
		}
	}
	return(0);
}

findapoint(color)
int color;
{
	int index;

	for (index = 0; index < NUMPOINTS; index++) {
		if (humancolor == color) {
			if (humanboard[humanpieces[index][0]] < humanpieces[index][1])
				return(humanpieces[index][0]);
		} else {
			if (computerboard[computerpieces[index][0]] < computerpieces[index][1])
				return(computerpieces[index][0]);
		}
	}
}

/* display the die rolls which were last on the screen */
/* a second call to showlastdice redisplays the original dice */

showlastdice()
{
	int currentroll[2][3];

	/* save current roll */

	currentroll[0][0] = numhdice;
	currentroll[0][1] = humandieroll[0];
	currentroll[0][2] = humandieroll[1];
	currentroll[1][0] = numcdice;
	currentroll[1][1] = computerdieroll[0];
	currentroll[1][2] = computerdieroll[1];

	/* insert last roll */

	numhdice = lastroll[0][0];
	humandieroll[0] = lastroll[0][1];
	humandieroll[1] = lastroll[0][2];
	numcdice = lastroll[1][0];
	computerdieroll[0] = lastroll[1][1];
	computerdieroll[1] = lastroll[1][2];

	/* set up current roll so it can be redisplayed later */

	lastroll[0][0] = currentroll[0][0];
	lastroll[0][1] = currentroll[0][1];
	lastroll[0][2] = currentroll[0][2];
	lastroll[1][0] = currentroll[1][0];
	lastroll[1][1] = currentroll[1][1];
	lastroll[1][2] = currentroll[1][2];

	/* display last roll */

	drawdice(BOTH, NOFLASH);
}
