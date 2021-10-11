/*	@(#)align.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Storage alignment parameters for sparc compilers.
 * In "lint", we can't #define SZCHAR, and the like, because they
 * are variables (since if "-p" is specified values are chosen that
 * will cause as many errors to be caught as possible).
 */

# ifndef _align_
# define _align_ 1

# define  _SZCHAR 8
# define  _SZINT 32
# define  _SZFLOAT 32
# define  _SZDOUBLE 64
# define  _SZLONG 32
# define  _SZSHORT 16
# define  _SZPOINT 32

# define  _ALCHAR 8
# define  _ALINT 32
# define  _ALFLOAT 32
# define  _ALDOUBLE 64
# define  _ALLONG 32
# define  _ALSHORT 16
# define  _ALPOINT 32
# define  _ALSTRUCT 8
# define  _ALSTACK 64

#ifndef LINT
# define SZCHAR		_SZCHAR
# define SZINT		_SZINT
# define SZFLOAT	_SZFLOAT
# define SZDOUBLE	_SZDOUBLE
# define SZEXTENDED	_SZEXTENDED
# define SZLONG		_SZLONG
# define SZSHORT	_SZSHORT
# define SZPOINT	_SZPOINT

# define ALCHAR		_ALCHAR
# define ALINT		_ALINT
# define ALFLOAT	_ALFLOAT
# define ALDOUBLE	_ALDOUBLE
# define ALEXTENDED	_ALEXTENDED
# define ALLONG		_ALLONG
# define ALSHORT	_ALSHORT
# define ALPOINT	_ALPOINT
# define ALSTRUCT	_ALSTRUCT
# define ALSTACK	_ALSTACK
#endif

# endif _align_
