#ident	"@(#)nfs_subr.c 1.1 92/07/30 SMI";

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/session.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/trace.h>

#include <net/if.h>
#include <netinet/in.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>

#ifdef	TRACE
#ifndef	NFSSERVER
#define	NFSSERVER
#include <nfs/nfs.h>
#undef	NFSSERVER
#else	NFSSERVER
#include <nfs/nfs.h>
#endif	NFSSERVER
#else	TRACE
#include <nfs/nfs.h>
#endif	TRACE

#include <nfs/nfs_clnt.h>
#include <nfs/rnode.h>
#include <sun/consdev.h>
#include <vm/pvn.h>
#include <vm/swap.h>

/*
 * Variables for maintaining the free list of rnode structures.
 */
static struct rnode *rn_free;
#if !(PAGESIZE - 4096)
static u_int nrn_incr = (3*PAGESIZE / (sizeof(struct rnode)));
#else
static u_int nrn_incr = (PAGESIZE / (sizeof(struct rnode)));
#endif PAGESIZE

/*
 * Client handle used to keep the pagedaemon from deadlocking waiting for mem.
 * XXX - bugid 1026717
 */
struct chtab  *ch_pagedaemon = (struct chtab *)NULL;

#ifdef NFSDEBUG
extern int nfsdebug;
#endif

extern struct vnodeops nfs_vnodeops;
static struct rnode *rfind();

extern int nrnode;		/* max rnodes to alloc, set in machdep.c */

/*
 * Client side utilities
 */

/*
 * client side statistics
 */
struct {
	u_int	nclsleeps;		/* client handle waits */
	u_int	nclgets;		/* client handle gets */
	u_int	ncalls;			/* client requests */
	u_int	nbadcalls;		/* rpc failures */
	u_int	reqs[32];		/* count of each request */
} clstat;

struct chtab {
	struct chtab *ch_next, *ch_prev;
	u_int	ch_timesused;
	bool_t	ch_inuse;
	CLIENT	*ch_client;
} chtabhead = { &chtabhead, &chtabhead, 0, 0, NULL };

extern u_int nchtable;
u_int nchtotal;
u_int nchfree;

u_int authdes_win = (60*60);	/* one hour -- should be mount option */

struct desauthent {
	struct mntinfo *da_mi;
	uid_t da_uid;
	short da_inuse;
	AUTH *da_auth;
} *desauthtab = NULL;
extern u_int ndesauthtab;
int nextdesvictim;

struct unixauthent {
	short ua_inuse;
	AUTH *ua_auth;
} *unixauthtab = NULL;
extern u_int nunixauthtab;
int nextunixvictim;

AUTH *
authget(mi, cr)
	struct mntinfo *mi;
	struct ucred *cr;
{
	int i;
	AUTH *auth;
	register struct unixauthent *ua;
	register struct desauthent *da;
	int authflavor;
	struct ucred *savecred;

	authflavor = mi->mi_authflavor;
	for (;;) switch (authflavor) {
	case AUTH_NONE:
		/*
		 * XXX: should do real AUTH_NONE, instead of AUTH_UNIX
		 */
	case AUTH_UNIX:
		if (unixauthtab == NULL) {
		/* XXX this should be in some initialization code */
			unixauthtab = (struct unixauthent *)
			    new_kmem_zalloc(
				nunixauthtab * sizeof (struct unixauthent),
				KMEM_SLEEP);
		}
		i = nunixauthtab;

		do {
			ua = &unixauthtab[nextunixvictim++];
			nextunixvictim %= nunixauthtab;
		} while (ua->ua_inuse && --i > 0);

		if (ua->ua_inuse) {
			/* overflow of unix auths */
			return (authkern_create());
		}

		if (ua->ua_auth == NULL) {
			ua->ua_auth = authkern_create();
		}
		ua->ua_inuse = 1;
		return (ua->ua_auth);

	case AUTH_DES:
		if (desauthtab == NULL) {
		/* XXX this should be in some initialization code */
			desauthtab = (struct desauthent *)
			    new_kmem_zalloc(
				ndesauthtab * sizeof (struct desauthent),
				KMEM_SLEEP);
		}
		for (da = desauthtab; da < &desauthtab[ndesauthtab]; da++) {
			if (da->da_mi == mi && da->da_uid == cr->cr_uid &&
			    !da->da_inuse && da->da_auth != NULL) {
				da->da_inuse = 1;
				return (da->da_auth);
			}
		}

		savecred = u.u_cred;
		u.u_cred = cr;
		auth = authdes_create(mi->mi_netname, authdes_win,
				&mi->mi_addr, (des_block *)NULL);
		u.u_cred = savecred;

		if (auth == NULL) {
			printf("authget: authdes_create failure\n");
			authflavor = AUTH_UNIX;
			continue;
		}

		i = ndesauthtab;
		do {
			da = &desauthtab[nextdesvictim++];
			nextdesvictim %= ndesauthtab;
		} while (da->da_inuse && --i > 0);

		if (da->da_inuse) {
			/* overflow of des auths */
			return (auth);
		}

		if (da->da_auth != NULL) {
			auth_destroy(da->da_auth);	/* should reuse!!! */
		}

		da->da_auth = auth;
		da->da_inuse = 1;
		da->da_uid = cr->cr_uid;
		da->da_mi = mi;
		return (da->da_auth);

	default:
		/*
		 * auth create must have failed, try AUTH_NONE
		 * (this relies on AUTH_NONE never failing)
		 */
		printf("authget: unknown authflavor %d\n", authflavor);
		authflavor = AUTH_NONE;
	}
}

