/*	@(#)rfs_syscalls.c 1.1 92/07/30 SMI 	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* 
 * RFS system call routines. Each routine implements an RFS remote system
 * call, sending the message and getting the response. Inputs and outputs
 * of the routines are in a form understood by RFS, i.e., these routines
 * contain no knowledge of vnodes.
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
#include <sys/fcntl.h>
#include <nfs/nfs.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/debug.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/pathname.h>
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
#include <rfs/hetero.h>
#include <rfs/idtab.h>

extern struct vnodeops rfs_vnodeops;
extern sndd_t cr_sndd();
extern char *strcpy();

int
du_access(name, out_port, cred, mode)
char *name;
sndd_t out_port;
struct ucred *cred;
u_short mode;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	int retry = 0;
	int type;

	DUPRINT4(DB_SYSCALL, "du_access: name %s, sd %x, mode %o\n", 
		 name, out_port, mode);

        out_port->sd_refcnt++;  /* rumount will not succeed if cnt > 1 */
resend:
	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	msg_out->rq_cmask = DEF_CMASK;
	msg_out->rq_rrdir = 0;

	msg_out->rq_opcode = DUACCESS;
        msg_out->rq_fmode = (long) ((unsigned) mode);
	(void) strcpy(msg_out->rq_data, name);
	msz = sizeof(struct request) - DATASIZE + strlen(msg_out->rq_data) + 1;

	/* send the message and wait for the response */
	if (error = 
            rsc(out_port, out_bp, msz, &in_bp, (sndd_t)NULL, (struct uio *)NULL))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 

	error = msg_in->rp_errno;
	type = msg_in->rp_type;
	freemsg(in_bp);
	if (error == ENOMEM && type == NACK_MSG && retry++ < RFS_RETRY) {
		DUPRINT2(DB_SYSCALL, "du_access: retry %d\n", retry);
		goto resend;
	}
out:
	DUPRINT2(DB_SYSCALL, "du_access: result %d\n", error);
        out_port->sd_refcnt--;
        return (error);
}


int
du_chmod(name, out_port, cred, mode)
char *name;
sndd_t out_port;
struct ucred *cred;
u_short mode;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	int retry = 0;
	int type;

	DUPRINT4(DB_SYSCALL, "du_chmod: name %s, sd %x, mode %o\n", 
		 name, out_port, mode);

        out_port->sd_refcnt++;  /* rumount will not succeed if cnt > 1 */
resend:
	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	msg_out->rq_cmask = DEF_CMASK;
	msg_out->rq_rrdir = 0;

	msg_out->rq_opcode = DUCHMOD;
        msg_out->rq_fmode = (long) ((unsigned) mode);
	(void) strcpy(msg_out->rq_data, name);
	msz = sizeof(struct request) - DATASIZE + strlen(msg_out->rq_data) + 1;

	/* send the message and wait for the response */
	if (error = 
            rsc(out_port, out_bp, msz, &in_bp, (sndd_t)NULL, (struct uio *)NULL))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	error = msg_in->rp_errno;
	type = msg_in->rp_type;
	freemsg(in_bp);
	if (error == ENOMEM && type == NACK_MSG && retry++ < RFS_RETRY) {
		DUPRINT2(DB_SYSCALL, "du_chmod: retry %d\n", retry);
		goto resend;
	}
out:
	DUPRINT2(DB_SYSCALL, "du_chmod: result %d\n", error);
        out_port->sd_refcnt--;
        return (error);
}


int
du_chown(name, out_port, cred, newuid, newgid)
char *name;
sndd_t out_port;
struct ucred *cred;
uid_t newuid;
gid_t newgid;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	int retry = 0;
	int type;

	DUPRINT5(DB_SYSCALL, "du_chown: name %s, sd %x, nuid %d, ngid %d\n", 
		 name, out_port, newuid, newgid);

        out_port->sd_refcnt++;  /* rumount will not succeed if cnt > 1 */
resend:
	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	msg_out->rq_cmask = DEF_CMASK;
	msg_out->rq_rrdir = 0;

	msg_out->rq_opcode = DUCHOWN;
        msg_out->rq_newuid = (long) ((unsigned) newuid);
        msg_out->rq_newgid = (long) ((unsigned) newgid);
	(void) strcpy(msg_out->rq_data, name);
	msz = sizeof(struct request) - DATASIZE + strlen(msg_out->rq_data) + 1;

	/* send the message and wait for the response */
	if (error = 
            rsc(out_port, out_bp, msz, &in_bp, (sndd_t)NULL, (struct uio *)NULL))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	error = msg_in->rp_errno;
	type = msg_in->rp_type;
	freemsg(in_bp);
	if (error == ENOMEM && type == NACK_MSG && retry++ < RFS_RETRY) {
		DUPRINT2(DB_SYSCALL, "du_chown: retry %d\n", retry);
		goto resend;
	}
out:
	DUPRINT2(DB_SYSCALL, "du_chown: result %d\n", error);
        out_port->sd_refcnt--;
        return (error);
}


int
du_stat(name, out_port, cred, ret_sbp)
char *name;
sndd_t out_port;
struct ucred *cred;
struct rfs_stat *ret_sbp;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	struct rfs_stat *rsbp;
	int msz;
	int error = 0;
	int retry = 0;
	int type;

	DUPRINT3(DB_SYSCALL, "du_stat: name %s, sd %x\n", name, out_port);

        out_port->sd_refcnt++;  /* rumount will not succeed if cnt > 1 */
resend:
	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	msg_out->rq_cmask = DEF_CMASK;
	msg_out->rq_rrdir = 0;

	msg_out->rq_opcode = DUSTAT;
	msg_out->rq_bufptr = (long) ret_sbp;
	(void) strcpy(msg_out->rq_data, name);
	msz = sizeof(struct request) - DATASIZE + strlen(msg_out->rq_data) + 1;

	/* send the message and wait for the response */
	if (error = 
            rsc(out_port, out_bp, msz, &in_bp, (sndd_t)NULL, (struct uio *)NULL))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	rsbp = (struct rfs_stat *) msg_in->rp_data;
	error = msg_in->rp_errno;
	type = msg_in->rp_type;
	if (error == ENOMEM && type == NACK_MSG && retry++ < RFS_RETRY) {
		DUPRINT2(DB_SYSCALL, "du_stat: retry %d\n", retry);
		freemsg(in_bp);
		goto resend;
	} else if (error) {
		freemsg(in_bp);
		goto out;
	}
	bcopy((caddr_t)rsbp, (caddr_t)ret_sbp, (u_int)sizeof(struct rfs_stat));
	freemsg(in_bp);
