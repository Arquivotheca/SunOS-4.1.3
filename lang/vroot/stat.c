/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)stat.c 1.1 92/07/30 Copyright 1986 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <vroot.h>
#include "args.h"

static int	stat_thunk(path) char *path;
{
	vroot_result= stat(path, vroot_args.stat.buffer);
	return(vroot_result == 0);
}

int	stat_vroot(path, buffer, vroot_path, vroot_vroot)
	char		*path;
	struct stat	*buffer;
	pathpt		vroot_path;
	pathpt		vroot_vroot;
{
	vroot_args.stat.buffer= buffer;
	translate_with_thunk(path, stat_thunk, vroot_path, vroot_vroot, rw_read);
	return(vroot_result);
}
