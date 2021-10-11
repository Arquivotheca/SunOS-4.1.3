#ifndef lint
static char     sccsid[] = "@(#)ieee_retro.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/ieeefp.h>

/* Print out retrospective information about unrequited IEEE exceptions. */

void
ieee_retrospective(f)
	FILE           *f;
{
	int             ie;
	char            out[20];
	char           *pout = out;
	int             ieeemask = (1 << fp_division) | (1 << fp_underflow) | (1 << fp_overflow) | (1 << fp_invalid);
	int             _is_nonstandard();

	ie = ieee_flags("get", "exception", "all", &pout);
	if (ie & ieeemask) {
		fprintf(f, " Warning: the following IEEE floating-point arithmetic exceptions \n");
		fprintf(f, " occurred in this program and were never cleared: \n");
		if (ie & (1 << fp_inexact))
			fprintf(f, " Inexact; ");
		if (ie & (1 << fp_division))
			fprintf(f, " Division by Zero; ");
		if (ie & (1 << fp_underflow))
			fprintf(f, " Underflow; ");
		if (ie & (1 << fp_overflow))
			fprintf(f, " Overflow; ");
		if (ie & (1 << fp_invalid))
			fprintf(f, " Invalid Operand; ");
		fprintf(f, "\n");
		fflush(f);
	}
	if (_is_nonstandard() != 0) {
		fprintf(f, " Warning: nonstandard floating-point arithmetic mode \n");
		fprintf(f, " was enabled in this program and was never cleared. \n");
		fflush(f);
	}
}

void
ieee_retrospective_()
{
	ieee_retrospective(stderr);
}
