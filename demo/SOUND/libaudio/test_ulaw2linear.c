#ifndef lint
static	char sccsid[] = "@(#)test_ulaw2linear.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif
/* Copyright (c) 1989 by Sun Microsystems, Inc. */

#include "ulaw2linear.h"

/* Test symmetry of u-law<->integer PCM conversions */
main()
{
	int		i;
	short		j;
	unsigned char	k;
	int		l;
	char		m;
	char		n;
	unsigned char	o;
	int		errs;

	errs = 0;
	for (i = 0; i < 0x100; i++) {
		j = audio_u2s(i);
		k = audio_s2u(j);
		if ((k != (unsigned char)i) && (j != 0)) {
			printf("(short) %x: %d -> %x\n", i, j, k);
			errs++;
		}
		l = audio_u2l(i);
		k = audio_l2u(l);
		if ((k != (unsigned char)i) && (l != 0)) {
			printf("(long)  %x: %d -> %x\n", i, l, k);
			errs++;
		}
		m = audio_u2c(i);
		k = audio_c2u(m);
		n = audio_u2c(k);
		if (m != n) {
			printf("(char)  %x: %d -> %x -> %d\n", i, m, k, n);
			errs++;
		}
	}
	for (i = -0x80; i < 0x80; i++) {
		k = audio_c2u(i);
		m = audio_u2c(k);
		o = audio_c2u(m);
		if (k != o) {
			printf("(char)  %d -> %x -> %d -> %x\n", i, k, m, o);
			errs++;
		}
	}
	exit(errs);
}
