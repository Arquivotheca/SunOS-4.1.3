#ifndef lint
static char sccsid[] = "@(#)cmdname.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <nse/param.h>
#include <nse/util.h>

static	char		*cmdname = NULL;

/*
 * Save the name of the current command for use in messages.
 */
char *
nse_set_cmdname(file)
	char		*file;
{
	char		name[MAXNAMLEN];

	NSE_DISPOSE(cmdname);
	_nse_parse_filename(file, (char *) NULL, name);
	cmdname = NSE_STRDUP(name);
	return cmdname;
}

/*
 * Return the command name.
 */
char *
nse_get_cmdname()
{
	return cmdname;
}
