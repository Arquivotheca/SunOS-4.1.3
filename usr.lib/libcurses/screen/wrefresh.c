/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)wrefresh.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.29 */
#endif

#include	"curses_inc.h"

/*
 * Make the screen look like "win" over the area covered by win.
 * This routine may use insert/delete char/line and scrolling-region.
 * win : the window being updated
 */

extern int	outchcount;

void	_updateln();

static	short	cy, cx,		/* current cursor coord */
		scrli,		/* actual screen lines */
		scrco;		/* actual screen columns */
static	char	**marks;	/* the mark table for cookie terminals */
#define	_ISMARK1(y, x)	(marks[y][x / BITSPERBYTE] & (1 << (x % BITSPERBYTE)))

#ifdef	_VR2_COMPAT_CODE
extern	char	_endwin;
#endif	/* _VR2_COMPAT_CODE */

wrefresh(win)
WINDOW	*win;
{
    extern	int	_outch();
    register	short	*bnsch, *ensch;
    register	SLK_MAP	*slk;
    int		wx, wy, nc, boty, clby, idby, *hs, curwin;

    curwin = (win == curscr);

    /* don't allow curscr refresh if the screen was just created */

    if (curwin && curscr->_sync)
	return (OK);

    /* go thru _stdbody */
    if (!curwin && (win != _virtscr))
	(void) wnoutrefresh(win);

    /* if there is typeahead and we're not forcing update */
    if (!cur_term->_forceupdate && (_INPUTPENDING = _chkinput()) == TRUE)
    {
	if (curwin)
	    curscr->_clear = TRUE;
	return (OK);
    }

    if (curwin || curscr->_clear)
	_virtscr->_clear = TRUE;

    /* save curscr cursor coordinates */
    cy = curscr->_cury;
    cx = curscr->_curx;

    /* to simplify code in some cases */
    marks = _MARKS;
    scrli = curscr->_maxy;
    scrco = curscr->_maxx;
    slk = SP->slk;

    outchcount = 0;

    /* make sure we're in program mode */
    if (SP->fl_endwin)
    {
	/* If endwin is equal to 2 it means we just did a newscreen. */
	if (SP->fl_endwin == TRUE)
	{
	    reset_prog_mode();
	    if (SP->kp_state)
		tputs(keypad_xmit, 1, _outch);
	    if (slk)
		(*_do_slk_tch)();
	    if (SP->fl_meta)
		tputs(meta_on, 1, _outch);
	    if (cur_term->_cursorstate != 1)
		_PUTS(cur_term->cursor_seq[cur_term->_cursorstate], 0);
	}
	_PUTS(enter_ca_mode, 1);
	tputs(ena_acs, 1, _outch);

	if (exit_attribute_mode)
	    _PUTS(exit_attribute_mode, 1);
	else
	    /*
	     * If there is no exit_attribute mode, then vidupdate
	     * could only possibly turn off one of the below three
	     * so that's all we ask it turn off.
	     */
	    vidupdate(A_NORMAL, (A_ALTCHARSET | A_STANDOUT | A_UNDERLINE), _outch);

	SP->fl_endwin = FALSE;

#ifdef	_VR2_COMPAT_CODE
	_endwin = (char) FALSE;
#endif	/* _VR2_COMPAT_CODE */
    }

    /* clear the screen if required */
    if (_virtscr->_clear)
    {
	_PUTS(clear_screen, scrli);
	cy = cx = curscr->_curx = curscr->_cury = 0;

	/* _sync indicates that this a new screen */
	if (!curscr->_sync)
	    (void) werase(curscr);
	else
	{
	    nc = scrco / BITSPERBYTE - (scrco % BITSPERBYTE ? 0 : 1);
	    wy = scrli - 1;
	    bnsch = _BEGNS; ensch = _ENDNS;
	    hs = _CURHASH;
	    for (; wy >= 0; --wy)
	    {
		*bnsch++ = scrco;
		*ensch++ = -1;
		*hs++ = 0;
		if (marks)
		    for(wx = nc; wx >= 0; --wx)
			marks[wy][wx] = 0;
	    }
	}

	_virtscr->_clear = curscr->_sync = curscr->_clear = FALSE;
	if (slk)
	    (*_do_slk_tch)();

	/* pretend _virtscr has been totally changed */
	(void) wtouchln(_virtscr, 0, scrli, -1);
	_VIRTTOP = 0;
	_VIRTBOT = scrli - 1;

	/* will not do clear-eod or ins/del lines */
	clby = idby = scrli;
    }
    else
	clby = idby = -1;

    /* Software soft labels; if _changed == 2, slk's are in clear mode. */
    if (slk && slk->_win && (slk->_changed == TRUE))
	(*_do_slk_noref)();

    /* do line updating */
    _virtscr->_clear = FALSE;
    wy = _VIRTTOP;
    boty = _VIRTBOT + 1;
    bnsch = _virtscr->_firstch + wy;
    ensch = _virtscr->_lastch + wy;

    for(; wy < boty; ++wy, ++bnsch, ++ensch)
    {
	/* this line is up-to-date */
	if (*bnsch >= scrco)
	    goto next;

	/* we're not forcing update and there is type-ahead */
	if (!curwin && !cur_term->_forceupdate &&
	    (_INPUTPENDING = _chkinput()) == TRUE)
	{
	    _VIRTTOP = wy;
	    goto done;
	}

	if (clby < 0)
	{
	    /* now we have to work, check for ceod */
	    clby = _getceod(wy, boty);

	    /* check for insert/delete lines */
	    if (_virtscr->_use_idl)
		idby = (*_setidln)(wy, boty);
	}

	/* try clear-to-eod */
	if (wy == clby)
	    _useceod(wy, boty);

	/* try ins/del lines */
	if (wy == idby)
	{
	    curscr->_cury = cy;
	    curscr->_curx = cx;
	    (*_useidln)();
	    cy = curscr->_cury;
	    cx = curscr->_curx;
	}

	if (*bnsch < scrco)
	    _updateln(wy);

next:
	*bnsch = _INFINITY;
	*ensch = -1;
    }

    /* do hardware soft labels; if _changed == 2, slk's are in clear mode. */
    if (slk && (slk->_changed == TRUE) && !(slk->_win))
	(*_do_slk_ref)();

    /* move cursor */
    wy = _virtscr->_cury;
    wx = _virtscr->_curx;
    if (wy != cy || wx != cx)
    {
	(void) mvcur(cy, cx, wy, wx);
	cy = wy;
	cx = wx;
    }

    /* reset the flags */
    curscr->_clear = FALSE;
    _virtscr->_use_idl = FALSE;
    _virtscr->_use_idc = TRUE;
    _INPUTPENDING = FALSE;

    /* virtual image is now up-to-date */
    _VIRTTOP = scrli;
    _VIRTBOT = -1;

done :
    curscr->_cury = cy;
    curscr->_curx = cx;
    (void) fflush(SP->term_file);
    return (outchcount);
}

