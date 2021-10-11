/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)readlink.c 1.1 92/07/30 Copyright 1986 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <vroot.h>
#include "args.h"

static int	readlink_thunk(path) char *path;
{
	vroot_result= readlink(path, vroot_args.readlink.buffer, vroot_args.readlink.buffer_size);
	return(vroot_result >= 0);
}

int	readlink_vroot(path, buffer, buffer_size, vroot_path, vroot_vroot)
	char		*path;
	char		*buffer;
	int		buffer_size;
	pathpt		vroot_path;
	pathpt		vroot_vroot;
{
	vroot_args.readlink.buffer= buffer;
	vroot_args.readlink.buffer_size= buffer_size;
	translate_with_thunk(path, readlink_thunk, vroot_path, vroot_vroot, rw_read);
	return(vroot_result);
}
