#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)cim_change.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Character image manipulation (except size change) routines.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <suntool/ttyansi.h>
#include <suntool/charimage.h>
#include <suntool/charscreen.h>

char	boldify;

extern	char *  strcpy();

#define JF

vpos(row, col)
	int row, col;
{
	register unsigned char *line = image[row];
	register char *bold = screenmode[row];
        unsigned char c;


	while (length(line) <= col) {
                c = ((unsigned char)(line[-1]));
                bold[c] = MODE_CLEAR;
                line[c] = ' ';
                line[-1] = ((unsigned char)(line[-1])) + 1;
	}
	setlinelength(line, (length(line)));
}

bold_mode()
{
	boldify |= MODE_BOLD;
}

nobold_mode()
{
	boldify &= ~MODE_BOLD;
}

underscore_mode()
{
	boldify |= MODE_UNDERSCORE;
}

nounderscore_mode()
{
	boldify &= ~MODE_UNDERSCORE;
}

inverse_mode()
{
	boldify |= MODE_INVERT;
}

noinverse_mode()
{
	boldify &= ~MODE_INVERT;
}

clear_mode()
{
	boldify = MODE_CLEAR;
}

writePartialLine(s, curscolStart)
	char *s;
	register int curscolStart;
{
	register char *sTmp;
	register unsigned char *line = image[cursrow]; 
	register char *bold = screenmode[cursrow];
	register int curscolTmp = curscolStart;

	/*
	 * Fix line length if start is past end of line length.
	 * This shouldn't happen but does.
	 */
	if (length(line) < curscolStart )
		(void)vpos(cursrow, curscolStart);
	/*
	 * Stick characters in line.
	 */
	for (sTmp=s;*sTmp!='\0';sTmp++) {
		line[curscolTmp] = *sTmp;
		bold[curscolTmp] = boldify;
		curscolTmp++;
	}
	/*
	 * Set new line length.
	 */
	if (length(line) < curscolTmp )
		setlinelength(line, curscolTmp);
/*
if (sTmp>(s+3)) printf("%d\n",sTmp-s);
*/
	/* Note: curscolTmp should equal curscol here */
/*
if (curscolTmp!=curscol)
	printf("csurscolTmp=%d, curscol=%d\n", curscolTmp,curscol);
*/
	(void)pstring(s, boldify, curscolStart, cursrow, PIX_SRC);
}

#ifdef	USE_WRITE_CHAR
writechar(c)
	char c;
{
	register char *line = image[cursrow];
	char unitstring[2];

	unitstring[0] = line[curscol] = c;
	unitstring[1] = 0;
	if (length(line) <= curscol )
		setlinelength(line, curscol+1);
	/* Note: if revive this proc then null terminate line */
	(void)pstring(unitstring, curscol, cursrow, PIX_SRC);
}
#endif	USE_WRITE_CHAR

#ifdef JF
cim_scroll(n)
register int n;
{	register int	new;

#ifdef DEBUG_LINES
printf(" cim_scroll(%d)	\n", n);
#endif
	if (n>0) {		/*	text moves UP screen	*/
		(void)delete_lines(top, n);
	} else {	/* (n<0)	text moves DOWN	screen	*/
		new = bottom + n;
		(void)roll(top, new, bottom);
		(void)pcopyscreen(top,top - n, new);
		(void)cim_clear(top, top - n);
	}
}
#else
cim_scroll(toy, fromy)
	int fromy, toy;
{

	if (toy < fromy) /* scrolling up */
		(void)roll(toy, bottom, fromy);
	else
		swapregions(fromy, toy, bottom-toy);
	if (fromy > toy) {
		(void)pcopyscreen(fromy, toy, bottom - fromy);
		(void)cim_clear(bottom-(fromy-toy), bottom);	/* move text up */
	} else {
		(void)pcopyscreen(fromy, toy, bottom - toy);
		(void)cim_clear(fromy, bottom-(toy-fromy));	/* down */
	}
}
#endif

