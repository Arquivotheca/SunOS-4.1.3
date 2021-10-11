#ifndef lint
static	char sccsid[] = "@(#)boggle.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * main.c: main routine and general windows routines
 */

#include <suntool/tool_hs.h>
#include <suntool/menu.h>
#include <suntool/wmgr.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_ioctl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <stdio.h>

#include "defs.h"
#include "images.h"

static short icon_data[256] = {
#include <images/boggle.icon>
};

mpr_static(boggle_pr, 64, 64, 1, icon_data);
struct icon boggle_icon = {
	TOOL_ICONWIDTH, TOOL_ICONHEIGHT, 0,
	{0, 0, TOOL_ICONWIDTH, TOOL_ICONHEIGHT},
	&boggle_pr, {0, 0, 0, 0}, 0, 0, 0
};

struct timeval		flashdelay = {0, 10000};

static			bog_sigwinch(),
			bog_selected(),
			sigwinch_handler();

static char		tool_name[] = "boggletool";

#define DEFDICT	"/usr/games/boggledict"		/* dictionary */

char board[16];
int setupboard = 0;

char *cubeletters[16] = {	/* the boggle cubes */
	"forixb", "moqabj", "gurilw", "setupl",
	"cmpdae", "acitao", "slcrae", "romash",
	"nodesw", "hefiye", "onudtk", "tevign",
	"anedvz", "pinesh", "abilyt", "gkyleu"
};

#define BOG_RESTARTTIMER	(caddr_t)1	/* restart the timer */
#define BOG_EXIT		(caddr_t)2	/* quit bogtool */
#define BOG_RESTARTGAME		(caddr_t)3	/* restart the game */
#define BOG_GIVEUP		(caddr_t)4	/* human gives up */

struct menuitem bogmenu_items[] = {
	MENU_IMAGESTRING,	"Restart Game",		BOG_RESTARTGAME,
	MENU_IMAGESTRING,	"Restart Timer",	BOG_RESTARTTIMER,
	MENU_IMAGESTRING,	"Give Up",		BOG_GIVEUP,
	MENU_IMAGESTRING,	"Quit",			BOG_EXIT
};

struct menu	b_menu = {
	MENU_IMAGESTRING, "Boggle",
	sizeof(bogmenu_items) / sizeof(struct menuitem),
	bogmenu_items, NULL, NULL
};

struct menu *bog_menu = &b_menu;

boggletool_main(argc, argv)
int argc;
char **argv;
{
	char **tool_attrs = NULL, *getenv();
	int quit();
	long atol();
	extern sand_delay;

	if (tool_parse_all(&argc, argv, &tool_attrs, tool_name) == -1) {
		tool_usage(tool_name);
		exit(1);
	}

	dictionary = DEFDICT;

	while (--argc) {
		++argv;
		if (**argv == '+') {
			while (*(*argv)++ == '+')
				reuse++;
		} else if (isdigit(**argv)) {
			sand_delay = compute_delay(atol(*argv));
			if (sand_delay <= 0) {
				fprintf(stderr, "Playing time cannot be negative!\n");
				exit(1);
			}
		} else if (isalpha(**argv)) {
			if (strlen(*argv) != 16)
				usage();
			strncpy(board, *argv, 16);
			setupboard = 1;
		} else if (**argv == '-') {
			if (*(++(*argv)) == 'd') {
				if (*(++(*argv)) != '\0') {
					dictionary = *argv;
				} else if (argc > 1) {
					dictionary = *(++argv);
					argc--;
				} else {
					usage();
				}
			} else {
				usage();
			}
		} else {
			usage();
		}
	}

	tool = tool_make(WIN_NAME_STRIPE, 1,
			WIN_LABEL, tool_name,
			WIN_ATTR_LIST, tool_attrs,
			WIN_ICON, &boggle_icon, 0);
	if (tool == NULL) {
		fprintf(stderr, "Couldn't create tool!\n");
		exit(1);
	}
	tool_free_attribute_list(tool_attrs);

	state = INSTRUCTIONS;

	initboggle();

	bogsw = tool_createsubwindow(tool, "", -1, -1);
	initbogsw();

	signal(SIGWINCH, sigwinch_handler);
	signal(SIGINT, quit);
	signal(SIGQUIT, quit);
	signal(SIGTERM, quit);

	tool_install(tool);

	tool_select(tool, 0);

	signal(SIGWINCH, SIG_IGN);

	tool_destroy(tool);

	killchild();

	exit(0);
}

