/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)chroot.c 1.1 92/07/30 Copyright 1986 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <vroot.h>
#include "args.h"

static int	chroot_thunk(path) char *path;
{
	vroot_result= chroot(path);
	return(vroot_result == 0);
}

int	chroot_vroot(path, vroot_path, vroot_vroot)
	char		*path;
	pathpt		vroot_path;
	pathpt		vroot_vroot;
{
	translate_with_thunk(path, chroot_thunk, vroot_path, vroot_vroot, rw_read);
	return(vroot_result);
}