insert_lines(where, n)
register int where, n;
{	register int new = where + n;

#ifdef DEBUG_LINES
printf(" insert_lines(%d,%d) bottom=%d	\n", where, n, bottom);
#endif
	if (new > bottom)
		new = bottom;
	(void)roll(where, new, bottom);
	(void)pcopyscreen(where, new, bottom - new);
	(void)cim_clear(where, new);
}

delete_lines(where, n)
register int where, n;
{	register int new = where + n;

#ifdef DEBUG_LINES
printf(" delete_lines(%d,%d)	\n", where, n);
#endif
	if (new > bottom) {
		n -= new - bottom;
		new = bottom;
	}
	(void)roll(where, bottom - n, bottom);
	(void)pcopyscreen(new, where, bottom - new);
	(void)cim_clear(bottom-n, bottom);
}

roll(first, mid, last)
	int first, last, mid;
{

/* printf("first=%d, mid=%d, last=%d\n", first, mid, last); */
	reverse(first, last);
	reverse(first, mid);
	reverse(mid, last);
}

static
reverse(a, b)
	int a, b;
{

	b--;
	while (a < b)
		(void)swap(a++, b--);
}

swapregions(a, b, n)
	int a, b, n;
{

	while (n--)
		(void)swap(a++, b++);
}

swap(a, b)
	int a, b;
{
	unsigned char *tmpline = image[a];
	char *tmpbold = screenmode[a];

	image[a] = image[b];
	image[b] = tmpline;
	screenmode[a] = screenmode[b];
	screenmode[b] = tmpbold;
}

cim_clear(a, b)
	int a, b;
{
	register int i;

	for (i = a; i < b; i++)
		setlinelength(image[i], 0);
	(void)pclearscreen(a, b);
	if (a == top && b == bottom) {
		if (delaypainting)
			(void)pdisplayscreen(1);
		else
			delaypainting = 1;
	}
}

deleteChar(fromcol, tocol, row)
	int fromcol, tocol, row;
{
	register unsigned char *line = image[row];
	register char *bold = screenmode[row];
	int len = length(line);

	if (fromcol >= tocol)
		return;
	if (tocol < len) {
		/*
		 * There's a fragment left at the end
		 */
		int	gap = tocol - fromcol;
		{
			register unsigned char *a = line+fromcol;
			register unsigned char *b = line+tocol;
			register char *am = bold + fromcol;
			register char *bm = bold + tocol;
			while (*a++ = *b++)
				*am++ = *bm++;
		}
		setlinelength(line, len-gap);
		(void)pcopyline(fromcol, tocol, len - tocol, row);
		(void)pclearline(len - gap, len, row);
	} else if (fromcol < len) {
		setlinelength(line, fromcol);
		(void)pclearline(fromcol, len, row);
	}
}

insertChar(fromcol, tocol, row)
	int fromcol;
	register int tocol;
	int row;
{
	register unsigned char *line = image[row];
	register char *bold = screenmode[row];
	int len = length(line);
	register int i;
	int delta, newlen, slug, rightextent;

	if (fromcol >= tocol || fromcol >= len)
		return;
	delta = tocol - fromcol;
	newlen = len+delta;
	if (newlen > right)
		newlen = right;
	if (tocol > right)
		tocol = right;
	for (i = newlen; i >= tocol; i--) {
		line[i] = line[i-delta];
		bold[i] = bold[i-delta];
	}
	for (i = fromcol; i < tocol; i++) {
		line[i] = ' ';
		bold[i] = MODE_CLEAR;
	}
	setlinelength(line, newlen);
	rightextent = len+(tocol-fromcol);
	slug = len - fromcol;
	if (rightextent > right)
		slug -= rightextent - right;
	(void)pcopyline(tocol, fromcol, slug, row);
	(void)pclearline(fromcol, tocol, row);
}
