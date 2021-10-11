#ifndef lint
#ifdef sccs
static	char sccsid[] = "%Z%%M% %I% %E% Copyright 1987 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * walkmenu_data.c: fix for shared libraries in SunOS4.0.  Code was isolated
 *		  from walkmenu_public.c 
 */


#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <fcntl.h>
#include <varargs.h>

#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>

#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/cursor_impl.h>

#include <suntool/walkmenu_impl.h>

/* Define 3 shades of gray for shadow replrop */
static	short gray25_data[16] = {/*25% gray so background will show through*/
	0x8888, 0x2222, 0x4444, 0x1111, 0x8888, 0x2222, 0x4444, 0x1111,
	0x8888, 0x2222, 0x4444, 0x1111, 0x8888, 0x2222, 0x4444, 0x1111
};
mpr_static(menu_gray25_pr, 16, 16, 1, gray25_data);

static	short gray50_data[16] = { /* 50% gray */
	0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555,
	0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555
};
mpr_static(menu_gray50_pr, 16, 16, 1, gray50_data);

static	short gray75_data[16] = { /* 75% gray */
	0x7777,0xDDDD,0xBBBB,0xEEEE,0x7777,0xDDDD,0xBBBB,0xEEEE,
	0x7777,0xDDDD,0xBBBB,0xEEEE,0x7777,0xDDDD,0xBBBB,0xEEEE
};
mpr_static(menu_gray75_pr, 16, 16, 1, gray75_data);

/* Pending display cursor for sticky menus */
/* /usr/view/2.0/usr/src/sun/usr.bin/suntool/images/menu_pending.cursor */
static short walkmenu_cursor_pending_image[] = {
    0x001F,0x0011,0x0011,0x0011,0x0011,0x0211,0x0311,0xFF91,
    0xFF91,0x0311,0x0211,0x0011,0x0011,0x0011,0x0011,0x001F
};

mpr_static(walkmenu_cursor_pending_mpr, 16, 16, 1,
		  walkmenu_cursor_pending_image);
struct cursor walkmenu_cursor_pending =
  { 17, 6, PIX_SRC^PIX_DST, &walkmenu_cursor_pending_mpr, FULLSCREEN};


extern struct cursor menu_cursor;	/* ?? defined by old menu pkg(menu.c) */
struct cursor *walkmenu_cursor = &menu_cursor; /* Settable cursor */
