#ifndef lint
static char	sccsid[] = "@(#)input277.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
#endif

/*
 * Copyright (c) 1985, 1986, 1987, 1988, 1989 by Sun Microsystems, Inc. 
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
/*
 * CGI Input functions
 */

#include "cgidefs.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfassoc		 	  		    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cfassoc_ (trigger, devclass, devnum)
int     *trigger;		/* trigger number */
int *devclass;
int     *devnum;			/* device type, device number */
{
    return (associate ((Cint) *trigger, (Cdevoff) *devclass, (Cint) *devnum));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfsdefatrigassoc & cfsdefatrigassoc_ 	  		    */
/*                                                                          */
/*              NOTE: underbar '_' is missing because FORTRAN uses 16       */
/*                    characters from name and follows it with no '_'.      */
/*		Note2: This was fixed in Fortran 1.1, so added a new entry  */
/*                                                                          */
/****************************************************************************/

int       cfsdefatrigassoc (devclass, devnum)
int *devclass; /* device type */
int *devnum; /* device number */
{
	return (set_default_trigger_associations((Cdevoff) *devclass,
		    (Cint) *devnum));
}

int       cfsdefatrigassoc_ (devclass, devnum)
int *devclass; /* device type */
int *devnum; /* device number */
{
	return (set_default_trigger_associations((Cdevoff) *devclass,
		    (Cint) *devnum));
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfdissoc		 	  		    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cfdissoc_ (trigger, devclass, devnum)
int     *trigger;		/* trigger number */
int *devclass;
int     *devnum;			/* device type, device number */
{
    return (dissociate ((Cint) *trigger, (Cdevoff) *devclass, (Cint) *devnum));
}
