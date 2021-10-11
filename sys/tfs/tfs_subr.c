/*	@(#)tfs_subr.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/session.h>
#include <sys/kernel.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/pathname.h>
#include <sys/kmem_alloc.h>
#include <netinet/in.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <sun/consdev.h>
#include <nfs/nfs_clnt.h>
#include <tfs/tfs.h>
#include <tfs/tnode.h>

#ifdef TFSDEBUG
extern int	tfsdebug;
static int	ntranslate;
#endif

extern struct vnodeops tfs_vnodeops;
struct tnode	*tfreelist;
int tnew;
int tnode_sleeps;

static int	set_ctime();
static void	tsave();
struct tnode	*tfind();
struct tfsdiropres *tfsdiropres_get();

/*
 * Perform the virtual -> physical translation for vnode 'vp', leaving
 * 'vp' with a real vnode.
 */
int
tfs_translate(tp, writeable, cred)
	struct tnode *tp;
	bool_t writeable;
	struct ucred *cred;
{
	struct tfstransargs args;
	struct tfsdiropres *dr;
	int error;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_translate(%d)\n", tp->t_nodeid);
	ntranslate++;
#endif
	TLOCK(tp);
	if (tp->t_realvp && tp->t_writeable == writeable) {
		/*
		 * Another process just did the translation for us.
		 */
		TUNLOCK(tp);
		return (0);
	}
	dr = tfsdiropres_get();
	args.tr_fh = tp->t_fh;
	args.tr_writeable = writeable;
	error = tfscall(vftomi(TTOV(tp)->v_vfsp), TFS_TRANSLATE,
			xdr_tfstransargs, (caddr_t)&args,
			xdr_tfsdiropres, (caddr_t)dr, cred);
	if (!error) {
		error = geterrno(dr->dr_status);
	}
	if (!error) {
		error = init_tnode(tp, dr, cred);
	}
	TUNLOCK(tp);
#ifdef TFSDEBUG
	if (error) {
		tfs_dprint(tfsdebug, 5, "tfs_translate returning %d\n", error);
	} else {
		tfs_dprint(tfsdebug, 5,
				"tfs_translate returning:  path '%s'\n",
				dr->dr_path);
	}
#endif
	tfsdiropres_put(dr);
	return (error);
}

/*
 * Given a TFS fhandle, build a TFS vnode for it.
 */
int
tfs_rootvp(fh, vfsp, vpp, cred)
	fhandle_t *fh;
	struct vfs *vfsp;
	struct vnode **vpp;
	struct ucred *cred;
{
	struct tfstransargs args;
	struct tfsdiropres *dr;
	int error;

	dr = tfsdiropres_get();
	args.tr_fh = *fh;
	args.tr_writeable = 0;
	error = tfscall(vftomi(vfsp), TFS_TRANSLATE,
			xdr_tfstransargs, (caddr_t)&args,
			xdr_tfsdiropres, (caddr_t)dr, cred);
	if (!error) {
		error = geterrno(dr->dr_status);
	}
	if (!error) {
		error = maketfsnode(dr, vfsp, vpp, cred);
	}
	tfsdiropres_put(dr);
	return (error);
}

/*
 * Return a vnode for the given fhandle, real pathname.  If no tnode exists
 * for this fhandle create one and put it in a table hashed by fhandle.  If
 * the tnode for this fhandle is already in the table return it.  The tnode
 * will be flushed from the table when tfs_inactive calls tunsave.
 */
int
maketfsnode(dr, vfsp, vpp, cred)
	struct tfsdiropres *dr;
	struct vfs *vfsp;
	struct vnode **vpp;
	struct ucred *cred;
{
	register struct tnode *tp;
	int error;

