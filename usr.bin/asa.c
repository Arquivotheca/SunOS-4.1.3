#ifndef lint
static	char sccsid[] = "@(#)asa.c 1.1 92/07/30 SMI"; /* from S5R2 1.3 */
#endif

/*
 *	asa - interpret asa carriage control characters
 *
 *	This program is designed to make sense out of the output
 *	of fortran programs whose authors have used asa carriage
 *	control characters.  "asa" processes either the files
 *	whose names are given as arguments or the standard input
 *	if no file names are given.  "asa" processes each input files
 *	and produces the results to the standard output.
 *	The first character of each line is assumed to be a control
 *	character: the meanings of the control characters are:
 *
 *	' '	single-space before printing
 *	'0'	double-space before printing
 *	'-'	triple-space before printing
 *	'1'	new page before printing
 *	'+'	do not space at all before printing
 *
 *	A line beginning with '+' will overprint the previous line.
 *
 *	Lines beginning with other than the above characters are
 *	treated as if they began with ' '.  The first characters
 *	of such lines will be replaced by ' '.
 *	If any such lines appear,
 *	an appropriate diagnostic will appear on the standard error
 *	file after processing each file.  The return code reflects
 *	the error conditions after processing:
 *		0	if no error
 *		-1	output error
 *		>0	It shows there is no output error and
 *			there are some input files which cannot
 *			be opened. The return code is the total number
 *			of input files which cannot be opened.
 *
 *	The program forces the first line of each file to
 *	start on a new page.  This program requires the printer
 *	be able to perform the formfeed character.
 *
 *	example:
 *		asa [-s] [files]
 *	-s will suppress the error messages.
 *	if there are no files the input file defaults to stdin.
 */

#include <stdio.h>

/* program name, for diagnostics */
char *pgmname;

/* count of lines with bad control characters */
long badlines = 0;

/* number of errors detected */
int retcode = 0;

/* mask for error message */
int prnt = 1;

main (argc, argv)
	int argc; char **argv;
{
int i;
	pgmname = argv[0];

	/* were any files given, or do we process stdin? */
	if (argc == 1)

		/* process standard input */
		dofile ("standard input");
	else {
		/* suppress error messages ? */
		i=1;
		if (argv[1][0]=='-' && argv[1][1]=='s') {
			prnt=0; i++;
			if (argc==2) dofile("standard input");
			}
		/* one iteration per input file */
		for ( ; i < argc; i++) {
			if (freopen (argv[i], "r", stdin) == NULL) {
			fprintf (stderr, "%s: cannot open %s\n",
				pgmname, argv[i]);
			retcode++;
			}
			else
				dofile (argv[i]);
	     }
	}

	/* Check if any error on output stream */
	if (ferror (stdout)) {
		fprintf (stderr, "%s: output error\n", pgmname);
		retcode= -1;
	}

	exit(retcode);
}

/*
 *	dofile - process the standard input.
 *
 *	This program is called once for each input file, with stdin
 *	redirected to the file.  The "fname" argument is used
 *	for writing diagnostic messages only.
 */

dofile (fname)
	char *fname;
{
	register int c;
	int firstchar; /* flag to show first char in a line */

	badlines=firstchar=0;
	c=getchar();
	lff: putchar('\f');  /* start with a new page */
	c=getchar();
	while (c!=EOF)
		{
		if (firstchar) {
			firstchar=0;
			switch (c) {
				/* new page */
				case '\f':
				case '1':
					putchar('\n');
					putchar('\f');
					break;

				/* triple space */
				/* this is not in the spec! */
				case '-':
					putchar('\n');

				/* double space */
				case '0':
					putchar('\n');

				/* single space */
				case ' ':
					putchar('\n');
					break;
				case '\n':
					firstchar=1;
					putchar('\n');
					break;

				/* no space at all */
				case '+':
					putchar('\r');
					break;
				default:
					badlines++;
					putchar('\n');
				}
			}
		else 	{
			switch (c) {
				case '\f':
					putchar('\n');
					putchar('\f');
					break;
				case '\n':
					firstchar=1;
					break;
				default:
					putchar(c);
					break;
				}
			}
		c=getchar();
		}
	putchar('\n');

	/* report invalid input lines -- dofile increments badlines */
	if (badlines && prnt)
	fprintf (stderr, "%s: %ld invalid input lines in %s\n",
		pgmname, badlines, fname);
	return;
}
