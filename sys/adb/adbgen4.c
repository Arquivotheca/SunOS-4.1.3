#ifndef lint
static  char sccsid[] = "@(#)adbgen4.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Post-process adb scripts to move increment and decrement around.
 * The reason is that at the start of each +/ line, adb prints out
 * the current location.  If the line then increments or decrements
 * dot before printing the user may be confused.  So we move the
 * increment or decrement to the preceeding line.
 */

#include <stdio.h>
#include <ctype.h>

#define BUFSIZE 1024	/* gross enough to never be exceeded (we hope) */

char buf1[BUFSIZE], buf2[BUFSIZE];

char *scanpastnum();

main()
{
	register char *cur, *last, *cp1, *cp2, *ep, *t;
	register int len;

	gets(buf1);
	last = buf1;
	cur = buf2;
	while (gets(cur) != NULL) {
		if (goodstart(cur) && goodstart(last)) {
			/*
			 * Scan cur for initial increment.
			 * Ignore quoted strings, tabbing, adb newlines.
			 */
			cp1 = cur + 2;
			while (*cp1) {
				if (*cp1 == '"') {
					/* scan past quoted string */
					while (*++cp1 && *cp1 != '"')
						;
					cp1++;
					continue;
				}
				if (*cp1 >= '0' && *cp1 <= '9') {
					cp2 = scanpastnum(cp1);
				} else {
					cp2 = cp1;
				}
				if (*cp2 == 't' || *cp2 == 'n'
				    || *cp2 == ' ') {
					/* ok to skip over this one */
					cp1 = cp2 + 1;
					continue;
				} else {
					break;
				}
			}
			/*
			 * Cp1 now points at the first non-quoted string and
			 * non adb tab specification.
			 * Now determine if it's an increment or decrement.
			 */
			cp2 = scanpastnum(cp1);
			if (*cp2 == '+' || *cp2 == '-') {
				/*
				 * This is what we were hoping to find.
				 * Move increment or decrement into last.
				 */
				cp2++;
				ep = last + strlen(last);
				len = cp2  - cp1;
				strncpy(ep, cp1, len);
				ep[len] = '\0';
				/*
				 * Remove it from cur.
				 */
				strcpy(cp1, cp2);
			}
		}
		/*
		 * Prepare for next iteration.
		 */
		puts(last);
		t = cur;
		cur = last;
		last = t;
	}
	puts(last);
	exit(0);
}

/*
 * Cp points at a digit.
 * Return pointer to next char that isn't a digit.
 */
char *
scanpastnum(cp1)
	register char *cp1;
{
	register char *cp2;

	for (cp2 = cp1; isdigit(*cp2); cp2++)
		;
	return(cp2);
}

/*
 * Check whether a line has a good starting string.
 * We need ./ or +/ at the beginning to even think
 * of doing this motion stuff.
 */
goodstart(cp)
	register char *cp;
{
	if (*cp == '.' || *cp == '+') {
		if (*++cp == '/') {
			return(1);
		}
	}
	return(0);
}
