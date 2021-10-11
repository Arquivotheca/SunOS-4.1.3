#ifndef lint
static  char sccsid[] = "@(#)chesstool.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems Inc.
 */

/*
 * chesstool: allows you to play out a game of chess
 */

#define CHESS "/usr/games/chess"

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <stdio.h>
#include "pieces.h"
#include "chesstool.h"

#define BIG	2048
short piecearr[8][8];
static short init_piecearr[8][8] = {
	ROOK,   PAWN, 0, 0, 0, 0, PAWN, ROOK,
	KNIGHT, PAWN, 0, 0, 0, 0, PAWN, KNIGHT,
	BISHOP, PAWN, 0, 0, 0, 0, PAWN, BISHOP,
	QUEEN,  PAWN, 0, 0, 0, 0, PAWN, QUEEN,
	KING,   PAWN, 0, 0, 0, 0, PAWN, KING,
	BISHOP, PAWN, 0, 0, 0, 0, PAWN, BISHOP,
	KNIGHT, PAWN, 0, 0, 0, 0, PAWN, KNIGHT,
	ROOK,   PAWN, 0, 0, 0, 0, PAWN, ROOK
};

short colorarr[8][8];
static short init_colorarr[8][8] = {
	WHITE, WHITE, 0, 0, 0, 0, BLACK, BLACK,
	WHITE, WHITE, 0, 0, 0, 0, BLACK, BLACK,
	WHITE, WHITE, 0, 0, 0, 0, BLACK, BLACK,
	WHITE, WHITE, 0, 0, 0, 0, BLACK, BLACK,
	WHITE, WHITE, 0, 0, 0, 0, BLACK, BLACK,
	WHITE, WHITE, 0, 0, 0, 0, BLACK, BLACK,
	WHITE, WHITE, 0, 0, 0, 0, BLACK, BLACK,
	WHITE, WHITE, 0, 0, 0, 0, BLACK, BLACK
};

short wcapture[16], bcapture[16];
int wcapcnt, bcapcnt;
static int capturepos[15] = {
	0,              1*SQUARESIZE,     2*SQUARESIZE,   3*SQUARESIZE,
	4*SQUARESIZE,   5*SQUARESIZE,     6*SQUARESIZE,   
	SQUARESIZE/2,   3*SQUARESIZE/2,   5*SQUARESIZE/2, 7*SQUARESIZE/2,
	9*SQUARESIZE/2, 11*SQUARESIZE/2,  1*SQUARESIZE/4, 3*SQUARESIZE/4
};
int movelist[500];
int movecnt;

static struct pixrect *piece_pr[NUMPIECES] = {
	(struct pixrect *)0, &piecepawn, &pieceknight, &piecebishop,
	&piecerook, &piecequeen, &pieceking
};
static struct pixrect *outline_pr[NUMPIECES] = {
	(struct pixrect *)0, &outlinepawn, &outlineknight, &outlinebishop,
	&outlinerook, &outlinequeen, &outlineking
};

