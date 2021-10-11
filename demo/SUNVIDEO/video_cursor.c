
#ifndef lint
static char sccsid[] = "@(#)video_cursor.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>

#include <suntool/sunview.h>


static short my_cursor_data[] = {
    0x07C6, 0x1FF7, 0x383B, 0x600C, 0x600C, 0xC006, 0xC006, 0xDF06,
    0xC106, 0xC106, 0x610C,0x610C, 0x3838, 0x1FF0, 0x07C0, 0x0000
};

mpr_static(busy_cursor, 16, 16, 1, my_cursor_data);


static Cursor old_cursor;

set_busy_cursor(window)
    Window window;
{
    old_cursor = window_get(window, WIN_CURSOR);
    window_set(window, WIN_CURSOR,
	       cursor_create(CURSOR_IMAGE, &busy_cursor, 0),
	       0);
}

set_regular_cursor(window)
{
    window_set(window, WIN_CURSOR,
	       old_cursor,
	       0);
    
}