usage()
{
	fprintf(stderr, "Usage: bogtool [+[+]] [playing-time-in-minutes] [16-letter-string] -d dictionary\n");
	exit(1);
}

killchild()
{
	if (childpid > 0) {
		kill(childpid, SIGINT);
		while (wait(0) > 0)
			;
		childpid = 0;
		close(pfd[0]);
		close(pfd[1]);
	}
}

static
quit()
{
	tool_done(tool);
}

#define TOOL_ADDWIDTH	(2 * tool_borderwidth(tool))
#define TOOL_ADDHEIGHT	(tool_borderwidth(tool) + tool_stripeheight(tool))

static
sigwinch_handler()
{
	int miny;
	struct rect r;

	if ((tool->tl_flags & TOOL_ICONIC) == 0 && (win_getuserflags(tool->tl_windowfd) & WMGR_ICONIC) == 0) {
		miny = DISPLAY_BOTTOM;
		win_getrect(tool->tl_windowfd, &r);
		if (miny % fontheight != 0)
			miny += fontheight - (miny % fontheight);
		if (r.r_width < DISPLAY_RIGHT + TOOL_ADDWIDTH) {
			if (r.r_height < DISPLAY_BOTTOM + miny + fontheight + TOOL_ADDHEIGHT) {
				r.r_width = DISPLAY_RIGHT + BOARD_SPACING + 10 * fontwidth + TOOL_ADDWIDTH;
				r.r_height = DISPLAY_BOTTOM + BOARD_SPACING + TOOL_ADDHEIGHT;
				win_setrect(tool->tl_windowfd, &r);
				return;
			} else {
				r.r_width = DISPLAY_RIGHT + TOOL_ADDWIDTH;
				win_setrect(tool->tl_windowfd, &r);
				return;
			}
		} else if (r.r_height < DISPLAY_BOTTOM + TOOL_ADDHEIGHT) {
			if (r.r_width < DISPLAY_RIGHT + BOARD_SPACING + 10 * fontwidth + TOOL_ADDWIDTH) {
				r.r_width = DISPLAY_RIGHT + BOARD_SPACING + 10 * fontwidth + TOOL_ADDWIDTH;
				r.r_height = DISPLAY_BOTTOM + BOARD_SPACING + TOOL_ADDHEIGHT;
				win_setrect(tool->tl_windowfd, &r);
				return;
			} else {
				r.r_height = DISPLAY_BOTTOM + BOARD_SPACING + TOOL_ADDHEIGHT;
				win_setrect(tool->tl_windowfd, &r);
				return;
			}
		}
	}
	tool_sigwinch(tool);
}

initbogsw()
{
	struct inputmask im;
	register x, y, triwidth, triheight;

	bogwin = pw_open(bogsw->ts_windowfd);
	bogsw->ts_io.tio_handlesigwinch = bog_sigwinch;
	bogsw->ts_io.tio_selected = bog_selected;
	input_imnull(&im);
	win_setinputcodebit(&im, MS_LEFT);
	win_setinputcodebit(&im, MS_MIDDLE);
	win_setinputcodebit(&im, MS_RIGHT);
	win_setinputcodebit(&im, LOC_MOVEWHILEBUTDOWN);
	win_setinputcodebit(&im, LOC_WINEXIT);
	im.im_flags |= IM_ASCII;
	im.im_flags |= IM_NEGEVENT;
	win_setinputmask(bogsw->ts_windowfd, &im, 0, WIN_NULLLINK);
	bogwin->pw_prretained = NULL;
	if ((bogfont = pw_pfsysopen()) == NULL) {
		fprintf(stderr, "Couldn't open font!\n");
		exit(1);
	}
	tri_left_pr = mem_create(fontwidth, fontheight, 1);
	tri_right_pr = mem_create(fontwidth, fontheight, 1);
	triheight = make_even(0.625 * (double)fontheight);
	triwidth = triheight / 2;
	for (x = 0; x < triwidth; x++) {
		for (y = x; y <= triheight - x; y++) {
			pr_put(tri_right_pr, x + 1, y + (fontheight - triheight) / 2, 1);
			pr_put(tri_left_pr, triwidth - x, y + (fontheight - triheight) / 2, 1);
		}
	}
}

