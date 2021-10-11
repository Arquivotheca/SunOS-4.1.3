#ifndef lint
static	char sccsid[] = "@(#)lists.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * lists.c: manage the lists of words found by the human and the computer
 */

#include <suntool/tool_hs.h>
#include <stdio.h>

#include "defs.h"

#define MAXWORDS	1000	/* maximum number of words that can be input */

struct item {			/* item in a list of words */
	char *word;		/* pointer to memory for word */
	int x, y;		/* location on screen (not defined for computer's words) */
} humanwords[MAXWORDS], compwords[MAXWORDS];	/* the two lists */
int humanwordcnt, compwordcnt;	/* index of next free space in respective list */

char *malloc();

/*
 * freehumanwords: free malloc'ed space in human word list
 */

freehumanwords()
{
	while (humanwordcnt)
		if (humanwords[--humanwordcnt].word != NULL)
			free(humanwords[humanwordcnt]);
}

/*
 * freecompwords: free malloc'ed space in computer word list
 */

freecompwords()
{
	while (compwordcnt)
		if (compwords[--compwordcnt].word != NULL)
			free(compwords[compwordcnt]);
}

/*
 * addhumanword: add a word to human list; also caches the screen location
 *               of the word for redisplay
 */

addhumanword(str, x, y)
char *str;		/* word to add */
int x, y;		/* screen position where word is drawn */
{
	if (humanwordcnt >= MAXWORDS) {
		fprintf(stderr, "Too many words!\n");
		exit(1);
	}
	humanwords[humanwordcnt].word = malloc(strlen(str) + 1);
	if (humanwords[humanwordcnt].word == NULL) {
		fprintf(stderr, "Out of memory!\n");
		exit(1);
	}
	strcpy(humanwords[humanwordcnt].word, str);
	humanwords[humanwordcnt].x = x;
	humanwords[humanwordcnt].y = y;
	humanwordcnt++;
}

/*
 * addcompword: add a word to computer list
 */

addcompword(str)
char *str;		/* word to add */
{
	if (compwordcnt >= MAXWORDS) {
		fprintf(stderr, "Too many words!\n");
		exit(1);
	}
	compwords[compwordcnt].word = malloc(strlen(str) + 1);
	if (compwords[compwordcnt].word == NULL) {
		fprintf(stderr, "Out of memory!\n");
		exit(1);
	}
	compwords[compwordcnt].x = compwords[compwordcnt].y = -1;
	strcpy(compwords[compwordcnt++].word, str);
}

/*
 * isduplicate: return true if human has already typed in the word str
 */

isduplicate(str)
char *str;	/* word to check */
{
	int index;

	for (index = 0; index < humanwordcnt; index++) {
		if (humanwords[index].word == NULL)
			continue;
		if (!strcmp(humanwords[index].word, str)) {
			if (humanwords[index].x >= 0 && humanwords[index].y >= 0) {
				feedback_x = humanwords[index].x;
				feedback_y = humanwords[index].y;
				feedback_w = min(strlen(str), MAXWORDLEN) * fontwidth;
				pw_write(bogwin, feedback_x, feedback_y, feedback_w, fontheight, PIX_NOT(PIX_DST), NULL, 0, 0);
			} else {
				feedback_w = -1;	/* off screen */
			}
			return(1);
		}
	}
	return(0);
}

/*
 * comparelists: sort the human's words and remove them from the computer's
 *               word list to end up with a list of words the human did not
 *               find
 */

comparelists()
{
	int compare(), i1, i2, i3, val;

	qsort(humanwords, humanwordcnt, sizeof(struct item), compare);
	i1 = 0;
	while (i1 < humanwordcnt - 1) {	/* remove duplicates in human list */
		if (!strcmp(humanwords[i1].word, humanwords[i1 + 1].word)) {
			for (i2 = i1 + 1; i2 < humanwordcnt - 1; i2++)
				humanwords[i2].word = humanwords[i2 + 1].word;
			humanwordcnt--;
		} else {
			i1++;
		}
	}
	i1 = i2 = 0;	/* remove human's words from computer list */
	while (i1 < humanwordcnt) {
		while (strcmp(humanwords[i1].word, compwords[i2].word))
			i2++;
		--compwordcnt;
		for (i3 = i2; i3 < compwordcnt; i3++)
			compwords[i3].word = compwords[i3 + 1].word;
		i1++;
	}
}

compare(a, b)
struct item *a, *b;
{
	if (a->word == NULL)
		return(1);
	if (b->word == NULL)
		return(-1);
	return(strcmp(a->word, b->word));
}

/*
 * displaylists: display the two lists of words (words human found and
 *               words in grid that human didn't find)
 */

#define nextline(x, y)							\
	y += fontheight;						\
	x = (y < DISPLAY_BOTTOM) ? DISPLAY_RIGHT + BOARD_SPACING : 0;	\
	if (x % ((MAXWORDLEN + 1) * fontwidth) != 0)			\
		x += ((MAXWORDLEN + 1) * fontwidth) - (x % ((MAXWORDLEN + 1) * fontwidth))

