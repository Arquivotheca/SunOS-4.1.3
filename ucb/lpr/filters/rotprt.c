/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)rotprt.c 1.1 92/07/30 SMI"; /* from UCB 5.1 5/15/85 */
#endif not lint

/*
 * Print a rotated font.
 */

#include <stdio.h>
#include <vfont.h>
#include <sys/types.h>
#include <sys/stat.h>

char	*chp;
char	*sbrk();

main(argc,argv)
char **argv;
{
	struct header h;
	struct dispatch d[256];
	struct stat stb;
	off_t tell();
	int i,size;

	argc--, argv++;
	if (argc > 0) {
		close(0);
		if (open(argv[0], 0) < 0)
			perror(argv[0]), exit(1);
	}
	if (read(0, &h, sizeof(h)) != sizeof(h))
		fprintf(stderr, "header read error\n"), exit(1);
	if (h.magic != 0436)
		fprintf(stderr, "bad magic number\n"), exit(1);
	if (read(0, d, sizeof(d)) != sizeof(d))
		fprintf(stderr, "dispatch read error\n"), exit(1);
	fstat(0, &stb);
	size = stb.st_size - tell(0);
	fprintf(stderr, "%d bytes of characters\n", size);
	chp = sbrk(size);
	read(0, chp, size);
	/* write(1, &h, sizeof (h)); */
	for (i = 0; i < 256; i++)
		rprt(i, &d[i], chp+d[i].addr);
	exit(0);
	/* NOTREACHED */
}

rprt(i, dp, cp)
	int i;
	struct dispatch *dp;
	char *cp;
{
	int bpl, j;

	if (dp->nbytes == 0)
		return;
	if (i >= 0200)
		printf("M-"), i -= 0200;
	if (i < 040)
		printf("^%c", i|'@');
	else if (i == 0177)
		printf("^?");
	else
		printf("%c", i);
	printf("%d bytes, l %d r %d u %d d %d:\n",
	    dp->nbytes, dp->left, dp->right, dp->up, dp->down);
	bpl = (dp->up+dp->down+7)/8;
	for (i = 0; i < dp->right+dp->left; i++) {
		for (j = 0; j < bpl; j++)
			pbits(cp[j]);
		cp += bpl;
		printf("\n");
	}
	printf("========\n");
}

pbits(i)
	register int i;
{
	register int j;

	for (j = 8; j > 0; j--) {
		printf((i & 0x80) ? " *" : "  ");
		i <<= 1;
	}
}
