
/*	@(#)unpkd.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * routines in q.s and unpkd.s also depend on these structures,
 * so they are changed at great risk.
 */
/*
 * the mantissa here has an explicit high-order bit.
 * the value is of the form 1.mmm * 2^exp
 */
struct unpkd { short exp; unsigned mantissa[2] };
struct quad  { unsigned q[2]; };
#define HOB	0x80000000 /* high-order-bit */
#define LTWO10	0x4d1
#define DOUBLEBIAS	1023
/*
 * 10^27 is the largest power of ten that can be represented exactly in
 * unpacked form. Thus _d_pot[] runs from 10^0 ... 10^27, for a total
 * of 28 entries. This also governs the size of _d_r_pot[].
 */
#define N_D_POT	28

extern struct unpkd _d_r_pot[], _d_r_big_pot[], _d_pot[], _d_big_pot[];

