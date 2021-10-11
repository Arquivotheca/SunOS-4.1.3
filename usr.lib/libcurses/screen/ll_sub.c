/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)ll_sub.c 1.1 92/07/30 SMI"; /* from S5R3 1.4.1.1 */
#endif

#include "curses.ext"

/*
 * _line_alloc returns a pointer to a new line structure.
 * One is malloc'd if none is available on the free list.
 */
struct line *
_line_alloc ()
{
	register struct line   *rv = SP->freelist;
	char *malloc();

#ifdef DEBUG
	if(outf) fprintf(outf, "mem: _line_alloc (), prev SP->freelist %x\n", SP->freelist);
#endif
	if (rv) {
		SP->freelist = rv -> next;
	} else {
#ifdef NONSTANDARD
		_ec_quit("No lines available in line_alloc", "");
#else
		rv = (struct line *) malloc (sizeof *rv);
		if (rv == NULL)
			(void) fprintf (stderr, "malloc returned NULL in _line_alloc \n");
		rv -> body = (chtype *) malloc (columns * sizeof (chtype));
		if (rv->body == NULL)
			(void) fprintf (stderr, "malloc returned NULL in _line_alloc \n");
#endif
	}
	rv -> length = 0;
	rv -> hash = 0;
#ifdef DEBUG
	if(outf) fprintf(outf, "_line_alloc (), rv= %x, rv->body= %x.\n",
		rv, rv->body);
#endif
	return rv;
}

/* '_line_free' returns a line object to the free list */
_line_free (p)
register struct line   *p;
{
	register int i, sl, n=0;
	register struct line **q;

	if (p == NULL)
		return;
#ifdef DEBUG
	if(outf) fprintf(outf, "mem: Releaseline (%x), prev SP->freelist %x", p, SP->freelist);
#endif
	/*
	 * see how many places this line is being used. only return
	 * it to the free list if it is only used once.
	 */
	sl = lines;
	for (i=sl,q = &SP->cur_body[sl]; i>0; i--,q--)
		if (p == *q)
			n++;
	for (i=sl,q = &SP->std_body[sl]; i>0; i--,q--)
		if (p == *q)
			n++;
#ifdef DEBUG
	if(outf) fprintf(outf, ", count %d\n", n);
#endif
	if (n > 1)
		return;
	p -> next = SP->freelist;
	p -> hash = -888;
	SP->freelist = p;
}

/*
 * update line length
 */
_ll_endmove()
{
	register struct line *p;
	register int newcol;

#ifdef DEBUG
	if(outf) fprintf(outf, "_ll_endmove()\n");
#endif

	if (SP->virt_y >= 0 && (p=SP->std_body[SP->virt_y+1]) &&
	    p->length < SP->virt_x) {
		newcol = SP->virt_x >= columns ?
			columns : SP->virt_x;
		if (newcol > p->length)
			p->length = newcol;
	}
}

/*
 * Validate row and col. Update virtx/y.
 */
_ll_setyx(row,col)
register int row, col;
{
	if (row > lines)
		row = lines;
	if (col > columns)
		col = columns;
	SP->virt_x = col;
	SP->virt_y = row;
}

/*
 * _ll_move positions the cursor at position (row,col)
 * in the virtual screen
 */
_ll_move (row, col)
register int row, col;
{
	register struct line *p, *cp;
	register int l;
	register int rp1;

#ifdef DEBUG
	if(outf) fprintf(outf, "_ll_move(%d, %d)\n", row, col);
#endif

	_ll_endmove();

	/*
	 * Validate row and col. Update virtx/y. This is identical code to
	 * _ll_setxy above, but done here to keep the new values of row & col.
	 */
	if (row > lines)
		row = lines;
	if (col > columns)
		col = columns;
	SP->virt_x = col;
	SP->virt_y = row;
	if (row < 0 || col < 0)
		return;

	/*
	 * allocate a line for std_body[row] and copy in info from cur_body[row]
	 */
	rp1 = row + 1;
	SP->fl_changed = TRUE;
	cp = SP->cur_body[rp1];
	if (SP->std_body[rp1] == cp || !SP->std_body[rp1])  {
		p = _line_alloc();
		if (cp) {
			p->length = l = cp->length;
			memcpy((char *)&(p->body[0]), (char *)&(cp->body[0]),
				l * (int)sizeof(chtype));
		}
		SP->std_body[rp1] = p;
	} else
		p = SP->std_body[rp1];
	p -> hash = 0;

	/*
	 * if virty is past end of row, set rest of row to blanks.
	 */
	l = col - p->length;
	if (l > 0) {
		memSset(&p->body[p->length], (chtype) ' ', l);
		p->length = col;
	}

	SP->curptr = &(p -> body[col]);
}

/*
 * This hash function is skewed to the right-most BITS characters on
 * the screen line (after leading and trailing blanks are removed from
 * consideration), where BITS is the number of bits in an int. Ignoring
 * the internal blanks within a line helps with boxed windows that have
 * short lines at the left hand side of the boxed area.
 */
_comphash (p)
register struct line   *p;
{
	register chtype  *c, *l;
	register int rc;

	if (p == NULL)
		return;
	if (p -> hash)
		return;
	rc = 1;
	c = p -> body;
	l = &p -> body[p -> length];
	while (--l > c && *l == ' ')
		;
	while (c <= l && *c == ' ')
		c++;
	p -> bodylen = l - c + 1;
	for ( ; c <= l ; c++)
		if (*c != ' ')
			rc = (rc<<1) + *c;
	p -> hash = rc;
	if (p->hash == 0)
		p->hash = 123;
	else if (p->hash == -888)
		p->hash = 124;
}
