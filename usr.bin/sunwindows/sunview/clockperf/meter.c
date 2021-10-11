#ifndef lint
static  char sccsid[] = "@(#)meter.c 1.1 92/07/30 Copyr 1985, 1986, 1987 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems Inc.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/time.h>
#include <suntool/sunview.h>
#include <sunwindow/rect.h>
#include <sunwindow/win_input.h>
#include <sunwindow/pixwin.h>
#include <suntool/icon.h>
#include <suntool/frame.h>
#include "meter.h"
#include "clockhands.h"

extern Frame	frame;
extern Icon	metericon;
/* 
 * icons for the meters
 */
static unsigned short ricon_data[] = {
#include <images/rspeedometer.icon>
};

static unsigned short icon_data[] = {
#include <images/speedometer.icon>
};

static unsigned short dead_data[] = {
#include <images/dead.icon>
};

static unsigned short sick_data[] = {
#include <images/sick.icon>
};


mpr_static(base_mpr, ICONWIDTH, ICONHEIGHT, 1, icon_data);
mpr_static(rbase_mpr, ICONWIDTH, RICONHEIGHT, 1, ricon_data);
mpr_static(dbase_mpr, ICONWIDTH, ICONHEIGHT, 1, dead_data);
mpr_static(sbase_mpr, ICONWIDTH, ICONHEIGHT, 1, sick_data);

char	*sprintf();

/*
 * Repaint the entire meter.
 * Paint the icon if iconic, otherwise paint the graph.
 */
meter_paint()
{
	register int i, j, left, right;
	register struct meter *mp = &meters[visible];
	register int numchars, len;
	char name[MAXCHARS];
	char host[MAXCHARS];
	int iconic;

	if (iconic = (int)window_get(frame, FRAME_CLOSED)) {
		numchars = NUMCHARS;
	} else {
		numchars = (width - 1) / PIXELSPERCHAR;
		if (numchars >= MAXCHARS)
			numchars = MAXCHARS-1;
	}
	/* 
	 * Combine name (left justified) and curmax (right justified).
	 */
	(void)sprintf(name, "%-*s%d", numchars - ndigits(mp->m_curmax),
	    mp->m_name, mp->m_curmax);
	if (remote) {
		if ((len = strlen(hostname)) > numchars)
			len = numchars;
		(void)sprintf(host, "%.*s%.*s", (numchars - len)/2,
		    "                    ", numchars, hostname);
	} 
	if (iconic) {
		struct pr_prpos where;
		
		/* Set entire icon picture */
		if (remote)
			(void)pr_rop(ic_mpr, 0, 0, ICONWIDTH, RICONHEIGHT,
			    PIX_SRC, &rbase_mpr, 0, 0);
		else
			(void)pr_rop(ic_mpr, 0, 0, ICONWIDTH, ICONHEIGHT,
			    PIX_SRC, &base_mpr, 0, 0);
		/* Draw dynamic portion */
		meter_update();
		/* Draw text portion */
		where.pr = ic_mpr;
		where.pos.x = 2;
		where.pos.y = 44; /* baseline of string */
		(void)pf_text(where, PIX_SRC, pfont, name);
		if (remote) {
			where.pos.y = 55; /* baseline of string */
			(void)pf_text(where, PIX_SRC|PIX_DST, pfont, host);
		}
		/* Redraw icon */
		tool_display(frame);
	} else {
		struct rect r;

		(void)win_getsize(window_get(frame, WIN_FD), &r);
		(void)pw_writebackground(pw, BORDER, BORDER, width, height, PIX_CLR);
		(void)pw_text(pw, BORDER, height + BORDER + 7,
		    PIX_SRC, pfont, name);
		if (remote)
			(void)pw_text(pw, BORDER, height + BORDER + NAMEHEIGHT + 7,
			    PIX_SRC, pfont, host);
		right = scaletograph(save_access(visible, saveptr));
		if (right == -1)
			goto out;
		mp->m_lastval = right;
		if ((j = saveptr - 1) < 0)
			j = MAXSAVE - 1;
		for (i = width - 1; (i > 0) && (j != saveptr); i--) {
			left = scaletograph(save_access(visible, j));
			if (left == -1)
				goto out;
			(void)pw_vector(pw, i - 1 + BORDER, left + BORDER,
			    i + BORDER, right + BORDER, PIX_SRC, 1);
			right = left;
			if (--j < 0)
				j = MAXSAVE - 1;
		}
	out:
		if (dead)
			(void)pw_write(pw, BORDER, BORDER, width, height,
				PIX_SRC|PIX_DST, &dbase_mpr, 0, 0);
		if (sick)
			(void)pw_write(pw, BORDER, BORDER, width, height,
				PIX_SRC|PIX_DST, &sbase_mpr, 0, 0);
	}
}

/*
 * Update the display with the current data points.
 * If iconic, paint in a new set of hands, otherwise
 * scroll the graph to the left and add the new data
 * point.
 */
