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
static	char sccsid[] = "@(#)colrm.c 1.1 92/07/30 SMI"; /* from UCB 5.1 5/31/85 */
#endif not lint

#include <stdio.h>
/*
COLRM removes unwanted columns from a file
	Jeff Schriebman  UC Berkeley 11-74
*/


main(argc,argv)
char **argv;
{
	register c, ct, first, last;

	first = 0;
	last = 0;
	if (argc > 1)
		first = getn(*++argv);
	if (argc > 2)
		last = getn(*++argv);

start:
	ct = 0;
loop1:
	c = getc(stdin);
	if (feof(stdin))
		goto fin;
	if (c == '\t')
		ct = (ct + 8) & ~7;
	else if (c == '\b')
		ct = ct ? ct - 1 : 0;
	else
		ct++;
	if (c == '\n') {
		putc(c, stdout);
		goto start;
	}
	if (!first || ct < first) {
		putc(c, stdout);
		goto loop1;
	}

/* Loop getting rid of characters */
	while (!last || ct < last) {
		c = getc(stdin);
		if (feof(stdin))
			goto fin;
		if (c == '\n') {
			putc(c, stdout);
			goto start;
		}
		if (c == '\t')
			ct = (ct + 8) & ~7;
		else if (c == '\b')
			ct = ct ? ct - 1 : 0;
		else
			ct++;
	}

/* Output last of the line */
	for (;;) {
		c = getc(stdin);
		if (feof(stdin))
			break;
		putc(c, stdout);
		if (c == '\n')
			goto start;
	}
fin:
	fflush(stdout);
	exit(0);
	/* NOTREACHED */
}

getn(ap)
char *ap;
{
	register int n,c;
	register char *p;

	p = ap;
	n = 0;
	while ((c = *p++) >= '0' && c <= '9')
		n = n*10 + c - '0';
	return(n);
}