out:
	DUPRINT6(DB_SYSCALL, "du_stat: errno %d, mode %o, dev %x, size %d, uid %d\n", 
	   error, error ? 0: (unsigned) ret_sbp->st_mode, 
	   error ? 0: (unsigned) ret_sbp->st_dev, 
	   error ? 0: (unsigned) ret_sbp->st_size, error ? 0: ret_sbp->st_uid);
        out_port->sd_refcnt--;
        return (error);
}


int
du_mkdir(name, out_port, cred, mode) 
char *name;
sndd_t out_port;
struct ucred *cred;
u_short mode;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	int retry = 0;
	int type;

	DUPRINT4(DB_SYSCALL, "du_mkdir: name %s, sd %x, mode %o\n", 
		 name, out_port, mode);

        out_port->sd_refcnt++;  /* rumount will not succeed if cnt > 1 */
resend:
	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	msg_out->rq_cmask = DEF_CMASK;
	msg_out->rq_rrdir = 0;

	msg_out->rq_opcode = DUMKDIR;
        msg_out->rq_fmode = (long) ((unsigned) mode);
	(void) strcpy(msg_out->rq_data, name);
	msz = sizeof(struct request) - DATASIZE + strlen(msg_out->rq_data) + 1;

	/* send the message and wait for the response */
	if (error = 
            rsc(out_port, out_bp, msz, &in_bp, (sndd_t)NULL, (struct uio *)NULL))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	error = msg_in->rp_errno;
	type = msg_in->rp_type;
	freemsg(in_bp);
	if (error == ENOMEM && type == NACK_MSG && retry++ < RFS_RETRY) {
		DUPRINT2(DB_SYSCALL, "du_mkdir: retry %d\n", retry);
		goto resend;
	}
out:
	DUPRINT2(DB_SYSCALL, "du_mkdir: result %d\n", error);
        out_port->sd_refcnt--;
        return (error);
}


int
du_rmdir(name, out_port, cred)
char *name;
sndd_t out_port;
struct ucred *cred;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	int retry = 0;
	int type;

	DUPRINT3(DB_SYSCALL, "du_rmdir: name %s, sd %x\n", name, out_port);

        out_port->sd_refcnt++;  /* rumount will not succeed if cnt > 1 */
resend:
	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	msg_out->rq_cmask = DEF_CMASK;
	msg_out->rq_rrdir = 0;

	msg_out->rq_opcode = DURMDIR;
	(void) strcpy(msg_out->rq_data, name);
	msz = sizeof(struct request) - DATASIZE + strlen(msg_out->rq_data) + 1;

	/* send the message and wait for the response */
	if (error = 
            rsc(out_port, out_bp, msz, &in_bp, (sndd_t)NULL, (struct uio *)NULL))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	error = msg_in->rp_errno;
	type = msg_in->rp_type;
	freemsg(in_bp);
	if (error == ENOMEM && type == NACK_MSG && retry++ < RFS_RETRY) {
		DUPRINT2(DB_SYSCALL, "du_rmdir: retry %d\n", retry);
		goto resend;
	}
out:
	DUPRINT2(DB_SYSCALL, "du_rmdir: result %d\n", error);
        out_port->sd_refcnt--;
        return (error);
}


/* 
 * In addition to its normal use in the vn_link() and vn_rename() ops, 
 * this RFS call is used by the client to perform vn_lookup() operations. 
 * It supports multi-component lookup (including EDOTDOT), and thus takes 
 * a pathname struct rather than just a component name as argument. An 
 * EDOTDOT error on return signifies that the pathname tried to back out 
 * of the server mount point. In case of error, the uneaten pathname 
 * is returned in the pathname structure.
 */
int
du_link(pnp, out_port, cred, gift, nip)
struct pathname *pnp;
sndd_t out_port;
struct ucred *cred;
sndd_t *gift;
struct nodeinfo *nip;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	int retry = 0;
	int type;

	DUPRINT3(DB_SYSCALL, "du_link: name %s, sd %x,\n", pn_getpath(pnp), 
		out_port);
        out_port->sd_refcnt++;  /* rumount will not succeed if cnt > 1 */
resend:
	/* Contains sd for remote inode of file on syscall return */
        if ((*gift = cr_sndd()) == (sndd_t) NULL)
                return(ENOMEM);

	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	msg_out->rq_cmask = DEF_CMASK;
	msg_out->rq_rrdir = 0;

	msg_out->rq_opcode = DULINK;
	(void) strcpy(msg_out->rq_data, pn_getpath(pnp));
	msz = sizeof(struct request) - DATASIZE + strlen(msg_out->rq_data) + 1;

	/* send the message and wait for the response */
	if (error = 
            rsc(out_port, out_bp, msz, &in_bp, *gift, (struct uio *)NULL))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	error = msg_in->rp_errno;
	type = msg_in->rp_type;

	if (error == ENOMEM && type == NACK_MSG && retry++ < RFS_RETRY) {
		DUPRINT2(DB_SYSCALL, "du_link: retry %d\n", retry);
		freemsg(in_bp);
                free_sndd(*gift);
		*gift = (sndd_t) NULL;
		goto resend;
	/* Backed out of serve mnt point, return remaining path */
	} else if (error == EDOTDOT) {
		(void) pn_set(pnp, msg_in->rp_data);
		freemsg(in_bp);
		goto out;
	} else if (error) {
		freemsg(in_bp);
		goto out;
	}
       	nip->rni_uid = gluid(GDP((*gift)->sd_queue),(ushort)msg_in->rp_uid);
       	nip->rni_gid = glgid(GDP((*gift)->sd_queue),(ushort)msg_in->rp_gid);
       	nip->rni_nlink = msg_in->rp_nlink;
       	nip->rni_size = msg_in->rp_size;
       	nip->rni_ftype = msg_in->rp_ftype;
	(*gift)->sd_mntindx = msg_in->rp_mntindx;
	(*gift)->sd_fhandle = msg_in->rp_fhandle;
	freemsg(in_bp);
out:
	DUPRINT2(DB_SYSCALL, "du_link: result %d\n", error);
        if (error) {
                free_sndd(*gift);
		*gift = (sndd_t) NULL;
	}
        out_port->sd_refcnt--;
        return (error);
}

