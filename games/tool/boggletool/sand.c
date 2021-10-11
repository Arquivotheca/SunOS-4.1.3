#ifndef lint
static	char sccsid[] = "@(#)sand.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * sand.c: routines to handle the hourglass timer
 */

#include <suntool/tool_hs.h>
#include <stdio.h>

#include "defs.h"
#include "outline.h"
#include "sandframes.h"

#define TOPSANDMAX	68

#define SANDMAXHEIGHT	62
#define STARTBUFFER	10

int bellcurve[TIMER_WIDTH] = {
	11, 10, 9, 8, 8, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 2, 2, 2, 1, 0,
	1, 2, 2, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8, 8, 9, 10, 11
};

int sandtodpy[TIMER_WIDTH], sandlefttodpy;
int sandheight;
int lastsandframe;
int topsandbuffer;
int topsandline;
struct rect timerrect = { TIMER_BASELEFT, TIMER_BASETOP, TIMER_WIDTH, TIMER_HEIGHT };
struct timeval lastgrain;
unsigned long sand_delay = compute_delay(3);	/* 3 minutes */
struct timeval		tv_poll = {0, 0};

#define restartsanddpy(c)	for (c = 0; c < TIMER_WIDTH; c++) { sandtodpy[c] = c; } sandlefttodpy = TIMER_WIDTH
#define shiftsanddpy(s,c)	for (c = s; c < TIMER_WIDTH-1; c++) { sandtodpy[c] = sandtodpy[c+1]; } sandlefttodpy--

starttimer()
{
	int x, y;

	lastsandframe = -1;
	sandheight = 0;
	topsandbuffer = STARTBUFFER;
	topsandline = 0;
	restartsanddpy(x);
	gettimeofday(&lastgrain, NULL);
	pr_rop(&sandstencil_pr, 0, 0, TIMER_WIDTH, TIMER_HEIGHT, PIX_NOT(PIX_SRC), &hourglass_pr, 0, 0);
	for (y = 0; y < TOPSANDHEIGHT; y++)
		pr_rop(&sandstencil_pr, topsandlines[y][0], y + STARTTOPSAND, topsandlines[y][1], 1, PIX_SRC, 0, 0, 0);
	bogsw->ts_io.tio_timer = &tv_poll;
}

drawtimer()
{
	pw_write(bogwin, TIMER_BASELEFT, TIMER_BASETOP, TIMER_WIDTH,
		TIMER_HEIGHT, PIX_NOT(PIX_SRC), &sandstencil_pr, 0, 0);
}

movesand()
{
	struct timeval now;
	unsigned long udiff;

	movefallingsand();
	gettimeofday(&now, 0);
	udiff = (now.tv_sec - lastgrain.tv_sec) * 1000000 + (now.tv_usec - lastgrain.tv_usec);
	lastgrain.tv_usec += (udiff - udiff % sand_delay);
	while (lastgrain.tv_usec > 1000000) {
		lastgrain.tv_sec += 1;
		lastgrain.tv_usec -= 1000000;
	}
	while (udiff > sand_delay) {
		if (movebottomsand()) {
			erasefallingsand();
			return(1);
		}
		udiff -= sand_delay;
	}
	return(0);
}

movefallingsand()
{
	pw_lock(bogwin, &timerrect);
	if (lastsandframe != -1)
		pw_stencil(bogwin, TIMER_BASELEFT + SANDFALL_SHIFT, TIMER_BASETOP + SANDFALL_OFFSET, SANDFALL_WIDTH, SANDFALL_HEIGHT, PIX_NOT(PIX_SRC) & PIX_DST, &sandstencil_pr, SANDFALL_SHIFT, SANDFALL_OFFSET, sand_pr[lastsandframe], 0, 0);
	lastsandframe = random() % SANDFRAMES;
	pw_write(bogwin, TIMER_BASELEFT + SANDFALL_SHIFT, TIMER_BASETOP + SANDFALL_OFFSET, SANDFALL_WIDTH, SANDFALL_HEIGHT, PIX_SRC | PIX_DST, sand_pr[lastsandframe], 0, 0);
	pw_unlock(bogwin);
}

erasefallingsand()
{
	if (lastsandframe != -1)
		pw_stencil(bogwin, TIMER_BASELEFT + SANDFALL_SHIFT, TIMER_BASETOP + SANDFALL_OFFSET, SANDFALL_WIDTH, SANDFALL_HEIGHT, PIX_NOT(PIX_SRC) & PIX_DST, &sandstencil_pr, SANDFALL_SHIFT, SANDFALL_OFFSET, sand_pr[lastsandframe], 0, 0);
	lastsandframe = -1;
}

movebottomsand()
{
	int index, x, y, c;

	if (sandheight > SANDMAXHEIGHT)
		return(1);
	for (;;) {
		if (sandlefttodpy <= 0) {
			restartsanddpy(c);
			sandheight++;
			if (sandheight > SANDMAXHEIGHT)
				return(1);
		}
		index = random() % sandlefttodpy;
		x = sandtodpy[index];
		shiftsanddpy(index, c);
		y = TIMER_HEIGHT - sandheight + bellcurve[x];
		if (y >= TIMER_HEIGHT)
			continue;
		if ((c = pr_get(&fullhourglass_pr, x, y)) == 0)
			continue;
		pr_put(&sandstencil_pr, x, y, c ^ 1);
		pw_put(bogwin, x + TIMER_BASELEFT, y + TIMER_BASETOP, c);
		if (++topsandbuffer == topsandlines[topsandline][1]) {
			pw_write(bogwin, TIMER_BASELEFT + topsandlines[topsandline][0], topsandline + STARTTOPSAND + TIMER_BASETOP, topsandlines[topsandline][1], 1, PIX_SRC, 0, 0, 0);
			pr_rop(&sandstencil_pr, topsandlines[topsandline][0], topsandline + STARTTOPSAND, topsandlines[topsandline][1], 1, PIX_NOT(PIX_SRC), 0, 0, 0);
			topsandbuffer = 0;
			topsandline++;
		}
		break;
	}
	return(0);
}
