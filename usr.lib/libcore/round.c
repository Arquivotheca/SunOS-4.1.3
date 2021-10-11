/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */

#ifndef lint
static char sccsid[] = "@(#)round.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Round a IEEE single precision floating pt number to an integer
 * without doing floating point math.
 */
_core_roundf(f)
	register float *f;
{
	register int ival, trunc;
	register char exp;

	trunc = *f;
	ival = *(int *)f;
	exp = (((ival & 0x7F800000) >> 23) - 125);
	if (exp == 0)
		goto alter;

	if ( (exp >= 24) || (exp < 0) )
		return(trunc);
	else
		if ((ival & (1<<(23 - exp))) == 0)
			return(trunc);

alter:
	if ((ival & 0x80000000) == 0)
		return (++trunc);
	else
		return (--trunc);
}
