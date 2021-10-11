#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tla_usage.c	1.1	92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 *  tla_usage.c - Print message indicating generic window command line options
 */

#include <stdio.h>

int
tool_usage(tool_name)
	char *tool_name;
{
	(void)fprintf(stderr, "usage of %s generic window arguments:\n\
FLAG\t(LONG FLAG)\t\tARGS\t\tNOTES\n\
-Ww\t(-width)\t\tcolumns\n\
-Wh\t(-height)\t\tlines\n\
-Ws\t(-size)\t\t\tx y\n\
-Wp\t(-position)\t\tx y\n\
-WP\t(-icon_position)\tx y\n\
-Wl\t(-label)\t\t\"string\"\n\
-Wi\t(-iconic)\t\n\
-Wn\t(-no_name_stripe)\t\n\
-Wt\t(-font)\t\t\tfilename\n\
-Wf\t(-foreground_color)\tred green blue\t0-255 (no color-full color)\n\
-Wb\t(-background_color)\tred green blue\t0-255 (no color-full color)\n\
-Wg\t(-set_default_color)\t\t\t(apply color to subwindows too)\n\
-WI\t(-icon_image)\t\tfilename\n\
-WL\t(-icon_label)\t\t\"string\"\n\
-WT\t(-icon_font)\t\tfilename\n\
-WH\t(-help)\t\n", (tool_name)? tool_name: "");
}

