#ifndef lint
#ifdef sccs
static	char sccsid[] = "%Z%%M% %I% %E% Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * notify_data.c: fix for shared libraries in SunOS4.0.  Code was isolated
 *		  from ndet_itimer.c 
 */

#include <sunwindow/ntfy.h>

struct	itimerval NOTIFY_NO_ITIMER = {{0,0},{0,0}};
struct	itimerval NOTIFY_POLLING_ITIMER = {{0,1},{0,1}};

/* performance: global cache of getdtablesize() */
int dtablesize_cache;
