/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)m_newterm.c 1.1 92/07/30 SMI"; /* from S5R3 1.3.1.1 */
#endif

# include	"curses.ext"
# include	<signal.h>

extern	char	*calloc();
extern	char	*malloc();
extern	char	*getenv();

static	WINDOW	*makenew();

SCREEN *
m_newterm(type, outfd, infd)
char *type;
FILE *outfd, *infd;
{
#ifdef SIGTSTP
	int		m_tstp();
#endif /* SIGTSTP */
	struct screen *scp;
	struct screen *_new_tty();
	WINDOW *tempwin;
	int i;

#ifdef DEBUG
	if(outf) fprintf(outf, "NEWTERM() isatty(2) %d, getenv %s\n",
		isatty(2), getenv("TERM"));
# endif
	/* use calloc because everything needs to be zero */
	SP = (struct screen *) calloc(1, sizeof (struct screen));
	if (SP == NULL)
	    (void) fprintf (stderr, "calloc returned NULL in m_newterm\n");
	SP->term_file = outfd;
	SP->input_file = infd;
	SP->check_fd = -1;
	typeahead(fileno(infd));
	savetty();
	scp = _new_tty(type, outfd);
# ifdef SYSV
	PROGTTY.c_oflag &= ~OCRNL;	/* why would anyone set this? */
	if (PROGTTY.c_oflag == OPOST)
		PROGTTY.c_oflag = 0;
	reset_prog_mode();
# endif
# ifdef SIGTSTP
	if (signal(SIGTSTP, SIG_IGN) != SIG_IGN)
		signal(SIGTSTP, m_tstp);
# endif

	LINES =	_num_lines();
	COLS =	columns;
# ifdef DEBUG
	if(outf) fprintf(outf, "LINES = %d, COLS = %d\n", LINES, COLS);
# endif
	curscr = makenew(LINES, COLS, 0, 0);
	SP->Ybelow = 0;
	if (_use_slk)
	    {
	    SP->slk = (struct slkdata *) malloc (sizeof(struct slkdata));
	    if (SP->slk == NULL)
	        (void) fprintf (stderr, "malloc returned NULL in newterm\n");
	    else if (plab_norm &&
	        (label_height * label_width >= 8) &&
		(num_labels >= 8))
		_slk_init();
	    else if (COLS >= 80)
		{
		SP->slk->window = newwin(1, COLS, LINES-1, 0);
		SP->Ybelow++;
		_slk_init();
		}
	    }
	_use_slk = 0;
	SP->Yabove = 0;
	if (_ripcounter)
	    for (i=0; i<_ripcounter; i++)
		if (_ripstruct[i].line > 0)
		    {
		    tempwin = newwin(1, COLS, 0, 0);
		    (*_ripstruct[i].initfunction)(tempwin, COLS);
		    SP->Yabove++;
		    }
		else
		    {
		    tempwin = newwin(1, COLS, LINES-(SP->Ybelow)-1, 0);
		    (*_ripstruct[i].initfunction)(tempwin, COLS);
		    SP->Ybelow++;
		    }
	_ripcounter = 0;
	LINES -= SP->Yabove + SP->Ybelow;
	stdscr = makenew(LINES, COLS, 0, 0);
# ifdef DEBUG
	if(outf) fprintf(outf, "SP %x, stdscr %x, curscr %x\n", SP, stdscr, curscr);
# endif
	SP->std_scr = stdscr;
	SP->cur_scr = curscr;
	SP->fl_endwin = FALSE;

#ifdef _VR2_COMPAT_CODE
	{ extern int _endwin; _endwin = FALSE; }
#endif

	return scp;
}

/*
 *	This routine sets up a _window buffer and returns a pointer to it.
 */
static WINDOW *
makenew(num_lines, num_cols, begy, begx)
int	num_lines, num_cols, begy, begx;
{
	register WINDOW	*win;
	char *calloc();

# ifdef	DEBUG
	if(outf) fprintf(outf, "MAKENEW(%d, %d, %d, %d)\n", num_lines, num_cols, begy, begx);
# endif
	/* use calloc because of all the flags that are FALSE */
	/* and need to be set to zero */
	if ((win = (WINDOW *) calloc(1, sizeof (WINDOW))) == NULL) {
		(void) fprintf (stderr, "calloc returned NULL in makenew\n");
		return NULL;
	}
# ifdef DEBUG
	if(outf) fprintf(outf, "MAKENEW: num_lines = %d\n", num_lines);
# endif
	win->_maxy = num_lines;
	win->_maxx = num_cols;
	win->_begy = begy;
	win->_begx = begx;
/*
    The following don't need initializing because calloc is used above.
	win->_cury = win->_curx = 0;
	win->_need_idl = 0;
	win->_scroll = win->_leave = FALSE;
	win->_tmarg = 0;
*/
	win->_yoffset = SP->Yabove;
	win->_bmarg = num_lines - 1;
	win->_use_idl = 1;
	return win;
}

# ifdef SIGTSTP

/*
 * handle stop and start signals
 *
 */
m_tstp()
{
# ifdef DEBUG
	if (outf) fflush(outf);
# endif
	_ll_move(lines-1, 0);
	endwin();
	fflush(stdout);
# ifdef SIGIO		/* supports 4.2BSD signal mechanism */
	/* reset signal handler so kill below stops us */
	signal(SIGTSTP, SIG_DFL);
# ifndef sigmask
#define	sigmask(s)	(1 << ((s)-1))
# endif
	(void) sigsetmask(sigblock(0) &~ sigmask(SIGTSTP));
# endif
	kill(0, SIGTSTP);
# ifdef SIGIO
	sigblock(sigmask(SIGTSTP));
# endif
	signal(SIGTSTP, m_tstp);
	reset_prog_mode();
	SP->doclear = 1;
	_ll_refresh(0);
}
# endif /* SIGTSTP */
