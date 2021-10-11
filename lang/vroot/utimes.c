/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)utimes.c 1.1 92/07/30 Copyright 1986 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <vroot.h>
#include "args.h"

static int	utimes_thunk(path) char *path;
{
	vroot_result= utimes(path, vroot_args.utimes.time);
	return(vroot_result == 0);
}

int	utimes_vroot(path, time, vroot_path, vroot_vroot)
	char		*path;
	struct timeval	**time;
	pathpt		vroot_path;
	pathpt		vroot_vroot;
{
	vroot_args.utimes.time= time;
	translate_with_thunk(path, utimes_thunk, vroot_path, vroot_vroot, rw_read);
	return(vroot_result);
}
