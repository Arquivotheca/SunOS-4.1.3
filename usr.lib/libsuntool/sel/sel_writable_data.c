#ifndef lint
#ifdef sccs
static	char sccsid[] = "%Z%%M% %I% %E%";
#endif
#endif

#include <suntool/selection_impl.h>
#include <suntool/selection.h>

/*
 *	sel_writable_data.c:	writable initialized data
 *				(must not go in text segment) 
 *
 */

struct timeval	seln_std_timeout = {
    SELN_STD_TIMEOUT_SEC, SELN_STD_TIMEOUT_USEC
};

int	seln_svc_program = SELN_SVC_PROGRAM;

struct	selection selnull;

