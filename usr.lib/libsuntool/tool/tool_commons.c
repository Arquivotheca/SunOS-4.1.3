#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tool_commons.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

#include <sys/file.h>
#include <stdio.h>
#include <varargs.h>
#include <suntool/tool_hs.h>
#include <sunwindow/sun.h>
#include <sunwindow/win_ioctl.h>
#include <suntool/tool_impl.h>
#include <suntool/wmgr.h>

static short _tool_dtgrayimage[16]={
	0x8888,
	0x8888,
	0x2222,
	0x2222,
	0x8888,
	0x8888,
	0x2222,
	0x2222,
	0x8888,
	0x8888,
	0x2222,
	0x2222,
	0x8888,
	0x8888,
	0x2222,
	0x2222};
static  short _tool_cursorimage[16] = {
#include        <images/bullseye.cursor>
};

mpr_static(_tool_pixrectdtgray, 16, 16, 1, _tool_dtgrayimage);
struct	pixrect *tool_bkgrd = &_tool_pixrectdtgray;

mpr_static(_tool_mpr, 12, 12, 1, _tool_cursorimage);
struct  cursor tool_cursor = {5, 5, PIX_SRC^PIX_DST, &_tool_mpr};
