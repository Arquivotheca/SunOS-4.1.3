#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)sys_read.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif
 
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Sys_read.c - Real system call to read.
 */

#include <syscall.h>
#include <sunwindow/ntfy.h>

pkg_private int
notify_read(fd, buf, nbytes)
	int fd;
	char *buf;
	int nbytes;
{
	return(syscall(SYS_read, fd, buf, nbytes));
}