make_even(c)
double c;
{
	int lower;

	lower = (int) c;
	if (lower % 2 != 0)
		lower--;
	if (c - (double)lower <= (double)(lower + 2) - c) {
		return(lower);
	} else {
		return(lower + 2);
	}
}

startdisplay()
{
	freehumanwords();
	cwordlen = 0;
	scrolled = 0;
}

startboggle(draw)
int draw;
{
	genboard();
	startdisplay();
	starttimer();
	startwordfind();
	if (draw)
		drawbog();
}

static
bog_sigwinch()
{
	struct rect r;

	win_getsize(bogsw->ts_windowfd, &r);
	if (!rect_equal(&r, &bogrect)) {
		pw_damaged(bogwin);
		pw_donedamaged(bogwin);
		bogwidth = r.r_width;
		bogheight = r.r_height;
		if (bogwin->pw_prretained != NULL)
			pr_destroy(bogwin->pw_prretained);
		bogwin->pw_prretained = mem_create(bogwidth, bogheight, 1);
		switch (state) {
		case INSTRUCTIONS:
			askinstr();
			break;
		case CONFIRM_INSTR:
			printinstr();
			break;
		case PLAYING:
			drawbog();
			break;
		case GAMEOVER:
			pw_write(bogwin, 0, 0, bogwidth, bogheight, PIX_SRC, 0, 0, 0);
			drawboard();
			drawtimer();
			displaylists();
			break;
		}
	} else {
		pw_damaged(bogwin);
		pw_repairretained(bogwin);
		pw_donedamaged(bogwin);
	}
}

static
bog_selected(data, ibits, obits, ebits, timer)
caddr_t data;
int *ibits, *obits, *ebits;
struct timeval **timer;
{
	struct inputevent ie;

	if (*ibits & (1 << bogsw->ts_windowfd)) {
		if (input_readevent(bogsw->ts_windowfd, &ie) < 0) {
			fprintf(stderr, "input_readevent failed!\n");
			exit(1);
		}
		process_input(&ie);
	}
	if (childactive && state == PLAYING) {
		if (*ibits && (1 << fromchild)) {
			if (!readchild()) {
				close(pfd[0]);
				close(pfd[1]);
				childactive = 0;
			}
		}
	}
	if (state == PLAYING) {
		if (movesand()) {
			stoptimer = 1;
			timeisup(1);
		}
	}
	if (stoptimer) {
		timer = NULL;
		stoptimer = 0;
	}
	*ibits = *obits = *ebits = 0;
	if (setchildbits) {
		childactive++;
		setchildbits = 0;
	}
	if (childactive && state == PLAYING)
		*ibits |= (1 << fromchild);
	*ibits |= (1 << bogsw->ts_windowfd);
}

process_input(ie)
struct inputevent *ie;
{
	int x, y;
	char *word, *mouseonword();

	if (isascii(ie->ie_code)) {
		if (win_inputnegevent(ie))
			return;
		switch (state) {
		case INSTRUCTIONS:
			if (ie->ie_code == 'y' || ie->ie_code == 'Y' || ie->ie_code == '?') {
				printinstr();
				state = CONFIRM_INSTR;
			} else {
				state = PLAYING;
				startboggle(1);
			}
			break;
		case CONFIRM_INSTR:
			if (isupper(ie->ie_code)) {
				state = PLAYING;
				startboggle(1);
			}
			break;
		case PLAYING:
			if (feedback_w > 0) {	/* remove feedback from isduplicate() */
				pw_write(bogwin, feedback_x, feedback_y, feedback_w, fontheight, PIX_NOT(PIX_DST), NULL, 0, 0);
				feedback_w = 0;
			}
			if (isalpha(ie->ie_code))
				addchar(ie->ie_code);
			else if (ie->ie_code == '\n' || ie->ie_code == '\r' || ie->ie_code == '\t' || ie->ie_code == ' ')
				newline();
			else if (ie->ie_code == '\177' || ie->ie_code == '\010')
				deletechar();
			else if (ie->ie_code == '\025')
				deleteline();
			else
				flash(1);
			break;
		case GAMEOVER:
			if (isupper(ie->ie_code)) {
				state = PLAYING;
				startboggle(1);
			}
			break;
		}
	} else if (ie->ie_code == MS_RIGHT) {
		if (win_inputposevent(ie))
			domenu(ie);
	} else if (ie->ie_code == MS_LEFT || ie->ie_code == MS_MIDDLE || ie->ie_code == LOC_MOVEWHILEBUTDOWN) {
		if (laststr[0] != '\0') {
			word = mouseonword(ie->ie_locx, ie->ie_locy);
			if (word == NULL)
				unshowword();
			else if (strcmp(laststr, word))
				unshowword();
		}
		if (ie->ie_locx > DISPLAY_RIGHT || ie->ie_locy > DISPLAY_BOTTOM) {
			if (win_inputposevent(ie)) {
				word = mouseonword(ie->ie_locx, ie->ie_locy);
				if (word != NULL && strcmp(word, laststr)) {
					if (laststr[0] != '\0')
						unshowword();
					showcubes(word);
				}
			} else {
				unshowword();
			}
		}
	} else if (ie->ie_code == LOC_WINEXIT) {
		unshowword();
	}
}

