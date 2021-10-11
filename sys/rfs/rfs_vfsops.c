/*	@(#)rfs_vfsops.c 1.1 92/07/30 SMI 	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <rpc/rpc.h>
#include <sys/stream.h>
#include <sys/user.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <nfs/nfs.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/debug.h>
#include <sys/uio.h>
#include <rfs/sema.h>
#include <rfs/rfs_misc.h>
#include <rfs/comm.h>
#include <rfs/nserve.h>
#include <rfs/cirmgr.h>
#include <rfs/message.h>
#include <rfs/rfs_node.h>
#include <rfs/rfs_mnt.h>
#include <rfs/adv.h>
#include <rfs/rdebug.h>
#include <rfs/rfs_xdr.h>

extern  struct timeval	time;
extern	int	bootstate;
extern	int	nadvertise;
extern	int	nsrmount;
extern	struct	advertise *getadv(), *findadv();
extern	rcvd_t	cr_rcvd();
extern char	*nameptr();

extern struct vnodeops rfs_vnodeops;

/*
 * rfs vfs operations.
 */
static int rf_mount();
static int rf_unmount();
static int rf_root();
static int rf_statfs();
static int rf_sync();
static int rf_noop();

struct vfsops rfs_vfsops = {
	rf_mount,
	rf_unmount,
	rf_root,
	rf_statfs,
	rf_sync,
	rf_noop,
	rf_noop,
	rf_noop,
};

/*
 *	remote mount
 */