int
du_link1(link_sd, name, out_port, cred)
sndd_t link_sd;
char *name;
sndd_t out_port;
struct ucred *cred;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	int retry = 0;
	int type;

	DUPRINT4(DB_SYSCALL, "du_link1: link_sd %x, name %s, sd %x,\n", 
		 link_sd, name, out_port);

        out_port->sd_refcnt++;  /* rumount will not succeed if cnt > 1 */
resend:
	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	msg_out->rq_cmask = DEF_CMASK;
	msg_out->rq_rrdir = 0;

	msg_out->rq_opcode = DULINK1;
	msg_out->rq_link = link_sd->sd_sindex;
	(void) strcpy(msg_out->rq_data, name);
	msz = sizeof(struct request) - DATASIZE + strlen(msg_out->rq_data) + 1;

	/* send the message and wait for the response */
	if (error = 
            rsc(out_port, out_bp, msz, &in_bp, (sndd_t)NULL, (struct uio *)NULL))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	error = msg_in->rp_errno;
	type = msg_in->rp_type;
	freemsg(in_bp);
	if (error == ENOMEM && type == NACK_MSG && retry++ < RFS_RETRY) {
		DUPRINT2(DB_SYSCALL, "du_link1: retry %d\n", retry);
		goto resend;
	}
out:
	DUPRINT2(DB_SYSCALL, "du_link1: result %d\n", error);
        out_port->sd_refcnt--;
        return (error);
}

int
du_unlink(name, out_port, cred)
char *name;
sndd_t out_port;
struct ucred *cred;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	int retry = 0;
	int type;

	DUPRINT3(DB_SYSCALL, "du_unlink: name %s, sd %x\n", name, out_port);

        out_port->sd_refcnt++;  /* rumount will not succeed if cnt > 1 */
resend:
	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	msg_out->rq_cmask = DEF_CMASK;
	msg_out->rq_rrdir = 0;

	msg_out->rq_opcode = DUUNLINK;
	(void) strcpy(msg_out->rq_data, name);
	msz = sizeof(struct request) - DATASIZE + strlen(msg_out->rq_data) + 1;

	/* send the message and wait for the response */
	if (error = 
            rsc(out_port, out_bp, msz, &in_bp, (sndd_t)NULL, (struct uio *)NULL))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	error = msg_in->rp_errno;
	type = msg_in->rp_type;
	freemsg(in_bp);
	if (error == ENOMEM && type == NACK_MSG && retry++ < RFS_RETRY) {
		DUPRINT2(DB_SYSCALL, "du_unlink: retry %d\n", retry);
		goto resend;
	}
out:
	DUPRINT2(DB_SYSCALL, "du_unlink: result %d\n", error);
        out_port->sd_refcnt--;
        return (error);
}


int
du_statfs(name, out_port, ret_sbp)
char *name; sndd_t out_port;
struct rfs_statfs *ret_sbp;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	struct rfs_statfs *rsbp, rsb;
	int msz;
	int error = 0;
	int retry = 0;
	int type;

	DUPRINT3(DB_SYSCALL, "du_statfs: name %s, sd %x\n", name, out_port);

        out_port->sd_refcnt++;  /* rumount will not succeed if cnt > 1 */
resend:
	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_uid = u.u_uid;
	msg_out->rq_gid = u.u_gid;
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	msg_out->rq_cmask = DEF_CMASK;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_rrdir = 0;

	msg_out->rq_opcode = DUSTATFS;
	(void) strcpy(msg_out->rq_data, name);
	msg_out->rq_bufptr = (long) &rsb;
	msg_out->rq_len = sizeof(rsb);
	msg_out->rq_fstyp = 0;
	msz = sizeof(struct request) - DATASIZE + strlen(msg_out->rq_data) + 1;

	/* send the message and wait for the response */
	if (error = 
            rsc(out_port, out_bp, msz, &in_bp,(sndd_t)NULL, (struct uio *)NULL))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	rsbp = (struct rfs_statfs *) msg_in->rp_data;
	error = msg_in->rp_errno;
	type = msg_in->rp_type;

	if (error == ENOMEM && type == NACK_MSG && retry++ < RFS_RETRY) {
		DUPRINT2(DB_SYSCALL, "du_statfs: retry %d\n", retry);
		freemsg(in_bp);
		goto resend;
	} else if (error) {
		freemsg(in_bp);
		goto out;
	}
	bcopy((caddr_t) rsbp, (caddr_t) ret_sbp, 
				(u_int) sizeof(struct rfs_statfs));
	freemsg(in_bp);
out:
	DUPRINT5(DB_SYSCALL, "du_statfs: errno %d, bsize %d, free blocks %d, free nodes %d\n", 
	   error, error ? 0: ret_sbp->f_bsize, error ? 0 : ret_sbp->f_bfree, 
		error ? 0 : ret_sbp->f_ffree);
        out_port->sd_refcnt--;
        return (error);
}


int
du_update(out_port)
sndd_t out_port;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	int retry = 0;
	int type;

	DUPRINT2(DB_SYSCALL, "du_update: sd %x\n", out_port);

        out_port->sd_refcnt++;  /* rumount will not succeed if cnt > 1 */
resend:
	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_uid = (long) ((unsigned) u.u_uid);
	msg_out->rq_gid = (long) ((unsigned) u.u_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	msg_out->rq_cmask = DEF_CMASK;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_rrdir = 0;

	msg_out->rq_opcode = DUUPDATE;
	msz = sizeof(struct request) - DATASIZE;

	/* send the message and wait for the response */
	if (error = 
            rsc(out_port, out_bp, msz, &in_bp,(sndd_t)NULL, (struct uio *)NULL))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	error = msg_in->rp_errno;
	type = msg_in->rp_type;
	freemsg(in_bp);
	if (error == ENOMEM && type == NACK_MSG && retry++ < RFS_RETRY) {
		DUPRINT2(DB_SYSCALL, "du_update: retry %d\n", retry);
		goto resend;
	}
out:
	DUPRINT2(DB_SYSCALL, "du_update: result %d\n", error);
        out_port->sd_refcnt--;
        return (error);
}


int
du_open(name, out_port, cred, mode, crtmode, gift, nip)
char *name;
sndd_t out_port;
struct ucred *cred;
u_short mode, crtmode;
sndd_t *gift;
struct nodeinfo *nip;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	int retry = 0;
	int type;
	int cacheflg, stat;

	DUPRINT5(DB_SYSCALL, "du_open: name %s, sd %x, mode %x, crtmode %x\n",
		 name, out_port, mode, crtmode);

        out_port->sd_refcnt++;  /* rumount will not succeed if cnt > 1 */
