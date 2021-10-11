#ifndef lint
static char sccsid[] = "@(#)nse_fchdir.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * File: nse_fchdir.c
 *
 * Copyright (c) 1988 Sun Microsystems, Inc.
 */

#ifdef SUN_OS_4
#define NSE_SYS_fchdir		176
#define NSE_SYS_fchroot		177
#else
#define NSE_SYS_fchdir		172
#define NSE_SYS_fchroot		173
#endif

int
nse_fchdir(fd)
	int		fd;
{
	return (syscall(NSE_SYS_fchdir, fd));
}


int
nse_fchroot(fd)
	int		fd;
{
	return (syscall(NSE_SYS_fchroot, fd));
}
