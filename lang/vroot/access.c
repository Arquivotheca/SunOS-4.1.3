/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)access.c 1.1 92/07/30 Copyright 1986 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <vroot.h>
#include "args.h"

static int	access_thunk(path)
	char		*path;
{
	vroot_result= access(path, vroot_args.access.mode);
	return((vroot_result == 0) || (errno != ENOENT));
}

int	access_vroot(path, mode, vroot_path, vroot_vroot)
	char		*path;
	int		mode;
	pathpt		vroot_path;
	pathpt		vroot_vroot;
{
	vroot_args.access.mode= mode;
	translate_with_thunk(path, access_thunk, vroot_path, vroot_vroot, rw_read);
	return(vroot_result);
}
