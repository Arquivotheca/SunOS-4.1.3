/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)statfs.c 1.1 92/07/30 Copyright 1986 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <vroot.h>
#include "args.h"

static int	statfs_thunk(path) char *path;
{
	vroot_result= statfs(path, vroot_args.statfs.buffer);
	return(vroot_result == 0);
}

int	statfs_vroot(path, buffer, vroot_path, vroot_vroot)
	char		*path;
	struct statfs	*buffer;
	pathpt		vroot_path;
	pathpt		vroot_vroot;
{
	vroot_args.statfs.buffer= buffer;
	translate_with_thunk(path, statfs_thunk, vroot_path, vroot_vroot, rw_read);
	return(vroot_result);
}