displaylists()
{
	int x, y, index, len;
	long inqueue;
	struct inputevent ie;

	pw_write(bogwin, DISPLAY_RIGHT, 0, bogwidth - DISPLAY_RIGHT, DISPLAY_BOTTOM, PIX_SRC, 0, 0, 0);
	pw_write(bogwin, 0, DISPLAY_BOTTOM, bogwidth, bogheight - DISPLAY_BOTTOM, PIX_SRC, 0, 0, 0);
	y = -fontheight;
	nextline(x, y);
	if (humanwordcnt > 0) {
		pw_text(bogwin, x, y + fontoffset, PIX_SRC, bogfont, "Words you found:");
		nextline(x, y);
		for (index = 0; index < humanwordcnt; index++) {
			len = strlen(humanwords[index].word);
			if (x + len * fontwidth > bogwidth) {
				nextline(x, y);
			}
			pw_text(bogwin, x, y + fontoffset, PIX_SRC, bogfont, humanwords[index].word);
			humanwords[index].x = x;
			humanwords[index].y = y;
			len = (len / 10 + 1) * 10;
			x += len * fontwidth;
		}
		nextline(x, y);
		nextline(x, y);
	}
	if (compwordcnt > 0) {
		pw_text(bogwin, x, y + fontoffset, PIX_SRC, bogfont, "Words you missed:");
		nextline(x, y);
		for (index = 0; index < compwordcnt; index++) {
			len = strlen(compwords[index].word);
			if ((x + (len * fontwidth)) > bogwidth) {
				nextline(x, y);	/* brace are mandatory!!! */
			}
			pw_text(bogwin, x, y + fontoffset, PIX_SRC, bogfont, compwords[index].word);
			compwords[index].x = x;
			compwords[index].y = y;
			len = (len / 10 + 1) * 10;
			x += len * fontwidth;
		}
		nextline(x, y);
		nextline(x, y);
	}
	pw_text(bogwin, x, y + fontoffset, PIX_SRC, bogfont, "Type any capital letter to play again...");
}

/*
 * drawtypein: redraw the human's typed in words when the display size
 *             changes (rescale to fit the new window)
 */

drawtypein()
{
	int index;
	int full, len, c;
	char buf[MAXWORDLEN+1];

	buf[MAXWORDLEN] = '\0';
	full = 0;
	typein_x = typein_y = leftmargin = -1;
	for (index = 0; index < humanwordcnt; index++) {
		if (full || (full = nextscreenpos(&leftmargin, &typein_x, &typein_y))) {	/* screen is full */
			humanwords[index].x = humanwords[index].y = -1;
		} else {
			strncpy(buf, humanwords[index].word, MAXWORDLEN);
			pw_text(bogwin, typein_x, typein_y + fontoffset, PIX_SRC, bogfont, buf);
			if (strlen(humanwords[index].word) > MAXWORDLEN)
				pw_write(bogwin, MAXWORDLEN * fontwidth, typein_y, fontwidth, fontheight, PIX_SRC, tri_right_pr, 0, 0);
			humanwords[index].x = leftmargin;
			humanwords[index].y = typein_y;
		}
	}
	if (full)
		pw_write(bogwin, leftmargin, typein_y, fontwidth * MAXWORDLEN,
					fontheight, PIX_SRC, NULL, 0, 0);
	else
		(void) nextscreenpos(&leftmargin, &typein_x, &typein_y);
	if (cwordlen != 0) {
		if (cwordlen > MAXWORDLEN) {
			pw_write(bogwin, leftmargin, typein_y, fontwidth, fontheight, PIX_SRC, tri_left_pr, 0, 0);
			c = cwordlen - SCROLL_OVERLAP;
			typein_x += fontwidth;
		} else {
			c = 0;
		}
		for (; c < cwordlen; c++) {
			pw_char(bogwin, typein_x, typein_y + fontoffset, PIX_SRC, bogfont, cword[c]);
			typein_x += fontwidth;
		}
	}
	drawcursor();
	if (feedback_w) {	/* note: feedback_w < 0 means feedback was off screen */
		feedback_w = 0;	/* redraw feedback in the right place */
		(void) isduplicate(cword);
	}
}

/*
 * removefeedback: make the position of the word that used to be displayed
 *                 at x,y undefined so no feedback will be displayed; used
 *                 when there are too many words to fit on the screen, and
 *                 thus some of them must be overwritten
 */

removefeedback(x, y)
register x, y;
{
	register index;

	for (index = 0; index < humanwordcnt; index++) {
		if (humanwords[index].x == x && humanwords[index].y == y) {
			humanwords[index].x = humanwords[index].y = -1;
			return;
		}
	}
}

#define isonword(x, y, wx, wy, ww) (x >= wx && x < wx + ww && y >= wy && y < wy + fontheight)

/*
 * mouseonword: determine the word pointed to by the mouse at x,y and
 *              return a pointer to it
 */

char *
mouseonword(x, y)
register x, y;
{
	register index;
	register width;

	for (index = 0; index < humanwordcnt; index++) {
		if (state == PLAYING) {
			width = MAXWORDLEN;
		} else {
			width = strlen(humanwords[index].word);
			width += width - width % MAXWORDLEN;
		}
		width *= fontwidth;
		if (isonword(x, y, humanwords[index].x, humanwords[index].y, width))
			return(humanwords[index].word);
	}
	if (state == PLAYING) {
		if (isonword(leftmargin, typein_y, humanwords[index].x, humanwords[index].y, MAXWORDLEN * fontwidth)) {
			cword[cwordlen] = '\0';
			return(cword);
		}
		return(NULL);
	}
	for (index = 0; index < compwordcnt; index++) {
		width = strlen(compwords[index].word);
		width += width - width % MAXWORDLEN;
		width *= fontwidth;
		if (isonword(x, y, compwords[index].x, compwords[index].y, width))
			return(compwords[index].word);
	}
	return(NULL);
}
