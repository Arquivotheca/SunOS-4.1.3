/*	@(#)rfcanon.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)kern-port:nudnix/rfcanon.c	10.5" */

#include <sys/types.h>
#include <sys/param.h>
#include <rfs/rfs_misc.h>
#include <rfs/sema.h>
#include <sys/stream.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/kmem_alloc.h>
#include <rfs/message.h>
#include <rpc/rpc.h>
#include <rfs/rfs_xdr.h>
#include <rfs/comm.h>
#include <rfs/hetero.h>
#include <rfs/rdebug.h>

extern u_int xdrwrap_rfs_flock();
extern u_int xdrwrap_rfs_stat();
extern u_int xdrwrap_rfs_statfs();
extern u_int xdrwrap_rfs_ustat();
extern u_int xdrwrap_rfs_dirent();
extern u_int xdrwrap_rfs_message();
extern u_int xdrwrap_rfs_request();
extern u_int xdrwrap_rfs_string();

/*
 *	tocanon routine for RFS
 *
 *	convert all RFS header and data parts to canonical formats
 *	called by sndmsg() before passing data to protocol module
 */

rftocanon (bp, hetero)
register mblk_t	*bp;
register int hetero;
{
	register struct response *msg;
	register struct message *mp;
	register int i = 0;

	msg = (struct response *)PTOMSG(bp->b_rptr);

	DUPRINT6(DB_CANON, "rftocanon: before: (longs) %x %x %x %x %x",
	bp->b_wptr - bp->b_rptr > 256 ? ((unsigned *) msg->rp_data)[0] : 0, 
	bp->b_wptr - bp->b_rptr > 256 ? ((unsigned *) msg->rp_data)[1] : 0,
	bp->b_wptr - bp->b_rptr > 256 ? ((unsigned *) msg->rp_data)[2] : 0, 
	bp->b_wptr - bp->b_rptr > 256 ? ((unsigned *) msg->rp_data)[3] : 0,
	bp->b_wptr - bp->b_rptr > 256 ? ((unsigned *) msg->rp_data)[4] : 0);
	DUPRINT6(DB_CANON, " %x %x %x %x %x\n",
	bp->b_wptr - bp->b_rptr > 256 ? ((unsigned *) msg->rp_data)[5] : 0, 
	bp->b_wptr - bp->b_rptr > 256 ? ((unsigned *) msg->rp_data)[6] : 0,
	bp->b_wptr - bp->b_rptr > 256 ? ((unsigned *) msg->rp_data)[7] : 0, 
	bp->b_wptr - bp->b_rptr > 256 ? ((unsigned *) msg->rp_data)[8] : 0,
	bp->b_wptr - bp->b_rptr > 256 ? ((unsigned *) msg->rp_data)[9] : 0);


	if (hetero == NO_CONV)
		return;

	/*
	 *	convert RFS data portion to canonical form
	 */
	if (msg->rp_type == RESP_MSG && msg->rp_errno == 0) {
		switch (msg->rp_opcode)  {
		case DUFCNTL:
			switch (msg->rp_rval) {
			case	RFS_F_GETLK:
				i = tcanon(rfs_flock, msg->rp_data,
							msg->rp_data);
				break;
			}
			break;

		case DUFSTAT:
			i = tcanon(rfs_stat, msg->rp_data, msg->rp_data);
			break;

		case DUFSTATFS:
			i = tcanon(rfs_statfs, msg->rp_data, msg->rp_data);
			break;

		case DUGETDENTS:
			i = dentcanon((int) msg->rp_count,
					msg->rp_data, msg->rp_data);
			if (i)
				msg->rp_count = i;
			break;

		case DUSTAT:
			i = tcanon(rfs_stat, msg->rp_data, msg->rp_data);
			break;

		case DUSTATFS:
			i = tcanon(rfs_statfs, msg->rp_data, msg->rp_data);
			break;

		case DUUTSSYS:
			i = tcanon(rfs_ustat, msg->rp_data, msg->rp_data);
			break;

		default:
			i = 0;
			break;
		}
		DUPRINT6(DB_CANON, "rftocanon: after: (longs) %x %x %x %x %x",
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[0] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[1] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[2] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[3] : 0,
		bp->b_wptr-bp->b_rptr>256 ? ((unsigned *) msg->rp_data)[4] : 0);
		DUPRINT6(DB_CANON, " %x %x %x %x %x\n",
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[5] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[6] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[7] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[8] : 0,
		bp->b_wptr-bp->b_rptr>256 ? ((unsigned *) msg->rp_data)[9] : 0);
	}
	if (msg->rp_type == REQ_MSG) {	/* rp_type first in req & resp msg */
		register struct request *rmsg;

		rmsg = (struct request *)PTOMSG(bp->b_rptr);
		switch (rmsg->rq_opcode) {
		case DUFCNTL:
			if (rmsg->rq_fflag & RFS_FRCACH) {
				switch (rmsg->rq_cmd) {
				case	RFS_F_GETLK:
				case	RFS_F_SETLK:
				case	RFS_F_SETLKW:
				case	RFS_F_CHKFL:
				case	RFS_F_FREESP:
					i = tcanon(rfs_flock, msg->rp_data,
							msg->rp_data);
					break;
				default:
					break;
				}
			}
			break;
		default:
			break;
		}

		DUPRINT6(DB_CANON, "rftocanon: after: (longs) %x %x %x %x %x",
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[0] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[1] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[2] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[3] : 0,
		bp->b_wptr-bp->b_rptr>256 ? ((unsigned *) msg->rp_data)[4] : 0);
		DUPRINT6(DB_CANON, " %x %x %x %x %x\n",
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[5] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[6] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[7] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[8] : 0,
		bp->b_wptr-bp->b_rptr>256 ? ((unsigned *) msg->rp_data)[9] : 0);
	}



	/* adjust the stream write pointer due to conversion expansion */
	if (i) {
		bp->b_wptr = bp->b_rptr + sizeof (struct message)
				+ sizeof (struct response) - DATASIZE + i;
			mp = (struct message *)bp->b_rptr;
			mp->m_size = bp->b_wptr - bp->b_rptr;
	}

	if (hetero == DATA_CONV)
		return;

	/*
	 *	convert RFS communication header to canonical form
	 */
	i = tcanon(rfs_message, (caddr_t) bp->b_rptr, (caddr_t) bp->b_rptr);

	/*
	 *	convert RFS request/response common header to canonical form
	 */
	i = tcanon(rfs_request, (caddr_t) PTOMSG(bp->b_rptr),
				(caddr_t) PTOMSG(bp->b_rptr));

}

