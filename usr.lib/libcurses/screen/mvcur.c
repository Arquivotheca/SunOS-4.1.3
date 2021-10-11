/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)mvcur.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.4.1.17 */
#endif

#include	"curses_inc.h"

/*
 * Cursor motion optimization routine.  This routine takes as parameters
 * the screen positions that the cursor is currently at, and the position
 * you want it to be at, and it will move the cursor there very
 * efficiently.  It isn't really optimal, since several approximations
 * are taken in the interests of efficiency and simplicity.  The code
 * here considers directly addressing the cursor, and also considers
 * local motions using left, right, up, down, tabs, backtabs, vertical
 * and horizontal addressing, and parameterized motions.  It does not
 * consider using home down, or taking advantage of automatic margins on
 * any of the four directions.  (Two of these directions, left and right,
 * are well defined by the am and bw capabilities, but up and down are
 * not defined, nor are tab or backtab off the ends.)
 *
 * General strategies considered:
 *	CA	Direct Cursor Addressing
 *	LM	Local Motions from the old position
 *	HR	Home + Local Motions from upper left corner
 *	HDR	Home Down + Local Motions from lower left corner
 *	CR	CR + Local Motions from left margin
 *
 * Local Motions can include
 *	Up	cuu, cuu1, vpa
 *	Down	cud, cud1, vpa
 *	Left	cul, cul1, hpa, bs, cbt
 *	Right	cuf, cuf1, hpa, tab, char moved over
 */

/* This is called _ISMARK2 so it doesn't conflict with _ISMARK1 in wrefresh.c */

#define	_ISMARK2(x)	(mks[(x) / BITSPERBYTE] & (1<<((x) % BITSPERBYTE)))

#define	H_UP	-1
#define	H_DO	1

static	short	Newy;

mvcur(cury, curx, newy, newx)
int	cury, curx, newy, newx;
{
    extern	int	_outch();
    register	int	hu,		/* cost home + relative */
			hd,		/* cost home-down + relative */
			rl,		/* cost relative */
			cm;		/* cost direct cursor motion */

    /* obvious case */
    if (cury == newy && curx == newx)
	goto good;

    /* not in the right mode for cursor movement */
    if (SP->fl_endwin)
	goto bad;

    if (!move_standout_mode && curscr->_attrs && !SP->_mks)
	_VIDS(A_NORMAL, curscr->_attrs);

    if (!move_insert_mode && SP->phys_irm)
   	_OFFINSERT();

    /* absolute movement only */
    if (cury < 0)
	if (cursor_address)
	{
	    _PUTS(tparm(cursor_address, newy, newx), 1);
	    goto good;
	}
	else
	    goto bad;

    Newy = newy;

    /* cost of using cm */
    cm = _COST(Cursor_address);

    /* baudrate optimization */
    if (cm < LARGECOST && SP->baud >= 2400)
    {
	if (cursor_down && (newy == (cury + 1)) &&
	    ((newx == curx) || (newx == 0 && carriage_return)))
	{
	    if (newx != curx) 
		_PUTS(carriage_return, 1);
	    _PUTS(cursor_down, 1);
	    goto done;
	}

	if (cury != newy || newx < curx-4 || newx > curx+4)
	    rl = hu = hd = LARGECOST;
	    /* fast horizontal move */
	else
	{
	    if (newx < curx)
		rl = _mvleft(curx, newx, FALSE);
	    else
		rl = _mvright(curx, newx, FALSE);
	    if (rl < cm)
	    {
		if (newx < curx)
		    rl = _mvleft(curx, newx, TRUE);
		else
		    rl = _mvright(curx, newx, TRUE);
		goto done;
	    }
	}
    }
    else
    {
	/* cost using relative movements */
	if (cury < 0 || cury >= curscr->_maxy || curx < 0 || curx >= curscr->_maxx)
	    rl = LARGECOST;
	else
	    rl = _mvrel(cury, curx, newy, newx, FALSE);

	/* cost of homing to upper-left corner first */
	hu = cursor_home ? _homefirst(newy, newx, H_UP, FALSE) : LARGECOST;

	/* cost of homing to lower-left corner first */
	hd = cursor_to_ll ? _homefirst(newy, newx, H_DO, FALSE) : LARGECOST;
    }

    /* can't do any one of them */
    if (cm >= LARGECOST && rl >= LARGECOST && hu >= LARGECOST && hd >= LARGECOST)
bad:
	return (ERR);

    /* do the best one */
    if (cm <= rl && cm <= hu && cm <= hd)
	_PUTS(tparm(cursor_address, newy, newx), 1);
    else
	if (rl <= hu && rl <= hd)
	    (void) _mvrel(cury, curx, newy, newx, TRUE);
	else
	    (void) _homefirst(newy, newx, hu <= hd ? H_UP : H_DO, TRUE);

done:
    /* update cursor position */
    curscr->_curx = newx;
    curscr->_cury = newy;

good:
    return (OK);
}

