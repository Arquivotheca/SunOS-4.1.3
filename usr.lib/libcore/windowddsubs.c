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

#if !defined(lint) && !defined(NOID)
static char sccsid[] = "@(#)windowddsubs.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1986-1989, Sun Microsystems, Inc.
 */

#include	<sunwindow/window_hs.h>
#include	<strings.h>
#include	<vfork.h>
#include	"coretypes.h"
#include	"corevars.h"
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#define NULL 0

int _core_gfxenvfd = -1;

static char *srchptr;
static struct screen  *scrptr;
static int foundscreen;

_core_get_window_vwsurf(pwprfdp, pidp, colorflg, vsurfp)
int *pwprfdp, *pidp, colorflg;
struct vwsurf *vsurfp;
	{
	int i1, i2;
	static char name[DEVNAMESIZE];
	static char *argvarray[] = {"view_surface", name, 0};
	static char initstring[80] = "WINDOW_INITIALDATA=";
	static char *envparray[] = {0, 0};
	int pwprfd, parentfd, fd, pipefd[2];
	int pid;
	struct screen screen;
	int fbfd;
	int chkscreen(), chkcolor();

	if ((*vsurfp->windowname != '\0') && !(vsurfp->flags & VWSURF_NEWFLG)) {
		if ((parentfd = openspecifiedwindow(vsurfp, colorflg)) < 0)
			return(1);
		if ((pwprfd = win_getnewwindow()) < 0)
			{
			close(parentfd);
			return(1);
			}
		if (win_insertblanket(pwprfd, parentfd))
			{
			close(parentfd);
			close(pwprfd);
			return(1);
			}
		close(parentfd);
		_core_gfxenvfd = pwprfd;
		*pwprfdp = pwprfd;
		*pidp = getpid();
	} else
	if ((_core_gfxenvfd == -1) && !(vsurfp->flags & VWSURF_NEWFLG) &&
	    (*vsurfp->screenname == '\0'))
		{
		if ((parentfd = opengfxwindowfromenv(colorflg)) < 0)
			return(1);
		if ((pwprfd = win_getnewwindow()) < 0)
			{
			close(parentfd);
			return(1);
			}
		if (win_insertblanket(pwprfd, parentfd))
			{
			close(parentfd);
			close(pwprfd);
			return(1);
			}
		close(parentfd);
		_core_gfxenvfd = pwprfd;
		*pwprfdp = pwprfd;
		*pidp = getpid();
		}
	else
		{
		if (*vsurfp->screenname != '\0')
			{
			foundscreen = FALSE;
			srchptr = vsurfp->screenname;
			scrptr = &screen;
			win_enumall(chkscreen);
			if (!foundscreen)
				return(1);
			if ((fbfd = open(screen.scr_fbname, O_RDWR, 0)) < 0)
				return(1);
			if (chkcolor(colorflg, fbfd))
				{
				close(fbfd);
				return(1);
				}
			close(fbfd);
			}
		else if (_core_gfxenvfd != -1)
			{
			win_screenget(_core_gfxenvfd, &screen);
			if ((fbfd = open(screen.scr_fbname, O_RDWR, 0)) < 0)
				return(1);
			if (chkcolor(colorflg, fbfd))
				{
				close(fbfd);
				return(1);
				}
			close(fbfd);
			}
		else
			{
			if ((fd = opengfxwindowfromenv(colorflg)) < 0)
				return(1);
			win_screenget(fd, &screen);
			close(fd);
			}
		if (pipe(pipefd) < 0)
			return(1);
		if ((pid = vfork()) == 0)
			{		/* child process */
			strncpy(name, screen.scr_rootname, DEVNAMESIZE);
			if (vsurfp->ptr == NULL)
				envparray[0] = NULL;
			else
				{
				envparray[0] = initstring;
				strncpy(initstring + 19, *vsurfp->ptr, 60);
				}
			if (dup2(pipefd[1], 1) < 0)
				_exit(0);
			close(0);  /* assume fds 0,1,2 are left */
			close(2);  /* undisturbed by SunCore programs */
				   /* so it is ok to close them */
			i2 = getdtablesize();
			for (i1 = 3; i1 < i2; i1++)
				fcntl(i1, F_SETFD, 1); /*close on exec*/
			execve("/usr/lib/view_surface", argvarray,
				envparray);
			_exit(0);     /* only return if unable to exec */
			}
		close(pipefd[1]);	/* parent process */
		if (pid < 0)
			{
			close(pipefd[0]);
			return(1);
			}
		if (read(pipefd[0], name, DEVNAMESIZE) < DEVNAMESIZE)
			{
			close(pipefd[0]);
			kill(pid, 9);
			return(1);
			}
		close(pipefd[0]);
		if ((pwprfd = open(name, O_RDWR, 0)) < 0)
			{
			kill(pid, 9);
			return(1);
			}
		win_setowner(pwprfd, getpid());
		*pwprfdp = pwprfd;
		*pidp = pid;
		}
	return(0);
	}

static int opengfxwindowfromenv(colorflg)
int colorflg;
	{
	int gwfd, fbfd;
	char name[DEVNAMESIZE];
	struct screen screen;
	int chkcolor();

	if (we_getgfxwindow(name))
		return(-1);
	if ((gwfd = open(name, O_RDWR, 0)) < 0)
		return(-1);
	win_screenget(gwfd, &screen);
	if ((fbfd = open(screen.scr_fbname, O_RDWR, 0)) < 0)
		{
		close(gwfd);
		return(-1);
		}
	if (chkcolor(colorflg, fbfd))
		{
		close(fbfd);
		close(gwfd);
		return(-1);
		}
	close(fbfd);
	return(gwfd);
	}

static int openspecifiedwindow(vsurfp, colorflg)
struct vwsurf *vsurfp;
int colorflg;
	{
	int gwfd, fbfd;
	struct screen screen;
	int chkcolor();

	if ((gwfd = open(vsurfp->windowname, O_RDWR, 0)) < 0)
		return(-1);
	win_screenget(gwfd, &screen);
	if ((fbfd = open(screen.scr_fbname, O_RDWR, 0)) < 0)
		{
		close(gwfd);
		return(-1);
		}
	if (chkcolor(colorflg, fbfd))
		{
		close(fbfd);
		close(gwfd);
		return(-1);
		}
	close(fbfd);
	return(gwfd);
	}

static int chkscreen(windowfd)
int windowfd;
	{
	win_screenget(windowfd, scrptr);
	if (strcmp(srchptr, scrptr->scr_fbname) == 0)
		{
		foundscreen = TRUE;
		return(TRUE);
		}
	return(FALSE);
	}

static
chkcolor(colorflg, fbfd)
	int colorflg, fbfd;
{
	struct fbgattr attr;

	return (ioctl(fbfd, FBIOGATTR, &attr) == -1 ||
		chkcolor2(colorflg, &attr.fbtype)) &&
		(ioctl(fbfd, FBIOGTYPE, &attr.fbtype) == -1 ||
		chkcolor2(colorflg, &attr.fbtype));
}

static
chkcolor2(colorflg, fb)
	struct fbtype *fb;
{
	switch (colorflg) {
	case BW_FB:
		return fb->fb_depth != 1;
	case COLOR_FB:
		return fb->fb_depth != 8;
	case BW_COLOR_FB:
		return 0;
	case GP1_FB:
		return fb->fb_type != FBTYPE_SUN2GP;
	default:
		return 1;
	}
}