/* Shift appropriate portions of a line to leave space for cookies. */

static	chtype	*
_shove(wy)
register	int	wy;
{
    register	chtype	*wcp, *cp, prev;
    register	int	x, curx, cury, didshift;
    static	chtype	*line;
    static	int	length;

    /* allocate space for shifted line */
    if (length < scrco)
    {
	if (line)
	    free((char *) line);
	line = (chtype *) malloc(scrco * sizeof(chtype));
	length = line ? scrco : 0;
    }

    /* no space to do it */
    if (!line)
	return (_virtscr->_y[wy]);

    prev = A_NORMAL;
    cp = line;
    wcp = _virtscr->_y[wy];
    curx = _virtscr->_curx;
    cury = _virtscr->_cury;
    didshift = FALSE;

    for (x = 0; x < scrco; ++x, ++wcp, ++cp)
    {
	if (_ATTR(*wcp) != prev)
	{
	    /* use existing blank */
	    if (_CHAR(*wcp) == ' ')
		*cp = ' ' | _ATTR(*(wcp + 1));
	    /* use previous blank */
	    else
		if ((x > 0) && _CHAR(*(cp - 1)) == ' ')
		{
		    *(cp - 1) = ' ' | _ATTR(*wcp);
		    *cp = *wcp;
		}
		/* insert new blank */
		else
		{
		    if ((curx >= x) && (cury == wy))
			++curx;
		    *cp = ' ' | _ATTR(*wcp);
		    --wcp;
		    didshift = TRUE;
		}
	    prev = _ATTR(*cp);
	}
	else
	    *cp = *wcp;
    }

    /* make sure that the end of the line is normal */
    cp = line + scrco - 1;
    if (didshift || (_ATTR(*cp) != A_NORMAL) ||
	((wy == scrli - 1) && (_ATTR(*(cp - 1)) != A_NORMAL)))
    {
	*cp = didshift ? ' ' : _CHAR(*cp);
	if (wy == scrli - 1)
	    *(cp - 1) = didshift ? ' ' : _CHAR(*(cp - 1));
    }

    if (wy == cury)
	_virtscr->_curx = curx >= scrco ? scrco - 1 : curx;

    return (line);
}

