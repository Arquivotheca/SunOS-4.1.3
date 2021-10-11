/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)slk_start.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.4 */
#endif

#include	"curses_inc.h"

/*
 * Initialize usage of soft labels
 * This routine should be called before each call of newscreen
 * or initscr to initialize for the next terminal.
 *
 * ng:	number of groupings. If gp is NULL, it denotes one
 * 	of two default groupings:
 * 	0:	- - -   - -   - - -
 * 	1:      - - - -     - - - -
 * gp:	groupings.
 */

static	void	_init_slk_func();
static	char	_ngroups, _groups[LABMAX];
extern	int	_slk_update(), slk_touch(), slk_noutrefresh();

slk_start(ng, gp)
int	ng, *gp;
{
    register	int	i = 0, j = 0;

    if (gp == NULL)
    {
	switch (ng)
	{
	    case 2 :
		_ngroups = 2;
		_groups[0] = 4;
		_groups[1] = 4;
		break;

	    case 3 :
no_format :
		_ngroups = 3;
		_groups[0] = 3;
		_groups[1] = 2;
		_groups[2] = 3;
		break;

	    default :
		if (label_format)
		{
		    int		k;
		    char	ch1[3], *ch = label_format;

		    while (TRUE)
		    {
			if ((*ch == ',') || (*ch == '\0'))
			{
			    ch1[i] = '\0';
			    if ((k = atoi(ch1)) <= 0)
				goto err;
			    _groups[j++] = (char) k;
			    i = 0;
			    if (*ch == '\0')
			    {
				ng = j;
				break;
			    }
			}
			else
			    ch1[i++] = *ch++;
		    }
		}
		else
		    goto no_format;
		break;
	}
    }
    else
    {
	for (; i < ng; i++)
	{
	    if ((j += gp[i]) > LABMAX)
err :
		return (ERR);
	    _groups[i] = gp[i];
	}
	_ngroups = ng;
    }

    /* signal newscreen() */
    _slk_init = _init_slk_func;
    return (OK);
}

static	void
_init_slk_func()
{
    register	int	i, len, num = 0;
    register	SLK_MAP	*slk;
    register	char	*cp, *ep;
    register	WINDOW	*win;

    /* clear this out to ready for next time */
    _slk_init = NULL;

    /* get space for slk structure */
    if ((slk = (SLK_MAP *) malloc(sizeof(SLK_MAP))) == NULL)
    {
	curs_errno = CURS_BAD_MALLOC;
#ifdef	DEBUG
	strcpy(curs_parm_err, "_init_slk_func");
#endif	/* DEBUG */
	return;
    }

    /* compute actual number of labels */
    num = 0;
    for (i = 0; i < _ngroups; i++)
	num += _groups[i];

    /* max label length */
    if (plab_norm && (label_height * label_width >= LABLEN) && (num_labels >= num))
    {
	win = NULL;
	goto next;
    }
    else
    {
	if ((win = newwin(1, COLS, LINES - 1, 0)) == NULL)
	    goto err;
	win->_leave = TRUE;
	wattrset(win, A_REVERSE | A_DIM);

	    /* remove one line from the screen */
	LINES = --SP->lsize;
	if ((len = (COLS - 1) / (num + 1)) > LABLEN)
	{
next :
	    len = LABLEN;
	}
    }

    /* positions to place labels */
    if (len <= 0 || num <= 0 || (_slk_setpos(len, slk->_labx) == ERR))
    {
	if (win != NULL)
	    (void) delwin (win);
err :
	free(slk);
    }
    else
    {
	slk->_num = num;
	slk->_len = len;

	for (i = 0; i < num; ++i)
	{
	    cp = slk->_ldis[i];
	    ep = cp + len;
	    for (; cp < ep; ++cp)
		*cp = ' ';
	    *ep = '\0';
	    slk->_lval[i][0] = '\0';
	    slk->_lch[i] = TRUE;
	}

	slk->_changed = TRUE;
	slk->_win = win;

	_do_slk_ref = _slk_update;
	_do_slk_tch = slk_touch;
	_do_slk_noref = slk_noutrefresh;

	SP->slk = slk;
    }
}


/*
 * Compute placements of labels. The general idea is to spread
 * the groups out evenly. This routine is designed for the day
 * when > 8 labels and other kinds of groupings may be desired.
 *
 * The main assumption behind the algorithm is that the total
 * # of labels in all the groups is <= LABMAX.
 *
 * len: length of a label
 * labx: to return the coords of the labels.
 */

static
_slk_setpos(len, labx)
int	len;
short	*labx;
{
    register	int	i, k, n, spread, left, begadd;
    int		grpx[LABMAX];

    /* compute starting coords for each group */
    grpx[0] = 0;
    if (_ngroups > 1)
    {
	/* spacing between groups */
	for (i = 0, n = 0; i < _ngroups; ++i)
	    n += _groups[i] * (len + 1) - 1;
	if ((spread = (COLS - (n + 1))/(_ngroups - 1)) <= 0)
	    return (ERR);
	left = (COLS-(n + 1)) % (_ngroups - 1);
	begadd = (_ngroups / 2) - (left / 2);

	/* coords of groups */
	for (i = 1; i < _ngroups; ++i)
	{
	    grpx[i] = grpx[i - 1] + (_groups[i - 1] * (len + 1) - 1) + spread;
	    if (left > 0 && i > begadd)
	    {
		grpx[i]++;
		left--;
	    }
	}
    }

    /* now set coords of each label */
    n = 0;
    for (i = 0; i < _ngroups; ++i)
	for (k = 0; k < _groups[i]; ++k)
	    labx[n++] = grpx[i] + k * (len + 1);
    return (OK);
}