authfree(auth)
	AUTH *auth;
{
	register struct unixauthent *ua;
	register struct desauthent *da;

	switch (auth->ah_cred.oa_flavor) {
	case AUTH_NONE: /* XXX: do real AUTH_NONE */
	case AUTH_UNIX:
		for (ua = unixauthtab; ua < &unixauthtab[nunixauthtab]; ua++) {
			if (ua->ua_auth == auth) {
				ua->ua_inuse = 0;
				return;
			}
		}
		auth_destroy(auth);	/* was overflow */
		break;
	case AUTH_DES:
		for (da = desauthtab; da < &desauthtab[ndesauthtab]; da++) {
			if (da->da_auth == auth) {
				da->da_inuse = 0;
				return;
			}
		}
		auth_destroy(auth);	/* was overflow */
		break;
	default:
		printf("authfree: unknown authflavor %d\n",
			auth->ah_cred.oa_flavor);
		break;
	}
}


CLIENT *
clget(mi, cred)
	struct mntinfo *mi;
	struct ucred *cred;
{
	register struct chtab *ch;
	int retrans;

	/*
	 * If soft mount and server is down just try once.
	 * Interruptable hard mounts get counted at this level.
	 */
	if ((!mi->mi_hard && mi->mi_down) || (mi->mi_int && mi->mi_hard)) {
		retrans = 1;
	} else {
		retrans = mi->mi_retrans;
	}

	/*
	 * If the pagedaemon wants a handle, then give it its pre-allocated
	 * handle.  If the pagedaemon gets here first and there isn't one yet,
	 * then it has to take its chances... XXX
	 */
	if ((u.u_procp == &proc[2]) && (ch_pagedaemon != (struct chtab *)NULL))
{
		ch = ch_pagedaemon;
		if (ch->ch_inuse)
			panic("clget:  pagedaemon needs > 1 cl handle!!\n");
		ch->ch_inuse = TRUE;
		if (ch->ch_client == (CLIENT *)NULL)
			panic("clget:  null client structure\n");
		clntkudp_init(ch->ch_client, &mi->mi_addr, retrans, cred);
		goto out;
	}

	/*
	 * Find an unused handle
	 */
	clstat.nclgets++;
	for (ch = chtabhead.ch_next; ch != &chtabhead; ch = ch->ch_next) {
		if (!ch->ch_inuse) {
			ch->ch_inuse = TRUE;
			clntkudp_init(ch->ch_client, &mi->mi_addr,
					retrans, cred);
			nchfree--;
			goto out;
		}
	}

again:

	/*
	 *  If we get here, we need to make a new handle
	 */
	ch = (struct chtab *)new_kmem_zalloc(
				sizeof (struct chtab), KMEM_SLEEP);
	/*
	 * First allocation:  stash one away for the pagedaemon
	 */
	if (ch_pagedaemon == (struct chtab *)NULL) {
		ch_pagedaemon = ch;
		ch->ch_client = clntkudp_create(&mi->mi_addr, NFS_PROGRAM,
					NFS_VERSION, retrans, cred);
		if (ch->ch_client == NULL) {
			panic("clget: null client");
		}
		auth_destroy(ch->ch_client->cl_auth); /* XXX */
		if (u.u_procp != &proc[2]) {
			ch->ch_inuse = FALSE;	/* pagedaemon will use later */
			goto again;
		} else {
			ch->ch_inuse = TRUE;	/* pagedaemon will use now */
			goto out;
		}
	}

