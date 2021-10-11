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
static char sccsid[] = "@(#)inqinput77.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "f77strings.h"
#include "../libcore/coretypes.h"

int inquire_echo();
int inquire_echo_position();
int inquire_echo_surface();
int inquire_keyboard();
int inquire_locator_2();
int inquire_stroke();
int inquire_valuator();

int inqecho_(devclass, devnum, echotype)
int *devclass, *devnum, *echotype;
	{
	return(inquire_echo(*devclass, *devnum, echotype));
	}

int inqechoposition_(devclass, devnum, x, y)
int *devclass, *devnum;
float *x, *y;
	{
	return(inquire_echo_position(*devclass, *devnum, x, y));
	}

#define DEVNAMESIZE 20

int inqechosurface_(devclass, devnum, surfname)
int *devclass, *devnum;
struct vwsurf *surfname;
	{
	return(inquire_echo_surface(*devclass, *devnum, surfname));
	}

int inqkeyboard_(keynum, bufsize, istr, pos, f77strleng)
int *keynum, *bufsize, *pos, f77strleng;
char *istr;
	{
	char c, *sptr;
	int f, n;

	f = inquire_keyboard(*keynum, bufsize, _core_77string, pos);
	sptr = _core_77string;
	n = 1;
	while ((c = *sptr++) && (n++ <= f77strleng))
		*istr++ = c;
	while (n++ <= f77strleng)
		*istr++ = ' ';
	return(f);
	}

int inqlocator2_(locnum, x, y)
int *locnum;
float *x, *y;
	{
	return(inquire_locator_2(*locnum, x, y));
	}

int inqstroke_(strokenum, bufsize, dist, time)
int *strokenum, *bufsize, *time;
float *dist;
	{
	return(inquire_stroke(*strokenum, bufsize, dist, time));
	}

int inqvaluator_(valnum, init, low, high)
int *valnum;
float *init, *low, *high;
	{
	return(inquire_valuator(*valnum, init, low, high));
	}
