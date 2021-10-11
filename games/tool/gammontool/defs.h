/*	@(#)defs.h 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* Conventions:
 *	points are numbered in increasing order from White's home
 *	in the lower right-hand corner to Black's home in the upper
 *	right-hand corner
 *
 *	the Bar and Home are not assigned numbers but are called
 *	BAR and HOME; the fact that BAR is defined to be 25 and HOME
 *	is defined as 0 has NO significance (since under this scheme
 *	scheme Black starts from 25, proceeds to 1, and ends up at 0)
 *
 *	all elements of play are done locally except for the actual
 *      strategy of the computer; checking the human's move, rolling
 *      the dice, scoring, etc. are all done by this program
 *
 *      the playing program must communicate using the codes in bkgcodes.h
 */

#include <stdio.h>
#include "bkgcodes.h"

#define ORIGINAL_CUR	0		/* indices into cursors array */
#define ROLL_CUR	1
#define MOVE_CUR	2
#define THINKING_CUR	3
#define DOUBLING_CUR	4

#define STARTGAME		0	/* all possible states of play */
#define COMPUTERDOUBLING	1
#define HUMANDOUBLING		2
#define ROLL			3
#define MOVE			4
#define THINKING		5
#define GAMEOVER		6

#define MSG	0	/* two kinds of messages for message() */
#define ERR	1

#define HUMAN		0	/* constants for whose turn it is, */
#define COMPUTER	1	/* who last doubled, etc. */
#define NOBODY		2
#define BOTH		3	/* also applies to both black and white */

#define STRAIGHT	1	/* types of wins */
#define GAMMON		2
#define BACKGAMMON	3
#define RESIGNED	4
#define REFUSEDDBLE	5

#define WHITE	0	/* colors of playing pieces */
#define BLACK	1
#define NONE	2
#define computercolor	(humancolor == WHITE ? BLACK : WHITE)

#define OUTOFBOUNDS	-1

#define BAR	25	/* the indices of the obvious in the board arrays */
#define HOME	0

#define ROLLUSED	8	/* bit mask indicating roll has been used */
#define SECONDROLLUSED	16	/* same for second use when doubles rolled */
#define NUMMASK		7	/* mask to get actual number of roll */

#define BLOT	-1	/* used by findmaxmoves */
#define BLOCKED	-2

#define TOP	0
#define BOTTOM	1
#define isontop(point)	(point > 12 && point != BAR && point != HOME)
#define isonbot(point)	(point < 13 && point != BAR && point != HOME)

#define FROM	0
#define TO	1
#define HITBLOT	2
#define DIEUSED	3

#define FLASH	1
#define NOFLASH	0

#define	MAXNAME	15

int execbkg;
int state;		/* the current state of play */
char *gammonbin;	/* actual pathname of backgammon program */
int humanscore, computerscore;	/* cumulative scores */
char humanname[MAXNAME+1];	/* name of human player */
int gammonpid;		/* pid of backgammon child */
int gammonfd;		/* fd of pipe to read from backgammon */
int gamevalue;		/* point value of current game */
int humancolor;		/* color of human */
int humanboard[26], computerboard[26];	/* actual playing boards */
int humandieroll[2], computerdieroll[2];	/* current roll of dice */
int numhdice, numcdice;		/* number of dice which are valid */
int movesmade;		/* moves actually accomplished this turn */
int maxmoves;		/* numbers of moves human can make this turn */
int lastdoubled;	/* player who last doubled */
int diddouble;		/* player double this turn */
int lastmoved;		/* for showlastmove: player who last moved */
int dicedisplayed;	/* for showlastmove: dice on display */
int alreadyrolled;	/* special flag for first roll */
FILE *logfp;		/* log file */
int lastroll[2][3];	/* last die roll for show-last-move */
