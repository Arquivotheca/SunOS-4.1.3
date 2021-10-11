#ifndef lint
#ifdef sccs
static	char sccsid[] = "%Z%%M% %I% %E%";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * win_misc_data.c: fix for shared libraries in SunOS4.0.  Code was isolated
 *		    from win_misc.c and win_screen.c
 */

#include <sunwindow/sun.h>
#include <sunwindow/win_struct.h>

extern	int win_errordefault();
int	(*win_error)() = win_errordefault;

unsigned win_screendestroysleep;	/* (See win_screendestroy) */

int	 winscreen_print;		/* Debugging flag */
int	sv_journal = FALSE;
