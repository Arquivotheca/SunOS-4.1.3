#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)align_equals.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* last edited on January 30, 1985, 14:50:47 */

#include <stdio.h>
#include <sunwindow/sun.h>
#include <sunwindow/string_utils.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

/*ARGSUSED*/
#ifdef STANDALONE
main(argc, argv)
#else
int align_equals_main(argc, argv)
#endif STANDALONE
	int             argc;
	char           *argv[];
{
	char            s[256], *a[30], c;
	int             maxpos, padding, i, j, k, m;

	maxpos = 0;
	for (i = 0; i < 30; i++) {
		if (!gets(s))
			break;
		a[i] = strdup(s);
		maxpos = MAX(string_find(s, "=",True), maxpos);
		/* string_find returns -1 if no = in s */
	}
	for (j = 0; j < i; j++) {
		m = string_find(a[j], "=", True);
		if (m == -1) {
			puts(a[j]);
			continue;
		}
		padding = maxpos - m;
		FOREVER {
			switch (a[j][m - 1]) {
			    case '+':
			    case '-':
			    case '*':
			    case '/':
			    case '%':
			    case '>':
			    case '<':
			    case '&':
			    case '^':
			    case '!':
				m--;
				/*
				 * e.g. if +=, align = sign, so need to write
				 * the blanks before the + 
				 */
				continue;
			}
			break;
		}
		c = a[j][m];
		a[j][m] = '\0';
		fputs(a[j], stdout);
		/*
		 * write everything up to the ='s use fputs so no newline
		 * printed 
		 */
		for (k = 0; k < padding; k++)
			putchar(' ');	/* write enough blanks */
		putchar(c);	/* write the equals */
		puts(a[j] + m + 1);	/* the rest of the characters */
	}
	
	EXIT(0);

}

