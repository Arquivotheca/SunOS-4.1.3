#ident	"@(#)rfs_serve.c 1.1 92/07/30 SMI"

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)kern-port:nudnix/serve.c	10.43" */
/*
 * 	RFS server.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/file.h>
#include <sys/kmem_alloc.h>
#include <rpc/rpc.h>
#include <sys/stream.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/vfs.h>
#include <nfs/nfs.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/debug.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/dirent.h>
#include <sys/ioctl.h>
#include <sys/pathname.h>
#include <sys/fcntl.h>
#include <rfs/rfs_misc.h>
#include <rfs/sema.h>
#include <rfs/comm.h>
#include <rfs/nserve.h>
#include <rfs/cirmgr.h>
#include <rfs/message.h>
#include <rfs/rdebug.h>
#include <rfs/recover.h>
#include <rfs/rfs_serve.h>
#include <rfs/rfs_mnt.h>
#include <rfs/rfs_xdr.h>
#include <rfs/adv.h>
#include <rfs/hetero.h>
#include <rfs/idtab.h>
#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_vn.h>

int	rs_read();
int	rs_write();
int	rs_open();
int	rs_close();
int	rs_creat();
int	rs_link();
int	rs_unlink();
int	rs_exec();
int	rs_chdir();
int	rs_mknod();
int	rs_chmod();
int	rs_chown();
int	rs_stat();
int	rs_seek();
int	rs_fstat();
int	rs_utime();
int	rs_access();
int	rs_statfs();
int	rs_fstatfs();
int	rs_ioctl();
int	rs_utssys();
int	rs_chroot();
int	rs_fcntl();
int	rs_rmdir();
int	rs_mkdir();
int	rs_getdents();
int	rs_srmount();
int	rs_srumount();
int	rs_link1();
int	rs_coredump();
int 	rs_iput();
int 	rs_iupdate();
int 	rs_uupdate();
int	rs_clientop();
int	rs_rsignal();
int	rs_badop();

/*
 * RFS server ops table.
 * Note:  these must match the numbering of RFS syscalls in message.h.
 */
int (*serv_op[])() = {
	rs_badop, rs_badop, rs_badop,
	rs_read,		/* DUREAD	   3 */
	rs_write,		/* DUWRITE	   4 */
	rs_open,		/* DUOPEN	   5 */
	rs_close,		/* DUCLOSE	   6 */
	rs_badop,
	rs_creat,		/* DUCREAT	   8 */
	rs_link,		/* DULINK	   9 */
	rs_unlink,		/* DUUNLINK	  10 */
	rs_exec,		/* DUEXEC	  11 */
	rs_chdir,		/* DUCHDIR	  12 */
	rs_badop,
	rs_mknod,		/* DUMKNOD	  14 */
	rs_chmod,		/* DUCHMOD	  15 */
	rs_chown,		/* DUCHOWN	  16 */
	rs_badop,
	rs_stat,		/* DUSTAT	  18 */
	rs_seek,		/* DUSEEK	  19 */
	rs_badop,
	rs_clientop,		/* DUMOUNT	  21 */
	rs_clientop,		/* DUUMOUNT	  22 */
	rs_badop, rs_badop, rs_badop, rs_badop, rs_badop,
	rs_fstat,		/* DUFSTAT	  28 */
	rs_badop,
	rs_utime,		/* DUUTIME	  30 */
	rs_badop, rs_badop,
	rs_access,		/* DUACCESS	  33 */
	rs_badop,
	rs_statfs,		/* DUSTATFS	  35 */
	rs_badop, rs_badop,
	rs_fstatfs,		/* DUFSTATFS	  38 */
	rs_badop, rs_badop, rs_badop, rs_badop, rs_badop, rs_badop, rs_badop,
	rs_badop, rs_badop, rs_badop, rs_badop, rs_badop,
	rs_clientop,		/* DUSYSACCT	  51 */
	rs_badop, rs_badop,
	rs_ioctl,		/* DUIOCTL	  54 */
	rs_badop, rs_badop,
	rs_utssys,		/* DUUTSSYS	  57 */
	rs_badop,
	rs_exec,		/* DUEXECE	  59 */
	rs_badop,
	rs_chroot,		/* DUCHROOT	  61 */
	rs_fcntl,		/* DUFCNTL	  62 */
	rs_badop, rs_badop, rs_badop, rs_badop, rs_badop, rs_badop, rs_badop,
	rs_clientop,		/* DUADVERTISE	  70 */
	rs_clientop,		/* DUUNADVERTISE  71 */
	rs_clientop,		/* DURMOUNT	  72 */
	rs_clientop,		/* DURUMOUNT	  73 */
	rs_badop, rs_badop, rs_badop, rs_badop, rs_badop,
	rs_rmdir,		/* DURMDIR	  79 */
	rs_mkdir,		/* DUMKDIR	  80 */
	rs_getdents,		/* DUGETDENTS	  81 */
	rs_badop, rs_badop, rs_badop, rs_badop, rs_badop, rs_badop, rs_badop,
	rs_badop, rs_badop, rs_badop, rs_badop, rs_badop, rs_badop, rs_badop,
	rs_badop,
	rs_srmount,		/* DUSRMOUNT	  97 */
	rs_srumount,		/* DUSRUMOUNT	  98 */
	rs_badop, rs_badop, rs_badop, rs_badop, rs_badop, rs_badop, rs_badop,
	rs_badop,		/* DUCOPYIN	 106 */
	rs_badop,		/* DUCOPYOUT	 107 */
	rs_badop,
	rs_link1,		/* DULINK1	 109 */
	rs_badop,
	rs_coredump,		/* DUCOREDUMP	 111 */
	rs_write,		/* DUWRITEI	 112 */
	rs_read,		/* DUREADI	 113 */
	rs_badop,
	rs_badop,		/* DULBMOUNT	 115 */
	rs_badop, rs_badop, rs_badop,
	rs_rsignal, 		/* DURSIGNAL	119 */
	rs_badop, rs_badop, rs_badop, rs_badop, rs_badop,
	rs_badop, rs_badop, rs_badop, rs_badop, rs_badop,
	rs_badop,
	rs_iput,		/* DUIPUT	131 */
	rs_iupdate,		/* DUIUPDATE	132 */
	rs_uupdate,		/* DUIUPDATE	133 */
	rs_badop,
};


extern	rcvd_t	cr_rcvd();
extern  sndd_t cr_sndd();
extern  struct serv_proc *sp_alloc();
extern	struct rd_user *cr_rduser();
extern	rcvd_t	rd_recover;
void	chkrsig();
extern mblk_t *alocbuf();
extern mblk_t *salocbuf();
extern void setrsig();
extern void make_response();
extern char *strcpy();
extern	struct	vnode *rfs_cdir;

/* For physio() on raw devices */
#define	SEGSTART 	((addr_t) MAXBSIZE)
#define	SEGSIZE 	(MAXBSIZE * 16)
u_int segsize = SEGSIZE;
struct as *rs_getas();

serve(sp)
struct serv_proc *sp;
{
	extern rcvd_t serve_dq();
	rcvd_t	gift;			/* what the requestor wanted	*/
	register struct	request	*msg_in;
	register struct sndd *sdp;	/* reply path back to requestor	*/
	int	insize, outsize;	/* size of out/incoming msg	*/
	int	i;			/* temporary variable		*/
	mblk_t	*bp, *out_bp;
	struct message *mp;
	int error;
	struct proc *p;
	extern struct ucred *crcopy();

	/* If got unexpected signal, try to exit gracefully */
	if (setjmp(&u.u_qsave)) {
		DUPRINT1(DB_SERVE, "server: unexpected signal, exiting\n");
		return;
	}

	ASSERT(sp->s_proc == u.u_procp);
	p = sp->s_proc;

	/* ignore all signals, except SIGKILL, SIGTERM, SIGUSR1 */
	for (i=0; i<NSIG; i++) {
		u.u_signal[i] = SIG_IGN;
		u.u_sigmask[i] =
			~(sigmask(SIGKILL)|sigmask(SIGTERM)|sigmask(SIGUSR1));
	}
	u.u_signal[SIGKILL] = SIG_DFL;
	u.u_signal[SIGTERM] = SIG_DFL;
	u.u_signal[SIGUSR1] = SIG_DFL;
	p->p_sig = 0;
	p->p_cursig = 0;
	p->p_sigmask = ~(sigmask(SIGKILL)|sigmask(SIGTERM)|sigmask(SIGUSR1));
	p->p_sigignore = ~(sigmask(SIGKILL)|sigmask(SIGTERM)|sigmask(SIGUSR1));
	p->p_sigcatch = 0;

	/* Don't share credentials with parent */
	u.u_cred = crcopy(u.u_cred);
	u.u_groups[0] = NOGROUP; /* Don't inherit group list from parent */

	/* Create an sd to send back response. */
	if ((sp->s_sdp = cr_sndd()) == (sndd_t) NULL) {
		if (s_active.sl_head)
			psignal(s_active.sl_head->s_proc, SIGUSR1);
		return;
	}
	sdp = sp->s_sdp;

	while (TRUE) {
		bcopy ("rfs:server", u.u_comm, (u_int)11);
		sdp->sd_queue = NULL;
		sdp->sd_stat = (SDUSED | SDSERVE);
		sdp->sd_srvproc =  (struct proc *) NULL;
		sp->s_epid = 0;
		sp->s_sysid = 0;
		u.u_cdir = rfs_cdir;
		u.u_rdir = (struct vnode *) NULL;

		/* Get request (bp) and rd to work on, quit if none */
		if ((sp->s_rdp = serve_dq(&bp, &insize, sp)) == (rcvd_t) NULL)
			return;

		/* set up server process for syscall */
		bcopy ("rfs:SERVER", u.u_comm, (u_int)11);
		mp = (struct message *) bp->b_rptr;
		msg_in = (struct request *) PTOMSG(bp->b_rptr);
		gift =  (rcvd_t) NULL;
		error = 0;
		sdp->sd_copycnt = 0;
		sp->s_gdpp =  GDP((queue_t *)mp->m_queue);
		sp->s_sysid = sp->s_gdpp->sysid;  /* Use SERVER's circuit id */
		sp->s_epid = msg_in->rq_pid;
		sp->s_op = msg_in->rq_opcode;
		sp->s_mntindx = msg_in->rq_mntindx;
		sp->s_fmode = RFSTOFFLAG(msg_in->rq_fmode);
		u.u_uid = gluid(sp->s_gdpp, (ushort) msg_in->rq_uid);
		u.u_gid = glgid(sp->s_gdpp, (ushort) msg_in->rq_gid);
		u.u_error = 0;
		u.u_rlimit[RLIMIT_FSIZE].rlim_cur =
				msg_in->rq_ulimit << SCTRSHFT;
		out_bp = (mblk_t *) NULL;

		/* fail req. if resource is in funny state - e.g., fumount */
		if (sp->s_op != DUSRMOUNT) {
			if  (srmount[sp->s_mntindx].sr_flags & MINTER) {
				freemsg (bp);
				DUPRINT1(DB_RECOVER, "req. MINTER resource\n");
				continue;
			}
			/* following two sd fields used by recovery/fumount */
			sdp->sd_mntindx = sp->s_mntindx;
			sdp->sd_srvproc = sp->s_proc;
		}
		/* Service the request */
		error = (*(serv_op[sp->s_op]))(sp, bp, &out_bp, &outsize, &gift);

		/* Send response, if there is one */
		if (out_bp) {
			error = sndmsg(sdp, out_bp, outsize, gift);
			if (error) {
				/*
				 * Error: assume link down, decr. server count
				 * on this resource and wakeup recovery if last
				 */
				DUPRINT1 (DB_SERVE, "server sndmsg failed\n");
				if (--srmount[sp->s_mntindx].sr_slpcnt == 0)
					wakeup ((caddr_t) &srmount[sp->s_mntindx]);
			}
		}
	}
}

/*
 * Server ops -- one for each RFS remote system call.
 */
rs_open(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	struct	response *msg_out;
	register char *name;
	struct vnode *vp = (struct vnode *) NULL;
	int error = 0;
	struct vattr va;
	int mode, crtmode;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT6(DB_SERVE,
	    "rs_open: sp %x, name %s, mode %x, crtmode %x,  rd->vp %x\n",
	    sp, msg_in->rq_data, msg_in->rq_fmode, msg_in->rq_crtmode,
	    sp->s_rdp->rd_vnode);

	/* Set client remote signal, if any */
	setrsig(sp, in_bp);

	/* Get file name and access mode from request message */
	name = msg_in->rq_data;
	mode = RFSTOOMODE(msg_in->rq_mode);
	crtmode = (msg_in->rq_crtmode & 07777) & ~msg_in->rq_cmask;

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Recover if system call is interrupted */
	if (setjmp(&u.u_qsave)) {
		error = EINTR;
		goto out;
	}
	/* Do the open */
	if (error = rs_vnopen(name, UIOSEG_KERNEL, mode, crtmode, sp, &vp))
		goto out;

	/* Get attributes of new vnode to return to client */
	if (error = VOP_GETATTR(vp, &va, u.u_cred))
		goto out;

	/* Make a receive descriptor for client requests on new vnode */
	if (error = make_gift(vp, FILE_QSIZE, sp, gift))
		goto out;

	srmount[sp->s_mntindx].sr_refcnt++;  /* New ref on this mnt point */
out:
	chkrsig(sp, &error);

	/* Make response message */
	make_response(sp, &in_bp, error, vp, &va, out_bp, outsize);

	msg_out = (struct response *) PTOMSG((*out_bp)->b_rptr);
	msg_out->rp_cache = 0;
	if (MND_LCK(va.va_mode))
		msg_out->rp_cache |= RP_MNDLCK;

	/* Free the request buffer (and the vnode if error) */
	if (in_bp)
		freemsg(in_bp);
	if (error) {
		if (vp)
			VN_RELE(vp);
	}
	DUPRINT4(DB_SERVE, "rs_open: result %d, name %s, vp %x\n",
			error, name, vp);
	return (error);
}