	tp = tfind(dr->dr_nodeid, vfsp);
	if (tp == NULL) {
		struct vnode *vp;

		if (tfreelist) {
			tp = tfreelist;
			tfreelist = tp->t_next;
		} else {
			tp = (struct tnode *)
				new_kmem_alloc((u_int)sizeof (*tp), KMEM_SLEEP);
			tnew++;
		}
		bzero((caddr_t)tp, sizeof (*tp));
		vp = TTOV(tp);
		vp->v_count = 1;
		vp->v_op = &tfs_vnodeops;
		vp->v_data = (caddr_t)tp;
		vp->v_vfsp = vfsp;
		tp->t_fh = dr->dr_fh;
		tp->t_nodeid = dr->dr_nodeid;
		tsave(tp);
		vftomi(vfsp)->mi_refct++;
	}
	error = init_tnode(tp, dr, cred);
	if (error) {
		VN_RELE(TTOV(tp));
		*vpp = NULL;
#ifdef TFSDEBUG
		tfs_dprint(tfsdebug, 3, "maketfsnode returning %d\n", error);
#endif
		return (error);
	} else {
		*vpp = TTOV(tp);
#ifdef TFSDEBUG
		tfs_dprint(tfsdebug, 3,
			"maketfsnode returning:  vp %x  realvp %x  count %d\n",
			*vpp, tp->t_realvp, (*vpp)->v_count);
#endif
		return (0);
	}
}

int
init_tnode(tp, dr, cred)
	struct tnode *tp;
	struct tfsdiropres *dr;
	struct ucred *cred;
{
	int error;

	bcopy((caddr_t)&dr->dr_sattrs, (caddr_t)&tp->t_sattrs,
		sizeof (dr->dr_sattrs));
	if (tp->t_realvp == NULL || tp->t_writeable != dr->dr_writeable) {
		struct vnode *root_vp;

		if (tp->t_realvp) {
			struct vnode *realvp = tp->t_realvp;

			/*
			 * VN_RELE may sleep, so be paranoid
			 */
			tp->t_realvp = NULL;
			VN_RELE(realvp);
		}
		/*
		 * Un-chroot to perform the virtual -> physical vnode
		 * translation.  This is necessary since the pathname the
		 * tfsd gave us is relative to the real root directory.
		 */
		root_vp = u.u_rdir;
		u.u_rdir = NULL;
		error = lookupname(dr->dr_path, UIOSEG_KERNEL, NO_FOLLOW,
					(struct vnode **)0, &tp->t_realvp);
		u.u_rdir = root_vp;
		if (error) {
			return (error);
		}
		tp->t_writeable = dr->dr_writeable;
		TTOV(tp)->v_type = tp->t_realvp->v_type;
		TTOV(tp)->v_rdev = tp->t_realvp->v_rdev;
	}
	if (TTOV(tp)->v_type == VDIR || !tp->t_writeable) {
		error = set_ctime(tp, cred);
		if (error) {
			return (error);
		}
	} else {
		tp->t_ctime.tv_sec = 0;
	}
	return (0);
}

/*
 * This routine exists to save kernel stack space.
 */
static int
set_ctime(tp, cred)
	struct tnode *tp;
	struct ucred *cred;
{
	struct vattr va;
	int error;

	error = VOP_GETATTR(tp->t_realvp, &va, cred);
	if (error) {
		return (error);
	}
	/*
	 * If the directory has changed, then purge all refs to it from
	 * the dnlc.
	 */
	if (tp->t_ctime.tv_sec &&
	    (tp->t_ctime.tv_sec != va.va_ctime.tv_sec ||
	    tp->t_ctime.tv_usec != va.va_ctime.tv_usec)) {
		dnlc_purge_vp(TTOV(tp));
	}
	tp->t_ctime = va.va_ctime;
	return (0);
}

tfs_tfree(tp)
	register struct tnode *tp;
{
	if (tp->t_realvp) {
		/*
		 * If the real vnode isn't in the frontmost layer, and the
		 * tfsd is on a remote machine, then we can't be sure
		 * that the real vnode will be purged from the dnlc, so
		 * do it here.  If the real file was removed, we don't
		 * want to find it under its old name.
		 */
		if (tp->t_realvp->v_count > 1) {
			dnlc_purge_vp(tp->t_realvp);
		}
		VN_RELE(tp->t_realvp);
	}
	tp->t_next = tfreelist;
	tfreelist = tp;
}

/*
 * Routines to maintain a hash table of tnodes hashed by nodeid.
 */
#define	TTABLESIZE	16
#define	TTABLEHASH(nodeid)	(nodeid & (TTABLESIZE - 1))

static struct tnode *ttable[TTABLESIZE];

