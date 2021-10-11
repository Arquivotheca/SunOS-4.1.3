/*	@(#)nfs_vfsops.c 1.1 92/07/30 SMI 	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/vfs.h>
#include <sys/vfs_stat.h>
#include <sys/vnode.h>
#include <sys/pathname.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/kernel.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/pmap_rmt.h>
#include <rpc/pmap_prot.h>
#include <rpcsvc/bootparam.h>
#include <nfs/nfs.h>
#include <nfs/nfs_clnt.h>
#include <nfs/rnode.h>
#include <sys/mount.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/if_ether.h>
#include <rpcsvc/mount.h>
#include <sys/bootconf.h>
#include <sys/ioctl.h>
#include <sys/unistd.h>

#define	satosin(sa)	((struct sockaddr_in *)(sa))

#ifdef	NFSDEBUG
extern int nfsdebug;
#endif

static int nfsrootvp();
extern struct vnode *makenfsnode();
#define	MAPSIZE  256/NBBY
static char nfs_minmap[MAPSIZE]; /* Map for minor device allocation */

/*
 * nfs vfs operations.
 */
static	int nfs_mount();
static	int nfs_unmount();
static	int nfs_root();
static	int nfs_statfs();
static	int nfs_sync();
static	int nfs_mountroot();
static	int nfs_swapvp();
static	int nfs_badop();

struct	vfsops nfs_vfsops = {
	nfs_mount,
	nfs_unmount,
	nfs_root,
	nfs_statfs,
	nfs_sync,
	nfs_badop,
	nfs_mountroot,
	nfs_swapvp,
};

/*
 * Called by vfs_mountroot when nfs is going to be mounted as root
 */
static int
nfs_mountroot(vfsp, vpp, name)
	struct vfs *vfsp;
	struct vnode **vpp;
	char *name;
{
	struct sockaddr_in root_sin;
	struct vnode *rtvp;
	char *root_path;
	char root_hostname[MAXHOSTNAMELEN+1];
	fhandle_t root_fhandle;
	int rc;
	struct pathname pn;

	/* do this BEFORE getfile which causes xid stamps to be initialized */
	inittodr(-1L);		/* hack for now - until we get time svc? */

	getfsname("root", name);
	pn_alloc(&pn);
	root_path = pn.pn_path;
	do {
		rc = getfile(*name ? name : "root", root_hostname,
			    (struct sockaddr *)&root_sin, root_path);
	} while (rc == ETIMEDOUT);
	if (rc) {
		pn_free(&pn);
		return (rc);
	}
	rc = mountnfs(&root_sin, root_hostname, root_path, &root_fhandle);
	if (rc) {
		pn_free(&pn);
		printf("mount root %s:%s failed, rpc status %d\n",
			root_hostname, root_path, rc);
		return (rc);
	}

	rc = nfsrootvp(&rtvp, vfsp, &root_sin, &root_fhandle, root_hostname,
	    (char *)NULL, -1, 0);
	if (rc) {
		pn_free(&pn);
		return (rc);
	}
	if (rc = vfs_add((struct vnode *)0, vfsp, 0)) {
		pn_free(&pn);
		return (rc);
	}
	/*
	 * Set maximum attribute timeouts and turn off close-to-open
	 * consistency checking
	 */
	vftomi(vfsp)->mi_acregmin = ACMINMAX;
	vftomi(vfsp)->mi_acregmax = ACMAXMAX;
	vftomi(vfsp)->mi_acdirmin = ACMINMAX;
	vftomi(vfsp)->mi_acdirmax = ACMAXMAX;
	vftomi(vfsp)->mi_nocto = 1;
	vfs_unlock(rtvp->v_vfsp);
	*vpp = rtvp;
	{
		register char *nm;
		extern char *strcpy();

		nm = strcpy(name, root_hostname);
		*nm++ = ':';
		(void) strcpy(nm, root_path);
	}

	pn_free(&pn);
	return (0);
}

extern struct sockaddr_in nfsdump_sin;
extern fhandle_t nfsdump_fhandle;
extern int nfsdump_maxcount;

/*
 * Set up for swapping to NFS.
 * Call nfsrootvp to set up the
 * RPC/NFS machinery.
 */
