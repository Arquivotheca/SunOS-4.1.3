#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)win_enum_ind.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Win_enum_ind.c: Implement the win_enum_input_device call.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <errno.h>
#include <stdio.h>
#include <sunwindow/rect.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/win_struct.h>

int	win_enum_many = 30;	/* Max cap on number of input devices
				   to prevent run away enumeration. */

/*
 * Returns -1 if error enumerating, 0 if went smoothly and
 * 1 if func terminated enumeration.
 */
int
win_enum_input_device(windowfd, func, data)
	int windowfd;
	int (*func)();
	caddr_t data;
{
	struct	input_device in_dev;
	extern	errno;

	for (in_dev.id = 0; in_dev.id < win_enum_many; in_dev.id++) {
		/* See if id is a valid input device */
		in_dev.name[0] = '\0';
		if (ioctl(windowfd, WINGETINPUTDEV, &in_dev) == -1) {
			if (errno == ENODEV)
				return(0);
			(void)werror(-1, WINGETINPUTDEV);
			return(-1);
		}
		/* Call function.  If returns non-zero value then return 1. */
		if ((*func)(in_dev.name, data))
			return(1);
	}
	return(0);
}