/*
 * Update a line.
 * Three schemes of coloring are allowed. The first is the usual
 * pen-up/pen-down model. The second is the HP26*-like model.
 * In this case, colorings are specified by intervals, the left
 * side of the interval has the coloring mark, the right side
 * has the end-coloring mark. We assume that clear sequences will
 * clear ALL marks in the affected regions. The second case is
 * signified by the boolean flag ceol_standout_glitch.
 * The third case is for terminals that leave visible cookies on
 * the screen. This last case is at most an approximation of what
 * can be done right.
 */

static	void
_updateln(wy)
int	wy;
{
    extern	int	_outch();
    register	chtype	*wcp, *scp, *wp, *sp;
    register	int	wx, lastx, x;
    int			mtch, idch, blnkx, idcx, attrx, maxi, endns, begns;
    bool		redraw, changed, didattr;
    chtype		wc, sc, sa;

    redraw = (_virtscr->_firstch[wy] == _REDRAW);
    endns = _ENDNS[wy];
    begns = _BEGNS[wy];

    /* easy case */
    if (!redraw && (_virtscr->_lastch[wy] == _BLANK) && (begns >= scrco))
	return;

    /* line images */
    wcp = magic_cookie_glitch <= 0 ? _virtscr->_y[wy] : _shove(wy);
    scp = curscr->_y[wy];

    /* the interval to be updated */
    if (redraw || magic_cookie_glitch >= 0)
    {
	wx = 0;
	lastx = scrco;
    }
    else
    {
	wx = _virtscr->_firstch[wy];
	lastx = _virtscr->_lastch[wy] == _BLANK ? scrco : _virtscr->_lastch[wy] + 1;
    }

    /* skip equal parts */
    if (!redraw)
    {
	/* skip the starting equal part */
	wp = wcp + wx;
	sp = scp + wx;
	for (; wx < lastx; ++wx)
	    if (*wp++ != *sp++)
		break;
	if (wx >= lastx)
	    return;

	/* skip the ending equal part */
	wp = wcp + lastx - 1;
	sp = scp + lastx - 1;
	for (; lastx > wx; --lastx)
	    if (*wp-- != *sp--)
		break;
    }

    /* place to do clear-eol */
    if (!clr_eol || endns >= lastx)
	blnkx = scrco;
    else
	if (_virtscr->_lastch[wy] == _BLANK)
	    blnkx = -1;
	else
	{
	    for (blnkx = lastx - 1, wp = wcp + blnkx; blnkx >= wx; --blnkx, --wp)
		if (_DARKCHAR(*wp))
		    break;
	    if (blnkx + _COST(Clr_eol) >= lastx)
		blnkx = scrco;
	}

    /* on cookie terminals, we may need to do more work */
    if (marks)
    {
	if (blnkx < scrco)
	    attrx = blnkx + 1;
	else
	{
	    attrx = scrco;
	    wp = wcp + attrx - 1;
	    for (; attrx > lastx; --attrx, --wp)
		if (_ATTR(*wp) != A_NORMAL)
		    break;
	    lastx = attrx;
	    if (attrx >= scrco)
		--attrx;
	    if (magic_cookie_glitch > 0 && wy == scrli - 1 && attrx == scrco - 1)
		--attrx;
	}
    }

    /* place for insert/delete chars */
#define	SLACK	4
    if (redraw || (!SP->dchok && !SP->ichok) || !(_virtscr->_use_idc) ||
	endns < wx || (endns >= lastx && (scrco - lastx) > SLACK))
    {
	idcx = scrco;
    }
    else
	if (!marks)
	    idcx = -1;
	else
	{
	    /* on cookie term, only do idch where no attrs are used */
	    for (idcx = scrco - 1, wp = wcp + idcx; idcx >= wx; --idcx, --wp)
		if (_ATTR(*wp) || _ISMARK1(wy, idcx))
		    break;
	    if (idcx >= scrco - SLACK)
		idcx = scrco;
	}

    if (idcx < lastx && endns >= lastx)
	lastx = scrco;

    /* max amount of insert allow */
    if (idcx == scrco || !SP->ichok)
	maxi = 0;
    else
	if (lastx == scrco)
	    maxi = scrco;
	else
	    maxi = lastx - (endns + 1);

    /* go */
    wcp += wx;
    scp += wx;
    didattr = changed = FALSE;

    while (wx < lastx)
    {
	/* skip things that are already right */
	if (!redraw)
	{
	    for (; wx < lastx; ++wx, ++wcp, ++scp)
		if (*wcp != *scp)
		    break;
	    if (wx >= lastx)
		goto done;
	}

	/* try clear-bol, we'll assume exclusive clr_bol */
	if (!changed && !marks && clr_bol && blnkx > wx && begns >= wx)
	{
	    for (x = wx, wp = wcp; x < lastx; ++x, ++wp)
		if (_DARKCHAR(*wp))
		    break;
	    x -= 1;

	    if ((x - (redraw ? 0 : begns)) > _COST(Clr_bol))
	    {
		(void) mvcur(cy, cx, wy, x);
		_PUTS(clr_bol, 1);
		cy = wy;
		cx = x;

		mtch = x - wx;
		(void) memcpy((char *) scp, (char *) wcp, (int) (mtch * sizeof(chtype)));
		wcp += mtch;
		scp += mtch;
		wx = x;
	    }
	}

	/* screen image is changing */
	changed = TRUE;

	/* move to the point to start refresh */
	if (cy != wy || cx != wx)
	    (void) mvcur(cy, cx, wy, wx);
	cy = wy;
	cx = wx;

	/* update screen image */
	while (wx < lastx)
	{
	    wc = *wcp;
	    sc = *scp;

	    if (!redraw && wc == sc)
		break;

	    /* real video attributes */
	    if (marks)
		curscr->_attrs = _ATTR(sc);

	    /* blanks only */
	    if (wx > blnkx)
	    {
		_PUTS(clr_eol, 1);
		curscr->_curx = wx;
		curscr->_cury = wy;
		(void) wclrtoeol(curscr);

		if (marks && wx > 0 && _ATTR(*(scp - 1)) != A_NORMAL)
		{
		    _VIDS(A_NORMAL, _ATTR(*(scp - 1)));
		    _setmark(wy, wx, NULL);
		}
		goto done;
	    }

	    /* try insert/delete chars */
	    if (wx > idcx && (mtch = _useidch(wcp, scp, lastx - wx, maxi, &idch)))
	    {
		maxi -= idch;
		wx += mtch;
		scp += mtch;
		wcp += mtch;
		break;
	    }

	    /* about to output chars, make sure insert mode is off */
	    if (SP->phys_irm)
		_OFFINSERT();

	    /* video attributes */
	    if (_ATTR(wc) != curscr->_attrs)
	    {
		/* prevent spilling out of line */
		if (marks && !didattr)
		{
		    didattr = TRUE;
		    if (!_ISMARK1(wy, attrx))
		    {
			(void) mvcur(wy, wx, wy, attrx);
			sc = _ATTR(curscr->_y[wy][attrx]);
			sa = curscr->_attrs;
			_VIDS(sc, ~sc);
			_setmark(wy, attrx, NULL);
			(void) mvcur(wy, attrx, wy, wx);
			curscr->_attrs = sa;
		    }
		}

		_VIDS(_ATTR(wc), curscr->_attrs);

		/* mark the interval */
		if (marks)
		    _setmark(wy, wx, scp);
	    }

	    /* end-of-line */
	    if (wx == scrco - 1)
	    {
		_rmargin(wc);
		goto done;
	    }

	    if (transparent_underline && erase_overstrike && _CHAR(wc) == '_')
	    {
		_outch(' ');
		(void) mvcur(wy, wx + 1, wy, wx);
	    }

	    /* put out the character */
	    _outch(tilde_glitch && _CHAR(wc) == '~' ? '`' : wc);

	    *scp++ = wc;
	    wcp++;
	    wx++;
	    cx++;
	}
    }

done:
    if (changed)
    {
	/* update the blank structure */
	for (wx = 0, scp = curscr->_y[wy]; wx < scrco; ++wx, ++scp)
	    if (_DARKCHAR(*scp))
		break;
	_BEGNS[wy] = wx;
	if (wx == scrco)
	    _ENDNS[wy] = -1;
	else
	{
	    wx = scrco - 1;
	    scp = curscr->_y[wy] + wx;
	    for (; wx >= 0; --wx, --scp)
		if (_DARKCHAR(*scp))
		    break;
	    _ENDNS[wy] = wx;
	}

	/* update the hash structure */
	_CURHASH[wy] = _BEGNS[wy] < scrco ? _NOHASH : 0;
    }
    return;
}

