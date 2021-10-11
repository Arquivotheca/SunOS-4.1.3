/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)idlok.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.15 */
#endif

#include	"curses_inc.h"


/* Functions to make use of insert/delete line caps */

#define	scrco	COLS

typedef	struct
	{
	    short	_wy,	/* matching lines */
			_sy;
	} IDST;
static	IDST	*id, *sid, *eid;	/* list of idln actions */
static	int	**mt;			/* the match length matrix */
static	int	idlen, scrli,		/* screen dimensions */
		cy, cx;			/* current cursor positions */
static	int	*hw, *hs;		/* the fingerprints of lines */
static	bool	didcsr;			/* scrolling region was used */
static	int	_hash_ln(), _use_idln(), _set_idln();
static	void	_find_idln();


/* Set insert/delete line mode for win */

idlok(win, bf)
WINDOW	*win;
bool	bf;
{
    _useidln = _use_idln;
    _setidln = _set_idln;

    SP->yesidln = (delete_line || parm_delete_line ||
	(change_scroll_region && (parm_index || scroll_forward))) &&
	(insert_line || parm_insert_line ||
	(change_scroll_region && (parm_rindex || scroll_reverse)));

    win->_use_idl = bf;
    return (OK);
}

/*
 * Set the places to do insert/delete lines
 * Return the start line for such action.
 */

static
_set_idln(topy, boty)
int	topy, boty;
{
    register	int	wy, bot, n;
    register	short	*begch, *endch, *begns, *endns;
    register	chtype	**wcp, **scp;

    if (!SP->yesidln || (topy+1) >= boty)
	return (curscr->_maxy);

    scrli = curscr->_maxy;

    /* set up space for computing idln actions */
    if (idlen < scrli)
    {
	if (mt && mt[0])
	    free((char *) mt[0]);
	if (mt)
	    free((char *) mt);
	if (id)
	    free(id);

	n = scrli + 1;
	if (!(mt = (int **) malloc(n * sizeof(int *))) ||
	    !(mt[0] = (int *) malloc(n * n * sizeof(int))) ||
	    !(id = (IDST *) malloc(scrli * sizeof(IDST))))
	{
	    if (mt && mt[0])
		free((char *) mt[0]);
	    if (mt)
		free((char *) mt);
	    idlen = 0;
	    return (scrli);
	}
	idlen = scrli;
	for (wy = 1; wy < n; ++wy)
	    mt[wy] = mt[wy - 1] + n;
	for (wy = 0; wy < n; ++wy)
	    mt[0][wy] = mt[wy][0] = 0;
    }

    /* compute fingerprints for fast line comparisons */
    begch = _virtscr->_firstch + topy;
    endch = _virtscr->_lastch + topy;
    wcp = _virtscr->_y + topy;
    hw = _VIRTHASH;

    begns = _BEGNS + topy;
    endns = _ENDNS + topy;
    scp = curscr->_y + topy;
    hs = _CURHASH;

    for (wy = topy; wy < boty; ++wy, ++begch, ++endch, ++wcp, ++begns, ++endns, ++scp)
    {
	if (*begch == _REDRAW || *begch == _INFINITY)
	    hw[wy] = _NOHASH;
	else
	    if (*endch == _BLANK)
		hw[wy] = 0;
	    else
		if (hw[wy] == _NOHASH)
		{
		    hw[wy] = _hash_ln(*wcp, *wcp + scrco - 1);
		    if (hw[wy] == 0)
			*endch = _BLANK;
		}

	if (hs[wy] == _NOHASH)
	    hs[wy] = _hash_ln(*scp + *begns, *scp + *endns);
    }

    /* go get them */
    sid = eid = id + scrli;
    while(topy < boty)
    {
	/* find a validly changed interval of lines */
	for (; topy < boty; ++topy)
	    if (hw[topy] != _NOHASH)
		break;
	for (bot = topy; bot < boty; ++bot)
	    if (hw[bot] == _NOHASH)
		break;

	/* skip starting and trailing matching blanks */
	for (wy = topy; wy < bot; ++wy)
	    if (hw[wy] != 0 || hs[wy] != 0)
		break;
	for (n = bot-1; n >= wy; --n)
	    if (hw[n] != 0 || hs[n] != 0)
		break;

	_find_idln(wy, n + 1);

	topy = bot + 1;
    }

    /*
     * The value we want to return is the lower line number of the top-most
     * range.
     *
     * If there is more than one range of lines on which we're operating,
     * _find_idln will get called more than once; in this case,
     * the desired return value is in (eid-1), not in sid.
     */
     {
             register IDST *idp;
             int rval = scrli;

             for (idp = sid; idp != eid; idp++) {
                     int tmp;

                     if ((tmp = _MIN(idp->_wy, idp->_sy)) < rval)
                             rval = tmp;
             }
             return rval;
     }
}

