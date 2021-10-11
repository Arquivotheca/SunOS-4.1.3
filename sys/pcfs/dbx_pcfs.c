#ifndef lint
static	char sccsid[] = "@(#)dbx_pcfs.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * This file is optionally brought in by including a
 * "psuedo-device dbx" line in the config file.  It is
 * compiled using the "-g" flag to generate structure
 * information which is used by dbx with the -k flag.
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/user.h>
#include <sys/vfs.h>
#include <sys/vnode.h>

#include <pcfs/pc_dir.h>
#include <pcfs/pc_fs.h>
#include <pcfs/pc_label.h>
#include <pcfs/pc_node.h>