static int
nfs_swapvp(vfsp, vpp, path)
	struct vfs *vfsp;
	struct vnode **vpp;
	char *path;
{
	struct sockaddr_in swap_sin;
	char *swap_path;
	char swap_hostname[MAXHOSTNAMELEN + 1];
	fhandle_t swap_fhandle;
	int rc;
	struct vattr va;
	struct vfs *dumpvfsp;

	swap_path = new_kmem_alloc(MAX_MACHINE_NAME + 1, KMEM_SLEEP);
	getfsname("swap", path);
	do {
		rc = getfile(*path ? path : "swap", swap_hostname,
			    (struct sockaddr *)&swap_sin, swap_path);
	} while (rc == ETIMEDOUT);
	if (rc != 0)
		goto error;

	rc = mountnfs(&swap_sin, swap_hostname, swap_path, &swap_fhandle);
	if (rc != 0) {
		printf("mount swapfile %s:%s failed, rpc status %d\n",
			swap_hostname, swap_path, rc);
		goto error;
	}
	rc = nfsrootvp(vpp, vfsp, &swap_sin, &swap_fhandle,
	    swap_hostname, (char *)NULL, -1, 0);
	if (rc != 0)
		goto error;

	hostpath(path, swap_hostname, swap_path);
	(void)VOP_GETATTR(*vpp, &va, u.u_cred);
	nfsdump_maxcount = va.va_size;
	nfsdump_fhandle = swap_fhandle;
	nfsdump_sin = swap_sin;

	/*
	 * While we're at it, configure dump too.  We need to be sure to
	 * call VOP_GETATTR for the dump file or else it'll never get done.
	 */
	rc = getfile("dump", swap_hostname,
			(struct sockaddr *)&nfsdump_sin, swap_path);
	if (rc != 0)
		goto done;

	rc = mountnfs(&nfsdump_sin, swap_hostname,
				swap_path, &nfsdump_fhandle);
	if (rc != 0) {
		printf("mount dumpfile %s:%s failed, rpc status %d\n",
			swap_hostname, swap_path, rc);
		goto done;
	}

	dumpvfsp = (struct vfs *)new_kmem_alloc(sizeof (*vfsp), KMEM_SLEEP);
	VFS_INIT(dumpvfsp, &nfs_vfsops, (caddr_t)0);
	rc = nfsrootvp(&dumpfile.bo_vp, dumpvfsp, &nfsdump_sin,
		    &nfsdump_fhandle, swap_hostname, (char *)NULL, -1, 0);
	if (rc != 0) {
		kmem_free((caddr_t)dumpvfsp, sizeof (*vfsp));
		goto done;
	}
	(void) strcpy(dumpfile.bo_fstype, "nfs");
	hostpath(dumpfile.bo_name, swap_hostname, swap_path);
	(void) VOP_GETATTR(dumpfile.bo_vp, &va, u.u_cred);
	dumplo = 0;
	nfsdump_maxcount = 0;

done:
	kmem_free(swap_path, MAX_MACHINE_NAME + 1);
	return (0);

error:
	kmem_free(swap_path, MAX_MACHINE_NAME + 1);
	return (rc);
}


/*
 * pmapper remote-call-service interface.
 * This routine is used to call the pmapper remote call service
 * which will look up a service program in the port maps, and then
 * remotely call that routine with the given parameters.  This allows
 * programs to do a lookup and call in one step.
*/
static enum clnt_stat
pmap_rmtcall(call_addr, progn, versn, procn, xdrargs, argsp,
		xdrres, resp, tout, resp_addr)
	struct sockaddr_in *call_addr;
	u_long progn, versn, procn;
	xdrproc_t xdrargs, xdrres;
	caddr_t argsp, resp;
	struct timeval tout;
	struct sockaddr_in *resp_addr;
{
	register CLIENT *client;
	struct rmtcallargs a;
	struct rmtcallres r;
	enum clnt_stat stat, clntkudp_callit_addr();
	u_long port;

	call_addr->sin_port = htons(PMAPPORT);
#define	PMAP_RETRIES 5
	client = clntkudp_create(call_addr, PMAPPROG, PMAPVERS, PMAP_RETRIES,
	    u.u_cred);
	if (client != (CLIENT *)NULL) {
		a.prog = progn;
		a.vers = versn;
		a.proc = procn;
		a.args_ptr = argsp;
		a.xdr_args = xdrargs;
		r.port_ptr = &port;
		r.results_ptr = resp;
		r.xdr_results = xdrres;
		stat = clntkudp_callit_addr(client, PMAPPROC_CALLIT,
		    xdr_rmtcall_args, (caddr_t)&a,
		    xdr_rmtcallres, (caddr_t) &r, tout, resp_addr, 1);
		resp_addr->sin_port = htons((u_short) port);
		CLNT_DESTROY(client);
	} else {
		panic("pmap_rmtcall: clntkudp_create failed");
	}
	return (stat);
}

struct	ifnet *ifb_ifwithaf();

static struct sockaddr_in bootparam_addr;

