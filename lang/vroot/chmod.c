/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)chmod.c 1.1 92/07/30 Copyright 1986 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <vroot.h>
#include "args.h"

static int	chmod_thunk(path) char *path;
{
	vroot_result= chmod(path, vroot_args.chmod.mode);
	return(vroot_result == 0);
}

int	chmod_vroot(path, mode, vroot_path, vroot_vroot)
	char		*path;
	int		mode;
	pathpt		vroot_path;
	pathpt		vroot_vroot;
{
	vroot_args.chmod.mode= mode;
	translate_with_thunk(path, chmod_thunk, vroot_path, vroot_vroot, rw_read);
	return(vroot_result);
}
