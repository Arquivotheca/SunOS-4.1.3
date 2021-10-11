#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)sys_fcntl.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif
 
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Sys_fcntl.c - Real system call to fcntl.
 */

#include <syscall.h>
#include <sunwindow/ntfy.h>

pkg_private int
notify_fcntl(fd, cmd, arg)
	int fd, cmd, arg;
{
	return(syscall(SYS_fcntl, fd, cmd, arg));
}

