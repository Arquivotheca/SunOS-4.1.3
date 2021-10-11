/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appears in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */

/*
 * @(#)langnames.h 1.1 92/07/30 SMI
 */

/*
 * Copyright 1984-1989 Sun Microsystems, Inc.
 */

/*
 * This file contains macros for defining entry points
 * for pascal and fortran programs. This used to be done with asm()
 * statements that would generate equates. Now, asm() statements
 * disable optimization, so the macros simply generate wrapper
 * functions.
 *
 * At one time Pascal routines prefaced with 2 extra underscores were
 * necessary, but this is no longer true.
 *
 * The DDFUNC, CNAME and GENDD macros are for generating shared library
 * .sa file stubs.
 */

#ifdef SHLIB
#define	_CNAME(X) _core_/**/X
#define	DDFUNC(X) _CNAME(X)
#else SHLIB
#define	_CNAME(X) X
#define	DDFUNC(X) \
FORTNAME(X) \
PASCNAME(X) \
_CNAME(X)
#endif SHLIB

#ifndef BW_FB
#define	ddargtype char
#endif BW_FB

#define	CNAME(X) \
X(ddstruct) \
	register ddargtype *ddstruct; \
{ \
	return _CNAME(X)(ddstruct); \
}

#define FORTNAME(X) \
X/**/_(ddstruct) \
	register ddargtype *ddstruct; \
{ \
	return _CNAME(X)(ddstruct); \
}

#define PASCNAME(X) /* nothing */

#define	GENDD(gen,func,type,name) \
func(dd) \
	ddargtype *dd; \
{ \
	return gen(dd, type, name); \
} \
FORTNAME(func) \
PASCNAME(func)
