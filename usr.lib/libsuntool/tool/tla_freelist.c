#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tla_freelist.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 *  tla_freelist.c - Free contents of attribute value list generated during
 *	tool_parse_*.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <stdio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_struct.h>
#include <suntool/icon.h>
#include <suntool/tool.h>
#include <suntool/tool_impl.h>

tool_free_attribute_list(avlist)
	char **avlist;
{
	register int i, n;

	if (avlist == NULL)
		return;
	i = 0;
	while (i < TOOL_ATTR_MAX) {
		if (avlist[i]) {
			if ((n = tool_card_attr(
				(int)(LINT_CAST(avlist[i])))) == -1) {
			    n = ATTR_CARDINALITY(avlist[i]);
/* 			    fprintf(stderr, "tool_attr illegal attr\n");
			    return;
*/
			} else {
			    (void)tool_free_attr((int)(LINT_CAST(avlist[i])), 
			    	(char *)(LINT_CAST(avlist[i+n])));
			}
			i += n+1;
		} else
			break;
	}
	free((char *)(LINT_CAST(avlist)));
}
	
