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
static char sccsid[] = "@(#)getvwsurfpas.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

#define DEVNAMESIZE 20

typedef struct 	{
    		char screenname[DEVNAMESIZE];
    		char windowname[DEVNAMESIZE];
		int fd;
		int (*dd)();
		int instance;
		int cmapsize;
    		char cmapname[DEVNAMESIZE];
		int flags;
		char *ptr;
		} vwsurf;

int get_view_surface();
extern char** _argv;

int getviewsurface(viewsurf)
vwsurf *viewsurf;
	{
	    char *sptr,*wptr,*cptr;
	    int is,res;
	    
	    res=get_view_surface(viewsurf,_argv);
	    sptr = viewsurf->screenname;
	    is = 0;
	    while ((*sptr++) != '\0') is++;
	    *--sptr = ' ';
	    while (++is < DEVNAMESIZE) *++sptr = ' ';
	    wptr = viewsurf->windowname;
	    is = 0;
	    while ((*wptr++) != '\0') is++;
	    *--wptr = ' ';
	    while (++is < DEVNAMESIZE) *++wptr = ' ';
	    cptr = viewsurf->cmapname;
	    is = 0;
	    while ((*cptr++) != '\0') is++;
	    *--cptr = ' ';
	    while (++is < DEVNAMESIZE) *++cptr = ' ';
	return(res);
	}