/*ARGSUSED*/
rs_close(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	rcvd_t rdp;
	struct vnode *vp = (struct vnode *) NULL;
	int holdvp = 0;
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT6(DB_SERVE, "rs_close: sp %x, mode %x, count %x, foffset %d, rd->vp %x\n", sp, msg_in->rq_mode, msg_in->rq_count, msg_in->rq_foffset, sp->s_rdp->rd_vnode);

	/* Set client remote signal, if any */
	setrsig(sp, in_bp);

	/* Recover if system call is interrupted */
	if (setjmp(&u.u_qsave)) {
		error = EINTR;
		goto out;
	}
	/* Get the vnode for the file to be closed */
	rdp = sp->s_rdp;
	vp = rdp->rd_vnode;

	/* Release any locks held by this client on this file */
	rsl_lockrelease(sp->s_rdp, sp->s_epid, (long) sp->s_mntindx);

	if (vp->v_count == 1 && rdp->rd_user_list != NULL &&
		rdp->rd_user_list->ru_fcount > 1) {
		VN_HOLD(vp);
		holdvp = 1;
	}
	error = VOP_CLOSE(vp, RFSTOFFLAG(msg_in->rq_mode),
				msg_in->rq_count, u.u_cred);

	if (holdvp)
		VN_RELE(vp);
out:
	chkrsig(sp, &error);

	/* Adjust client user counts for recovery */
	if (msg_in->rq_count == 1)
		del_rduser(sp->s_rdp, sp);

	/* Make response message */
	make_response(sp, &in_bp, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Free the request buffer */
	if (in_bp)
		freemsg(in_bp);

	DUPRINT3(DB_SERVE, "rs_close: result %d, vp %x\n", error, vp);
	return (error);
}

rs_creat(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	struct	response *msg_out;
	register char *name;
	struct vnode *vp = (struct vnode *) NULL;
	int error = 0;
	struct vattr va;
	int mode, crtmode;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT5(DB_SERVE, "rs_creat: sp %x, name %s, mode %x, rd->vp %x\n",
		sp, msg_in->rq_data, msg_in->rq_fmode, sp->s_rdp->rd_vnode);

	/* Set client remote signal, if any */
	setrsig(sp, in_bp);

	/* Get file name and create mode from request message */
	name = msg_in->rq_data;
	crtmode = (msg_in->rq_fmode & 07777) & ~msg_in->rq_cmask;
	mode  = FWRITE | FCREAT | FTRUNC;

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Recover if system call is interrupted */
	if (setjmp(&u.u_qsave)) {
		error = EINTR;
		goto out;
	}
	/* Do the open */
	if (error = rs_vnopen(name, UIOSEG_KERNEL, mode, crtmode, sp, &vp))
		goto out;

	/* Get attributes of new vnode to return to client */
	if (error = VOP_GETATTR(vp, &va, u.u_cred))
		goto out;

	/* Make a receive descriptor for client requests on new vnode */
	if (error = make_gift(vp, FILE_QSIZE, sp, gift))
		goto out;

	srmount[sp->s_mntindx].sr_refcnt++;  /* New ref on this mnt point */
out:
	chkrsig(sp, &error);
	/* Make response message */
	make_response(sp, &in_bp, error, vp, &va, out_bp, outsize);

	msg_out = (struct response *) PTOMSG((*out_bp)->b_rptr);
	msg_out->rp_cache = 0;
	if (MND_LCK(va.va_mode))
		msg_out->rp_cache |= RP_MNDLCK;

	/* Free the request buffer (and the vnode if error) */
	if (in_bp)
		freemsg(in_bp);
	if (error) {
		if (vp)
			VN_RELE(vp);
	}
	DUPRINT4(DB_SERVE, "rs_creat: result %d, name %s, vp %x\n",
					error, name, vp);
	return (error);
}

/*
 * First half of remote link system call: just does a lookup on first name
 * No permission checking is done so that this can be used as a lookup op
 * by an RFS client without any special permissions.
 */
rs_link(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register char *name;
	struct vnode *vp = (struct vnode *) NULL;
	int error = 0;
	struct vattr va;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT4(DB_SERVE, "rs_link: sp %x, name %s, rd->vp %x\n",
			sp, msg_in->rq_data, sp->s_rdp->rd_vnode);

	/* Get file name and access mode from request message */
	name = msg_in->rq_data;

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Get vnode for file to be linked */
	if (error = rs_lookupname(name, UIOSEG_KERNEL, NO_FOLLOW, sp,
		(struct vnode **) NULL, &vp))
		goto out;

	/* Get attributes of new vnode to return to client */
	if (error = VOP_GETATTR(vp, &va, u.u_cred))
		goto out;

	/* Make a receive descriptor for client requests on new vnode */
	if (error = make_gift(vp, FILE_QSIZE, sp, gift))
		goto out;
	srmount[sp->s_mntindx].sr_refcnt++;  /* New ref on this mnt point */
out:
	/* Make response message */
	make_response(sp, &in_bp, error, vp, &va, out_bp, outsize);

	/* Free the request buffer (and the vnode if error) */
	if (in_bp)
		freemsg(in_bp);
	if (error) {
		if (vp)
			VN_RELE(vp);
	}
	DUPRINT4(DB_SERVE, "rs_link: result %d, name %s, vp %x\n",
			error, name, vp);
	return (error);
}

/*
 * Second half of remote link system call: do the link.
 */
/*ARGSUSED*/
rs_link1(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register char *name;
	struct vnode *fvp = (struct vnode *) NULL;
	struct vnode *tdvp = (struct vnode *) NULL;
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT5(DB_SERVE,
	    "rs_link1: sp %x, name %s, fvp %x, rd->vp %x\n", sp,
	    msg_in->rq_data,
	    msg_in->rq_link ?
		rcvd[msg_in->rq_link].rd_vnode : (struct vnode *) NULL,
	    sp->s_rdp->rd_vnode);

	/* Get dest file name from request message */
	name = msg_in->rq_data;
	/* Get source vp: if rq_link == 0, source vp is not "on" this server */
	if (msg_in->rq_link)
		fvp = rcvd[msg_in->rq_link].rd_vnode;
	else
		fvp = (struct vnode *) NULL;

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Get vnode for file to be linked */
	if (error = rs_lookupname(name, UIOSEG_KERNEL, NO_FOLLOW, sp,
		&tdvp, (struct vnode **) NULL))
		goto out;

	/*
	 * Make sure both source vnode and target directory vnode are in
	 * the same vfs and that it is writeable.
	 */
	if (fvp == (struct vnode *) NULL || fvp->v_vfsp != tdvp->v_vfsp) {
		error = EXDEV;
		goto out;
	}
	if (tdvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/* Only super-user can link to directories */
	if (fvp->v_type == VDIR && u.u_uid != 0) {
		error = EPERM;
		goto out;
	}

	/* do the link */
	error = VOP_LINK(fvp, tdvp, name, u.u_cred);
out:
	/* Make response message */
	make_response(sp, &in_bp, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Free the request buffer and the directory vnode */
	if (in_bp)
		freemsg(in_bp);
	if (tdvp)
		VN_RELE(tdvp);
	DUPRINT3(DB_SERVE, "rs_link1: result %d, name %s\n",
			error, name);
	return (error);
}

/*ARGSUSED*/
rs_unlink(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register char *name;
	struct vnode *vp = (struct vnode *) NULL;
	struct vnode *dvp = (struct vnode *) NULL;
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT4(DB_SERVE, "rs_unlink: sp %x, name %s, rd->vp %x\n",
		sp, msg_in->rq_data, sp->s_rdp->rd_vnode);

	/* Get dir name from request message */
	name = msg_in->rq_data;

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Get vnode for file to be unlinked */
	if (error = rs_lookupname(name, UIOSEG_KERNEL, NO_FOLLOW, sp,
		&dvp, &vp))
		goto out;

	/* make sure there is an entry */
	if (vp == (struct vnode *)0) {
		error = ENOENT;
		goto out;
	}
	/* make sure filesystem is writeable */
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/* don't unlink the root of a mounted filesystem.  */
	if (vp->v_flag & VROOT) {
		error = EBUSY;
		goto out;
	}
	/* Only super-user can unlink directories */
	if (vp->v_type == VDIR && u.u_uid != 0) {
		error = EPERM;
		goto out;
	}
	/* release vnode before removing */
	VN_RELE(vp);
	vp = (struct vnode *)0;

	/* remove the file */
	error = VOP_REMOVE(dvp, name, u.u_cred);

out:
	/* Make response message */
	make_response(sp, &in_bp, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Free the request buffer (and the vnode if error) */
	if (in_bp)
		freemsg(in_bp);
	if (vp)
		VN_RELE(vp);
	if (dvp)
		VN_RELE(dvp);

	DUPRINT3(DB_SERVE, "rs_unlink: result %d, name %s\n", error, name);
	return (error);
}

rs_chroot(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register char *name;
	struct vnode *vp = (struct vnode *) NULL;
	int error = 0;
	struct vattr va;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT4(DB_SERVE, "rs_chroot: sp %x, name %s, rd->vp %x\n",
			sp, msg_in->rq_data, sp->s_rdp->rd_vnode);

	/* Get file name and access mode from request message */
	name = msg_in->rq_data;

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Get vnode for directory to be chdir'ed to */
	if (error = rs_lookupname(name, UIOSEG_KERNEL, NO_FOLLOW, sp,
		(struct vnode **) NULL, &vp))
		goto out;

	/* Must be a directory */
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto out;
	}
	/* Must have x access */
	if (error = VOP_ACCESS(vp, VEXEC, u.u_cred))
		goto out;

	/* Get attributes of new vnode to return to client */
	if (error = VOP_GETATTR(vp, &va, u.u_cred))
		goto out;

	/* Make a receive descriptor for client requests on new vnode */
	if (error = make_gift(vp, FILE_QSIZE, sp, gift))
		goto out;

	srmount[sp->s_mntindx].sr_refcnt++;  /* New ref on this mnt point */
out:
	/* Make response message */
	make_response(sp, &in_bp, error, vp, &va, out_bp, outsize);

	/* Free the request buffer (and the vnode if error) */
	if (in_bp)
		freemsg(in_bp);
	if (error) {
		if (vp)
			VN_RELE(vp);
	}
	DUPRINT4(DB_SERVE, "rs_chroot: result %d, name %s, vp %x\n",
				error, name, vp);
	return (error);
}

rs_exec(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register struct	response *msg_out;
	register char *name;
	struct vnode *vp = (struct vnode *) NULL;
	int error = 0;
	struct vattr va;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT4(DB_SERVE, "rs_exec: sp %x, name %s, rd->vp %x\n",
			sp, msg_in->rq_data, sp->s_rdp->rd_vnode);

	/* Get file name and access mode from request message */
	name = msg_in->rq_data;

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Get vnode for directory to be chroot'ed to */
	if (error = rs_lookupname(name, UIOSEG_KERNEL, NO_FOLLOW, sp,
		(struct vnode **) NULL, &vp))
		goto out;

	/* Must have x access */
	if (error = VOP_ACCESS(vp, VEXEC, u.u_cred))
		goto out;

	/* Get attributes of new vnode to return to client */
	if (error = VOP_GETATTR(vp, &va, u.u_cred))
		goto out;

	/* Make a receive descriptor for client requests on new vnode */
	if (error = make_gift(vp, FILE_QSIZE, sp, gift))
		goto out;

	(*gift)->rd_qtype |= RDTEXT;

	srmount[sp->s_mntindx].sr_refcnt++;  /* New ref on this mnt point */
out:
	/* Make response message */
	make_response(sp, &in_bp, error, vp, &va, out_bp, outsize);

	msg_out = (struct response *) PTOMSG((*out_bp)->b_rptr);
	*outsize = sizeof (struct response) - DATASIZE + RFS_DIRSIZ;
	msg_out->rp_mode = va.va_mode;
	msg_out->rp_subyte = 0; 	/* Message contains data */
	bcopy(name, (caddr_t) msg_out->rp_data, RFS_DIRSIZ);

	/* Free the request buffer (and the vnode if error) */
	if (in_bp)
		freemsg(in_bp);
	if (error) {
		if (vp)
			VN_RELE(vp);
	}
	DUPRINT4(DB_SERVE, "rs_exec: result %d, name %s, vp %x\n",
				error, name, vp);
	return (error);
}

/*ARGSUSED*/
rs_mknod(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register char *name;
	struct vnode *dvp = (struct vnode *) NULL;
	struct vnode *vp = (struct vnode *) NULL;
	struct vattr va;
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT6(DB_SERVE, "rs_mknod: sp %x, name %s, mode %x, dev %x, rd-vp %x\n", sp, msg_in->rq_data, msg_in->rq_fmode, msg_in->rq_dev, sp->s_rdp->rd_vnode);

	/* Get file name, type, mode and dev # from request message */
	name = msg_in->rq_data;
	vattr_null(&va);
	va.va_mode = (msg_in->rq_fmode & 07777) & ~msg_in->rq_cmask;
	va.va_type = RFSTOVT(msg_in->rq_fmode);
	va.va_rdev = msg_in->rq_dev;

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Get vnode for directory of file to be mknod'ed */
	if (error = rs_lookupname(name, UIOSEG_KERNEL, NO_FOLLOW, sp,
		&dvp, (struct vnode **) NULL))
		goto out;

	/* Can't change mode on read-only filesystems */
	if (dvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/* Make the node */
	error = VOP_CREATE(dvp, name, &va, EXCL, 0, &vp, u.u_cred);
out:
	/* Make response message */
	make_response(sp, &in_bp, error, vp, &va, out_bp, outsize);

	/* Free the request buffer (and the vnode if error) */
	if (in_bp)
		freemsg(in_bp);
	if (vp)
		VN_RELE(vp);
	if (dvp)
		VN_RELE(dvp);

	DUPRINT4(DB_SERVE, "rs_mknod: result %d, name %s, vp %x\n",
			error, name, vp);
	return (error);
}

/*ARGSUSED*/
rs_chmod(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register char *name;
	struct vnode *vp = (struct vnode *) NULL;
	struct vattr va;
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT5(DB_SERVE, "rs_chmod: sp %x, name %s, mode %x, rd->vp %x\n",
		sp, msg_in->rq_data, msg_in->rq_fmode, sp->s_rdp->rd_vnode);

	/* Get file name and mode change from request message */
	name = msg_in->rq_data;
	vattr_null(&va);
	va.va_mode = msg_in->rq_fmode & 07777;

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Get vnode for file to be chmod'ed */
	if (error = rs_lookupname(name, UIOSEG_KERNEL, NO_FOLLOW, sp,
		(struct vnode **) NULL, &vp))
		goto out;

	/* Can't change mode on read-only filesystems */
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/* Change the mode */
	error = VOP_SETATTR(vp, &va, u.u_cred);
out:
	/* Make response message */
	make_response(sp, &in_bp, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Free the request buffer (and the vnode if error) */
	if (in_bp)
		freemsg(in_bp);
	if (vp)
		VN_RELE(vp);

	DUPRINT4(DB_SERVE, "rs_chmod: result %d, name %s, vp %x\n",
			error, name, vp);
	return (error);
}

/*ARGSUSED*/
rs_chown(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register char *name;
	struct vnode *vp = (struct vnode *) NULL;
	struct vattr va;
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT6(DB_SERVE, "rs_chown: sp %x, name %s, owner %d, group %d, rd->vp %x\n", sp, msg_in->rq_data, msg_in->rq_newuid, msg_in->rq_newgid, sp->s_rdp->rd_vnode);

	/* Get file name and owner, group from request message */
	name = msg_in->rq_data;
	vattr_null(&va);
	va.va_uid = gluid(sp->s_gdpp, (ushort) msg_in->rq_newuid);
	va.va_gid = glgid(sp->s_gdpp, (ushort) msg_in->rq_newgid);

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Get vnode for file to be chown'ed */
	if (error = rs_lookupname(name, UIOSEG_KERNEL, NO_FOLLOW, sp,
		(struct vnode **) NULL, &vp))
		goto out;

	/* Can't change owner on read-only filesystems */
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/* Change the mode */
	error = VOP_SETATTR(vp, &va, u.u_cred);
out:
	/* Make response message */
	make_response(sp, &in_bp, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Free the request buffer (and the vnode if error) */
	if (in_bp)
		freemsg(in_bp);
	if (vp)
		VN_RELE(vp);

	DUPRINT4(DB_SERVE, "rs_chown: result %d, name %s, vp %x\n",
		error, name, vp);
	return (error);
}

/*ARGSUSED*/
rs_stat(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register struct	response *msg_out;
	register char *name;
	long bufp;
	struct vnode *vp = (struct vnode *) NULL;
	struct rfs_stat *stp;
	struct vattr va;
	extern int rfsdev_pseudo();
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT4(DB_SERVE, "rs_stat: sp %x, name %s, rd->vp %x\n",
		sp, msg_in->rq_data, sp->s_rdp->rd_vnode);

	/* Get syscall params: file name and client user buffer pointer */
	name = msg_in->rq_data;
	bufp = msg_in->rq_bufptr;

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Get vnode for file to be stat'ed */
	if (error = rs_lookupname(name, UIOSEG_KERNEL, NO_FOLLOW, sp,
		(struct vnode **) NULL, &vp))
		goto out;

	/* Get the file attributes */
	error = VOP_GETATTR(vp, &va, u.u_cred);

	/*
	 * Return EMFILE when inode is greater than 64k, more than a u_short
	 * can hold
	 */
	if (va.va_nodeid >= 0xfa00)
		error = EMFILE;

out:
	/* Make response message */
	make_response(sp, &in_bp, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Put stat data into response */
	if (!error) {
		*outsize = sizeof (struct response) - DATASIZE +
				sizeof (struct rfs_stat);
		msg_out = (struct response *) PTOMSG((*out_bp)->b_rptr);
		msg_out->rp_bufptr = bufp;
		msg_out->rp_subyte = 0; 	/* Message contains data */
		msg_out->rp_count = sizeof (struct rfs_stat);
		stp = (struct rfs_stat *) msg_out->rp_data;
		stp->st_mode = va.va_mode;
		stp->st_uid = va.va_uid;
		stp->st_gid = va.va_gid;
		stat_rmap(sp->s_gdpp, stp);
		/* Kludge up a dev# whose lower 8 bits are unique */
		stp->st_dev = rfsdev_pseudo((short) va.va_fsid);
		stp->st_ino = va.va_nodeid;
		stp->st_nlink = va.va_nlink;
		stp->st_size = va.va_size;
		stp->st_atime = va.va_atime.tv_sec + sp->s_gdpp->time;
		stp->st_mtime = va.va_mtime.tv_sec + sp->s_gdpp->time;
		stp->st_ctime = va.va_ctime.tv_sec + sp->s_gdpp->time;
		stp->st_rdev = (dev_t)va.va_rdev;
	}
	/* Free the request buffer and the vnode */
	if (in_bp)
		freemsg(in_bp);
	if (vp)
		VN_RELE(vp);

	DUPRINT4(DB_SERVE, "rs_stat: result %d, name %s, vp %x\n",
			error, name, vp);
	return (error);
}

/*
 * Remote seek -- only gets called if seek is re end of file. Only job
 * is to return current file size.
 */
/*ARGSUSED*/
rs_seek(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	response *msg_out;
	struct vnode *vp = (struct vnode *) NULL;
	struct vattr va;
	int error = 0;

	DUPRINT3(DB_SERVE, "rs_seek: sp %x, rd->vp %x\n", sp,
			sp->s_rdp->rd_vnode);

	freemsg(in_bp);
	vp = sp->s_rdp->rd_vnode;

	/* Get the file attributes */
	error = VOP_GETATTR(vp, &va, u.u_cred);

	/* Make response message */
	make_response(sp, (mblk_t **) NULL, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Return file size */
	if (!error) {
		msg_out = (struct response *) PTOMSG((*out_bp)->b_rptr);
		msg_out->rp_rval = va.va_size;
	}
	DUPRINT3(DB_SERVE, "rs_seek: result %d size %d\n", error, va.va_size);
	return (error);
}

/*ARGSUSED*/
rs_fstat(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register struct	response *msg_out;
	extern int rfsdev_pseudo();
	long bufp;
	struct vnode *vp = (struct vnode *) NULL;
	struct vattr va;
	struct rfs_stat *stp;
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT3(DB_SERVE, "rs_fstat: sp %x, rd->vp %x\n", sp,
			sp->s_rdp->rd_vnode);

	/* Get syscall params: client user buffer pointer and vnode */
	bufp = msg_in->rq_bufptr;
	vp = sp->s_rdp->rd_vnode;

	/* Get the file attributes */
	error = VOP_GETATTR(vp, &va, u.u_cred);

#define	FLAG 1
#ifdef FLAG
	/*
	 * Return EMFILE when inode is greater than 64k, more than a u_short
	 * can hold
	 */
	if (va.va_nodeid >= 0xfa00)
		error = EMFILE;
#endif

	/* Make response message */
	make_response(sp, &in_bp, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Put stat data into response */
	if (!error) {
		*outsize = sizeof (struct response) - DATASIZE +
				sizeof (struct rfs_stat);
		msg_out = (struct response *) PTOMSG((*out_bp)->b_rptr);
		msg_out->rp_bufptr = bufp;
		msg_out->rp_subyte = 0; 	/* Message contains data */
		msg_out->rp_count = sizeof (struct rfs_stat);
		stp = (struct rfs_stat *) msg_out->rp_data;
		stp->st_mode = va.va_mode;
		stp->st_uid = va.va_uid;
		stp->st_gid = va.va_gid;
		stat_rmap(sp->s_gdpp, stp);
		/* Kludge up a dev# whose lower 8 bits are unique */
		stp->st_dev = rfsdev_pseudo((short) va.va_fsid);
		stp->st_ino = va.va_nodeid;
		stp->st_nlink = va.va_nlink;
		stp->st_size = va.va_size;
		stp->st_atime = va.va_atime.tv_sec + sp->s_gdpp->time;
		stp->st_mtime = va.va_mtime.tv_sec + sp->s_gdpp->time;
		stp->st_ctime = va.va_ctime.tv_sec + sp->s_gdpp->time;
		stp->st_rdev = (dev_t)va.va_rdev;
	}

	/* Free the request buffer */
	if (in_bp)
		freemsg(in_bp);

	DUPRINT3(DB_SERVE, "rs_fstat: result %d, vp %x\n",
			error, vp);
	return (error);
}

/*ARGSUSED*/
rs_utime(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register struct	response *msg_out;
	register char *name;
	struct vnode *vp = (struct vnode *) NULL;
	struct vattr va;
	struct rfs_utimbuf *tp;
	extern  struct timeval  time;
	mblk_t *data_bp, *go_bp;
	struct	response *data_msg;
	long bufp;
	rcvd_t rd = (rcvd_t) NULL;
	int no_sig = 1;
	int rs;
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT5(DB_SERVE, "rs_utime: sp %x, name %s, bufp %x, rd->vp %x\n",
		sp, msg_in->rq_data, msg_in->rq_bufptr, sp->s_rdp->rd_vnode);

	/* Get file name and client user time buffer pointer */
	name = msg_in->rq_data;
	bufp = msg_in->rq_bufptr;
	vattr_null(&va);

	/* Use local current time or get from client if specified */
	if (!bufp) {
		va.va_mtime.tv_sec = time.tv_sec;
		va.va_mtime.tv_usec = -1;
	} else {
		/* Create rd on which to receive time values */
		if ((rd = cr_rcvd(FILE_QSIZE, SPECIFIC)) == (rcvd_t) NULL) {
			error = ENOMEM;
			goto out;
		}
		rd->rd_sd = sp->s_sdp;
		rd->rd_qsize = SIGQSIZE;

		/* Send back go-ahead message */
		go_bp = salocbuf(sizeof (struct response), BPRI_MED);
		msg_out = (struct response *)PTOMSG(go_bp->b_rptr);
		msg_out->rp_type = RESP_MSG;
		msg_out->rp_opcode = DUCOPYIN;
		msg_out->rp_errno = 0;
		msg_out->rp_bufptr = bufp;
		msg_out->rp_bp = (long) go_bp;
		msg_out->rp_count = sizeof (struct rfs_utimbuf);
		if (error = sndmsg(sp->s_sdp, go_bp,
			sizeof (struct response) - DATASIZE, rd)) {
			free_rcvd(rd);
			goto out;
		}

		/* Get back times */
		error = de_queue(rd, &data_bp, (sndd_t)NULL, &rs, &no_sig,
			(sndd_t)NULL);
		free_rcvd(rd);
		if (error)
			goto out;
		data_msg = (struct response *)PTOMSG(data_bp->b_rptr);
		if (data_msg->rp_errno) {
			error = RFSTOERR(data_msg->rp_errno);
			freemsg(data_bp);
			goto out;
		}
		if (sp->s_gdpp->hetero != NO_CONV)
			(void) fcanon(rfs_utimbuf, data_msg->rp_data,
					data_msg->rp_data);
		tp = (struct rfs_utimbuf *) data_msg->rp_data;
		vattr_null(&va);
		/* Adjust to local time */
		va.va_atime.tv_sec = tp->ut_actime - sp->s_gdpp->time;
		va.va_mtime.tv_sec = tp->ut_modtime - sp->s_gdpp->time;
		va.va_atime.tv_usec = 0;
		va.va_mtime.tv_usec = 0;
		freemsg(data_bp);

		DUPRINT3(DB_SERVE, "rs_utime: times %d %d\n",
			va.va_atime.tv_sec, va.va_mtime.tv_sec);
	}

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Get vnode for file whose times are to be changed */
	if (error = rs_lookupname(name, UIOSEG_KERNEL, NO_FOLLOW, sp,
		(struct vnode **) NULL, &vp))
		goto out;

	/* Can't change attributes on read-only filesystems */
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/* Change the time */
	error = VOP_SETATTR(vp, &va, u.u_cred);
out:
	/* Make response message */
	make_response(sp, &in_bp, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Free the request buffer (and the vnode if error) */
	if (in_bp)
		freemsg(in_bp);
	if (vp)
		VN_RELE(vp);

	DUPRINT4(DB_SERVE, "rs_utime: result %d, name %s, vp %x\n",
			error, name, vp);
	return (error);
}


/*ARGSUSED*/
rs_access(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register char *name;
	struct vnode *vp = (struct vnode *) NULL;
	u_short mode;
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT5(DB_SERVE, "rs_access: sp %x, name %s, mode %x, rd->vp %x\n",
		sp, msg_in->rq_data, msg_in->rq_fmode, sp->s_rdp->rd_vnode);

	/* Get file name and access mode from request message */
	name = msg_in->rq_data;
	mode = RFSTOVACC(msg_in->rq_fmode);

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Get vnode for file to be accessed */
	if (error = rs_lookupname(name, UIOSEG_KERNEL, NO_FOLLOW, sp,
		(struct vnode **) NULL, &vp))
		goto out;

	/* No write access to read-only filesystems */
	if ((mode & VWRITE) && (vp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		error = EROFS;
		goto out;
	}
	/* Do the access */
	error = VOP_ACCESS(vp, mode, u.u_cred);
out:
	/* Make response message */
	make_response(sp, &in_bp, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Free the request buffer and the vnode */
	if (in_bp)
		freemsg(in_bp);
	if (vp)
		VN_RELE(vp);

	DUPRINT4(DB_SERVE, "rs_access: result %d, name %s, vp %x\n",
			error, name, vp);
	return (error);
}


/*ARGSUSED*/
rs_statfs(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register struct	response *msg_out;
	register char *name;
	long bufp, len, fstyp;
	struct vnode *vp = (struct vnode *) NULL;
	register struct rfs_statfs *stp;
	struct statfs sb;
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);

	DUPRINT6(DB_SERVE,
	    "rs_statfs: sp %x, name %x, bufp %x, len %d, rd->vp %x\n",
	    sp, msg_in->rq_data, msg_in->rq_bufptr, msg_in->rq_len,
	    sp->s_rdp->rd_vnode);

	/*
	 * Get syscall params: file name, client user buffer pointer, length
	 * of data to be returned, file system type
	 */
	name = msg_in->rq_data;
	bufp = msg_in->rq_bufptr;
	len = msg_in->rq_len;
	fstyp = msg_in->rq_fstyp;

	/*
	 * Client must have right length. Note: this is not a particularly
	 * meaningful value as it refers to the size of a data structure whose
	 * size may vary from machine to machine.
	 */
	if (len < 0) {
		error = EINVAL;
		goto out;
	}
	/*
	 * Only support mounted filesystems (fstyp = 0, and name is name
	 * of file in filesystem to statfs'ed)
	 */
	if (fstyp) {
		error = EINVAL;
		goto out;
	}
	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Get vnode for file to be statfs'ed */
	if (error = rs_lookupname(name, UIOSEG_KERNEL, NO_FOLLOW, sp,
		(struct vnode **) NULL, &vp))
		goto out;

	error = VFS_STATFS(vp->v_vfsp, &sb);
out:
	/* Make response message */
	make_response(sp, &in_bp, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Put statfs data into response */
	if (!error) {
		*outsize = sizeof (struct response) - DATASIZE +
				sizeof (struct rfs_statfs);
		msg_out = (struct response *) PTOMSG((*out_bp)->b_rptr);
		msg_out->rp_bufptr = bufp;
		msg_out->rp_subyte = 0; 	/* Message contains data */
		msg_out->rp_count = sizeof (struct rfs_statfs);
		stp = (struct rfs_statfs *) msg_out->rp_data;
		stp->f_fstyp = 0;	/* Not a meaningful value */
		stp->f_bsize = sb.f_bsize;
		stp->f_frsize = 0;
		stp->f_blocks = sb.f_blocks;
		stp->f_bfree = sb.f_bfree;
		stp->f_files = sb.f_files;
		stp->f_ffree = sb.f_ffree;
		stp->f_fname[0] = '\0';
		stp->f_fpack[0] = '\0';
	}

	/* Free the request buffer and the vnode */
	if (in_bp)
		freemsg(in_bp);
	if (vp)
		VN_RELE(vp);

	DUPRINT4(DB_SERVE, "rs_statfs: result %d, name %s, vp %x\n",
			error, name, vp);
	return (error);
}

/*ARGSUSED*/
rs_fstatfs(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register struct	response *msg_out;
	long bufp, len, fstyp;
	struct vnode *vp = (struct vnode *) NULL;
	struct rfs_statfs *stp;
	struct statfs sb;
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT6(DB_SERVE,
	    "rs_fstatfs: sp %x, bufp %x, len %d, fstyp %d, rd->vp %x\n",
	    sp, msg_in->rq_bufptr, msg_in->rq_len, msg_in->rq_fstyp,
	    sp->s_rdp->rd_vnode);

	/*
	 * Get syscall params: client user buffer pointer, length
	 * of data to be returned, file system type, vnode
	 */
	bufp = msg_in->rq_bufptr;
	len = msg_in->rq_len;
	fstyp = msg_in->rq_fstyp;
	vp = sp->s_rdp->rd_vnode;

	/*
	 * Client must have right length. Note: this is not a particularly
	 * meaningful value as it refers to the size of a data structure whose
	 * size may vary from machine to machine.
	 */
	if (len < 0) {
		error = EINVAL;
		goto out;
	}
	/*
	 * Only support mounted filesystems (fstyp = 0 and vnode is in
	 * filesystem to statfs)
	 */
	if (fstyp) {
		error = EINVAL;
		goto out;
	}
	/* Get the statfs info */
	error = VFS_STATFS(vp->v_vfsp, &sb);
out:
	/* Make response message */
	make_response(sp, &in_bp, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Put statfs data into response */
	if (!error) {
		*outsize = sizeof (struct response) - DATASIZE +
				sizeof (struct rfs_statfs);
		msg_out = (struct response *) PTOMSG((*out_bp)->b_rptr);
		msg_out->rp_bufptr = bufp;
		msg_out->rp_subyte = 0; 	/* Message contains data */
		msg_out->rp_count = sizeof (struct rfs_statfs);
		stp = (struct rfs_statfs *) msg_out->rp_data;
		stp->f_fstyp = sb.f_type;
		stp->f_bsize = sb.f_bsize;
		stp->f_frsize = 0;
		stp->f_blocks = sb.f_blocks;
		stp->f_bfree = sb.f_bfree;
		stp->f_files = sb.f_files;
		stp->f_ffree = sb.f_ffree;
		stp->f_fname[0] = '\0';
		stp->f_fpack[0] = '\0';
	}

	/* Free the request buffer */
	if (in_bp)
		freemsg(in_bp);
	DUPRINT2(DB_SERVE, "rs_fstatfs: result %d\n",
			error);
	return (error);
}

/*ARGSUSED*/
rs_ioctl(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register struct	response *msg_out;
	struct vnode *vp = (struct vnode *) NULL;
	long cmd, arg, flag;
	int size;
	char data[_IOCPARM_MASK+1];
	mblk_t *data_bp, *go_bp;
	struct	response *data_msg;
	rcvd_t rd = (rcvd_t) NULL;
	int no_sig = 1;
	int rs;
	int msgsz;
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT6(DB_SERVE,
	    "rs_ioctl: sp %x, cmd %x, arg %x, flag %x, rd->vp %x\n",
	    sp, msg_in->rq_cmd, msg_in->rq_ioarg, msg_in->rq_fflag,
	    sp->s_rdp->rd_vnode);
	/*
	 * Can only do ioctl between homogeneous machines, since you
	 * don't know what data elements are in the ioctl args
	 */
	if (sp->s_gdpp->hetero != NO_CONV) {
		freemsg(in_bp);
		error = EINVAL;
		goto out;
	}
	/* Get syscall params: cmd, arg, flag */
	cmd = msg_in->rq_cmd;
	arg = msg_in->rq_ioarg;
	flag = msg_in->rq_fflag;
	size = (cmd &~ (_IOC_INOUT|_IOC_VOID)) >> 16;
	vp = sp->s_rdp->rd_vnode;

	/* Set client remote signal, if any */
	setrsig(sp, in_bp);
	freemsg(in_bp);

	/* Recover if system call is interrupted */
	if (setjmp(&u.u_qsave)) {
		error = EINTR;
		goto out;
	}
	/* If data is to come in, get it (DUCOPYIN) */
	if (cmd & _IOC_IN)  {
		/* Create rd on which to receive data */
		if ((rd = cr_rcvd(FILE_QSIZE, SPECIFIC)) == (rcvd_t) NULL) {
			error = ENOMEM;
			goto out;
		}
		rd->rd_sd = sp->s_sdp;
		rd->rd_qsize = SIGQSIZE;

		/* Send back go-ahead message */
		go_bp = salocbuf(sizeof (struct response), BPRI_MED);
		msg_out = (struct response *)PTOMSG(go_bp->b_rptr);
		msg_out->rp_type = RESP_MSG;
		msg_out->rp_opcode = DUCOPYIN;
		msg_out->rp_bufptr = arg;
		msg_out->rp_bp = (long) go_bp;
		msg_out->rp_count = size;
		msg_out->rp_errno = 0;
		if (error = sndmsg(sp->s_sdp, go_bp,
			sizeof (struct response) - DATASIZE, rd)) {
			free_rcvd(rd);
			goto out;
		}
		/* Get back data */
		error = de_queue(rd, &data_bp, (sndd_t)NULL, &rs, &no_sig,
			(sndd_t)NULL);
		free_rcvd(rd);
		if (error)
			goto out;
		data_msg = (struct response *)PTOMSG(data_bp->b_rptr);
		if (data_msg->rp_errno) {
			error = RFSTOERR(data_msg->rp_errno);
			freemsg(data_bp);
			goto out;
		}
		ASSERT(size == data_msg->rp_count);
		bcopy((caddr_t) data_msg->rp_data, (caddr_t) data, (u_int) size);
		freemsg(data_bp);
		DUPRINT5(DB_SERVE, "rs_ioctl: DUCOPYIN data: %x %x %x %x %x\n",
			(unsigned *) data[0], (unsigned *) data[1],
			(unsigned *) data[2], (unsigned *) data[3]);
	}

	/* Do the ioctl */
	error = VOP_IOCTL(vp, cmd, data, flag, u.u_cred);

	/* If data is to go out, send it out (DUCOPYOUT) */
	if (cmd & _IOC_OUT)  {
		data_bp = salocbuf(sizeof (struct response), BPRI_MED);
		msg_out = (struct response *)PTOMSG(data_bp->b_rptr);
		msg_out->rp_type = RESP_MSG;
		msg_out->rp_opcode = DUCOPYOUT;
		msg_out->rp_bufptr = arg;
		msg_out->rp_bp = (long) data_bp;
		msg_out->rp_subyte = 0; 	/* Message contains data */
		msg_out->rp_count = size;
		msgsz = sizeof (struct response) - DATASIZE + size;
		msg_out->rp_copysync = 0;
		msg_out->rp_errno = 0;
		bcopy(data, (caddr_t) msg_out->rp_data, (u_int) size);
		error = sndmsg(sp->s_sdp, data_bp, msgsz, (rcvd_t) NULL);
		DUPRINT5(DB_SERVE, "rs_ioctl: DUCOPYOUT data: %x %x %x %x\n",
			(unsigned *) data[0], (unsigned *) data[1],
			(unsigned *) data[2], (unsigned *) data[3]);
	}
out:
	chkrsig(sp, &error);

	/* Make response message */
	make_response(sp, (mblk_t **) NULL, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	DUPRINT3(DB_SERVE, "rs_ioctl: result %d, vp %x\n",
			error, vp);
	return (error);
}

/*ARGSUSED*/
rs_utssys(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register struct	response *msg_out;
	struct statfs sb;
	struct rfs_ustat *rsb;
	struct vfs *vfs;
	long dev;
	long bufp;
	int error = 0;
	short rfsdev_real();

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT4(DB_SERVE, "rs_utssys: sp %x, dev %x, bufp %x\n",
		sp, msg_in->rq_dev, msg_in->rq_bufptr);

	dev = msg_in->rq_dev;
	bufp = msg_in->rq_bufptr;

	/* Get the vfs corresponding to this device number */
	if (error = vafsidtovfs((long) rfsdev_real((int) dev) & 0x0ffff, &vfs))
		goto out;

	/* Statfs the filesystem */
	if (error = VFS_STATFS(vfs, &sb))
		goto out;
out:
	/* Make response message */
	make_response(sp, &in_bp, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Put statfs data into response */
	if (!error) {
		*outsize = sizeof (struct response) - DATASIZE +
				sizeof (struct rfs_ustat);
		msg_out = (struct response *) PTOMSG((*out_bp)->b_rptr);
		msg_out->rp_bufptr = bufp;
		msg_out->rp_subyte = 0; 	/* Message contains data */
		msg_out->rp_count = sizeof (struct rfs_ustat);
		rsb = (struct rfs_ustat *) msg_out->rp_data;
		rsb->f_tfree = sb.f_bavail * (sb.f_bsize / DEV_BSIZE);
		rsb->f_tinode = sb.f_ffree;
		bzero((caddr_t)rsb->f_fname, 6);
		bzero((caddr_t)rsb->f_fpack, 6);
	}

	/* Free the request buffer and the vnode */
	if (in_bp)
		freemsg(in_bp);

	DUPRINT3(DB_SERVE, "rs_utssys: result %d, free blocks %d\n",
		error, error ? 0 : rsb->f_tfree);
	return (error);
}

rs_chdir(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register char *name;
	struct vnode *vp = (struct vnode *) NULL;
	int error = 0;
	struct vattr va;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT4(DB_SERVE, "rs_chdir: sp %x, name %s, rd->vp %x\n",
			sp, msg_in->rq_data, sp->s_rdp->rd_vnode);

	/* Get file name and access mode from request message */
	name = msg_in->rq_data;

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Get vnode for directory to be chroot'ed to */
	if (error = rs_lookupname(name, UIOSEG_KERNEL, NO_FOLLOW, sp,
		(struct vnode **) NULL, &vp))
		goto out;

	/* Must be a directory */
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto out;
	}
	/* Must have x access */
	if (error = VOP_ACCESS(vp, VEXEC, u.u_cred))
		goto out;

	/* Get attributes of new vnode to return to client */
	if (error = VOP_GETATTR(vp, &va, u.u_cred))
		goto out;

	/* Make a receive descriptor for client requests on new vnode */
	if (error = make_gift(vp, FILE_QSIZE, sp, gift))
		goto out;
	srmount[sp->s_mntindx].sr_refcnt++;  /* New ref on this mnt point */
out:
	/* Make response message */
	make_response(sp, &in_bp, error, vp, &va, out_bp, outsize);

	/* Free the request buffer (and the vnode if error) */
	if (in_bp)
		freemsg(in_bp);
	if (error) {
		if (vp)
			VN_RELE(vp);
	}
	DUPRINT4(DB_SERVE, "rs_chdir: result %d, name %s, vp %x\n",
			error, name, vp);
	return (error);
}

/*ARGSUSED*/
rs_fcntl(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register struct	response *msg_out;
	struct vnode *vp = (struct vnode *) NULL;
	long cmd, arg, flag;
	struct file ff;
	int size;
	struct flock ld;
	mblk_t *data_bp, *go_bp;
	struct	response *data_msg;
	rcvd_t rd = (rcvd_t) NULL;
	int no_sig = 1;
	int rs, s;
	int msgsz;
	int error = 0;
	int frcache;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT6(DB_SERVE, "rs_fcntl: sp %x, cmd %x, foffset %x, flag %x, rd->vp %x\n", sp, msg_in->rq_cmd, msg_in->rq_foffset, msg_in->rq_fflag, sp->s_rdp->rd_vnode);

	/* Get syscall params: cmd, arg, flag */
	s = splrf();
	cmd = RFSTOFCNTL(msg_in->rq_cmd);
	flag = msg_in->rq_fflag;
	frcache = (flag & RFS_FRCACH);
	flag &= ~RFS_FRCACH;
	flag = RFSTOFFLAG(flag);
	arg = msg_in->rq_fcntl;
	size = sizeof (struct rfs_flock);
	vp = sp->s_rdp->rd_vnode;
	ff.f_offset = msg_in->rq_foffset;
	ff.f_data = (caddr_t) vp;

	/* Set client remote signal, if any */
	setrsig(sp, in_bp);
	if (!frcache)
		freemsg(in_bp);


	/* Recover if system call is interrupted */
	if (setjmp(&u.u_qsave)) {
		error = EINTR;
		goto out;
	}
	/* Check for valid command */
	if (cmd != F_GETLK && cmd != F_SETLK &&
		cmd != F_SETLKW && cmd != F_CHKFL && cmd != F_FREESP) {
		error = EINVAL;
		goto out;
	}
	/* Not supported */
	if (cmd == F_FREESP) {
		error = EINVAL;
		goto out;
	}
	/*
	 * Check file flags for validity of requested changes --
	 * S5R3 does nothing
	 */
	if (cmd == F_CHKFL) {
		goto out;
	}

	/* If lock structure is to come in, get it */
	if (cmd == F_GETLK || cmd == F_SETLK || cmd == F_SETLKW ||
		cmd == F_CHKFL) {
		/* New (5.3.1) style -- pass lock req. with fcntl call */
		if (frcache) {
			if (msg_in->rq_prewrite > 0)  {
				bcopy((caddr_t) msg_in->rq_data, (caddr_t) &ld,
					(u_int) size);
				freemsg(in_bp);
			}
		/* Old style (DUCOPYIN) */
		} else {
			/* Create rd on which to receive data */
			if ((rd = cr_rcvd(FILE_QSIZE, SPECIFIC))
				== (rcvd_t) NULL) {
				error = ENOMEM;
				goto out;
			}
			rd->rd_sd = sp->s_sdp;
			rd->rd_qsize = SIGQSIZE;

			/* Send back go-ahead message */
			go_bp = salocbuf(sizeof (struct response), BPRI_MED);
			msg_out = (struct response *)PTOMSG(go_bp->b_rptr);
			msg_out->rp_type = RESP_MSG;
			msg_out->rp_opcode = DUCOPYIN;
			msg_out->rp_bufptr = arg;
			msg_out->rp_bp = (long) go_bp;
			msg_out->rp_count = size;
			msg_out->rp_errno = 0;
			if (error = sndmsg(sp->s_sdp, go_bp,
					sizeof (struct response) - DATASIZE, rd)) {
				free_rcvd(rd);
				goto out;
			}
			/* Get back data */
			error = de_queue(rd, &data_bp, (sndd_t)NULL, &rs,
					&no_sig, (sndd_t)NULL);
			free_rcvd(rd);
			if (error)
				goto out;
			data_msg = (struct response *)PTOMSG(data_bp->b_rptr);
			if (data_msg->rp_errno) {
				error = RFSTOERR(data_msg->rp_errno);
				freemsg(data_bp);
				goto out;
			}
			ASSERT(size == data_msg->rp_count);
			bcopy((caddr_t) data_msg->rp_data, (caddr_t) &ld,
				(u_int) size);
			freemsg(data_bp);
			DUPRINT6(DB_SERVE, "rs_fcntl: DUCOPYIN: type %x, whence %x, start %x, len %x, pid %x\n", ld.l_type, ld.l_whence, ld.l_start, ld.l_len, ld.l_pid);
		}
	}

	/* The following code is basically ripped out of fcntl() */

	/*
	 * *** NOTE ***
	 * The SVID does not say what to return on file access errors!
	 * Here, EBADF is returned, which is compatible with S5R3
	 * and is less confusing than EACCES
	 */
	/* check access permissions */
	if (cmd != F_GETLK) {
		switch (ld.l_type) {
			case F_RDLCK:
				if (!(flag & FREAD)) {
					error = EBADF;
					goto out;
				}
				break;
			case F_WRLCK:
				if (!(flag & FWRITE)) {
					error = EBADF;
					goto out;
				}
				break;
			case F_UNLCK:
				break;
			default:
				error = EINVAL;
				goto out;
		}
	}

	/* convert negative lengths to positive */
	if (ld.l_len < 0) {
		ld.l_start += ld.l_len;		/* adjust start point */
		ld.l_len = -(ld.l_len);		/* absolute value */
	}

	/* check for validity */
	if (ld.l_start < 0) {
		error = EINVAL;
		goto out;
	}
	/*
	 * Dispatch out to vnode layer to do the actual locking.
	 * Then, translate error codes for SVID compatibility
	 */
	error = VOP_LOCKCTL(vp, &ld, cmd, u.u_cred,
				RSL_PID(sp->s_mntindx, sp->s_epid));
	if (error) {
		if (error == EWOULDBLOCK)
			error = EACCES;
		goto out;
	}
	/*
	 * If request was to set a lock, allocate a server locking record.
	 * These are attached to the receive descriptor for the file.
	 * They are used to release locks when the client does a close or
	 * the connection is broken. They are also freed at that time.
	 */
	if (((cmd == F_SETLK) || (cmd == F_SETLKW)) && (ld.l_type != F_UNLCK))
		rsl_alloc(sp->s_rdp, sp->s_epid, (long)sp->s_mntindx, u.u_cred);

	/* If data is to go out, send it out (DUCOPYOUT) (old 5.3.0 style) */
	if (cmd == F_GETLK && !frcache) {
		data_bp = salocbuf(sizeof (struct response), BPRI_MED);
		msg_out = (struct response *)PTOMSG(data_bp->b_rptr);
		msg_out->rp_type = RESP_MSG;
		msg_out->rp_opcode = DUCOPYOUT;
		msg_out->rp_bufptr = arg;
		msg_out->rp_bp = (long) data_bp;
		msg_out->rp_subyte = 0; 	/* Message contains data */
		msg_out->rp_count = size;
		msgsz = sizeof (struct response) - DATASIZE + size;
		msg_out->rp_copysync = 0;
		msg_out->rp_errno = 0;
		/* Convert from local to RFS flock structure */
		((struct rfs_flock *) &ld)->l_pid = ld.l_pid;
		bcopy((caddr_t) &ld, (caddr_t) msg_out->rp_data, (u_int) size);
		error = sndmsg(sp->s_sdp, data_bp, msgsz, (rcvd_t) NULL);
		DUPRINT6(DB_SERVE, "rs_fcntl: DUCOPYOUT: type %x, whence %x, start %x, len %x, pid %x\n", ld.l_type, ld.l_whence, ld.l_start, ld.l_len, ld.l_pid);
	}
out:
	chkrsig(sp, &error);

	/* Make response message */
	make_response(sp, (mblk_t **) NULL, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);
	msg_out = (struct response *)PTOMSG((*out_bp)->b_rptr);

	/* New style (5.3.1) -- return lock with response */
	if (cmd == F_GETLK && frcache) {
		msg_out->rp_bufptr = arg;
		*outsize = sizeof (struct response) - DATASIZE + size;
		/* Convert from local to RFS flock structure */
		((struct rfs_flock *) &ld)->l_pid = ld.l_pid;
		((struct rfs_flock *) &ld)->l_whence = ld.l_whence;
		((struct rfs_flock *) &ld)->l_start = ld.l_start;
		((struct rfs_flock *) &ld)->l_len = ld.l_len;
		((struct rfs_flock *) &ld)->l_type = ld.l_type;
		bcopy((caddr_t) &ld, (caddr_t) msg_out->rp_data, (u_int) size);
	}
	msg_out->rp_rval = FCNTLTORFS(cmd);

	DUPRINT3(DB_SERVE, "rs_fcntl: result %d, vp %x\n", error, vp);
	(void) splx(s);
	return (error);
}

/*ARGSUSED*/
rs_rmdir(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register char *name;
	struct vnode *vp = (struct vnode *) NULL;
	struct vnode *dvp = (struct vnode *) NULL;
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT4(DB_SERVE, "rs_rmdir: sp %x, name %s, rd->vp %x\n",
			sp, msg_in->rq_data, sp->s_rdp->rd_vnode);

	/* Get dir name from request message */
	name = msg_in->rq_data;

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Get vnode for file to be rmdir'ed */
	error = rs_lookupname(name, UIOSEG_KERNEL, NO_FOLLOW, sp, &dvp, &vp);
	if (error)
		goto out;

	/* make sure there is an entry */
	if (vp == (struct vnode *)0) {
		error = ENOENT;
		goto out;
	}
	/* make sure filesystem is writeable */
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/* don't unlink the root of a mounted filesystem.  */
	if (vp->v_flag & VROOT) {
		error = EBUSY;
		goto out;
	}
	/* Must be a directory */
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto out;
	}

	/* release vnode before removing */
	VN_RELE(vp);
	vp = (struct vnode *)0;

	/* remove the directory */
	error = VOP_RMDIR(dvp, name, u.u_cred);

out:
	/* Make response message */
	make_response(sp, &in_bp, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Free the request buffer (and the vnode if error) */
	if (in_bp)
		freemsg(in_bp);
	if (vp)
		VN_RELE(vp);
	if (dvp)
		VN_RELE(dvp);

	DUPRINT4(DB_SERVE, "rs_rmdir: result %d, name %s, vp %x\n",
			error, name, vp);
	return (error);
}

/*ARGSUSED*/
rs_mkdir(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register char *name;
	struct vnode *dvp = (struct vnode *) NULL;
	struct vnode *vp = (struct vnode *) NULL;
	struct vattr va;
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT5(DB_SERVE, "rs_mkdir: sp %x, name %s, mode %x, rd->vp %x\n",
		sp, msg_in->rq_data, msg_in->rq_fmode, sp->s_rdp->rd_vnode);

	/* Get file name and mode change from request message */
	name = msg_in->rq_data;
	vattr_null(&va);
	va.va_mode = (msg_in->rq_fmode & 07777) & ~msg_in->rq_cmask;
	va.va_type = VDIR;

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Get vnode for directory of file to be mkdir'ed */
	if (error = rs_lookupname(name, UIOSEG_KERNEL, NO_FOLLOW, sp,
		&dvp, (struct vnode **) NULL))
		goto out;

	/* Can't change mode on read-only filesystems */
	if (dvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/* Make the directory */
	error = VOP_MKDIR(dvp, name, &va, &vp, u.u_cred);
out:
	/* Make response message */
	make_response(sp, &in_bp, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Free the request buffer (and the vnode if error) */
	if (in_bp)
		freemsg(in_bp);
	if (vp)
		VN_RELE(vp);
	if (dvp)
		VN_RELE(dvp);

	DUPRINT4(DB_SERVE, "rs_mkdir: result %d, name %s, vp %x\n",
			error, name, vp);
	return (error);
}

/*
 * Server side of RFS remote mount
 */
rs_srmount(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	extern struct advertise *findadv();
	extern  struct timeval  time;
	register struct	request	*msg_in;
	register struct	response *msg_out;
	struct	srmnt *smp = (struct srmnt *) NULL;
	struct	advertise *ap;
	register struct vnode *vp;
	char	resname[NMSZ];
	int cl_mntindx;
	struct vattr vattr;
	long rwflag;
	int error = 0;

	DUPRINT3(DB_SERVE, "rs_srmount: sp %x, rd->vp %x\n",
		sp, sp->s_rdp->rd_vnode);
	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	rwflag = msg_in->rq_flag;

	/* For first mount on circuit initialize circuit sysid and time */
	if (sp->s_gdpp->mntcnt == 0) {
		hibyte(sp->s_gdpp->sysid) = lobyte(loword(msg_in->rq_sysid));
		sp->s_gdpp->time = msg_in->rq_synctime - time.tv_sec;
	}

	/* De-serialize the resource name */
	(void) fcanon(rfs_string, msg_in->rq_data, resname);

	/* Free the request buffer */
	freemsg(in_bp);

	/* Find the resource in the table of advertised resources */
	if ((ap = findadv(resname)) == (struct advertise *) NULL) {
		error = ENODEV;
		goto out;
	}
	/* Get the root vnode of the resource */
	vp = ap->a_queue->rd_vnode;

	/* Deny a read/write mount request on a read-only resource */
	if ((ap->a_flags & A_RDONLY) && !(rwflag & RFS_RDONLY)) {
		error = EROFS;
		goto out;
	}
	ap->a_count++;

	/* see if client is authorized	*/
	if (ap->a_clist && !checkalist(ap->a_clist, sp->s_gdpp->token.t_uname)) {
		error = EACCES;
		goto out;
	}
	/* Give up if resource has been unadvertised */
	if (ap->a_flags & A_MINTER) {
		error = ENONET;
		goto out;
	}
	/*  allocate an entry in the srmount table, and fill it in  */
	if (error = alocsrm(vp, sp->s_gdpp->sysid, &smp))
		goto out;

	cl_mntindx = sp->s_mntindx; 	/* Get client mount index */
	sp->s_mntindx = smp - srmount;  /* Store server mount index */

	/* Create rd_user struct for root of mount point, for recovery */
	if (cr_rduser(ap->a_queue, sp) == (struct rd_user *) NULL) {
		error = ENOSPC;
		goto out;
	}

	smp->sr_sysid = sp->s_gdpp->sysid;
	smp->sr_rootvnode = vp;
	smp->sr_mntindx = cl_mntindx;
	smp->sr_flags = MINUSE;
	smp->sr_bcount = 0;
	smp->sr_slpcnt = 0;
	if (rwflag & RFS_RDONLY)
		smp->sr_flags |= MRDONLY;
	smp->sr_refcnt = 1;
	VN_HOLD(vp);
	*gift = ap->a_queue;
	sp->s_gdpp->mntcnt++;
	(*gift)->rd_refcnt++;

	/* Get mount point vnode attributes for response. */
	error = VOP_GETATTR(vp, &vattr, u.u_cred);
out:
	/* Make response message */
	make_response(sp, (mblk_t **) NULL, error, vp, &vattr, out_bp, outsize);

	msg_out = (struct response *)PTOMSG((*out_bp)->b_rptr);
	msg_out->rp_mntindx = smp - srmount;

	if (error) {
		/*
		 * If last user of resource and it has been unadvertised,
		 * free the advertise table entry and associated vnode
		 */
		if (ap && (--(ap->a_count) == 0) && (ap->a_flags & A_MINTER)) {
			ap->a_flags = A_FREE;
			VN_RELE(ap->a_queue->rd_vnode);
		}
	}
	DUPRINT3(DB_SERVE, "rs_srmount: result %d, vp %x\n", error, vp);
	return (error);
}


/*
 * Server side of RFS remote unmount
 */
/*ARGSUSED*/
rs_srumount(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request	*msg_in;
	struct	srmnt *smp = (struct srmnt *) NULL;
	int mntindx;
	extern int nsrmount;
	extern struct advertise *getadv();
	struct vnode *vp;
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	mntindx = msg_in->rq_mntindx;
	DUPRINT3(DB_SERVE, "rs_srumount: sp %x, mntindx %d\n", sp, mntindx);

	if (mntindx < 0 || mntindx >= nsrmount) {
		error = EINVAL;
		goto out;
	}
	/* Get server side mnt table entry and associated vnode for mnt point */
	smp = &srmount[mntindx];
	vp = smp->sr_rootvnode;

	/* Release mount table entry and associated resources */
	if (error = srumount(smp, vp))
		goto out;

	/* Release receive descriptor, associated circuit and vnode */
	del_rcvd(sp->s_rdp, sp);
	sp->s_gdpp->mntcnt--;
	VN_RELE(vp);
out:
	/* Make response message */
	make_response(sp, &in_bp, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Free the request buffer */
	if (in_bp)
		freemsg(in_bp);

	DUPRINT3(DB_SERVE, "rs_srumount: result %d, vp %x\n", error, vp);
	return (error);
}

/*
 * DCHUNK is the maximum number of bytes of directory entries the server
 * will return to the client, regardless of what is asked for.  The value
 * is chosen so that after XDR expansion it will still be comfortably less
 * than the maximum RFS message size (DATASIZE bytes). We can't currently
 * send more because of RFS heterogeneity bugs.
 */
#define	DCHUNK 512

/*ARGSUSED*/
rs_getdents(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register struct	response *msg_out;
	struct vnode *vp = (struct vnode *) NULL;
	long base, count, offset;
	int reqsize, chunksize;
	int error = 0;
	struct uio uio;
	struct iovec iov;
	caddr_t buf = (caddr_t) NULL;
	caddr_t bufp;
	int bytes_read;
	struct dirent *idp;
	struct rfs_dirent *odp;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT6(DB_SERVE, "rs_getdents: sp %x, offset %d, count %d, base %x, rd->vp %x\n", sp, msg_in->rq_offset, msg_in->rq_count, msg_in->rq_base, sp->s_rdp->rd_vnode);

	/*
	 * Get syscall params: base, count, offset and mode are implicit
	 * params supplied from client's file table entry
	 */
	base = msg_in->rq_base;
	offset = msg_in->rq_offset;
	reqsize = msg_in->rq_count;
	chunksize = reqsize > DCHUNK ? DCHUNK : reqsize;
	freemsg(in_bp);
	/* Get the vnode for the file */
	vp = sp->s_rdp->rd_vnode;

	/* Initialize uio for data copy to temp buffer */
	buf = (caddr_t)new_kmem_alloc((u_int)chunksize, KMEM_SLEEP);
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_seg = UIOSEG_KERNEL;
	uio.uio_offset = offset;
	uio.uio_resid = chunksize;
	iov.iov_len = chunksize;
	iov.iov_base = bufp = buf;

	/* Read data into buffer */
	error = VOP_READDIR(vp, &uio, u.u_cred);
	bytes_read = chunksize - uio.uio_resid;

	/* Get response message */
	make_response(sp, (mblk_t **) NULL, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/*
	 * Copy the directory entries into the response buffer, converting
	 * to RFS format
	 */
	msg_out = (struct response *) PTOMSG((*out_bp)->b_rptr);
	if (!error) {
		for (count=0,
		    idp = (struct dirent *) bufp,
		    odp = (struct rfs_dirent *) msg_out->rp_data;
		    count < bytes_read;
		    count += idp->d_reclen,
		    idp = (struct dirent *) ((char *) idp + idp->d_reclen),
		    odp = (struct rfs_dirent *)((char *)odp + odp->d_reclen)) {
			odp->d_ino = idp->d_fileno;
			odp->d_off = idp->d_off;
			odp->d_reclen =
			    (((unsigned) odp->d_name - (unsigned) odp) +
				idp->d_namlen + 1 + 3) & ~3;
			idp->d_name[idp->d_namlen] = '\0';
			(void) strcpy(odp->d_name, idp->d_name);
			DUPRINT4(DB_SERVE, " ino %d, reclen %d, name %s\n",
				odp->d_ino, odp->d_reclen, odp->d_name);
		}
		msg_out->rp_subyte = 0; 	/* Message contains data */
		msg_out->rp_count =
			(long) ((unsigned) odp -  (unsigned) msg_out->rp_data);
		msg_out->rp_rval = msg_out->rp_count;
		msg_out->rp_offset = uio.uio_offset;
		msg_out->rp_bufptr = base;
		*outsize = sizeof (struct response) - DATASIZE +
				msg_out->rp_count;
	}
	DUPRINT6(DB_SERVE, "rs_readdir: rslt %d, cnt %d, addr %x, rval %d, su %d\n",
	    error, msg_out->rp_count, (unsigned) msg_out->rp_bufptr,
	    msg_out->rp_rval, msg_out->rp_subyte);
	kmem_free((caddr_t)buf, (u_int)chunksize);
	return (error);
}

/*ARGSUSED*/
rs_read(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register struct	response *msg_out;
	struct vnode *vp = (struct vnode *) NULL;
	long mode, base, count, offset;
	int reqsize;
	int ioflag;
	int error = 0;
	struct uio uio;
	struct iovec iov;
	mblk_t *dbp = (mblk_t *) NULL;
	mblk_t *ack_bp = (mblk_t *) NULL;
	rcvd_t ackrd = (rcvd_t) NULL;
	struct vattr va;
	int msgsz;
	int no_sig = 1;
	int rs;
	int mand_chg = 0;
	int size;
	int raw_read = 0; /* Size of i/o requests for a raw device */
	int raw_off; /* Off into current raw read for this RFS resp. msg */
	int raw_ret; /* raw dev: #bytes of data ret'd in this RFS resp. msg */

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT6(DB_SERVE, "rs_read: sp %x, offset %d, count %d, mode %x, rd->vp %x\n", sp, msg_in->rq_offset, msg_in->rq_count, msg_in->rq_fmode, sp->s_rdp->rd_vnode);

	/*
	 * Get syscall params: base, count, offset and mode are implicit
	 * params supplied from client's file table entry
	 */
	base = msg_in->rq_base;
	offset = msg_in->rq_offset;
	reqsize = msg_in->rq_count;
	mode = msg_in->rq_fmode;
	ioflag = (mode & RFS_FAPPEND ? IO_APPEND : 0) |
			(mode & RFS_FSYNC ? IO_SYNC : 0);
	mode = RFSTOFFLAG(msg_in->rq_fmode);
	/* Get the vnode for the file */
	vp = sp->s_rdp->rd_vnode;

	/* Set client remote signal, if any */
	if (sp->s_op == DUREAD)
		setrsig(sp, in_bp);

	/* If file has changed to mand. lockable, inform client via NACK */
	if (!(msg_in->rq_flags & RQ_MNDLCK) &&
		(((struct message *)in_bp->b_rptr)->m_stat & VER1) &&
		mnd_lck(vp, u.u_cred)) {
		error = ENOMEM;
		mand_chg = 1;
		freemsg(in_bp);
		goto out;
	}
	freemsg(in_bp);

	/* Recover if system call is interrupted */
	if (setjmp(&u.u_qsave)) {
		error = EINTR;
		goto out;
	}
	/* Initialize uio for data copy from system buffer to response message */
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_seg = UIOSEG_KERNEL;
	uio.uio_offset = offset;
	uio.uio_fmode = mode;

	/*
	 * If necessary, setup for raw i/o. Allocate a user address space
	 * (if one isn't already there) so that the resulting physio()
	 * requests will work; data will be read into the user address space
	 * (physio() won't work to kernel address space) and copied back in.
	 * Also, choose the size in which reads will be done. Requests
	 * are handed to the device (via VOP_RDWR) with the size the client
	 * specified (unless they exceed the size of the segment allocated
	 * in the user's address space (segsize)). This is to preserve things
	 * such as tape block sizes.
	 */
	if (vp->v_type == VCHR) {
		if (sp->s_proc->p_as == NULL) {
			if ((sp->s_proc->p_as = rs_getas(segsize)) == NULL) {
				error = ENOMEM;
				goto out;
			}
		}
		raw_read = (reqsize <= segsize ? reqsize : DATASIZE);
		raw_off = raw_read + 1;
	}

	/* Read and send resp. back in DATASIZE chunks, as RFS requires */
	for (count = reqsize; ; count -= DATASIZE, base += DATASIZE) {
		/* Setup response message and copy params */
		dbp = salocbuf(sizeof (struct response), BPRI_MED);
		size = (count > DATASIZE ? DATASIZE : count);
		msg_out = (struct response *)PTOMSG(dbp->b_rptr);
		msg_out->rp_type = RESP_MSG;
		msg_out->rp_opcode = DUCOPYOUT;
		msg_out->rp_bufptr = base;
		msg_out->rp_bp = (long) dbp;
		msg_out->rp_subyte = 0; 	/* Message contains data */
		msg_out->rp_count = size;
		msg_out->rp_errno = 0;
		msgsz = sizeof (struct response) - DATASIZE + size;
		uio.uio_resid = size;
		if (vp->v_type == VFIFO)  /* FIFOs always expect 0 offset */
			uio.uio_offset = 0;
		iov.iov_len = size;
		iov.iov_base = msg_out->rp_data;

		/* Normal read, put next DATASIZE chunk in response message */
		if (!raw_read) {
			error = VOP_RDWR(vp, &uio, UIO_READ, ioflag, u.u_cred);
		} else { /* raw dev */
			/* If need next raw-size block, read it to user land */
			if (raw_off >= raw_read) {
				raw_off = 0;
				raw_read = MIN(count, raw_read);
				uio.uio_seg = UIOSEG_USER;
				uio.uio_iov = &iov;
				uio.uio_iovcnt = 1;
				uio.uio_offset = 0;
				iov.iov_base = SEGSTART;
				iov.iov_len = raw_read;
				uio.uio_resid = raw_read;
				error =
				 VOP_RDWR(vp, &uio, UIO_READ, ioflag, u.u_cred);
				if (error)
					goto out;
				raw_read -= uio.uio_resid;
			}
			/* copyin chunk from userland for next response msg */
			raw_ret = MIN(size, raw_read - raw_off);
			error =
			  copyin(SEGSTART + raw_off, msg_out->rp_data, 
				(u_int) raw_ret);
			if (error)
				goto out;
			raw_off += raw_ret;
			uio.uio_resid = size - raw_ret;
		}

		DUPRINT6(DB_SERVE, "rs_read: req %d, rd %d, count %d, ccnt %d, caddr %x\n", size, size - uio.uio_resid, count, sp->s_sdp->sd_copycnt,
		(unsigned) msg_out->rp_bufptr);

		srmount[sp->s_mntindx].sr_bcount +=
			(size - uio.uio_resid + RFS_BKSIZE/2) / RFS_BKSIZE;

		if (error || uio.uio_resid || count <= DATASIZE)
			goto out;

		/* Send back the data */
		sp->s_sdp->sd_copycnt++;
		if (sp->s_sdp->sd_copycnt < FILE_QSIZE) {
			msg_out->rp_copysync = 0;
			error = sndmsg(sp->s_sdp, dbp, msgsz, (rcvd_t) NULL);
			dbp = (mblk_t *) NULL;
			if (error)
				goto out;
		} else { /* Flow control: let client ACK before sending more */
			sp->s_sdp->sd_copycnt = 0;
			msg_out->rp_copysync = 1; /* Signals client to ACK */
			/* Get rd to receive client ACK on */
			if (!ackrd && !(ackrd = cr_rcvd(FILE_QSIZE, SPECIFIC))) {
				error = ENOMEM;
				goto out;
			}
			ackrd->rd_sd = sp->s_sdp;
			error = sndmsg(sp->s_sdp, dbp, msgsz, ackrd);
			dbp = (mblk_t *) NULL;
			if (error)
				goto out;
			/* Wait for the ACK */
			if (error = de_queue(ackrd, &ack_bp, (sndd_t) NULL, &rs,
				&no_sig, (sndd_t) NULL))
				goto out;
			DUPRINT3(DB_SERVE, "rs_read: ack, opcode %d, ccnt %d\n",
			((struct response *)PTOMSG(ack_bp->b_rptr))->rp_opcode,
			sp->s_sdp->sd_copycnt);
			freemsg(ack_bp);
		}
	}
out:
	chkrsig(sp, &error);

	/* Return the file size */
	if (!error)
		error = VOP_GETATTR(vp, &va, u.u_cred);

	/* Last chunk is returned as response message */
	if (dbp)
		*out_bp = dbp;
	else
		make_response(sp, (mblk_t **) NULL, error, (struct vnode *) NULL,
				(struct vattr *) NULL, out_bp, outsize);

	msg_out = (struct response *) PTOMSG((*out_bp)->b_rptr);
	msg_out->rp_opcode = sp->s_op;
	msg_out->rp_sig = SIGTORFS(GET_SIGS(sp->s_proc));
	msg_out->rp_sysid = sp->s_sysid;
	msg_out->rp_errno = ERRTORFS(error);
	msg_out->rp_rval = count - size + uio.uio_resid;

	if (mand_chg) {
		msg_out->rp_type = NACK_MSG;
		msg_out->rp_cache |= RP_MNDLCK;
	}
	if (error) {
		msg_out->rp_subyte = 1;
		msg_out->rp_count = 0;
		*outsize = sizeof (struct response) - DATASIZE;
	} else {
		msg_out->rp_isize = va.va_size;
		msg_out->rp_count = size - uio.uio_resid;
		msg_out->rp_subyte = size - uio.uio_resid ? 0 : 1;
		*outsize = sizeof (struct response) - DATASIZE +
				msg_out->rp_count;
	}
	/* Cleanup */
	sp->s_sdp->sd_copycnt = 0;
	if (ackrd)
		free_rcvd(ackrd);

	DUPRINT6(DB_SERVE, "rs_read: rslt %d, cnt %d, addr %x, rval %d, su %d\n",
	    error, msg_out->rp_count, (unsigned) msg_out->rp_bufptr,
	    msg_out->rp_rval, msg_out->rp_subyte);
	return (error);
}


/*ARGSUSED*/
rs_write(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register struct	response *msg_out;
	register struct	response *data_msg;
	struct vnode *vp = (struct vnode *) NULL;
	long mode, base, count, offset;
	int reqsize;
	int ioflag;
	int error = 0;
	struct uio uio;
	struct iovec iov;
	mblk_t *data_bp = (mblk_t *) NULL;
	mblk_t *go_bp = (mblk_t *) NULL;
	rcvd_t rd = (rcvd_t) NULL;
	struct vattr va;
	int size, msgsz;
	int no_sig = 1;
	int rs;
	int mand_chg = 0;
	int raw_write = 0; /* Size of i/o requests for a raw device */
	int raw_off; 	/* Off into currently accumulating raw write */


	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT6(DB_SERVE, "rs_write: sp %x, offset %d, count %d, mode %x, rd->vp %x\n", sp, msg_in->rq_offset, msg_in->rq_count, msg_in->rq_fmode, sp->s_rdp->rd_vnode);

	/*
	 * Get syscall params: base, count, offset and mode are implicit
	 * params supplied from client's file table entry
	 */
	base = msg_in->rq_base;
	offset = msg_in->rq_offset;
	reqsize = msg_in->rq_count;
	mode = msg_in->rq_fmode;
	ioflag = (mode & RFS_FAPPEND ? IO_APPEND : 0) |
			(mode & RFS_FSYNC ? IO_SYNC : 0);
	mode = RFSTOFFLAG(msg_in->rq_fmode);
	/* Get the vnode for the file */
	vp = sp->s_rdp->rd_vnode;

	/* Writing is not allowed to symbolic links */
	if (vp->v_type == VLNK) {
		error = EINVAL;
		goto out;
	}
	/* ... or to read-only filesystems */
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/* Set client remote signal, if any */
	if (sp->s_op == DUWRITE)
		setrsig(sp, in_bp);

	/* If file has changed to mand. lockable, inform client via NACK */
	if (!(msg_in->rq_flags & RQ_MNDLCK) &&
		(((struct message *)in_bp->b_rptr)->m_stat & VER1) &&
		mnd_lck(vp, u.u_cred)) {
		error = ENOMEM;
		mand_chg = 1;
		freemsg(in_bp);
		goto out;
	}
	/* Recover if system call is interrupted */
	if (setjmp(&u.u_qsave)) {
		error = EINTR;
		goto out;
	}
	/* For append write, get end of file to append to. */
	if (ioflag & IO_APPEND || (offset < 0 && sp->s_op == DUWRITEI)) {
		if (error = VOP_GETATTR(vp, &va, u.u_cred))
			goto out;
		offset = va.va_size;
	}
	/* Create rd on which to receive rest of data */
	if ((rd = cr_rcvd(FILE_QSIZE, SPECIFIC)) == (rcvd_t) NULL) {
		error = ENOMEM;
		goto out;
	}
	rd->rd_sd = sp->s_sdp;
	rd->rd_qsize = SIGQSIZE;

	/* Initialize uio for data copy from network to system buffers */
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_seg = UIOSEG_KERNEL;
	uio.uio_offset = offset;
	uio.uio_fmode = mode;

	/*
	 * If necessary, setup for raw i/o. Allocate a user address space
	 * (if one isn't already there) for doing physio() to. Requests
	 * are handed to the device (via VOP_RDWR) with the size the client
	 * specified (unless they exceed the size of the segment allocated
	 * in the user's address space (segsize)). This is to preserve things
	 * such as tape block sizes.
	 */
	if (vp->v_type == VCHR) {
		if (sp->s_proc->p_as == NULL) {
			if ((sp->s_proc->p_as = rs_getas(segsize)) == NULL) {
				error = ENOMEM;
				goto out;
			}
		}
		raw_write = (reqsize <= segsize ? reqsize : DATASIZE);
		raw_off = 0;
	}

	/* Write data, send go-ahead and get more data, in DATASIZE chunks */
	for (count = reqsize, data_bp = in_bp, size = msg_in->rq_prewrite,
	    iov.iov_base = msg_in->rq_data; ; ) {
		/* Do the write */
		uio.uio_resid = size;
		iov.iov_len = size;
		if (vp->v_type == VFIFO)  /* FIFOs always expect 0 offset */
			uio.uio_offset = 0;

		/* Normal write: write the chunk received in this msg */
		if (!raw_write) {
			error = VOP_RDWR(vp, &uio, UIO_WRITE, ioflag, u.u_cred);
		} else { /* raw write */
			/* copyout this msg's data chunk to userland */
			ASSERT(raw_off + size <= raw_write);
			error =
				copyout(iov.iov_base, SEGSTART + raw_off, 
					(u_int) size);
			if (error) {
				freemsg(data_bp);
				data_bp = (mblk_t *) NULL;
				goto out;
			}
			DUPRINT4(DB_SERVE, "rs_write: raw copyout %d, off %d, write size %d\n",
				size, raw_off, raw_write);
			raw_off += size;
			uio.uio_resid = 0;
			/* If got next raw-size block, write it */
			if (raw_off >= raw_write) {
				uio.uio_seg = UIOSEG_USER;
				uio.uio_iov = &iov;
				uio.uio_iovcnt = 1;
				uio.uio_offset = 0;
				iov.iov_base = SEGSTART;
				iov.iov_len = raw_write;
				uio.uio_resid = raw_write;
				error =
				 VOP_RDWR(vp, &uio, UIO_WRITE, ioflag, u.u_cred);
				DUPRINT4(DB_SERVE, "rs_write: raw write: raw_write %d, wrote %d, error %d\n",
				   raw_write, raw_write - uio.uio_resid, error);
				raw_off = 0;
				raw_write = MIN(count - size, raw_write);
			}
		}

		freemsg(data_bp);
		data_bp = (mblk_t *) NULL;
		if (error)
			goto out;

		srmount[sp->s_mntindx].sr_bcount +=
			(size - uio.uio_resid + RFS_BKSIZE/2) / RFS_BKSIZE;

		if (uio.uio_resid) {
			count -= (size - uio.uio_resid);
			base += (size - uio.uio_resid);
			goto out;
		}
		count -= size; base += size;
		DUPRINT4(DB_SERVE, "rs_write: wrote %d, rem %d, newbase %x\n",
			size, count, (unsigned) base);
		if (count <= 0)
			goto out;

		/* Send back go-ahead message */
		go_bp = salocbuf(sizeof (struct response), BPRI_MED);
		size = (count > DATASIZE ? DATASIZE : count);
		msg_out = (struct response *)PTOMSG(go_bp->b_rptr);
		msg_out->rp_type = RESP_MSG;
		msg_out->rp_opcode = DUCOPYIN;
		msg_out->rp_bufptr = base;
		msg_out->rp_bp = (long) go_bp;
		msg_out->rp_count = size;	/* Send back this much */
		msg_out->rp_errno = 0;
		msgsz = sizeof (struct response) - DATASIZE;
		if (error = sndmsg(sp->s_sdp, go_bp, msgsz, rd))
			goto out;

		/* Get next chunk of data */
		if (error = de_queue(rd, &data_bp, (sndd_t) NULL, &rs, &no_sig,
			(sndd_t) NULL))
			goto out;
		data_msg = (struct response *)PTOMSG(data_bp->b_rptr);
		if (data_msg->rp_errno) {
			error = RFSTOERR(data_msg->rp_errno);
			goto out;
		}
		ASSERT(data_msg->rp_count == size); /* Expect size we asked for */
		iov.iov_base = data_msg->rp_data;
	}
out:
	chkrsig(sp, &error);

	/* If synchronous specified, flush the write to disk */
	if (ioflag & IO_SYNC && !error)
		error = VOP_FSYNC(vp, u.u_cred);

	/* Return the file size */
	if (!error)
		error = VOP_GETATTR(vp, &va, u.u_cred);

	make_response(sp, (mblk_t **) NULL, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);
	msg_out = (struct response *) PTOMSG((*out_bp)->b_rptr);
	if (mand_chg) {
		msg_out->rp_type = NACK_MSG;
		msg_out->rp_cache |= RP_MNDLCK;
	}
	if (ioflag & IO_APPEND)
		msg_out->rp_isize = offset;
	else
		msg_out->rp_isize = va.va_size;
	msg_out->rp_rval = count;

	if (rd)
		free_rcvd(rd);
	if (data_bp)
		freemsg(data_bp);

	DUPRINT3(DB_SERVE, "rs_write: rslt %d, rval %d\n",
		error, msg_out->rp_rval);
	return (error);
}

rs_coredump(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	char name[20];
	struct vnode *vp = (struct vnode *) NULL;
	int error = 0;
	struct vattr vattr;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT3(DB_SERVE, "rs_coredump: sp %x, rd->vp %x\n",
		sp, sp->s_rdp->rd_vnode);

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Set attributes of the file */
	vattr_null(&vattr);
	vattr.va_type = VREG;
	vattr.va_mode = 0666 & ~msg_in->rq_cmask;
	(void) strcpy(name, "core");

	/* Create the file */
	error = rs_vncreate(name, UIOSEG_KERNEL, &vattr,
				NONEXCL, VWRITE, sp, &vp);
	if (error != 0)
		goto out;

	/* Zero the file */
	vattr_null(&vattr);
	vattr.va_size = 0;
	if (error = VOP_SETATTR(vp, &vattr, u.u_cred))
		goto out;

	/* Get attributes of new vnode to return to client */
	if (error = VOP_GETATTR(vp, &vattr, u.u_cred))
		goto out;

	if (vattr.va_nlink != 1) {
		error = EFAULT;
		goto out;
	}
	/* Make a receive descriptor for client requests on new vnode */
	if (error = make_gift(vp, FILE_QSIZE, sp, gift))
		goto out;

	srmount[sp->s_mntindx].sr_refcnt++;  /* New ref on this mnt point */
out:
	/* Make response message */
	make_response(sp, &in_bp, error, vp, &vattr, out_bp, outsize);

	/* Free the request buffer (and the vnode if error) */
	if (in_bp)
		freemsg(in_bp);
	if (error) {
		if (vp)
			VN_RELE(vp);
	}
	DUPRINT4(DB_SERVE, "rs_coredump: result %d, name %s, vp %x\n",
					error, name, vp);
	return (error);
}

/*ARGSUSED*/
rs_iput(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request	*msg_in;
	struct	srmnt *smp = (struct srmnt *) NULL;
	int mntindx;
	extern int nsrmount;
	struct vnode *vp;
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	mntindx = msg_in->rq_mntindx;
	vp = sp->s_rdp->rd_vnode;
	DUPRINT3(DB_SERVE, "rs_iput: sp %x, vnode %x\n", sp, vp);

	if (mntindx < 0 || mntindx >= nsrmount) {
		error = EINVAL;
		goto out;
	}
	/* Release the vnode and associated resources */
	smp = &srmount[mntindx];
	--smp->sr_refcnt;
	if (smp->sr_refcnt == 0) { /* Means erroneous client DUIPUT */
		printf("serve DUIPUT: free srmount entry\n");
		smp->sr_flags = MFREE;
	}
	del_rcvd(sp->s_rdp, sp);
	VN_RELE(vp);
out:
	/* Make response message */
	make_response(sp, &in_bp, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Free the request buffer */
	if (in_bp)
		freemsg(in_bp);

	DUPRINT2(DB_SERVE, "rs_iput: result %d\n", error);
	return (error);
}

/*
 * Update a remote inode with the current time. VOP_SETATTR accomplishes this.
 * For 4.2 filesystem by calling iupdat(). For NFS by calling VOP_SETATTR() on
 * server. However, in general VOP_SETATTR does not make a requirement that the
 * attributes be flushed to stable storage as does FS_IUPDAT.
 */
/*ARGSUSED*/
rs_iupdate(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	struct vnode *vp;
	struct vattr va;
	extern  struct timeval time;
	int error = 0;

	freemsg(in_bp);
	vp = sp->s_rdp->rd_vnode;
	DUPRINT3(DB_SERVE, "rs_iupdate: sp %x, vnode %x\n", sp, vp);

	/* Update the vnode with the current time */
	vattr_null(&va);
	va.va_atime = time;
	va.va_mtime = time;
	error = VOP_SETATTR(vp, &va, u.u_cred);

	/* Make response message */
	make_response(sp, (mblk_t **) NULL, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	DUPRINT2(DB_SERVE, "rs_iupdate: result %d\n", error);
	return (error);
}

/*
 * Remote file system flush. Leave this a no-op for now since, 1) AT&T doesn't
 * really do the appropriate thing either (they just flush the super-block),
 * 2) if you had many clients mounting the same resource and each doing a
 * sync() every 30 seconds, this would put a non-trivial load on the server.
 * It's not clear that clients should be allowed to do remote sync's anyways.
 * It's really a server responsibility.
 */
/*ARGSUSED*/
rs_uupdate(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	struct vnode *vp = (struct vnode *) NULL;
	int error = 0;

	DUPRINT3(DB_SERVE, "rs_uupdate: sp %x, rd->vp %x\n", sp,
			sp->s_rdp->rd_vnode);

	freemsg(in_bp);
	vp = sp->s_rdp->rd_vnode;

	/* Make response message */
	make_response(sp, (mblk_t **) NULL, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	DUPRINT3(DB_SERVE, "rs_uupdate: result %d, vp %x\n",
			error, vp);
	return (error);
}

/*
 * RFS syscalls come here which take a file pathname, but where the operation
 * is only allowed on files local to the client. In this case the server has
 * to evaluate the pathname because it may contain a ".." and eventually be
 * resolved on the client. If the lookup returns with an EDOTDOT, then the
 * remaining pathname is returned to the client;  if it's successfully resolved
 * on the server it's an EREMOTE error. Currently, the following syscalls use
 * this:
 *  DUSYSACCT, DUADVERTISE, DUUNADVERTISE,
 *  DUMOUNT, DUUMOUNT, DURMOUNT, DURUMOUNT
 */
/*ARGSUSED*/
rs_clientop(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	register struct	response *msg_out;
	register char *name;
	struct vnode *vp = (struct vnode *) NULL;
	int error = 0;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT4(DB_SERVE, "rs_clientop: sp %x, name %s, rd->vp %x\n",
		sp, msg_in->rq_data, sp->s_rdp->rd_vnode);

	/* Get file name and mode change from request message */
	name = msg_in->rq_data;

	/* Set the current working and root directories */
	set_dir(sp->s_rdp, msg_in->rq_rrdir);

	/* Lookup the file */
	error = rs_lookupname(name, UIOSEG_KERNEL, NO_FOLLOW, sp,
		(struct vnode **) NULL, &vp);

	/* Make response message */
	make_response(sp, &in_bp, error, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);

	/* Free the request buffer, and the vnode */
	if (in_bp)
		freemsg(in_bp);
	if (vp)
		VN_RELE(vp);

	/* Pathname was resolved on server: error */
	if (!error) {
		msg_out = (struct response *) PTOMSG((*out_bp)->b_rptr);
		msg_out->rp_errno = RFS_EREMOTE;
		error = EREMOTE;
	}

	DUPRINT4(DB_SERVE, "rs_clientop: result %d, name %s, vp %x\n",
			error, name, vp);
	return (error);
}

/*
 * Handle client remote signal:
 * If there's an active server handling the signalling client's request,
 * signal that server.
 * Else, look for a matching client request awaiting service and set the signal
 * bit in the request. This will cause the server process who handles
 * the request to signal itself.
 */
/*ARGSUSED*/
rs_rsignal(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	register struct	request *msg_in;
	int cl_pid;	/* Process id of signalling client */
	int cl_sysid;	/* Machine id of signalling client */
	rcvd_t rdp;	/* rd of signalling client's original request */
	int s;
	struct serv_proc *tp;
	mblk_t *bp;
	extern mblk_t *chkrdq();
	struct message *msig;

	msg_in = (struct request *) PTOMSG(in_bp->b_rptr);
	DUPRINT5(DB_SERVE, "rs_rsignal: sp %x, cl sysid %x, srv sysid %x, epid %d, \n",
		sp, msg_in->rq_sysid, sp->s_sysid, msg_in->rq_pid);

	/* Get pid, sysid and rd of signalling client process from message */
	cl_pid = msg_in->rq_pid;
	cl_sysid = msg_in->rq_sysid;
	rdp = inxtord(((struct message *)in_bp->b_rptr)->m_dest);
	freemsg(in_bp);

	/* Look for currently active server serving signalling client */
	for (tp = s_active.sl_head;
		tp != (struct serv_proc *) NULL; tp = tp->s_rlink) {
		if (tp == sp)
			continue;
		if (tp->s_epid == cl_pid && tp->s_sysid == sp->s_sysid)
			break;
	}

	/*
	 * If found, signal server; else set signal bit in matching client
	 * request awaiting service (if any)
	 */
	if (tp)
		psignal(tp->s_proc, SIGTERM);
	else {
		s = splrf();
		sp->s_sdp->sd_srvproc = (struct proc *) NULL;
		if ((bp = chkrdq(rdp, (long)cl_pid, (long)cl_sysid)) != NULL) {
			msig = (struct message *)bp->b_rptr;
			msig->m_stat |= SIGNAL;
			enque(&rdp->rd_rcvdq, bp);
			rdp->rd_qcnt++;
			add_to_msgs_list(rdp);
		}
		(void) splx(s);
	}
	/* No response message for remote signal request */
	*out_bp = (mblk_t *) NULL;

	DUPRINT2(DB_SERVE, "rs_rsignal: %s\n", tp ?
		"signalled active server" : "set signal bit in pending requst");
	return (0);
}

/*
 * Invalid op -- shouldn't get these, but just in case return a response to
 * the client.
 */
/*ARGSUSED*/
rs_badop(sp, in_bp, out_bp, outsize, gift)
	struct serv_proc *sp;
	mblk_t *in_bp, **out_bp;
	int *outsize;
	rcvd_t *gift;
{
	DUPRINT4(DB_SERVE, "rs_badop: sp %x, op %d, rd->vp %x\n",
		sp, sp->s_op, sp->s_rdp->rd_vnode);
	freemsg(in_bp);

	/* Make response message */
	make_response(sp, (mblk_t **) NULL, EINVAL, (struct vnode *) NULL,
			(struct vattr *) NULL, out_bp, outsize);
	return (-1);
}

/*
 * Get user virtual address space for an rfs server process. Used for doing
 * physio() on raw devices,  which doesn't work with data to be moved to/from
 * kernel address space. The address space is created and a zfod() segment
 * of size "size" placed in it. Note that this reserves swap space.
 */
struct as *
rs_getas(size)
	u_int size;
{
	struct as *as;
	int error;

	/* Allocate the address space */
	as = as_alloc();
	if (as == NULL)
		goto skip;

	/* Create a zfod segment in it */
	error = as_map(as, SEGSTART, size, segvn_create, zfod_argsp);

	if (error) {
		as_free(as);
		as = NULL;
	}
skip:
	/* Sanity checks */
	if (as == NULL || as->a_segs == NULL ||
	    as->a_segs->s_next != as->a_segs ||
	    as->a_segs->s_base != SEGSTART) {
		printf("rfs_serve: bad as!\n");
	}
	return (as);
}