	ch->ch_inuse = TRUE;
	ch->ch_next = chtabhead.ch_next;
	ch->ch_prev = &chtabhead;
	chtabhead.ch_next->ch_prev = ch;
	chtabhead.ch_next = ch;
	ch->ch_client = clntkudp_create(&mi->mi_addr, NFS_PROGRAM, NFS_VERSION,
					retrans, cred);
	if (ch->ch_client == NULL) {
		panic("clget: null client");
	}
	auth_destroy(ch->ch_client->cl_auth); /* XXX */
	nchtotal++;
out:
	ch->ch_client->cl_auth = authget(mi, cred);
	if (ch->ch_client->cl_auth == NULL)
		panic("clget: null auth");
	ch->ch_timesused++;
	return (ch->ch_client);

}

clfree(cl)
	CLIENT *cl;
{
	register struct chtab *ch;

	authfree(cl->cl_auth);
	cl->cl_auth = NULL;
	/*
	 * If we're deallocating the pagedaemon's allotted handle, just hang
	 * onto it instead.
	 */
	if (ch_pagedaemon != (struct chtab *)NULL) {
		if (cl == ch_pagedaemon->ch_client) {
			ch_pagedaemon->ch_inuse = FALSE;
			if (u.u_procp != &proc[2])
				panic("clfree: paged doesn't own its handle\n");
			return;
		}
	}
	for (ch = chtabhead.ch_next; ch != &chtabhead; ch = ch->ch_next) {
		if (ch->ch_client == cl) {
			if (nchfree >= nchtable) {
				/*
				*  We have enough free client handles,  so
				*  we can simply pop this one back into the
				*  heap where someone else can use the space.
				*/
				ch->ch_prev->ch_next = ch->ch_next;
				ch->ch_next->ch_prev = ch->ch_prev;
				CLNT_DESTROY(cl);
				kmem_free((caddr_t)ch, sizeof (struct chtab));
				nchtotal--;
			} else {
				/*
				*  We don't have enough free client handles,
				*  so leave this one in the list.
				*/
				ch->ch_inuse = FALSE;
				nchfree++;
			}
			return;
		}
	}
	panic("clfree: can't find client\n");
}

char *rfsnames[] = {
	"null", "getattr", "setattr", "unused", "lookup", "readlink", "read",
	"unused", "write", "create", "remove", "rename", "link", "symlink",
	"mkdir", "rmdir", "readdir", "fsstat" };

/*
 * This table maps from NFS protocol number into call type.
 * Zero means a "Lookup" type call
 * One  means a "Read" type call
 * Two  means a "Write" type call
 * This is used to select a default time-out as given below.
 */
static char call_type[] = {
	0, 0, 1, 0, 0, 0, 1,
	0, 2, 2, 2, 2, 2, 2,
	2, 2, 1, 0 };

/*
 * Minimum time-out values indexed by call type
 * These units are in "eights" of a second to avoid multiplies
 */
static unsigned int minimum_timeo[] = {
	6, 7, 10 };

/*
 * Similar table, but to determine which timer to use
 * (only real reads and writes!)
 */
static char timer_type[] = {
	0, 0, 0, 0, 0, 0, 1,
	0, 2, 0, 0, 0, 0, 0,
	0, 0, 1, 0 };

/*
 * Back off for retransmission timeout, MAXTIMO is in hz of a sec
 */
#define	MAXTIMO	20*hz
#define	backoff(tim)	((((tim) << 1) > MAXTIMO) ? MAXTIMO : ((tim) << 1))

#define	MIN_NFS_TSIZE 512	/* minimum "chunk" of NFS IO */
#define	REDUCE_NFS_TIME (hz/2)	/* rtxcur we try to keep under */
#define	INCREASE_NFS_TIME (hz/3*8) /* srtt we try to keep under (scaled*8) */

/*
 * Function called when the RPC package notices that we are
 * re-transmitting, or when we get a response without retransmissions.
 */
