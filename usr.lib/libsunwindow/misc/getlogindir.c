#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)getlogindir.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 *  getlogindir - Get login directory.  First try HOME environment variable.
 *	Next try password file.  Print message if can't get login directory.
 */

#include <stdio.h>
#include <pwd.h>

char *
getlogindir()
{
	extern char *getlogin(), *getenv();
	extern struct passwd *getpwnam(), *getpwuid();
	struct passwd *passwdent;
	char *home, *loginname;

	home = getenv("HOME");
	if (home != NULL)
		return (home);
	loginname = getlogin();
	if (loginname == NULL)
		passwdent = getpwuid(getuid());
	else
		passwdent = getpwnam(loginname);
	if (passwdent == NULL) {
		(void)fprintf(stderr,
		    "getlogindir: couldn't find user in password file.\n");
		return (NULL);
	}
	if (passwdent->pw_dir == NULL) {
		(void)fprintf(stderr,
		    "getlogindir: no home directory in password file.\n");
		return (NULL);
	}
	return (passwdent->pw_dir);
}