domenu(ie)
struct inputevent *ie;
{
	struct menuitem *m_item;

	m_item = menu_display(&bog_menu, ie, bogsw->ts_windowfd);
	if (m_item != NULL) {
		switch (m_item->mi_data) {
		case BOG_RESTARTTIMER:
			if (state != PLAYING)
				break;
			starttimer();
			drawtimer();
			break;
		case BOG_EXIT:
			if (wmgr_confirm(bogsw->ts_windowfd, "Press the left mouse button to confirm Quit.  To cancel, press the right mouse button now."))
				tool_done(tool);
			break;
		case BOG_RESTARTGAME:
			state = PLAYING;
			startboggle(1);
			break;
		case BOG_GIVEUP:
			if (state != PLAYING)
				break;
			state = GAMEOVER;
			stoptimer = 1;
			timeisup(0);
		}
	}
}

drawbog()
{
	pw_write(bogwin, 0, 0, bogwidth, bogheight, PIX_SRC, 0, 0, 0);
	drawboard();
	drawtimer();
	drawtypein();
}

static
drawboard()
{
	char *b = board;
	int row, column, x, y;

	for (column = 0; column < 4; column++) {
		for (row = 0; row < 4; row++) {
			x = row * (TILE_WIDTH + BOARD_SPACING) + BOARD_BASELEFT;
			y = column * (TILE_WIDTH + BOARD_SPACING) + BOARD_BASETOP;
			pw_write(bogwin, x, y, TILE_WIDTH, TILE_WIDTH,
						PIX_SRC, &tile_pr, 0, 0);
			if (*b == 'q') {
				pw_write(bogwin, x + FIRST_LETTER_X_OFFSET,
					y + LETTER_Y_OFFSET, LETTER_WIDTH,
					LETTER_HEIGHT, PIX_SRC,
					letters['q'-'a'], 0, 0);
				pw_write(bogwin, x + SECOND_LETTER_X_OFFSET,
					y + LETTER_Y_OFFSET, LETTER_WIDTH,
					LETTER_HEIGHT, PIX_SRC, &small_u_pr,
									0, 0);
			} else {
				pw_write(bogwin, x + CENTER_LETTER_X_OFFSET,
					y + LETTER_Y_OFFSET, LETTER_WIDTH,
					LETTER_HEIGHT, PIX_SRC,
					letters[*b - 'a'], 0, 0);
			}
			b++;
		}
	}
}

/*
 * addchar: handle a typed character; make sure this part of word is in
 *          the grid
 */

addchar(c)
char c;
{
	cword[cwordlen] = c;
	cword[cwordlen + 1] = '\0';
	if (!checkword(cword, cwordlen + 1)) {	/* be sure word is in grid */
		flash(1);
		return;
	}
	drawcursor();	/* erase cursor */
	if ((!scrolled && cwordlen >= MAXWORDLEN) ||		/* scroll it */
			(scrolled && cwordlen - scrolled + 1 >= MAXWORDLEN)) {
		if (!scrolled)
			scrolled++;
		scrolled += MAXWORDLEN - SCROLL_OVERLAP;
		pw_write(bogwin, leftmargin, typein_y, fontwidth * MAXWORDLEN, fontheight, PIX_SRC, NULL, 0, 0);
		pw_write(bogwin, leftmargin, typein_y, fontwidth, fontheight, PIX_SRC, tri_left_pr, 0, 0);
		pw_text(bogwin, leftmargin + fontwidth, typein_y + fontoffset, PIX_SRC, bogfont, &cword[scrolled]);
		typein_x = leftmargin + (cwordlen - scrolled) * fontwidth + fontwidth;
	}
	pw_char(bogwin, typein_x, typein_y + fontoffset, PIX_SRC, bogfont, c);
	typein_x += fontwidth;
	drawcursor();	/* replace cursor */
	cwordlen++;
}