void
nfs_feedback(flag, which, mi)
	int flag;
	int which;
	register struct mntinfo *mi;
{
	int kind;
	if (flag == FEEDBACK_REXMIT1) {
		if (mi->mi_timers[NFS_CALLTYPES].rt_rtxcur != 0 &&
		    mi->mi_timers[NFS_CALLTYPES].rt_rtxcur < REDUCE_NFS_TIME)
			return;
		if (mi->mi_curread > MIN_NFS_TSIZE) {
			mi->mi_curread /= 2;
			if (mi->mi_curread < MIN_NFS_TSIZE)
				mi->mi_curread = MIN_NFS_TSIZE;
		}
		if (mi->mi_curwrite > MIN_NFS_TSIZE) {
			mi->mi_curwrite /= 2;
			if (mi->mi_curwrite < MIN_NFS_TSIZE)
				mi->mi_curwrite = MIN_NFS_TSIZE;
		}
	} else if (flag == FEEDBACK_OK) {
		kind = timer_type[which];
		if (kind == 0) return;
		if (mi->mi_timers[kind].rt_srtt >= INCREASE_NFS_TIME)
			return;
		if (kind==1) {
			if (mi->mi_curread >= mi->mi_tsize)
				return;
			mi->mi_curread +=  MIN_NFS_TSIZE;
			if (mi->mi_curread > mi->mi_tsize/2)
				mi->mi_curread = mi->mi_tsize;
		}
		if (kind==2) {
			if (mi->mi_curwrite >= mi->mi_stsize)
				return;
			mi->mi_curwrite += MIN_NFS_TSIZE;
			if (mi->mi_curwrite > mi->mi_stsize/2)
				mi->mi_curwrite = mi->mi_stsize;
		}
	}
}

int
rfscall(mi, which, xdrargs, argsp, xdrres, resp, cred)
	register struct mntinfo *mi;
	int	 which;
	xdrproc_t xdrargs;
	caddr_t	argsp;
	xdrproc_t xdrres;
	caddr_t	resp;
	struct ucred *cred;
{
	CLIENT *client;
	register enum clnt_stat status;
	struct rpc_err rpcerr;
	struct timeval wait;
	struct ucred *newcred;
	int timeo;		/* in units of HZ, 20ms. */
	int count;
	bool_t tryagain;
	u_long xid = alloc_xid();
	int access_retry = 0;  /* Fix bug 1041409, set when retry for EACCESS. */
	CLIENT *client_tmp = (CLIENT *)NULL;    /* Fix bug 1041409 */

#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "rfscall: %x, %d, %x, %x, %x, %x\n",
	    mi, which, xdrargs, argsp, xdrres, resp);
#endif
	clstat.ncalls++;
	clstat.reqs[which]++;

	rpcerr.re_errno = 0;
	rpcerr.re_status = RPC_SUCCESS;
	newcred = NULL;
