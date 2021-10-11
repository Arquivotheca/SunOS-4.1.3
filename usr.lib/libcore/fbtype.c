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
static char sccsid[] = "@(#)fbtype.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
#include "usercore.h"
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include        <sunwindow/window_hs.h>
/*
 * Get the framebuffer type of the current window or raw display
 */
_core_fbtype()
{
    struct fbtype fbtype;
    char name[DEVNAMESIZE];
    int gwfd, fbfd;
    struct screen screen;

	if (we_getgfxwindow(name)) {			/* if no suntools */
		strncpy( name, "/dev/fb", DEVNAMESIZE);
		if ((fbfd = open(name, O_RDWR, 0)) < 0)
			return(-1);
	} else {
		if ((gwfd = open(name, O_RDWR, 0)) < 0)
			return(-1);
		win_screenget(gwfd, &screen);
		if ((fbfd = open(screen.scr_fbname, O_RDWR, 0)) < 0) {
			close(gwfd);
			return(-1);
		}
		close(gwfd);
	}
	ioctl(fbfd, FBIOGTYPE, &fbtype);
	close(fbfd);
	return( fbtype.fb_type);
}
