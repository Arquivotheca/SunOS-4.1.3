#ifndef lint
static	char sccsid[] = "@(#)miniinit.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"
# include	<signal.h>

char	*calloc();
extern	char	*getenv();

static WINDOW	*makenew();

struct screen *m_newterm();

/*
 *	This routine initializes the current and standard screen.
 *
 * 3/5/81 (Berkeley) @(#)initscr.c	1.2
 */

WINDOW *
m_initscr() {
	reg char	*sp;
	
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

struct screen *
m_newterm(type, outfd, infd)
char *type;
FILE *outfd, *infd;
{
	int		m_tstp();
	struct screen *scp;
	struct screen *_new_tty();
	extern int _endwin;

#ifdef DEBUG
	if(outf) fprintf(outf, "NEWTERM() isatty(2) %d, getenv %s\n",
		isatty(2), getenv("TERM"));
# endif
	SP = (struct screen *) calloc(1, sizeof (struct screen));
	SP->term_file = outfd;
	SP->input_file = infd;
	savetty();
	scp = _new_tty(type, outfd);
# ifdef USG
	(cur_term->Nttyb).c_oflag &= ~OCRNL;
	reset_prog_mode();
# endif USG
# ifdef SIGTSTP
	signal(SIGTSTP, m_tstp);
# endif

	LINES =	lines;
	COLS =	columns;
# ifdef DEBUG
	if(outf) fprintf(outf, "LINES = %d, COLS = %d\n", LINES, COLS);
# endif
	curscr = makenew(LINES, COLS, 0, 0);
	stdscr = makenew(LINES, COLS, 0, 0);
# ifdef DEBUG
	if(outf) fprintf(outf, "SP %x, stdscr %x, curscr %x\n", SP, stdscr, curscr);
# endif
	SP->std_scr = stdscr;
	SP->cur_scr = curscr;
	_endwin = FALSE;
	return scp;
}

/*
 * Low level interface, for compatibility with old curses.
 */
setterm(type)
char *type;
{
	setupterm(type, 1, 0);
}

gettmode()
{
	/* No-op included only for upward compatibility. */
}

/*
 *	This routine sets up a _window buffer and returns a pointer to it.
 */
static WINDOW *
makenew(num_lines, num_cols, begy, begx)
int	num_lines, num_cols, begy, begx;
{
	reg WINDOW	*win;
	char *calloc();

# ifdef	DEBUG
	if(outf) fprintf(outf, "MAKENEW(%d, %d, %d, %d)\n", num_lines, num_cols, begy, begx);
# endif
	if ((win = (WINDOW *) calloc(1, sizeof (WINDOW))) == NULL)
		return NULL;
# ifdef DEBUG
	if(outf) fprintf(outf, "MAKENEW: num_lines = %d\n", num_lines);
# endif
	win->_cury = win->_curx = 0;
	win->_maxy = num_lines;
	win->_maxx = num_cols;
	win->_begy = begy;
	win->_begx = begx;
	win->_scroll = win->_leave = win->_use_idl = FALSE;
	return win;
}
