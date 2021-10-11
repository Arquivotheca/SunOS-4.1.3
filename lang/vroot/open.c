/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)open.c 1.1 92/07/30 Copyright 1986 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <vroot.h>
#include "args.h"

static int	open_thunk(path) char *path;
{
	vroot_result= open(path, vroot_args.open.flags, vroot_args.open.mode);
	return(vroot_result >= 0);
}

int	open_vroot(path, flags, mode, vroot_path, vroot_vroot)
	char		*path;
	int		flags;
	int		mode;
	pathpt		vroot_path;
	pathpt		vroot_vroot;
{
	vroot_args.open.flags= flags;
	vroot_args.open.mode= mode;
	translate_with_thunk(path, open_thunk, vroot_path, vroot_vroot,
				((flags & (O_CREAT|O_APPEND)) != 0) ? rw_write : rw_read);
	return(vroot_result);
}
