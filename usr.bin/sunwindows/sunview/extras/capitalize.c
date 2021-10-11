#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)capitalize.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 *	capitalize -- Read stdin; "capitalize" it as described in Windows
 *	and Window Based Tools: Beginner's Guide, Chapter 16; and write
 *	the result to stdout.
 *
 *	Rules of capitalization:
 *	String completely capitalized with and without whitespace ==>
 *		convert all alphabetics to lower case.
 *
 *	String not completely capitalized but containing whitespace ==>
 *		toggle case of first letter of each "word"; note that a "word"
 *		is considered a continuous string of non-whitespace characters.
 *		A "word" beginning with punctuation will not, strictly speaking,
 *		be capitalized.
 *
 *	String not completely capitalized but without whitespace ==>
 *		convert all alphabetics to upper case.  Note that mixed case
 *		strings become entirely uppercase.
 *
 *	Note that non-alphabetic and non-whitespace characters do not
 *	count in figuring out the "caseness" or "whiteness" of a string.
 *	They are significant only in whether the first character of a "word"
 *	will be capitalized.
 *
 *	If one of the following options is specified then the nature
 *	of the input is ignored and the corresponding action takes place:
 *
 *	-l	Convert all alphabetics to lower case regardless of input
 *	-c	Toggle case of first letter of each word
 *	-u	Convert all alphabetics to upper case regardless of input
 */
#include <stdio.h>
#include <ctype.h>
#include <sunwindow/sun.h>

#define EXIT_NOTEMP	1
#define EXIT_NOCLOSE	2

/*ARGSUSED*/
#ifdef STANDALONE
main(ac, av)
#else
capitalize_main(ac, av)
#endif STANDALONE
	int 	ac;
	char	**av;
{
	char	*temp = "/tmp/capXXXXXX";
	FILE	*tfp, *fopen();
	int	c;
	Bool	default_behavior, allcaps, whitespace;
	char    *mktemp();
	char	*progname;

	progname = av[0];
	default_behavior = True;

	while(ac > 1 && av[1][0] == '-') {
		switch(av[1][1]) {
			case 'l':	/* Fold to lower case */
				allcaps = True;
				whitespace = False;
				default_behavior = False;
				break;
			case 'c':	/* Capitalize individual words */
				whitespace = True;
				allcaps = False;
				default_behavior = False;
				break;
			case 'u':	/* Fold to upper case */
				whitespace = False;
				allcaps = False;
				default_behavior = False;
				break;
			default:
				fprintf(stderr, "Unknown flag: %s\n", av[1]);
				fprintf(stderr, "Usage: %s [-lcu]\n", progname);
		}
		--ac;
		av++;
	}

	/* Attempt to open temporary file; if failure then just copy input
	 * to output.
	 */
	(void)mktemp(temp);
	if ((tfp = fopen(temp, "w+")) == NULL) {
		(void)fprintf(stderr, "%s: can't open /tmp file\n", av[0]);
		while ((c = getchar()) != EOF)
			putchar(c);

		exit(EXIT_NOTEMP);
	}

	/* Scan stdin writing to temporary file; determine if there is
	 * whitespace and if all alphabetic characters are capitalized
	 */
	if(default_behavior) {
		allcaps = True;
		whitespace = False;
	}
	while ((c = getchar()) != EOF) {
		if(default_behavior) {
			if (isspace(c))
				whitespace = True;
			else if (isalpha(c) && islower(c))
				allcaps = False;
		}
		putc(c, tfp);
	}

	/* Now depending on what we have learned or the requested option, do
	 * the following:
	 *	allcaps ==> convert all alphabetic characters to lowercase (-l)
	 *
	 *	whitespace ==> toggle the case of characters that begin
	 *		a "word".  Observation:  "words" that start with a
	 *		non-alphabetic character, e.g., parenthesis, will not
	 *		be capitalized. (-c)
	 *
	 *	none of the above ==> there were some lower case characters
	 *		and therefore everything is converted to upper case (-u)
	 */
	(void)fseek(tfp, 0L, 0);			/* rewind file */

	if (allcaps)
		while ((c = getc(tfp)) != EOF)
			putchar((isalpha(c) && isupper(c)) ? tolower(c) : c);

	else if (whitespace)
		do {
			while ((c = getc(tfp)) != EOF && isspace(c))
			putchar(c);

			if (c != EOF) {
				if (isalpha(c))
					putchar((islower(c)) ? toupper(c) : c);
				else
					putchar(c);
			}

			while ((c = getc(tfp)) != EOF && !isspace(c))
				putchar(c);

			if (c != EOF)
				putchar(c);

		} while (c != EOF);
	else
		while ((c = getc(tfp)) != EOF)
			putchar((isalpha(c) && !isupper(c)) ? toupper(c) : c);

	/* Cleanup temporary file */
	if (unlink(temp) != 0) {
		(void)fprintf(stderr, "%s: couldn't delete /tmp file\n", av[0]);
		exit(EXIT_NOCLOSE);
	}

	exit(0);

}
