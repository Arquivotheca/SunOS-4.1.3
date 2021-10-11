/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)ll_newtty.c 1.1 92/07/30 SMI"; /* from S5R3 1.5.1.1 */
#endif

#include "curses.ext"

static isfilter = 0;
extern char *malloc(), *calloc();

char *_c_why_not = NULL;
static char *
_stupid = "Sorry, I don't know how to deal with your '%s' terminal.\r\n";
static char *
_unknown = "Sorry, I need to know a more specific terminal type than '%s'.\r\n";
static char *
_noterminal = "Sorry, I don't know anything about your '%s' terminal.\r\n";
static char *
_nodatabase = "Sorry, I can't find /usr/lib/terminfo.\r\n";
static char *
_bad_malloc = "malloc returned NULL in _new_tty\n";

SCREEN *
_new_tty(type, fd)
char	*type;
FILE	*fd;
{
	int retcode;
	struct map *_init_keypad();

#ifdef DEBUG
	if(outf) fprintf(outf, "_new_tty: type %s, fd %x\n", type, fd);
#endif
	/*
	 * Allocate an SP structure if there is none, or if SP is
	 * still pointing to an old structure from a previous call
	 * to this routine.  But don't allocate one if we are being
	 * called from a higher level curses routine.  Since it's our
	 * job to initialize the phys_scr field and higher level routines
	 * aren't supposed to, we check that field to figure which to do.
	 */
	if (SP == NULL || SP->cur_body!=0)
		/* must use calloc here */
		SP = (struct screen *) calloc(1, sizeof (struct screen));
	if (SP == NULL) {
		_c_why_not = _bad_malloc;
		return NULL;
	}
	SP->term_file = fd;
	if (type == 0)
		type = "unknown";
	_setbuffered(fd);
	(void) setupterm(type, fileno(fd), &retcode);
	savetty();	/* as a "useful default" - hanging up is nasty. */

	/*
	 * This happens if /usr/lib/terminfo does not exist, there is
	 * no such terminal type, or the file is corrupted.
	 * This would be a good place to print an error message.
	 */
	if (retcode == 0) {
		_c_why_not = _noterminal;
		return NULL;
	} else if (retcode < 0) {
		_c_why_not = _nodatabase;
		return NULL;
	}
	if (chk_trm() == ERR)
		return NULL;
	SP->tcap = cur_term;
	SP->doclear = 1;
	SP->cur_body = (struct line **) calloc((unsigned)(lines+2), sizeof (struct line *));
	SP->std_body = (struct line **) calloc((unsigned)(lines+2), sizeof (struct line *));
	if (SP->cur_body == NULL || SP->std_body == NULL) {
		_c_why_not = _bad_malloc;
		return NULL;
	}
	_init_keypad();
	_init_acs();
	SP->input_queue = (short *) malloc(INP_QSIZE * sizeof (short));
	if (SP->input_queue == NULL) {
		_c_why_not = _bad_malloc;
		return NULL;
	}
	SP->input_queue[0] = -1;

	SP->virt_x = 0;		/* X and Y coordinates of the SP->curptr */
	SP->virt_y = 0;		/* between updates. */
	_init_costs();
	SP->cursorstate = 1;	/* assume a normal cursor */
	return SP;
}

static int
chk_trm()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "chk_trm().\n");
#endif

	if (generic_type) {
		_c_why_not = _unknown;
		return ERR;
	}
	if (isfilter) {
		_forget();
		/* Only need to move left or right on current line */
		if (cursor_left || carriage_return ||
		    column_address || parm_left_cursor) {
			/* we can handle it */
		} else {
			_c_why_not = _stupid;
			return ERR;
		}
	} else {
		if (hard_copy || over_strike) {
			_c_why_not = _stupid;
			return ERR;
		}
		if (cursor_address) {
			/* we can handle it */
		} else
		if ( (cursor_up || cursor_home)	/* some way to move up */
		    && cursor_down		/* some way to move down */
		    && (cursor_left || carriage_return)	/* ... move left */
						/* printing chars moves right */
			) {
			/* we can handle it */
		} else {
			_c_why_not = _stupid;
			return ERR;
		}
	}

	/* some way to clear the screen */
	if ( clear_screen || clr_eol || (delete_line && insert_line) )
	    { /* we can handle it */ }
	else
	    {
	    _c_why_not = _stupid;
	    return ERR;
	    }
	return OK;
}

/*
 * The program is a filter.  This routine must be called before initscr,
 * otherwise initscr may fail on some terminals, and may set up the data
 * structures wrong.  Arrange that curses thinks there is a 1 line screen,
 * and does not use any terminal capabilities that assume they know what
 * line on the screen the cursor is on.  Also do not clear the screen.
 */
filter()
{
	isfilter = 1;
}

static _save_lines;

/* Forget how to do things that assume what line the cursor is on. */
static
_forget()
{
	clear_screen = NULL;
	cursor_address = NULL;
	row_address = NULL;
	parm_up_cursor = NULL;
	parm_down_cursor = NULL;
	cursor_up = NULL;
	cursor_down = NULL;
	cursor_home = carriage_return;
	_save_lines = lines;
	lines = 1;
}

/* Return the true # of lines on the terminal, saved from before */
int
_num_lines()
{
	if (isfilter)
		return _save_lines;
	else
		return lines;
}