static int
whoami()
{
	struct sockaddr_in sa;
	struct ifnet *ifp;
	struct bp_whoami_arg arg;
	struct bp_whoami_res res;
	struct timeval tv;
	struct in_addr ipaddr;
	enum clnt_stat status;
	struct rtentry rtentry;
	struct sockaddr_in *sin;
	struct ifreq req;
	int error = 0;
	int printed_waiting_msg;
	enum clnt_stat callrpc();

	bzero((caddr_t)&sa, sizeof sa);
	ifp = ifb_ifwithaf(AF_INET);
	if (ifp == 0) {
		printf("whoami: zero ifp\n");
		return (EHOSTUNREACH);
	}
	if (!address_known(ifp)) {
		revarp_myaddr(ifp);
	}

	/*
	 * Pick up the interface broadcast address.
	 */
	if ((error = in_control((struct socket *)0, SIOCGIFBRDADDR,
	    (caddr_t)&req, ifp)) != 0) {
		printf("whoami: in_control err=%d if_flags 0x%x\n",
			error, ifp->if_flags);
		panic("bad SIOCGIFBRDADDR in_control");
	}
	bcopy((caddr_t)&req.ifr_dstaddr, (caddr_t)&sa,
	    sizeof (struct sockaddr_in));

	arg.client_address.address_type = IP_ADDR_TYPE;

	if (in_control((struct socket *)0, SIOCGIFADDR, (caddr_t)&req,
	    ifp) != 0)
		panic("bad SIOCGIFADDR in_control");
	ipaddr = (satosin (&req.ifr_addr)->sin_addr);

	bcopy((caddr_t)&ipaddr, (caddr_t)&arg.client_address.bp_address.ip_addr,
	    sizeof (struct in_addr));

	/* initial retransmission interval */
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	res.client_name = (bp_machine_name_t)
			new_kmem_alloc(MAX_MACHINE_NAME + 1, KMEM_SLEEP);
	res.domain_name = (bp_machine_name_t)
			new_kmem_alloc(MAX_MACHINE_NAME + 1, KMEM_SLEEP);

	printed_waiting_msg = 0;
	do {
		status = pmap_rmtcall(&sa, (u_long)BOOTPARAMPROG,
		    (u_long)BOOTPARAMVERS, (u_long)BOOTPARAMPROC_WHOAMI,
		    xdr_bp_whoami_arg, (caddr_t)&arg,
		    xdr_bp_whoami_res, (caddr_t)&res,
		    tv, &bootparam_addr);
		if (status == RPC_TIMEDOUT && !printed_waiting_msg) {
			printf(
			    "No bootparam server responding; still trying\n");
			printf("whoami: pmap_rmtcall status 0x%x\n", status);
			printed_waiting_msg = 1;
		}
		/*
		 * Retransmission interval for second and subsequent tries.
		 * We expect first pmap_rmtcall to retransmit and backoff to
		 * at least this value.
		 */
		tv.tv_sec = 20;
		tv.tv_usec = 0;
	} while (status == RPC_TIMEDOUT);

	if (printed_waiting_msg)
		printf("Bootparam response received\n");

	if (status != RPC_SUCCESS) {
		/*
		 * XXX should get real error here
		 */
		error = (int)status;
		printf("whoami RPC call failed with status %d\n", error);
		goto done;
	}

	hostnamelen = strlen(res.client_name);
	if (hostnamelen > sizeof hostname) {
		printf("whoami: hostname too long");
		error = ENAMETOOLONG;
		goto done;
	}
	if (hostnamelen > 0) {
		bcopy((caddr_t)res.client_name, (caddr_t)hostname,
		    (u_int)hostnamelen);
		bcopy((caddr_t)res.client_name, (caddr_t)utsname.nodename,
		    (u_int)hostnamelen);
	} else {
		printf("whoami: no host name\n");
		error = ENXIO;
		goto done;
	}
	printf("hostname: %s\n", hostname);

	domainnamelen = strlen(res.domain_name);
	if (domainnamelen > sizeof domainname) {
		printf("whoami: domainname too long");
		error = ENAMETOOLONG;
		goto done;
	}
	if (domainnamelen > 0) {
		bcopy((caddr_t)res.domain_name, (caddr_t)domainname,
		    (u_int)domainnamelen);
		printf("domainname: %s\n", domainname);
	} else {
		printf("whoami: no domain name\n");
	}

	bcopy((caddr_t)&res.router_address.bp_address.ip_addr, (caddr_t)&ipaddr,
	    sizeof (struct in_addr));
	if (ipaddr.s_addr != (u_long) 0) {
		if (res.router_address.address_type == IP_ADDR_TYPE) {
			sin = (struct sockaddr_in *)&rtentry.rt_dst;
			bzero((caddr_t)sin, sizeof *sin);
			sin->sin_family = AF_INET;
			sin = (struct sockaddr_in *)&rtentry.rt_gateway;
			bzero((caddr_t)sin, sizeof *sin);
			sin->sin_family = AF_INET;
			sin->sin_addr.s_addr = ipaddr.s_addr;
			rtentry.rt_flags = RTF_GATEWAY | RTF_UP;
			(void) rtrequest(SIOCADDRT, &rtentry);
		} else {
			printf("whoami: unknown gateway addr family %d\n",
			    res.router_address.address_type);
		}
	}

done:
	kmem_free(res.client_name, MAX_MACHINE_NAME + 1);
	kmem_free(res.domain_name, MAX_MACHINE_NAME + 1);
	return (error);
}

