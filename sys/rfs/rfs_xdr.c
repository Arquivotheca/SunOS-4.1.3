/*	@(#)rfs_xdr.c 1.1 92/07/30 SMI 	*/

/*
 *	XDR routines for RFS kernel data structures.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <rpc/rpc.h>
#include <rfs/sema.h>
#include <rfs/rfs_misc.h>
#include <rfs/message.h>
#include <rfs/comm.h>
#include <rfs/rfs_xdr.h>


bool_t
xdr_char_array(xdrp, objp, len)
        XDR *xdrp;
        char *objp;
	u_int len;
{
        return( xdr_bytes(xdrp, &objp, &len, len));
}

bool_t
xdr_rfs_string(xdrp, objp)
	XDR *xdrp;
	char *objp;
{
	u_int len, maxlen;

	if (xdrp->x_op == XDR_ENCODE) {
		len = strlen(objp) + 1;
		maxlen = len;
	} else {
		maxlen = len = DATASIZE;
	}
	return( xdr_bytes(xdrp, (char **) &objp, &len, maxlen));
}

bool_t
xdr_rfs_flock(xdrs,objp)
	XDR *xdrs;
	rfs_flock *objp;
{
	if (! xdr_short(xdrs, &objp->l_type)) {
		return(FALSE);
	}
	if (! xdr_short(xdrs, &objp->l_whence)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->l_start)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->l_len)) {
		return(FALSE);
	}
	if (! xdr_short(xdrs, &objp->l_sysid)) {
		return(FALSE);
	}
	if (! xdr_short(xdrs, &objp->l_pid)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_rfs_stat(xdrs,objp)
	XDR *xdrs;
	rfs_stat *objp;
{
	if (! xdr_short(xdrs, &objp->st_dev)) {
		return(FALSE);
	}
	if (! xdr_short(xdrs, (short *) &objp->st_ino)) {
		return(FALSE);
	}
	if (! xdr_short(xdrs, (short *) &objp->st_mode)) {
		return(FALSE);
	}
	if (! xdr_short(xdrs, &objp->st_nlink)) {
		return(FALSE);
	}
	if (! xdr_short(xdrs, (short *) &objp->st_uid)) {
		return(FALSE);
	}
	if (! xdr_short(xdrs, (short *) &objp->st_gid)) {
		return(FALSE);
	}
	if (! xdr_short(xdrs, &objp->st_rdev)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->st_size)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->st_atime)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->st_mtime)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->st_ctime)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_rfs_statfs(xdrs,objp)
	XDR *xdrs;
	rfs_statfs *objp;
{
	if (! xdr_short(xdrs, &objp->f_fstyp)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->f_bsize)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->f_frsize)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->f_blocks)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->f_bfree)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->f_files)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->f_ffree)) {
		return(FALSE);
	}
	if (! xdr_char_array(xdrs, objp->f_fname, sizeof(objp->f_fname))) {
		return(FALSE);
	}
	if (! xdr_char_array(xdrs, objp->f_fpack, sizeof(objp->f_fpack))) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_rfs_ustat(xdrs,objp)
	XDR *xdrs;
	rfs_ustat *objp;
{
	if (! xdr_long(xdrs, &objp->f_tfree)) {
		return(FALSE);
	}
	if (! xdr_short(xdrs, (short *) &objp->f_tinode)) {
		return(FALSE);
	}
	if (! xdr_char_array(xdrs, objp->f_fname, sizeof(objp->f_fname))) {
		return(FALSE);
	}
	if (! xdr_char_array(xdrs, objp->f_fpack, sizeof(objp->f_fpack))) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_rfs_dirent(xdrs,objp)
	XDR *xdrs;
	rfs_dirent *objp;
{
	if (! xdr_long(xdrs, &objp->d_ino)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->d_off)) {
		return(FALSE);
	}
	if (! xdr_short(xdrs, &objp->d_reclen)) {
		return(FALSE);
	}
	if (! xdr_rfs_string(xdrs, objp->d_name)) {
		return(FALSE);
	}
	return(TRUE);
}


bool_t
xdr_rfs_utimbuf(xdrs,objp)
	XDR *xdrs;
	rfs_utimbuf *objp;
{
	if (! xdr_long(xdrs, &objp->ut_actime)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->ut_modtime)) {
		return(FALSE);
	}
	return(TRUE);
}



bool_t
xdr_rfs_message(xdrs,objp)
	XDR *xdrs;
	struct message *objp;
{
	if (! xdr_long(xdrs, &objp->m_cmd)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->m_stat)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->m_dest)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->m_connid)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->m_gindex)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->m_gconnid)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->m_size)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->m_queue)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_rfs_common(xdrs,objp)
	XDR *xdrs;
	struct common *objp;
{
	if (! xdr_long(xdrs, &objp->opcode)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->sysid)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->type)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->pid)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->uid)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->gid)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->ftype)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->nlink)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->size)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->mntindex)) {
		return(FALSE);
	}
	return(TRUE);
}

/* 
   Common routine for request/response header -- this work
   only because the data structures have identical formats.
   If they ever change, look out!
*/

bool_t
xdr_rfs_request(xdrs,objp)
	XDR *xdrs;
	struct request *objp;
{
	if (! xdr_rfs_common(xdrs, &objp->rq)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->rq_rrdir)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->rq_ulimit)) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->rq_arg[0])) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->rq_arg[1])) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->rq_arg[2])) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->rq_arg[3])) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->rq_tmp[0])) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->rq_tmp[1])) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->rq_tmp[2])) {
		return(FALSE);
	}
	if (! xdr_long(xdrs, &objp->rq_tmp[3])) {
		return(FALSE);
	}
	return(TRUE);
}