/*
 * See if a left or right shift is apppropriate
 * This routine is called only if !cookie_glitch or no video attributes
 * are used in the affected part.
 * The main idea is to find a longest common substring which is a
 * prefix of one of 'wcp' or 'scp', then either delete or
 * insert depending on where the prefix is.
 *
 * wcp : what we want the screen to look like
 * scp : what the screen looks like now
 * length: the length to be updated
 * maxi: maximum possible insert amount
 * id; *id returns the amount of insert/delete
 *
 * Return the number of chars matched after the shift.
 */

static
_useidch(wcp, scp, length, maxi, id)
chtype	*wcp, *scp;
int	length, maxi, *id;
{
    extern	int	_outch();
    register	int	x1, x2, blnk;
    register	chtype	wc;
    int			idch, cost, cost_ich1, match;

    /* try deletion */
    if (SP->dchok && _CHAR(*wcp) != ' ')
    {
	if ((match = _prefix(wcp, scp, length, length / 2, &idch)) > 0)
	    cost = _COST(dcfixed) +
		(parm_dch ? _COST(Parm_dch) : _COST(Delete_character) * idch);
	else
	    cost = _INFINITY;

	if (match >= cost)
	{
	    if (SP->dmode)
	    {
		if (SP->sid_equal)
		{
		    if (!(SP->phys_irm))
			_ONINSERT();
		}
		else
		{
		    if (SP->phys_irm)
			_OFFINSERT();
		    _PUTS(enter_delete_mode, 1);
		}
	    }

	    if (parm_dch)
		_PUTS(tparm(parm_dch, idch), 1);
	    else
		for (x1 = 0; x1 < idch; ++x1)
		    _PUTS(delete_character, 1);

	    if (SP->dmode)
	    {
		if (SP->eid_equal)
		    SP->phys_irm = FALSE;
		_PUTS(exit_delete_mode, 1);
	    }

	    /* update screen image */
	    for (x1 = 0, x2 = idch; x2 < length; ++x1, ++x2)
		scp[x1] = scp[x2];
	    for (; x1 < length; ++x1)
		scp[x1] = ' ';

	    *id = -idch;
	    return (match);
	}
    }

    /* no insertion wanted or possible */
    if (!(SP->ichok) || _CHAR(*scp) == ' ')
	return (0);

    /* see if insertion is worth it */
    maxi = (idch = length / 2) < maxi ? idch : maxi;
    if ((match = _prefix(scp, wcp, length, maxi, &idch)) <= 0)
	return (0);

    /* see if inserting blanks only */
    for (blnk = 0; blnk < idch; ++blnk)
	if (wcp[blnk] != ' ')
	{
	    blnk = 0;
	    break;
	}

    /* see if doing insertion is worth it */
    cost_ich1 = idch * _COST(Insert_character);
    if (SP->imode)
    {
	cost = SP->phys_irm ? 0 : _COST(icfixed);
	if (blnk > _COST(Parm_ich) && _COST(Parm_ich) < cost_ich1)
	    cost += _COST(Parm_ich);
	else
	    if (insert_character)
		cost += cost_ich1;
    }
    else
    {
	if (parm_ich && _COST(Parm_ich) < cost_ich1)
	    cost = _COST(Parm_ich);
	else
	    cost = cost_ich1;
    }
    if ((cost - blnk) > match)
	return (0);

    /* perform the insertions */
    if (SP->imode)
    {
	if (!SP->phys_irm)
	    _ONINSERT();
	if (blnk > _COST(Parm_ich) && _COST(Parm_ich) < cost_ich1)
	    _PUTS(tparm(parm_ich, idch), 1);
	else
	    if (insert_character)
		goto do_insert_char;
	    else
		/* so that we'll do real char insertions */
		blnk = 0;
    }
    else
    {
	if (parm_ich && _COST(Parm_ich) < cost_ich1)
	    _PUTS(tparm(parm_ich, idch), 1);
	else
	{
do_insert_char:
	    for (x1 = 0; x1 < idch; ++x1)
	        _PUTS(insert_character, 1);
	}
    }

    /* inserting desired characters */
    if (!blnk)
	for(x1 = 0; x1 < idch; ++x1)
	{
	    wc = wcp[x1];
	    if (_ATTR(wc) != curscr->_attrs)
		_VIDS(_ATTR(wc),curscr->_attrs);
	    _outch(_CHAR(wc) == '~' && tilde_glitch ? '`' : wc);
	    ++cx;
	}

    /* update the screen image */
    for (x1 = length - 1, x2 = length - idch - 1; x2 >= 0; --x1, --x2)
	scp[x1] = scp[x2];
    (void) memcpy((char *) scp, (char *) wcp, idch * sizeof(chtype));

    *id = idch;
    return (match + idch);
}