static enum clnt_stat
callrpc(sin, prognum, versnum, procnum, inproc, in, outproc, out)
	struct sockaddr_in *sin;
	u_long prognum, versnum, procnum;
	xdrproc_t inproc, outproc;
	char *in, *out;
{
	CLIENT *cl;
	struct timeval tv;
	enum clnt_stat cl_stat;

	cl = clntkudp_create(sin, prognum, versnum, 5, u.u_cred);
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	cl_stat = CLNT_CALL(cl, procnum, inproc, in, outproc, out, tv);
	AUTH_DESTROY(cl->cl_auth);
	CLNT_DESTROY(cl);
	return (cl_stat);
}

static int
getfile(fileid, server_name, server_address, server_path)
	char *fileid;
	char *server_name;
	struct sockaddr *server_address;
	char *server_path;
{
	struct bp_getfile_arg arg;
	struct bp_getfile_res res;
	enum clnt_stat status;
	int tries;
	int error;
	struct in_addr ipaddr;

	arg.client_name = hostname;
	arg.file_id = fileid;
	bzero((caddr_t)&res, sizeof res);
	if (bootparam_addr.sin_addr.s_addr == 0) {
		if ((error = whoami()) != 0)
			return (error);
	}
	res.server_name = (bp_machine_name_t)new_kmem_alloc(
					MAX_MACHINE_NAME + 1, KMEM_SLEEP);
	res.server_path = (bp_machine_name_t)new_kmem_alloc(
					MAX_MACHINE_NAME + 1, KMEM_SLEEP);
	for (tries = 0; tries < 5; tries++) {
		status = callrpc(&bootparam_addr, (u_long)BOOTPARAMPROG,
		    (u_long)BOOTPARAMVERS, (u_long)BOOTPARAMPROC_GETFILE,
		    xdr_bp_getfile_arg, (caddr_t)&arg,
		    xdr_bp_getfile_res, (caddr_t)&res);
		if (status != RPC_TIMEDOUT)
			break;
	}
	if (status == RPC_SUCCESS) {
		(void) strcpy(server_name, res.server_name);
		(void) strcpy(server_path, res.server_path);
	}
	kmem_free(res.server_name, MAX_MACHINE_NAME + 1);
	kmem_free(res.server_path, MAX_MACHINE_NAME + 1);
	if (status != RPC_SUCCESS)
		return ((status == RPC_TIMEDOUT) ? ETIMEDOUT : (int)status);

	bcopy((caddr_t)&res.server_address.bp_address.ip_addr, (caddr_t)&ipaddr,
	    sizeof (struct in_addr));
	if (*server_name == '\0' || *server_path == '\0' ||
	    ipaddr.s_addr == 0) {
		return (EINVAL);
	}
	switch (res.server_address.address_type) {
	case IP_ADDR_TYPE:
		bzero((caddr_t)server_address, sizeof *server_address);
		server_address->sa_family = AF_INET;
		satosin(server_address)->sin_addr.s_addr = ipaddr.s_addr;
		break;
	default:
		printf("getfile: unknown address type %d\n",
			res.server_address.address_type);
		return (EPROTONOSUPPORT);
	}
	return (0);
}

/*
 * Call mount daemon on server sin to mount path.
 * sin_port is set to nfs port and fh is the fhandle
 * returned from the server.
 */
static
mountnfs(sin, server, path, fh)
	struct sockaddr_in *sin;
	char *server;
	char *path;
	fhandle_t *fh;
{
	struct fhstatus fhs;
	int error;
	enum clnt_stat status;

	do {
		error = pmap_kgetport(sin, (u_long)MOUNTPROG,
		    (u_long)MOUNTVERS, (u_long)IPPROTO_UDP);
		if (error == -1) {
			return ((int)RPC_PROGNOTREGISTERED);
		} else if (error == 1) {
			printf("mountnfs: %s:%s portmap not responding\n",
			    server, path);
		}
	} while (error == 1);
	do {
		status = callrpc(sin, (u_long)MOUNTPROG, (u_long)MOUNTVERS,
		    (u_long)MOUNTPROC_MNT,
		    xdr_bp_path_t, (caddr_t)&path,
		    xdr_fhstatus, (caddr_t)&fhs);
		if (status == RPC_TIMEDOUT) {
			printf("mountnfs: %s:%s mount server not responding\n",
			    server, path);
		}
	} while (status == RPC_TIMEDOUT);
	if (status != RPC_SUCCESS) {
		return ((int)status);
	}
	sin->sin_port = htons(NFS_PORT);
	*fh = fhs.fhs_fh;
	return ((int)fhs.fhs_status);
}