/*
 *	frcanon routine for RFS
 *
 *	convert all RFS header and data parts back to local formats
 *	called by arrmsg() before passing data to GDP module
 *	Returns 1 if successful
 *		0 if failed (lack of memory).
 */

rffrcanon (bp, hetero)
register mblk_t	*bp;
register int hetero;
{
	register struct response *msg;
	register int i;

	msg = (struct response *)PTOMSG(bp->b_rptr);

	DUPRINT6(DB_CANON, "rffrcanon: before: (longs) %x %x %x %x %x",
	bp->b_wptr - bp->b_rptr > 256 ? ((unsigned *) msg->rp_data)[0] : 0, 
	bp->b_wptr - bp->b_rptr > 256 ? ((unsigned *) msg->rp_data)[1] : 0,
	bp->b_wptr - bp->b_rptr > 256 ? ((unsigned *) msg->rp_data)[2] : 0, 
	bp->b_wptr - bp->b_rptr > 256 ? ((unsigned *) msg->rp_data)[3] : 0,
	bp->b_wptr - bp->b_rptr > 256 ? ((unsigned *) msg->rp_data)[4] : 0);

	if (hetero == NO_CONV)
		return (1);

	if (hetero == ALL_CONV) {
		/*
		 *	convert RFS communication header back to local form
		 */
		(void) fcanon(rfs_message, (caddr_t) bp->b_rptr,
				(caddr_t) bp->b_rptr);

		/*
		 * convert RFS request/response common header back to local form
		 */
		(void) fcanon(rfs_request, (caddr_t) PTOMSG(bp->b_rptr),
					(caddr_t) PTOMSG(bp->b_rptr));
	}

	/*
	 *	convert RFS data portion back to local form
	 */
	if (msg->rp_type == RESP_MSG && msg->rp_errno == 0) {
		switch (msg->rp_opcode)  {
		case DUFCNTL:
			switch (msg->rp_rval) {
			case	RFS_F_GETLK:
				i = fcanon(rfs_flock, msg->rp_data,
						msg->rp_data);
				break;
			}
			break;

		case DUFSTAT:
			i = fcanon(rfs_stat, msg->rp_data, msg->rp_data);
			break;

		case DUFSTATFS:
			i = fcanon(rfs_statfs, msg->rp_data, msg->rp_data);
			break;

		case DUGETDENTS:
			i = denfcanon((int) msg->rp_count,
					msg->rp_data, msg->rp_data);
			if (i == -1)
				return (0);
			if (i)
				msg->rp_count = i;
			break;

		case DUSTAT:
			i = fcanon(rfs_stat, msg->rp_data, msg->rp_data);
			break;

		case DUSTATFS:
			i = fcanon(rfs_statfs, msg->rp_data, msg->rp_data);
			break;

		case DUUTSSYS:
			i = fcanon(rfs_ustat, msg->rp_data, msg->rp_data);
			break;

		default:
			break;
		}

		DUPRINT6(DB_CANON, "rftocanon: before: (longs) %x %x %x %x %x",
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[0] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[1] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[2] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[3] : 0,
		bp->b_wptr-bp->b_rptr>256 ? ((unsigned *) msg->rp_data)[4] : 0);
	}
	if (msg->rp_type == REQ_MSG) {	/* rp_type first in req & resp msg */
		register struct request *rmsg;
		rmsg = (struct request *)PTOMSG(bp->b_rptr);
		switch (rmsg->rq_opcode) {
		case DUFCNTL:
			switch (rmsg->rq_cmd) {
			case	RFS_F_GETLK:
			case	RFS_F_SETLK:
			case	RFS_F_SETLKW:
			case	RFS_F_CHKFL:
			case	RFS_F_FREESP:
				i = fcanon(rfs_flock, msg->rp_data,
						msg->rp_data);
				rmsg->rq_prewrite = sizeof (struct rfs_flock);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		DUPRINT6(DB_CANON, "rffrcanon: after: (longs) %x %x %x %x %x",
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[0] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[1] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[2] : 0,
		bp->b_wptr-bp->b_rptr >256 ? ((unsigned *) msg->rp_data)[3] : 0,
		bp->b_wptr-bp->b_rptr>256 ? ((unsigned *) msg->rp_data)[4] : 0);
	}
	return (1);
}






/*This routine is written to convert directory entries to canon form for getdent
*/

dentcanon(count, from, to)
register int count;
register char *from, *to;

{
	register int tlen, tcc;
	struct rfs_dirent *dir;
	register char *tmp, *start;

	tlen = 0;
	start = new_kmem_alloc(DATASIZE, KMEM_SLEEP);
	tmp = start;

	while (count > 0){
		dir = (struct rfs_dirent *)from;
		tcc = tcanon(rfs_dirent, from, tmp);
		tcc = (tcc + 3) & ~3;
		tmp += tcc;
		tlen += tcc;
		from += dir->d_reclen;
		count -= dir->d_reclen;
	}
	bcopy(start, to, (u_int) tlen);
	kmem_free(start, DATASIZE);
	return (tlen);
}

/*
 * This routine is called to convert directory entries from canon form
 * to local form.
 *	Returns # of bytes in local form or -1 if failed (lack of memory).
 */
denfcanon(count, from, to)
register int count;
register char *from;
register char *to;
{

	register int tlen, tcc;
	struct rfs_dirent *dir;
	register char *tmp, *start;

	tlen = 0;
	start = new_kmem_alloc(DATASIZE, KMEM_NOSLEEP);
	if (start == NULL)
		return (-1);
	tmp = start;

	while (count > 0){
		tcc = fcanon(rfs_dirent, from, tmp);
		dir = (struct rfs_dirent *)tmp;
		dir->d_reclen = (((unsigned) dir->d_name - (unsigned) dir) +
			strlen(dir->d_name) + 1 + 3) & ~3;
		tmp += dir->d_reclen;
		tlen += dir->d_reclen;
		tcc = 4 * sizeof (long) + ((strlen(from + 4 * sizeof (long))
			+1 + 3) & ~3);
		from += tcc;
		count -= tcc;
	}
	bcopy(start, to, (u_int) tlen);
	kmem_free(start, DATASIZE);
	return (tlen);
}