/*ARGSUSED*/
static int 
rf_mount(vfsp, mntpt, data) 
struct vfs *vfsp;
char *mntpt;
caddr_t data;
{
	register struct	rfsmnt *rmp = NULL, *srmp;	
	char	name[MAXDNAME+1];
	struct token	token;
	sndd_t	sdp = NULL;
	sndd_t	gift = NULL;
	struct	advertise *ap;
	struct	queue	*qp = NULL;
	queue_t	*get_circuit();
	struct rfs_args args;
	struct vnode *rootvp;
	struct rfs_statfs rsb;
	struct nodeinfo ni;
	int error = 0;
	extern char *strcpy();
	extern char Domain[];
	char *rsrc;
	int flags = 0; 	/* XXX -- should be part of private data */

	DUPRINT2(DB_MNT_ADV,"rmount: entering, data addr %x\n", data);

	if (bootstate != DU_UP)   /*  have to be on network  */
		return (ENONET);

	if (!suser())
		return (EPERM);

	/* bring the arguments into kernel space */
	if (error = copyin(data, (caddr_t)&args, sizeof (args))) {
		DUPRINT1(DB_MNT_ADV,"rmount: data copyin failed\n");
		return (error);
	}

	/* bring the token into kernel space	*/
	if (error = copyin((caddr_t)args.token, (caddr_t)&token, 
						sizeof(struct token))) {
		DUPRINT1(DB_MNT_ADV,"rmount: copyin failed...\n");
		return (error);
	}
	DUPRINT3(DB_MNT_ADV,"rmount: token.t_id=%x, t_uname=%s\n",
		token.t_id, token.t_uname);

	if (token.t_id & MS_CACHE) {
		token.t_id &= ~MS_CACHE;
/*
		if (rcache_enable)
			flags |= MS_CACHE;
*/
	}
	flags |= (vfsp->vfs_flag & VFS_RDONLY ? RFS_RDONLY :0);

	/*  bring the advertised name into kernel space  */
	switch (upath(args.rmtfs,name,MAXDNAME+1)) {
		case -2:	/* too long	*/
		case  0:	/* too short	*/
			return (EINVAL);
		case -1:	/* bad user address	*/
			return (EFAULT);
	}

	/* if DB_LOOPBCK has been set with rdebug, then allow mounts
	   of locally advertised resources.  Otherwise, they fail. */
	if (!(dudebug & DB_LOOPBCK)) {
		/*  make sure that resource isn't advertised locally.
		 to do this, you separate the resource and the domain
		 name (if there is one).  If the domain is the current
		 domain, check to see if the resource is in the local
		 advertise table.  THIS ALGORITHM WILL CHANGE WHEN MULTI-
		 LEVEL DOMAIN STRUCTURES ARE INTRODUCED TO RFS */
		for (rsrc = name; *rsrc && *rsrc != '.'; rsrc++)
				;
		if (*rsrc == '.') {
			*rsrc = '\0';
			rsrc++;		/* move past the . (now a null) */
			if (strcmp(name, Domain) == 0) 
				if ((ap = findadv(rsrc)) != NULL) {
					if (!(ap->a_flags & A_MINTER))
						return(EINVAL);
				}
			*--rsrc = '.';	/* replace the . in the name */
		} else {
			if ((ap = findadv(name)) != NULL) {
				if (!(ap->a_flags & A_MINTER))
					return (EINVAL);
			}
		}
	}

	/* Get the GDP circuit associated with this token */
	if ((qp = get_circuit(-1, &token)) == NULL) {
		DUPRINT3(DB_MNT_ADV,"rmount fails: token.t_id=%x, t_uname=%s\n",
			token.t_id, token.t_uname);
		/*
		 * WARNING - this is ONLY place rmount() can return this error!
		 */
		return (ENOLINK);
	}

	/*  allocate a remote mount table entry, and initialize it.  
	    also check for resource already mounted. */
	for (srmp = NULL, rmp = rfsmount; rmp < &rfsmount[nrfsmount]; rmp++) {
		if (rmp->rm_flags == MFREE)
				srmp = rmp;
		else if (rmp->rm_flags && 
				!strncmp(name, rmp->rm_name, MAXDNAME+1)) {
			error = EBUSY;
			rmp = NULL;	/* don't free mount table entry */
			goto failed;
		}
	}
	if (!(rmp = srmp)) {
		error = EBUSY;
		goto failed;
	}
       	bzero((caddr_t)rmp, sizeof(*rmp));
	rmp->rm_flags = MINTER;
	rmp->rm_vfsp = vfsp;

	/* Get and initialize a send descriptor for the remote system call */
	if ((sdp = cr_sndd()) == NULL)
		return (ENOMEM);
	set_sndd(sdp, qp, CFRD, 0);

	/* Make the RFS remote mount system call */
	if (error = du_mount(nameptr(name), sdp, u.u_cred, 
				qp, rmp, flags, &gift, &ni))
		goto failed;

	/* Do a statfs system call to get additional mount table info */
	if (error = du_statfs("", gift, &rsb))
		goto failed;

	/* Fill in the mount table entry */
        rmp->rm_fstyp = rsb.f_fstyp;
        rmp->rm_bsize = rsb.f_bsize;
        rmp->rm_frsize = rsb.f_frsize;
        rmp->rm_blocks = rsb.f_blocks;
        rmp->rm_files = rsb.f_files;
	bcopy((caddr_t) rsb.f_fname, (caddr_t) rmp->rm_fname, 6);
	bcopy((caddr_t) rsb.f_fpack, (caddr_t) rmp->rm_fpack, 6);
	rmp->rm_flags = MDOTDOT | MINUSE;
	if (flags & RFS_RDONLY)
		rmp->rm_flags |= MRDONLY;
	(void) strcpy (rmp->rm_name, name);
	vfsp->vfs_data = (caddr_t) rmp;

	/* get an rfs node/vnode for the root: assign it a parent sd of
	   itself and a name of "". */
	if (error = get_rfsnode(vfsp, gift, "", gift, &ni, &rootvp))
		goto failed;
        rootvp->v_flag |= VROOT;
	rmp->rm_rootvp = rootvp;
        rmp->rm_mntno = vfs_getmajor(vfsp);  /* Assign major dev # to mount */

	DUPRINT4(DB_SYSCALL, "rmount ok: blksz %d, blocks %d, files %d\n",
		rmp->rm_bsize, rmp->rm_blocks, rmp->rm_files);
	/* bump the mount count */
	GDP(qp)->mntcnt++;
	/* Free sd for mount call -- "gift" contains new sd for root */
	free_sndd(sdp);

	/* If multi-component lookup is requested don't allow mounts
	   beneath this filesystem, to avoid pathname evaluation ambiguities */
	if (vfsp->vfs_flag & VFS_MULTI)
		vfsp->vfs_flag |= VFS_NOSUB;

	return (0);

failed:
	if (qp)
		put_circuit (qp);
	if (sdp)
		free_sndd (sdp);
	if (gift) {
		(void) du_unmount(gift, u.u_cred);
		free_sndd(gift);
	}
	if (rmp)
		rmp->rm_flags = MFREE;
	vfsp->vfs_data = (caddr_t) NULL;
	DUPRINT2(DB_MNT_ADV, "rmount failed: error is %d\n", error);
	return (error);
}



/*
 *	rf_unmount - unmount a remote file system.
 */