/*
 * nfs mount vfsop
 * Set up mount info record and attach it to vfs struct.
 */
/*ARGSUSED*/
static int
nfs_mount(vfsp, path, data)
	struct vfs *vfsp;
	char *path;
	caddr_t data;
{
	int error;
	struct vnode *rtvp = NULL;	/* the server's root */
	struct mntinfo *mi;		/* mount info, pointed at by vfs */
	fhandle_t fh;			/* root fhandle */
	struct sockaddr_in saddr;	/* server's address */
	char shostname[HOSTNAMESZ];	/* server's hostname */
	int hlen;			/* length of hostname */
	char netname[MAXNETNAMELEN+1];	/* server's netname */
	int nlen;			/* length of netname */
	struct nfs_args args;		/* nfs mount arguments */

	/*
	 * get arguments
	 */
	error = copyin(data, (caddr_t)&args, sizeof (args));
	if (error) {
		goto errout;
	}
	/*
	 * Remounts need to save the pathconf information,
	 * Part of the infamous static pathconf kludge.
	 */
	if (vfsp->vfs_flag & VFS_REMOUNT) {
		return (pathconf_get((struct mntinfo*)vfsp->vfs_data, &args));
	}
	/*
	 * Get server address
	 */
	error = copyin((caddr_t)args.addr, (caddr_t)&saddr,
	    sizeof (saddr));
	if (error) {
		goto errout;
	}
	/*
	 * For now we just support AF_INET
	 */
	if (saddr.sin_family != AF_INET) {
		error = EPFNOSUPPORT;
		goto errout;
	}

	/*
	 * Get the root fhandle
	 */
	error = copyin((caddr_t)args.fh, (caddr_t)&fh, sizeof (fh));
	if (error) {
		goto errout;
	}

	/*
	 * Get server's hostname
	 */
	if (args.flags & NFSMNT_HOSTNAME) {
		error = copyinstr(args.hostname, shostname,
			sizeof (shostname), (u_int *)&hlen);
		if (error) {
			goto errout;
		}
	} else {
		extern char *inet_ntoa();

		(void) strcpy(inet_ntoa(saddr.sin_addr), shostname);
	}

	/*
	 * Get server's netname
	 */
	if (args.flags & NFSMNT_SECURE) {
		error = copyinstr(args.netname, netname, sizeof (netname),
			(u_int *)&nlen);
	} else {
		nlen = -1;
	}

	/*
	 * Get root vnode.
	 */
	error = nfsrootvp(&rtvp, vfsp, &saddr, &fh, shostname, netname, nlen,
	    args.flags);
	if (error)
		return (error);

	/*
	 * Set option fields in mount info record
	 */
	mi = vtomi(rtvp);
	mi->mi_noac = ((args.flags & NFSMNT_NOAC) != 0);
	mi->mi_nocto = ((args.flags & NFSMNT_NOCTO) != 0);
	if (args.flags & NFSMNT_RETRANS) {
		mi->mi_retrans = args.retrans;
		if (args.retrans < 0) {
			error = EINVAL;
			goto errout;
		}
	}
	if (args.flags & NFSMNT_TIMEO) {
		/*
		 * With dynamic retransmission, the mi_timeo is used only
		 * as a hint for the first one. The deviation is stored in
		 * units of hz shifted left by two, or 5msec. Since timeo
		 * was in units of 100msec, multiply by 20 to convert.
		 * rtxcur is in unscaled ticks, so multiply by 5.
		 */
		mi->mi_timeo = args.timeo;
		mi->mi_timers[3].rt_deviate = (args.timeo*hz*2)/5;
		mi->mi_timers[3].rt_rtxcur = args.timeo*hz/10;
		if (args.timeo <= 0) {
			error = EINVAL;
			goto errout;
		}
	}
	if (args.flags & NFSMNT_RSIZE) {
		if (args.rsize <= 0) {
			error = EINVAL;
			goto errout;
		}
		mi->mi_tsize = MIN(mi->mi_tsize, args.rsize);
		mi->mi_curread = mi->mi_tsize;
	}
	if (args.flags & NFSMNT_WSIZE) {
		if (args.wsize <= 0) {
			error = EINVAL;
			goto errout;
		}
		mi->mi_stsize = MIN(mi->mi_stsize, args.wsize);
		mi->mi_curwrite = mi->mi_stsize;
	}
	if (args.flags & NFSMNT_ACREGMIN) {
		if (args.acregmin < 0) {
			mi->mi_acregmin = ACMINMAX;
		} else if (args.acregmin == 0) {
			error = EINVAL;
			printf("nfs_mount: acregmin == 0\n");
			goto errout;
		} else {
			mi->mi_acregmin = imin(args.acregmin, ACMINMAX);
		}
	}
	if (args.flags & NFSMNT_ACREGMAX) {
		if (args.acregmax < 0) {
			mi->mi_acregmax = ACMAXMAX;
		} else if (args.acregmax < mi->mi_acregmin) {
			error = EINVAL;
			printf("nfs_mount: acregmax < acregmin\n");
			goto errout;
		} else {
			mi->mi_acregmax = imin(args.acregmax, ACMAXMAX);
		}
	}
	if (args.flags & NFSMNT_ACDIRMIN) {
		if (args.acdirmin < 0) {
			mi->mi_acdirmin = ACMINMAX;
		} else if (args.acdirmin == 0) {
			error = EINVAL;
			printf("nfs_mount: acdirmin == 0\n");
			goto errout;
		} else {
			mi->mi_acdirmin = imin(args.acdirmin, ACMINMAX);
		}
	}
	if (args.flags & NFSMNT_ACDIRMAX) {
		if (args.acdirmax < 0) {
			mi->mi_acdirmax = ACMAXMAX;
		} else if (args.acdirmax < mi->mi_acdirmin) {
			error = EINVAL;
			printf("nfs_mount: acdirmax < acdirmin\n");
			goto errout;
		} else {
			mi->mi_acdirmax = imin(args.acdirmax, ACMAXMAX);
		}
	}
	if (mi->mi_noac) {
		mi->mi_acregmin = 0;
		mi->mi_acregmax = 0;
		mi->mi_acdirmin = 0;
		mi->mi_acdirmax = 0;
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 1,
	    "nfs_mount: hard %d timeo %d retries %d wsize %d rsize %d\n",
	    mi->mi_hard, mi->mi_timeo, mi->mi_retrans, mi->mi_stsize,
	    mi->mi_tsize);
	dprint(nfsdebug, 1,
	    "           regmin %d regmax %d dirmin %d dirmax %d\n",
	    mi->mi_acregmin, mi->mi_acregmax, mi->mi_acdirmin, mi->mi_acdirmax);
#endif
	error = pathconf_get(mi, &args);	/* static pathconf kludge */
errout:
	if (error) {
		if (rtvp) {
			VN_RELE(rtvp);
		}
	}
	return (error);
}

