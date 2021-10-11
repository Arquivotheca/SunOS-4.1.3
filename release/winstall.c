

#ifndef lint
static	char sccsid[] = "@(#)winstall.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

char *name = "install";

main(argc, argv)
int	argc;
char	*argv[];
{
	int	uid;

	uid = geteuid();
	setuid(uid);

	execvp(name, argv);
}
