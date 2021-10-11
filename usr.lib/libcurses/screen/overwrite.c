/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)overwrite.c 1.1 92/07/30 SMI"; /* from S5R3 1.4 */
#endif

# include	"curses.ext"

# define	min(a,b)	((a) < (b) ? (a) : (b))

/*
 *	This routine writes Srcwin on Dstwin destructively.
 *
 */
int
overwrite(Srcwin, Dstwin)
register WINDOW	*Srcwin, *Dstwin;
{
	register int minx, miny;

# ifdef DEBUG
	if(outf) fprintf(outf, "OVERWRITE(0%o, 0%o);\n", Srcwin, Dstwin);
# endif
	miny = min(Srcwin->_maxy, Dstwin->_maxy) - 1;
	minx = min(Srcwin->_maxx, Dstwin->_maxx) - 1;
# ifdef DEBUG
	if(outf) fprintf(outf, "OVERWRITE:\tminx = %d,  miny = %d\n", minx, miny);
# endif
	return copywin(Srcwin, Dstwin, 0, 0, 0, 0, miny, minx, FALSE);
}
