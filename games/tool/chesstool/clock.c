#ifndef lint
static  char sccsid[] = "@(#)clock.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems Inc.
 */

#include <stdio.h>
#include <suntool/tool_hs.h>
#include <suntool/msgsw.h>
#include "chesstool.h"

struct	hands	{
	char	/* use char to save data space (biggest value is < 256)	*/
		x1, y1,		/* first origin for this position	*/
		x2, y2,		/*  second origin for this position	*/
		sec_x, sec_y,	/*  second-hand origin for this minute	*/
		min_x, min_y,	/*  end of minute- and second hands	*/
		hour_x, hour_y; /*  end of hour hand			*/
};
#include "clockhands.h"		/* Defines hand_points			*/

static short icon_data[] = {
#include <images/chess.clock.icon>
};
mpr_static(base_mpr, 64, 64, 1, icon_data);

static	struct pixrect *clock_mpr;
static	struct icon clockicon = {64, 64, (struct pixrect *)0,
	    {0, 0, 64, 64}, 0,
	    {0, 0, 0, 0}, (char *)0, (struct pixfont *)0, 0};

static wtime, btime, wstart, bstart;

init_clock()
{
	clock_mpr = mem_create(64, 64, 1);
}

startclock()
{
	wtime = 0;		/* elapsed white time */
	btime = 0;
	wstart = 1;		/* just started white's clock */
	bstart = 0;
}

paint_clocks()
{
	clock_painthands(btime);
	pwwrite(chessboard_pixwin, SQUARESIZE * 7 + 1, 1,
	    (SQUARESIZE - 1), (SQUARESIZE - 1), PIX_SRC, clock_mpr, 0, 0);

	clock_painthands(wtime);
	pwwrite(chessboard_pixwin, SQUARESIZE * 7 + 1, 9 * SQUARESIZE + 1,
	    (SQUARESIZE - 1), (SQUARESIZE - 1), PIX_SRC, clock_mpr, 0, 0);
}

/*
 * called every second (roughly)
 */
update_clock()
{
	int tmp;
	static int blastreading, wlastreading;
	int i, j, k, l, mv;

	if (done)
		return;
	if (flashon && !thinking && !done && movecnt > 0) {
		mv = movelist[movecnt-1];
		unpack(mv, &i, &j, &k, &l);
		if (mv & WPASSANT)
			l++;
		else if (mv & BPASSANT)
			l--;
		colorarr[k][l] ^= 1;
		paint_square(k, l);
		colorarr[k][l] ^= 1;
		paint_square(k, l);
	}
	if (movecnt & 1) {	/* run black's clock */
		wstart = 1;
		if (bstart) {
			bstart = 0;
			blastreading = time(0);
		}
		tmp = time(0);
		btime += (tmp - blastreading);
		blastreading = tmp;
		clock_painthands(btime);
		pwwrite(chessboard_pixwin, SQUARESIZE * 7 + 1,
		    1, (SQUARESIZE - 1),
		    (SQUARESIZE - 1), PIX_SRC, clock_mpr, 0, 0);
	}
	else {
		bstart = 1;
		if (wstart) {
			wstart = 0;
			wlastreading = time(0);
		}
		tmp = time(0);
		wtime += (tmp - wlastreading);
		wlastreading = tmp;
		clock_painthands(wtime);
		pwwrite(chessboard_pixwin, SQUARESIZE * 7 + 1,
		    9*SQUARESIZE + 1, (SQUARESIZE - 1),
		    (SQUARESIZE - 1), PIX_SRC, clock_mpr, 0, 0);
	}
}

clock_painthands(tm)
{
	struct hands   *hand;
	int hours, mins, secs;

	hours = tm/3600;
	mins = (tm/60) % 60;
	secs = tm % 60;

	pr_rop(clock_mpr, 0, 0, 640, 64, PIX_SRC, &base_mpr, 0, 0);
	hand = &hand_points[(hours*5 + (mins + 6)/12) % 60];
	pr_vector(clock_mpr, hand->x1, hand->y1,
	    hand->hour_x, hand->hour_y, PIX_SET, 1);
	pr_vector(clock_mpr, hand->x2, hand->y2,
	    hand->hour_x, hand->hour_y, PIX_SET, 1);

	hand = &hand_points[mins];
	pr_vector(clock_mpr, hand->x1, hand->y1,
	    hand->min_x, hand->min_y, PIX_SET, 1);
	pr_vector(clock_mpr, hand->x2, hand->y2,
	    hand->min_x, hand->min_y, PIX_SET, 1);

	hand = &hand_points[secs];
	pr_vector(clock_mpr, hand->sec_x, hand->sec_y,
	    hand->min_x, hand->min_y, PIX_SET, 1);
}