/* Move by homing first. */

static
_homefirst(ny, nx, type, doit)
int	ny, nx, type, doit;
{
    extern	int	_outch();
    register	char	*home;
    register	int	cy, cost;

    if (type == H_UP)
    {
	home = cursor_home;
	cost = _COST(Cursor_home);
	cy = 0;
    }
    else
    {
	home = cursor_to_ll;
	cost = _COST(Cursor_to_ll);
	cy = curscr->_maxy - 1;
    }

    if (!home)
	return (LARGECOST);
    if (!doit)
	return (cost + _mvrel(cy, 0, ny, nx, FALSE));

    _PUTS(home, 1);
    return (_mvrel(cy, 0, ny, nx, TRUE));
}

/* Move relatively */

static
_mvrel(cy, cx, ny, nx, doit)
int	cy, cx, ny, nx, doit;
{
    register	int	cv, ch;

    /* do in this order since _mvhor may need the curscr image */
    cv = _mvvert(cy, ny, doit);
    ch = _mvhor(cx, nx, doit);

    return (cv + ch);
}

/* Move vertically */

static
_mvvert(cy, ny, doit)
int	cy, ny, doit;
{
    extern	int	_outch();
    register	char	*ve;
    register	int	dy, st_1, st_n, cv;

    if (cy == ny)
	goto out;

    /* cost of stepwise movement */
    if (cy < ny)
    {
	dy = ny-cy;
	st_1 = _COST(Cursor_down) * dy;
	st_n = _COST(Parm_down_cursor);
    }
    else
    {
	dy = cy-ny;
	st_1 = _COST(Cursor_up) * dy;
	st_n = _COST(Parm_up_cursor);
    }

    /* cost of using vertical move */
    cv = _COST(Row_address);

    /* if calculating cost only */
    if (!doit)
	return ((cv < st_1 && cv < st_n) ? cv : (st_n < st_1) ? st_n : st_1);

    /* do it */
    if (cv < st_1 && cv < st_n)
	_PUTS(tparm(row_address, ny), 1);
    else
	if (st_n < st_1)
	{
	    if (cy < ny)
		_PUTS(tparm(parm_down_cursor, dy), 1);
	    else
		_PUTS(tparm(parm_up_cursor, dy), 1);
	}
	else
	{
	    if (cy < ny)
		ve = cursor_down;
	    else
		ve = cursor_up;
	    for (; dy > 0; --dy)
		_PUTS(ve, 1);
	}

out:
    return (0);
}

/* Move horizontally */