resend:
	/* Contains sd for remote inode of file on syscall return */
        if ((*gift = cr_sndd()) == (sndd_t) NULL)
		return(ENOMEM);

	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	msg_out->rq_cmask = DEF_CMASK;
	msg_out->rq_rrdir = 0;

	msg_out->rq_opcode = DUOPEN;
        msg_out->rq_fmode = (long) ((unsigned) mode);
        msg_out->rq_crtmode = (long) ((unsigned) crtmode);
	(void) strcpy(msg_out->rq_data, name);
	msz = sizeof(struct request) - DATASIZE + strlen(msg_out->rq_data) + 1;

	/* send the message and wait for the response */
	if (error = 
	    rsc(out_port, out_bp, msz, &in_bp, *gift, (struct uio *)NULL)) {
		goto out;
	}

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	error = msg_in->rp_errno;
	type = msg_in->rp_type;
	if (error == ENOMEM && type == NACK_MSG && retry++ < RFS_RETRY) {
		DUPRINT2(DB_SYSCALL, "du_open: retry %d\n", retry);
		freemsg(in_bp);
		free_sndd(*gift);
		*gift = (sndd_t) NULL;
		goto resend;
	} else if (error) {
		freemsg(in_bp);
		goto out;
	}
        nip->rni_uid = gluid(GDP((*gift)->sd_queue),(ushort)msg_in->rp_uid);
        nip->rni_gid = glgid(GDP((*gift)->sd_queue),(ushort)msg_in->rp_gid);
        nip->rni_nlink = msg_in->rp_nlink;
        nip->rni_size = msg_in->rp_size;
        nip->rni_ftype = msg_in->rp_ftype;
	(*gift)->sd_mntindx = msg_in->rp_mntindx;
	(*gift)->sd_fhandle = msg_in->rp_fhandle;
	(*gift)->sd_vcode = msg_in->rp_rval;
	cacheflg = msg_in->rp_cache;
	stat = ((struct message *) (in_bp->b_rptr))->m_stat;
	if ((cacheflg & RP_MNDLCK) && (stat & VER1))
		(*gift)->sd_stat |= SDMNDLCK;
	if (cacheflg & CACHE_ENABLE)
		(*gift)->sd_stat |= SDCACHE;
	freemsg(in_bp);
out:
	DUPRINT2(DB_SYSCALL, "du_open: result %d\n", error);
	if (error) {
		free_sndd(*gift);
		*gift = (sndd_t) NULL;
	}
        out_port->sd_refcnt--;
        return (error);
}


int
du_mknod(name, out_port, cred, fmode, dev)
char *name;
sndd_t out_port;
struct ucred *cred;
u_short fmode;
dev_t dev;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	int retry = 0;
	int type;

	DUPRINT6(DB_SYSCALL, "du_mknod: name %s, sd %x, fmode %o, dev %x, uid %d\n",
		 name, out_port, fmode, dev, cred->cr_uid);

        out_port->sd_refcnt++;  /* rumount will not succeed if cnt > 1 */
resend:
	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	msg_out->rq_cmask = DEF_CMASK;
	msg_out->rq_rrdir = 0;

	msg_out->rq_opcode = DUMKNOD;
        msg_out->rq_fmode = (long) ((unsigned) fmode);
        msg_out->rq_dev = (long) ((unsigned) dev);

	(void) strcpy(msg_out->rq_data, name);
	msz = sizeof(struct request) - DATASIZE + strlen(msg_out->rq_data) + 1;

	/* send the message and wait for the response */
	if (error = 
	    rsc(out_port, out_bp, msz, &in_bp,(sndd_t) NULL, (struct uio *)NULL))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	error = msg_in->rp_errno;
	type = msg_in->rp_type;
	freemsg(in_bp);
	if (error == ENOMEM && type == NACK_MSG && retry++ < RFS_RETRY) {
		DUPRINT2(DB_SYSCALL, "du_mknod: retry %d\n", retry);
		goto resend;
	}
out:
	DUPRINT2(DB_SYSCALL, "du_mknod: result %d\n", error);
        out_port->sd_refcnt--;
        return (error);
}


int
du_close(out_port, flag, count, cred)
sndd_t out_port;
int flag;
int count;
struct ucred *cred;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	int retry = 0;
	int type;

	DUPRINT3(DB_SYSCALL, "du_close: sd %x, flag %x\n", out_port, flag);

        out_port->sd_refcnt++;  /* rumount will not succeed if cnt > 1 */
resend:
	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	msg_out->rq_cmask = DEF_CMASK;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_rrdir = 0;

	msg_out->rq_opcode = DUCLOSE;
        msg_out->rq_fmode = flag;
        msg_out->rq_foffset = 0;
        msg_out->rq_count  = count;	/*  as parameters */

	msz = sizeof(struct request) - DATASIZE;

	/* send the message and wait for the response */
	if (error = 
            rsc(out_port, out_bp, msz, &in_bp, (sndd_t)NULL, (struct uio *)NULL))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	error = msg_in->rp_errno;
	type = msg_in->rp_type;
	if (error == ENOMEM && type == NACK_MSG) {
		DUPRINT2(DB_SYSCALL, "du_close: retry %d\n", retry);
		freemsg(in_bp);
		goto resend;
	}
	out_port->sd_vcode = msg_in->rp_rval;
	freemsg(in_bp);
out:
	DUPRINT2(DB_SYSCALL, "du_close: result %d\n", error);
        out_port->sd_refcnt--;
        return (error);
}


/*
 * If both times are -1 pass a NULL time struct to server. Server then
 * sets time from it's own local time.
 */
int
du_utime(name, out_port, cred, atime, mtime)
char *name;
sndd_t out_port;
struct ucred *cred;
struct timeval *atime, *mtime;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	sndd_t gift;
	time_t times[2];
	struct uio auio, *uiop;
	struct iovec aiov;
	int retry = 0;
	int type;

	DUPRINT3(DB_SYSCALL, "du_utime: name %s, sd %x\n", name, out_port); 
	out_port->sd_refcnt++;  /* rumount will not succeed if cnt > 1 */ 
