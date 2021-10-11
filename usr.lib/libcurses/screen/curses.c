/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)curses.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.9 */
#endif

/* Define global variables */

#include	"curses_inc.h"

WINDOW	*stdscr, *curscr, *_virtscr;
int	LINES, COLS, TABSIZE;
short	curs_errno = -1;
int	(*_setidln)(), (*_useidln)(), (*_quick_ptr)();
int	(*_do_slk_ref)(), (*_do_slk_tch)(), (*_do_slk_noref)();
void	(*_rip_init)();		/* to initialize rip structures */
void	(*_slk_init)();		/* to initialize slk structures */
SCREEN	*SP;

#ifdef	_VR3_COMPAT_CODE
void	(*_y16update)();
chtype	*acs32map;

#undef	acs_map
_ochtype	*acs_map;
#else	/* _VR3_COMPAT_CODE */
chtype		*acs_map;
#endif	/* _VR3_COMPAT_CODE */

char	*curses_version = "SVR3", curs_parm_err[32];

#ifdef	DEBUG
FILE	*outf = stderr;		/* debug output file */
#endif	/* DEBUG */
