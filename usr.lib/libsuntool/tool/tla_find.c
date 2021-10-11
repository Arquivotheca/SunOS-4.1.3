#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tla_find.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 *  tla_find.c - Find attribute on attribute value list generated during
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

static	tf_level;	/* Depth of recursion, 0 first time thru */
static	tf_found;	/* If 1 then tf_value is uncopied value */
static	char *tf_value;	/* Uncopied value found, valid if tf_found == 1 */

tool_find_attribute(avlist, attr, v)
	char **avlist;
	int attr;
	char **v;
{
	register int i, n;

	/* Argument checking */
	if (avlist == NULL || attr == (int) WIN_ATTR_LIST)
		goto Error;
	/* Initialize found value if first time thru (possibly recursive) */
	if (tf_level == 0) {
		tf_value = NULL;
		tf_found = 0;
	}
	i = 0;
	while (i < TOOL_ATTR_MAX) {
		switch ((int)avlist[i]) {
		case 0:
			/* End of attr list */
			goto Done;
		case WIN_ATTR_LIST:
			/* Decend down embedded attr list via recursion */
			tf_level++;
			(void) tool_find_attribute((char **)(LINT_CAST(
				avlist[i+1])), attr, v);
			tf_level--;
			i += 2;
			break;
		default:
			/* See if understand this attr */
			if ((n = tool_card_attr((int)(LINT_CAST(
				avlist[i])))) == -1) {
				if (tool_debug_attr)
					(void)fprintf(stderr,
					"tool_find_attribute illegal attr\n");
				goto Error;
			}
			/* See if this attr matches asked for attr */
			if (avlist[i] == (char *)attr) {
				/* Remember attr value */
				tf_value = avlist[i+1];
				/* Remember that found attr */
				tf_found = 1;
			}
			i += n+1;
			break;
		}
	}
Error:
	return(0);
Done:
	/* Copy if found attr */
	if (tf_found)
		*v = tool_copy_attr(attr, tf_value);
	return(tf_found);
}
	
