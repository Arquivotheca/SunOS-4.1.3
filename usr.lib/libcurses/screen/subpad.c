/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)subpad.c 1.1 92/07/30 SMI"; /* from S5R3 1. */
#endif

# include	"curses.ext"

extern WINDOW *_makenew();

/*
 *  'subpad' is like subwin but lets you make a subpad anywhere on
 *	the parent pad including off screen thus sidestepping the
 *	restrictions imposed by 'makenew'. Notice that the upper
 *	left corner of a subpad must be specified (unlike a simple
 *	pad). begy and begx are pad coordinates, not screen coords.
 */
WINDOW *
subpad(orig, num_lines, num_cols, begy, begx)
register WINDOW	*orig;
int	num_lines, num_cols, begy, begx;
{
	register int i;
	register WINDOW	*pad;
	register int by, bx, nlines, ncols;
	register int j, k;

	by = begy;
	bx = begx;
	nlines = num_lines;
	ncols = num_cols;

# ifdef	DEBUG
	if(outf) fprintf(outf, "SUBPAD(%0.2o, %d, %d, %d, %d)\n", orig, nlines, ncols, by, bx);
# endif

	/*
	 * make sure subpad fits inside the original one
	 */

	if ((!(orig->_flags & _ISPAD)) || 
	    by >= orig->_maxy ||
	    bx >= orig->_maxx)
		return NULL;

	if (nlines == 0)
		nlines = orig->_maxy - by;
	if (ncols == 0)
		ncols = orig->_maxx - orig->_begx - bx;

	if ((pad = _makenew(nlines, ncols, 0, 0)) == NULL)
		return NULL;
	pad->_flags |= (_ISPAD | _SUBWIN);
	pad->_yoffset = orig->_yoffset;
	pad->_pminrow = orig->_pminrow; pad->_pmincol = orig->_pmincol;
	pad->_sminrow = orig->_sminrow; pad->_smincol = orig->_smincol;
	pad->_smaxrow = orig->_smaxrow; pad->_smaxcol = orig->_smaxcol;

	j = by - orig->_begy;
	k = bx - orig->_begx;
	for (i = 0; i < nlines; i++)
		pad->_y[i] = &orig->_y[j++][k];
	return pad;
}
