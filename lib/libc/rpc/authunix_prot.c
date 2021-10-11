#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)authunix_prot.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * authunix_prot.c
 * XDR for UNIX style authentication parameters for RPC
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#ifdef KERNEL
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#endif

#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/auth_unix.h>

/*
 * XDR for unix authentication parameters.
 */
bool_t
xdr_authunix_parms(xdrs, p)
	register XDR *xdrs;
	register struct authunix_parms *p;
{

	if (xdr_u_long(xdrs, &(p->aup_time)) &&
	    xdr_string(xdrs, &(p->aup_machname), MAX_MACHINE_NAME) &&
	    xdr_int(xdrs, (int *)&(p->aup_uid)) &&
	    xdr_int(xdrs, (int *)&(p->aup_gid)) &&
	    xdr_array(xdrs, (caddr_t *)&(p->aup_gids),
		    &(p->aup_len), NGRPS, sizeof (int), xdr_int)) {
		return (TRUE);
	}
	return (FALSE);
}

#ifdef KERNEL
/*
 * XDR kernel unix auth parameters.
 * Goes out of the u struct directly.
 * NOTE: this is an XDR_ENCODE only routine.
 */
xdr_authkern(xdrs)
	register XDR *xdrs;
{
	int	*gp;
	int	uid = u.u_uid;
	int	gid = u.u_gid;
	int	len;
	caddr_t	groups;
	char	*name = hostname;

	if (xdrs->x_op != XDR_ENCODE) {
		return (FALSE);
	}

	for (gp = &u.u_groups[NGROUPS]; gp > u.u_groups; gp--) {
		if (gp[-1] >= 0) {
			break;
		}
	}
	len = gp - u.u_groups;
	groups = (caddr_t)u.u_groups;
	if (xdr_u_long(xdrs, (u_long *)&time.tv_sec) &&
	    xdr_string(xdrs, &name, MAX_MACHINE_NAME) &&
	    xdr_int(xdrs, &uid) &&
	    xdr_int(xdrs, &gid) &&
	    xdr_array(xdrs, &groups, (u_int *)&len, NGRPS,
			sizeof (int), xdr_int)) {
		return (TRUE);
	}
	return (FALSE);
}
#endif
