/*      @(#)uparm.h 1.1 92/07/30 SMI; from S5R3.1 1.6     */

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
 */

/* Path to library files */
#define libpath(file) "/usr/lib/file"

/* Path to local library files */
#define loclibpath(file) "/usr/local/lib/file"

/* Path to binaries */
#define binpath(file) "/usr/bin/file"

/* Path to things under /usr (e.g. /usr/preserve) */
#define usrpath(file) "/var/file"

/* Location of terminfo binary directory tree */
#define termpath(file)	"/usr/share/lib/terminfo/file"

#define TMPDIR	"/var/tmp"
