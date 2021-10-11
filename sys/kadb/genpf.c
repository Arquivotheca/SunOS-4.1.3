#ifndef lint
static  char sccsid[] = "@(#)genpf.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "pf.h"
#include <stdio.h>
#include <ctype.h>

#define MAXENTRY	512
char *entry[MAXENTRY];

char *myname;
char *dir = ".";
char *strsav(), *rindex(), *strcpy();
FILE *pfout;
int lastent = 0;

#define	PF	"pf.c"

main(argc, argv)
	char *argv[];
{

	myname = argv[0];
	argv++; argc--;
	if (argc > 1 && strcmp(*argv, "-d") == 0) {
		argv++; argc--;
		if (argc > 2) {
			dir = strsav(*argv);
			argv++; argc--;
		} else {
			usage();
			/*NOTREACHED*/
		}
	}
	if (argc <= 0) {
		usage();
		/*NOTREACHED*/
	}
	if ((pfout = fopen(PF, "w")) == NULL) {
		perror(PF);
		exit(4);
	}
	if (chdir(dir) == -1) {
		perror(dir);
		exit(4);
	}
	pf_prologue();
	while (argc > 0) {
		process_file(*argv);
		argv++; argc--;
	}
	pf_epilogue();
	exit(0);
}

char *
strsav(s)
	char *s;
{
	extern char *malloc();
	char *m;

	if ((m = malloc(strlen(s) + 1)) == NULL) {
		error("no memory");
		/*NOTREACHED*/
	}
	(void) strcpy(m, s);
	return (m);
}

usage()
{

	fprintf(stderr, "usage: %s [-d dir] file ...\n", myname);
	exit(1);
}

/*VARARGS1*/
error(s, a1, a2, a3, a4)
	char *s;
{

	fprintf(stderr, s, a1, a2, a3, a4);
	exit(2);
}

pf_prologue()
{

	fprintf(pfout, "#include \"pf.h\"\n");
}

process_file(name)
	char *name;
{
#define	MAXLINE	256
	register FILE *f;
	register char *pname, *s;
	char buf[MAXLINE];

	if ((f = fopen(name, "r")) == NULL) {
		perror(name);
		return;
	}
	pname = rindex(name, '/');
	if (pname == NULL)
		pname = name;
	else
		pname++;
	/*
	 * Convert all '.' in file name to '_'
	 */
	while ((s = rindex(pname, '.')) != NULL)
		*s = '_';
	fprintf(pfout, "\nchar %s_NAME[] = \"%s\";\nchar %s_STRING[] = \n\"",
	    pname, pname, pname);
	while (fgets(buf, MAXLINE, f) != NULL) {
		for (s = buf; *s; s++) {
			if (*s == '"')
				putc('\\', pfout);
			else if (*s == '\n')
				fprintf(pfout, "\\n\\");
			putc(*s, pfout);
		}
	}
	fprintf(pfout, "\";\n");
	entry[lastent++] = strsav(pname);
	fclose(f);
}

pf_epilogue()
{
	register int i;

	fprintf(pfout, "\nstruct pseudo_file pf[] = {\n");
	for (i = 0; i < lastent; i++)
		fprintf(pfout, "\t{ %s_NAME, %s_STRING },\n",
		    entry[i], entry[i]);
	fprintf(pfout, "};\n\nint npf = %d;\n", lastent);
	fclose(pfout);
}
