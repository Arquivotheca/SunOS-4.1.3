/*	@(#)defs.h 1.1 92/07/30 SMI */


/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#define TILE_WIDTH	34

#define TIMER_WIDTH	41
#define TIMER_HEIGHT	142

#define LETTER_WIDTH	12
#define LETTER_HEIGHT	20
#define BOARD_BASELEFT	2
#define BOARD_BASETOP	2
#define BOARD_SPACING	2
#define LETTER_Y_OFFSET	7
#define CENTER_LETTER_X_OFFSET	11
#define FIRST_LETTER_X_OFFSET	5
#define SECOND_LETTER_X_OFFSET	(FIRST_LETTER_X_OFFSET + LETTER_WIDTH)

#define TIMER_BASELEFT	(BOARD_BASELEFT + 4 * (TILE_WIDTH + BOARD_SPACING))
#define TIMER_BASETOP	BOARD_BASETOP

#define DISPLAY_BOTTOM	(BOARD_BASETOP + 4 * (TILE_WIDTH + BOARD_SPACING))
#define DISPLAY_RIGHT	(TIMER_BASELEFT + TIMER_WIDTH)

#define MAXWORDLEN	9
#define SCROLL_OVERLAP	3

#define bogwidth	(bogrect.r_width)
#define bogheight	(bogrect.r_height)
#define fontwidth	(bogfont->pf_defaultsize.x)
#define fontheight	(bogfont->pf_defaultsize.y)
#define fontoffset	(-bogfont->pf_char['n'].pc_home.y)

#define drawcursor()	pw_write(bogwin, typein_x, typein_y, fontwidth, fontheight, PIX_NOT(PIX_DST), NULL, 0, 0)

#define SANDGRAINS	2308
#define compute_delay(c)	((60 * c * 1000000) / SANDGRAINS)

#define toparent	pfd[1]
#define fromchild	pfd[0]

int childactive;
int setchildbits;
int childpid;	/* process id of child process */
int pfd[2];	/* pipe between two processes */

FILE *childfp;

int stoptimer;

int typein_x, typein_y;	/* the upper-left corner of the current cursor location */
int leftmargin;		/* left coordinate of current type-in column */
struct pixrect *tri_left_pr, *tri_right_pr;

char cword[512];	/* word currently being typed in */
int cwordlen,		/* number of characters in cword */
    scrolled;		/* index of first character displayed (if scrolled) */

struct tool	*tool;
struct toolsw		*bogsw;
struct pixwin		*bogwin;
struct rect		bogrect;
struct pixfont		*bogfont;

/*
 * reuse controls whether or not cubes can be reused; if reuse == 0,
 * no cubes may be reused; if reuse == 1, cubes may be reused; if
 * reuse > 1, cubes may be reused and cubes are adjacent to themselves
 */

int reuse;

int feedback_x;	/* used for feedback for duplicate words (see isduplicate) */
int feedback_y;
int feedback_w;

char *dictionary;	/* name of dictionary to use */

#define INSTRUCTIONS	0	/* ask if player wants instructions */
#define CONFIRM_INSTR	1	/* let player confirm having read instr. */
#define PLAYING		2	/* playing game */
#define GAMEOVER	3	/* time is up */

int state;

#define MWORDLEN	64	/* maximum size of a typed in word (note: can be > 16) */

char laststr[MWORDLEN];	/* last string highlighted by showcubes() */
