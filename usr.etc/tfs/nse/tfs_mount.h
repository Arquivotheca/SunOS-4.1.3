/*	@(#)tfs_mount.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef _TFS_MOUNT_H
#define	_TFS_MOUNT_H

#include <nse/types.h>
#include <nse/list.h>

#define NSE_MAXSITENAMELEN	64

/* nse_mtab.c */
Nse_err		nse_mtab_read();
Nse_err		nse_mtab_add();
Nse_err		nse_mtab_remove();
void		nse_mntent_destroy();
Nse_list	nse_mtablist_create();

/* nse_tfs_mount.c */
Nse_err		nse_tfs_mount();
Nse_err		nse_tfs_unmount();
Nse_err		nse_mount_loopback();

#endif	_TFS_MOUNT_H
