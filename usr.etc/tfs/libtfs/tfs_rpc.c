#ifndef lint
static char sccsid[] = "@(#)tfs_rpc.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 Sun Microsystems, Inc.
 */

/*
 * Routines to create RPC client connections with the TFS and to make
 * RPC calls to the TFS.
 */

#include <nse/param.h>
#include <nse/util.h>
#include <nse_impl/tfs_user.h>
#include <nse_impl/tfs.h>
#include <nse/tfs_calls.h>
#include <nse/nse_rpc.h>

/*
 * Client handle for RPC connection to TFS server.
 */
static Nse_rpc_client_rec tfsd_client;
static Nse_rpc_client_rec otfsd_client;	/* Connection to version 1 server */
static char	*tfsd_name = "tfsd";

static Nse_err	tfs_rpc_call_to_host();
static Nse_err	tfs_client_create();
static Nse_err	otfs_client_create();


/*
 * Make RPC call of procedure 'procnum' to the TFS server serving branch
 * 'branch'.  'in' is the address of the procedure's args, 'out' the
 * address of the procedure's results.  (NOTE: this is the 'tfs_mount'
 * version of this routine; it knows nothing about NSE branches.)
 */
/* ARGSUSED */
Nse_err
nse_tfs_rpc_call(branch, procnum, inproc, in, outproc, out)
	char		*branch;
	int		procnum;
	xdrproc_t	inproc;
	char		*in;
	xdrproc_t	outproc;
	char		*out;
{
	return tfs_rpc_call_to_host(_nse_hostname(), procnum, inproc, in,
				    outproc, out);
}


static Nse_err
tfs_rpc_call_to_host(host, procnum, inproc, in, outproc, out)
	char		*host;
	int		procnum;
	xdrproc_t	inproc;
	char		*in;
	xdrproc_t	outproc;
	char		*out;
{
	Nse_err		err;

	err = nse_tfs_rpc_call_to_host(host, procnum, inproc, in,
				       outproc, out);
	if (err && err->code == NSE_RPC_ERROR &&
	    nse_rpc_error() == RPC_PROGVERSMISMATCH) {
		procnum = procnum + TFS_OLD_MOUNT - TFS_MOUNT;
		err = nse_tfs_rpc_call_to_host_1(host, procnum, inproc, in,
						 outproc, out);
	}
	return err;
}


/*
 * Make RPC call of procedure 'procnum' to the TFS server on machine 'host'.
 */
Nse_err
nse_tfs_rpc_call_to_host(host, procnum, inproc, in, outproc, out)
	char		*host;
	int		procnum;
	xdrproc_t	inproc;
	char		*in;
	xdrproc_t	outproc;
	char		*out;
{
	return nse_rpc_call(procnum, inproc, in, outproc, out, host, tfsd_name,
			    &tfsd_client, tfs_client_create, (char *) NULL,
			    (char *) NULL, (char *) NULL);
}


/*
 * Make an RPC call to a version 1 TFS server
 */
Nse_err
nse_tfs_rpc_call_to_host_1(host, procnum, inproc, in, outproc, out)
	char		*host;
	int		procnum;
	xdrproc_t	inproc;
	char		*in;
	xdrproc_t	outproc;
	char		*out;
{
	return nse_rpc_call(procnum, inproc, in, outproc, out, host, tfsd_name,
			    &otfsd_client, otfs_client_create, (char *) NULL,
			    (char *) NULL, (char *) NULL);
}


static Nse_err
tfs_client_create(tfs_client, host, timeout)
	Nse_rpc_client	tfs_client;
	char		*host;
	long		timeout;
{
	Nse_err		err;

	if (err = nse_rpc_client_create(tfs_client, host, timeout,
					TFS_PROGRAM, TFS_VERSION,
					tfsd_name, 0)) {
		return err;
	}
	tfs_client->client->cl_auth = authunix_create_default();
	return NULL;
}


static Nse_err
otfs_client_create(tfs_client, host, timeout)
	Nse_rpc_client	tfs_client;
	char		*host;
	long		timeout;
{
	Nse_err		err;

	if (err = nse_rpc_client_create(tfs_client, host, timeout,
					TFS_PROGRAM, TFS_OLD_VERSION,
					tfsd_name, 0)) {
		return err;
	}
	tfs_client->client->cl_auth = authunix_create_default();
	return NULL;
}


/*
 * Called by nse_tfs_mount(), which needs to know the address of the TFS
 * server.  Saves us a gethostbyname() call.
 */
void
nse_tfs_addr(addr, version)
	struct sockaddr_in **addr;
	u_long		version;
{
	if (version == TFS_VERSION) {
		*addr = &tfsd_client.addr;
	} else {
		*addr = &otfsd_client.addr;
	}
}
