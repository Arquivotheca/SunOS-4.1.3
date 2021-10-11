#ifndef lint
static	char sccsid[] = "@(#)test_dbl2int.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif
/* Copyright (c) 1989 by Sun Microsystems, Inc. */

#include "libaudio.h"

/* Test symmetry of double<->integer PCM conversions */
main()
{
	char		i, ii;
	short		j, jj;
	long		k, kk;
	double		x;
	int		errs;

	errs = 0;
	for (i = -128; ; i++) {
		x = audio_c2d(i);
		ii = audio_d2c(x);
		if ((i != ii) && ((i != -128) || (ii != -127))) {
			printf("(char) %x: %d -> %f -> %x\n", i, i, x, ii);
			errs++;
		}
		if (i == 127)
			break;
	}
	for (j = -32768; ; j++) {
		x = audio_s2d(j);
		jj = audio_d2s(x);
		if ((j != jj) && ((j != -32768) || (jj != -32767))) {
			printf("(short) %x: %d -> %f -> %x\n", j, j, x, jj);
			errs++;
		}
		if (j == 32767)
			break;
	}
	for (k = -2147483648; ; k += 8003) {
loop:
		x = audio_l2d(k);
		kk = audio_d2l(x);
		if ((k != kk) && ((k != -2147483648) || (kk != -2147483647))) {
			printf("(long) %x: %d -> %f -> %x\n", k, k, x, kk);
			errs++;
		}
		if (k == 2147483647)
			break;
		if (k >= (2147483648 - 8003))
			k = 2147483647 - 8003;
	}
	exit(errs);
}