static
_mvhor(cx, nx, doit)
int	cx, nx, doit;
{
    extern	int	_outch();
    register	int	st, ch, hl;

    if (cx == nx)
	goto out;

    /* cost using horizontal move */
    ch = _COST(Row_address);

    /* cost doing stepwise */
    st = cx < nx ? _mvright(cx, nx, FALSE) : _mvleft(cx, nx, FALSE);

    /* cost homeleft first */
    hl = (_COST(Carriage_return) < LARGECOST) ?
	_COST(Carriage_return) + _mvright(0, nx, FALSE) : LARGECOST;

    if (!doit)
	return ((ch < st && ch < hl) ? ch : (hl < st ? hl : st));

    if (ch < st && ch < hl)
	_PUTS(tparm(column_address, nx), 1);
    else
	if (hl < st)
	{
	    _PUTS(carriage_return, 1);
	    (void) _mvright(0, nx, TRUE);
	}
	else
	{
	    if (cx < nx)
		(void) _mvright(cx, nx, TRUE);
	    else
		(void) _mvleft(cx, nx, TRUE);
	}
out:
    return (0);
}

/* Move right. */

static
_mvright(cx, nx, doit)
int	cx, nx, doit;
{
    extern	int	_outch();
    register	chtype	*scp;
    register	char	*mks;
    register	int	nt, tx, x, stcost;

    if (!cursor_right && !parm_right_cursor)
	return (LARGECOST);

    scp = curscr->_y[Newy];
    mks = magic_cookie_glitch >= 0 ? SP->_mks[Newy] : NULL;

    if (cursor_right)
    {
	/* number of tabs used in stepwise movement */
	nt = tab ? (nx / TABSIZE - cx / TABSIZE) : 0;
	tx = (nt > 0) ? (cx / TABSIZE + nt) * TABSIZE : cx;

	/* calculate stepwise cost */
	stcost = nt * _COST(Tab);
	for (x = tx; x < nx; ++x)
	    if ((!ceol_standout_glitch && !mks &&
		_ATTR(scp[x]) == curscr->_attrs) ||
		ceol_standout_glitch || (mks && !_ISMARK2(x)))
	    {
		stcost += 1;
	    }
	    else
		stcost += _COST(Cursor_right);
    }
    else
	stcost = LARGECOST;

    if (!doit)
	return ((_COST(Parm_right_cursor) < stcost) ? _COST(Parm_right_cursor) : stcost);

    /* actually move */
    if (_COST(Parm_right_cursor) < stcost)
	_PUTS(tparm(parm_right_cursor, nx-cx), 1);
    else
    {
	if (SP->phys_irm)
	    _OFFINSERT();
	for (; nt > 0; --nt)
	    _PUTS(tab, 1);
	for (x = tx; x < nx; ++x)
	    if ((!ceol_standout_glitch && !mks &&
		_ATTR(scp[x]) == curscr->_attrs) ||
		ceol_standout_glitch || (mks && !_ISMARK2(x)))
	    {
		_outch(_CHAR(scp[x]));
	    }
	    else
		_PUTS(cursor_right, 1);
    }

    return (0);
}

/* Move left */

static
_mvleft(cx, nx, doit)
int	cx, nx, doit;
{
    extern	int	_outch();
    register	int	tx, nt, x, stcost;

    if (!cursor_left && !parm_left_cursor)
	return (LARGECOST);

    if (cursor_left)
    {
	/* stepwise cost */
	tx = cx;
	nt = 0;
	if (back_tab)
	{
	    /* the TAB position >= nx */
	    x = (nx % TABSIZE) ? (nx / TABSIZE + 1) * TABSIZE : nx;

	    /* # of tabs used and position after using them */
	    if (x < cx)
	    {
		nt = (cx / TABSIZE - x / TABSIZE) + ((cx % TABSIZE) ? 1 : 0);
		tx = x;
	    }
	}
	stcost = nt * _COST(Back_tab) + (tx-nx) * _COST(Cursor_left);
    }
    else
	stcost = LARGECOST;

    /* get cost only */
    if (!doit)
	return ((_COST(Parm_left_cursor) < stcost) ? _COST(Parm_left_cursor) : stcost);

    /* doit */
    if (_COST(Parm_left_cursor) < stcost)
	_PUTS(tparm(parm_left_cursor, cx - nx), 1);
    else
    {
	for (; nt > 0; --nt)
	    _PUTS(back_tab, 1);
	for (; tx > nx; --tx)
	    _PUTS(cursor_left, 1);
    }

    return (0);
}