/*
 * static pathconf kludge BEGIN
 *
 * The pathconf information is kept on a linked list of kmemalloc'ed
 * structs.  We search the list & add a new struct iff there is no
 * other struct with the same information.
 * See sys/pathconf.h for ``the rest of the story.''
 */
struct	pathcnf *allpc = NULL;

static int
pathconf_get(mi, ap)
	struct mntinfo *mi;		/* mount info, pointed at by vfs */
	struct nfs_args *ap;		/* nfs mount arguments */
{
	int err;

	if (mi->mi_pathconf)
	    pathconf_rele(mi);
	mi->mi_pathconf = NULL;
	if ((ap->flags & NFSMNT_POSIX) && ap->pathconf) {
		struct pathcnf pc;
		register struct pathcnf *p;

		err = copyin((caddr_t)ap->pathconf, (caddr_t)&pc, sizeof pc);
		if (err)
			return (err);
		if (_PC_ISSET(_PC_ERROR, pc.pc_mask))
			return (EINVAL);
		for (p=allpc; p; p=p->pc_next)
			if (PCCMP(p, &pc) == 0)
				break;
		if (p) {
			mi->mi_pathconf = p;
			p->pc_refcnt++;
		} else {
			p = (struct pathcnf *)
				new_kmem_alloc(sizeof *p, KMEM_SLEEP);
			*p = pc;
			p->pc_next = allpc;
			p->pc_refcnt = 1;
			allpc = mi->mi_pathconf = p;
		}
	}
	return (0);
}

/*
 * release the static pathconf information
 */
static int
pathconf_rele(mi)
	struct mntinfo *mi;
{
	if (mi->mi_pathconf) {
		if (!--mi->mi_pathconf->pc_refcnt) {
			register struct pathcnf *p;
			register struct pathcnf *p2;

			for (p2 = p = allpc; p && p != mi->mi_pathconf; ) {
				p2 = p;
				p = p->pc_next;
			}
			if (!p)
				panic("mi->pathconf");
			if (p == allpc)
				allpc = p->pc_next;
			else
				p2->pc_next = p->pc_next;
			kmem_free((caddr_t)p, (u_int)sizeof *p);
			mi->mi_pathconf = NULL;
		}
	}
} /* static pathconf kludge END */

