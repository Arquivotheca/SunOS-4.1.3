#ifndef lint
static char sccsid[] = "@(#)detach.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
/*
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */

#include <sys/termios.h>
#include <fcntl.h>

/*
 * detach from tty
 */
detachfromtty()
{
	int tt;

	close(0);
	close(1);
	close(2);
	switch (fork()) {
	case -1:
		perror("fork");
		break;
	case 0:
		break;
	default:
		exit(0);
	}
	tt = open("/dev/tty", O_RDWR);
	if (tt > 0) {
		ioctl(tt, TIOCNOTTY, 0);
		close(tt);
	}
	(void)open("/dev/null", O_RDWR, 0);
	dup(0);
	dup(0);
}
 