static int
rf_unmount(vfsp)
register struct vfs *vfsp;
{
	register struct	rfsmnt	*rmp;
	struct rfsnode *rfp;
	sndd_t sdp;
	int error = 0;

	DUPRINT3(DB_MNT_ADV,"rf_unmount, vfsp %x, resource %s\n", vfsp,
	((struct rfsmnt *)vfsp->vfs_data)->rm_name);

	if (!suser())
		return (EPERM);

	if (!(bootstate & DU_UP))  /*  have to be on network  */
		return (ENONET);

	/* Get the private mount struct and check that it looks unmountable */
	rmp = (struct rfsmnt *) vfsp->vfs_data;
	rmp->rm_flags &= ~MINUSE;
	rmp->rm_flags |= MINTER;
	rfp = (struct rfsnode *) rmp->rm_rootvp->v_data;
	sdp = rfp->rfs_sdp;

	/* Don't unmount if somebody's using the file system */
        if (rmp->rm_refcnt != 1 || rmp->rm_rootvp->v_count != 1) {
		DUPRINT3(DB_MNT_ADV,"rumount fail: rm_refcnt %d, rootvp  %d\n", 
			rmp->rm_refcnt, rmp->rm_rootvp->v_count);
		error = EBUSY;
		goto fail;
	}

	/* Make the remote unmount system call */
	error = du_unmount(rfp->rfs_sdp, u.u_cred);

	if (error == ENOLINK || error == ECOMM)  {
		DUPRINT1(DB_MNT_ADV,"rumount succeeds because link is gone\n");
		error = 0;
	}
	if (error)
		goto fail;

	/*  Success - srmnt entry was removed from remote  */
	/*  (or link is down, so let unmount succeed anyway) */
	/* invalidate cache for this mount device */
	if (rmp->rm_flags & MCACHE)
		/* rmntinval(sd); */ ;
	GDP(sdp->sd_queue)->mntcnt--;
	put_circuit (sdp->sd_queue);

	/* Release rfs_node and sd for root -- remote unmount has already
	released remote inode so hold the sd to prevent a DUIPUT, and manually
	free sd */
	rfslock(rfp);
	sdp->sd_refcnt++;
	VN_RELE(rmp->rm_rootvp);
	free_sndd(sdp);		

	rmp->rm_flags = MFREE;
	vfs_putmajor(vfsp, rmp->rm_mntno);  /* Release major dev # of mount */
	DUPRINT1(DB_MNT_ADV,"rf_unmount succeeded\n");
	return (0);

fail:	rmp->rm_flags &= ~MINTER;
	rmp->rm_flags |=  MINUSE;
	DUPRINT2(DB_MNT_ADV,"rf_unmount failed, error %d\n", error);
	return (error);
}


static int
rf_root(vfsp, vpp)
        struct vfs *vfsp;
        struct vnode **vpp;
{
	/* Error if vfs not initialized or locked */
	if (!vfsp->vfs_data || (vfsp->vfs_flag & VFS_MLOCK))
		return(ENOENT);

        *vpp = (struct vnode *)((struct rfsmnt *)vfsp->vfs_data)->rm_rootvp;
        (*vpp)->v_count++;

	DUPRINT2(DB_MNT_ADV,"rf_root: rootvp count = %d\n",(*vpp)->v_count);

	return(0);
}

static int
rf_statfs(vfsp, sbp)
register struct vfs *vfsp;
struct statfs *sbp;
{
	struct vnode *rootvp;
	struct rfsnode *rfp;
	struct rfs_statfs rsb;
	int error = 0;

	DUPRINT2(DB_MNT_ADV,"rf_statfs: entering vfsp %x\n", vfsp);

	/* Get the root node for the file system */
	if (error = rf_root(vfsp, &rootvp))
		return(error);
	rfp = vtorfs(rootvp);

	/* Do the statfs system call */
	if (error = du_statfs(rfp->rfs_name, rfp->rfs_sdp, &rsb))
		goto out;

	/* Unpack the response */
        sbp->f_type = 0;
        sbp->f_bsize = rsb.f_bsize;
        sbp->f_blocks = rsb.f_blocks;
        sbp->f_bfree = rsb.f_bfree;
        sbp->f_bavail = rsb.f_bfree;
        sbp->f_files = rsb.f_files;
        sbp->f_ffree = rsb.f_ffree;
        sbp->f_fsid.val[0] = ((long *) rsb.f_fname)[0];
        sbp->f_fsid.val[1] = ((long *) rsb.f_fname)[1];

out:
	VN_RELE(rootvp);
        return (error);
}


static int 
rf_sync(vfsp) 
struct vfs *vfsp;
{
	int error = 0;

	DUPRINT2(DB_SYNC,"rf_sync: entering vfsp %x\n", vfsp);
        return (error);
}

static int 
rf_noop()
{
	return(EOPNOTSUPP);
}