/*
 * Fix for bug 1038327 - corbin 7/18/90
 * Global variable to disable NFS dynamic retransmission feature for the
 * client.  This should be a mount option. XXX
 */
int nfs_dynamic = 1;

static int
nfsrootvp(rtvpp, vfsp, sin, fh, shostname, netname, nlen, flags)
	struct vnode **rtvpp;		/* where to return root vp */
	register struct vfs *vfsp;	/* vfs of fs, if NULL make one */
	struct sockaddr_in *sin;	/* server address */
	fhandle_t *fh;			/* swap file fhandle */
	char *shostname;		/* server's hostname */
	char *netname;			/* server's netname */
	int nlen;			/* length of netname, -1 if none */
	int flags;			/* mount flags */
{
	register struct vnode *rtvp;	/* the server's root */
	register struct mntinfo *mi;	/* mount info, pointed at by vfs */
	struct vattr va;		/* root vnode attributes */
	struct nfsfattr na;		/* root vnode attributes in nfs form */
	struct statfs sb;		/* server's file system stats */
	register int error;
	struct ifaddr *ifa;

	rtvp = NULL;
	mi = NULL;
	/*
	 * Create a mount record and link it to the vfs struct.
	 */
	mi = (struct mntinfo *)new_kmem_zalloc(sizeof (*mi), KMEM_SLEEP);
	mi->mi_hard = ((flags & NFSMNT_SOFT) == 0);
	mi->mi_int = ((flags & NFSMNT_INT) != 0);
	mi->mi_printed = 0;			/* bug 1038327  */
	mi->mi_usertold = 0;			/* bug 1038327  */
	mi->mi_addr = *sin;
	mi->mi_retrans = NFS_RETRIES;
	mi->mi_timeo = NFS_TIMEO;
	mi->mi_mntno = vfs_getnum(nfs_minmap, MAPSIZE);
	bcopy(shostname, mi->mi_hostname, HOSTNAMESZ);
	mi->mi_acregmin = ACREGMIN;
	mi->mi_acregmax = ACREGMAX;
	mi->mi_acdirmin = ACDIRMIN;
	mi->mi_acdirmax = ACDIRMAX;
	mi->mi_netnamelen = nlen;
	if (nlen >= 0) {
		mi->mi_netname = new_kmem_alloc((u_int)nlen, KMEM_SLEEP);
		bcopy(netname, mi->mi_netname, (u_int)nlen);
	}
	mi->mi_authflavor =
		(flags & NFSMNT_SECURE) ? AUTH_DES : AUTH_UNIX;

	/*
	 * Use heuristic to turn off transfer size adjustment
	 */
	ifa = ifa_ifwithdstaddr((struct sockaddr *)sin);
	if (ifa == (struct ifaddr *)0)
		ifa = ifa_ifwithnet((struct sockaddr *)sin);
	/*
	 * Fix for bug 1038327 - corbin 7/18/90
	 * Give the user control over the use of dynamic retransmission.
	 * This should be a mount option. XXX
	 */
	mi->mi_dynamic = (nfs_dynamic == 1 &&
		(ifa == (struct ifaddr *)0 ||
		 ifa->ifa_ifp == (struct ifnet *)0 ||
		 ifa->ifa_ifp->if_mtu < ETHERMTU));
	/*
	 * Make a vfs struct for nfs.  We do this here instead of below
	 * because rtvp needs a vfs before we can do a getattr on it.
	 */
	vfsp->vfs_fsid.val[0] = mi->mi_mntno;
	vfsp->vfs_fsid.val[1] = MOUNT_NFS;
	vfsp->vfs_data = (caddr_t)mi;

	/*
	 * Make the root vnode, use it to get attributes,
	 * then remake it with the attributes.
	 */
	rtvp = makenfsnode(fh, (struct nfsfattr *)0, vfsp);
	if ((rtvp->v_flag & VROOT) != 0) {
		error = EINVAL;
		goto bad;
	}
	rtvp->v_flag |= VROOT;
	error = VOP_GETATTR(rtvp, &va, u.u_cred);
	if (error)
		goto bad;
	VN_RELE(rtvp);
	vattr_to_nattr(&va, &na);
	rtvp = makenfsnode(fh, &na, vfsp);
	rtvp->v_flag |= VROOT;
	mi->mi_rootvp = rtvp;

	/*
	 * Get server's filesystem stats.  Use these to set transfer
	 * sizes, filesystem block size, and read-only.
	 */
	error = VFS_STATFS(vfsp, &sb);
	if (error)
		goto bad;
	mi->mi_tsize = min(NFS_MAXDATA, (u_int)nfstsize());
	mi->mi_curread = mi->mi_tsize;
	mi->mi_lastmod = time_in_hz;		/* bug 1038327  */

	/*
	 * Set filesystem block size to maximum data transfer size
	 */
	mi->mi_bsize = NFS_MAXDATA;
	vfsp->vfs_bsize = mi->mi_bsize;

	/*
	 * Need credentials in the rtvp so do_bio can find them.
	 */
	crhold(u.u_cred);
	vtor(rtvp)->r_cred = u.u_cred;

	*rtvpp = rtvp;
	return (0);
bad:
	if (mi) {
		if (mi->mi_netnamelen >= 0) {
			kmem_free((caddr_t)mi->mi_netname,
			    (u_int)mi->mi_netnamelen);
		}
		kmem_free((caddr_t)mi, sizeof (*mi));
	}
	if (rtvp) {
		VN_RELE(rtvp);
	}
	*rtvpp = NULL;
	return (error);
}