/*
 * newline: handle a typed newline; make sure word is in dictionary
 *          and hasn't already been entered; insert into list; move
 *          cursor on display
 */

newline()
{
	if (cwordlen == 0)
		return;
	if (cwordlen < 3) {
		flash(1);
		return;
	}
	cword[cwordlen] = '\0';
	if (isduplicate(cword)) {	/* already entered this word */
		flash(1);
		return;
	}
	if (!verify(cword)) {	/* isn't in the dictionary */
		flash(1);
		return;
	}
	addhumanword(cword, leftmargin, typein_y);
	cwordlen = 0;
	drawcursor();	/* erase cursor */
	if (scrolled) {	/* unscroll word if it is scrolled */
		char buf[MAXWORDLEN];

		strncpy(buf, cword, MAXWORDLEN - 1);
		buf[MAXWORDLEN - 1] = '\0';
		pw_text(bogwin, leftmargin, typein_y + fontoffset, PIX_SRC, bogfont, buf);
		pw_write(bogwin, leftmargin + (MAXWORDLEN - 1) * fontwidth,
			typein_y, fontwidth, fontheight, PIX_SRC, tri_right_pr, 0, 0);
		scrolled = 0;
	}
	(void) nextscreenpos(&leftmargin, &typein_x, &typein_y);	/* move cursor to new line */
	removefeedback(typein_x, typein_y);	/* remove feedback address from word which was here (if wrapped) */
	pw_write(bogwin, typein_x, typein_y, MAXWORDLEN * fontwidth,	/* erase any previous word */
					fontheight, PIX_SRC, NULL, 0, 0);
	drawcursor();	/* replace cursor */
}

/*
 * deletechar: handle a backspace from user
 */

deletechar()
{
	if (cwordlen > 0) {
		drawcursor();	/* erase cursor */
		if (scrolled && cwordlen == scrolled + 2) {	/* unscroll */
			if (cwordlen <= MAXWORDLEN) {	/* word will still be scrolled */
				scrolled = 0;
				pw_write(bogwin, leftmargin, typein_y, fontwidth * MAXWORDLEN, fontheight, PIX_SRC, NULL, 0, 0);
				cword[--cwordlen] = '\0';
				pw_text(bogwin, leftmargin, typein_y + fontoffset, PIX_SRC, bogfont, cword);
				typein_x = leftmargin + cwordlen * fontwidth;
			} else {	/* word no longer scrolled */
				scrolled -= MAXWORDLEN - SCROLL_OVERLAP;
				pw_write(bogwin, leftmargin, typein_y, fontwidth * MAXWORDLEN, fontheight, PIX_SRC, NULL, 0, 0);
				pw_write(bogwin, leftmargin, typein_y, fontwidth, fontheight, PIX_SRC, tri_left_pr, 0, 0);
				cword[--cwordlen] = '\0';
				pw_text(bogwin, leftmargin + fontwidth, typein_y + fontoffset, PIX_SRC, bogfont, &cword[scrolled]);
				typein_x = leftmargin + (cwordlen - scrolled) * fontwidth + fontwidth;
			}
		} else {	/* handle a non-scrolled word */
			typein_x -= fontwidth;
			pw_write(bogwin, typein_x, typein_y, fontwidth, fontheight, PIX_SRC, NULL, 0, 0);
			cwordlen--;
		}
		drawcursor();	/* redraw cursor */
	}
}

/*
 * deleteline: handle a line kill from user
 */

deleteline()
{
	if (cwordlen > 0) {
		drawcursor();	/* erase cursor */
		typein_x = leftmargin;
		pw_write(bogwin, typein_x, typein_y, fontwidth * cwordlen,
					fontheight, PIX_SRC, NULL, 0, 0);
		drawcursor();	/* redraw cursor */
		cwordlen = 0;
		scrolled = 0;
	}
}

