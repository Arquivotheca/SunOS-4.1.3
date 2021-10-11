/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)mkdir.c 1.1 92/07/30 Copyright 1986 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <vroot.h>
#include "args.h"

static int	mkdir_thunk(path) char *path;
{
	vroot_result= mkdir(path, vroot_args.mkdir.mode);
	return(vroot_result == 0);
}

int	mkdir_vroot(path, mode, vroot_path, vroot_vroot)
	char		*path;
	int		mode;
	pathpt		vroot_path;
	pathpt		vroot_vroot;
{
	vroot_args.mkdir.mode= mode;
	translate_with_thunk(path, mkdir_thunk, vroot_path, vroot_vroot, rw_write);
	return(vroot_result);
}
