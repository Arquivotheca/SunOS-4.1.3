#ifndef lint
static char	sccsid[] = "@(#)metanew77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI initialization functions
 */

/*
int open_vws
*/

#include "cgidefs.h"
#include "cf77.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfopenvws                                                  */
/*                                                                          */
/*                                                                          */
/*		 pertainent view surface information is stored and the view */
/*		 surface is added to the list of view surfaces 		    */
/****************************************************************************/

int  cfopenvws_ (name,screenname,windowname,windowfd,retained,dd,cmapsize,
	cmapname,flags,ptr,noargs,f77strlen1,f77strlen2,f77strlen3,f77strlen4)
int *name;
char *screenname; /* physical screen */
char *windowname; /* window */
int *windowfd;		  /* window file */
int *retained;		  /* retained flag */
int *dd;			  /* device */
int *cmapsize;		  /* color map size */
char *cmapname;   /* color map name */
int *flags;			  /* new flag */
char *ptr;			  /* CGI tool descriptor */
int	*noargs;	/* number of string entries in 'ptr' */
int	f77strlen1;	/* length of screenname */
int	f77strlen2;	/* length of windowname */
int	f77strlen3;	/* length of cmapname */
int	f77strlen4;	/* length of ptr buffers */
{
Cvwsurf  devdd;
    char	*vec[2], buf[MAXCHAR];

    PASS_STRING(devdd.screenname, screenname, f77strlen1, DEVNAMESIZE);
    PASS_STRING(devdd.windowname, windowname, f77strlen2, DEVNAMESIZE);
    devdd.windowfd = *windowfd;
    devdd.retained = *retained;
    devdd.dd = *dd;
    devdd.cmapsize = *cmapsize;
    PASS_STRING(devdd.cmapname, cmapname, f77strlen3, DEVNAMESIZE);
    devdd.flags = *flags;
    devdd.ptr = vec;
    vec[0] = buf;
    vec[1] = 0;
    PASS_STRING(buf, ptr, f77strlen4, MAXCHAR);
    return (open_vws (name,&devdd));
}
