/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)slk_init.c 1.1 92/07/30 SMI"; /* from S5R3 1.2 */
#endif

/*
 * Initialize the screen to use soft labels. This call must be made BEFORE
 * initscr is called. Two formats of labels are available if the labels
 * are maintained on-screen rather than using the soft-labels that many
 * terminals have.
 *
 * If we got rid of m_newterm(), then these routines and the global variables
 * _use_slk and _ripstruct could be moved inside of newterm.c and made static.
 */

#include "curses.ext"

slk_init(format)
int format;				/* 0 => 3-2-3, 1 => 4-4 */
{
    if (format < 0 || format > 1)
	format = 0;
    _use_slk = format + 1;
}

/*
 * This routine is the lower-level interface to actually initialize
 * the soft labels. It is called by newterm().
 */

static char blanks[] = "        ";

_slk_init()
{
    register struct slkdata *SLK = SP->slk;
    register int i, slklen;

    if (!SLK)
	return;

    SLK->fl_changed = TRUE;
    for (i=0; i<8; i++)
        {
	strcpy(SLK->label[i], blanks);
	SLK->nblabel[i][0] = '\0';
	SLK->scrlabel[i][0] = '\0';
	SLK->changed[i] = TRUE;
	}

    slklen = columns / 8 - 1;
    if (slklen > 8)
        slklen = 8;
    SLK->len = slklen;

    if (_use_slk == 1)			/* 1 => 3-2-3 */
        {
	SLK->offset[0] = 0;				/* 0 */
	SLK->offset[1] = slklen+1;			/* 9 */
	SLK->offset[2] = (slklen+1)*2;			/* 18 */
	SLK->offset[3] = (columns/2-slklen-1);		/* 31 */
	SLK->offset[4] = (columns/2);			/* 40 */
	SLK->offset[5] = columns-(slklen+1)*3;		/* 53 */
	SLK->offset[6] = columns-(slklen+1)*2;		/* 62 */
	SLK->offset[7] = columns-(slklen+1);		/* 71 */
	}
    else				/* 2 => 4-4 */
        {
	SLK->offset[0] = 0;				/* 0 */
	SLK->offset[1] = slklen+1;			/* 9 */
	SLK->offset[2] = (slklen+1)*2;			/* 18 */
	SLK->offset[3] = (slklen+1)*3;			/* 27 */
	SLK->offset[4] = columns-(slklen+1)*4;		/* 44 */
	SLK->offset[5] = columns-(slklen+1)*3;		/* 53 */
	SLK->offset[6] = columns-(slklen+1)*2;		/* 62 */
	SLK->offset[7] = columns-(slklen+1);		/* 71 */
	}

}

_slk_winit(window)
register WINDOW *window;
{
    register struct slkdata *SLK;
    register int i, slklen;

    if (!window)
	return;

    SLK = SP->slk;
    slklen = SLK->len;
    wattron(window, A_REVERSE|A_DIM);

    for (i=0; i<8; i++)
        mvwprintw(window, 0, SLK->offset[i], "%*s", slklen, blanks);

    wnoutrefresh(window);
}
