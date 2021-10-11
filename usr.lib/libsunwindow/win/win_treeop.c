#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)win_treeop.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Win_treeop.c: Implement the tree operation functions
 *	of the win_struct.h interface.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <sunwindow/rect.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/win_struct.h>
#define	WIN_SVJ
extern char *sprintf();
extern char *getenv();

/*
 * Writing a sync point for journalling
 */
void
win_sync(sync_id, fd)
	int	sync_id;
	int	fd;
{
	static 	int	rootwin_fd;
	char	*rootwin_name=0;
	static	Winrecplay_syncbuf syncbuf = {WINSYNCUSRSTD, 0, 0,
				   WINSETSYNCPT, (WINSYNCPLAY|WINSYNCREC) };

	errno=0;
	if (fd == -1) {
		if ((rootwin_name = getenv("WINDOW_ME")) == NULL) {
			perror("** win_sync ** Getting WINDOW_ME");
			return;
		}
		if ((rootwin_fd = open(rootwin_name, O_RDWR, 0)) == -1){
			perror("** win_sync ** Opening window");
			return;
		}
	}
	else {
	  	rootwin_fd = fd;
	}
	syncbuf.sync_type = sync_id;
	if (ioctl(rootwin_fd,WINSETSYNCPT, &syncbuf)) {
		perror("** win_sync ** Writing CapSync Point");
	}
	if (fd == -1)
		close (rootwin_fd);
}

/*
 * Tree operations.
 */
int
win_getlink(windowfd, linkname)
	int	windowfd, linkname;
{
	struct	winlink winlink;

	winlink.wl_which = linkname;
	(void)werror(ioctl(windowfd, WINGETLINK, &winlink), WINGETLINK);
	return(winlink.wl_link);
}

win_setlink(windowfd, linkname, number)
	int	windowfd, linkname, number;
{
	struct	winlink winlink;

	winlink.wl_which = linkname;
	winlink.wl_link = number;
	(void)werror(ioctl(windowfd, WINSETLINK, &winlink), WINSETLINK);
	return;
}

win_insert(windowfd)
	int	windowfd;
{
	(void)werror(ioctl(windowfd, WININSERT, 0), WININSERT);
	return;
}

win_remove(windowfd)
	int	windowfd;
{
	(void)werror(ioctl(windowfd, WINREMOVE, 0), WINREMOVE);
	return;
}

int
win_nextfree(windowfd)
	int	windowfd;
{
	struct	winlink winlink;

	(void)werror(ioctl(windowfd, WINNEXTFREE, &winlink), WINNEXTFREE);
	return(winlink.wl_link);
}

/*
 * Utilities
 */
win_numbertoname(number, name)
	int	number;
	char	*name;
{
	*name = '\0';
	(void)sprintf(name, "/dev/win%d", number);
	return;
}

int
win_nametonumber(name)
	char	*name;
{
	int	number;

	if (sscanf(name, "/dev/win%d", &number)!=1)
		return(-1);
	return(number);
}

win_fdtoname(windowfd, name)
	int	windowfd;
	char	*name;
{
	struct	stat buf;

	(void)fstat(windowfd, &buf); /*Note: check args */
	(void)win_numbertoname(minor(buf.st_rdev), name);
	return;
}

int
win_fdtonumber(windowfd)
	int	windowfd;
{
	char	name[WIN_NAMESIZE];

	(void)win_fdtoname(windowfd, name);
	return(win_nametonumber(name));
}

int
win_getnewwindow()
{
	char	name[WIN_NAMESIZE];
	int	windowfd, newwindowfd = -1;
	int	newwindownum, i;
	extern	int errno;


	/*
	 * Open /dev/win0 to get fd so can talk to kernel window ioctl calls
	 */
	(void)win_numbertoname(0, name);
	if ((windowfd = open(name, O_RDONLY, 0)) < 0) {
		(void)fprintf(stderr, "%s would not open\n", name);
		(void)werror(-1, windowfd);
		return(-1);
	}
	/*
	 * Need to loop on trying to get new window because others are
	 * competing for this same window.
	 */
#define	NEWWINTRIES	16
	for (i=0;i<NEWWINTRIES;i++) {
		/*
		 * Get the next free window
		 */
		newwindownum = win_nextfree(windowfd);
		if (newwindownum==WIN_NULLLINK) {
			if (i == NEWWINTRIES-1)
				break;
			else
				continue;
		}
		(void)win_numbertoname(newwindownum, name);
		if ((newwindowfd = open(name, O_RDWR|WIN_EXCLOPEN, 0)) < 0) {
			if (errno==EACCES || errno==ENOENT)
				continue;
			(void)fprintf(stderr,
			    "%s would not open (be created)(errno = %ld)\n",
			    name, errno);
			continue;
		}
		if (newwindowfd <= 2) {
			(void)fprintf(stderr,
			    "%s window created is std i/o/e: fd = %d\n",
			    name, newwindowfd);
		} else
			goto GotOne;
	}
	(void)fprintf(stderr, "no more windows available\n");
	(void)werror(-1, ENOSPC);
	/* return -1 if no more windows or file descriptors */
	newwindowfd = WIN_NULLLINK ; 
GotOne:
	(void)close(windowfd);
	return(newwindowfd);
}

