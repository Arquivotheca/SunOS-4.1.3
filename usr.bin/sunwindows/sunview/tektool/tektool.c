#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tektool.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Author: Steve Kleiman
 *
 * Tektool: tektronix 4014 emulator tool.
 * Like shelltool only it looks like a 4014.
 */

#include <stdio.h>
#include <sys/wait.h>

#include <suntool/sunview.h>
#include "teksw.h"

static	short icon_data[256] = {
#include <images/tektool.icon>
};

mpr_static(tekic_mpr, 64, 64, 1, icon_data);

static struct icon icon = {
	64, 64,
	(struct pixrect *) 0,
	0, 0, 64, 64,
	&tekic_mpr,
	0, 0, 0, 0,
	(char *) 0,
	(struct pixfont *) 0,
	ICON_BKGRDGRY
};


#ifdef STANDALONE
main(argc, argv)
#else
tektool_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	Window tool_frame;
	Window tek;
	Rect *rectp;
	Rect nullrect;
	int child_pid;
	extern Window teksw_create();
	extern Notify_value wait_child();

	/*
	 * We want tektool to fill the screen unless the user overrides
	 * with a command line arg. Since FRAME_ARGC_PTR_ARGV only works
	 * in window_create() we set a null open rect which will remain
	 * unless the user overrides.
	 */
	rect_construct(&nullrect, 0, 0, 0, 0);
	tool_frame = window_create(NULL, FRAME,
		FRAME_OPEN_RECT, &nullrect,
		FRAME_ICON, &icon,
		FRAME_LABEL, "tektool",
		FRAME_SHOW_LABEL, TRUE,
		FRAME_ARGC_PTR_ARGV, &argc, argv,
		0);
	if (tool_frame == NULL) {
		fprintf(stderr, "Cannot create base frame. Process aborted.\n");
		exit(1);
	}
	rectp = (Rect *)window_get(tool_frame, FRAME_OPEN_RECT);
	if (rect_isnull(rectp)) {
		rectp = (Rect *)window_get(tool_frame, WIN_SCREEN_RECT);
		window_set(tool_frame,
			FRAME_OPEN_RECT, rectp,
			0);
	}
	tek = teksw_create(tool_frame, argc, argv);
	if (tek == NULL) {
		exit(1);
	}
	window_set(tool_frame,
		WIN_CLIENT_DATA, tek,
		0);
	child_pid = teksw_fork(tek, argc, argv);
	if (child_pid == -1) {
		exit (1);
	}
	(void) notify_set_wait3_func(tool_frame, wait_child, child_pid);
	window_main_loop(tool_frame);
	exit(0);
}

static Notify_value
wait_child(tool_frame, pid, status, rusage)
	Window tool_frame;
	int pid;
	union wait *status;
	struct rusage *rusage;
{

	if (WIFSTOPPED(*status)) {
		return (NOTIFY_IGNORED);
	} else {
		window_set(tool_frame,
			FRAME_NO_CONFIRM, TRUE,
			0);
		window_done(tool_frame);
		return (NOTIFY_DONE);
	}
}
