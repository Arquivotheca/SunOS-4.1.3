#ifndef lint
static	char sccsid[] = "@(#)pnoutrfrsh.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/*
 * make the current screen look like "win" over the area covered by
 * win.
 *
 * 7/9/81 (Berkeley) @(#)refresh.c	1.6
 */

#include	"curses.ext"

extern	WINDOW *lwin;

/* Put out pad but don't actually update screen. */
pnoutrefresh(pad, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol)
register WINDOW	*pad;
int pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol;
{
	register int pr, r, c;
	register chtype	*nsp, *lch;

# ifdef DEBUG
	if(outf) fprintf(outf, "PREFRESH(pad %x, pcorner %d,%d, smin %d,%d, smax %d,%d)", pad, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol);
	_dumpwin(pad);
	if(outf) fprintf(outf, "PREFRESH:\n\tfirstch\tlastch\n");
# endif

	/* Make sure everything fits */
	if (pminrow < 0) pminrow = 0;
	if (pmincol < 0) pmincol = 0;
	if (sminrow < 0) sminrow = 0;
	if (smincol < 0) smincol = 0;
	if (smaxrow >= lines) smaxrow = lines-1;
	if (smaxcol >= columns) smaxcol = columns-1;
	if (smaxrow - sminrow > pad->_maxy - pminrow)
		smaxrow = sminrow + (pad->_maxy - pminrow);

	/* Copy it out, like a refresh, but appropriately offset */
	for (pr=pminrow,r=sminrow; r <= smaxrow; r++,pr++) {
		/* No record of what previous loc looked like, so do it all */
		lch = &pad->_y[pr][pad->_maxx-1];
		nsp = &pad->_y[pr][pmincol];
		_ll_move(r, smincol);
		for (c=smincol; nsp<=lch; c++) {
			if (SP->virt_x++ < columns && c <= smaxcol)
				*SP->curptr++ = *nsp++;
			else
				break;
		}
		pad->_firstch[pr] = _NOCHANGE;
	}
	lwin = pad;
	return OK;
}
