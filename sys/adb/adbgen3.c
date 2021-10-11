#ifndef lint
static  char sccsid[] = "@(#)adbgen3.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Post-process adb script.
 * All we do is collapse repeated formats into number*format.
 * E.g. XXX is collapsed to 3X.
 */

#include <stdio.h>

main()
{
	register int c, quote, paren, savec, count, dispcmd;

	savec = count = 0;
	quote = 0;	/* not in quoted string */
	paren = 0;	/* not in parenthesized string */
	dispcmd = 0;	/* not in display command */
	while ((c = getchar()) != EOF) {
		if (c == '"') {
			quote = !quote;
		} else if (c == '(') {
			paren++;
		} else if (c == ')') {
			paren--;
		} else if (c == '/' || c == '?') {
			dispcmd = 1;
		} else if (c == '\n') {
			dispcmd = 0;
		}
		if (c == savec) {
			count++;
			continue;
		}
		if (savec) {
			if (count > 1) {
				printf("%d", count);
			}
			putchar(savec);
			savec = 0;
		}
		if (quote == 0 && paren == 0 && dispcmd
		    && index("FXOQDUfYpPxoqdubcC+IaAtrn-", c)) {
			savec = c;
			count = 1;
		} else {
			putchar(c);
		}
	}
	if (savec) {
		if (count > 1) {
			printf("%d", count);
		}
		putchar(savec);
	}
	exit(0);
}
