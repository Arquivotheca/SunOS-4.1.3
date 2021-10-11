/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)truncate.c 1.1 92/07/30 Copyright 1986 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <vroot.h>
#include "args.h"

static int	truncate_thunk(path) char *path;
{
	vroot_result= truncate(path, vroot_args.truncate.length);
	return(vroot_result == 0);
}

int	truncate_vroot(path, length, vroot_path, vroot_vroot)
	char		*path;
	int		length;
	pathpt		vroot_path;
	pathpt		vroot_vroot;
{
	vroot_args.truncate.length= length;
	translate_with_thunk(path, truncate_thunk, vroot_path, vroot_vroot, rw_read);
	return(vroot_result);
}
