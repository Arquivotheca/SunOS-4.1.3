#ifndef lint
static	char sccsid[] = "@(#)dbx_tfs.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * This file is optionally brought in by including a
 * "psuedo-device dbx" line in the config file.  It is
 * compiled using the "-g" flag to generate structure
 * information which is used by dbx with the -k flag.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <rpc/types.h>

#include <tfs/tfs.h>
#include <tfs/tnode.h>
