#ifndef lint
#ifdef sccs
static	char sccsid[] = "%Z%%M% %I% %E%";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * teksw_data.c: fix for shared libraries in SunOS4.0.  Code was 
 *		 isolated from teksw_ui.c
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>


static short hg_data[] = {
#include <images/hglass.cursor>
};

Pixrect	hglass_cursor_mpr;
mpr_static(hglass_cursor_mpr, 16, 16, 1, hg_data);

#ifdef STANDALONE
/* stop (page full) cursor (stop sign) */
static short stop_data[16] = {
	0x07C0, 0x0FE0, 0x1FF0, 0x1FF0, 0x1FF0, 0x1FF0, 0x1FF0, 0x0FE0,
	0x07C0, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0FE0
};

mpr_static(stop_mpr, 16, 16, 1, stop_data);
#endif STANDALONE

