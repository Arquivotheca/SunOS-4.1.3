/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)vdmp.c 1.1 92/07/30 SMI"; /* from UCB 5.1 5/15/85 */
#endif not lint

/*
 *  reads raster file created by cifplot and dumps it onto the
 *  Varian or Versatec plotter.
 *  Assumptions:
 *	Input is from device 0.
 *	plotter is already opened as device 1.
 *	error output file is device 2.
 */
#include <stdio.h>
#include <sys/vcmd.h>

#define IN	0
#define OUT	1

#define MAGIC_WORD	0xA5CF4DFA

#define BUFSIZE		1024*128
#define BLOCK		1024

int	plotmd[] = { VPLOT };
int	prtmd[]	= { VPRINT };

int	inbuf[BLOCK/sizeof(int)];
char	buf[BUFSIZE];
int	lines;

int	varian;			/* 0 for versatec, 1 for varian. */
int	BYTES_PER_LINE;		/* number of bytes per raster line. */
int	PAGE_LINES;		/* number of raster lines per page. */

char	*name, *host, *acctfile;

main(argc, argv)
	int argc;
	char *argv[];
{
	register int n;

	while (--argc) {
		if (**++argv == '-') {
			switch (argv[0][1]) {
			case 'x':
				BYTES_PER_LINE = atoi(&argv[0][2]) / 8;
				varian = BYTES_PER_LINE == 264;
				break;

			case 'y':
				PAGE_LINES = atoi(&argv[0][2]);
				break;

			case 'n':
				argc--;
				name = *++argv;
				break;

			case 'h':
				argc--;
				host = *++argv;
			}
		} else
			acctfile = *argv;
	}

	n = read(IN, inbuf, BLOCK);
	if (inbuf[0] == MAGIC_WORD && n == BLOCK) {
		/* we have a formatted dump file */
		inbuf[(BLOCK/sizeof(int))-1] = 0;  /* make sure string terminates */
		ioctl(OUT, VSETSTATE, prtmd);
		write(OUT, &inbuf[4], (strlen(&inbuf[4])+1) & ~1);
		write(OUT, "\n", 2);
	} else				/* dump file not formatted */
		lseek(IN, 0L, 0);	/* reset in's seek pointer and plot */

	n = putplot();

	/* page feed */
	ioctl(OUT, VSETSTATE, prtmd);
	if (varian)
		write(OUT, "\f", 2);
	else
		write(OUT, "\n\n\n\n\n", 6);
	account(name, host, acctfile);
	exit(n);
	/* NOTREACHED */
}

putplot()
{
	register char *cp;
	register int bytes, n;

	cp = buf;
	bytes = 0;
	ioctl(OUT, VSETSTATE, plotmd);
	while ((n = read(IN, cp, sizeof(buf))) > 0) {
		if (write(OUT, cp, n) != n)
			return(1);
		bytes += n;
	}
	/*
	 * Make sure we send complete raster lines.
	 */
	if ((n = bytes % BYTES_PER_LINE) > 0) {
		n = BYTES_PER_LINE - n;
		for (cp = &buf[n]; cp > buf; )
			*--cp = 0;
		if (write(OUT, cp, n) != n)
			return(1);
		bytes += n;
	}
	lines += bytes / BYTES_PER_LINE;
	return(0);
}

account(who, from, acctfile)
	char *who, *from, *acctfile;
{
	register FILE *a;

	if (who == NULL || acctfile == NULL)
		return;
	if (access(acctfile, 02) || (a = fopen(acctfile, "a")) == NULL)
		return;
	/*
	 * Varian accounting is done by 8.5 inch pages;
	 * Versatec accounting is by the (12 inch) foot.
	 */
	fprintf(a, "t%6.2f\t", (double)lines / (double)PAGE_LINES);
	if (from != NULL)
		fprintf(a, "%s:", from);
	fprintf(a, "%s\n", who);
	fclose(a);
}
