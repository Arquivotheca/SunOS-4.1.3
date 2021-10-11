/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)mount.c 1.1 92/07/30 Copyright 1986 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <vroot.h>
#include "args.h"

static int	mount_thunk(path) char *path;
{
	vroot_result= mount(path, vroot_args.mount.name, vroot_args.mount.mode);
	return(vroot_result == 0);
}

int	mount_vroot(target, name, mode, vroot_path, vroot_vroot)
	char		*target;
	char		*name;
	int		mode;
	pathpt		vroot_path;
	pathpt		vroot_vroot;
{
	vroot_args.mount.name= name;
	vroot_args.mount.mode= mode;
	translate_with_thunk(target, mount_thunk, vroot_path, vroot_vroot, rw_read);
	return(vroot_result);
}
