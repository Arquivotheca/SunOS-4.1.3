/*	@(#)uparm.h 1.1 92/07/30 SMI; from S5R3.1 1.2	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Local configuration of various files.  Used if you can't put these
 * things in the standard places or aren't the super user, so you
 * don't have to modify the source files.  Thus, you can install updates
 * without having to re-localize your sources.
 *
 * This file used to go in /usr/include/local/uparm.h.  Every version of
 * UNIX has undone this, so now it wants to be installed in each source
 * directory that needs it.  This means you now include "uparm.h" instead
 * of "local/uparm.h".
 */

/* Path to library files */
#define libpath(file) "/usr/lib/file"

/* Path to local library files */
#define loclibpath(file) "/usr/local/lib/file"

/* Path to binaries */
#define binpath(file) "/usr/ucb/file"

/* Path to things under /usr (e.g. /usr/preserve) */
#define usrpath(file) "/usr/file"

/* Location of termcap file */
#define E_TERMCAP	"/etc/termcap"

/* Location of terminfo source file */
#define E_TERMINFO	"./terminfo.src"

/* Location of terminfo binary directory tree */
#define _TERMPATH(file)	"/usr/share/lib/terminfo/file"

/* Location of the C shell */
#define B_CSH		"/bin/csh"