retry:
	client = clget(mi, cred);
	
		/* Fix bug 1041409, huey */
	if (access_retry)
	{
		access_retry = 0;
		clfree(client_tmp);
	}

	timeo = clntkudp_settimers(client,
	    &(mi->mi_timers[timer_type[which]]),
	    &(mi->mi_timers[NFS_CALLTYPES]),
	    (minimum_timeo[call_type[which]]*hz)>>3,
	    mi->mi_dynamic ? nfs_feedback : (void (*)()) 0, (caddr_t)mi, xid);


	/*
	 * If hard mounted fs, retry call forever unless hard error occurs.
	 * If interruptable, need to count retransmissions at this level.
	 */
	if (mi->mi_int && mi->mi_hard)
		count = mi->mi_retrans;
	else
		count = 1;
	do {
		tryagain = FALSE;

		wait.tv_sec = timeo / hz;
		wait.tv_usec = 1000000/hz * (timeo % hz);
		status = CLNT_CALL(client, which, xdrargs, argsp,
		    xdrres, resp, wait);
		switch (status) {
		case RPC_SUCCESS:
			break;

		/*
		 * Unrecoverable errors: give up immediately
		 */
		case RPC_AUTHERROR:
		case RPC_CANTENCODEARGS:
		case RPC_CANTDECODERES:
		case RPC_VERSMISMATCH:
		case RPC_PROGVERSMISMATCH:
		case RPC_PROGUNAVAIL:
		case RPC_PROCUNAVAIL:
		case RPC_CANTDECODEARGS:
			break;

		default:
			if (status == RPC_INTR) {
				tryagain = (bool_t)(mi->mi_hard && !mi->mi_int);
				if (tryagain)
					continue;
				rpcerr.re_status = RPC_INTR;
				rpcerr.re_errno = EINTR;
			} else
				tryagain = (bool_t)mi->mi_hard;

			if (tryagain) {
				timeo = backoff(timeo);
				if (--count > 0 && timeo < hz*15)
					continue;
				if (!mi->mi_printed) {
					mi->mi_printed = 1;
	printf("NFS server %s not responding still trying\n", mi->mi_hostname);
				}
				/* bug 1038327 - put user_told into mntinfo */
				if (!mi->mi_usertold &&
				    u.u_procp->p_sessp->s_ttyp &&
				    u.u_procp->p_sessp->s_ttyd != consdev &&
				    u.u_procp->p_sessp->s_ttyd != rconsdev) {
					mi->mi_usertold = 1;
	uprintf("NFS server %s not responding still trying\n", mi->mi_hostname);
				}
				if (mi->mi_dynamic) {
					/*
					 * If dynamic retransmission is on,
					 * return back to the vnode ops level
					 * on read or write calls
					 */
					if (timer_type[which] != 0) {
						clfree(client);
						if (newcred) {
							crfree(newcred);
						}
						return (ENFS_TRYAGAIN);
					}
				}
			}
		}
	} while (tryagain);

	if (status != RPC_SUCCESS) {
		clstat.nbadcalls++;
		mi->mi_down = 1;
		if (status != RPC_INTR) {
			CLNT_GETERR(client, &rpcerr);
			printf("NFS %s failed for server %s: %s\n",
				rfsnames[which], mi->mi_hostname,
				clnt_sperrno(status));
			if (u.u_procp->p_sessp->s_ttyp &&
			    u.u_procp->p_sessp->s_ttyd != consdev &&
			    u.u_procp->p_sessp->s_ttyd != rconsdev) {
				uprintf("NFS %s failed for server %s: %s\n",
					rfsnames[which], mi->mi_hostname,
					clnt_sperrno(status));
			}
		}
	} else if (resp && *(int *)resp == EACCES &&
	    newcred == NULL && cred->cr_uid == 0 && cred->cr_ruid != 0) {
		/*
		 * Boy is this a kludge!  If the reply status is EACCES
		 * it may be because we are root (no root net access).
		 * Check the real uid, if it isn't root make that
		 * the uid instead and retry the call.
		 */
		newcred = crdup(cred);
		cred = newcred;
		cred->cr_uid = cred->cr_ruid;
			/* Fix bug 1041409, huey 
			clfree(client);   */
		access_retry = 1;
		client_tmp = client;
		goto retry;
	} else if (mi->mi_hard) {
		if (mi->mi_printed) {
			printf("NFS server %s ok\n", mi->mi_hostname);
			mi->mi_printed = 0;
		}
		/* bug 1038327 - put user_told into mntinfo */
		if (mi->mi_usertold) {
			uprintf("NFS server %s ok\n", mi->mi_hostname);
			mi->mi_usertold = 0;
		}
	} else {
		mi->mi_down = 0;
	}

	clfree(client);
#ifdef NFSDEBUG
	dprint(nfsdebug, 7, "rfscall: returning %d\n", rpcerr.re_errno);
#endif
	if (newcred) {
		crfree(newcred);
	}
	/*
	 *	This should never happen, but we suspect something is
	 *	garbaging packets, so ...
	 */
	if (rpcerr.re_status != RPC_SUCCESS && rpcerr.re_errno == 0) {
		printf("rfscall: re_status %d, re_errno 0\n", rpcerr.re_status);
		panic("rfscall");
		/* NOTREACHED */
	}
	return (rpcerr.re_errno);
}