/*
 * Compute the largest set of matched lines.
 * This algorithm is an extended version of
 * the Longest Common Subsequence algorithm.
 * The idea is that we want the largest set of matches
 * (hence the LCS) for which the total insert/delete amount is minimum.
 */

static	void
_find_idln(top, bot)
int	top, bot;
{
    register	int	*mw, *m1, w, s, wy, sy, n, nn, hwv;

    if ((n = bot - top) <= 0)
	return;
    nn = n * n + n;

    /* compute the max match length matrix */
    for(w = 1, wy = top; w <= n; ++w, ++wy)
    {
	m1 = mt[w - 1];
	mw = mt[w] + 1;
	hwv = hw[wy];
	for (s = 1, sy = top; s <= n; ++s, ++sy, ++m1, ++mw)
	{
	    register	int	d, u, l;

	    /* diagonal value */
	    d = hwv != hs[sy] ? 0 : *m1 + nn - (s > w ? s - w : w - s);

	    /* up value */
	    u = *(m1 + 1);

	    /* left value */
	    l = *(mw - 1);

	    *mw = d > u ? _MAX(d, l) : _MAX(u, l);
	}
    }

    /* now find the matches */
    for(w = s = n, wy = sy = bot-1; w > 0 && s > 0; --w,--s, --wy,--sy)
    {
	for (mw = mt[w] + s; s > 0; --s, --sy, --mw)
	    if (*(mw - 1) < *mw)
		break;

	for (; w > 0; --w, --wy)
	    if (mt[w - 1][s] < mt[w][s])
		break;

	if (w > 0 && s > 0 && w != s)
	{
	    --sid;
	    sid->_wy = wy;
	    sid->_sy = sy;
	}
    }
}

/* Use hardware line delete/insert */

static
_use_idln()
{
    extern	int	_outch();
    register	int	tsy, bsy, idn, dir, nomore;
    IDST		*ip, *ep, *eip;

    cy = curscr->_cury;
    cx = curscr->_curx;
    didcsr = FALSE;

    /* first cycle do deletions, second cycle do insertions */
    for (dir = 1; dir > -2; dir -= 2)
    {
	if (dir > 0)
	{
	    ip = sid;
	    eip = eid;
	}
	else
	{
	    ip = eid - 1;
	    eip = sid - 1;
	}

	nomore = TRUE;
	while(ip != eip)
	{
	    /* skip deletions or insertions */
	    if ((dir > 0 && ip->_wy > ip->_sy) ||
		(dir < 0 && ip->_wy < ip->_sy))
	    {
		nomore = FALSE;
		ip += dir;
		continue;
	    }

	    /* find a contiguous block */
	    for (ep = ip+dir; ep != eip; ep += dir)
		if (ep->_wy != (ep - dir)->_wy + dir ||
		    ep->_sy != (ep - dir)->_sy + dir)
		{
		    break;
		}
	    ep -= dir;

	    /* top and bottom lines of the affected region */
	    if (dir > 0)
	    {
		tsy = _MIN(ip->_wy, ip->_sy);
		bsy = _MAX(ep->_wy, ep->_sy) + 1;
	    }
	    else
	    {
		tsy = _MIN(ep->_wy, ep->_sy);
		bsy = _MAX(ip->_wy, ip->_sy) + 1;
	    }

	    /* amount to insert/delete */
	    if ((idn = ip->_wy - ip->_sy) < 0)
		idn = -idn;

	    /* do the actual output */
	    _do_idln(tsy, bsy, idn, dir == -1);

	    /* update change structure */
	    (void) wtouchln(_virtscr, tsy, bsy - tsy, -1);

	    /* update screen image */
	    curscr->_tmarg = tsy;
	    curscr->_bmarg = bsy - 1;
	    curscr->_cury = tsy;
	    (void) winsdelln(curscr, dir > 0 ? -idn : idn);
	    curscr->_tmarg = 0;
	    curscr->_bmarg = scrli - 1;

	    /* for next while cycle */
	    ip = ep + dir;
	}

	if (nomore)
	    break;
    }

    /* reset scrolling region */
    if (didcsr)
    {
	_PUTS(tparm(change_scroll_region, 0, scrli - 1), scrli);
	(void) mvcur(-1, -1, cy, cx);
    }

    curscr->_cury = cy;
    curscr->_curx = cx;
    return (OK);
}

/* Do the actual insert/delete lines */

