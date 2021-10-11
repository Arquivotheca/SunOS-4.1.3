#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)insert_brackets.c 1.1 92/07/30 SMI";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 *	insert_brackets -- Filter to insert brackets.  Read from stdin and
 *	write the result to stdout.  Left and right bracket strings obtained
 *	as argv[1] and argv[2], respectively.  Note that the .textswrc file
 *	is processed such that one layer of \'s are removed.  Therefore, to
 *	get the string \fP specify either \\\\fP or "\\fP" (literally).  Strings
 *	so read by the filter are processed according to K&R rules for 
 *	the backslash so that the string \\fP becomes \fP.  Notice that K&R
 *	recognizes \f as formfeed (ascii octal 14), so that without the second
 *	\ in \\fP the result becomes <FF>P.
 */
#include <stdio.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

#define EXIT_BADARGS	1

extern char	*process_string();
extern char	octal_char();

#ifdef STANDALONE
main(ac, av)
#else
int insert_brackets_main(ac, av)
#endif STANDALONE
	int 	ac;
	char	**av;
{
	int	c;
	char	*left, *right;
	
	if (ac != 3) {
		(void)fprintf(stderr, "Usage: %s leftbracket rightbracket\n", av[0]);
		while ((c = getchar()) != EOF)
			putchar(c);
		EXIT(EXIT_BADARGS);
	}

	left = process_string(av[1]);
	right = process_string(av[2]);

	fputs(left, stdout);
	while ((c = getchar()) != EOF)
		putchar(c);
	fputs(right, stdout);

	EXIT(0);
}
