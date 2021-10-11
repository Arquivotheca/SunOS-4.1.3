#ifndef lint
static  char sccsid[] = "@(#)swapgeneric.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/vm.h>
#include <sys/systm.h>
#include <sys/file.h>
#include <sys/vfs.h>
#undef NFS
#include <sys/mount.h>

#include <sas_simdef.h>

/*
 * Generic configuration;  all in one
 */
dev_t	rootdev;
struct vnode *rootvp;

struct vfssw *
getfstype(askfor, fsname)
	char *askfor;
	char *fsname;
{

	return (&vfssw[MOUNT_UFS]);
}

dev_t
getblockdev(askfor, name)
	char *askfor;
	char *name;
{

	strcpy(name, "sim0");
	return (makedev(1, 0));
}