/*
 * Find a substring of s2 that match a prefix of s1.
 * The substring is such that:
 * 	1. it does not start with an element
 *	   that is in perfect alignment with one in s1 and
 * 	2: it is at least as long as the displacement.
 *
 * length: the length of s1, s2.
 * maxs: only search for match in [1,maxs]  of s2.
 * begm: *begm returns where the match begins.
 *
 * Return the number of matches.
 */

static
_prefix(s1, s2, length, maxs, begm)
chtype	*s1, *s2;
int	length, maxs, *begm;
{
    register	int	m, n, k;

    n = 0;
    for (m = 1; m <= maxs; ++m)
	/* testing for s1[m] != s2[m] is condition 1 */
	if (s1[0] == s2[m] && s1[m] != s2[m])
	{
	    /* see if it's long enough (condition 2) */
	    for (k = 2 * m - 1; k > m; --k)
		if (s1[k - m] != s2[k])
		    break;
	    /* found a match with a good length */
	    if (k == m)
	    {
		*begm = m;

		/* count the # of matches */
		s2 += m;
		length -= m;
		for (n = m; n < length; ++n)
		    if (s1[n] != s2[n])
			break;
		goto done;
	    }
	}

done:
    return (n);
}

/* Set markers for cookie terminal. */

static
_setmark(y, x, s)
int	y, x;
chtype	*s;
{
    register	int	a;

    /* set the mark map */
    marks[y][x / BITSPERBYTE] |= (1 << (x % BITSPERBYTE));

    if (s)
    {
	a  = _ATTR(curscr->_attrs);

	/* set the video attr of the first char here */
	*s = _CHAR(*s) | a;

	/* now the video attr of the rest of the affected interval */
	for (x += 1, s += 1; x < scrco; ++x, ++s)
	    if (_ISMARK1(y, x))
		break;
	    else
		*s = _CHAR(*s) | a;
    }
}


