#ifndef lint
static	char sccsid[] = "@(#)soelim.c 1.1 92/07/30 SMI"; /* from UCB 4.4 06/28/83 */
#endif

#include <stdio.h>
/*
 * soelim - a filter to process n/troff input eliminating .so's
 *
 * Author: Bill Joy UCB July 8, 1977
 *
 * This program eliminates .so's from a n/troff input stream.
 * It can be used to prepare safe input for submission to the
 * phototypesetter since the software supporting the operator
 * doesn't let him do chdir.
 *
 * This is a kludge and the operator should be given the
 * ability to do chdir.
 *
 * This program is more generally useful, it turns out, because
 * the program tbl doesn't understand ".so" directives.
 */
#define	STDIN_NAME	"-"

main(argc, argv)
	int argc;
	char *argv[];
{

	argc--;
	argv++;
	if (argc == 0) {
		(void)process(STDIN_NAME);
		exit(0);
	}
	do {
		(void)process(argv[0]);
		argv++;
		argc--;
	} while (argc > 0);
	exit(0);
	/* NOTREACHED */
}

int process(file)
	char *file;
{
	register char *cp;
	register int c;
	char fname[BUFSIZ];
	FILE *soee;
	int isfile;

	if (!strcmp(file, STDIN_NAME)) {
		soee = stdin;
	} else {
		soee = fopen(file, "r");
		if (soee == NULL) {
			perror(file);
			return(-1);
		}
	}
	for (;;) {
		c = getc(soee);
		if (c == EOF)
			break;
		if (c != '.')
			goto simple;
		c = getc(soee);
		if (c != 's') {
			putchar('.');
			goto simple;
		}
		c = getc(soee);
		if (c != 'o') {
			printf(".s");
			goto simple;
		}
		do
			c = getc(soee);
		while (c == ' ' || c == '\t');
		cp = fname;
		isfile = 0;
		for (;;) {
			switch (c) {

			case ' ':
			case '\t':
			case '\n':
			case EOF:
				goto donename;

			default:
				*cp++ = c;
				c = getc(soee);
				isfile++;
				continue;
			}
		}
donename:
		if (cp == fname) {
			printf(".so");
			goto simple;
		}
		*cp = 0;
		if (process(fname) < 0)
			if (isfile)
				printf(".so %s\n", fname);
		continue;
simple:
		if (c == EOF)
			break;
		putchar(c);
		if (c != '\n') {
			c = getc(soee);
			goto simple;
		}
	}
	if (soee != stdin) {
		fclose(soee);
	}
	return(0);
}