static void
tsave(tp)
	struct tnode *tp;
{
	long nodeid;

	nodeid = tp->t_nodeid;
	tp->t_next = ttable[TTABLEHASH(nodeid)];
	ttable[TTABLEHASH(nodeid)] = tp;
}

tfs_tunsave(tp)
	struct tnode *tp;
{
	struct tnode *tt;
	struct tnode *ttprev = NULL;
	long nodeid;

	nodeid = tp->t_nodeid;
	tt = ttable[TTABLEHASH(nodeid)];
	while (tt) {
		if (tt == tp) {
			if (ttprev == NULL) {
				ttable[TTABLEHASH(nodeid)] = tt->t_next;
			} else {
				ttprev->t_next = tt->t_next;
			}
			return;
		}
		ttprev = tt;
		tt = tt->t_next;
	}
}

struct tnode *
tfind(nodeid, vfsp)
	u_long nodeid;
	struct vfs *vfsp;
{
	register struct tnode *tp;

	tp = ttable[TTABLEHASH(nodeid)];
	while (tp) {
		if (tp->t_nodeid == nodeid &&
		    vfsp == TTOV(tp)->v_vfsp) {
			VN_HOLD(TTOV(tp));
			return (tp);
		}
		tp = tp->t_next;
	}
	return (NULL);
}

/*
 * Routines to maintain a free list of tfsdiropres structs, to avoid
 * excessive calls to kmem_alloc/free.  We also keep buffers around for
 * the pathnames of the real files.  The pathname buffer is enlarged
 * in xdr_tfsdiropres whenever the incoming pathname is longer than the
 * length of the buffer.
 */
#define	INIT_PATHLEN	32

struct tfsspace {
	union {
		struct tfsdiropres tsu_dr;
		struct tfsspace	*tsu_next;
	} ts_un;
	int		ts_pathlen;
};
#define	ts_next	ts_un.tsu_next

static struct tfsspace *tfsfreesp;

struct tfsdiropres *
tfsdiropres_get()
{
	struct tfsdiropres *res;

	if (!tfsfreesp) {
		tfsfreesp = (struct tfsspace *)new_kmem_alloc(
				(u_int)sizeof (*tfsfreesp), KMEM_SLEEP);
		tfsfreesp->ts_un.tsu_dr.dr_path = (caddr_t)new_kmem_alloc(
				(u_int)INIT_PATHLEN + 1, KMEM_SLEEP);
		tfsfreesp->ts_pathlen = INIT_PATHLEN;
		tfsfreesp->ts_next = NULL;
	}
	res = (struct tfsdiropres *)tfsfreesp;
	res->dr_pathlen = tfsfreesp->ts_pathlen;
	tfsfreesp = tfsfreesp->ts_next;
	return (res);
}

tfsdiropres_put(dr)
	struct tfsdiropres *dr;
{
	struct tfsspace	*ts = (struct tfsspace *)dr;

	if (dr->dr_pathlen > ts->ts_pathlen) {
		ts->ts_pathlen = dr->dr_pathlen;
	}
	ts->ts_next = tfsfreesp;
	tfsfreesp = ts;
}

#ifdef TFSDEBUG
/*VARARGS2*/
int
tfs_dprint(var, level, str, a1, a2, a3, a4, a5, a6, a7, a8, a9)
	int var;
	int level;
	char *str;
	int a1, a2, a3, a4, a5, a6, a7, a8, a9;
{
	if (var == level || (var > 10 && (var - 10) >= level)) {
		printf(str, a1, a2, a3, a4, a5, a6, a7, a8, a9);
	}
}
#endif

/*
 * Client side utilities.  Copied this from the NFS client side code.
 * Too bad we couldn't have just used the NFS's rfscall() procedure, but
 * it was too NFS-specific.
 */

/*
 * client side statistics
 */
struct {
	int	nclgets;		/* client handle gets */
	int	ncalls;			/* client requests */
	int	nbadcalls;		/* rpc failures */
	int	reqs[TFS_NPROC];	/* count of each request */
} tfs_clstat;


#define	MAXCLIENTS	2
struct chtab {
	int	ch_timesused;
	bool_t	ch_inuse;
	CLIENT	*ch_client;
} tfs_chtable[MAXCLIENTS];

