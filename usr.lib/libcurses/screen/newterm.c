/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)newterm.c 1.1 92/07/30 SMI"; /* from S5R3 1.4.1.1 */
#endif

# include	"curses.ext"
# include	<signal.h>

extern char *calloc(), *malloc();
extern WINDOW *_makenew();
#ifdef DEBUG
extern char *getenv();
#endif

/*
 * newterm sets up a terminal and returns a pointer to the terminal
 * structure or NULL in case of an error.  The parameters are the
 * terminal type name and two file descriptors to be used for output
 * to the screen and input from the keyboard.
 */

SCREEN *
newterm(type, outfd, infd)
char *type;
FILE *outfd, *infd;
{
	int		_tstp();
	SCREEN *scp;
	SCREEN *_new_tty();
	int i;
	WINDOW *tempwin;

# ifdef DEBUG
	if (outf == NULL) {
		outf = fopen("trace", "w");
		if (outf == NULL) {
			perror("trace");
			exit(-1);
		}
		setbuf(outf, (char *) NULL);
	}
#endif

#ifdef DEBUG
	if(outf) fprintf(outf, "NEWTERM(type=%s, outfd=%x %d, infd=%x %d) isatty(2) %d, getenv %s\n",
		type, outfd, fileno(outfd), infd, fileno(infd), isatty(2), getenv("TERM"));
# endif

	/* use calloc because everything needs to be zero */
	SP = (struct screen *) calloc(1, sizeof (struct screen));
	if (SP == NULL)
	    (void) fprintf (stderr, "calloc returned NULL in newterm\n");
	SP->term_file = outfd;
	SP->input_file = infd;
	SP->check_fd = -1;
	typeahead(fileno(infd));

	/*
	 * The default is echo, for upward compatibility, but we do
	 * all echoing in curses to avoid problems with the tty driver
	 * echoing things during critical sections.
	 */
	SP->fl_echoit = 1;
	scp = _new_tty(type, outfd);
	if (scp == NULL)
		return NULL;
#ifdef SYSV
	PROGTTY.c_lflag &= ~ECHO;
	PROGTTY.c_oflag &= ~OCRNL;	/* why would anyone set this? */
	if (PROGTTY.c_oflag == OPOST)
		PROGTTY.c_oflag = 0;
#else
	PROGTTY.sg_flags &= ~ECHO;
#endif
	reset_prog_mode();
# ifdef SIGTSTP
	if (signal(SIGTSTP, SIG_IGN) != SIG_IGN)
		signal(SIGTSTP, _tstp);
# endif
	if (curscr != NULL) {
# ifdef DEBUG
		if(outf) fprintf(outf, "INITSCR: non null curscr = 0%o\n", curscr);
# endif
	}
	LINES =	_num_lines();
	COLS =	columns;
# ifdef DEBUG
	if(outf) fprintf(outf, "LINES = %d, COLS = %d\n", LINES, COLS);
# endif

	curscr = _makenew(lines, COLS, 0, 0);
	for (i = 0; i < lines; i++)
		curscr->_y[i] = (chtype *) 0;
	SP->Ybelow = 0;
	if (_use_slk) {
		if (!SP->slk) {
			SP->slk = (struct slkdata *) malloc (sizeof(struct slkdata));
			if (SP->slk == NULL)
			        (void) fprintf (stderr, "malloc returned NULL in newterm\n");
			_slk_init();
		}
		if (plab_norm &&
		    (label_height * label_width >= 8) &&
		    (num_labels >= 8))
		        SP->slk->window = 0;
	        else if (COLS >= 15) {
			_slk_winit(SP->slk->window = newwin(1, COLS, LINES-1, 0));
			SP->Ybelow++;
			LINES--;
		}
		_use_slk = 0;
	}
	SP->Yabove = 0;
	if (_ripcounter)
	    for (i=0; i<_ripcounter; i++)
		if (_ripstruct[i].line > 0)
		    {
		    tempwin = newwin(1, COLS, 0, 0);
		    (*_ripstruct[i].initfunction)(tempwin, COLS);
		    SP->Yabove++;
		    LINES--;
		    }
		else
		    {
		    tempwin = newwin(1, COLS, LINES-1, 0);
		    (*_ripstruct[i].initfunction)(tempwin, COLS);
		    SP->Ybelow++;
		    LINES--;
		    }
	_ripcounter = 0;
	stdscr = newwin(LINES, COLS, 0, 0);
# ifdef DEBUG
	if(outf) fprintf(outf, "SP %x, stdscr %x, curscr %x\n", SP, stdscr, curscr);
# endif
	SP->std_scr = stdscr;
	SP->cur_scr = curscr;
	/* Maybe should use makewin and glue _y's to DesiredScreen. */
	SP->fl_endwin = FALSE;

#ifdef _VR2_COMPAT_CODE
	{ extern int _endwin; _endwin = FALSE; }
#endif


	return scp;
}

/*
 * allocate space for and set up defaults for a new _window
 *
 * 1/26/81 (Berkeley).  This used to be newwin.c
 */

WINDOW *
newwin(nlines, ncols, by, bx)
register int	nlines, ncols, by, bx;
{
	register WINDOW	*win;
	register int i;
	extern void memSset();

	if (by + nlines > LINES || nlines == 0)
		nlines = LINES - by;
	if (bx + ncols > COLS || ncols == 0)
		ncols = COLS - bx;

	if (by < 0 || bx < 0)
		return (WINDOW *) NULL;

	if ((win = _makenew(nlines, ncols, by, bx)) == (WINDOW *) NULL)
		return (WINDOW *) NULL;
	for (i = 0; i < nlines; i++)
		if ((win->_y[i] = (chtype *) malloc(ncols * sizeof (chtype))) == NULL) {
			register int j;
			(void) fprintf (stderr, "malloc returned NULL in newwin\n");

			for (j = 0; j < i; j++)
				free((char *)win->_y[j]);
			free((char *)win->_firstch);
			free((char *)win->_lastch);
			free((char *)win->_y);
			free((char *)win);
			return (WINDOW *) NULL;
		}
		else
			memSset(win->_y[i], (chtype) ' ', ncols);
	win->_yoffset = SP->Yabove;
	return win;
}
