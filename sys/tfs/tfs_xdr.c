/*	@(#)tfs_xdr.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <sys/kmem_alloc.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#undef NFSSERVER
#include <tfs/tfs.h>

bool_t
xdr_tfstransargs(xdrs, args)
	XDR *xdrs;
	struct tfstransargs *args;
{
	return (xdr_fhandle(xdrs, &args->tr_fh) &&
		xdr_bool(xdrs, &args->tr_writeable));
}

bool_t
xdr_tfsdiropres(xdrs, dr)
	XDR *xdrs;
	struct tfsdiropres *dr;
{
#ifdef KERNEL
	int		old_pathlen;
#endif KERNEL

	if (!xdr_enum(xdrs, (enum_t *)&dr->dr_status)) {
		return (FALSE);
	}
	if (dr->dr_status != NFS_OK) {
		return (TRUE);
	}
#ifdef KERNEL
	old_pathlen = dr->dr_pathlen;
#endif KERNEL
	if (!xdr_u_int(xdrs, &dr->dr_pathlen)) {
		return (FALSE);
	}
#ifdef KERNEL
	if (dr->dr_pathlen > old_pathlen) {
		kmem_free(dr->dr_path, (u_int)(old_pathlen + 1));
		dr->dr_path = (caddr_t)new_kmem_alloc(
				(u_int)(dr->dr_pathlen + 1), KMEM_SLEEP);
	}
#endif KERNEL
	return (xdr_fhandle(xdrs, &dr->dr_fh) &&
		xdr_u_long(xdrs, &dr->dr_nodeid) &&
		xdr_string(xdrs, &dr->dr_path, dr->dr_pathlen) &&
		xdr_bool(xdrs, &dr->dr_writeable) &&
		xdr_sattr(xdrs, &dr->dr_sattrs));
}
