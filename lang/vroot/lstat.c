/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)lstat.c 1.1 92/07/30 Copyright 1986 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <vroot.h>
#include "args.h"

static int	lstat_thunk(path) char *path;
{
	vroot_result= lstat(path, vroot_args.lstat.buffer);
	return(vroot_result == 0);
}

int	lstat_vroot(path, buffer, vroot_path, vroot_vroot)
	char		*path;
	struct stat	*buffer;
	pathpt		vroot_path;
	pathpt		vroot_vroot;
{
	vroot_args.lstat.buffer= buffer;
	translate_with_thunk(path, lstat_thunk, vroot_path, vroot_vroot, rw_read);
	return(vroot_result);
}
