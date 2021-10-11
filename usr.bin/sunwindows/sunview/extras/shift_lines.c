#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)shift_lines.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* last edited on Fri Apr 11 05:18:48 PST 1986 */

#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <strings.h>
#include <sunwindow/sun.h>
#include <sunwindow/string_utils.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

#define STR_SIZE	1024

static int	indent_pro_tabsize();


#ifdef STANDALONE
main(argc, argv)
#else
int shift_lines_main(argc, argv)
#endif STANDALONE
	int             argc;
	char           *argv[];
{
	char            s[STR_SIZE];
	int             tabs = 0;
	int             delta = 0;
	int             depth, i, j;
	Bool            saw_space = False;

   	/*
    	* argument is number of tab stops. corresponding number of
    	* characters to be computed below. 
        */
	if (argc > 1 && strequal(argv[1], "-t")) {
                if (argv[2] == NULL) {
                     fprintf(stderr, "shift_lines: Missing value specifying number of tab stops!\n");
                     exit(1);
                }
                if (argv[2][0] == '-') i = 1;
                else i = 0;
                while (argv[2][i] != '\0') {
                     if (!isdigit(argv[2][i])) {
                        fprintf(stderr, "shift_lines: '-t' argument value must be an integer!\n");
                        fprintf(stderr, "Usage: shift_lines [-t] n \n");
                        exit(1);
                     }
                     i++;
                }
                tabs = atoi(argv[2]);
	} else if (argc > 1)
		delta = atoi(argv[1]);

	FOREVER {
		if (!fgets(s, STR_SIZE, stdin))
			break;	/* no more input */
		/*
		 * compute depth of indentation for this line, also  position
		 * of first non-white space character 
		 */
		for (i = 0, depth = 0; s[i] != '\0'; i++) {
			if (s[i] == '\t')
				depth += 8;
			else if (s[i] == ' ') {
				saw_space = True;
				depth++;
			} else
				break;
		}
		if (tabs != 0) {
			/* compute value of tab stop (only done once) */
			if (saw_space)
				/*
				 * assume four space tab. Note that even if
				 * user has specified 8 spaces in his
				 * .indent.pro using four spaces here is
				 * probably what he wants. For example, he
				 * could have used indent_tool to reformat
				 * this particular file with four spaces
				 * because it uses long identifier names. 
				 */
				delta = tabs * 4;
			else {
				delta = tabs * indent_pro_tabsize();
			}
			tabs = 0;	/* so won't try to recompute each
					 * line */
		}
		for (j = 0; j < (depth + delta) / 8; j++)
			putchar('\t');
		for (j = 0; j < (depth + delta) % 8; j++)
			putchar(' ');
		fputs(s + i, stdout);
	}
	EXIT(0);
}

/*
 * look in .indent.pro to see if user is using 4 space or 8 space 
 * note: loops allows for finding "-i" entries not followed by num
 *       and multiple line format
 * ex: -ip
 */
static int
indent_pro_tabsize()
{
	FILE           *stream;
	char           *p, s[STR_SIZE], *target="-i";
	int		i, tabsize = 0;

	(void)sprintf(s, "%s/.indent.pro", getpwuid(getuid())->pw_dir);
	if ((stream = fopen(s, "r")) != NULL) {
		while(!tabsize && fgets(s, STR_SIZE, stream)) {
			p=s;
			do {
				i = string_find(p, target, True);
				if (i != -1) {
					p = p + i + strlen(target);
					tabsize = atoi(p);
				}
			} while (!tabsize && i != -1);
		}
		(void)fclose(stream);
	}
	
	return (tabsize ? tabsize : 8);
}
