#ifndef lint
#ifdef sccs
static	char sccsid[] = "%Z%%M% %I% %E%";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * panel_util_data.c: fix for shared libraries in SunOS4.0.  Code was 
 *		      isolated from panel_util.c, panel_create.c
 */
 
#include <suntool/panel_impl.h>

/* caret handling functions */
void	(*panel_caret_on_proc)()	= (void(*)()) panel_nullproc;
void	(*panel_caret_invert_proc)()	= (void(*)()) panel_nullproc;

/* selection service functions */
void	(*panel_seln_inform_proc)()	= (void(*)()) panel_nullproc;
void	(*panel_seln_destroy_proc)()	= (void(*)()) panel_nullproc;

static unsigned short	cycle_data[] = {
#include <images/cycle.pr>
};
mpr_static(panel_cycle_pr, 16, 14, 1, cycle_data);
