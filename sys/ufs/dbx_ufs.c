#ifndef lint
static	char sccsid[] = "@(#)dbx_ufs.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * This file is optionally brought in by including a
 * "psuedo-device dbx" line in the config file.  It is
 * compiled using the "-g" flag to generate structure
 * information which is used by dbx with the -k flag.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/vfs.h>
#include <sys/vnode.h>

#include <ufs/fs.h>
#include <ufs/fsdir.h>
#include <ufs/inode.h>
#include <ufs/mount.h>
#ifdef QUOTA
#include <ufs/quota.h>
#endif QUOTA
