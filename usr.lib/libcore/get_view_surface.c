/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appears in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */

#ifndef lint
static char sccsid[] = "@(#)get_view_surface.c 1.1 92/07/30 SMI";
#endif


#include <sunwindow/window_hs.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <strings.h>
#include <stdio.h>
#include <usercore.h>

/* 
 * All device-independent/device-dependent routines are referenced in
 * this function.  This means the linker will pull in all of them
 */
int bwdd();
int cgdd();
int gp1dd();
int pixwindd();
int cgpixwindd();
int gp1pixwindd();

static struct vwsurf nullvs = NULL_VWSURF;

static char *devchk;
static int devhaswindows;

/*
 *	Determine from command-line arguments and
 *	the environment a reasonable view surface
 *	for a SunCore program to run on.
 */
int 
get_view_surface(vsptr, argv)
    struct vwsurf *vsptr;
    char **argv;
{
    int devfnd, fd, chkdevhaswindows();
    char *wptr, dev[DEVNAMESIZE], *getenv();
    struct screen screen;
    struct fbgattr attr;

    *vsptr = nullvs;
    devfnd = FALSE;

    /*
     * If command-line arguments are passed, process them using
     * win_initscreenfromargv (see the SunView 1 System Programmer's
     * Guide for details).  The only option used by get_view_surface
     * is the -d option, allowing the user to specify the display device
     * on which to run. 
     */
    if (argv) {
	(void)win_initscreenfromargv(&screen, argv);
	if (screen.scr_fbname[0] != '\0') {
	    /* -d option was found */
	    devfnd = TRUE;
	    (void)strncpy(dev, screen.scr_fbname, DEVNAMESIZE);

	    /*
	     * Check to see if this device has a window system running on it. 
	     * If so devhaswindows will be TRUE following the call to
	     * win_enumall.  win_enumall is a function in libsunwindow.a.  It
	     * takes a function as its argument, and applies this function to
	     * every window being displayed on any screen by the window
	     * system.  To do this it opens each window and passes the
	     * windowfd to the function.  The enumeration continues until all
	     * windows have been tried or the function returns TRUE. 
	     */
	    devchk = dev;
	    devhaswindows = FALSE;
	    (void)win_enumall(chkdevhaswindows);
	}
    }

    if (!devfnd)
	/* No -d option was specified */
	if (wptr = getenv("WINDOW_ME")) {
	    /*
	     * Running in the window system.  Find the device from which this
	     * program was started. 
	     */
	    devhaswindows = TRUE;
	    if ((fd = open(wptr, O_RDWR, 0)) < 0) {
		(void)fprintf(stderr, "get_view_surface: Can't open %s\n",
			wptr);
		return (1);
	    }
	    (void)win_screenget(fd, &screen);
	    (void)close(fd);
	    (void)strncpy(dev, screen.scr_fbname, DEVNAMESIZE);
	}
	else {
	    /*
	     * Not running in the window system.  Assume device is /dev/fb. 
	     */
	    devhaswindows = FALSE;
	    (void)strncpy(dev, "/dev/fb", DEVNAMESIZE);
	}

    /* Now have device name.  Find device type. */
    if ((fd = open(dev, O_RDWR, 0)) < 0) {
	(void) fprintf(stderr, "get_view_surface: cannot open %s\n", dev);
	return (1);
    }

    if (ioctl(fd, FBIOGATTR, &attr) == -1 &&
	attr.fbtype.fb_depth != 1 &&
	attr.fbtype.fb_depth != 8 &&
	ioctl(fd, FBIOGTYPE, &attr.fbtype) == -1) {
	(void)fprintf(stderr, "get_view_surface: FBIOGTYPE failed for %s\n",
 		dev);
	(void)close(fd);
	return (1);
    }
    (void)close(fd);

    /* Now have device type and know if window system is running on it. */
    switch (attr.fbtype.fb_depth) {
    case 1:
	vsptr->dd = devhaswindows ? pixwindd : bwdd;
	break;
    case 8:
	if (attr.fbtype.fb_type == FBTYPE_SUN2GP)
	    vsptr->dd = devhaswindows ? gp1pixwindd : gp1dd;
	else
	    vsptr->dd = devhaswindows ? cgpixwindd : cgdd;
	break;
    default:
	(void)fprintf(stderr,
	    "get_view_surface: cannot handle %d bits framebuffer %s\n",
	    attr.fbtype.fb_depth, dev);
	return (1);
    }

    /* Now SunCore device driver pointer is set up. */
    if (!devhaswindows || devfnd)
	/*
	 * If no window system on device or -d option was specified, tell
	 * SunCore which device.  Otherwise, let SunCore figure out the
	 * device itself from WINDOW_GFX so the default window will be used
	 * if desired. 
	 */
	(void)strncpy(vsptr->screenname, dev, DEVNAMESIZE);
    return (0);
}

/*
 * See if device has windows.
 */
static int 
chkdevhaswindows(windowfd)
    int windowfd;
{
    struct screen windowscreen;

    (void)win_screenget(windowfd, &windowscreen);
    if (strcmp(devchk, windowscreen.scr_fbname) == 0) {

	/*
	 * If this window is on the display device we are checking, set the
	 * flag TRUE.  Return TRUE to terminate the enumeration. 
	 */
	devhaswindows = TRUE;
	return (TRUE);
    }
    return (FALSE);
}
