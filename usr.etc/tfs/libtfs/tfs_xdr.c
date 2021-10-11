#ifndef lint
static char sccsid[] = "@(#)tfs_xdr.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#include <nse_impl/tfs_user.h>

bool_t		xdr_tfs_mount_args();
bool_t		xdr_tfs_unmount_args();
bool_t		xdr_tfs_old_mount_args();
bool_t		xdr_tfs_mount_res();
bool_t		xdr_tfs_fhandle();
bool_t		xdr_tfs_name_args();
bool_t		xdr_tfs_get_wo_args();
bool_t		xdr_tfs_get_wo_res();
bool_t		xdr_tfs_getname_res();
bool_t		xdr_tfs_searchlink_args();

bool_t		xdr_fhandle();
static bool_t	_xdr_path();


bool_t
xdr_tfs_mount_args(xdrs, args)
	XDR		*xdrs;
	Tfs_mount_args	args;
{
	return (_xdr_path(xdrs, &args->environ) &&
		_xdr_path(xdrs, &args->tfs_mount_pt) &&
		_xdr_path(xdrs, &args->directory) &&
		_xdr_path(xdrs, &args->hostname) &&
		xdr_short(xdrs, &args->pid) &&
		xdr_short(xdrs, &args->writeable_layers) &&
		xdr_short(xdrs, &args->back_owner) &&
		xdr_bool(xdrs, &args->back_read_only) &&
		_xdr_path(xdrs, &args->default_view) &&
		_xdr_path(xdrs, &args->conditional_view));
}


bool_t
xdr_tfs_unmount_args(xdrs, args)
	XDR		*xdrs;
	Tfs_unmount_args args;
{
	return (_xdr_path(xdrs, &args->environ) &&
		_xdr_path(xdrs, &args->tfs_mount_pt) &&
		_xdr_path(xdrs, &args->hostname) &&
		xdr_short(xdrs, &args->pid));
}


bool_t
xdr_tfs_old_mount_args(xdrs, args)
	XDR		*xdrs;
	Tfs_old_mount_args args;
{
	return (_xdr_path(xdrs, &args->environ) &&
		_xdr_path(xdrs, &args->tfs_mount_pt) &&
		_xdr_path(xdrs, &args->directory) &&
		_xdr_path(xdrs, &args->hostname) &&
		xdr_int(xdrs, &args->pid) &&
		xdr_int(xdrs, &args->writeable_layers));
}


bool_t
xdr_tfs_mount_res(xdrs, args)
	XDR		*xdrs;
	Tfs_mount_res	args;
{
	if (!xdr_int(xdrs, &args->status)) {
		return (FALSE);
	}
	if (args->status != 0) {
		return (TRUE);
	}
	return (xdr_fhandle(xdrs, &args->fh) &&
		xdr_int(xdrs, &args->port) &&
		xdr_int(xdrs, &args->pid));
}


bool_t
xdr_tfs_fhandle(xdrs, fhp)
	XDR		*xdrs;
	Tfs_fhandle	fhp;
{
	return (xdr_long(xdrs, &fhp->nodeid) &&
		xdr_long(xdrs, &fhp->parent_id));
}


bool_t
xdr_tfs_name_args(xdrs, args)
	XDR		*xdrs;
	Tfs_name_args	args;
{
	return (xdr_tfs_fhandle(xdrs, &args->fhandle) &&
		_xdr_path(xdrs, &args->name));
}


bool_t
xdr_tfs_get_wo_args(xdrs, args)
	XDR		*xdrs;
	Tfs_get_wo_args	args;
{
	return (xdr_tfs_fhandle(xdrs, &args->fhandle) &&
		xdr_int(xdrs, &args->nbytes) &&
		xdr_int(xdrs, &args->offset));
}


bool_t
xdr_tfs_get_wo_res(xdrs, res)
	XDR		*xdrs;
	Tfs_get_wo_res	res;
{
	if (!xdr_enum(xdrs, (enum_t *) & res->status)) {
		return (FALSE);
	}
	if (res->status != NFS_OK) {
		return (TRUE);
	}
	return (xdr_int(xdrs, &res->count) &&
		xdr_int(xdrs, &res->offset) &&
		xdr_int(xdrs, &res->eof) &&
		xdr_bytes(xdrs, &res->buf, (u_int *) &res->count,
			  (u_int) res->count));
}


bool_t
xdr_tfs_getname_res(xdrs, res)
	XDR		*xdrs;
	Tfs_getname_res	res;
{
	if (!xdr_enum(xdrs, (enum_t *) & res->status)) {
		return (FALSE);
	}
	if (res->status != NFS_OK) {
		return (TRUE);
	}
	return (_xdr_path(xdrs, &res->path));
}


bool_t
xdr_tfs_searchlink_args(xdrs, args)
	XDR		*xdrs;
	Tfs_searchlink_args args;
{
	return (xdr_tfs_fhandle(xdrs, &args->fhandle) &&
		_xdr_path(xdrs, &args->value) &&
		xdr_int(xdrs, &args->conditional));
}


bool_t
xdr_fhandle(xdrs, fhp)
	XDR            *xdrs;
	fhandle_t      *fhp;
{
	return (xdr_opaque(xdrs, (char *) fhp, NFS_FHSIZE));
}


static bool_t
_xdr_path(xdrs, pathp)
	XDR            *xdrs;
	char          **pathp;
{
	return (xdr_string(xdrs, pathp, 1024));
}