vattr_to_sattr(vap, sa)
	register struct vattr *vap;
	register struct nfssattr *sa;
{

	if (vap->va_mode == (unsigned short) -1)
		sa->sa_mode = (unsigned long) -1;
	else
		sa->sa_mode = vap->va_mode;

	if (vap->va_uid == (unsigned short) -1)
		sa->sa_uid = (unsigned long) -1;
	else if (vap->va_uid == (unsigned short)AU_NOAUDITID)
		sa->sa_uid = (unsigned long)AU_NOAUDITID;
	else
		sa->sa_uid = vap->va_uid;

	if (vap->va_gid == (unsigned short) -1)
		sa->sa_gid = (unsigned long) -1;
	else if (vap->va_gid == (unsigned short)AU_NOAUDITID)
		sa->sa_gid = (unsigned long)AU_NOAUDITID;
	else
		sa->sa_gid = vap->va_gid;

	sa->sa_size = vap->va_size;
	sa->sa_atime.tv_sec  = vap->va_atime.tv_sec;
	sa->sa_atime.tv_usec = vap->va_atime.tv_usec;
	sa->sa_mtime.tv_sec  = vap->va_mtime.tv_sec;
	sa->sa_mtime.tv_usec = vap->va_mtime.tv_usec;
}

setdiropargs(da, nm, dvp)
	struct nfsdiropargs *da;
	char *nm;
	struct vnode *dvp;
{

	da->da_fhandle = *vtofh(dvp);
	da->da_name = nm;
}

int
setdirgid(dvp)
	struct vnode *dvp;
{

	/*
	 * To determine the expected group-id of the created file:
	 *  1)	If the filesystem was not mounted with the Old-BSD-compatible
	 *	GRPID option, and the directory's set-gid bit is clear,
	 *	then use the process's gid.
	 *  2)	Otherwise, set the group-id to the gid of the parent directory.
	 */
	if (!(dvp->v_vfsp->vfs_flag & VFS_GRPID) &&
	    !(vtor(dvp)->r_attr.va_mode & VSGID))
		return ((int)u.u_gid);
	else
		return ((int)vtor(dvp)->r_attr.va_gid);
}

u_int
setdirmode(dvp, om)
	struct vnode *dvp;
	u_int om;
{

	/*
	 * Modify the expected mode (om) so that the set-gid bit matches
	 * that of the parent directory (dvp).
	 */
	om &= ~VSGID;
	if (vtor(dvp)->r_attr.va_mode & VSGID)
		om |= VSGID;
	return (om);
}

struct rnode *rpfreelist = NULL;
int rreuse, rnew, ractive, rreactive, rnfree, rnhash, rnpages;
short rno_free_at_front = 0;

/*
 * Return a vnode for the given fhandle.
 * If no rnode exists for this fhandle create one and put it
 * in a table hashed by fh_fsid and fs_fid.  If the rnode for
 * this fhandle is already in the table return it (ref count is
 * incremented by rfind.  The rnode will be flushed from the
 * table when nfs_inactive calls runsave.
 */
struct vnode *
makenfsnode(fh, attr, vfsp)
	fhandle_t *fh;
	struct nfsfattr *attr;
	struct vfs *vfsp;
{
	register struct rnode *rp;
	char newnode = 0;

	if ((rp = rfind(fh, vfsp)) == NULL) {
		if (rpfreelist &&
		    (rtov(rpfreelist)->v_pages == NULL || rnew >= nrnode)) {
			rp = rpfreelist;
			rpfreelist = rpfreelist->r_freef;
			rm_free(rp);
			rp_rmhash(rp);
			if (rtov(rp)->v_pages == NULL && rno_free_at_front > 0)
				rno_free_at_front--;
			rinactive(rp);
			rreuse++;
		} else {
			register struct rnode *trp;

			rp = (struct rnode *) new_kmem_fast_alloc((caddr_t *)
			&rn_free, sizeof(*rn_free), (int)nrn_incr, KMEM_SLEEP);
			/*
			 * There may be a race condition if someone else
			 * alloc's the rnode while we're asleep, so we
			 * check again and recover if found
			 */
			if ((trp = rfind(fh, vfsp)) != NULL)
			{
				kmem_fast_free((caddr_t *)&rn_free,
					       (caddr_t)rp);
				rp = trp;
				goto rnode_found;
			} else
				rnew++;
		}
		bzero((caddr_t)rp, sizeof (*rp));
		rp->r_fh = *fh;
#ifdef TRACE
		{
			int	tmp_buf[2];

			bcopy((caddr_t)fh->fh_data, (caddr_t)tmp_buf,
				sizeof(tmp_buf));
			trace6(TR_MP_RNODE, rtov(rp), fh->fh_fsid.val[0],
		    		fh->fh_fsid.val[1], fh->fh_len, tmp_buf[0],
		    		tmp_buf[1]);
		}
#endif
		rtov(rp)->v_count = 1;
		rtov(rp)->v_op = &nfs_vnodeops;
		if (attr) {
			rtov(rp)->v_type = n2v_type(attr);
			rtov(rp)->v_rdev = n2v_rdev(attr);
		}
		rtov(rp)->v_data = (caddr_t)rp;
		rtov(rp)->v_vfsp = vfsp;
		rp_addhash(rp);
		((struct mntinfo *)(vfsp->vfs_data))->mi_refct++;
		newnode++;
	}
rnode_found:
	if (attr) {
		if (!newnode) {
			nfs_cache_check(rtov(rp), attr->na_mtime,
			    attr->na_size);
		}
		nfs_attrcache(rtov(rp), attr);
	}
	return (rtov(rp));
}