meter_update()
{
	register int minutedata, hourdata;
	register struct meter *mp = &meters[visible];
	register int curmax = mp->m_curmax;

	minutedata = getminutedata();
	hourdata = gethourdata();

	/*
	 * If data is off scale, increase max, up to limit of maxmax.
	 */
	if (minutedata > curmax*FSCALE) {
		while (curmax*FSCALE < minutedata && 2*curmax <= mp->m_maxmax)
			curmax *= 2;
		mp->m_curmax = curmax;
		meter_paint();
		return;
	}

	/*
	 * If all data on graph is under 1/3 of max, decrease max,
	 * to limit of minmax.
	 */
	if (minutedata < curmax*FSCALE/3)
		mp->m_undercnt++;
	else
		mp->m_undercnt = 0;
	if (mp->m_undercnt >= width) {
		if (curmax/2 >= mp->m_minmax) {
			mp->m_undercnt = 0;
			mp->m_curmax /= 2;
			meter_paint();
			return;
		}
	}

	
	if ((int)window_get(frame, FRAME_CLOSED)) {
		register struct hands *hand;
		register int trunc, datatometer;

		if (remote)
			(void)pr_rop(ic_mpr, 0, 0, ICONWIDTH, 36,
			    PIX_SRC, &rbase_mpr, 0, 0);
		else
			(void)pr_rop(ic_mpr, 0, 0, ICONWIDTH, 36,
			    PIX_SRC, &base_mpr, 0, 0);

		datatometer = curmax*FSCALE/30;
		/* short hand displays short average */
		trunc = ((hourdata > curmax*FSCALE) ? curmax*FSCALE : hourdata);
		hand = &hand_points[(trunc/datatometer + 45) % 60];
		(void)pr_vector(ic_mpr, hand->x1, hand->y1,
		    hand->hour_x, hand->hour_y, PIX_SET, 1);
		(void)pr_vector(ic_mpr, hand->x2, hand->y2,
		    hand->hour_x, hand->hour_y, PIX_SET, 1);

		/* long hand displays long average */
		trunc = ((minutedata > curmax*FSCALE) ? curmax*FSCALE : minutedata);
		hand = &hand_points[(trunc/datatometer + 45) % 60];
		(void)pr_vector(ic_mpr, hand->x1, hand->y1, hand->min_x, hand->min_y,
		    PIX_SET, 1);
		(void)pr_vector(ic_mpr, hand->x2, hand->y2, hand->min_x, hand->min_y,
		    PIX_SET, 1);
		if (dead)
			(void)pr_rop(ic_mpr, 0, 0, ICONWIDTH, 35,
				PIX_SRC|PIX_DST, &dbase_mpr, 0, 0);
		if (sick)
			(void)pr_rop(ic_mpr, 0, 0, ICONWIDTH, 35,
				PIX_SRC|PIX_DST, &sbase_mpr, 0, 0);
		tool_display(frame);
	} else {
		register int graphdata;
		struct rect r;
		
		if (dead || sick)
			return;
		graphdata = scaletograph(minutedata);
		if (mp->m_lastval == -1)
			mp->m_lastval = graphdata;
		rect_construct(&r, BORDER, BORDER, width, height);
		(void)pw_copy(pw, BORDER, BORDER, width, height,
		    PIX_SRC, pw, BORDER+1, BORDER);
		/* 
		 * Check to see if the source of the copy was obscured.
		 */
		if (!rl_empty(&pw->pw_fixup)) {
			m_set_clipping_equal_fixup();
			meter_paint();
			(void)pw_exposed(pw);
		}

		(void)pw_writebackground(pw, width-1+BORDER, BORDER, 1, height,
		    PIX_CLR);
		(void)pw_vector(pw, width-2+BORDER, BORDER+mp->m_lastval,
		    width-1+BORDER, graphdata+BORDER, PIX_SRC, 1);
		mp->m_lastval = graphdata;
	}
}

/*
 * Hack to set the clipping area for the pixwin to be the fixup
 * area so that later repaint will paint only the area that needs
 * to be fixed up.
 *
 * XXX - there should be a standard way to do this.
 */
m_set_clipping_equal_fixup()
{
	int screenX, screenY;
	struct rect screenrect;

	(void)win_getsize(window_get(frame, WIN_FD), &screenrect);
	(void)win_getscreenposition(pw->pw_clipdata->pwcd_windowfd,
	    &screenX, &screenY);
	screenrect.r_left = screenX;
	screenrect.r_top = screenY;
	(void)rl_free(&pw->pw_clipdata->pwcd_clipping);
	pw->pw_clipdata->pwcd_clipping = pw->pw_fixup;
	pw->pw_fixup = rl_null;
	(void)_pw_setclippers(pw, &screenrect);
}

/*
 * Scale data value to graph value.
 */
scaletograph(x)
	int x;
{
	
	if (x == -1)		/* to detect end of data */
		return (-1);
	if (x > meters[visible].m_curmax*FSCALE)
		return (0);
	else {
		/* should round */
		return ((height-1) - (x*(height-1)) /
		    (FSCALE * meters[visible].m_curmax));
	}
}

/*
 * How many digits are required to display n?
 */
ndigits(n)
	register int n;
{
	register int cnt = 1;
	
	if (n < 0) {
		n = -n;
		cnt++;
	}
	while (n >= 10) {
		n /= 10;
		cnt++;
	}
	return (cnt);
}
