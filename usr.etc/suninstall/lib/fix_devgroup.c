#ifndef lint
static	char		mls_sccsid[] = "@(#)fix_devgroup.c 1.1 92/07/30 SMI; SunOS MLS";
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		fix_devgroup()1
 *
 *	Description:	Fix an entry in the DEVICE_GROUPS file.  The entry
 *		is named by 'name'.  The new values are given by 'minlab',
 *		'maxlab' and 'clean'.  The prefix to the correct file is
 *		given by 'prefix'.
 */

#include <stdio.h>
#include <string.h>
#include "install.h"
#include "menu.h"

extern	char *		sprintf();


void
fix_devgroup(name, minlab, maxlab, clean, prefix)
	char *		name;
	char *		minlab;
	char *		maxlab;
	char *		clean;
	char *		prefix;
{
	char		cmd[MAXPATHLEN * 2];	/* command buffer */


	(void) sprintf(cmd,
"%s %s%s%s %s %s %s %s %s %s %s %s %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s %s %s %s%s > %s%s.tmp",
		       "sed -n",
		       "-e '/^", name, "/b test'",
		       "-e 'b done'",
		       "-e ':test'",
		       "-e '/\\\\$/b join'",
		       "-e 'b edit'",
		       "-e ':join'",
		       "-e 'N'",
		       "-e 'b test'",
		       "-e ':edit'",
    		       "-e 's/\\([^:]*\\)[ \t\\n:\\\\]*",
			    "\\([^:]*\\)[ \t\\n:\\\\]*",
		            "\\([^:]*\\)[ \t\\n:\\\\]*",
		            "\\([^:]*\\)[ \t\\n:\\\\]*",
		            "\\([^:]*\\)[ \t\\n:\\\\]*.*/",
			    "\\1:\\\\\\\n",
		            "\t\\2:\\\\\\\n",
		            "\t'", minlab, "':\\\\\\\n",
		            "\t'", maxlab, "':\\\\\\\n",
		            "\t\\5:\\\\\\\n",
			    "\t'", clean, "'/'",
		       "-e ':done'",
		       "-e 'p'",
		       prefix, DEVICE_GROUPS, prefix, DEVICE_GROUPS);
	x_system(cmd);

	(void) sprintf(cmd, "cp %s%s.tmp %s%s; rm -f %s%s.tmp", prefix,
		       DEVICE_GROUPS, prefix, DEVICE_GROUPS, prefix,
		       DEVICE_GROUPS);
	x_system(cmd);
} /* end fix_devgroup() */
