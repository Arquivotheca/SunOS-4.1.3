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
static char sccsid[] = "@(#)inqtextent77.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "f77strings.h"

int inquire_text_extent_2();
int inquire_text_extent_3();

int inqtextextent2_(s, dx, dy, f77strleng)
char *s;
float *dx, *dy;
int f77strleng;
	{
	char *sptr;

	f77strleng = f77strleng > MAXLEN ? MAXLEN : f77strleng;
	sptr = _core_77string;
	while (f77strleng--)
		*sptr++ = *s++;
	*sptr = '\0';
	return(inquire_text_extent_2(_core_77string, dx, dy));
	}

int inqtextextent3_(s, dx, dy, dz, f77strleng)
char *s;
float *dx, *dy, *dz;
int f77strleng;
	{
	char *sptr;

	f77strleng = f77strleng > MAXLEN ? MAXLEN : f77strleng;
	sptr = _core_77string;
	while (f77strleng--)
		*sptr++ = *s++;
	*sptr = '\0';
	return(inquire_text_extent_3(_core_77string, dx, dy, dz));
	}