resend:
	/* Temporary sd used by rsc(), to DUCOPYIN args to server */ 
	if ((gift = cr_sndd()) == (sndd_t) NULL) 
		return(ENOMEM); 

	/* Allocate a buffer for the remote system call and fill it in */ 
	out_bp = alocbuf(sizeof (struct request), BPRI_LO); 
	if (out_bp == (mblk_t *) NULL) { 
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	msg_out->rq_cmask = DEF_CMASK;
	msg_out->rq_rrdir = 0;

	msg_out->rq_opcode = DUUTIME;
	msg_out->rq_prewrite = 0;

	(void) strcpy(msg_out->rq_data, name);
	msz = sizeof(struct request) - DATASIZE + strlen(msg_out->rq_data) + 1;

	/* Setup uio struct so server can DUCOPYIN the time arguments */
	if (atime->tv_sec == -1 && mtime->tv_sec == -1) {
		msg_out->rq_bufptr = NULL;
		uiop = NULL;
	} else {
		times[0] = (time_t) atime->tv_sec;
		times[1] = (time_t) mtime->tv_sec;
		msg_out->rq_bufptr = (long) times;
		aiov.iov_base = (caddr_t) times;
		aiov.iov_len = sizeof(times);
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = 0;
		auio.uio_seg = UIOSEG_KERNEL;
		auio.uio_fmode = 0;
		auio.uio_resid = sizeof(times);
		uiop = &auio;
	}

	/* send the message and wait for the response */
	if (error = rsc(out_port, out_bp, msz, &in_bp, gift, uiop))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	error = msg_in->rp_errno;
	type = msg_in->rp_type;
	freemsg(in_bp);
	if (error == ENOMEM && type == NACK_MSG && retry++ < RFS_RETRY) {
		DUPRINT2(DB_SYSCALL, "du_utime: retry %d\n", retry);
		free_sndd(gift);
		goto resend;
	}
out:
	DUPRINT2(DB_SYSCALL, "du_utime: result %d\n", error);
	free_sndd(gift);
        out_port->sd_refcnt--;
        return (error);
}


int
du_fstat(out_port, cred, ret_sbp)
sndd_t out_port;
struct ucred *cred;
struct rfs_stat *ret_sbp;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	struct rfs_stat *rsbp;
	int msz;
	int error = 0;
	int type;
	int retry = 0;

	DUPRINT2(DB_SYSCALL, "du_fstat: sd %x\n", out_port);
resend:
	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL)
		return(EINTR);
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	if (out_port->sd_stat & SDMNDLCK ) 
		msg_out->rq_flags |= RQ_MNDLCK;
	else
		msg_out->rq_flags &= ~RQ_MNDLCK;

	msg_out->rq_opcode = DUFSTAT;
	msg_out->rq_bufptr = (long) ret_sbp;
	msz = sizeof(struct request) - DATASIZE;

	/* send the message and wait for the response */
	if (error = 
            rsc(out_port, out_bp, msz, &in_bp,(sndd_t)NULL, (struct uio *)NULL))
		return(error);

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	rsbp = (struct rfs_stat *) msg_in->rp_data;
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	error = msg_in->rp_errno;
	type = msg_in->rp_type;

	if (!error)
		bcopy((caddr_t) rsbp, (caddr_t) ret_sbp, 
			(u_int) sizeof(struct rfs_stat));
	/* Retry if server NACK'ED */
	if (error == ENOMEM && type == NACK_MSG) {
		if (msg_in->rp_cache & RP_MNDLCK)
			out_port->sd_stat |= SDMNDLCK;
		if (retry++ < RFS_RETRY) {
			DUPRINT2(DB_SYSCALL, "du_fstat: retry %d\n", retry);
			freemsg(in_bp);
			goto resend;
		}
	}
	DUPRINT6(DB_SYSCALL, "du_fstat: errno %d, mode %o, dev %x, size %d, uid %d\n", 
	   error, error ? 0 : rsbp->st_mode, error ? 0 : rsbp->st_dev, 
		  error ? 0 : rsbp->st_size, error ? 0 : rsbp->st_uid);
	freemsg(in_bp);
        return (error);
}

/* RFS ioctl system call 
 * 	Note that this routine assumes the ioctl command "com" is
 * in a form consistent with the Sun ioctl command semantics and that
 * it sends the command over the wire exactly as received. Thus AT&T
 * ioctl commands will not work, because the numbering is different. To
 * solve this problem would require agreement on a set of over-the-wire 
 * ioctl commands, thus including these commands in the RFS protocol spec.
 * 	Note also that this command will not work between heterogeneous
 * machines, since this would require the data to be serialized/de-serialized
 * using XDR. This can't be done without specifying the contents of the data.
 * 	Both these problems reduce to specification of ioctl commands for RFS, 
 * which can't be done in general since the commands passed in ioctl() are 
 * device-specific.
 */

int
du_ioctl(out_port, com, data, flag, cred)
sndd_t out_port;
int com;
caddr_t data;
int flag;
struct ucred *cred;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	sndd_t gift;
	struct uio auio;
	struct uio *uiop = (struct uio *) NULL;
	struct iovec aiov;
	int retry = 0;

	DUPRINT5(DB_SYSCALL, "du_ioctl: sd %x, com %x, flag %x, data %x\n", 
		out_port, com, flag, data);

	/* Can only do ioctl between homogeneous machines, since you
	don't know what data elements are in the ioctl args */
        if (GDP(out_port->sd_queue)->hetero != NO_CONV)
		return(EOPNOTSUPP);
resend:
	/* Temporary sd used by rsc(), to DUCOPYIN args to server */
        if ((gift = cr_sndd()) == (sndd_t) NULL)
		return(ENOMEM);

	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	if (out_port->sd_stat & SDMNDLCK ) 
		msg_out->rq_flags |= RQ_MNDLCK;
	else
		msg_out->rq_flags &= ~RQ_MNDLCK;

	msg_out->rq_opcode = DUIOCTL;
        msg_out->rq_cmd = com;
        msg_out->rq_ioarg = (long) data;
        msg_out->rq_fflag = flag;
	msz = sizeof(struct request) - DATASIZE;

	/* Setup uio struct so server can DUCOPYIN/DUCOPYOUT ioctl data */
	if (com & _IOC_INOUT) {
		aiov.iov_base = data; 
		aiov.iov_len = (com &~ (_IOC_INOUT|_IOC_VOID)) >> 16; 
		auio.uio_iov = &aiov; 
		auio.uio_iovcnt = 1;
		auio.uio_offset = 0;
		auio.uio_seg = UIOSEG_KERNEL;
		auio.uio_fmode = 0;
		auio.uio_resid = aiov.iov_len;
		uiop = &auio;
	}

	/* send the message and wait for the response */
	if (error = rsc(out_port, out_bp, msz, &in_bp, gift, uiop))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	error = msg_in->rp_errno;
	/* Retry if server NACK'ED */
	if (error == ENOMEM && msg_in->rp_type == NACK_MSG) {
		if (msg_in->rp_cache & RP_MNDLCK)
			out_port->sd_stat |= SDMNDLCK;
		if (retry++ < RFS_RETRY) {
			DUPRINT2(DB_SYSCALL, "duioctl: retry %d\n", retry);
			freemsg(in_bp);
			free_sndd(gift);
			goto resend;
		}
	}
	freemsg(in_bp);
