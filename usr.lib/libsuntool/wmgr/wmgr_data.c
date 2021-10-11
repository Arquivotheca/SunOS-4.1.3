#ifndef lint
#ifdef sccs
static	char sccsid[] = "%Z%%M% %I% %E%";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * wmgr_data.c: fix for shared libraries in SunOS4.0.  Code was 
 *		 isolated from wmgr_menu.c
 */

#include <sys/types.h>
#include <suntool/tool_hs.h>
#include <suntool/menu.h>

#define TOOL_REFRESH	(caddr_t)1
#define TOOL_QUIT	(caddr_t)2
#define TOOL_OPEN	(caddr_t)3
#define TOOL_CLOSE	(caddr_t)4
#define TOOL_MOVE	(caddr_t)5
#define TOOL_STRETCH	(caddr_t)6
#define TOOL_TOP	(caddr_t)7
#define TOOL_BOTTOM	(caddr_t)8

int	WMGR_STATE_POS = 0;
int	WMGR_MOVE_POS = 1;
int	WMGR_STRETCH_POS = 2;
int	WMGR_TOP_POS = 3;
int	WMGR_BOTTOM_POS = 4;
int	WMGR_REFRESH_POS = 5;
int	WMGR_DESTROY_POS = 6;

struct	menuitem tool_items[] = {
	MENU_IMAGESTRING,	"Open",		TOOL_OPEN,
	MENU_IMAGESTRING,	"Move",		TOOL_MOVE,
	MENU_IMAGESTRING,	"Resize",	TOOL_STRETCH,
	MENU_IMAGESTRING,	"Expose",	TOOL_TOP,
	MENU_IMAGESTRING,	"Hide",		TOOL_BOTTOM,
	MENU_IMAGESTRING,	"Redisplay",	TOOL_REFRESH,
	MENU_IMAGESTRING,	"Quit",		TOOL_QUIT,
};

struct	menu wmgr_toolmenubody = {
	MENU_IMAGESTRING,
	"Frame",
	sizeof(tool_items) / sizeof(struct menuitem),
	tool_items,
	0,
	0
};
struct	menu *wmgr_toolmenu = &wmgr_toolmenubody;