/* At the right margin various weird things can happen.  We treat them here. */

static
_rmargin(wc)
chtype	wc;
{
    extern	int	_outch();
    register	int	x;
    chtype	sc;

    x = scrco - 1;

    /* screen may scroll */
    if (cy == scrli - 1)
    {
	/* can't do anything */
	if (!SP->ichok)
	    return;

	/* just one pos left of the bottom-right corner */
	sc = curscr->_y[cy][x - 1];

	(void) mvcur(cy, cx, cy, x - 1);
	_outch(tilde_glitch && _CHAR(wc) == '~' ? '`' : wc);

	/* insert sc back in and push wc right */
	(void) mvcur(cy, x, cy, x - 1);
	if (SP->imode && !SP->phys_irm)
	    _ONINSERT();
	if (insert_character)
	    _PUTS(insert_character, 1);
	else
	    if (parm_ich && !SP->imode)
		_PUTS(tparm(parm_ich, 1), 1);
	if (_ATTR(sc) != curscr->_attrs)
	    _VIDS(_ATTR(sc), curscr->_attrs);
	_outch(tilde_glitch && _CHAR(sc) == '~' ? '`' : sc);

	/* make sure the video attrs are ok */
	if (marks && (_ATTR(sc) || _ATTR(wc)))
	    _VIDS(_ATTR(wc), ~_ATTR(sc));

	/* update screen image */
	cx = x;
	curscr->_y[cy][x] = wc;
	return;
    }

    /* put char out and update screen image */
    _outch(tilde_glitch && _CHAR(wc) == '~' ? '`' : wc);
    curscr->_y[cy][x] = wc;

    /* make sure that wrap-around happens */
    if (!auto_right_margin || eat_newline_glitch)
    {
	_outch('\r');
	_outch('\n');
    }
    cx = 0;
    ++cy;
}

