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
static char sccsid[] = "@(#)savesegment77.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "f77strings.h"

int restore_segment();
int save_segment();

int restoresegment_(segname, filename, f77strleng)
char *filename;
int *segname, f77strleng;
	{
	char *sptr;

	sptr = filename + f77strleng - 1;
	while (*sptr-- == ' ')
		f77strleng--;
	f77strleng = f77strleng > MAXLEN ? MAXLEN : f77strleng;
	sptr = _core_77string;
	while (f77strleng--)
		*sptr++ = *filename++;
	*sptr = '\0';
	return(restore_segment(*segname, _core_77string));
	}

int savesegment_(segnum, filename, f77strleng)
char *filename;
int *segnum, f77strleng;
	{
	char *sptr;

	sptr = filename + f77strleng - 1;
	while (*sptr-- == ' ')
		f77strleng--;
	f77strleng = f77strleng > MAXLEN ? MAXLEN : f77strleng;
	sptr = _core_77string;
	while (f77strleng--)
		*sptr++ = *filename++;
	*sptr = '\0';
	return(save_segment(*segnum, _core_77string));
	}
