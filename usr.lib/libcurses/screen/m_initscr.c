/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)m_initscr.c 1.1 92/07/30 SMI"; /* from S5R3 1.3 */
#endif

# include	"curses.ext"

extern	char	*getenv();
extern	struct screen *m_newterm();

/*
 *	This routine initializes the current and standard screen.
 *
 */

WINDOW *
m_initscr()
{
	register char	*sp;

# ifdef DEBUG
	if (outf == NULL) {
		outf = fopen("trace", "w");
		if (outf == NULL) {
			perror("trace");
			exit(-1);
		}
	}
#endif

	if (isatty(2)) {
		if ((sp = getenv("TERM")) == NULL)
			sp = Def_term;
# ifdef DEBUG
		if(outf) fprintf(outf, "INITSCR: term = %s\n", sp);
# endif
	}
	else {
		sp = Def_term;
	}
	(void) m_newterm(sp, stdout, stdin);
	return stdscr;
}