/*
 * Rnode lookup stuff.
 * These routines maintain a table of rnodes hashed by fhandle so
 * that the rnode for an fhandle can be found if it already exists.
 * NOTE: RTABLESIZE must be a power of 2 for rtablehash to work!
 */

#define	BACK	0
#define	FRONT	1

#define	RTABLESIZE	64
#define	rtablehash(fh) \
	((fh->fh_data[2] ^ fh->fh_data[5] ^ fh->fh_data[15]) & (RTABLESIZE-1))

struct rnode *rtable[RTABLESIZE];

/*
 * Put a rnode in the hash table
 */
static
rp_addhash(rp)
	struct rnode *rp;
{

	rp->r_hash = rtable[rtablehash(rtofh(rp))];
	rtable[rtablehash(rtofh(rp))] = rp;
	rnhash++;
}

/*
 * Remove a rnode from the hash table
 */
rp_rmhash(rp)
	struct rnode *rp;
{
	register struct rnode *rt;
	register struct rnode *rtprev = NULL;

	rt = rtable[rtablehash(rtofh(rp))];
	while (rt != NULL) {
		if (rt == rp) {
			if (rtprev == NULL) {
				rtable[rtablehash(rtofh(rp))] = rt->r_hash;
			} else {
				rtprev->r_hash = rt->r_hash;
			}
			rnhash--;
			return;
		}
		rtprev = rt;
		rt = rt->r_hash;
	}
}

/*
 * Add an rnode to the front of the free list
 */


static
add_free(rp, front)
	register struct rnode *rp;
	int front;
{
	if (rp->r_freef != NULL) {
		return;
	}
	if (rpfreelist == NULL) {
		rp->r_freef = rp;
		rp->r_freeb = rp;
		rpfreelist = rp;
	} else {
		rp->r_freef = rpfreelist;
		rp->r_freeb = rpfreelist->r_freeb;
		rpfreelist->r_freeb->r_freef = rp;
		rpfreelist->r_freeb = rp;
		if (front != BACK) {
			rpfreelist = rp;
			rno_free_at_front++;
		}
	}
	rnfree++;
}

/*
 * Remove an rnode from the free list
 */
rm_free(rp)
	register struct rnode *rp;
{
	if (rp->r_freef == NULL) {
		return;
	}
	if (rp->r_freef == rp) {
		rpfreelist = NULL;
	} else {
		if (rp == rpfreelist) {
			rpfreelist = rp->r_freef;
		}
		rp->r_freeb->r_freef = rp->r_freef;
		rp->r_freef->r_freeb = rp->r_freeb;
	}
	rp->r_freef = rp->r_freeb = NULL;
	rnfree--;
}

/*
 * free resource for rnode
 */
rinactive(rp)
	struct rnode *rp;
{
	if (rtov(rp)->v_pages) {
		(void) VOP_PUTPAGE(rtov(rp), 0, 0, B_INVAL, rp->r_cred);
		/*
		 * XXX:  The following test should never be true but I
		 * have already fixed three separate bugs that cause it
		 * so I suppose that there may be more.  At any rate, I
		 * don't believe that there is any harm in truncing these
		 * pages here;  if we leave them they are likely to
		 * "turn up" in other files. (VLS)
		 */
		if ((rtov(rp)->v_pages != NULL) && (rtov(rp)->v_type != VCHR) &&
		    (rtov(rp)->v_type != VSOCK)) {
			RLOCK(rp);
			rp->r_flags &= ~RDIRTY;
			pvn_vptrunc(rtov(rp), 0, 0);  /* Toss those pages! */
			RUNLOCK(rp);
		}
	}
	if (rp->r_cred) {
		crfree(rp->r_cred);
		rp->r_cred = NULL;
	}
}

