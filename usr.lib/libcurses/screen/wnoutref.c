/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)wnoutref.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.11 */
#endif

#include	"curses_inc.h"

/* Like refresh but does not output */

wnoutrefresh(win)
register	WINDOW	*win;
{
    register	short	*bch, *ech, *sbch, *sech;
    register	chtype	**wcp, **scp, *wc, *sc;
    register	int	*hash;
    int		y, x, xorg, yorg, scrli, scrco,
		boty, sminy, smaxy, minx, maxx, lo, hi;
    bool	doall;

    if (win->_parent)
	wsyncdown(win);

    doall = win->_clear;

    sminy = SP->Yabove;
    smaxy = sminy + LINES;
    scrli = curscr->_maxy;
    scrco = curscr->_maxx;

    yorg = win->_begy + win->_yoffset;
    xorg = win->_begx;

    /* save flags, cursor positions */
    if (!win->_leave && ((y = win->_cury + yorg) >= 0) && (y < scrli) &&
	((x = win->_curx + xorg) >= 0) && (x < scrco))
    {
	_virtscr->_cury = y;
	_virtscr->_curx = x;
    }
    if (!(win->_use_idc))
	_virtscr->_use_idc = FALSE;
    if (win->_use_idl)
	_virtscr->_use_idl = TRUE;
    if (win->_clear)
    {
	_virtscr->_clear = TRUE;
	win->_clear = FALSE;
	win->_flags |= _WINCHANGED;
    }

    if (!(win->_flags & _WINCHANGED))
	goto done;

    /* region to update */
    boty = win->_maxy+yorg;
    if (yorg >= sminy && yorg < smaxy && boty >= smaxy)
	boty = smaxy;
    else
	if (boty > scrli)
	    boty = scrli;
    boty -= yorg;

    minx = 0;
    if ((maxx = win->_maxx+xorg) > scrco)
	maxx = scrco;
    maxx -= xorg + 1;

    /* update structure */
    bch = win->_firstch;
    ech = win->_lastch;
    wcp = win->_y;

    hash = _VIRTHASH + yorg;
    sbch = _virtscr->_firstch + yorg;
    sech = _virtscr->_lastch + yorg;
    scp  = _virtscr->_y + yorg;

    /* first time around, set proper top/bottom changed lines */
    if (curscr->_sync)
    {
	_VIRTTOP = scrli;
	_VIRTBOT = -1;
    }

    /* update each line */
    for (y = 0; y < boty; ++y, ++hash, ++bch, ++ech, ++sbch, ++sech, ++wcp, ++scp)
    {
	if (!doall && *bch == _INFINITY)
	    continue;

	lo = (doall || *bch == _REDRAW || *bch < minx) ? minx : *bch;
	hi = (doall || *bch == _REDRAW || *ech > maxx) ? maxx : *ech;

	if (hi < lo)
	    continue;

	/* update the change structure */
	if (*bch == _REDRAW || *sbch == _REDRAW)
	    *sbch =  _REDRAW;
	else
	{
	    if (*sbch > lo+xorg)
		*sbch = lo+xorg;
	    if (*sech < hi+xorg)
		*sech = hi+xorg;
	}
	if ((y + yorg) < _VIRTTOP)
	    _VIRTTOP = y+yorg;
	if ((y + yorg) > _VIRTBOT)
	    _VIRTBOT = y + yorg;
	*bch = _INFINITY;
	*ech = -1;

	/* update the image */
	wc = *wcp + lo;
	sc = *scp + lo + xorg;
	(void) memcpy((char *) sc, (char *) wc, (int) (((hi - lo) + 1) * sizeof(chtype)));

	/* the hash value of the line */
	*hash = _NOHASH;
    }

done:
    _virtscr->_flags |= _WINCHANGED;
    win->_flags &= ~(_WINCHANGED | _WINMOVED | _WINSDEL);
    return (OK);
}
