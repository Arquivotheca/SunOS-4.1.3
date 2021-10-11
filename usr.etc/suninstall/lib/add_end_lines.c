#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)add_end_lines.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)add_end_lines.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		add_end_lines()
 *
 *	Description:	Add ending comment lines to the named file.
 *		Add a '+' to the end of each file if 'yp' is not YP_NONE.
 */

#include <stdio.h>
#include "install.h"
#include "menu.h"


/*
 *	External functions:
 */
extern	char *		sprintf();


static	char *		end_lines[] = {
	"#",
	"#\tEnd of lines added by the suninstall program.",
	"#",

	CP_NULL
};


void
add_end_lines(file, yp)
	char *		file;
	int		yp;
{
	char		cmd[MAXPATHLEN * 2];	/* command buffer */
	int		i;			/* index variable */
	FILE *		pp;			/* ptr to ed(1) process */


	(void) sprintf(cmd, "ed %s > /dev/null 2>&1", file);

	pp = popen(cmd, "w");
	if (pp == NULL) {
		menu_log("%s: '%s' failed.", progname, cmd);
		menu_abort(1);
	}

	/*
	 *	Delete any existing '+'
	 */
	(void) fprintf(pp, "g/^+$/d\n");

	/*
	 *	Add the end lines.
	 */
	(void) fprintf(pp, "$a\n");

	for (i = 0; end_lines[i]; i++)
		(void) fprintf(pp, "%s\n", end_lines[i]);

	if (yp != YP_NONE)
		(void) fprintf(pp, "+\n");

	(void) fprintf(pp, ".\nw\nq\n");

	(void) pclose(pp);
} /* end add_end_lines() */
