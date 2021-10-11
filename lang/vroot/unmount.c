/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)unmount.c 1.1 92/07/30 Copyright 1986 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <vroot.h>
#include "args.h"

static int	unmount_thunk(path) char *path;
{
	vroot_result= unmount(path);
	return(vroot_result == 0);
}

int	unmount_vroot(path, vroot_path, vroot_vroot)
	char		*path;
	pathpt		vroot_path;
	pathpt		vroot_vroot;
{
	translate_with_thunk(path, unmount_thunk, vroot_path, vroot_vroot, rw_read);
	return(vroot_result);
}
