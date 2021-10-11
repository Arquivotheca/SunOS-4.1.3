#ifndef lint
static	char sccsid[] = "@(#)_ll_move.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

extern	struct	line	*_line_alloc();

/*
 * _ll_move positions the cursor at position (row,col)
 * in the virtual screen
 */
_ll_move (row, col)
register int row, col; 
{
	register struct line *p;
	register int l;
	register chtype *b1, *b2;
	register int rp1 = row+1;

#ifdef DEBUG
	if(outf) fprintf(outf, "_ll_move(%d, %d)\n", row, col);
#endif
	if (SP->virt_y >= 0 && (p=SP->std_body[SP->virt_y+1]) &&
		p->length < SP->virt_x)
		p->length = SP->virt_x >= columns ? columns : SP->virt_x;
	SP->virt_x = col;
	SP->virt_y = row;
	if (row < 0 || col < 0)
		return;
	if (!SP->std_body[rp1] || SP->std_body[rp1] == SP->cur_body[rp1]) {
		p = _line_alloc ();
		if (SP->cur_body[rp1]) {
			p->length = l = SP->cur_body[rp1]->length;
			b1 = &(p->body[0]);
			b2 = &(SP->cur_body[rp1]->body[0]);
			for ( ; l>0; l--)
				*b1++ = *b2++;
		}
		SP->std_body[rp1] = p;
	}
	p = SP->std_body[rp1];
	p -> hash = 0;
	while (p -> length < col)
		p -> body[p -> length++] = ' ';
	SP->curptr = &(SP->std_body[rp1] -> body[col]);
}
