#ifndef lint
static	char sccsid[] = "@(#)vswap.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Converts byte-swapped vfont files.
 */
#include <stdio.h>
#include <vfont.h>

struct header	header;
struct dispatch	dispatch[NUM_DISPATCH];
unsigned char	bitmaps[BUFSIZ];
int reverse;

#define byteswap(x)	( (((x)>>8)&255) | (((x)&255)<<8) )

main(argc, argv)
	int argc;
	char **argv;
{
	int n;
	
	if (argc > 1 && strcmp(argv[1], "-r") == 0)
		reverse++;
	if (fread(&header, sizeof(header), 1, stdin) != 1)
		pexit(1, "header read error");
	if (reverse) {
		if (byteswap(header.magic) == VFONT_MAGIC)
			pexit(0, "already in reversed vfont format");
		/* Convert the byte-swapped fields in the header */
		if (header.magic != VFONT_MAGIC)
			pexit(1, "not a vfont file");
		header.magic = byteswap(header.magic);
	} else {
		if (header.magic == VFONT_MAGIC)
			pexit(0, "already in vfont format");
		/* Convert the byte-swapped fields in the header */
		header.magic = byteswap(header.magic);
		if (header.magic != VFONT_MAGIC)
			pexit(1, "not a vfont file");
	}
	header.size = byteswap(header.size);
	header.maxx = byteswap(header.maxx);
	header.maxy = byteswap(header.maxy);
	header.xtend = byteswap(header.xtend);
	if (fwrite(&header, sizeof(header), 1, stdout) != 1)
		pexit(2, "header write error");

	if (fread(dispatch, sizeof(dispatch), 1, stdin) != 1)
		pexit(1, "dispatch table read error");
	/* Convert the byte-swapped fields in the dispatch table */
	for (n = 0; n < NUM_DISPATCH; n++) {
		dispatch[n].addr   = byteswap(dispatch[n].addr);
		dispatch[n].nbytes = byteswap(dispatch[n].nbytes);
		dispatch[n].width  = byteswap(dispatch[n].width);
	}
	if (fwrite(dispatch, sizeof(dispatch), 1, stdout) != 1)
		pexit(2, "dispathc table write error");

	/* Now copy the bit maps over */
	while ((n = fread(bitmaps, 1, sizeof(bitmaps), stdin)) > 0)
		if (fwrite(bitmaps, 1, n, stdout) != n)
			pexit(2, "bitmap write error");
	exit(0);
	/* NOTREACHED */
}


/* Print error and exit. */
pexit(n, s)
	int n;
	char *s;
{
	fprintf(stderr, "vswap: %s\n", s);
	exit(n);
}
