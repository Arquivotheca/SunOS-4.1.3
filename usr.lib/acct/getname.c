#ifndef lint
static	char sccsid[] = "@(#)getname.c 1.1 92/07/30 SMI";
#endif

/*
 * List the entire password database (i.e., "/etc/passwd" with NIS
 * entries expanded), or search the database for a given entry and print the
 * entry.
 */
#include <stdio.h>
#include <pwd.h>

struct passwd *getpwent();

main(argc, argv)
	int argc;
	char **argv;
{
	register struct passwd *pwd;

	if (argc == 1) {
		while ((pwd = getpwent()) != NULL)
			putpwent(pwd, stdout);
	} else {
		if ((pwd = getpwnam(argv[1])) != NULL)
			putpwent(pwd, stdout);
	}
}