static int	tfs_cltoomany = 0;

extern AUTH	*authget();

static CLIENT *
tfs_clget(mi, cred)
	struct mntinfo *mi;
	struct ucred *cred;
{
	register struct chtab *ch;
	int retrans;
	CLIENT *client;

	/*
	 * If soft mount and server is down just try once
	 */
	if (!mi->mi_hard && mi->mi_down) {
		retrans = 1;
	} else {
		retrans = mi->mi_retrans;
	}

	/*
	 * Find an unused handle or create one
	 */
	tfs_clstat.nclgets++;
	for (ch = tfs_chtable; ch < &tfs_chtable[MAXCLIENTS]; ch++) {
		if (!ch->ch_inuse) {
			ch->ch_inuse = TRUE;
			if (ch->ch_client == NULL) {
				ch->ch_client =
				    clntkudp_create(&mi->mi_addr,
				    TFS_PROGRAM, TFS_VERSION,
				    retrans, cred);
				if (ch->ch_client == NULL) {
					panic("tfs_clget: null client");
				}
				auth_destroy(ch->ch_client->cl_auth);
			} else {
				clntkudp_init(ch->ch_client,
				    &mi->mi_addr, retrans, cred);
			}
			ch->ch_client->cl_auth = authget(mi, cred);
			if (ch->ch_client->cl_auth == NULL) {
				panic("tfs_clget: null auth");
			}
			ch->ch_timesused++;
			return (ch->ch_client);
		}
	}

	/*
	 * If we got here there are no available handles
	 * To avoid deadlock, don't wait, but just grab another
	 */
	tfs_cltoomany++;
	client = clntkudp_create(&mi->mi_addr, TFS_PROGRAM, TFS_VERSION,
				retrans, cred);
	if (client == NULL)
		panic("tfs_clget: null client");
	auth_destroy(client->cl_auth);
	client->cl_auth = authget(mi, cred);
	if (client->cl_auth == NULL)
		panic("tfs_clget: null auth");
	return (client);
}

static void
tfs_clfree(cl)
	CLIENT *cl;
{
	register struct chtab *ch;

	authfree(cl->cl_auth);
	cl->cl_auth = NULL;
	for (ch = tfs_chtable; ch < &tfs_chtable[MAXCLIENTS]; ch++) {
		if (ch->ch_client == cl) {
			ch->ch_inuse = FALSE;
			return;
		}
	}
	/* destroy any extra allocated above MAXCLIENTS */
	CLNT_DESTROY(cl);
}

static char *tfsnames[] = {
	"null", "setattr", "lookup", "create", "remove", "rename", "link",
	"symlink", "mkdir", "rmdir", "readdir", "statfs", "translate" };

/*
 * This table maps from TFS protocol number into call type.
 * Zero means a "Lookup/Read" type call
 * One  means a "Write" type call
 * Two  means an "Expensive write" type call
 * This is used to select a minimum timeout value.
 */
static char call_type[] = {
	0, 1, 0, 1, 1, 2, 1,
	1, 2, 2, 0, 0, 0 };

/*
 * Minimum time-out values indexed by call type.  The timeouts are in
 * seconds.
 */
static unsigned int minimum_timeo[] = {
	5, 10, 15 };

/*
 * Back off for retransmission timeout, MAXTIMO is in hz of a sec
 */
#define	MAXTIMO	30*hz
#define	backoff(tim)	((((tim) << 1) > MAXTIMO) ? MAXTIMO : ((tim) << 1))

/*
 * minimum "chunk" of TFS readdir I/O
 */
#define	MIN_TFS_TSIZE	512
/*
 * reduce read size if rtxcur > 5 sec
 */
#define	REDUCE_TFS_TIME	(5*hz)
/*
 * increase read size if srtt < 1 sec  (srtt == 8 * avg round-trip time)
 */
#define	INCREASE_TFS_TIME	(8*hz)

/*
 * Function called when the RPC package notices that we are
 * re-transmitting, or when we get a response without retransmissions.
 */
