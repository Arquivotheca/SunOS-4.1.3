/* @(#)chesstool.h 1.1 92/07/30 Copyr 1984 Sun Micro */

/*
 * Copyright (c) 1984 by Sun Microsystems Inc.
 */


#ifndef DST_NONE
#include <sys/time.h>
#endif

#define SQUARESIZE	65	/* width of each square in pixels */
#define	BOARDSIZE	(8*SQUARESIZE + 1)

#define NUMPIECES	7
#define PAWN		1
#define KNIGHT		2
#define BISHOP		3
#define ROOK		4
#define QUEEN		5
#define KING		6

#define CAPTURE 12
#define WPROMOTE (1 << 15)
#define BPROMOTE (1 << 16)
#define WQCASTLE (1 << 17)
#define WKCASTLE (1 << 18)
#define BQCASTLE (1 << 19)
#define BKCASTLE (1 << 20)
#define WPASSANT (1 << 21)
#define BPASSANT (1 << 22)

#define WHITE 0		/* use fact that WHITE = 1 ^ BLACK */
#define BLACK 1

extern struct msgsubwindow *mswerror;
extern struct pixwin *chessboard_pixwin;
extern int thinking;
extern short colorarr[8][8];
extern short piecearr[8][8];
extern short wcapture[], bcapture[];
extern int wcapcnt, bcapcnt;
extern int movelist[];
extern movecnt;
extern int undoqueue;
extern int rotate;
extern int done;
extern int flashon;
