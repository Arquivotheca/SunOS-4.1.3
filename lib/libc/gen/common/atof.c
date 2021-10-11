#if !defined(lint) && defined(SCCSIDS)
static char     sccsid[] = "@(#)atof.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc. 
 */

#include <stdio.h>

extern double 
strtod();

double
atof(cp)
	char           *cp;
{
	return strtod(cp, (char **) NULL);
}