/*
 * vfs operations
 */
static int
nfs_unmount(vfsp)
	struct vfs *vfsp;
{
	struct mntinfo *mi = (struct mntinfo *)vfsp->vfs_data;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_unmount(%x) mi = %x\n", vfsp, mi);
#endif
	rflush(vfsp);
	rinval(vfsp);

	if (mi->mi_refct != 1 || mi->mi_rootvp->v_count != 1) {
		return (EBUSY);
	}
	/*
	 * Release root vnode -- but first manually remove its identity
	 * and pages, since VN_RELE() might decide to hang onto them
	 */
	rp_rmhash(vtor(mi->mi_rootvp));
	rinactive(vtor(mi->mi_rootvp));
	VN_RELE(mi->mi_rootvp);
	vfs_putnum(nfs_minmap, mi->mi_mntno);
	if (mi->mi_netnamelen >= 0) {
		kmem_free((caddr_t)mi->mi_netname, (u_int)mi->mi_netnamelen);
	}
	pathconf_rele(mi);	/* static pathconf kludge */
	kmem_free((caddr_t)mi, sizeof (*mi));
	return (0);
}

/*
 * find root of nfs
 */
static int
nfs_root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{

	*vpp = (struct vnode *)((struct mntinfo *)vfsp->vfs_data)->mi_rootvp;
	(*vpp)->v_count++;
#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_root(0x%x) = %x\n", vfsp, *vpp);
#endif
	VFS_RECORD(vfsp, VS_ROOT, VS_CALL);
	return (0);
}

/*
 * Get file system statistics.
 */
static int
nfs_statfs(vfsp, sbp)
	register struct vfs *vfsp;
	struct statfs *sbp;
{
	struct nfsstatfs fs;
	struct mntinfo *mi;
	fhandle_t *fh;
	int error = 0;

	mi = vftomi(vfsp);
	fh = vtofh(mi->mi_rootvp);
#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_statfs vfs %x\n", vfsp);
#endif
	VFS_RECORD(vfsp, VS_STATFS, VS_CALL);
	VFS_RECORD(vfsp, VS_STATFS, VS_MISS);
	error = rfscall(mi, RFS_STATFS, xdr_fhandle,
	    (caddr_t)fh, xdr_statfs, (caddr_t)&fs, u.u_cred);
	if (!error) {
		error = geterrno(fs.fs_status);
	}
	if (!error) {
		if (mi->mi_stsize) {
			mi->mi_stsize = MIN(mi->mi_stsize, fs.fs_tsize);
		} else {
			mi->mi_stsize = fs.fs_tsize;
			mi->mi_curwrite = mi->mi_stsize;
		}
		sbp->f_bsize = fs.fs_bsize;
		sbp->f_blocks = fs.fs_blocks;
		sbp->f_bfree = fs.fs_bfree;
		sbp->f_bavail = fs.fs_bavail;
		sbp->f_files = -1;
		sbp->f_ffree = -1;
		/*
		 * XXX - This is wrong, should be a real fsid
		 */
		bcopy((caddr_t)&vfsp->vfs_fsid,
		    (caddr_t)&sbp->f_fsid, sizeof (fsid_t));
	}
#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfs_statfs returning %d\n", error);
#endif
	return (error);
}

/*
 * Flush dirty nfs files for file system vfsp.
 * If vfsp == NULL, all nfs files are flushed.
 */
static int
nfs_sync(vfsp)
	struct vfs *vfsp;
{
	static int nfslock;

	if (nfslock == 0) {
#ifdef	NFSDEBUG
		dprint(nfsdebug, 5, "nfs_sync\n");
#endif
		nfslock++;
		rflush(vfsp);
		nfslock = 0;
	}
	return (0);
}

static int
nfs_badop()
{

	panic("nfs_badop");
}

static
hostpath(name, shostname, path)
	char *name;
	char *shostname;
	char *path;
{
	register char *nm;

	(void) strcpy(name, shostname);
	for (nm = name; *nm; nm++)
		;
	*nm++ = ':';
	(void) strcpy(nm, path);
}
