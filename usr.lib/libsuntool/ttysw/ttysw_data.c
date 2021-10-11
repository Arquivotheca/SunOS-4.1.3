#ifndef lint
#ifdef sccs
static	char sccsid[] = "%Z%%M% %I% %E%";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * ttysw_data.c: fix for shared libraries in SunOS4.0.  Code was 
 *		 isolated from ttyansi.c and csr_change.c
 */

#include <sys/time.h>
#include <suntool/ttyansi.h>

int tty_cursor = BLOCKCURSOR | LIGHTCURSOR;

struct timeval ttysw_bell_tv = {0, 100000}; /* 1/10 second */