static void
tfs_feedback(flag, which, mi)
	int flag;
	int which;
	register struct mntinfo *mi;
{
	if (flag == FEEDBACK_OK) {
		if (which != TFS_READDIR) {
			return;
		}
		if (mi->mi_curread < mi->mi_tsize &&
		    mi->mi_timers[1].rt_srtt < INCREASE_TFS_TIME) {
			mi->mi_curread += MIN_TFS_TSIZE;
			if (mi->mi_curread > mi->mi_tsize/2) {
				mi->mi_curread = mi->mi_tsize;
			}
		}
	} else if (flag == FEEDBACK_REXMIT1) {
		if (mi->mi_timers[NFS_CALLTYPES].rt_rtxcur != 0 &&
		    mi->mi_timers[NFS_CALLTYPES].rt_rtxcur < REDUCE_TFS_TIME) {
			return;
		}
		if (mi->mi_curread > MIN_TFS_TSIZE) {
			mi->mi_curread /= 2;
			if (mi->mi_curread < MIN_TFS_TSIZE) {
				mi->mi_curread = MIN_TFS_TSIZE;
			}
		}
	}
}

int
tfscall(mi, which, xdrargs, argsp, xdrres, resp, cred)
	register struct mntinfo *mi;
	int which;
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
	int timeo;		/* in units of HZ */
	int user_told;
	bool_t tryagain;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 6, "tfscall: %x, %d, %x, %x, %x, %x\n",
		    mi, which, xdrargs, argsp, xdrres, resp);
#endif
	tfs_clstat.ncalls++;
	tfs_clstat.reqs[which]++;

	rpcerr.re_errno = 0;
	rpcerr.re_status = RPC_SUCCESS;
	newcred = NULL;
	user_told = 0;
retry:
	client = tfs_clget(mi, cred);
	timeo = clntkudp_settimers(client,
			&(mi->mi_timers[which == TFS_READDIR ? 1 : 0]),
			&(mi->mi_timers[NFS_CALLTYPES]),
			minimum_timeo[call_type[which]] * hz,
			mi->mi_dynamic ? tfs_feedback : (void (*)()) 0,
			(caddr_t) mi, 0L);

	/*
	 * If hard mounted fs, retry call forever unless hard error occurs
	 */
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
				if (!mi->mi_printed) {
					mi->mi_printed = 1;
	printf("TFS server %s not responding still trying\n", mi->mi_hostname);
				}
				if (!user_told && u.u_procp->p_sessp->s_ttyp &&
				    u.u_procp->p_sessp->s_ttyd != consdev &&
				    u.u_procp->p_sessp->s_ttyd != rconsdev) {
					user_told = 1;
	uprintf("TFS server %s not responding still trying\n", mi->mi_hostname);
				}
				if (which == TFS_READDIR && mi->mi_dynamic) {
					/*
					 * Return back to the vnode ops
					 * level, so that the readdir retry
					 * can be made with the (possibly)
					 * new transfer size.
					 */
					tfs_clfree(client);
					if (newcred) {
						crfree(newcred);
					}
					return (ENFS_TRYAGAIN);
				}
			}
		}
	} while (tryagain);

	if (status != RPC_SUCCESS) {
		tfs_clstat.nbadcalls++;
		mi->mi_down = 1;
		if (status != RPC_INTR) {
			CLNT_GETERR(client, &rpcerr);
			printf("TFS %s failed for server %s: %s\n",
				tfsnames[which], mi->mi_hostname,
				clnt_sperrno(status));
			if (u.u_procp->p_sessp->s_ttyp &&
			    u.u_procp->p_sessp->s_ttyd != consdev &&
			    u.u_procp->p_sessp->s_ttyd != rconsdev) {
				uprintf("TFS %s failed for server %s: %s\n",
					tfsnames[which], mi->mi_hostname,
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
		tfs_clfree(client);
		goto retry;
	} else if (mi->mi_hard) {
		if (mi->mi_printed) {
			printf("TFS server %s ok\n", mi->mi_hostname);
			mi->mi_printed = 0;
		}
		if (user_told) {
			uprintf("TFS server %s ok\n", mi->mi_hostname);
		}
	} else {
		mi->mi_down = 0;
	}

	tfs_clfree(client);
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 7, "tfscall: returning %d\n", rpcerr.re_errno);
#endif
	if (newcred) {
		crfree(newcred);
	}
	return (rpcerr.re_errno);
}
