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
static char sccsid[] = "@(#)ddsubs.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>

int _core_getfb(dev, type, winsys)
char *dev;
int type, winsys;
	{
	char c;
	int fbfd;

	for (c = '0'; c <= '9'; c++)
		{
		dev[10] = c;
		if ((fbfd = _core_chkdev(dev, type, winsys)) >= 0)
			return(fbfd);
		}
	return(-1);
	}

int _core_chkdev(dev, type, winsys)
char *dev;
int type, winsys;
	{
	int fbfd;
	struct fbtype fbtype;

	if ((fbfd = open(dev, O_RDWR, 0)) < 0)
		return(-1);
	if (ioctl(fbfd, FBIOGTYPE, &fbtype) == -1 ||
	    fbtype.fb_type != type)
		{
		close(fbfd);
		return(-1);
		}
	if (winsys)
		;	/* check for already used in window system */
	close(fbfd);
	return(fbfd);
	}
