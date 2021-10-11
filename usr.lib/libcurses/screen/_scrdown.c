#ifndef lint
static	char sccsid[] = "@(#)_scrdown.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

/*
 * Scroll the screen down (e.g. in the normal direction of text) one line
 * physically, and update the internal notion of what's on the screen
 * (SP->cur_body) to know about this.  Do it in such a way that we will
 * realize this has been done later and take advantage of it.
 */
_scrdown()
{
	struct line *old_d, *old_p;
	register int i, l=lines;

#ifdef DEBUG
	if(outf)
	{
		fprintf(outf, "_scrdown()\n");
		fprintf( outf, "\tDoing _pos( %d, 0 )\n", lines - 1 );
		fprintf( outf, "\tDoing _scrollf(1)\n" );
	}
#endif

	/* physically... */
	_pos(lines-1, 0);
	_scrollf(1);

	/* internally */
	old_d = SP->std_body[1];
	old_p = SP->cur_body[1];
#ifdef	DEBUG
	if(outf)
	{
		fprintf( outf, "lines = l = %d\n", l );
	}
#endif	DEBUG

	for( i=1; i<=l; i++ )
	{
		SP->std_body[i] = SP->std_body[i+1];
		SP->cur_body[i] = SP->cur_body[i+1];
	}
	SP->std_body[1] = NULL;
	SP->cur_body[1] = NULL;
	_line_free(old_d);
	if( old_d != old_p )
	{
		_line_free(old_p);
	}
}