/*
 * Find the top-most line to do clear-to-eod.
 *
 * topy, boty: the region to consider
 */

static
_getceod(topy, boty)
int	topy, boty;
{
    register	chtype	*wcp, *ecp;
    register	int	wy;
    short		*begch, *endch, *begns;

    /* do nothing */
    if ((topy + 1) >= boty)
	return (boty);

    wy = boty - 1;
    begch = _virtscr->_firstch + wy;
    endch = _virtscr->_lastch + wy;
    begns = _BEGNS + wy;

    for (; wy >= topy; --wy, --begch, --endch, --begns)
    {
	if (*endch == _BLANK || (*begch >= scrco && *begns >= scrco))
	    continue;

	wcp = _virtscr->_y[wy];
	ecp = wcp + scrco;
	for (; wcp < ecp; ++wcp)
	    if (_DARKCHAR(*wcp))
		break;
	if (wcp != ecp)
	    break;

	*endch = _BLANK;
    }

    return (wy + 1);
}

/* Use hardware clear-to-bottom. */

static
_useceod(topy, boty)
int	topy, boty;
{
    extern	int	_outch();
    register	short	*begns, *begch;

    /* skip lines already blanked */
    begch = _virtscr->_firstch + topy;
    begns = _BEGNS + topy;
    for (; topy < boty; ++topy, ++begns, ++begch)
	if (*begns < scrco || *begch == _REDRAW)
	    break;
	else
	    *begch = _INFINITY;

    /* nothing to do */
    if (topy + 1 >= boty)
	return;

    /* see if bottom is clear */
    for (begns = _BEGNS + boty; boty < scrli; ++boty, ++begns)
	if (*begns < scrco)
	    return;

    /* use clear-screen if appropriate */
    if (topy == 0)
    {
	_PUTS(clear_screen, scrli);
	cy = 0; cx = 0;
	(void) werase(curscr);
    }

    /* use clear-to-end-of-display or delete lines */
    else
	if (clr_eos || (parm_delete_line && !memory_below))
	{
	    (void) mvcur(cy, cx, topy, 0);
	    cy = topy;
	    cx = 0;
	    _PUTS(clr_eos ? clr_eos : tparm(parm_delete_line, scrli - topy),
		    scrli - topy);

	    /* update curscr */
	    curscr->_cury = topy;
	    (void) wclrtobot(curscr);
	}

    /* no hardware support */
	else
	    return;

    /* correct the update structure */
    (void) wtouchln(_virtscr, topy, scrli, FALSE);
}