genboard()
{
	int i, j;

	if (setupboard)
		return;
	for (i = 0; i < 16; i++)
		board[i] = 0;
	for (i = 0; i < 16; i++) {
		j = random() % 16;
		while (board[j] != 0)
			j = (j + 1) % 16;
		board[j] = cubeletters[i][random() % 6];
	}
}

/*
 * nextscreenpos: determine where on the screen the next word goes
 */

nextscreenpos(lm, x, y)
int *lm, *x, *y; /* current left margin, x, and y; gets changed to new value */
{
	int full = 0;

	if (*x < 0 || *y < 0) {	/* display is blank; move to first position */
		*y = DISPLAY_BOTTOM;
		if (*y % fontheight != 0)
			*y += fontheight - (*y % fontheight);
		if (*y + fontheight > bogheight) {
			*y = 0;
			*lm = *x = DISPLAY_RIGHT + BOARD_SPACING;
		} else {
			*lm = *x = 0;
		}
		return(0);
	}
	*x = *lm;
	*y += fontheight;
	if (*y + fontheight > bogheight) {
		*lm = *x += fontwidth * (MAXWORDLEN + 1);
		if (*lm + 10 * fontwidth > bogwidth) {
			*y = DISPLAY_BOTTOM;	/* wrap to beginning of screen */
			if (*y % fontheight != 0)
				*y += fontheight - (*y % fontheight);
			if (*y + fontheight > bogheight) {
				*lm = *x = DISPLAY_RIGHT + BOARD_SPACING;
				*y = 0;
			} else {
				*lm = *x = 0;
			}
			full = 1;
		} else if (*x > DISPLAY_RIGHT + BOARD_SPACING) {
			*y = 0;
		} else {
			*y = DISPLAY_BOTTOM;
			if (*y % fontheight != 0)
				*y += fontheight - (*y % fontheight);
		}
	}
	return(full);
}

askinstr()
{
	pw_write(bogwin, 0, 0, bogwidth, bogheight, PIX_SRC, 0, 0, 0);
	pw_text(bogwin, 0, fontoffset, PIX_SRC, bogfont, "Would you like instructions? ");
	pw_write(bogwin, 29 * fontwidth, 0, fontwidth, fontheight, PIX_NOT(PIX_DST), NULL, 0, 0);
}

#define INSTRLEN	15
char *instr[INSTRLEN] = {
	"     The object of Boggle  (TM Parker Bros.)  is to find as many words as",
	"possible in a 4 by 4 grid of letters within a certain time limit.   Words",
	"may be formed from any sequence of 3 or more adjacent letters in the grid.",
	"The letters may join horizontally, vertically, or diagonally.    Normally,",
	"no letter in the grid may be used more than once in a word.  If boggletool",
	"is invoked with a numeric argument,  it is taken to be the time limit  (in",
	"minutes).    A single  +  on the command line removes the restriction that",
	"letters may be used only once, and two  +  signs allow a letter to be con-",
	"sidered adjacent to itself as well as its neighbors.",
	"     Boggletool displays a random grid of letters and an hourglass to mark",
	"the remaining time.   Enter words separated by newlines, using the DEL key",
	"or the line-kill character  (^U)  to correct mistakes.   When your time is",
	"up,  the computer will display  a list  of words  it found from the online",
	"dictionary.",
	"     Type any capital letter to begin."
};

printinstr()
{
	int y;

	pw_write(bogwin, 0, 0, bogwidth, bogheight, PIX_SRC, 0, 0, 0);
	for (y = 0; y < INSTRLEN; y++)
		pw_text(bogwin, 0, y * fontheight + fontoffset, PIX_SRC, bogfont, instr[y]);
}

timeisup(c)
int c;
{
	if (c)
		flash(3);
	state = GAMEOVER;
	comparelists();
	feedback_w = 0;
	displaylists();
}

flash(times)
int times;
{
	while (times--) {
		pw_write(bogwin, 0, 0, bogwidth, bogheight, PIX_NOT(PIX_DST), 0, 0, 0);
		select(0, 0, 0, 0, &flashdelay);
		pw_write(bogwin, 0, 0, bogwidth, bogheight, PIX_NOT(PIX_DST), 0, 0, 0);
		if (times)
			select(0, 0, 0, 0, &flashdelay);
	}
}