static short    wait_image[] = {
#include <images/hglass.cursor>
};
DEFINE_CURSOR_FROM_IMAGE(hourglasscursor, 7, 7, PIX_SRC | PIX_DST, wait_image);
DEFINE_CURSOR(oldcursor1, 0, 0, 0,
	      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
DEFINE_CURSOR(oldcursor2, 0, 0, 0,
	      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
DEFINE_CURSOR(oldcursor3, 0, 0, 0,
	      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

/*
 * general tool area
 */

static short icon_image[] ={
#include <images/chesstool.icon>
};
DEFINE_ICON_FROM_IMAGE(chesstool_icon, icon_image);

static char tool_name[] = "chesstool";
Frame	frame;

static	int	client;
struct itimerval period;
int	paint_chessboard();
int	chess_event();
static Notify_value chesspipeready();
Notify_value	clock_itimer();
static int pipefd;
int flashon;
int rotate;
int undoqueue;
static  int killkids();

int zflag;

int thinking = 0;
int done = 0;
Panel		panel;
Panel_item	msg_item;
Canvas	chessboard;
struct pixwin *chessboard_pixwin;
int highwater;
int human_proc(), last_proc(), machine_proc(), flash_proc(),
	undo_proc(), forward_proc(), backward_proc();
static int quit_proc();
char **av;

#ifdef STANDALONE
main(argc, argv)
#else
chesstool_main(argc, argv)
#endif
	char **argv;
{
	Panel	msg_panel;
	struct rect rectnormal, recticon;
	int iconic;
	static char *buf[3];
	
	frame = window_create(NULL, FRAME,
		WIN_WIDTH, BOARDSIZE + 10,
		FRAME_ARGC_PTR_ARGV, &argc, argv,
		FRAME_ICON, &chesstool_icon,
		FRAME_LABEL, tool_name, 0);
	if (frame == 0) {
		fprintf(stderr, "chesstool: couldn't create window\n");
		exit(1);
	}

	if (argc > 1 && argv[1][0] == '-') {
		zflag = 1;
		argc--;
		argv++;
	}
	if (argc == 1) {
		av = buf;
		av[1] = CHESS;
		av[2] = NULL;
	}
	else
		av = argv;

	msg_panel = window_create(frame, PANEL, 0);
	msg_item = panel_create_item(msg_panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, "Move pieces with left button", 0);
	window_fit_height(msg_panel);

	panel = window_create(frame, PANEL, 0);

	init_panel();
	chessboard = window_create(frame, CANVAS,
		WIN_HEIGHT, BOARDSIZE + 2*SQUARESIZE,
		CANVAS_REPAINT_PROC, paint_chessboard,
		WIN_EVENT_PROC, chess_event, 0);
	chessboard_pixwin = (Pixwin *)window_get(chessboard, CANVAS_PIXWIN);
	window_fit_height(frame);
	
	init_clock();
	resetboard();
	paint_chessboard();
	signal(SIGTERM, killkids);

	pipefd = startchess(av);
	update_clock();

	period.it_interval.tv_sec = 1;
	period.it_interval.tv_usec = 0;
	period.it_value.tv_sec = 1;
	period.it_value.tv_usec = 0;
	notify_set_itimer_func(&client, clock_itimer, ITIMER_REAL,
	    &period, 0);
	notify_set_input_func(&client, chesspipeready, pipefd);

	window_main_loop(frame);
	
	killkids();
	exit(0);
}

static
init_panel()
{
	panel_create_item(panel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, panel_button_image(panel,
		    "Last Play", 0, 0),
		PANEL_NOTIFY_PROC, last_proc, 0);
	panel_create_item(panel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, panel_button_image(panel,
		    "Undo", 0, 0),
		PANEL_NOTIFY_PROC, undo_proc, 0);
	panel_create_item(panel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, panel_button_image(panel,
		    "Machine White", 0, 0),
		PANEL_NOTIFY_PROC, machine_proc, 0);
	panel_create_item(panel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, panel_button_image(panel,
		    "Human White", 0, 0),
		PANEL_NOTIFY_PROC, human_proc, 0);
	panel_create_item(panel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, panel_button_image(panel,
		    "Quit", 0, 0),
		PANEL_NOTIFY_PROC, quit_proc, 0);
	panel_create_item(panel, PANEL_TOGGLE,
		PANEL_CHOICE_STRINGS, "Flash", 0,
		PANEL_NOTIFY_PROC, flash_proc, 0);
	if (zflag) {
		panel_create_item(panel, PANEL_BUTTON,
			PANEL_ITEM_X, PANEL_CU(0),
			PANEL_ITEM_Y, PANEL_CU(1) + 8,
			PANEL_LABEL_IMAGE, panel_button_image(panel,
			    "Forward", 0, 0),
			PANEL_NOTIFY_PROC, forward_proc, 0);
		panel_create_item(panel, PANEL_BUTTON,
			PANEL_ITEM_Y, PANEL_CU(1) + 8,
			PANEL_LABEL_IMAGE, panel_button_image(panel,
			    "Backward", 0, 0),
			PANEL_NOTIFY_PROC, backward_proc, 0);
	}
	window_fit_height(panel);
}

forward_proc()
{
	int mv, i, j, k, l;
	
	if (movecnt >= highwater) {
		putmsg( "Can't go any forward from here!");
		return;
	}
	if (thinking) {
		putmsg(
		    "Can't go forwards during computer's move");
		return;
	}
	else {
		mv = movelist[movecnt];
		unpack(mv, &i, &j, &k, &l);
		make_move(i, j, k, l);
	}
}

backward_proc()
{
	if (movecnt >= highwater)
		highwater = movecnt;
	if (movecnt <= 0) {
		putmsg( "Can't go back any further!");
		return;
	}
	if (thinking) {
		putmsg(
		    "Can't go backwards during computer's move");
		return;
	}
	undo(movecnt-1);
	movecnt--;
}


static
undo_proc(item, event)
	Panel_item item;
	struct inputevent *event;
{
	if (movecnt <= 0) {
		putmsg( "No move to undo!");
		return;
	}
	if (thinking) {
		putmsg(
		    "Will undo as soon as machine finishes move ...");
		undoqueue = 1;
	}
	else
		sendundo();
	highwater = movecnt-1;
}

static
machine_proc(item, event)
	Panel_item item;
	struct inputevent *event;
{
	if (!done && movecnt > 0) {
		putmsg("Do you really want to start a new game?");
		fullscreen_prompt("\
Game is still in progress.\n\
Do you really want to restart?\n\
Press left mouse button to\n\
confirm Restarting and exit\n\
current game.   To cancel,\n\
press the right mouse button.", "Really Restart", "Cancel Restart", event,
		    window_get(frame, WIN_FD));
		if (event_id(event) != MS_LEFT)
			return;
	}
	rotate = 1;
	reset();
	first();
}

static
human_proc(item, event)
	Panel_item item;
	struct inputevent *event;
{
	if (!done && movecnt > 0) {
		putmsg(
		    "Do you really want to start a new game?");

		fullscreen_prompt("\
Game is still in progress.\n\
Do you really want to restart?\n\
Press left mouse button to\n\
confirm Restarting and exit\n\
current game.   To cancel,\n\
press the right mouse button.", "Really Restart", "Cancel Restart", event,
	    window_get(frame, WIN_FD));

		if (event_id(event) != MS_LEFT)
			return;
	}
	rotate = 0;
	reset();
}

static
quit_proc(item, event)
	Panel_item item;
	struct inputevent *event;
{
	putmsg( "Please confirm exit with the left button.");

		fullscreen_prompt("\
Game is still in progress.\n\
Do you really want to quit?\n\
Press left mouse button to\n\
exit.   To cancel, press\n\
the right mouse button.", "Really Quit", "Cancel Quit", event,
	    window_get(frame, WIN_FD));

	if (event_id(event) != MS_LEFT)
		return;
	exit(0);
	/* killkids? */
}

reset() {
	highwater = 0;
	resetboard();
	paint_chessboard();
	restartchess();
	putmsg( "Move pieces with left button");
}

static
flash_proc(item, event)
	Panel_item item;
	struct inputevent *event;
{
	flashon ^= 1;
}

restartchess()
{
	closepipes();
	pipefd = startchess(av);
}

resetboard()
{
	int i,j;
	
	for(i = 0; i < 8; i++)
	for(j = 0; j < 8; j++) {
		piecearr[i][j] = init_piecearr[i][j];
		colorarr[i][j] = init_colorarr[i][j];
	}
	movecnt = wcapcnt = bcapcnt = 0;
	thinking = 0;
	done = 0;
	startclock();
	unhourglass();
}

static
chess_event(canvas, ie)
	Canvas	canvas;
	struct inputevent *ie;
{
	int row, col, x, y;
	static up = 1;
	static oldx, oldy;
	static int oldrow, oldcol, oldpiece;
	static startx, starty;

	if (thinking && ie->ie_code == MS_LEFT) {
		putmsg( "It's not your turn!");
		return;
	}
	else if (done) {
		putmsg( "Game is Over!");
		return;
	}
	if (ie->ie_code == LOC_RGNEXIT) {
		if (!up) {
			up = 1;
			pwwrite(chessboard_pixwin,
			    SQUARESIZE * oldrow + 1 + oldx,
			    SQUARESIZE * (8 - oldcol) + 1 + oldy,
			    (SQUARESIZE - 1),
			    (SQUARESIZE - 1), PIX_SRC ^ PIX_DST,
			    outline_pr[oldpiece], 0, 0);
			return;
		}
		else
			return;
	}
	if (ie->ie_code == MS_LEFT) {
		row = (ie->ie_locx) / SQUARESIZE;
		col = 8 - ((ie->ie_locy) / SQUARESIZE);
		if (rotate) {
			row = 7 - row;
			col = 7 - col;
		}
		if (win_inputposevent(ie)) {
	                if (row < 0 || row > 7 || col < 0 || col > 7)
				return;
			up = 0;
			oldrow = row;
			oldcol = col;
			oldpiece = piecearr[row][col];
			if (oldpiece != 0) {
				pwwrite(chessboard_pixwin,
				    SQUARESIZE*oldrow + 1 + 1,
				    SQUARESIZE*(8 - oldcol) + 1 + 1,
				    (SQUARESIZE - 1),
				    (SQUARESIZE - 1), PIX_SRC ^ PIX_DST,
				    outline_pr[oldpiece], 0, 0);
				startx = ie->ie_locx;
				starty = ie->ie_locy;
				oldx = 1;
				oldy = 1;
			}
		} else {
			if (oldpiece != 0 && !up) {
				up = 1;
				pwwrite(chessboard_pixwin,
				    SQUARESIZE * oldrow + 1 + oldx,
				    SQUARESIZE * (8 - oldcol) + 1 + oldy,
				    (SQUARESIZE - 1),
				    (SQUARESIZE - 1), PIX_SRC ^ PIX_DST,
				    outline_pr[oldpiece], 0, 0);
			}
			else {
				up = 1;
				return;
			}
	                if (row < 0 || row > 7 || col < 0 || col > 7)
				return;
			if ((oldrow != row || oldcol != col) &&
			    (sendmove(oldrow, oldcol, row, col) >= 0)) {
				make_move(oldrow, oldcol, row, col);
			    }
		}
		return;
	}
	if (ie->ie_code == LOC_MOVE && event_is_down(ie)) {
		if (up || oldpiece == 0)
			return;
		if (!rotate) {
			x = ie->ie_locx - startx;
			y = ie->ie_locy - starty;
		}
		else {
			x = startx - ie->ie_locx;
			y = starty - ie->ie_locy;
		}
		pwwrite(chessboard_pixwin, SQUARESIZE * oldrow + 1 + x,
		    SQUARESIZE * (8 - oldcol) + 1 + y, (SQUARESIZE - 1),
		    (SQUARESIZE - 1), PIX_SRC ^ PIX_DST,
		    outline_pr[oldpiece], 0, 0);
		pwwrite(chessboard_pixwin, SQUARESIZE * oldrow + 1 + oldx,
		    SQUARESIZE * (8 - oldcol) + 1 + oldy, (SQUARESIZE - 1),
		    (SQUARESIZE - 1), PIX_SRC ^ PIX_DST,
		    outline_pr[oldpiece], 0, 0);
		oldx = x;
		oldy = y;
		return;
	}
	/* fprintf(stderr, "internal error: %x is illegal ie->ie_code\n",
	     ie->ie_code);
	exit(1); */
}

paint_chessboard()
{
	register int i, j;
	pw_writebackground(chessboard_pixwin, 0, 0, BIG, BIG, PIX_CLR);
	for (i = 0; i < 9; i++) {
		pw_vector(chessboard_pixwin, SQUARESIZE * i, SQUARESIZE,
		    SQUARESIZE * i, BOARDSIZE + SQUARESIZE, PIX_SET, 1);
		pw_vector(chessboard_pixwin, 0, SQUARESIZE * (i + 1),
		    BOARDSIZE, SQUARESIZE * (i + 1), PIX_SET, 1);
	}
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			paint_square(i, j);
		}
	}
	for (i = 0; i < bcapcnt; i++)
		paint_capture(BLACK, i);
	for (i = 0; i < wcapcnt; i++)
		paint_capture(WHITE, i);
	paint_clocks();
}

paint_square(i, j)
	register int i, j;
{
	int k;
	
	if ((i + j) % 2 == 1)
		pwwrite(chessboard_pixwin, SQUARESIZE * i + 1,
		    SQUARESIZE * (8-j) + 1, (SQUARESIZE - 1),
		    (SQUARESIZE - 1), PIX_SRC, &light_square, 0, 0);
	else
		pwwrite(chessboard_pixwin, SQUARESIZE * i + 1,
		    SQUARESIZE * (8-j) + 1, (SQUARESIZE - 1),
		    (SQUARESIZE - 1), PIX_SRC, &dark_square, 0, 0);
	if ((k = piecearr[i][j]) == 0)
		return;

	if (colorarr[i][j] == BLACK) {
		pwwrite(chessboard_pixwin, SQUARESIZE * i + 1,
		    SQUARESIZE * (8 - j) + 1, (SQUARESIZE - 1),
		    (SQUARESIZE - 1), PIX_SRC | PIX_DST, piece_pr[k], 0, 0);
		pwwrite(chessboard_pixwin,
		    SQUARESIZE * i + 1, SQUARESIZE * (8 - j) + 1,
		    (SQUARESIZE - 1), (SQUARESIZE - 1),
		    PIX_NOT(PIX_SRC) & PIX_DST, outline_pr[k], 0, 0);
	}
	else if (colorarr[i][j] == WHITE) {
		pwwrite(chessboard_pixwin, SQUARESIZE * i + 1,
		    SQUARESIZE * (8 - j) + 1, (SQUARESIZE - 1),
		    (SQUARESIZE - 1),
		    PIX_NOT(PIX_SRC) & PIX_DST, piece_pr[k], 0, 0);
		pwwrite(chessboard_pixwin,
		    SQUARESIZE * i + 1, SQUARESIZE * (8 - j) + 1,
		    (SQUARESIZE - 1), (SQUARESIZE - 1),
		    PIX_SRC | PIX_DST, outline_pr[k], 0, 0);
	}
}

clearwcaparea()
{
	pwwritebackground(chessboard_pixwin, 1, 1,
	    7*SQUARESIZE - 1, (SQUARESIZE - 1), PIX_CLR);

}

clearbcaparea()
{
	pwwritebackground(chessboard_pixwin, 1, 9 * SQUARESIZE + 1,
	    7*SQUARESIZE - 1, (SQUARESIZE - 1), PIX_CLR);

}

paint_capture(color, cnt)
{
	int k;

	if (color == BLACK) {
		k = bcapture[cnt];
		pwwrite(chessboard_pixwin, capturepos[cnt] + 1,
		    9*SQUARESIZE + 1, (SQUARESIZE - 1),
		    (SQUARESIZE - 1), PIX_SRC | PIX_DST, piece_pr[k], 0, 0);
		pwwrite(chessboard_pixwin, capturepos[cnt] + 1,
		    9*SQUARESIZE + 1, (SQUARESIZE - 1),
		    (SQUARESIZE - 1),
		    PIX_NOT(PIX_SRC) & PIX_DST, outline_pr[k], 0, 0);
	}
	else {
		k = wcapture[cnt];
		pwwrite(chessboard_pixwin, capturepos[cnt] + 1, 1,
		    (SQUARESIZE - 1),  (SQUARESIZE - 1),
		    PIX_NOT(PIX_SRC) & PIX_DST, piece_pr[k], 0, 0);
		pwwrite(chessboard_pixwin, capturepos[cnt] + 1, 1,
		    (SQUARESIZE - 1), (SQUARESIZE - 1),
		    PIX_SRC | PIX_DST, outline_pr[k], 0, 0);
	}
}

static Notify_value
chesspipeready(clnt, fd)
	int	clnt;
	int	fd;
{
	readfromchess();
	return(NOTIFY_DONE);
}

static	int
killkids()
{
	killpg(getpgrp(), SIGINT);/* get rid of forked processes */
}

static showinghour;

hourglass()
{
	int             fd;

	if (showinghour)
		return;
	fd = (int) window_get(panel, WIN_FD);
	win_getcursor(fd, &oldcursor1);
	win_setcursor(fd, &hourglasscursor);

	fd = (int) window_get(chessboard, WIN_FD);
	win_getcursor(fd, &oldcursor2);
	win_setcursor(fd, &hourglasscursor);

	fd = (int) window_get(frame, WIN_FD);
	win_getcursor(fd, &oldcursor3);
	win_setcursor(fd, &hourglasscursor);
	showinghour = 1;
}

unhourglass()
{
	int             fd;

	if (!showinghour)
		return;
	fd = (int) window_get(panel, WIN_FD);
	win_setcursor(fd, &oldcursor1);
	fd = (int) window_get(chessboard, WIN_FD);
	win_setcursor(fd, &oldcursor2);
	fd = (int) window_get(frame, WIN_FD);
	win_setcursor(fd, &oldcursor3);
	showinghour = 0;
}

/* 
 * 1 sec timer: If waiting for human and iconic, flash icon.
 *		Otherwise just update clocks.
 */
Notify_value
clock_itimer(clnt, which)
	Notify_client clnt;
{
	if (window_get(frame, FRAME_CLOSED)) {
		if (!thinking && flashon)
			flashicon();
	}
	else
		update_clock();
}

flashicon()
{
	Pixwin *pw;
	int	w, h;
	
	pw = (Pixwin *)window_get(frame, WIN_PIXWIN);
	h = (int)window_get(frame, WIN_HEIGHT);
	w = (int)window_get(frame, WIN_WIDTH);
	pw_writebackground(pw, 0, 0, w, h, PIX_NOT(PIX_DST));
	slp(2);
	pw_writebackground(pw, 0, 0, w, h, PIX_NOT(PIX_DST));
}

slp(x)
{
	struct timeval tv;
	
	tv.tv_sec = x / 64;
	tv.tv_usec = (x % 64) * (1000000/64);
	select(32, 0, 0, 0, &tv);
}

pwwrite(pw, xd, yd, w, h, op, pr, xs, ys)
	struct pixwin *pw;
	struct pixrect *pr;
{
	if (!rotate)
		pw_write(pw, xd, yd, w, h, op, pr, xs, ys);
	else
		pw_write(pw, BOARDSIZE - xd - w,
		    BOARDSIZE + 2*SQUARESIZE - yd - h, w, h, op, pr, xs, ys);
}

pwwritebackground(pw, xd, yd, w, h, op)
	struct pixwin *pw;
{
	if (!rotate)
		pw_writebackground(pw, xd, yd, w, h, op);
	else
		pw_writebackground(pw, BOARDSIZE - xd - w,
		    BOARDSIZE + 2*SQUARESIZE - yd - h, w, h, op);
}

putmsg(str)
	char	*str;
{
	panel_set(msg_item, PANEL_LABEL_STRING, str, 0);
}
