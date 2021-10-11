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
c/* @(#)usercore77.h 1.1 92/07/30 */

c/*
c * Copyright (c) 1983 by Sun Microsystems, Inc.
c */


	integer TRUE, FALSE
	integer STRING, CHARACTER
	integer MAXVSURF
	integer PARALLEL, PERSPECTIVE
	integer NONE, XLATE2, XFORM2, XLATE3, XFORM3
	integer SOLID, DOTTED, DASHED, DOTDASHED
	integer CONSTANT, GOURAUD, PHONG
	integer PICK, KEYBOARD, BUTTON, LOCATOR, VALUATOR, STROKE
	integer ROMAN, GREEK, SCRIPT, OLDENGLISH, STICK, SYMBOLS
	integer OFF, LEFT, CENTER, RIGHT
	integer NORMAL, XORROP, ORROP
	integer PLAIN, SHADED
	integer BASIC, BUFFERED, DYNAMICA, DYNAMICB, DYNAMICC
	integer NOINPUT, SYNCHRONOUS, COMPLETE
	integer TWOD, THREED
	integer VWSURFSIZE, DDINDEX, VWSURFNEWFLG

	parameter(TRUE=1)
	parameter(FALSE=0)

c				/* Charprecision constants */
	parameter(STRING=0)
	parameter(CHARACTER=1)

c				/* View surfaces; maximum number of */
	parameter(MAXVSURF=5)

c				/* Transform constants */
	parameter(PARALLEL=0)
	parameter(PERSPECTIVE=1)

c				/* Image transformation types */
	parameter(NONE=1)
	parameter(XLATE2=2)
	parameter(XFORM2=3)
	parameter(XLATE3=2)
	parameter(XFORM3=3)

c				/* Line styles */
	parameter(SOLID	=0)
	parameter(DOTTED=1)
	parameter(DASHED=2)
	parameter(DOTDASHED=3)

c				/* Polygon shading modes */
	parameter(CONSTANT=0)
	parameter(GOURAUD=1)
	parameter(PHONG=2)

c				/* Input device constants */
	parameter(PICK=0)
	parameter(KEYBOARD=1)
	parameter(BUTTON=2)
	parameter(LOCATOR=3)
	parameter(VALUATOR=4)
	parameter(STROKE=5)

c				/* Font select constants */
	parameter(ROMAN	=0)
	parameter(GREEK	=1)
	parameter(SCRIPT=2)
	parameter(OLDENGLISH=3)
	parameter(STICK	=4)
	parameter(SYMBOLS=5)

c				/* Character justification constants */
	parameter(OFF=0)
	parameter(LEFT=1)
	parameter(CENTER=2)
	parameter(RIGHT=3)

c				/* Rasterop selection constants */
	parameter(NORMAL=0)
	parameter(XORROP=1)
	parameter(ORROP=2)

c				/* Polygon interior style constants */
	parameter(PLAIN=0)
	parameter(SHADED=1)

c				/* Core output levels */
	parameter(BASIC=0)
	parameter(BUFFERED=1)
	parameter(DYNAMICA=2)
	parameter(DYNAMICB=3)
	parameter(DYNAMICC=4)

c				/* Core input levels */
	parameter(NOINPUT=0)
	parameter(SYNCHRONOUS=1)
	parameter(COMPLETE=2)

c				/* Core dimensions */
	parameter(TWOD=0)
	parameter(THREED=1)

c				/* vwsurf struct constants */
	parameter(VWSURFSIZE=21)
	parameter(DDINDEX=12)
	parameter(VWSURFNEWFLG=1)
