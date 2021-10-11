#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)win_blanket.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Overview: Implement the blanket window.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sunwindow/rect.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/win_struct.h>

int
win_isblanket(windowfd)
	int	windowfd;
{
	int	flags = win_getuserflags(windowfd);

	return(flags&WUF_BLANKET);
}

int
win_insertblanket(windowfd, parentfd)
	register int	windowfd, parentfd;
{
	/*
	 * Acquire window data lock.
	 */
	(void)win_lockdata(parentfd);
	if (!win_isblanket(windowfd)) {
		int	flags;
		int	parentlink, link;
		struct	rect rect;

		/*
		 * Setup links to be top child of parent.
		 */
		parentlink = win_fdtonumber(parentfd);
		(void)win_setlink(windowfd, WL_PARENT, parentlink);
		link = win_getlink(parentfd, WL_TOPCHILD);
		(void)win_setlink(windowfd, WL_OLDERSIB, link);
		/*
		 * Setup rect to cover parent.
		 */
		(void)win_getsize(parentfd, &rect);
		(void)win_setrect(windowfd, &rect);
		/*
		 * Setup flags
		 */
		flags = win_getuserflags(windowfd);
		(void)win_setuserflags(windowfd, flags|WUF_BLANKET);
		/*
		 * Install in window tree.
		 */
		(void)win_insert(windowfd);
	}
	/*
	 * Release window data lock.
	 */
	(void)win_unlockdata(parentfd);
	/*
	 * See if window still thinks its a blanket window.
	 */
	return(!win_isblanket(windowfd));
}

win_removeblanket(windowfd)
	int	windowfd;
{
	/*
	 * Acquire window data lock.
	 */
	(void)win_lockdata(windowfd);
	if (win_isblanket(windowfd)) {
		int	flags = win_getuserflags(windowfd);

		/*
		 * Remove from window tree.
		 */
		(void)win_remove(windowfd);
		/*
		 * Reset blanket flag.
		 */
		(void)win_setuserflags(windowfd, flags&(~WUF_BLANKET));
	}
	/*
	 * Release window data lock.
	 */
	(void)win_unlockdata(windowfd);
}

