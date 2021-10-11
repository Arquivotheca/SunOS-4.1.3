#ifndef lint
static char     sccsid[] = "@(#)underflow.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Routines to control underflow handling. These are no-ops except on
 * machines for which abrupt underflow is significantly faster.
 */

gradual_underflow_()
{				/* Default. */
	standard_arithmetic();
}

abrupt_underflow_()
{				/* For Weitek-style implementations. */
	nonstandard_arithmetic();
}
