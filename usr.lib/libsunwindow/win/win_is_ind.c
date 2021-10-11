#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)win_is_ind.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Win_is_ind.c: Implement the win_is_input_device call.
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
#include <strings.h>

extern	int	errno;

int
win_is_input_device(windowfd, name)
	int windowfd;
	char *name;
{
	struct	input_device in_dev;

	(void)strncpy(in_dev.name, name, SCR_NAMESIZE);
	in_dev.id = -1;
	if (ioctl(windowfd, WINGETINPUTDEV, &in_dev) == -1) {
		if (errno == ENODEV)
			return(0);
		(void)werror(-1, WINGETINPUTDEV);
		return(-1);
	}
	return(1);
}

