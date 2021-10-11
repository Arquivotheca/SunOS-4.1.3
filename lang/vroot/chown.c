/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)chown.c 1.1 92/07/30 Copyright 1986 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <vroot.h>
#include "args.h"

static int	chown_thunk(path) char *path;
{
	vroot_result= chown(path, vroot_args.chown.user, vroot_args.chown.group);
	return(vroot_result == 0);
}

int	chown_vroot(path, user, group, vroot_path, vroot_vroot)
	char		*path;
	int		user;
	int		group;
	pathpt		vroot_path;
	pathpt		vroot_vroot;
{
	vroot_args.chown.user= user;
	vroot_args.chown.group= group;
	translate_with_thunk(path, chown_thunk, vroot_path, vroot_vroot, rw_read);
	return(vroot_result);
}
