/*	@(#)usercorepas.h 1.1 92/07/30 SMI	*/
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
	const
	TRUE=1;
	FALSE=0;

				{ Charprecision constants }
	STRING=0;
	CHARACTER=1;

				{ View surfaces; maximum number of}
	MAXVSURF=5;

				{ Transform constants}
	PARALLEL=0;
	PERSPECTIVE=1;

				{ Image transformation types }
	NONE=1;
	XLATE2=2;
	XFORM2=3;
	XLATE3=2;
	XFORM3=3;

				{ Line styles }
	SOLID	=0;
	DOTTED=1;
	DASHED=2;
	DOTDASHED=3;

				{ Polygon shading modes }
	CONSTANT=0;
	GOURAUD=1;
	PHONG=2;

				{ Input device constants }
	PICK=0;
	KEYBOARD=1;
	BUTTON=2;
	LOCATOR=3;
	VALUATOR=4;
	STROKE=5;

				{ Font select constants}
	ROMAN	=0;
	GREEK	=1;
	SCRIPT=2;
	OLDENGLISH=3;
	STICK	=4;
	SYMBOLS=5;

				{ Character justification constants }
	OFF=0;
	LEFT=1;
	CENTER=2;
	RIGHT=3;

				{ Rasterop selection constants}
	NORMAL=0;
	XORROP=1;
	ORROP=2;

				{ Polygon interior style constants }
	PLAIN=0;
	SHADED=1;

				{ Core output levels }
	BASIC=0;
	BUFFERED=1;
	DYNAMICA=2;
	DYNAMICB=3;
	DYNAMICC=4;

				{ Core input levels }
	NOINPUT=0;
	SYNCHRONOUS=1;
	COMPLETE=2;

				{ Core dimensions }
	TWOD=0;
	THREED=1;

	DEVNAMESIZE = 20;
	VWSURFNEWFLG = 1;