out:
	DUPRINT2(DB_SYSCALL, "du_ioctl: results %d\n", error);
	free_sndd(gift);
        return (error);
}


/* 
 * RFS getdents() system call -- gets directory entries in SVR3 file-system
 * independent format. The entries are moved as designated by the uio
 * structure. uio_resid and iov_len contain the amount of space available for
 * reply. To be safe these values should be greater than reqsz, the number of
 * bytes of data requested. uio->uio_offset returns an opaque pointer to the 
 * next entry as a side effect.
 */

int
du_getdents(out_port, uio, reqsz, cred)
sndd_t out_port;
struct uio *uio;
int reqsz;
struct ucred *cred;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	sndd_t gift;
	int old_offset;
	int retry = 0;
	struct uio suio;
	struct iovec siov[IOVSIZE];

	DUPRINT2(DB_SYSCALL, "du_getdents: sd %x\n", out_port);
	uio_save(uio, &suio, siov);
resend:
	uio_restore(uio, &suio, siov);

	/* Temporary sd used by rsc(), to DUCOPYOUT data from server */
        if ((gift = cr_sndd()) == (sndd_t) NULL)
		return(ENOMEM);
	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	if (out_port->sd_stat & SDMNDLCK ) 
		msg_out->rq_flags |= RQ_MNDLCK;
	else
		msg_out->rq_flags &= ~RQ_MNDLCK;

	msg_out->rq_opcode = DUGETDENTS;
        msg_out->rq_offset = uio->uio_offset;
        msg_out->rq_base = (int) uio->uio_iov->iov_base;
        msg_out->rq_count = reqsz;
	msz = sizeof(struct request) - DATASIZE;
        old_offset = uio->uio_offset;	/* Save offset for restore if error */

	DUPRINT3(DB_SYSCALL, "du_getdents: rq_count %d, offset %d\n",
		msg_out->rq_count, msg_out->rq_offset);

	/* send the message and wait for the response */
	if (error = rsc(out_port, out_bp, msz, &in_bp, gift, uio))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 

	DUPRINT5(DB_SYSCALL,"du_getdents: rslt %d, val %d, count %d, off %d\n", 
	  	msg_in->rp_errno, msg_in->rp_rval, msg_in->rp_count, 
		msg_in->rp_offset);
	error = msg_in->rp_errno;
	if (error || (msg_in->rp_rval == 0))
        	uio->uio_offset = old_offset;
	else
		uio->uio_offset = msg_in->rp_offset;

	/* Retry if server NACK'ED */
	if (error == ENOMEM && msg_in->rp_type == NACK_MSG) {
		if (msg_in->rp_cache & RP_MNDLCK)
			out_port->sd_stat |= SDMNDLCK;
		if (retry++ < RFS_RETRY) {
			DUPRINT2(DB_SYSCALL, "dugetdents: retry %d\n", retry);
			freemsg(in_bp);
			free_sndd(gift);
			goto resend;
		}
	}
	freemsg(in_bp);
out:
	free_sndd(gift);
        return (error);
}



int
du_write(out_port, uio, flag, cred)
sndd_t out_port;
struct uio *uio;
int flag;
struct ucred *cred;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	sndd_t gift;
	int retry = 0;
	int req = 0;
	struct uio suio;
	struct iovec siov[IOVSIZE];

	DUPRINT4(DB_SYSCALL, "du_write: sd %x, flag %x, count %d\n",
		out_port, flag, uio->uio_resid);
	uio_save(uio, &suio, siov);
resend:
	uio_restore(uio, &suio, siov);

	/* Temporary sd used by rsc(), to DUCOPYIN data to server */
        if ((gift = cr_sndd()) == (sndd_t) NULL)
		return(ENOMEM);

	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	if (out_port->sd_stat & SDMNDLCK ) 
		msg_out->rq_flags |= RQ_MNDLCK;
	else
		msg_out->rq_flags &= ~RQ_MNDLCK;

	msg_out->rq_opcode = DUWRITE;
        msg_out->rq_offset = uio->uio_offset;
        msg_out->rq_base = (int) uio->uio_iov->iov_base;
        msg_out->rq_count = req = uio->uio_resid;
	msg_out->rq_fmode = flag;
	msz = sizeof(struct request) - DATASIZE;

	/* send the message and wait for the response */
	if (error = rsc(out_port, out_bp, msz, &in_bp, gift, uio))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	DUPRINT3(DB_SYSCALL, "du_write: results %d, val %d\n", 
			msg_in->rp_errno, msg_in->rp_rval);

	error = msg_in->rp_errno;
	/* Retry if server NACK'ED */
	if (error == ENOMEM && msg_in->rp_type == NACK_MSG) {
		if (msg_in->rp_cache & RP_MNDLCK)
			out_port->sd_stat |= SDMNDLCK;
		if (retry++ < RFS_RETRY) {
			DUPRINT2(DB_SYSCALL, "duwrite: retry %d\n", retry);
			freemsg(in_bp);
			free_sndd(gift);
			goto resend;
		}
	}
	/* In append mode server returns file size = offset for appending */
        if (!error && (flag & RFS_FAPPEND))
		uio->uio_offset = msg_in->rp_isize + req - msg_in->rp_rval;

	freemsg(in_bp);
out:
	free_sndd(gift);
        return (error);
}


int
du_read(out_port, uio, flag, cred)
sndd_t out_port;
struct uio *uio;
int flag;
struct ucred *cred;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	sndd_t gift;
	int retry = 0;
	struct uio suio;
	struct iovec siov[IOVSIZE];

	DUPRINT4(DB_SYSCALL, "du_read: sd %x, flag %x, count %d\n", 
		out_port, flag, uio->uio_resid);
	uio_save(uio, &suio, siov);