/*
 * Put an rnode on the free list.
 * The rnode has already been removed from the hash table.
 * If there are no pages on the vnode remove inactivate it,
 * otherwise put it back in the hash table so it can be reused
 * and the vnode pages don't go away.
 */
rfree(rp)
	register struct rnode *rp;
{
	((struct mntinfo *)rtov(rp)->v_vfsp->vfs_data)->mi_refct--;
	if (rtov(rp)->v_pages == NULL) {
		rinactive(rp);
		add_free(rp, FRONT);
	} else {
		rp_addhash(rp);
		add_free(rp, BACK);
	}
}

/*
 * Lookup a rnode by fhandle.
 */
static struct rnode *
rfind(fh, vfsp)
	fhandle_t *fh;
	struct vfs *vfsp;
{
	register struct rnode *rt;

	rt = rtable[rtablehash(fh)];
	while (rt != NULL) {
		if (bcmp((caddr_t)rtofh(rt), (caddr_t)fh, sizeof (*fh)) == 0 &&
		    vfsp == rtov(rt)->v_vfsp) {
			if (++rtov(rt)->v_count == 1) {
				/*
				 * reactivating a free rnode, up vfs ref count
				 * and remove rnode from free list.
				 */
				rm_free(rt);
				((struct mntinfo *)
				    (rtov(rt)->v_vfsp->vfs_data))->mi_refct++;
				rreactive++;
				if (rtov(rt)->v_pages) {
					rnpages++;
				}
			} else {
				ractive++;
			}
			rm_free(rt);
			return (rt);
		}
		rt = rt->r_hash;
	}
	return (NULL);
}

/*
 * Invalidate all vnodes for this vfs.
 * NOTE:  assumes vnodes have been written already via rflush().
 * Called by nfs_unmount in an attempt to get the "mount info count"
 * back to zero.  This routine will be filled in if we ever have
 * an LRU rnode cache.
 */
/*ARGSUSED*/
rinval(vfsp)
	struct vfs *vfsp;
{
	register struct rnode *rp;

	if (rpfreelist == NULL) {
		return;
	}
	rp = rpfreelist;
	do {
		if (rtov(rp)->v_vfsp == vfsp && rtov(rp)->v_count == 0) {
			rp_rmhash(rp);
			rinactive(rp);
		}
		rp = rp->r_freef;
	} while (rp != rpfreelist);
}

/*
 * Flush all vnodes in this (or every) vfs.
 * Used by nfs_sync and by nfs_unmount.
 */
rflush(vfsp)
	struct vfs *vfsp;
{
	register struct rnode **rpp, *rp;
	register struct vnode *vp;

	for (rpp = rtable; rpp < &rtable[RTABLESIZE]; rpp++) {
		for (rp = *rpp; rp != (struct rnode *)NULL; rp = rp->r_hash) {
			vp = rtov(rp);
			/*
			 * Don't bother sync'ing a vp if it
			 * is part of virtual swap device or
			 * if VFS is read-only
			 */
			if (IS_SWAPVP(vp) ||
			    (vp->v_vfsp->vfs_flag & VFS_RDONLY) != 0)
				continue;
			if (vfsp == (struct vfs *)NULL || vp->v_vfsp == vfsp) {
				(void) VOP_PUTPAGE(vp, 0, 0, B_ASYNC,
				    rp->r_cred);
			}
		}
	}
}

char *
newname()
{
	char *news;
	register char *s;
	register u_int id;
	static u_int newnum = 0;
	extern char *strcpy();

	if (newnum == 0) {
		newnum = time.tv_sec & 0xffff;
	}

	news = (char *)new_kmem_alloc((u_int)NFS_MAXNAMLEN, KMEM_SLEEP);
	s = strcpy(news, ".nfs");
	id = newnum++;
	while (id != 0) {
		*s++ = "0123456789ABCDEF"[id & 0x0f];
		id >>= 4;
	}
	*s = '\0';
	return (news);
}
