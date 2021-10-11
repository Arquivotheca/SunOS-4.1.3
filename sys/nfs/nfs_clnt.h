/*	  @(#)nfs_clnt.h 1.1 92/07/30 SMI	*/

#ifndef _nfs_nfs_clnt_h
#define	_nfs_nfs_clnt_h

/*
 * vfs pointer to mount info
 */
#define	vftomi(vfsp)	((struct mntinfo *)((vfsp)->vfs_data))

/*
 * vnode pointer to mount info
 */
#define	vtomi(vp)	((struct mntinfo *)(((vp)->v_vfsp)->vfs_data))

/*
 * NFS vnode to server's block size
 */
#define	vtoblksz(vp)	(vtomi(vp)->mi_bsize)


#define	HOSTNAMESZ	32
#define	ACREGMIN	3	/* min secs to hold cached file attr */
#define	ACREGMAX	60	/* max secs to hold cached file attr */
#define	ACDIRMIN	30	/* min secs to hold cached dir attr */
#define	ACDIRMAX	60	/* max secs to hold cached dir attr */
#define	ACMINMAX	3600	/* 1 hr is longest min timeout */
#define	ACMAXMAX	36000	/* 10 hr is longest max timeout */

#define	NFS_CALLTYPES	3	/* Lookups, Reads, Writes */

/*
 * Fake errno passed back from rfscall to indicate transfer size adjustment
 */
#define	ENFS_TRYAGAIN	999

/*
 * NFS private data per mounted file system
 */
struct mntinfo {
	struct sockaddr_in mi_addr;	/* server's address */
	struct vnode	*mi_rootvp;	/* root vnode */
	u_int		 mi_hard : 1;	/* hard or soft mount */
	u_int		 mi_printed : 1;/* not responding console msg printed */
	u_int		 mi_int : 1;	/* interrupts allowed on hard mount */
	u_int		 mi_down : 1;	/* server is down */
	u_int		 mi_noac : 1;	/* don't cache attributes */
	u_int		 mi_nocto : 1;	/* no close-to-open consistency */
	u_int		 mi_dynamic : 1; /* dynamic transfer size adjustment */
	u_int		 mi_usertold : 1;/* not responding user msg printed */
	int		 mi_refct;	/* active vnodes for this vfs */
	long		 mi_tsize;	/* transfer size (bytes) */
					/* really read size */
	long		 mi_stsize;	/* server's max transfer size (bytes) */
					/* really write size */
	long		 mi_bsize;	/* server's disk block size */
	int		 mi_mntno;	/* kludge to set client rdev for stat*/
	int		 mi_timeo;	/* inital timeout in 10th sec */
	int		 mi_retrans;	/* times to retry request */
	char		 mi_hostname[HOSTNAMESZ];	/* server's hostname */
	char		*mi_netname;	/* server's netname */
	int		 mi_netnamelen;	/* length of netname */
	int		 mi_authflavor;	/* authentication type */
	u_int		 mi_acregmin;	/* min secs to hold cached file attr */
	u_int		 mi_acregmax;	/* max secs to hold cached file attr */
	u_int		 mi_acdirmin;	/* min secs to hold cached dir attr */
	u_int		 mi_acdirmax;	/* max secs to hold cached dir attr */
	/*
	 * Extra fields for congestion control, one per NFS call type,
	 * plus one global one.
	 */
	struct rpc_timers mi_timers[NFS_CALLTYPES+1];
	long		mi_curread;	/* current read size */
	long		mi_curwrite;	/* current write size */
	long		mi_lastmod;	/* last time rw size modified (hz) */
	struct pathcnf *mi_pathconf;	/* static pathconf kludge */
};

/*
 * Mark cached attributes as timed out
 */
#define	PURGE_ATTRCACHE(vp)	(vtor(vp)->r_attrtime = time)

/*
 * Mark cached attributes as uninitialized (must purge all caches first)
 */
#define	INVAL_ATTRCACHE(vp)	(vtor(vp)->r_attrtime.tv_sec = 0)

/*
 * If returned error is ESTALE flush all caches.
 */
#define	PURGE_STALE_FH(errno, vp) \
	if ((errno) == ESTALE) { pvn_vptrunc(vp, 0, 0); nfs_purge_caches(vp); }

/*
 * Is cache valid?
 * Swap is always valid, if no attributes (attrtime == 0) or
 * if mtime matches cached mtime it is valid
 * If server thinks that filesize has changed then caches aren't valid.
 */
#define	CACHE_VALID(rp, mtime, fsize) \
	((rtov(rp)->v_flag & VISSWAP) == VISSWAP || \
	    (rp)->r_attrtime.tv_sec == 0 || \
	    (((mtime).tv_sec == (rp)->r_attr.va_mtime.tv_sec && \
	    (mtime).tv_usec == (rp)->r_attr.va_mtime.tv_usec) && \
	    ((fsize) == (rp)->r_attr.va_size)))

/*
 * Fix for bug 1038327 - corbin
 * Stole this macro from rpc/clnt_kudp.c to calculate current time in hz
 */
#define	time_in_hz	(time.tv_sec*hz + time.tv_usec/(1000000/hz))

#endif /*!_nfs_nfs_clnt_h*/
