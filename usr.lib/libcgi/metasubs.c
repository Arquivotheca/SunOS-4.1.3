#ifndef lint
static char	sccsid[] = "@(#)metasubs.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI Color initialization functions
 */

#include <sys/file.h>
#include <stdio.h>
#include <errno.h>
#include "cgipriv.h"
#include        <sunwindow/cms.h>

/*
 * _cgi_sharedcmap accesses all windows on a specified device and if they
 * have a colormap of specified name, returns the colormap.  An error(0) return
 * results if no windows have the named colormap or there are no windows.
 */

/*   
buglist: colormap
*/
static int      windownotactive;
static int      (*wmgr_errorcached) ();

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_sharedcmap 					    */
/*                                                                          */
/*		shared colormap routine					    */
/****************************************************************************/

_cgi_sharedcmap(wfd, name, dcms, dcmap)
int             wfd;
char           *name;
struct colormapseg *dcms;
struct cms_map *dcmap;
{
    char            winname[WIN_NAMESIZE];
    int             testwfd, winnumber, satisfied = 0;
    struct screen   screen, newscreen;
    struct colormapseg scms;
    struct cms_map  cmap;
    extern int      errno;

    /*
     * Get the base screen 
     */
    win_screenget(wfd, &screen);
    cmap.cm_red = cmap.cm_green = cmap.cm_blue = 0;
    /*
     * Find window numbers associated with wfd's device. 
     */
    for (winnumber = 0;; winnumber++)
    {
	int             _cgi_screenget_error();
	extern int      (*win_errorhandler()) ();
	int             (*prev_error_rtn) ();
	/*
	 * win_errorhandler takes 1 arg: a ptr to an int-returning func, the
	 * desired window error handling routine. win_errorhandler returns:   a
	 * ptr to an int-returning func, the previous window error handling
	 * routine. 
	 */
	/*
	 * Open window. 
	 */
	win_numbertoname(winnumber, winname);
	if ((testwfd = open(winname, O_RDONLY, 0)) < 0)
	{
	    if (errno == ENXIO || errno == ENOENT)
		break;		/* Last window passed. */
	    goto badopen;
	}
	/*
	 * Get window's device name (if active). 
	 */
	prev_error_rtn = win_errorhandler(_cgi_screenget_error);
	/* Avoid recursion */
	if (prev_error_rtn != _cgi_screenget_error)
	    wmgr_errorcached = prev_error_rtn;
	windownotactive = 0;
	win_screenget(testwfd, &newscreen);
	if (windownotactive)
	{
	    (void) close(testwfd);
	    continue;
	}
	/* Put back previous error handler */
	(void) win_errorhandler(wmgr_errorcached);
	/*
	 * Is this window on the same screen 
	 */
	if (strncmp(screen.scr_fbname, newscreen.scr_fbname,
		    SCR_NAMESIZE) == 0)
	{
	    /*
	     * Does it have the named colormap 
	     */
	    win_getcms(testwfd, &scms, &cmap);
	    if (strncmp(name, scms.cms_name, CMS_NAMESIZE) == 0)
	    {
/*
				scms.cms_size = 0;
*/
		win_getcms(testwfd, &scms, dcmap);
		if (dcms)
		    *dcms = scms;
		satisfied = 1;
		(void) close(testwfd);
		break;
	    }
	}
	(void) close(testwfd);
    }
    return (satisfied);
badopen:return (0);
}

_cgi_screenget_error(errnum, winopnum)
int             errnum, winopnum;
{
    switch (errnum)
    {
    case 0:
	return;
    case -1:
	if (errno == ESPIPE)
	{
	    windownotactive = 1;
	    return;
	}
    }
    if (wmgr_errorcached)
	wmgr_errorcached(errnum, winopnum);
    return;
}
