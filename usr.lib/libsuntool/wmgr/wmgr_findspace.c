#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)wmgr_findspace.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sunwindow/window_hs.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/win_enum.h>
#include "sunwindow/sv_malloc.h"
#include <suntool/wmgr_policy.h>

#define WMGR_MAX_NODES	32

#define is_open(nodep)	((nodep->flags&WIN_NODE_OPEN) != 0)

struct rectlist *
wmgr_findspace(rootfd, selector, ignorefd)
    Window_handle   rootfd, ignorefd;
    int             selector;
{
    register Win_enum_node
                   *nodep, *limit;
    register int    ignore, result;
    Rect            root_rect;
    Rectlist       *rl;
    Win_enum_node   nodes[WMGR_MAX_NODES];
    char *malloc();

    rl = (Rectlist *) (LINT_CAST(sv_malloc(sizeof(Rectlist))));
    ignore = (ignorefd == 0) ? -1 : win_fdtonumber(ignorefd);
    (void)win_getrect(rootfd, &root_rect);
    (void)rl_initwithrect(&root_rect, rl);
    if ((result = win_get_tree_layer(rootfd, sizeof nodes, (char *)nodes)) < 0)
	return (Rectlist *) 0;
    limit = nodes + (result / sizeof(Win_enum_node));
    for (nodep = nodes; nodep < limit; nodep++) {
	if (nodep->me == ignore)
	    continue;
	switch (selector) {
	  case WFS_CURRENT:
	    if (is_open(nodep)) {
		(void)rl_rectdifference(&nodep->open_rect, rl, rl);
	    } else {
		(void)rl_rectdifference(&nodep->icon_rect, rl, rl);
	    }
	    break;
	  case WFS_BOTH:
	    (void)rl_rectdifference(&nodep->open_rect, rl, rl);
	    (void)rl_rectdifference(&nodep->icon_rect, rl, rl);
	    break;
	  case WFS_OPEN:
	    (void)rl_rectdifference(&nodep->open_rect, rl, rl);
	    break;
	  case WFS_ICONS:
	    (void)rl_rectdifference(&nodep->icon_rect, rl, rl);
	    break;
	}
    }
    return rl;
}

int
wmgr_is_child_inserted(parent, child)
    Window_handle   parent, child;

{
    register Win_enum_node
                   *nodep, *limit;
    register int  my_num, result;
    Win_enum_node   nodes[WMGR_MAX_NODES];

    my_num = win_fdtonumber(child);
    if ((result = win_get_tree_layer(parent, sizeof nodes, (char *)nodes)) < 0)
	return FALSE;
    limit = nodes + (result / sizeof(Win_enum_node));
    for (nodep = nodes;nodep < limit; nodep++) {
	if (nodep->me == my_num)
	    return ((nodep->flags & WIN_NODE_INSERTED) != 0);
    }
    return FALSE;
}
