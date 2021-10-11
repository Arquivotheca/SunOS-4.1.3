#ifndef lint
static char sccsid[] = "@(#)parse_filenam.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <nse/param.h>

/*
 * Given a file name, divide it into its directory and simple name
 * components.
 *
 * Note: a file name ending in '/' is interpreted as a directory name
 * with a file name of "".
 */
void
_nse_parse_filename(file, dir, sname)
	char		*file;
	char		*dir;
	char		*sname;
{
	char		savec;
	char		*slash;

	slash = rindex(file, '/');
	if (slash == NULL) {
		if (dir != NULL) {
			strcpy(dir, ".");
		}
		if (sname != NULL) {
			strcpy(sname, file);
		}
	} else if (slash == file) {
		if (dir != NULL) {
			strcpy(dir, "/");
		}
		if (sname != NULL) {
			strcpy(sname, &slash[1]);
		}
	} else {
		if (dir != NULL) {
			savec = *slash;
			*slash = '\0';
			strcpy(dir, file);
			*slash = savec;
		}
		if (sname != NULL) {
			strcpy(sname, &slash[1]);
		}
	}
}
