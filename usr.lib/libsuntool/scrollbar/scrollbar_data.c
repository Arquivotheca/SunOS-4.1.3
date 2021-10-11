#ifndef lint
#ifdef sccs
static	char sccsid[] = "%Z%%M% %I% %E%";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * scrollbar_data.c: fix for shared libraries in SunOS4.0.  Code was 
 *		     isolated from scrollbar_event.c
 */

#include <suntool/scrollbar_impl.h> 
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>

static short saved_client_cursor_image[16];
mpr_static(scrollbar_saved_client_mpr, 16, 16, 1, saved_client_cursor_image);
struct cursor scrollbar_client_saved_cursor = 
   {0x7FFF, 0, 0, &scrollbar_saved_client_mpr};
