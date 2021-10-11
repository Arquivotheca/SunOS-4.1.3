/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
All rights reserved.\n";
#endif not lint

#ifndef lint
static	char sccsid[] = "@(#)from.c 1.1 92/07/30 SMI"; /* from UCB 5.2 11/4/85 */
#endif not lint

#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/param.h>

struct	passwd *getpwuid();
char *strcpy();

main(argc, argv)
	int argc;
	register char **argv;
{
	char lbuf[BUFSIZ];
	char lbuf2[BUFSIZ];
	register struct passwd *pp;
	int stashed = 0;
	register char *name;
	char *sender;
	char *getlogin();
	char mailbox[MAXPATHLEN];
	char *tmp_mailbox, *getenv();

	if (argc > 1 && *(argv[1]) == '-' && (*++argv)[1] == 's') {
		if (--argc <= 1) {
			(void) fprintf (stderr,
			    "Usage: from [-s sender] [user]\n");
			exit (1);
		}
		--argc;
		sender = *++argv;
		for (name = sender; *name; name++)
			if (isupper(*name))
				*name = tolower(*name);

	} else
		sender = NULL;
	if (argc > 1)
		(void) sprintf(mailbox, "/usr/spool/mail/%s", argv[1]);
	else {
		if (tmp_mailbox = getenv("MAIL")) {
			(void) strcpy(mailbox, tmp_mailbox);
		} else {
			name = getlogin();
			if (name == NULL || strlen(name) == 0) {
				pp = getpwuid(getuid());
				if (pp == NULL) {
					(void) fprintf(stderr,
					    "Who are you?\n");
					exit(1);
				}
				name = pp->pw_name;
			}
			(void) sprintf(mailbox, "/usr/spool/mail/%s", name);
		}
	}
	if (freopen(mailbox, "r", stdin) == NULL) {
		(void) fprintf(stderr, "Can't open %s\n", mailbox);
		exit(0);
	}
	while (fgets(lbuf, sizeof lbuf, stdin) != NULL)
		if (lbuf[0] == '\n' && stashed) {
			stashed = 0;
			(void) printf("%s", lbuf2);
		} else if (strncmp(lbuf, "From ", 5) == 0 &&
		    (sender == NULL || match(&lbuf[4], sender))) {
			(void) strcpy(lbuf2, lbuf);
			stashed = 1;
		}
	if (stashed)
		(void) printf("%s", lbuf2);
	exit(0);
	/* NOTREACHED */
}

match (line, str)
	register char *line, *str;
{
	register char ch;

	while (*line == ' ' || *line == '\t')
		++line;
	if (*line == '\n')
		return (0);
	while (*str && *line != ' ' && *line != '\t' && *line != '\n') {
		ch = isupper(*line) ? tolower(*line) : *line;
		if (ch != *str++)
			return (0);
		line++;
	}
	return (*str == '\0');
}
