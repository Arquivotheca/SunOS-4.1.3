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
static char sccsid[] = "@(#)savesegpas.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

#include "f77strings.h"

int restore_segment();
int save_segment();

int restoresegment(segname, filename)
char *filename;
int segname;
	{
	char *sptr;
	char pasarg[257];
	int i,strlen;

	strlen = 256;
	sptr = filename+256;
	while ((*--sptr) == ' ') {strlen--;};
	strncpy (pasarg,filename,strlen);
	pasarg[strlen] = '\0';
	i = restore_segment(segname,filename);
	return(i);
	}

int savesegment(segnum, filename)
char *filename;
int segnum;
	{
	char *sptr;
	char pasarg[257];
	int i,strlen;

	strlen = 256;
	sptr = filename+256;
	while ((*--sptr) == ' ') {strlen--;};
	strncpy (pasarg,filename,strlen);
	pasarg[strlen] = '\0';
	i = save_segment(segnum,pasarg);
	return(i);
	}