resend:
	uio_restore(uio, &suio, siov);

	/* Temporary sd used by rsc(), to DUCOPYIN data to server */
        if ((gift = cr_sndd()) == (sndd_t) NULL)
		return(ENOMEM);

	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	if (out_port->sd_stat & SDMNDLCK ) 
		msg_out->rq_flags |= RQ_MNDLCK;
	else
		msg_out->rq_flags &= ~RQ_MNDLCK;

	msg_out->rq_opcode = DUREAD;
        msg_out->rq_offset = uio->uio_offset;
        msg_out->rq_base = (int) uio->uio_iov->iov_base;
        msg_out->rq_count = uio->uio_resid;
	msg_out->rq_fmode = flag;
	msz = sizeof(struct request) - DATASIZE;

	/* send the message and wait for the response */
	if (error = rsc(out_port, out_bp, msz, &in_bp, gift, uio))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	DUPRINT3(DB_SYSCALL, "du_read: results %d, val %d\n", 
			msg_in->rp_errno, msg_in->rp_rval);
	error = msg_in->rp_errno;
	/* Retry if server NACK'ED */
	if (error == ENOMEM && msg_in->rp_type == NACK_MSG) {
		if (msg_in->rp_cache & RP_MNDLCK)
			out_port->sd_stat |= SDMNDLCK;
		if (retry++ < RFS_RETRY) {
			DUPRINT2(DB_SYSCALL, "duread: retry %d\n", retry);
			freemsg(in_bp);
			free_sndd(gift);
			goto resend;
		}
	}
	/* Turn cache back on if got go-ahead */
	if (msg_in->rp_cache & (CACHE_ENABLE | CACHE_REENABLE))
		out_port->sd_stat |= SDCACHE;
	freemsg(in_bp);
out:
	free_sndd(gift);
        return (error);
}

/* RFS remote mount system call. In this case "name" is the name of the
 * advertised resource, rather than a file name. qp is the circuit 
 * descriptor for connecting to the machine containing the resource. 
 * The sd for the remote root is returned in gift.
 */

du_mount(name, out_port, cred, qp, rmp, flags, gift, nip)
char *name;
sndd_t out_port;
struct ucred *cred;
struct  queue *qp;
struct rfsmnt *rmp;
int flags;
sndd_t *gift;
struct nodeinfo *nip;
{

	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	extern char *nameptr();
	extern  struct timeval  time;
	int retry = 0;
	int type;

	DUPRINT3(DB_SYSCALL, "du_mount: name %s, sd %x,\n", name, out_port);
resend:
	/* Contains sd for remote inode of root on syscall return */
        if ((*gift = cr_sndd()) == (sndd_t) NULL)
                return(ENOMEM);

	/* Fill in arguments for remote mount system call and make the call */
	if ((out_bp = alocbuf(sizeof (struct request), BPRI_LO)) == NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_opcode = DUSRMOUNT;
	error = tcanon(rfs_string, name, msg_out->rq_data);

	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_flag = flags;
	msg_out->rq_mntindx = rmp - rfsmount;
	msg_out->rq_sysid = GDP(qp)->sysid;
	/* Synchronize time for first mount on this circuit */
	if (!GDP(qp)->mntcnt)
		msg_out->rq_synctime = time.tv_sec;
	msz = sizeof(struct request);

	/* send the message and wait for the response */
	if (error = 
            rsc(out_port, out_bp, msz, &in_bp, *gift, (struct uio *)NULL))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	error = msg_in->rp_errno;
	type = msg_in->rp_type;
	if (error == ENOMEM && type == NACK_MSG && retry++ < RFS_RETRY) {
		DUPRINT2(DB_SYSCALL, "du_mount: retry %d\n", retry);
		freemsg(in_bp);
                free_sndd(*gift);
		*gift = (sndd_t) NULL;
		goto resend;
	} else if (error) {
		freemsg(in_bp);
		goto out;
	}
        nip->rni_uid = gluid(GDP((*gift)->sd_queue),(ushort)msg_in->rp_uid);
        nip->rni_gid = glgid(GDP((*gift)->sd_queue),(ushort)msg_in->rp_gid);
        nip->rni_nlink = msg_in->rp_nlink;
        nip->rni_size = msg_in->rp_size;
        nip->rni_ftype = msg_in->rp_ftype;
	if (msg_in->rp_rval & MCACHE)
		rmp->rm_flags |= MCACHE;

	if (!GDP(qp)->mntcnt) {   /* Only done on first mount on a circuit */
	    hibyte(GDP(qp)->sysid) = lobyte(loword(msg_in->rp_sysid));
	    DUPRINT2(DB_MNT_ADV,"rmount: set sysid to %d\n",GDP(qp)->sysid);
	}
	(*gift)->sd_mntindx = msg_in->rp_mntindx;
	(*gift)->sd_fhandle = msg_in->rp_fhandle;

	freemsg(in_bp);
out:
	DUPRINT2(DB_SYSCALL, "du_mount: result %d\n", error);
        if (error) {
                free_sndd(*gift);
		*gift = (sndd_t) NULL;
	}
        return (error);
}

/* Remote unmount system call. If the call succeeds, the remote inode referred
 * to by the "out_port" has been freed on return. Thus the caller should free
 * the "out_port" if the call is successful.
 */

du_unmount(out_port, cred)
sndd_t out_port;
struct ucred *cred;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	int retry = 0;
	int type;

resend:
	/* Fill in arguments and make remote unmount system call */
	out_bp = alocbuf(sizeof (struct request)-DATASIZE, BPRI_LO);
	if (out_bp == NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_opcode = DUSRUMOUNT;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msz =  sizeof(struct request) - DATASIZE;

	if (error = 
	    rsc (out_port,out_bp, msz, &in_bp, (sndd_t)NULL, (struct uio *)NULL))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	error = msg_in->rp_errno;
	type = msg_in->rp_type;
	freemsg(in_bp);
	if (error == ENOMEM && type == NACK_MSG && retry++ < RFS_RETRY) {
		DUPRINT2(DB_SYSCALL, "du_unmount: retry %d\n", retry);
		goto resend;
	}
out:
	DUPRINT2(DB_SYSCALL, "du_unmount: result %d\n", error);
        return (error);
}

/* remote iput() system call. Causes the server to do an iput on the inode
 * referred to by "out_port".
 */
du_iput(out_port, cred)
sndd_t out_port;
struct ucred *cred;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	int resp_type;
	extern mblk_t *rfs_getbuf();

	DUPRINT2(DB_SYSCALL, "du_iput: sd %x,\n", out_port);
resend:
	/* Fill in arguments and make the call */
        out_bp = rfs_getbuf(sizeof(struct request)-DATASIZE, BPRI_MED);

        msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
        msg_out->rq_type = REQ_MSG;
        msg_out->rq_sysid = get_sysid(out_port);
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
        msg_out->rq_opcode = DUIPUT;
        msg_out->rq_mntindx = out_port->sd_mntindx;
	msz = sizeof (struct request) - DATASIZE;
        if (error = 
         rsc(out_port,out_bp, msz, &in_bp, (sndd_t)NULL, (struct uio *)NULL))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	error = msg_in->rp_errno;
	resp_type = msg_in->rp_type;
        freemsg (in_bp);
	if (error == ENOMEM && resp_type == NACK_MSG)
		goto resend;
out:
	DUPRINT2(DB_SYSCALL, "du_iput: result %d\n", error);
        return (error);
}


int
du_fcntl(out_port, com, ld, cred)
sndd_t out_port;
int com;
struct flock *ld;
struct ucred *cred;
{
	mblk_t	*out_bp;
	mblk_t *in_bp;
	register struct	request	*msg_out;
	struct	response  *msg_in;
	int msz;
	int error = 0;
	sndd_t gift;
	struct uio auio;
	struct uio *uiop = (struct uio *) NULL;
	struct iovec aiov;
	int size;
	int retry = 0;

	DUPRINT4(DB_SYSCALL, "du_fcntl: sd %x, com %x, ld %x\n", 
		out_port, com, ld);
resend:
	/* Temporary sd used by rsc(), to DUCOPYIN args to server */
        if ((gift = cr_sndd()) == (sndd_t) NULL)
		return(ENOMEM);

	/* Allocate a buffer for the remote system call and fill it in */
	out_bp = alocbuf(sizeof (struct request), BPRI_LO);
	if (out_bp == (mblk_t *) NULL) {
		error = EINTR;
		goto out;
	}
	msg_out = (struct request *) PTOMSG(out_bp->b_rptr);
	msg_out->rq_type = REQ_MSG;
	msg_out->rq_sysid = get_sysid(out_port); 
	msg_out->rq_mntindx = out_port->sd_mntindx;
	msg_out->rq_uid = (long) ((unsigned) cred->cr_uid);
	msg_out->rq_gid = (long) ((unsigned) cred->cr_gid);
	msg_out->rq_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
	if (out_port->sd_stat & SDMNDLCK ) 
		msg_out->rq_flags |= RQ_MNDLCK;
	else
		msg_out->rq_flags &= ~RQ_MNDLCK;

	msg_out->rq_opcode = DUFCNTL;
        msg_out->rq_cmd = FCNTLTORFS(com);
        msg_out->rq_fcntl = (long) ld;

	/* Following two fields are not provided by the vnode interface, but
	   work associated with them has already been done above vnode layer. */
 	/* Already checked lock request against file perms, so give all perms */
        msg_out->rq_fflag = RFS_FREAD | RFS_FWRITE;
	/* Already converted lock req. re start of file, so set offset to 0. */
        msg_out->rq_foffset = 0;
	msz = sizeof(struct request) - DATASIZE;

	/* For performance (5.3.1), send reclck/F_FREESP data with request. */
	if (com == F_GETLK || com == F_SETLK || com == F_SETLKW ||
	    com == F_CHKFL || com == F_FREESP) {
		size = sizeof(struct rfs_flock);
		msg_out->rq_prewrite = size;
		bcopy((caddr_t) ld, (caddr_t) msg_out->rq_data, (u_int) size); 
		msz += size;
        	msg_out->rq_fflag |= RFS_FRCACH;
		/* Setup uio to DUCOPYOUT response (only for 5.3.0 servers) */
                aiov.iov_base = (caddr_t) ld;
                aiov.iov_len = sizeof(struct rfs_flock);
                auio.uio_iov = &aiov;
                auio.uio_iovcnt = 1;
                auio.uio_offset = 0;
                auio.uio_seg = UIOSEG_KERNEL;
                auio.uio_fmode = 0;
                auio.uio_resid = sizeof(struct rfs_flock);
                uiop = &auio;
	}
	/* send the message and wait for the response */
	if (error = rsc(out_port, out_bp, msz, &in_bp, gift, uiop))
		goto out;

	/* Unpack response */
	msg_in = (struct response *) PTOMSG(in_bp->b_rptr);
	u.u_procp->p_sig |= msg_in->rp_sig; 	/* Get server's signals */ 
	error = msg_in->rp_errno;
	/* Retry if server NACK'ED */
	if (error == ENOMEM && msg_in->rp_type == NACK_MSG) {
		if (msg_in->rp_cache & RP_MNDLCK)
			out_port->sd_stat |= SDMNDLCK;
		if (retry++ < RFS_RETRY) {
			DUPRINT2(DB_SYSCALL, "dufcntl: retry %d\n", retry);
			freemsg(in_bp);
			free_sndd(gift);
			goto resend;
		}
	}
	/* For performance (5.3.1) reclock is returned with response */ 
	if (!error) {
		if (RFSTOFCNTL(msg_in->rp_rval) == F_GETLK) {
			bcopy((caddr_t) msg_in->rp_data, (caddr_t) ld, 
				sizeof(struct rfs_flock));
		}
	}
	/* Convert from RFS structure to local */
	ld->l_pid = ((struct rfs_flock *) ld)->l_pid;

	freemsg(in_bp);
out:
	DUPRINT2(DB_SYSCALL, "du_fcntl: results %d\n", error);
	free_sndd(gift);
        return (error);
}

/* 
 * Send an interrupt message across the network. Does not wait for a reply 
 * because the reply is a natural (albeit) interrupted return from the remote
 * system call.
 */

int
du_rsignal(sdp)
sndd_t sdp;
{
	register struct message *smsg;
	register struct request *req;
	register mblk_t	*bp;
	int msz;
	int error = 0;
	extern mblk_t *rfs_getbuf();

	ASSERT(sdp);
	DUPRINT2(DB_SYSCALL, "du_rsignal: sd %x\n", sdp);
	msz = sizeof(struct request) - DATASIZE;
	bp = rfs_getbuf(msz, BPRI_MED);
	req = (struct request *) PTOMSG(bp->b_rptr);
	req->rq_type = REQ_MSG;
	req->rq_opcode = DURSIGNAL;	/* this is a interrupt message */
	req->rq_sysid = get_sysid(sdp); 
	req->rq_mntindx = sdp->sd_mntindx;
	smsg = (struct message *)bp->b_rptr;
	smsg->m_stat |= SIGNAL;
	error = sndmsg(sdp, bp, msz, (rcvd_t)NULL);
	DUPRINT2(DB_SYSCALL, "du_rsignal: result %d\n", error);
	return (error);
}
