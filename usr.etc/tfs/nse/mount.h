/*	@(#)mount.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 Sun Microsystems, Inc.
 */

#ifndef _NSE_MOUNT_H
#define _NSE_MOUNT_H

#include <sys/mount.h>

#ifndef SUN_OS_4
/* NSE mount options */
#define	M_RECURSE	0x10	/* don't use all of buf cache for vfs */

#undef MOUNT_NFS
#define MOUNT_NFS	1

/* NSE file system types */
#define	MOUNT_LO	3
#define MOUNT_TFS	4

#ifndef M_NEWTYPE
#ifdef LOFS
struct lo_args {
	char	*fsdir;
};
#endif LOFS
#endif M_NEWTYPE

#else

#ifndef M_NEWTYPE
#define M_NEWTYPE	0x04
#endif
#endif SUN_OS_4

#endif _NSE_MOUNT_H
