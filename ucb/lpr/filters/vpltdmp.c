/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)vpltdmp.c 1.1 92/07/30 SMI"; /* from UCB 5.1 5/15/85 */
#endif not lint

/*
 * Copyright (C) 1981, Regents of the University of California
 *	All rights reserved
 *
 *  Reads raster file created by vplot and dumps it onto the
 *    Varian or Versatec plotter.
 *  Supports new rasterfile formats AND old vpltdmp format.
 *  Input comes from file descriptor 0, output is to file descriptor 1.
 */
#include <stdio.h>
#include <sys/vcmd.h>

#ifndef NO_PR_LOAD
#include <pixrect/pixrect_hs.h>
#else
#include <rasterfile.h>
#endif

#define	nbytes(x)	((((x)+15)/16)*2)

#define IN	0
#define OUT	1

int	plotmd[] = { VPLOT };
int	prtmd[]  = { VPRINT };

char	buf[BUFSIZ];		/* output buffer */

int	lines;			/* number of raster lines printed */
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

	n = putplot();

	ioctl(OUT, VSETSTATE, prtmd);
	if (varian)
		write(OUT, "\f", 2);
	else
		write(OUT, "\n\n\n\n\n", 6);
	account(name, host, *argv);
	exit(n);
	/* NOTREACHED */
}

putplot()
{
	struct rasterfile rh;
	register char *rp, *zp, *sp;
	extern char *malloc();
	register int w, h, rn;
	register int i, wm, hm, ws;

	ioctl(OUT, VSETSTATE, plotmd);
	if (read(0, &rh, sizeof rh) != sizeof rh || rh.ras_magic != RAS_MAGIC) {
		lseek(0, 0, 0);
		return (vpltdmp());
	}
	switch(rh.ras_type) {
	case RT_OLD:		/* Fall through */
	case RT_STANDARD:	break;
#ifndef NO_PR_LOAD
	default:		return(putnstdras());
#endif
	}
	lseek(0, rh.ras_maplength, 1);
	w = nbytes(rh.ras_width);
	h = rh.ras_height;
	if (w < BYTES_PER_LINE) {
		wm = (BYTES_PER_LINE-w)/2;
		ws = 0;
	} else {
		wm = 0;
		ws = w - BYTES_PER_LINE;
		w = BYTES_PER_LINE;
		if (ws)
			sp = malloc(ws);
	}
	hm = (PAGE_LINES-h)/2;
	rp = malloc(w);
	zp = malloc(BYTES_PER_LINE);
	bzero(zp, BYTES_PER_LINE);
	setbuf(stdout, buf);
	for (i = 0; i < hm; i++) {
		fwrite(zp, 1, BYTES_PER_LINE, stdout);
		lines++;
	}
	for (rn = 0; rn < h; rn++) {
		if (read(0, rp, w) != w)
			break;
		if (ws)
			read(0, sp, ws);
		if (wm)
			fwrite(zp, 1, wm, stdout);
		fwrite(rp, 1, w, stdout);
		if (wm + w < BYTES_PER_LINE)
			fwrite(zp, 1, BYTES_PER_LINE-(w+wm), stdout);
		lines++;
	}
	fflush(stdout);
	return (0);
}

vpltdmp()
{
	register char *cp;
	register int bytes, n;

	cp = buf;
	bytes = 0;
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

#ifndef NO_PR_LOAD
putnstdras()
{
	extern char *malloc();
	extern struct pixrect *pr_load();
	register char *rp, *zp, *sp;
	register int w, h, rn;
	register int i, wm, hm, ws;
	struct pixrect *image;

	/*
	 * Non-std rasterfile that may need filtering - use pr_load.
	 */
	lseek(0, 0, 0);
	image = pr_load(stdin, NULL);
	if ((image == NULL) || (image->pr_depth != 1))
		 return(1);
	w = nbytes(image->pr_width);
	h = image->pr_height;
	if (w < BYTES_PER_LINE) {
		wm = (BYTES_PER_LINE-w)/2;
	} else {
		wm = 0;
		w = BYTES_PER_LINE;
	}
	hm = (PAGE_LINES-h)/2;

	zp = malloc(BYTES_PER_LINE);
	bzero(zp, BYTES_PER_LINE);
	setbuf(stdout, buf);
	for (i = 0; i < hm; i++) {
		fwrite(zp, 1, BYTES_PER_LINE, stdout);
		lines++;
	}
	rp = (char *) ((struct mpr_data *) image->pr_data)->md_image;
	for (rn = 0; rn < h; rn++) {
		if (wm)
			fwrite(zp, 1, wm, stdout);
		fwrite(rp, 1, w, stdout);
		rp += ((struct mpr_data *) image->pr_data)->md_linebytes;
		if (wm + w < BYTES_PER_LINE)
			fwrite(zp, 1, BYTES_PER_LINE-(w+wm), stdout);
		lines++;
	}
	fflush(stdout);
	return (0);
}
#endif