static
_do_idln(tsy, bsy, idn, doinsert)
int	tsy, bsy, idn, doinsert;
{
    extern	int	_outch();
    register	int	y, usecsr, yesscrl;
    register	short	*begns;

    /* change scrolling region */
    yesscrl = usecsr = FALSE;
    if (tsy > 0 || bsy < scrli)
    {
	if (change_scroll_region)
	{
	    _PUTS(tparm(change_scroll_region, tsy, bsy - 1), bsy - tsy);
	    (void) mvcur(-1, -1, tsy, 0);
	    cy = tsy;
	    cx = 0;
	    yesscrl = usecsr = didcsr = TRUE;
	}
    }
    else
    {
	if (didcsr)
	{
	    _PUTS(tparm(change_scroll_region, 0, scrli - 1), scrli);
	    (void) mvcur(-1, -1, cy, cx);
	    didcsr = FALSE;
	}
	yesscrl = TRUE;
    }

    if (doinsert)
    {
	/* memory below, clobber it now */
	if (memory_below && clr_eol &&
	    ((usecsr && non_dest_scroll_region) || bsy == scrli))
	{
	    for (y = bsy - idn, begns = _BEGNS + y; y < bsy; ++y, ++begns)
		if (*begns < scrco)
		{
		    (void) mvcur(cy, cx, y, 0);
		    cy = y;
		    cx = 0;
		    _PUTS(clr_eol, 1);
		}
	}

	/* if not change_scroll_region, delete, then insert */
	if (!usecsr && bsy < scrli)
	{
	    /* delete appropriate number of lines */
	    (void) mvcur(cy, cx, bsy - idn, 0);
	    cy = bsy - idn;
	    cx = 0;
	    if (parm_delete_line && (idn > 1 || !delete_line))
		_PUTS(tparm(parm_delete_line, idn), scrli - cy);
	    else
		for(y = 0; y < idn; ++y)
		    _PUTS(delete_line, scrli - cy);
	}

	/* now do insert */
	(void) mvcur(cy, cx, tsy, 0);
	cy = tsy;
	cx = 0;
	if (yesscrl)
	{
	    if (!parm_rindex &&
		(!scroll_reverse || (parm_insert_line && idn > 1)))
	    {
		goto hardinsert;
	    }
	    if (parm_rindex && (idn > 1 || !scroll_reverse))
		_PUTS(tparm(parm_rindex, idn), scrli - cy);
	    else
		for(y = 0; y < idn; ++y)
		    _PUTS(scroll_reverse, scrli - cy);
	}
	else
	{
hardinsert:
	    if (parm_insert_line && (idn > 1 || !insert_line))
		_PUTS(tparm(parm_insert_line, idn), scrli - cy);
	    else
		for(y = 0; y < idn; ++y)
		    _PUTS(insert_line, scrli - cy);
	}
    }
    else
    /* doing deletion */
    {
	/* memory above, clobber it now */
	if (memory_above && clr_eol &&
	    ((usecsr && non_dest_scroll_region) || tsy == 0))
	{
	    for (y = 0, begns = _BEGNS + y + tsy; y < idn; ++y, ++begns)
		if (*begns < scrco)
		{
		    (void) mvcur(cy, cx, tsy + y, 0);
		    cy = tsy + y;
		    cx = 0;
		    _PUTS(clr_eol, 1);
		}
	}

	if (yesscrl)
	{
	    if(!parm_index &&
		(!scroll_forward || (parm_delete_line && idn > 1)))
	    {
		goto harddelete;
	    }
	    (void) mvcur(cy, cx, bsy - 1, 0);
	    cy = bsy - 1;
	    cx = 0;
	    if (parm_index && (idn > 1 || !scroll_forward))
		_PUTS(tparm(parm_index, idn), scrli - cy);
	    else
		for(y = 0; y < idn; ++y)
		    _PUTS(scroll_forward, scrli - cy);
	}
	else
	{
harddelete:
	    /* do deletion */
	    (void) mvcur(cy, cx, tsy, 0);
	    cy = tsy;
	    cx = 0;
	    if (parm_delete_line && (idn > 1 || !delete_line))
		_PUTS(tparm(parm_delete_line, idn), scrli - cy);
	    else
		for(y = 0; y < idn; ++y)
		    _PUTS(delete_line, scrli - cy);
	}

	/* if not change_scroll_region, do insert to restore bottom */
	if (!usecsr && bsy < scrli)
	{
	    y = scrli - 1;
	    begns = _BEGNS + y;
	    for (; y >= bsy; --y, --begns)
		if (*begns < scrco)
		    break;
	    if (y >= bsy)
	    {
		(void) mvcur(cy, cx, bsy - idn, 0);
		cy = bsy - idn;
		cx = 0;
		if (parm_insert_line && (idn > 1 || !insert_line))
		    _PUTS(tparm(parm_insert_line, idn), scrli - cy);
		else
		    for(y = 0; y < idn; ++y)
			_PUTS(insert_line, scrli - cy);
	    }
	}
    }
}

/* Get the hash value of a line */

static
_hash_ln(sc, ec)
chtype	*sc, *ec;
{
    unsigned	int	hv;

    hv = 0;
    for(; sc <= ec; ++sc)
	if (*sc != _BLNKCHAR)
	    hv = (hv << 1) + ((unsigned) *sc);
    switch (hv)
    {
	case 0:
	    return (_THASH);
	case _NOHASH:
	    return (_THASH + 1);
	default:
	    return (hv);
    }
}
