#ifndef lint
#ifdef sccs
static	char sccsid[] = "%Z%%M% %I% %E%";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * rect_data.c: fix for shared libraries in SunOS4.0.  Code was isolated
 *		  from rectlist.c and rect.c 
 */


#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>

/*
 * rectlist constants
 */
struct	rectlist rl_null = {0, 0, 0, 0, 0, 0, 0, 0};

struct	rect rect_null   = {0, 0, 0, 0};
