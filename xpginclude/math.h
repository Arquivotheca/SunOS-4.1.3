/*	@(#)math.h 1.1 92/07/30 SMI; */
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Math library definitions for all the public functions implemented in libm.a.
 * With extensions for xpg2 compatibility
 */

#ifndef _xpgmath_h
#define _xpgmath_h

#ifndef __math_h
#include "../include/math.h"
#endif


#ifndef _values_h

#if u3b || u3b5 || sun
#define MAXFLOAT	((float)3.40282346638528860e+38)
#endif

#if pdp11 || vax
#define MAXFLOAT	((float)1.701411733192644299e+38)
#endif

#if gcos
#define MAXFLOAT	((float)1.7014118219281863150e+38)
#endif

#endif /* _values_h */
#endif /* math_h */
