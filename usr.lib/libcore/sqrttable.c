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
static char sccsid[] = "@(#)sqrttable.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Integer square root lookup table.
 */
unsigned _core_sqrttable[] = {
    0,     2896,  4096,  5016,  5792,  6476,  7094,  7662,  8192,  8688,
    9158,  9605,  10033, 10442, 10836, 11217, 11585, 11941, 12288, 12624,
    12952, 13272, 13584, 13890, 14188, 14481, 14768, 15049, 15325, 15597,
    15863, 16125, 16384, 16638, 16888, 17134, 17377, 17617, 17854, 18087,
    18317, 18545, 18770, 18992, 19211, 19429, 19643, 19856, 20066, 20274,
    20480, 20683, 20885, 21085, 21283, 21479, 21673, 21866, 22057, 22246,
    22434, 22620, 22805, 22988, 23170, 23350, 23529, 23707, 23883, 24058,
    24232, 24404, 24576, 24746, 24914, 25082, 25249, 25415, 25579, 25742,
    25905, 26066, 26227, 26386, 26545, 26702, 26859, 27014, 27169, 27323,
    27476, 27629, 27780, 27930, 28080, 28229, 28377, 28525, 28672, 28817,
    28963, 29107, 29251, 29394, 29536, 29678, 29819, 29959, 30099, 30238,
    30376, 30514, 30651, 30788, 30924, 31059, 31194, 31328, 31461, 31595,
    31727, 31859, 31990, 32121, 32251, 32381, 32510, 32639, 32768, 32896,
    33023, 33150, 33277, 33403, 33528, 33653, 33778, 33902, 34026, 34149,
    34269, 34391, 34513, 34634, 34755, 34876, 34996, 35115, 35235, 35354 };
